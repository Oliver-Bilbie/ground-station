#!/usr/bin/env python3
import os
import random
import signal
import subprocess
import sys
import time
from typing import List

import boto3

GROUNDSTATION_CMD = ["./GroundStation", "--ws"]
SATELLITE_CMD = ["./Satellite"]
EXPIRE_AT_PARAMETER = os.environ.get("EXPIRE_AT_PARAMETER")
LEASE_CHECK_S = 5


class Orchestrator:
    def __init__(self):
        self.gs_process: subprocess.Popen | None = None
        self.satellite_processes: List[subprocess.Popen] = []
        self.running = True
        self.ssm = boto3.client("ssm") if EXPIRE_AT_PARAMETER else None

    def start_groundstation(self):
        if self.gs_process and self.gs_process.poll() is None:
            return
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
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            print(
                f"[Orchestrator] Satellite {proc.pid} did not terminate gracefully, killing it"
            )
            proc.kill()

    def stop_children(self):
        for proc in self.satellite_processes:
            proc.terminate()
        self.satellite_processes = []
        if self.gs_process:
            self.gs_process.terminate()
            self.gs_process = None

    def reap_dead_children(self):
        if self.gs_process and self.gs_process.poll() is not None:
            print("[Orchestrator] GroundStation died — restarting...")
            self.gs_process = None
            self.start_groundstation()

        self.satellite_processes = [
            p for p in self.satellite_processes if p.poll() is None
        ]

    def expired(self) -> bool:
        if not self.ssm or not EXPIRE_AT_PARAMETER:
            return False
        try:
            value = self.ssm.get_parameter(Name=EXPIRE_AT_PARAMETER)["Parameter"][
                "Value"
            ]
            return time.time() >= float(value)
        except Exception as e:
            print(f"[Orchestrator] Failed to read expire_at: {e}")
            return False

    def shutdown(self, signum=None, frame=None):
        print("[Orchestrator] Shutting down — killing all children...")
        self.running = False
        self.stop_children()
        sys.exit(0)

    def sleep_interruptible(self, seconds: int):
        deadline = time.time() + seconds
        while self.running and time.time() < deadline:
            if self.expired():
                return
            remaining = deadline - time.time()
            if remaining <= 0:
                return
            time.sleep(min(LEASE_CHECK_S, remaining))

    def run(self):
        signal.signal(signal.SIGTERM, self.shutdown)
        signal.signal(signal.SIGINT, self.shutdown)

        while self.running:
            if self.expired():
                if self.gs_process or self.satellite_processes:
                    print("[Orchestrator] expire_at reached — stopping")
                    self.stop_children()
                time.sleep(LEASE_CHECK_S)
                continue

            self.start_groundstation()
            self.reap_dead_children()

            current_count = len(self.satellite_processes)
            target = random.randint(1, 8)

            print(f"[Orchestrator] Current: {current_count} | Target: {target}")

            if current_count < target:
                self.spawn_satellite()
            elif current_count > target:
                self.kill_one_satellite()
            else:
                print("[Orchestrator] At target count — no change")

            sleep_time = random.randint(15, 45)
            print(f"[Orchestrator] Next adjustment in {sleep_time} seconds...\n")
            self.sleep_interruptible(sleep_time)


if __name__ == "__main__":
    random.seed()
    orch = Orchestrator()
    try:
        orch.run()
    except KeyboardInterrupt:
        orch.shutdown()
