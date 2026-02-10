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

add_library(Packet src/packet.cpp src/packet.h)
add_library(GPS src/gps.cpp src/gps.h)
add_library(Noise src/simulate_noise.cpp src/simulate_noise.h)

target_link_libraries(GroundStation
   PRIVATE
   Packet
)
target_link_libraries(Satellite
   PRIVATE
   Packet
   GPS
   Noise
)
