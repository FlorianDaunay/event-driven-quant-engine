cmake_minimum_required(VERSION 3.14)
project(quant_engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(_USE_MATH_DEFINES)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(include)

file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")

set(LIB_SOURCES ${ALL_SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX "src/main\\.cpp$")

add_library(quant_engine STATIC ${LIB_SOURCES})

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(quant_engine_app src/main.cpp)
    target_link_libraries(quant_engine_app PRIVATE quant_engine)
endif()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp/test_pricing.cpp")
    add_executable(test_pricing tests/cpp/test_pricing.cpp)
    target_link_libraries(test_pricing PRIVATE quant_engine gtest_main)

    include(GoogleTest)
    gtest_discover_tests(test_pricing)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/cpp/test_backtest.cpp")
    add_executable(test_backtest tests/cpp/test_backtest.cpp)
    target_link_libraries(test_backtest PRIVATE quant_engine gtest_main)
    gtest_discover_tests(test_backtest)
endif()