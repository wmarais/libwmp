#pragma once

#include <memory>
#include <atomic>
#include <algorithm>
#include <mutex>
#include <thread>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <condition_variable>

#define WMP_LOG_MSG(lvl, msg) \
  wmp::log_t::write(wmp::log_t::msg_t(lvl, __FILE__, __FUNCTION__, __LINE__) \
    << msg)

/*******************************************************************************
 * Write a TRACE message to the log. This is not to be used directly, use the
 * WMP_TRACE_FUNC() macro to trance the entry and exit of a function.
 ******************************************************************************/
#define WMP_LOG_TRACE(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::trace, msg)

/*******************************************************************************
 * Write a DEBUG message to the log. Generally debug message will be compiled
 * out for release versions of the library to aid performance.
 ******************************************************************************/
#define WMP_LOG_DEBUG(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::debug, msg)

/*******************************************************************************
 * Write a INFO message to the log.
 ******************************************************************************/
#define WMP_LOG_INFO(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::info, msg)

#define WMP_LOG_NOTIFY(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::notify, msg)

#define WMP_LOG_WARN(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::warn, msg)

#define WMP_LOG_ERROR(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::error, msg)

#define WMP_LOG_FATAL(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::fatal, msg)

#define WMP_LOG_EXCEPTION(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::exception, msg)

#define WMP_MAX_LOG_QUEUE_LEN     10000


namespace wmp
{
  class log_t final
  {
  public:

    enum class levels_t : std::int_fast8_t
    {
      /** The entry and exists of function calls are traced. */
      trace   = 0,

      /** Provide very detailed debug information only useful to developers.
       * Does not trace the entry and exist of functions. */
      debug,

      /** Provides more verbose information than notify, but not at the density
       * of debug. */
      info,

      /** The default log level. This is used to indicate significantly
       * informative information about the status of the application. I.e.
       * application starting, stopping, loading config files, etc. */
      notify,

      /** Warn the user about something that is not fatal to the execution of
       * the application, but my produce diffirent results that expected. I.e.
       * a configuration options that is being ignored etc. */
      warn,

      /** Notify the user of an non fatal error that stops a particular
       * operation from being complete. I.e. if the user attempts to open a
       * file that they do not have permission too. */
      error,

      /** Notify the user of any fatal errors that has been detected and that
       * the program must shut down for safety and security reasons. For
       * example mismatching Hash values for plugins. */
      fatal,

      /** Notify the user of any exceptions that occured which is not handled
       * by standard fatal error detection mechanisms. These are generally
       * hardware faults such as being out of memory or harddrive space. Though
       * when these things happens, its a bit of a gamble whether it still
       * works or not. */
      exception,
    };

    class msg_t final
    {
      /** The log level being recorded. */
      levels_t lvl_v;

      /** The file name in which the log message was generated. */
      std::string file_name_v;

      /** The function name in which the log message was generated. */
      std::string func_name_v;

      /** The line number onwhich the log message was generated. */
      std::size_t line_num_v;

      /** The string stream object used to build the log message. */
      std::stringstream ss_v;

    public:
      msg_t(levels_t lvl = levels_t::exception,
            const char * file = "",
            const char * func = "",
            std::size_t line = 0);

      template <typename t> msg_t & operator << (const t & val)
      {
        ss_v << val;
        return * this;
      }

      inline std::string text() const
      {
        return ss_v.str();
      }

      inline levels_t level() const
      {
        return  lvl_v;
      }
    };

  private:

    /** Delete the copy constructor to force signelton pattern. */
    log_t(const log_t &) = delete;

    /** Delete the move constructor to force singleton pattern */
    log_t(const log_t &&) = delete;

    /** Delete the copy assignment to force singleton pattern. */
    log_t & operator = (const log_t &) = delete;

    /** Delete the move assignment to force singleton pattern. */
    log_t & operator = (const log_t &&) = delete;

    /** The name of the application being logged. */
    std::string app_name_v;

    /** Track the minimum log message level to record. */
    std::atomic_int_fast8_t level_v;

    /** Track whether the write thread is still running or not. */
    std::atomic_bool executing_v;

    /** The pointer to any exception the was raised in the write thread. */
    std::exception_ptr write_exception_v;

    /** The mutex that is used for controlling access to the message queue. */
    std::mutex mutex_v;

    /** The lock object to use for make the write thread wait for new queue
     * contents rather than sitting in a spin lock. */
    std::mutex queue_empty_mtx_v;

    /** The conditional variable to block / wake up the write thread when the
     * message queue is empty / new data is entered respectively. */
    std::condition_variable queue_empty_v;

    /** The head pointer of the queue. */
    std::size_t head_v;

    /** The tail pointer of the queue. */
    std::size_t tail_v;

    /** The maximum number of messages in the queue. */
    const std::size_t max_count_c;

    /** The number of messages in the queue. */
    std::size_t count_v;

    /** The messages stored in the queue. */
    std::unique_ptr<msg_t[]> messages_v;

    /** The thread used for writting message to the output. */
    std::unique_ptr<std::thread> thread_v;

    log_t();
    ~log_t();

    static log_t & ref()
    {
      static log_t instance_v;

      /* Return the static instance of the log. */
      return instance_v;
    }

    void write_thread();

    bool write_log_entry();

    /***************************************************************************
     *
     *
     **************************************************************************/
    void queue_msg(msg_t & msg);


  public:



    /***************************************************************************
     * Rethrow any exceptions that were thrown in the write thread.
     *
     **************************************************************************/
    void check_exceptions() const
    {
      /* If execution has finished, then the the write thread has likely
       * prematurely died. */
      if(!executing_v && write_exception_v)
        std::rethrow_exception(write_exception_v);
    }

    /***************************************************************************
     * Set the minimum level (inclusive) of message to be logged. To set only
     * messages of error or higher severity to be logged, invoke:
     *
     * @code{.cpp}
     * log::ptr()->min_level(log_t::levels_t::error);
     * @endcode
     *
     * @param[in] lvl The minimum log level of messages to be logged.
     **************************************************************************/
    static void min_level(levels_t lvl)
    {
      /* Check if any exceptions were thrown in the output thread. */
      log_t::ref().check_exceptions();

      /* Set the minimum message level to record. */
      log_t::ref().level_v = static_cast<std::int_fast8_t>(lvl);
    }

    /***************************************************************************
     * Get the minimum level (inclusive) of message to be logged.
     **************************************************************************/
    static levels_t min_level()
    {
      /* Check if any exceptions were thrown in the output thread. */
      log_t::ref().check_exceptions();

      /* Retrieve the minimum message level to log. */
      return static_cast<levels_t>(log_t::ref().level_v.load());
    }

    /***************************************************************************
     * Set the name of the application be logged.
     **************************************************************************/
    static void app_name(const std::string & name);

    static void write(msg_t & msg)
    {
      log_t::ref().queue_msg(msg);
    }
  };

  std::ostream & operator << (std::ostream & os, const log_t::levels_t & lvl);
}
