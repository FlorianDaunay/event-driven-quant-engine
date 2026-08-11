cmake_minimum_required(VERSION 3.14)
project(quant_engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /USE_MATH_DEFINES)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(quant_engine STATIC ${SOURCES})

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_pricing tests/cpp/test_pricing.cpp)
target_link_libraries(test_pricing PRIVATE quant_engine gtest_main)

include(GoogleTest)
gtest_discover_tests(test_pricing)