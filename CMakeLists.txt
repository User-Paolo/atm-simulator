cmake_minimum_required(VERSION 3.16)
project(atm_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(ATM_WARNINGS /W4)
else()
    set(ATM_WARNINGS -Wall -Wextra -Wpedantic)
endif()

add_library(atm_core src/ATM.cpp)
target_include_directories(atm_core PUBLIC src)
target_compile_options(atm_core PRIVATE ${ATM_WARNINGS})

add_executable(atm_simulator src/main.cpp)
target_link_libraries(atm_simulator PRIVATE atm_core)
target_compile_options(atm_simulator PRIVATE ${ATM_WARNINGS})

option(ATM_BUILD_TESTS "Build the unit tests" ON)
if(ATM_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_executable(atm_tests tests/test_atm.cpp tests/test_ui.cpp)
    target_link_libraries(atm_tests PRIVATE atm_core GTest::gtest_main)
    target_compile_options(atm_tests PRIVATE ${ATM_WARNINGS})

    include(GoogleTest)
    gtest_discover_tests(atm_tests)
endif()
