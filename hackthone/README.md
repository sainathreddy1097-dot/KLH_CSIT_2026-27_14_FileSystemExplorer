# Dining Philosophers Problem – Deadlock Prevention System

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/Tests-10%2F10%20Passing-brightgreen.svg)](tests/test_cases.md)
[![Language: C11](https://img.shields.io/badge/Language-C11%20%7C%20POSIX-orange.svg)](src/)
[![Frontend: HTML5/CSS3/JS](https://img.shields.io/badge/Frontend-Vanilla%20JS%20%7C%20Chart.js-yellowgreen.svg)](frontend/)

An academic, production-grade Operating Systems project demonstrating the classical **Dining Philosophers Problem** solved using **Deadlock Prevention**. Built with a native **C11 POSIX multi-threaded engine** (`pthread`, `mutex`, `semaphore`), a lightweight **Python Flask integration bridge**, and a modern, responsive **web telemetry dashboard**.

---

## 1. Problem Statement

The Dining Philosophers problem, originally formulated by Edsger W. Dijkstra in 1965, models the synchronization challenges that arise when multiple concurrent processes compete for a shared, finite set of reusable resources.

Five philosophers sit around a circular table. Between each pair of philosophers lies exactly one shared fork. Each philosopher alternates between thinking and eating. To eat, a philosopher requires exclusive possession of **both adjacent forks** simultaneously.

If every philosopher simultaneously picks up their left fork and waits for their right fork, all philosophers hold one resource while waiting for the other in a closed ring. This causes **deadlock**:
$$P_0 \to F_1 \to P_1 \to F_2 \to P_2 \to F_3 \to P_3 \to F_4 \to P_4 \to F_0 \to P_0$$

This project implements a reliable deadlock prevention strategy using POSIX synchronization primitives to permanently eliminate the Circular Wait condition while maximizing safe concurrency.

---

## 2. Key Objectives & Learning Outcomes

- Understand **deadlock** and Coffman's four necessary conditions: *Mutual Exclusion*, *Hold and Wait*, *No Preemption*, and *Circular Wait*.
- Master POSIX threads (`pthread_t`), mutual exclusion locks (`pthread_mutex_t`), and counting semaphores (`sem_t`).
- Contrast **Deadlock Prevention** with **Deadlock Avoidance** (Banker's Algorithm) and **Deadlock Detection**.
- Differentiate real kernel-level multi-threading from single-threaded UI animations.
- Analyze synchronization fairness, throughput, and starvation avoidance.

---

## 3. System Architecture

The project employs a clean 3-tier decoupled architecture:

```
┌─────────────────────────────────────────────────────────────┐
│                    WEB PRESENTATION TIER                    │
│   HTML5 Canvas/SVG Arena │ Chart.js Analytics │ Event Log   │
└──────────────────────────────┬──────────────────────────────┘
                               │ HTTP REST & SSE Stream
┌──────────────────────────────▼──────────────────────────────┐
│                  INTEGRATION & CONTROL TIER                 │
│        Python Flask Server (backend/server.py)              │
│   Process Lifecycle │ Command Pipe │ JSON Stream Buffer     │
└──────────────────────────────┬──────────────────────────────┘
                               │ stdin/stdout Pipes
┌──────────────────────────────▼──────────────────────────────┐
│                 SIMULATION CONCURRENCY ENGINE               │
│               POSIX C Executable (GCC / Linux)              │
│   Philosopher Threads (pthread) │ Fork Mutexes (mutex)      │
│   Permit Counting Semaphore     │ Condition Variables       │
└─────────────────────────────────────────────────────────────┘
```

---

## 4. Deadlock Prevention Strategy

Our system breaks Coffman's fourth necessary condition—**Circular Wait**—via concurrency throttling using a counting semaphore.

### Algorithm Steps:
1. Initialize a POSIX counting semaphore (`permit_semaphore`) to $N - 1$ (e.g., 4 permits for 5 philosophers).
2. Before requesting any forks, a philosopher thread must acquire a permit:
   ```c
   sem_wait(&permit_semaphore);
   ```
3. The philosopher locks their left fork mutex: `pthread_mutex_lock(&fork_mutexes[left])`.
4. The philosopher locks their right fork mutex: `pthread_mutex_lock(&fork_mutexes[right])`.
5. The philosopher eats, incrementing their meal counter.
6. The philosopher unlocks their right fork mutex: `pthread_mutex_unlock(&fork_mutexes[right])`.
7. The philosopher unlocks their left fork mutex: `pthread_mutex_unlock(&fork_mutexes[left])`.
8. The philosopher returns the semaphore permit:
   ```c
   sem_post(&permit_semaphore);
   ```

### Formal Proof of Deadlock Freedom (Pigeonhole Principle):
- Let $N$ be the number of philosophers and forks.
- At most $N - 1$ philosophers are permitted to hold permits and compete for forks simultaneously.
- If all competing philosophers hold 1 fork, the total number of forks held is at most $N - 1$.
- Since there are $N$ total forks, at least one fork $F_{\text{free}} = N - (N - 1) = 1$ remains unheld on the table.
- The philosopher adjacent to $F_{\text{free}}$ can acquire it, eat, and release both forks, unblocking neighbors.
- Therefore, a permanent circular wait cannot form. $\blacksquare$

---

## 5. Technology Stack

- **Backend Simulation Engine:** C11, POSIX Threads (`pthread`), Counting Semaphores (`semaphore.h`), Mutexes (`pthread_mutex_t`), GCC Compiler.
- **Integration Layer:** Python 3.8+, Flask, Server-Sent Events (SSE), Subprocess Pipes.
- **Frontend Presentation:** HTML5, CSS3 (Dark/Light themes), Vanilla JavaScript (ES6+), Chart.js.
- **Build System:** GNU Make (`Makefile` with `-Wall -Wextra -Wpedantic -O2 -std=c11 -pthread -lrt`).
- **Supported Environments:** Ubuntu Linux, Debian, or WSL2 (Windows Subsystem for Linux) on Windows 10/11.

---

## 6. Directory Structure

```
dining-philosophers-deadlock-prevention/
├── README.md                     # Comprehensive project documentation
├── .gitignore                    # Git ignore file for build & cache artifacts
├── Makefile                      # Strict build system (all, run, test, clean)
├── LICENSE                       # MIT Open Source License
├── src/
│   ├── dining_philosophers.c     # CLI driver, stdin command listener, entry point
│   ├── simulation.h              # Data structures, enums, API signatures
│   └── simulation.c              # POSIX threads, semaphores, mutexes, event streaming
├── backend/
│   ├── server.py                 # Flask REST API & SSE event stream integration
│   └── requirements.txt          # Python dependencies (Flask, flask-cors)
├── frontend/
│   ├── index.html                # Responsive multi-tab visual dashboard
│   ├── style.css                 # Dark/light theme, circular table styling, animations
│   └── app.js                    # SSE client, circular arena rendering, Chart.js logic
├── tests/
│   ├── test_suite.c              # Automated C test harness verifying 10 invariants
│   ├── run_tests.sh              # Test runner script with timeout controls
│   └── test_cases.md             # Detailed specifications and results for all 10 tests
├── docs/
│   ├── project_report.md         # 24-section comprehensive academic project report
│   ├── system_design.md          # Architecture, sequence diagram, concurrency safety
│   ├── algorithm.md              # Mathematical proof, pseudocode, Coffman analysis
│   ├── viva_questions.md         # 22+ college viva questions and concise answers
│   └── user_manual.md            # Detailed user manual and troubleshooting
├── screenshots/
│   └── README.md                 # Screenshot placeholders and execution visual guide
└── presentation/
    └── presentation_outline.md   # 9-slide academic presentation with speaker notes
```

---

## 7. Installation & Compilation Instructions

### Prerequisites
On Ubuntu Linux or WSL2 on Windows, install GCC, Make, and Python:
```bash
sudo apt update
sudo apt install -y build-essential python3 python3-pip
```

### Build the C Simulation Engine
```bash
make clean
make
```
This compiles the C simulation engine with strict compiler warnings into `build/dining_philosophers`.

### Run the Automated Invariant Test Suite
```bash
make test
```

---

## 8. Execution Instructions

### Option A: Interactive Web Application (Recommended for Demonstration)
1. Install Python dependencies:
   ```bash
   pip install -r backend/requirements.txt
   ```
2. Start the local integration server:
   ```bash
   python backend/server.py
   ```
3. Open your browser and navigate to:
   ```
   http://localhost:5000
   ```

### Option B: Direct Command-Line Interface (CLI)
You can execute the C simulation engine directly in the terminal:
```bash
# Run with 5 philosophers, 3 meals each, delay 400ms:
./build/dining_philosophers -n 5 -m 3 -s 0 -d 400

# Run Unsafe Circular Wait demonstration:
./build/dining_philosophers -n 5 -m 1 -s 1 -d 300

# Run in interactive mode with stdin commands:
./build/dining_philosophers -n 5 -m 5 -s 0 -d 500 --interactive
```

Interactive commands when `--interactive` is enabled:
- `PAUSE`: Pauses active simulation threads.
- `RESUME`: Resumes all threads.
- `STEP`: Executes a single state transition step.
- `SPEED <ms>`: Dynamically adjusts delay (e.g. `SPEED 200`).
- `STOP`: Gracefully terminates threads and prints summary.

---

## 9. Automated Test Results

The test harness in `tests/test_suite.c` verifies 10 critical synchronization invariants:

```
=====================================================================
   DINING PHILOSOPHERS DEADLOCK PREVENTION - AUTOMATED TEST HARNESS  
=====================================================================
Executing 10 Comprehensive Synchronization Invariant & Feature Tests:

  [PASS] Test 1: Normal Execution                   : All 5 philosophers completed 2 meals
  [PASS] Test 2: Deadlock Prevention                : Completed 4 meals with 0 deadlocks
  [PASS] Test 3: Mutual Exclusion Invariant         : No fork was ever held by >1 philosopher
  [PASS] Test 4: Resource Release Invariant         : All 5 forks & 4 permits safely released
  [PASS] Test 5: Repeated Simulation                : 3 consecutive full runs completed cleanly
  [PASS] Test 6: Invalid Config Rejection           : Properly rejected N=1, N=15, and meals=0
  [PASS] Test 7: Unsafe Scenario Demonstration      : Circular wait detected and unblocked safely
  [PASS] Test 8: Concurrent Activity                : Observed >=2 philosophers eating concurrently
  [PASS] Test 9: Fast Clean Shutdown                : Terminated threads cleanly within < 1000 ms
  [PASS] Test 10: Stress Testing Matrix             : Passed configs N=3, 5, 7, 10 with varied speeds

=====================================================================
TEST SUMMARY: 10 Total | 10 Passed | 0 Failed
=====================================================================
```

---

## 10. Sample Terminal Output

```
Initializing Dining Philosophers Simulation...
Philosophers: 5 | Meals: 3 | Strategy: Deadlock Prevention (N-1 Semaphore) | Delay: 400 ms

[0 ms][Step 1  ] Event: INIT               | Phil -1: -            | Permits: 4 | Simulation context initialized successfully.
[0 ms][Step 2  ] Event: STATE_CHANGE       | Phil 0: THINKING     | Permits: 4 | Philosopher 0 is thinking.
[0 ms][Step 3  ] Event: STATE_CHANGE       | Phil 1: THINKING     | Permits: 4 | Philosopher 1 is thinking.
[401 ms][Step 7  ] Event: STATE_CHANGE       | Phil 0: HUNGRY       | Permits: 4 | Philosopher 0 is hungry and waiting for resources.
[401 ms][Step 8  ] Event: PERMIT_ACQUIRED    | Phil 0: HUNGRY       | Permits: 3 | Philosopher 0 acquired a semaphore permit (3 permits remaining).
[401 ms][Step 9  ] Event: FORK_ACQUIRED      | Phil 0: HUNGRY       | Permits: 3 | Philosopher 0 acquired left fork 0.
[401 ms][Step 10 ] Event: FORK_ACQUIRED      | Phil 0: HUNGRY       | Permits: 3 | Philosopher 0 acquired right fork 1.
[401 ms][Step 11 ] Event: STATE_CHANGE       | Phil 0: EATING       | Permits: 3 | Philosopher 0 is eating meal 1 of 3 (waited 0 ms).
...
=======================================================
               SIMULATION EXECUTION SUMMARY            
=======================================================
Mode: Deadlock Prevention (N-1 Semaphore)
Deadlock Occurred: NO (Prevented Successfully)
Total Simulation Steps: 142
Runtime: 4.82 seconds
Philosopher Statistics:
  Phil 0: 3/3 meals | Total Wait: 120 ms | State: COMPLETED
  Phil 1: 3/3 meals | Total Wait: 165 ms | State: COMPLETED
  Phil 2: 3/3 meals | Total Wait: 140 ms | State: COMPLETED
  Phil 3: 3/3 meals | Total Wait: 180 ms | State: COMPLETED
  Phil 4: 3/3 meals | Total Wait: 155 ms | State: COMPLETED
=======================================================
```

---

## 11. Troubleshooting

- **WSL Path Warning on Windows:** If using Windows, ensure WSL2 Ubuntu is installed (`wsl --install`). Running `python backend/server.py` automatically routes execution to WSL without manual intervention.
- **Port Conflict:** If port 5000 is occupied by another service:
  ```bash
  PORT=8080 python backend/server.py
  ```
- **Permission Denied on Binaries:** Ensure execution permissions are set:
  ```bash
  chmod +x build/dining_philosophers tests/run_tests.sh
  ```

---

## 12. Limitations & Future Enhancements

### Limitations
- **Conservative Concurrency:** Restricting competing diners to $N - 1$ ensures deadlock freedom, but caps maximum concurrent eaters at $\lfloor N/2 \rfloor$.
- **Centralized Semaphore:** The global counting semaphore can become a synchronization bottleneck for very large $N$ ($N > 1000$).

### Future Enhancements
- **Starvation Prevention via Aging:** Add dynamic priority weighting where philosophers with longer wait times receive expedited permits.
- **Alternative Algorithms:** Implement Chandy/Misra asymmetric odd-even fork hierarchy as an in-app comparison mode.
- **Distributed Actor Simulation:** Port the synchronization logic to distributed actor systems (e.g. Erlang or Akka).

---

## 13. Team Member Contributions

| Team Member | Role | Key Contributions |
| :--- | :--- | :--- |
| **[Member 1 Name]** | Systems & C Engineer | Implemented POSIX thread engine, mutex locking, semaphores, and Makefile. |
| **[Member 2 Name]** | Backend & Integration Engineer | Built Flask REST API, Server-Sent Events (SSE) bridge, and process supervisor. |
| **[Member 3 Name]** | Frontend & UI/UX Designer | Developed circular dining arena, Chart.js analytics, and event filter UI. |
| **[Member 4 Name]** | QA & Documentation Lead | Authored 10 automated test cases, 24-section report, and viva guide. |

---

## 14. License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for complete details.
