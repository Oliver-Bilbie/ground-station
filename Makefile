.PHONY: build server dash dashboard
default: build

build:
	@cmake -B build -DCMAKE_BUILD_TYPE=Release
	@cmake --build build

server:
	@cmake -B build-arm64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$(CURDIR)/cmake/zig-aarch64.cmake"
	@cmake --build build-arm64
	@cd server && zip -j app.zip Dockerfile GroundStation Satellite orchestrator.py
	@cd server && zip -j attach.zip attach.py
	@cd terraform && terraform init && terraform apply

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
