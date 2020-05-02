/*******************************************************************************
 * @file        log.hpp
 * @author      Wynand Marais
 * @copyright   MIT License
 ******************************************************************************/

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
#include <map>
#include <condition_variable>
#include <array>

#include "config.hpp"

/*******************************************************************************
 * Creates a message object with all the relevant debug information contained
 * in the log messages, i.e. file name, function name, line number, etc.
 ******************************************************************************/
#define WMP_LOG_MSG(lvl, msg) \
  wmp::log_t::write(wmp::log_t::msg_t(lvl, \
    std::chrono::high_resolution_clock::now().time_since_epoch().count(), \
    std::this_thread::get_id(), \
    __FILE__, __FUNCTION__, __LINE__) \
    << msg << "\n")

/*******************************************************************************
 * Write a TRACE message to the log. This is not to be used directly, use the
 * WMP_TRACE_FUNC() macro to trance the entry and exit of a function. Function
 * traces are almost always overkill and should be avoided. It generally
 * generates far to much information.
 *
 * <b>Example</b>
 * @code
 * WMP_TRACE_FUNC("Entering Function #" << 1 << ", then #" << 2);
 * @endcode
 *
 * @param[in] msg The message to be printer to the log. Multiple objects and
 *                values can be concatenated togerther using the stream
 *                insertion operator (<<) assuming the user has implemented
 *                the the overloaded operator for their type.
 ******************************************************************************/
#define WMP_LOG_TRACE(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::trace, msg)

/*******************************************************************************
 * Write a DEBUG message to the log. Generally debug message will be compiled
 * out for release versions of the library to aid performance. Debug messages
 * are things that an expert user or developer will find useful when debugging
 * issues in the software.
 ******************************************************************************/
#define WMP_LOG_DEBUG(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::debug, msg)

/*******************************************************************************
 * Write an INFO message to the log. Information messages are things that a
 * normal user will find helpful when configuring and tuning the system or
 * software.
 ******************************************************************************/
#define WMP_LOG_INFO(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::info, msg)

/*******************************************************************************
 * Write a NOTIFY message to the log. Notifications are things that a normal
 * user will find important to know under normal circumstances.
 ******************************************************************************/
#define WMP_LOG_NOTIFY(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::notify, msg)

/*******************************************************************************
 * Write a warning message to the log. Warning messages are things that a
 * normal user need to know about, though can be safely ignored in most cases.
 ******************************************************************************/
#define WMP_LOG_WARN(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::warn, msg)

/*******************************************************************************
 * Write an ERROR mesasge to the log. Error messages are things that a normal
 * user should definitely know about because it will affect their experience
 * though it will not lead to a safety or security issue.
 ******************************************************************************/
#define WMP_LOG_ERROR(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::error, msg)

/*******************************************************************************
 * Write a FATAL message to the log. Fatal messages are things that anormal
 * user must know about and will likely lead to a safety or security issue. The
 * application must be shut down for protection.
 ******************************************************************************/
#define WMP_LOG_FATAL(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::fatal, msg)

/*******************************************************************************
 * Write an EXCEPT message to the log. Exceptions are things that are outside
 * the error handling methods of the software and the application must shutdown
 * for safety and security reasons.
 ******************************************************************************/
#define WMP_LOG_EXCEPTION(msg) \
  WMP_LOG_MSG(wmp::log_t::levels_t::excep, msg)

namespace wmp
{
  /*****************************************************************************
   * General purpose thread safe log class that cache and write log message
   * in a seperate thread. It is able to print messages to std::cout, std::cerr,
   * std::clog, syslog() and any accessible file.
   *
   * The user can / should use the the helper macros #WMP_LOG_DEBUG(msg),
   * #WMP_LOG_INFO(msg), #WMP_LOG_NOTIFY(msg), #WMP_LOG_WARN(msg),
   * #WMP_LOG_ERROR(msg), #WMP_LOG_FATAL(msg) and #WMP_LOG_EXCEPTION(msg),
   * instead of building up their own message types. It's not a requirement
   * to user the logger, but it's a waste of your time to make your own.
   ****************************************************************************/
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
      excep
    };

    class msg_t final
    {
      /** The log level being recorded. */
      levels_t lvl_v;

      /** The moment the log entry was created. */
      std::uint64_t time_stamp_v;

      /** The thread that generated the log message. */
      std::thread::id thread_id_v;

      /** The file name in which the log message was generated. */
      std::string file_name_v;

      /** The function name in which the log message was generated. */
      std::string func_name_v;

      /** The line number onwhich the log message was generated. */
      std::size_t line_num_v;

      /** The string stream object used to build the log message. */
      std::stringstream ss_v;

    public:
      msg_t(levels_t lvl = levels_t::excep,
            std::uint64_t time_stamp = 0,
            std::thread::id thread_id = std::thread::id(),
            const char * file = "",
            const char * func = "",
            std::size_t line = 0);

      template <typename t> msg_t & operator << (const t & val)
      {
        ss_v << val;
        return * this;
      }

      std::uint64_t time() const
      {
        return time_stamp_v;
      }

      std::thread::id thread_id() const
      {
        return thread_id_v;
      }

      const std::string & file_name() const
      {
        return file_name_v;
      }

      const std::string & func_name() const
      {
        return func_name_v;
      }

      std::size_t line_num() const
      {
        return line_num_v;
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

    /** All the possible states that syslog can be in. */
    enum class syslog_states_t
    {
      /** Syslog must be disabled. */
      disable,

      /** Syslog must be enabled. */
      enable,

      /** Thename of the application change. Syslog must close and reopen. */
      app_name_changed,

      /** Syslog is open and can can write log messages. */
      open,

      /** Syslog is closed and can not write log messages. */
      closed
    };

    /** The list of file streams that must be destroyed clsoed when the object
     * is destroyed. */
    std::map<std::string, std::unique_ptr<std::ofstream>> ofs_v;

    /** The list of output streams for each message type. */
    std::array<std::vector<std::ostream*>,
      static_cast<std::size_t>(levels_t::excep) + 1> os_v;

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

    syslog_states_t syslog_state_v;

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

    void write_log_entry(levels_t lvl, const std::string & header,
                         const std::string & body,
                         const std::vector<std::ostream*> & os);

    /***************************************************************************
     *
     *
     **************************************************************************/
    void queue_msg(msg_t & msg);

    /***************************************************************************
     * This function requires that the calling function lock the write mutex
     * otherwise the thread operation will not be thread ssafe.
     **************************************************************************/
    void add_output_unsafe(std::ostream * os,
                           std::initializer_list<log_t::levels_t> lvls);

    void remove_output_unsafe(std::ostream * os,
                              std::initializer_list<log_t::levels_t> lvls);


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

    static void add_output(std::ostream * os,
                           std::initializer_list<log_t::levels_t>  lvl = {});

    static void remove_output(std::ostream * os,
                              std::initializer_list<log_t::levels_t> lvls = {});

    /***************************************************************************
     * Log messages of the specified levels to the file. If lvls is empty, then
     * all messages will be logged to the specified file. Log messages can
     * either be appended to the log file, or the log file can be truncated /
     * cleared first before writting the new messages. This behaviour is
     * specified by append, and by default the log files are truncated (false)
     * to prevent excessive log growth.
     *
     * @param[in] path    The path of the log file.
     * @param[in] lvls    The levels of the log messages to be written to this
     *                    file. If empty ({}), then all log messages will be
     *                    written to this file.
     * @param[in] append  Indicate whether log messages should be appended to
     *                    the end of the file (true) or whether the file should
     *                    truncated (false) before writting new messages.
     * @return            True if the file was opened and added as an output
     *                    stream else false.
     **************************************************************************/
    static bool add_output(const std::string & path,
                           std::initializer_list<levels_t> lvls = {},
                           bool append = false);

    static bool remove_output(const std::string & path,
                              std::initializer_list<levels_t> lvls = {});

    /***************************************************************************
     * Allow message to be written to syslog. Note that if syslog has not been
     * configured to accept the particular message level / priority, then it
     * will not show up in syslog. The syslog priorities map to log levels as:
     *
     *  | log levels          | syslog      |
     *  | --------------------|-------------|
     *  | levels_t::except    | LOG_EMERG   |
     *  | levels_t::fatal     | LOG_CRIT    |
     *  | levels_t::error     | LOG_ERR     |
     *  | levels_t::warn      | LOG_WARNING |
     *  | levels_t::notify    | LOG_NOTICE  |
     *  | levels_t::info      | LOG_INFO    |
     *  | levels_t::debug     | LOG_DEBUG   |
     *  | levels_t::trace     | LOG_DEBUG   |
     **************************************************************************/
    static void enable_syslog();

    /***************************************************************************
     * Stop sending messages to syslog. By default syslog is disabled, so it is
     * not necisary to call this upon startup if syslog is not used. Syslog is
     * also automatically closed when the log_t object is destroyed, so this
     * does not have to be called either before a shutdown.
     **************************************************************************/
    static void disable_syslog();

    /***************************************************************************
     * Set the name of the application be logged.
     **************************************************************************/
    static void app_name(const std::string & name);

    /***************************************************************************
     * Write a message to the log streams. The log message contains all the
     * information about the log message, i.e. file name, function name, line
     * number, when it occured, which thread it was in, etc.
     *
     * @param[in] msg The mesasge to be written to the log.
     **************************************************************************/
    static void write(msg_t & msg)
    {
      log_t::ref().queue_msg(msg);
    }
  };

  /*****************************************************************************
   * Overload the global stream insertion operator for to print log levels to
   * output streams. The respective string representations are:
   *
   *  | log levels          | String  |
   *  | --------------------|---------|
   *  | levels_t::except    | EXCEP   |
   *  | levels_t::fatal     | FATAL   |
   *  | levels_t::error     | ERROR   |
   *  | levels_t::warn      | WARN    |
   *  | levels_t::notify    | NOTIF   |
   *  | levels_t::info      | INFO    |
   *  | levels_t::debug     | DEBUG   |
   *  | levels_t::trace     | DEBUG   |
   *
   * @param[in, out]  os  The output stream where the level must be written too.
   * @param[in]       lvl The level that must written out a string.
   * @return          The ostream object that the level string was inserted into.
   ****************************************************************************/
  std::ostream & operator << (std::ostream & os, const log_t::levels_t & lvl);
}
