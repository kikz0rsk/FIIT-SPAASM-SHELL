# CMakeList.txt : CMake project for CMakeProject1, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("spaasm-shell")

# Add source to this project's executable.
add_executable (spaasm-shell "kshell.c" "kshell.h" "common.h" "arguments.h" "arguments.c" "client.c" "client.h" "server.c" "server.h" "common.c" "parser.h" "parser.c")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(spaasm-shell PRIVATE Threads::Threads)

# TODO: Add tests and install targets if needed.
