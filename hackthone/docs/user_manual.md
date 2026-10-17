# User Manual: Dining Philosophers Deadlock Prevention System

This guide provides complete instructions for compiling, running, testing, and demonstrating the **Dining Philosophers Deadlock Prevention System**.

---

## 1. System Requirements

- **Operating System:** Ubuntu Linux, Debian, or WSL2 (Windows Subsystem for Linux) on Windows 10/11.
- **C Compiler:** GCC 9.0+ or Clang with POSIX Threads (`-pthread`) and Real-time (`-lrt`) support.
- **Build System:** GNU Make 4.0+.
- **Backend Runtime:** Python 3.8+ with `flask` and `flask-cors`.
- **Browser:** Modern web browser (Chrome, Edge, Firefox, or Safari).

---

## 2. Quick Start Guide

### Step 1: Clone or Navigate to the Project Directory
```bash
cd dining-philosophers-deadlock-prevention
```

### Step 2: Compile the C Simulation Engine
```bash
make
```
This invokes `gcc` with `-Wall -Wextra -Wpedantic -O2 -std=c11 -pthread -lrt` and outputs the executable to `build/dining_philosophers`.

### Step 3: Run the Automated Invariant Test Suite
```bash
make test
```
Executes all 10 unit and invariant tests, verifying mutual exclusion, deadlock freedom, and concurrency.

### Step 4: Launch the Web Dashboard
Install Python requirements (if not already installed):
```bash
pip install -r backend/requirements.txt
```
Start the local server:
```bash
python backend/server.py
```
Open your browser and navigate to:
```
http://localhost:5000
```

---

## 3. Command-Line Interface (CLI) Guide

The C simulation engine can be executed directly in the terminal without starting the web server.

### Basic Execution
```bash
./build/dining_philosophers -n 5 -m 3 -s 0 -d 400
```

### Command-Line Arguments
| Option | Argument | Description | Default |
| :--- | :--- | :--- | :--- |
| `-n` | `<int>` | Number of philosophers ($2 \le N \le 10$) | `5` |
| `-m` | `<int>` | Target meals per philosopher | `5` |
| `-s` | `<0 or 1>` | Strategy (`0`: Deadlock Prevention, `1`: Unsafe Demo) | `0` |
| `-d` | `<ms>` | Simulation delay in milliseconds per phase | `600` |
| `--json` | Flag | Output structured JSON events instead of plain text | Off |
| `--interactive` | Flag | Enable interactive stdin command listener | Off |
| `-h, --help` | Flag | Display CLI usage summary and exit | - |

### Interactive CLI Controls
When run with `--interactive`, the engine listens for commands typed directly into `stdin`:
- `PAUSE`: Pauses active simulation threads cleanly.
- `RESUME`: Wakes all paused threads.
- `STEP`: Advances the simulation by a single step.
- `SPEED <ms>`: Dynamically adjusts delay (e.g., `SPEED 200`).
- `STATUS`: Prints snapshot of philosopher states and fork locks.
- `STOP` or `QUIT`: Gracefully terminates threads and exits.

---

## 4. Web Dashboard User Guide

The web dashboard is organized into 6 tabs:

### 1. Dashboard (Live Simulation)
- **Top Control Bar:**
  - Select between **Deadlock Prevention (N-1 Semaphore)** and **Unsafe Demo (Circular Wait)**.
  - Choose number of philosophers ($N=3$ to $8$).
  - Configure target meals and simulation speed ($100\text{ ms}$ to $1500\text{ ms}$).
  - Control execution with **Start**, **Pause**, **Resume**, **Step**, and **Reset** buttons.
- **Status Cards:** Live counts of Thinking, Hungry, Eating, available Semaphore Permits, Deadlock Status, and Runtime.
- **Circular Table Arena:** Real-time visual representation of philosophers and forks.
  - Blue: Thinking
  - Amber: Hungry (requesting permit/forks)
  - Green: Eating (holding both adjacent forks)
  - Crimson: Deadlocked (circular wait)
  - Metallic Gray / Silver: Available fork
  - Red with Glow: Held fork
- **Live Event Stream:** Timestamped events emitted by the C engine. Supports filtering by philosopher and event type, text search, and auto-scrolling.

### 2. Algorithm & Proof
- Mathematical breakdown of Dijkstra's problem formulation.
- Step-by-step POSIX synchronization flow.
- Formal contradiction proof of deadlock freedom using the Pigeonhole Principle.
- Summary table of concurrency primitives.

### 3. Deadlock Analysis
- Coffman's 4 necessary conditions breakdown.
- Visual explanation of how the $N-1$ semaphore breaks Circular Wait.
- Comparative analysis between Prevention, Avoidance (Banker's), and Detection.

### 4. Live Statistics
- Dynamic Chart.js bar chart of meals completed per philosopher.
- Real-time chart of cumulative waiting times.
- Detailed tabular metrics on average wait times, fork utilization, and acquisition counts.

### 5. Test Suite
- Displays status of all 10 automated test cases.
- Includes button to re-run test suite on-demand.

### 6. Viva Guide
- Interactive accordion containing 22+ questions and concise answers for viva voce preparation.

---

## 5. Troubleshooting & FAQs

### Problem: `gcc` command not found on Windows
- **Solution:** This project uses POSIX threads and semaphores which are natively available in Linux. On Windows, use WSL2 (Windows Subsystem for Linux). Simply type `wsl` in PowerShell to open your Ubuntu terminal, or run `python backend/server.py` on Windows which automatically invokes WSL.

### Problem: Port 5000 is already in use
- **Solution:** Specify a custom port via the `PORT` environment variable:
  ```bash
  PORT=8080 python backend/server.py
  ```
  Then open `http://localhost:8080`.

### Problem: Resetting a running simulation
- **Solution:** If a simulation is in progress, clicking **Reset** triggers a safety confirmation modal to prevent accidental disruption. Confirming releases all locks and rejoins threads cleanly.
