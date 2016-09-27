cmake_minimum_required(VERSION 2.6)
project(cdownloader)

add_executable(cdownloader main.cpp)

install(TARGETS cdownloader RUNTIME DESTINATION bin)
