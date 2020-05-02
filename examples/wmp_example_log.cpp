#include "../include/wmp/log.hpp"

using namespace wmp;

int main(int argc, char * argv[])
{
  /* Set the name of the application being logged. */
  log_t::app_name(argv[0]);

  /* Set the minimum log level to enable. */
  log_t::min_level(log_t::levels_t::trace);


  WMP_LOG_DEBUG("Debug test message!");

  std::this_thread::sleep_for(std::chrono::seconds(60));

  return 0;
}
