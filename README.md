# 📡 Ground Station

This is a project that I built to explore some lower-level networking.
It simulates a connection between a satellite and a ground station, where the satellite periodically broadcasts its position over UDP. Random noise is introduced to simulate the packet travelling through space; some packets are delayed, while others are never sent at all. The ground station will receive these packets and log them in the correct order (even if they are received out of order) and re-request missing packets from the satellite.

## Requirements

This project targets POSIX operating systems, so if you are running Windows for some reason then you will need to use WSL.
Other than that, you will need standard C++ tooling (CMake etc) and a compiler supporting C++17 or later.

## Building the project

To build the project (client + server) simply run `make` from the project root.

## Usage

You can spin up a ground station instance by running
```
make gs
```
from the project root.

Similarly you can spin up a satellite by running
```
make sat
```

## Tests

To run the unit tests, run `make test` from the project root.
