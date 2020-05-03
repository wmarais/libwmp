cmake_minimum_required(VERSION 3.10)

project(wmp)

set(WMP_VERSION_MAJOR 0)
set(WMP_VERSION_MINOR 1)

################################################################################
# C++ CONFIGURATION
################################################################################
# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Make cmake affix a "d" character at the end of library name when compiled in
# debug.
set(CMAKE_DEBUG_POSTFIX d)

################################################################################
# CONFIGURATION
################################################################################
# Generate option recommendation based on whether this is a debug build or not.
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(WMP_IF_DEBUG ON)
  set(WMP_IF_NOT_DEBUG OFF)
else()
  set(WMP_IF_DEBUG OFF)
  set(WMP_IF_NOT_DEBUG ON)
endif()

# Check if the debug messages should be compiled in or not.
option(WMP_CONF_LOG_DEBUG_MSG_EN "Enable debug messages." ${WMP_IF_DEBUG})

# Check if the component examples must be compiled or not.
option(WMP_CONF_BUILD_EXAMPLES "Compile the example programs." ${WMP_IF_DEBUG})

# Check if the doxygen documentation must be compiled or not.
option(WMP_CONF_BUILD_DOCS "Compile the doxygen docs." ${WMP_IF_NOT_DEBUG})

# Set the number of messages to be queue in the log.
set(WMP_CONF_MAX_LOG_QUEUE_LEN 10000 CACHE STRING
  "The maximum number of log messages that can be cached before
  log_t::write() starts to block.")





# Check if syslog is present on the system.
find_file(WMP_CONF_HAS_SYSLOG NAMES syslog.h)

# Generate the config file.
configure_file(config.hpp.in "${CMAKE_BINARY_DIR}/config.hpp" @ONLY)

# Add the cmake binary directory so that config.hpp can be directly included
# and still work as intended after installed.
include_directories(${CMAKE_CURRENT_BINARY_DIR})

################################################################################
# WMP - LOG
################################################################################
set(WMP_LOG_HEADERS
  include/wmp/log.hpp
)

set(WMP_LOG_SOURCES
  source/wmp/log.cpp
)

# Create the log library.
add_library(wmplog ${WMP_LOG_HEADERS} ${WMP_LOG_SOURCES})

# Need to link to pthread in linux / gnu.
find_library(WMP_LIB_PTHREAD pthread)
target_link_libraries(wmplog ${WMP_LIB_PTHREAD})


################################################################################
# EXAMPLES
################################################################################
if(WMP_CONF_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

################################################################################
# DOXYGEN DOCS
################################################################################
if(WMP_CONF_BUILD_DOCS)
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    set(DOXYGEN_OUTPUT_DIRECTORY doxygen)
    set(DOXYGEN_JAVADOC_BANNER YES)
    doxygen_add_docs(doxygen "${CMAKE_SOURCE_DIR}/include")
  else()
    set(WMP_CONF_BUILD_DOCS OFF)
    message("Doxygen is required to generate documentation.")
  endif()
endif()

