cmake_minimum_required(VERSION 3.21)
project(de_tutorial)

set(CMAKE_CXX_STANDARD 23)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(de_tutorial main.cpp)
#test.hpp test.cpp
target_link_libraries(de_tutorial PRIVATE ${CONAN_LIBS})
target_include_directories(de_tutorial PRIVATE ${CONAN_INCLUDE_DIRS})
message(WARNING ${CONAN_LIBS})
#message(WARNING ${CONAN_LIBS_DILIGENT-CORE})
#message(WARNING ${CONAN_INCLUDE_DIRS})

