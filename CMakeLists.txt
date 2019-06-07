cmake_minimum_required(VERSION 3.13)
project(robot_io)

set(CMAKE_CXX_STANDARD 14)


add_executable(robot_io main.cpp)
TARGET_LINK_LIBRARIES(robot_io boost_system boost_thread)
