.PHONY: build dash dashboard
default: build

build:
	@cmake -B build -DCMAKE_BUILD_TYPE=Release
	@cmake --build build

gs: ground_station
ground_station:
	@./build/GroundStation

sat: satellite
satellite:
	@./build/Satellite

test: build
	@./build/UnitTests

debug:
	@cmake -B build -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build

dash: dashboard
dashboard:
	@cd dashboard && npm run dev
