cmake_minimum_required(VERSION 3.20)
project(potad CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/main.cpp
    src/sys.cpp
    src/process.cpp
    src/fs.cpp
)

add_executable(potad_init ${SOURCES})

target_include_directories(potad_init PRIVATE include)

target_compile_options(potad_init PRIVATE
    -Wall
    -Wextra
    -O2
)