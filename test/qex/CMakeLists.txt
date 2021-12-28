

set(CMAKE_C_FLAGS "-Wall -Werror -pedantic -O3 -std=c99")

add_executable(qex EXCLUDE_FROM_ALL
    qex.c
)

target_link_libraries(qex
    delta
)
