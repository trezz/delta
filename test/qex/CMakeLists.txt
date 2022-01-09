
add_executable(qex qex.c)
target_link_libraries(qex delta)
set_property(TARGET qex PROPERTY C_STANDARD 11)
