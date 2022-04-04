cmake_minimum_required(VERSION 3.1.2)

project(diligent_engine_tutorial)

set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(blur main.cpp)
target_link_libraries(blur PRIVATE ${CONAN_LIBS})
target_include_directories(blur PRIVATE ${CONAN_INCLUDE_DIRS})
