cmake_minimum_required(VERSION 3.22)
project(mcrconpp)

set(CMAKE_CXX_STANDARD 23)

add_executable(mcrconpp main.cpp mcrcon.cpp mcrcon.h keycode.h)

find_package(sockpp CONFIG REQUIRED)
target_include_directories(mcrconpp PRIVATE ${SOCKPP_INCLUDE_DIRS})
target_link_libraries(mcrconpp PRIVATE ${SOCKPP_LIBRARIES})

find_package(argparse CONFIG REQUIRED)
target_link_libraries(mcrconpp PRIVATE argparse::argparse)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(mcrconpp PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

target_link_libraries(mcrconpp PRIVATE readline)
