cmake_minimum_required(VERSION 3.16)
project(chip LANGUAGES CXX)
add_executable(chip src/25LC040A.cpp src/W25Q128.cpp example.cpp)
target_include_directories(chip PRIVATE ${CMAKE_SOURCE_DIR}/include)