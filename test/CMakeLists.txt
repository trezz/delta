
add_executable(countchars countchars.c)
target_link_libraries(countchars delta)
set_property(TARGET countchars PROPERTY C_STANDARD 11)

add_subdirectory(bench)
add_subdirectory(qex)
