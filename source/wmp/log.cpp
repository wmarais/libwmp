#include "../../include/wmp/log.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>

#define WMP_CONF_HAS_SYSLOG 1

#ifdef WMP_CONF_HAS_SYSLOG
#include <syslog.h>
#endif /* WMP_CONF_HAS_SYSLOG */

using namespace wmp;

/******************************************************************************/
namespace wmp
{
  std::ostream & operator << (std::ostream & os, const log_t::levels_t & lvl)
  {
    switch(lvl)
    {
      case log_t::levels_t::trace     : return os << "TRACE";
      case log_t::levels_t::debug     : return os << "DEBUG";
      case log_t::levels_t::info      : return os << "INFO ";
      case log_t::levels_t::notify    : return os << "NOTIF";
      case log_t::levels_t::warn      : return os << "WARN ";
      case log_t::levels_t::error     : return os << "ERROR";
      case log_t::levels_t::fatal     : return os << "FATAL";
      case log_t::levels_t::excep     : return os << "EXCEP";
    }

    return os << "????";
  }
}

/******************************************************************************/
log_t::log_t(): level_v(static_cast<std::int_fast8_t>(log_t::levels_t::notify)),
  executing_v(true), write_exception_v(nullptr), head_v(0), tail_v(0),
  max_count_c(WMP_CONF_MAX_LOG_QUEUE_LEN), count_v(0), messages_v(nullptr),
  thread_v(nullptr), syslog_state_v(log_t::syslog_states_t::closed)
{
  /* Set a default name in case the user does not specify something. */
  app_name_v = "WMP LOG";

  /* Create the new message queue length. */
  messages_v = std::make_unique<msg_t[]>(max_count_c);

  /* Create the new writing thread. */
  thread_v = std::make_unique<std::thread>(&log_t::write_thread, this);
}

/******************************************************************************/
log_t::~log_t()
{
  /* Stop the writing thread. */
  executing_v = false;

  /* Wait for the write thread to finish / flush. */
  if(thread_v)
    thread_v->join();
}

/******************************************************************************/
void log_t::enqueue(msg_t & msg)
{
  /* Check if any exceptions were thrown in the output thread. */
  check_exceptions();

  /* Check if the message is even to be recorded. */
  if(msg.level() < min_level())
    return;

  /* Try to push the message into the queue. */
  while(executing_v)
  {
    /* Check if any exceptions were thrown in the output thread. */
    check_exceptions();

    {
      /* Lock access to queue. */
      std::scoped_lock<std::mutex> l(mutex_v);

      /* Check if there is a free slot. */
      if(count_v < max_count_c)
      {
        /* Swap the contents of the messages. */
        messages_v[head_v] = std::move(msg);

        /* Increment the count and head position. */
        ++count_v;
        ++head_v;
        if(head_v >= max_count_c)
          head_v = 0;

        /* Break out of the loop. */
        break;
      }
    }

    /* See if any other thread needs the cpu more. */
    std::this_thread::yield();
  }

  /* Notify the writer thread there is something new to write. */
  queue_empty_v.notify_all();
}

/******************************************************************************/
void log_t::write_thread()
{
  try
  {
    while(executing_v)
    {
      /* Write the message to the log and check if the queue is empty. */
      if(!write_log_entry())
      {
        /* Wait until a new message has been written, or a time out occur. I
         * know this can cause a race condition, between the wait call and a
         * new message being concurrenlty entered, however, this is not a
         * problem because the time out will prevent the thread from locking
         * up completely and blocking the main thread. Though under heavy load
         * it could be possible that this result in a momentary hickup, but
         * given that logs should not be run inside of very fast loops, this
         * is not a real concern either. */
        std::unique_lock<std::mutex> l(queue_empty_mtx_v);
        queue_empty_v.wait_for(l, std::chrono::milliseconds(1));
      }
    }

    /* Flush the remaining messages to the log. */
    while(write_log_entry());

#ifdef WMP_CONF_HAS_SYSLOG

    /* Close down syslog if it was enabled. */
    if(syslog_state_v == syslog_states_t::open)
      closelog();

#endif /* WMP_CONF_HAS_SYSLOG */
  }
  catch (...)
  {
    /* Store the pointer to the currently thrown exception. */
    write_exception_v = std::current_exception();

    /* Mark the write thread as finished. */
    executing_v = false;
  }
}

/******************************************************************************/
bool log_t::write_log_entry()
{
  /* Lock the mutex. */
  std::scoped_lock<std::mutex> l(mutex_v);

  /* Check if there is anything to write. */
  if(count_v == 0)
    return false;

  /* Get a reference to the message to be written. */
  msg_t & msg = messages_v[tail_v];

#ifdef WMP_CONF_HAS_SYSLOG

  /* Check if the log name changed. */
  if(syslog_state_v == syslog_states_t::app_name_changed)
  {
    /* Close the log and re-open it. */
    closelog();

    /* Reopen syslog with the new name. */
    syslog_state_v = syslog_states_t::enable;
  }

  /* Check if syslog must be enabled. */
  if(syslog_state_v == syslog_states_t::enable)
  {
    /* Enable all the log fields that maps to the supported log types. */
    setlogmask(LOG_EMERG | LOG_CRIT | LOG_ERR | LOG_WARNING | LOG_NOTICE |
               LOG_INFO | LOG_DEBUG);

    /* Open the log. Note that if the name changes, this need to reopen. */
    openlog(app_name_v.c_str(), LOG_NDELAY | LOG_PID, LOG_USER);

    /* Mark syslog as open. */
    syslog_state_v = syslog_states_t::open;
  }
  /* Check if syslog must be closed. */
  else if(syslog_state_v == syslog_states_t::disable)
  {
    /* Close syslog. */
    closelog();

    /* Mark syslog as closed. */
    syslog_state_v == syslog_states_t::closed;
  }

  /* If syslog is open, then write the log message. */
  if(syslog_state_v == syslog_states_t::open)
  {
    /* Select the best matching priority / level to write too. */
    int priority = LOG_DEBUG;
    switch(msg.level())
    {
      case levels_t::excep : priority = LOG_EMERG; break;
      case levels_t::fatal : priority = LOG_CRIT; break;
      case levels_t::error : priority = LOG_ERR; break;
      case levels_t::warn : priority = LOG_WARNING; break;
      case levels_t::notify : priority = LOG_NOTICE; break;
      case levels_t::info : priority = LOG_INFO; break;
    }

    /* Write the message. */
    syslog(priority, "%s", msg.text().c_str());
  }

#endif /* WMP_CONF_HAS_SYSLOG */

  /* The string stream to build the message. */
  std::stringstream ss;
  ss << app_name_v << " | "
     << messages_v[tail_v].level() << " | "
     << std::this_thread::get_id() << " | "
     << msg.time() << " | "
     << std::filesystem::path(msg.file_name()).filename().string() << " | "
     << msg.func_name() << " | "
     << msg.line_num() << " | "
     << messages_v[tail_v].text();

  /* Write the message to all the output streams. */
  for(auto os : os_v[static_cast<std::int_fast8_t>(msg.level())])
  {
    (*os) << ss.str();
    os->flush();
  }

  /* Dequeue the log entry. */
  --count_v;
  ++tail_v;
  if(tail_v >= max_count_c)
    tail_v = 0;

  /* Check if there is anything left to write. */
  return count_v == 0;
}

/******************************************************************************/
void log_t::write_log_entry(levels_t lvl,
    const std::string & header,
                            const std::string & body,
                            const std::vector<std::ostream*> & os)
{

}

/******************************************************************************/
void log_t::app_name(const std::string & name)
{
  /* Lock access to the name field. */
  std::scoped_lock<std::mutex> l(log_t::ref().mutex_v);

  /* Set the application name. */
  log_t::ref().app_name_v = name;

  /* Tell syslog to reopen with the new name. */
  log_t::ref().syslog_state_v = syslog_states_t::app_name_changed;
}

/******************************************************************************/
void log_t::add_output_unsafe(std::ostream * os,
                              std::initializer_list<log_t::levels_t> lvls)
{
  /* If no levels were specified, then output all message levels to this
   * stream. */
  if(lvls.size() == 0)
  {
    for(auto & vec : log_t::ref().os_v)
    {
      /* Make sure it is not allready included in the list of output streams. */
      if(std::find(vec.begin(), vec.end(), os) == vec.end())
      {
        vec.push_back(os);
      }
    }
  }

  /* Else, add it to just the specified streams. */
  for(auto lvl : lvls)
  {
    auto & vec = log_t::ref().os_v[static_cast<std::int_fast8_t>(lvl)];

    if(std::find(vec.begin(), vec.end(), os) == vec.end())
    {
      vec.push_back(os);
    }
  }
}

/******************************************************************************/
void log_t::remove_output_unsafe(std::ostream * os,
                                 std::initializer_list<log_t::levels_t> lvls)
{
  /* If no levels were specified, then remove it from all output levels.*/
  if(lvls.size() == 0)
  {
    for(auto & vec : log_t::ref().os_v)
    {
      /* Find the item in the current lof level's output streams. */
      auto iter = std::find(vec.begin(), vec.end(), os);

      /* Make sure it is not allready included in the list of output streams. */
      if(iter != vec.end())
      {
        vec.erase(iter);
      }
    }
  }

  /* Else, add it to just the specified streams. */
  for(auto lvl : lvls)
  {
    auto & vec = log_t::ref().os_v[static_cast<std::int_fast8_t>(lvl)];

    /* Find the item in the current lof level's output streams. */
    auto iter = std::find(vec.begin(), vec.end(), os);

    if(iter != vec.end())
    {
      vec.erase(iter);
    }
  }
}

/******************************************************************************/
void log_t::add_output(std::ostream * os,
                       std::initializer_list<log_t::levels_t> lvls)
{
  /* Check if any exceptions were thrown in the output thread. */
  log_t::ref().check_exceptions();

  /* Lock access to the name field. */
  std::scoped_lock<std::mutex> l(log_t::ref().mutex_v);

  /* Add it as an output stream. */
  log_t::ref().add_output_unsafe(os, lvls);
}

/******************************************************************************/
bool log_t::add_output(const std::string & path,
                       std::initializer_list<log_t::levels_t> lvls,
                       bool append)
{
  /* Check if any exceptions were thrown in the output thread. */
  log_t::ref().check_exceptions();

  /* Lock access to the name field. */
  std::scoped_lock<std::mutex> l(log_t::ref().mutex_v);

  /* Check if the file is allready open. */
  if(log_t::ref().ofs_v.find(path) == log_t::ref().ofs_v.end())
  {
    /* Open the file for writing. */
     std::unique_ptr<std::ofstream> file =
         std::make_unique<std::ofstream>(path, append ? std::ios_base::app :
                                                        std::ios_base::trunc);

     /* Make sure it was opened. */
     if(!file->is_open())
       return false;

    /* Store it so it can be properly closed. */
    log_t::ref().ofs_v[path] = std::move(file);
  }

  /* Add it as an output stream. */
  log_t::ref().add_output_unsafe(log_t::ref().ofs_v[path].get(), lvls);

  /* Return true to indicate it all went well. */
  return true;
}

/******************************************************************************/
void log_t::remove_output(std::ostream * os,
                          std::initializer_list<levels_t> lvls)
{
  /* Check if any exceptions were thrown in the output thread. */
  log_t::ref().check_exceptions();

  /* Lock access to the name field. */
  std::scoped_lock<std::mutex> l(log_t::ref().mutex_v);

  /* Remove the output. */
  log_t::ref().remove_output_unsafe(os, lvls);
}

/******************************************************************************/
void log_t::remove_output(const std::string & path,
                          std::initializer_list<levels_t> lvls)
{
  /* Check if any exceptions were thrown in the output thread. */
  log_t::ref().check_exceptions();

  /* Lock access to the name field. */
  std::scoped_lock<std::mutex> l(log_t::ref().mutex_v);

  /* Find the log file to close. */
  if(log_t::ref().ofs_v.find(path) != log_t::ref().ofs_v.end())
  {
    /* Remove the output stream. */
    log_t::ref().remove_output_unsafe(log_t::ref().ofs_v[path].get(), lvls);
    log_t::ref().ofs_v.erase(log_t::ref().ofs_v.find(path));
  }
}

/******************************************************************************/
void log_t::enable_syslog()
{
  /* Check if any exceptions were thrown in the output thread. */
  log_t::ref().check_exceptions();

  /* Lock access to the name field. */
  std::scoped_lock<std::mutex> l(log_t::ref().mutex_v);

  /* Enable syslog. */
  log_t::ref().syslog_state_v = syslog_states_t::enable;
}

/******************************************************************************/
void log_t::disable_syslog()
{
  /* Check if any exceptions were thrown in the output thread. */
  log_t::ref().check_exceptions();

  /* Lock access to the name field. */
  std::scoped_lock<std::mutex> l(log_t::ref().mutex_v);

  /* Disable syslog. */
  log_t::ref().syslog_state_v = syslog_states_t::disable;
}

/******************************************************************************/
void log_t::write(msg_t & msg)
{
  /* Enqueue the message into to write queue. All locking and checks are
   * performed in the enqueue function. */
  log_t::ref().enqueue(msg);
}

/******************************************************************************/
log_t::msg_t::msg_t(levels_t lvl,  std::uint64_t time_stamp,
  std::thread::id thread_id, const char * file, const char * func,
  std::size_t line) : lvl_v(lvl), time_stamp_v(time_stamp),
  thread_id_v(thread_id), file_name_v(file), func_name_v(func),
  line_num_v(line) {}
