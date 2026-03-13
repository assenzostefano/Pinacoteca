cmake_minimum_required(VERSION 3.16)
project(pinacoteca_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(pinacoteca_tests
  all_tests.cpp
)

target_include_directories(pinacoteca_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/mocks
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(pinacoteca_tests PRIVATE
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(pinacoteca_tests)
