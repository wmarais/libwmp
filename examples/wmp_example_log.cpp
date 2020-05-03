/*******************************************************************************
 *
 *
 ******************************************************************************/
#include "../include/wmp/log.hpp"

#include <iostream>
#include <filesystem>

using namespace wmp;

/******************************************************************************/
int main(int argc, char * argv[])
{
  /* Set the name of the application being logged. Since argv[0] contains
   * the full path of the application, strip the path and extension off. Of
   * course any old name can be specified here, but this is a generic way of
   * doing it, so perhaps just copy and paste this directly. */
  log_t::app_name(std::filesystem::path(argv[0]).filename().string());

  /* Set the minimum log level to enable. Generally when releasing software,
   * you do not want to set a log level any lower than notify() since anything
   * lower is only usable for developers. Also note that anything lower than
   * log_t::levels_t::info is only available in release if library has been
   * build to support debug log messages  in release mode. */
  log_t::min_level(log_t::levels_t::trace);


  /* If you want to output messages to any of the standard output streams, you
   * can do this using add_output and passing a pointer to the output streams.
   * Don't do this for any other output streams that will not outlive the
   * static instance of the log because that will probably do very nasty things
   * to your log. */


  /* Write only non error messages to cout. */
  log_t::add_output(&std::cout, {
                      log_t::levels_t::trace,
                      log_t::levels_t::debug,
                      log_t::levels_t::info,
                      log_t::levels_t::notify
                    });

  /* Only write error messages to cerr. */
  log_t::add_output(&std::cerr, {
                      log_t::levels_t::warn,
                      log_t::levels_t::error,
                      log_t::levels_t::fatal,
                      log_t::levels_t::excep
                    });

  /* Write all messages to clog. */
  log_t::add_output(&std::clog);

  /* IF you want to write to a log file(s), add it by providing a path string
   * to the log object. */
  log_t::add_output("wmp_example_log.log");

  /* If you want to write messages to syslog, then enable syslog. If syslog is
   * not supported on your system, the command will be ignored. */
  log_t::enable_syslog();

  /* Write a debug message to the log. */
  WMP_LOG_DEBUG("Debug test message!");

  /* WMP_LOG_EXCEPTION is reserved for printing exceptions only. Don't use it
   * for printing general errors for which there are checks and guards in place
   * for. This is to indicate that an exception event has occured. */
  try
  {
    throw std::runtime_error("This is a test exception!");
  }
  catch(std::exception & ex)
  {
    WMP_LOG_EXCEPTION("An exception occured: " << ex.what());
  }

  /* At this point the log will be destroyed and all the messages will be
   * flushed out. */
  return 0;
}
