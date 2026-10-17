#!/usr/bin/env python3
"""
Dining Philosophers Problem - Backend Integration Layer
Connects the web frontend to the POSIX C Simulation Engine via REST & SSE.
"""

import os
import sys
import json
import time
import subprocess
import threading
import queue
from pathlib import Path
from flask import Flask, request, jsonify, Response, send_from_directory
from flask_cors import CORS

app = Flask(__name__, static_folder='../frontend', static_url_path='')
CORS(app)

PROJECT_ROOT = Path(__file__).resolve().parent.parent
BUILD_BIN = PROJECT_ROOT / 'build' / 'dining_philosophers'

class SimulationProcessManager:
    def __init__(self):
        self.process = None
        self.lock = threading.Lock()
        self.event_queue = queue.Queue(maxsize=1000)
        self.subscribers = []
        self.is_running = False
        self.is_paused = False
        self.current_state = {
            "status": "STOPPED",
            "num_philosophers": 5,
            "max_meals": 5,
            "strategy": "DEADLOCK_PREVENTION_N_MINUS_1_SEMAPHORE",
            "speed_ms": 600,
            "total_steps": 0,
            "permits": 4,
            "deadlock": False,
            "fork_owners": [-1, -1, -1, -1, -1],
            "philosopher_states": ["THINKING", "THINKING", "THINKING", "THINKING", "THINKING"],
            "meals": [0, 0, 0, 0, 0],
            "wait_times_ms": [0, 0, 0, 0, 0],
            "start_time": None,
            "runtime_seconds": 0.0
        }
        self.event_history = []
        self.reader_thread = None

    def start_simulation(self, num_philosophers=5, max_meals=5, strategy=0, speed_ms=600):
        with self.lock:
            self.stop_simulation_locked()

            # Verify binary exists or compile it
            if not BUILD_BIN.exists():
                print("[INFO] Binary not found. Invoking build via Makefile...")
                self._build_binary()

            # Command creation
            if os.name == 'nt':
                wsl_root = str(PROJECT_ROOT).replace('\\', '/').replace('C:', '/mnt/c').replace('c:', '/mnt/c')
                # Ensure clean symlink without spaces in WSL
                try:
                    subprocess.run(['wsl', '-e', 'bash', '-c', f"ln -sfn '{wsl_root}' /tmp/dining_sim"], check=True)
                except Exception as e:
                    print(f"[WARN] Failed to set up WSL symlink: {e}", flush=True)

                cmd = [
                    'wsl', '/tmp/dining_sim/build/dining_philosophers',
                    '-n', str(num_philosophers),
                    '-m', str(max_meals),
                    '-s', str(strategy),
                    '-d', str(speed_ms),
                    '--json', '--interactive'
                ]
            else:
                cmd = [
                    str(BUILD_BIN),
                    '-n', str(num_philosophers),
                    '-m', str(max_meals),
                    '-s', str(strategy),
                    '-d', str(speed_ms),
                    '--json', '--interactive'
                ]

            print(f"[INFO] Launching C simulation engine: {' '.join(cmd)}", flush=True)
            try:
                self.process = subprocess.Popen(
                    cmd,
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    cwd=str(PROJECT_ROOT)
                )
            except Exception as e:
                print(f"[ERROR] Failed to spawn process: {e}", flush=True)
                return False, f"Failed to spawn C simulation process: {e}"

            self.is_running = True
            self.is_paused = False
            self.event_history = []
            self.current_state.update({
                "status": "RUNNING",
                "num_philosophers": num_philosophers,
                "max_meals": max_meals,
                "strategy": "DEADLOCK_PREVENTION_N_MINUS_1_SEMAPHORE" if strategy == 0 else "UNSAFE_CIRCULAR_WAIT_DEMO",
                "speed_ms": speed_ms,
                "total_steps": 0,
                "permits": num_philosophers - 1,
                "deadlock": False,
                "fork_owners": [-1] * num_philosophers,
                "philosopher_states": ["THINKING"] * num_philosophers,
                "meals": [0] * num_philosophers,
                "wait_times_ms": [0] * num_philosophers,
                "start_time": time.time(),
                "runtime_seconds": 0.0
            })

            # Start background reader thread
            proc_ref = self.process
            self.reader_thread = threading.Thread(target=self._read_stdout, args=(proc_ref,), daemon=True)
            self.reader_thread.start()
            return True, "Simulation started successfully"

    def _build_binary(self):
        try:
            if os.name == 'nt':
                wsl_root = str(PROJECT_ROOT).replace('\\', '/').replace('C:', '/mnt/c').replace('c:', '/mnt/c')
                subprocess.run(['wsl', '-e', 'bash', '-c', f"cd '{wsl_root}' && make"], check=True)
            else:
                subprocess.run(['make'], cwd=str(PROJECT_ROOT), check=True)
        except Exception as e:
            print(f"[ERROR] Failed to compile binary: {e}", flush=True)

    def _read_stdout(self, proc):
        print(f"[DEBUG] _read_stdout started for pid {proc.pid}", flush=True)
        while proc and proc.stdout:
            bline = proc.stdout.readline()
            if not bline:
                print(f"[DEBUG] EOF from process stdout. Poll: {proc.poll()}", flush=True)
                break
            line = bline.decode('utf-8', errors='ignore').strip()
            if not line:
                continue

            try:
                event = json.loads(line)
                self._process_event(event)
            except json.JSONDecodeError:
                print(f"[C ENGINE RAW]: {line}", flush=True)

        with self.lock:
            if self.process == proc:
                self.is_running = False
                self.current_state["status"] = "FINISHED" if not self.current_state.get("deadlock") else "DEADLOCKED"
                print(f"[DEBUG] _read_stdout finished. Final status: {self.current_state['status']}", flush=True)

    def _process_event(self, event):
        with self.lock:
            if "step" in event:
                self.current_state["total_steps"] = event["step"]
            if "fork_owners" in event:
                self.current_state["fork_owners"] = event["fork_owners"]
            if "philosopher_states" in event:
                self.current_state["philosopher_states"] = event["philosopher_states"]
            if "meals" in event:
                self.current_state["meals"] = event["meals"]
            if "wait_times_ms" in event:
                self.current_state["wait_times_ms"] = event["wait_times_ms"]
            if "permits" in event:
                self.current_state["permits"] = event["permits"]
            if "deadlock" in event:
                self.current_state["deadlock"] = event["deadlock"]
                if event["deadlock"]:
                    self.current_state["status"] = "DEADLOCKED"

            if self.current_state["start_time"]:
                self.current_state["runtime_seconds"] = round(time.time() - self.current_state["start_time"], 2)

            # Store recent event in history
            self.event_history.append(event)
            if len(self.event_history) > 2000:
                self.event_history.pop(0)

            # Broadcast to SSE subscribers
            dead_subs = []
            for sub_queue in self.subscribers:
                try:
                    sub_queue.put_nowait(event)
                except queue.Full:
                    dead_subs.append(sub_queue)
            for d in dead_subs:
                self.subscribers.remove(d)

    def send_command(self, cmd_str):
        with self.lock:
            if not self.is_running or not self.process or not self.process.stdin:
                return False, "Simulation is not actively running"
            try:
                self.process.stdin.write(f"{cmd_str}\n".encode('utf-8'))
                self.process.stdin.flush()
                return True, f"Command '{cmd_str}' sent"
            except Exception as e:
                return False, f"Error sending command: {e}"

    def pause(self):
        ok, msg = self.send_command("PAUSE")
        if ok:
            with self.lock:
                self.is_paused = True
                self.current_state["status"] = "PAUSED"
        return ok, msg

    def resume(self):
        ok, msg = self.send_command("RESUME")
        if ok:
            with self.lock:
                self.is_paused = False
                self.current_state["status"] = "RUNNING"
        return ok, msg

    def step(self):
        return self.send_command("STEP")

    def set_speed(self, speed_ms):
        ok, msg = self.send_command(f"SPEED {speed_ms}")
        if ok:
            with self.lock:
                self.current_state["speed_ms"] = speed_ms
        return ok, msg

    def stop_simulation(self):
        with self.lock:
            return self.stop_simulation_locked()

    def stop_simulation_locked(self):
        if self.process:
            try:
                if self.process.stdin:
                    self.process.stdin.write(b"STOP\n")
                    self.process.stdin.flush()
                self.process.wait(timeout=1.5)
            except Exception:
                try:
                    self.process.terminate()
                except Exception:
                    pass
            self.process = None
        self.is_running = False
        self.is_paused = False
        self.current_state["status"] = "STOPPED"
        return True, "Simulation stopped"

manager = SimulationProcessManager()

# --- HTTP Endpoints ---

@app.route('/')
def index():
    return send_from_directory(app.static_folder, 'index.html')

@app.route('/api/status', methods=['GET'])
def get_status():
    with manager.lock:
        state = dict(manager.current_state)
        if state.get("start_time") and manager.is_running:
            state["runtime_seconds"] = round(time.time() - state["start_time"], 2)
        return jsonify({
            "success": True,
            "data": state
        })

@app.route('/api/start', methods=['POST'])
def start_simulation():
    data = request.get_json() or {}
    num_philosophers = int(data.get('num_philosophers', 5))
    max_meals = int(data.get('max_meals', 5))
    strategy = int(data.get('strategy', 0))
    speed_ms = int(data.get('speed_ms', 600))

    if num_philosophers < 2 or num_philosophers > 10:
        return jsonify({"success": False, "error": "Philosophers must be between 2 and 10"}), 400
    if max_meals < 1 or max_meals > 100:
        return jsonify({"success": False, "error": "Meals must be between 1 and 100"}), 400

    ok, msg = manager.start_simulation(num_philosophers, max_meals, strategy, speed_ms)
    return jsonify({"success": ok, "message": msg}), (200 if ok else 500)

@app.route('/api/pause', methods=['POST'])
def pause_simulation():
    ok, msg = manager.pause()
    return jsonify({"success": ok, "message": msg})

@app.route('/api/resume', methods=['POST'])
def resume_simulation():
    ok, msg = manager.resume()
    return jsonify({"success": ok, "message": msg})

@app.route('/api/step', methods=['POST'])
def step_simulation():
    ok, msg = manager.step()
    return jsonify({"success": ok, "message": msg})

@app.route('/api/speed', methods=['POST'])
def speed_simulation():
    data = request.get_json() or {}
    speed_ms = int(data.get('speed_ms', 600))
    ok, msg = manager.set_speed(speed_ms)
    return jsonify({"success": ok, "message": msg})

@app.route('/api/stop', methods=['POST'])
def stop_simulation():
    ok, msg = manager.stop_simulation()
    return jsonify({"success": ok, "message": msg})

@app.route('/api/reset', methods=['POST'])
def reset_simulation():
    ok, msg = manager.stop_simulation()
    with manager.lock:
        manager.event_history = []
        manager.current_state.update({
            "status": "STOPPED",
            "total_steps": 0,
            "deadlock": False,
            "fork_owners": [-1] * manager.current_state["num_philosophers"],
            "philosopher_states": ["THINKING"] * manager.current_state["num_philosophers"],
            "meals": [0] * manager.current_state["num_philosophers"],
            "wait_times_ms": [0] * manager.current_state["num_philosophers"],
            "runtime_seconds": 0.0
        })
    return jsonify({"success": True, "message": "Simulation reset successfully"})

@app.route('/api/events', methods=['GET'])
def get_events():
    since_step = int(request.args.get('since', 0))
    limit = int(request.args.get('limit', 100))
    with manager.lock:
        filtered = [e for e in manager.event_history if e.get('step', 0) > since_step]
        return jsonify({
            "success": True,
            "events": filtered[-limit:],
            "total_steps": manager.current_state["total_steps"]
        })

@app.route('/api/stream')
def sse_stream():
    """Server-Sent Events (SSE) streaming live C simulation events"""
    def event_generator():
        client_queue = queue.Queue(maxsize=100)
        with manager.lock:
            manager.subscribers.append(client_queue)

        try:
            # Send current snapshot first
            with manager.lock:
                snapshot = dict(manager.current_state)
            yield f"event: snapshot\ndata: {json.dumps(snapshot)}\n\n"

            while True:
                try:
                    event = client_queue.get(timeout=25.0)
                    yield f"event: message\ndata: {json.dumps(event)}\n\n"
                except queue.Empty:
                    # Keep-alive heartbeat
                    yield ": heartbeat\n\n"
        except GeneratorExit:
            with manager.lock:
                if client_queue in manager.subscribers:
                    manager.subscribers.remove(client_queue)

    return Response(event_generator(), mimetype='text/event-stream', headers={
        'Cache-Control': 'no-cache',
        'X-Accel-Buffering': 'no',
        'Connection': 'keep-alive'
    })

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    print(f"================================================================")
    print(f"  Dining Philosophers Simulation Web Server")
    print(f"  Listening at: http://localhost:{port}")
    print(f"  Serving UI from: {Path(app.static_folder).resolve()}")
    print(f"================================================================")
    app.run(host='0.0.0.0', port=port, debug=False, threaded=True)
