cmake_minimum_required(VERSION 3.13)
project(udp-over-tcp VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
    if(WIN32)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()
endif()

file(GLOB SOURCES *.cpp *.h)
add_executable(udp-over-tcp ${SOURCES})

if(WIN32)
    target_link_libraries(udp-over-tcp PRIVATE ws2_32)
    target_compile_definitions(udp-over-tcp PRIVATE _WIN32_WINNT=0x600)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(udp-over-tcp PRIVATE Threads::Threads)
endif()

install(TARGETS udp-over-tcp RUNTIME DESTINATION ./)


# CPack (package target)

if(WIN32)
	set(CPACK_GENERATOR ZIP)
else()
	set(CPACK_GENERATOR TGZ)
endif()

set(CPACK_STRIP_FILES ON)
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

include(CPack)