.PHONY: build
default: build

build:
	@cmake -B build
	@cd build && make all

gs: ground_station
ground_station:
	@./build/GroundStation

sat: satellite
satellite:
	@./build/Satellite

test: build
	@./build/UnitTests
