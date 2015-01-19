project(pctrack)
cmake_minimum_required(VERSION 2.8)

add_executable(
  pctrack
  src/main.cpp
)

include(FindPkgConfig)

pkg_search_module(SDL2 REQUIRED sdl2)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(pctrack ${SDL2_LIBRARIES})
