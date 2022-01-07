cmake_minimum_required(VERSION 3.6)
project("delta")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -std=c11")

add_library(delta
  ${CMAKE_SOURCE_DIR}/src/hash.c
  ${CMAKE_SOURCE_DIR}/src/strmap.c
  ${CMAKE_SOURCE_DIR}/src/vec.c
)

target_include_directories(delta PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS delta DESTINATION lib)
install(
  FILES
    include/delta/hash.h
    include/delta/strmap.h
    include/delta/vec.h
  DESTINATION
    include/delta)

add_subdirectory(test)
