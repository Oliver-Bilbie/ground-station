cmake_minimum_required(VERSION 3.22)
project(GroundStation)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


include(FetchContent)
FetchContent_Declare(uSockets_content
   GIT_REPOSITORY https://github.com/uNetworking/uSockets.git
   GIT_TAG v0.8.8
)
FetchContent_Declare(uWebSockets_content
  GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
  GIT_TAG v20.76.0
)
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(
   uSockets_content
   uWebSockets_content
   googletest
)


file(GLOB_RECURSE USOCKETS_SOURCES ${usockets_content_SOURCE_DIR}/src/*.c)
add_library(uSockets STATIC ${USOCKETS_SOURCES})
target_include_directories(uSockets PUBLIC ${usockets_content_SOURCE_DIR}/src)
target_compile_definitions(uSockets PUBLIC LIBUS_NO_ZLIB LIBUS_NO_SSL)

add_library(uWebSockets INTERFACE)
target_include_directories(uWebSockets INTERFACE ${uwebsockets_content_SOURCE_DIR}/src/)
target_link_libraries(uWebSockets INTERFACE uSockets)
target_compile_definitions(uWebSockets INTERFACE UWS_NO_ZLIB)


add_library(gs_core
    src/logger.cpp src/logger.h
    src/packets.cpp src/packets.h
    src/space.cpp src/space.h
    src/buffer.h
    src/client.h
    src/dispatcher.h
    src/globals.h
    src/gps.h
    src/latency_tracker.h
    src/rolling_avg.h
    src/server.h
    src/telemetry_server.h
    src/timer.h
)
target_include_directories(gs_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(gs_core PUBLIC uWebSockets)


add_executable(GroundStation src/ground_station.cpp)
target_link_libraries(GroundStation PRIVATE gs_core)


add_executable(Satellite src/satellite.cpp)
target_link_libraries(Satellite PRIVATE gs_core)


add_executable(UnitTests
   tests/test_buffer.cpp
   tests/test_dispatcher.cpp
   tests/test_gps.cpp
   tests/test_logger.cpp
   tests/test_timer.cpp
)
target_link_libraries(UnitTests PRIVATE gs_core uWebSockets GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(UnitTests)
enable_testing()
