cmake_minimum_required(VERSION 3.6)
project("delta")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

add_library(delta
    ${CMAKE_SOURCE_DIR}/src/allocator.c
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
    include/delta/vec.h
    include/delta/strmap.h
  DESTINATION
    include/delta)

add_subdirectory(test)
