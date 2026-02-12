cmake_minimum_required(VERSION 4.2)
project(GroundStation)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_library(gs_core
    src/packets.cpp src/packets.h
    src/gps.cpp src/gps.h
    src/space.cpp src/space.h
    src/logger.cpp src/logger.h
    src/dispatcher.cpp src/dispatcher.h
    src/buffer.h
    src/rolling_avg.h
    src/globals.h
)
target_include_directories(gs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(GroundStation src/ground_station.cpp)
target_link_libraries(GroundStation PRIVATE gs_core)

add_executable(Satellite src/satellite.cpp)
target_link_libraries(Satellite PRIVATE gs_core)

add_executable(UnitTests
   tests/test_buffer.cpp
   tests/test_dispatcher.cpp
   tests/test_logger.cpp
)
target_link_libraries(UnitTests PRIVATE gs_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(UnitTests)
