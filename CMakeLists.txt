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

