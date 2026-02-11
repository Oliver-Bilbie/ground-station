cmake_minimum_required(VERSION 4.2)
project(GroundStation)

add_executable(GroundStation)
target_sources(GroundStation
   PRIVATE
   src/ground_station.cpp
)

add_executable(Satellite)
target_sources(Satellite
   PRIVATE
   src/satellite.cpp
)

add_library(Packets src/packets.cpp src/packets.h)
add_library(GPS src/gps.cpp src/gps.h)
add_library(Space src/space.cpp src/space.h)
add_library(Logger src/logger.cpp src/logger.h)
target_link_libraries(Logger PRIVATE Packets)
add_library(Dispatcher src/dispatcher.cpp src/dispatcher.h)
target_link_libraries(Dispatcher PRIVATE Space)

target_link_libraries(GroundStation
   PRIVATE
   Packets
   Logger
   Dispatcher
)
target_link_libraries(Satellite
   PRIVATE
   Packets
   Space
   GPS
)
