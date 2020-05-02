#include "../../include/wmp/log.hpp"
#include <chrono>
#include <filesystem>

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
      case log_t::levels_t::info      : return os << "INFO";
      case log_t::levels_t::notify    : return os << "NOTIFY";
      case log_t::levels_t::warn      : return os << "WARN";
      case log_t::levels_t::error     : return os << "ERROR";
      case log_t::levels_t::fatal     : return os << "FATAL";
      case log_t::levels_t::exception : return os << "EXCEPTION";
    }
  }
}

/******************************************************************************/
log_t::log_t(): level_v(static_cast<std::int_fast8_t>(log_t::levels_t::notify)),
  executing_v(true), write_exception_v(nullptr), head_v(0), tail_v(0),
  max_count_c(WMP_MAX_LOG_QUEUE_LEN), count_v(0), messages_v(nullptr),
  thread_v(nullptr)
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
void log_t::queue_msg(msg_t & msg)
{
  /* Check if any exceptions were thrown in the output thread. */
  check_exceptions();

  /* Check if the message is even to be recorded. */
  if(msg.level() < min_level())
    return;

  /* Try to push the message into the queue. */
  while(executing_v)
  {
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

  /* The string stream to build the message. */


  /* Write the message to all the output streams. */
  if(messages_v[tail_v].level() < levels_t::warn)
  {
    /* Write the normal output stream of the terminal. */
    std::cout << app_name_v << " | "
              << messages_v[tail_v].level() << " | "
              << messages_v[tail_v].text();
    std::cout.flush();
  }
  else
  {
    /* Write to the error output stream. */
    std::cerr << messages_v[tail_v].text();
  }

  /* Decrement the log entry. */
  --count_v;
  ++tail_v;
  if(tail_v >= max_count_c)
    tail_v = 0;

  /* Check if there is anything left to write. */
  return count_v == 0;
}

/******************************************************************************/
void log_t::app_name(const std::string & name)
{
  /* Lock access to the name field. */
  std::scoped_lock<std::mutex> l(log_t::ref().mutex_v);

  //std::filesystem::path(name).filename().string()

  /* Set the application name. */
  log_t::ref().app_name_v = name;
}

/******************************************************************************/
log_t::msg_t::msg_t(levels_t lvl, const char * file, const char * func,
  std::size_t line) : lvl_v(lvl), file_name_v(file), func_name_v(func),
  line_num_v(line) {}







