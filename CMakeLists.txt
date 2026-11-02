cmake_minimum_required(VERSION 3.14)
project(a3)
set(CMAKE_CXX_STANDARD 17)
# optimize by default: an unoptimized build makes the per-block matrix math much slower
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(SOURCE_FILES main.cpp A3Engine.cpp A3Engine.h Cameras.h World.h World.cpp Character.h Character.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

include("${CMAKE_CURRENT_SOURCE_DIR}/../../OpenGLMake.cmake")