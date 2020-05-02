cmake_minimum_required(VERSION 3.10)

project(wmp)

set(${CMAKE_PROJECT_NAME}_VERSION_MAJOR 0)
set(${CMAKE_PROJECT_NAME}_VERSION_MINOR 1)

set(WMP_LOG_HEADERS
  include/wmp/log.hpp
)

set(WMP_LOG_SOURCES
  source/wmp/log.cpp
)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(wmplog ${WMP_LOG_HEADERS} ${WMP_LOG_SOURCES})

add_subdirectory(examples)

#add_executable(test main.cpp)
#target_link_libraries(test ${CMAKE_PROJECT_NAME})

