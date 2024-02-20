cmake_minimum_required(VERSION 3.13)
project(udp-over-tcp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

file(GLOB SOURCES *.cpp *.h)
add_executable(udp-over-tcp ${SOURCES})

if(WIN32)
    target_link_libraries(udp-over-tcp PRIVATE ws2_32)
endif()
