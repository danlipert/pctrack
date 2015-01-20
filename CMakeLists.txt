project(pctrack)
cmake_minimum_required(VERSION 2.8)

add_executable(
  pcvis
  src/common.cpp
  src/pcvis.cpp
  src/sggl/opengl_data.c
  src/sggl/opengl_load.c
)

add_executable(
  pckinect
  src/common.cpp
  src/pckinect.cpp
)

include(FindPkgConfig)

pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(GL REQUIRED gl)
pkg_search_module(FREENECT REQUIRED libfreenect)

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS} -Wall -Wextra")

include_directories(
  src
  ${SDL2_INCLUDE_DIRS}
  ${GL_INCLUDE_DIRS}
  ${FREENECT_INCLUDE_DIRS}
)

target_link_libraries(
  pckinect
  freenect
  freenect_sync
  m
)

target_link_libraries(
  pcvis
  ${SDL2_LIBRARIES}
  ${GL_LIBRARIES}
)
