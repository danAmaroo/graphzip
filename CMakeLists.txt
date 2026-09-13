cmake_minimum_required(VERSION 3.20)
project(graphzip CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(graphzip_core
    src/bitstream.cpp
    src/sqlparse.cpp
    src/dumps.cpp
    src/graph.cpp
)
target_include_directories(graphzip_core PUBLIC include)
target_compile_options(graphzip_core PRIVATE -Wall -Wextra)

add_executable(graphzip src/main.cpp)
target_link_libraries(graphzip PRIVATE graphzip_core)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()