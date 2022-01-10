add_executable(bench_delta_vec delta_vec.c)
target_link_libraries(bench_delta_vec delta)
set_property(TARGET bench_delta_vec PROPERTY C_STANDARD 17)

add_executable(bench_cpp_vector cpp_vector.cpp)
set_property(TARGET bench_cpp_vector PROPERTY CXX_STANDARD 20)
