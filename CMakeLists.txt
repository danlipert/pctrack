project(pctrack)
cmake_minimum_required(VERSION 2.8)

add_executable(
  pctrack
  src/main.cpp
  src/sggl/opengl_data.c
  src/sggl/opengl_load.c
)

include(FindPkgConfig)

pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(GL REQUIRED gl)
include_directories(src ${SDL2_INCLUDE_DIRS} ${GL_INCLUDE_DIRS})
target_link_libraries(pctrack ${SDL2_LIBRARIES} ${GL_LIBRARIES})
