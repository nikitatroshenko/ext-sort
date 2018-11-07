cmake_minimum_required(VERSION 3.12)
project(ext_sort)

set(CMAKE_CXX_STANDARD 14)
add_compile_definitions(DEFAULT_MEMORY_SIZE=2048)
add_compile_definitions(DEFAULT_MERGE_RANK=8)
add_compile_definitions(_LOCAL_TEST=1)

add_compile_options(-Wpedantic)
add_executable(ext_sort.out main.cpp)
add_executable(test_gen.out test_gen.cpp)
add_executable(test.out test.cpp)
add_executable(exhaustive_test_gen.out exhaustive_test_gen.cpp)

#enable_testing()
#
#add_test(NAME 8 COMMAND '../test-case.bash 8')
#add_test(NAME 512 COMMAND './test-case.bash 512')
#add_test(NAME 0 COMMAND './test-case.bash 0')
#add_test(NAME 262144 COMMAND './test-case.bash 262144')
#add_test(NAME 524288 COMMAND './test-case.bash 524288')
#add_test(NAME 1280000 COMMAND './test-case.bash 1280000')
