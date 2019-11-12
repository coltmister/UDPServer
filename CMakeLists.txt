cmake_minimum_required(VERSION 3.14)
project(14var2 C)

set(CMAKE_C_STANDARD 11)

add_executable(14var2 main.c client.c)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(14var2 pthread)