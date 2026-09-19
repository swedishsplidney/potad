cmake_minimum_required(VERSION 3.20)
project(potad CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(potad_init src/main.cpp
        src/sys.cpp
        src/process.cpp
        src/fs.cpp)