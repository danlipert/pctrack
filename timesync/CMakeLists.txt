project(timesync)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -Wall -Wextra")

include(FindPkgConfig)

pkg_search_module(CV REQUIRED opencv)

include_directories(
  ${CV_INCLUDE_DIRS}
)

add_executable(
  ts_extract
  src/extract.cpp
)
target_link_libraries(
  ts_extract
  ${CV_LIBRARIES}
)
