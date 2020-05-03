cmake_minimum_required(VERSION 3.10)

project(wmp)

set(WMP_VERSION_MAJOR 0)
set(WMP_VERSION_MINOR 1)

configure_file(config.hpp.in "${CMAKE_BINARY_DIR}/config.hpp" @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(WMP_CONF_MAX_LOG_QUEUE_LEN 10000 CACHE STRING
  "The maximum number of log messages that can be cached before
  log_t::write() starts to block.")


set(WMP_LOG_HEADERS
  include/wmp/log.hpp
)

set(WMP_LOG_SOURCES
  source/wmp/log.cpp
)

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Make cmake affix a "d" character at the end of library name when compiled in
# debug.
set(CMAKE_DEBUG_POSTFIX d)




find_library(WMP_LIB_PTHREAD pthread)
list(APPEND WMP_LOG_DEPENDS_LIBS ${WMP_LIB_PTHREAD})

add_library(wmplog ${WMP_LOG_HEADERS} ${WMP_LOG_SOURCES})
target_link_libraries(wmplog ${WMP_LOG_DEPENDS_LIBS})

add_subdirectory(examples)

#add_executable(test main.cpp)
#target_link_libraries(test ${CMAKE_PROJECT_NAME})

find_package(Doxygen REQUIRED)
if(DOXYGEN_FOUND)
  set(DOXYGEN_OUTPUT_DIRECTORY doxygen)
  set(DOXYGEN_JAVADOC_BANNER YES)
  doxygen_add_docs(doxygen "${CMAKE_SOURCE_DIR}/include")
endif()
