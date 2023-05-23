cmake_minimum_required(VERSION 3.25)
project(crlib)

set(CMAKE_CXX_STANDARD 20)
include(cmake/CPM.cmake)

enable_testing()

add_subdirectory(crutil)
add_subdirectory(crmath)
add_subdirectory(crui)