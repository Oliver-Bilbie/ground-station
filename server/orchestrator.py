#!/usr/bin/env python3
import subprocess
import time
import random
import signal
import sys
import os
from typing import List

GROUNDSTATION_CMD = ["./GroundStation"]
SATELLITE_CMD = ["./Satellite"]


class Orchestrator:
    def __init__(self):
        self.gs_process: subprocess.Popen | None = None
        self.satellite_processes: List[subprocess.Popen] = []
        self.running = True

    def start_groundstation(self):
        if self.gs_process and self.gs_process.poll() is None:
            return  # already running
        print("[Orchestrator] Starting GroundStation...")
        self.gs_process = subprocess.Popen(GROUNDSTATION_CMD, cwd=os.getcwd())

    def spawn_satellite(self):
        print(
            f"[Orchestrator] Spawning new Satellite... (current: {len(self.satellite_processes)})"
        )
        proc = subprocess.Popen(SATELLITE_CMD, cwd=os.getcwd())
        self.satellite_processes.append(proc)

    def kill_one_satellite(self):
        if not self.satellite_processes:
            return
        proc = self.satellite_processes.pop()
        print(
            f"[Orchestrator] Killing satellite (PID {proc.pid})... (remaining: {len(self.satellite_processes)})"
        )
        proc.terminate()  # SIGTERM first
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            print(
                f"[Orchestrator] Satellite {proc.pid} did not terminate gracefully, killing it"
            )
            proc.kill()

    def reap_dead_children(self):
        # Restart GroundStation if it died
        if self.gs_process and self.gs_process.poll() is not None:
            print("[Orchestrator] GroundStation died — restarting...")
            self.gs_process = None
            self.start_groundstation()

        # Remove any dead satellites from our list
        self.satellite_processes = [
            p for p in self.satellite_processes if p.poll() is None
        ]

    def shutdown(self, signum=None, frame=None):
        print("[Orchestrator] Shutting down — killing all children...")
        self.running = False
        for proc in self.satellite_processes:
            proc.terminate()
        if self.gs_process:
            self.gs_process.terminate()
        sys.exit(0)

    def run(self):
        signal.signal(signal.SIGTERM, self.shutdown)
        signal.signal(signal.SIGINT, self.shutdown)

        self.start_groundstation()

        while self.running:
            self.reap_dead_children()

            current_count = len(self.satellite_processes)
            target = random.randint(1, 8)

            print(f"[Orchestrator] Current: {current_count} | Target: {target}")

            # Add or remove ONLY ONE at a time
            if current_count < target:
                self.spawn_satellite()
            elif current_count > target:
                self.kill_one_satellite()
            else:
                print("[Orchestrator] At target count — no change")

            sleep_time = random.randint(15, 45)
            print(f"[Orchestrator] Next adjustment in {sleep_time} seconds...\n")
            time.sleep(sleep_time)


if __name__ == "__main__":
    random.seed()
    orch = Orchestrator()
    try:
        orch.run()
    except KeyboardInterrupt:
        orch.shutdown()
