cmake_minimum_required(VERSION 3.16)
project(atm_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(atm_core src/ATM.cpp)
target_include_directories(atm_core PUBLIC src)

add_executable(atm_simulator src/main.cpp)
target_link_libraries(atm_simulator PRIVATE atm_core)
