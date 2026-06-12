cmake_minimum_required(VERSION 3.20)
project(keyrecord)

set(CMAKE_CXX_STANDARD 20)

if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_definitions(-DUNICODE -D_UNICODE)

add_executable(keyrecord WIN32 main.cpp)
target_link_libraries(keyrecord PRIVATE unofficial::sqlite3::sqlite3 shell32)
