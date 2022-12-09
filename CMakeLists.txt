cmake_minimum_required(VERSION 3.14)
project(chAT)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.*")
add_library(chAT ${SOURCES})
target_include_directories(chAT PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(chAT_example_parser examples/parser.cpp)
target_link_libraries(chAT_example_parser chAT)

add_executable(chAT_example_posix_stdio_blocking examples/posix_stdio_blocking.cpp)
target_link_libraries(chAT_example_posix_stdio_blocking chAT)

add_executable(chAT_example_posix_stdio_nonblocking examples/posix_stdio_nonblocking.cpp)
target_link_libraries(chAT_example_posix_stdio_nonblocking chAT)
