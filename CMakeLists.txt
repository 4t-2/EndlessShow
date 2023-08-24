cmake_minimum_required(VERSION 3.10)

project(EndlessShow)

file(GLOB_RECURSE SRC ./src/*.cpp)
file(GLOB_RECURSE AGL ./lib/AGL/src/*.cpp)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER "g++")

message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

if(CMAKE_BUILD_TYPE MATCHES Debug)
	set (CMAKE_CXX_FLAGS "-g -fsanitize=address")
	message(STATUS "Building with debug flags")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
	set (CMAKE_CXX_FLAGS "-Ofast -flto=auto")
	message(STATUS "Building with release flags")
endif()

include_directories("./lib")
include_directories("/usr/include/freetype2")

add_executable(EndlessShow ${SRC} ${AGL})

target_link_libraries(EndlessShow  -lX11 -lGL -lGLEW -lSOIL -lfreetype)
