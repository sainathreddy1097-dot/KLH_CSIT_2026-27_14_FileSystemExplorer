# ACADEMIC PROJECT REPORT

# DINING PHILOSOPHERS PROBLEM USING DEADLOCK PREVENTION
### A Real-Time POSIX Multi-Threaded Synchronization & Interactive Visual Simulation System

---

## 1. Title Page

- **Project Title:** Dining Philosophers Problem Using Deadlock Prevention
- **Domain:** Operating Systems, Concurrency Control, Multi-threading & Synchronization
- **Technologies:** C (C11, POSIX Threads, Semaphores, Mutexes), Python (Flask, REST, SSE), HTML5/CSS3, JavaScript (ES6, Chart.js)
- **Academic Year:** 2025 – 2026
- **Target Audience:** Computer Science & Engineering Undergraduate Evaluators, Viva Examiners, and Operating Systems Students

---

## 2. Abstract

The Dining Philosophers problem, originally conceived by Edsger W. Dijkstra in 1965, represents one of the foundational synchronization paradigms in computer science. It abstracts the challenge of concurrent resource allocation among independent processes competing for shared, reusable resources without encountering deadlocks, race conditions, or starvation.

This project delivers a complete, production-grade academic implementation consisting of a high-performance C simulation engine built upon POSIX threads (`pthread`), mutual exclusion locks (`pthread_mutex_t`), and counting semaphores (`sem_t`). Deadlock freedom is guaranteed by breaking Coffman's fourth necessary condition—Circular Wait—using an $N - 1$ counting semaphore throttle based on the Pigeonhole Principle. An educational Unsafe Mode deterministically demonstrates circular wait with an automated watchdog monitor to preserve system stability. A lightweight integration layer communicates real-time synchronization events to an interactive web dashboard rendering a circular dining arena, real-time Chart.js analytics, dynamic event streams, and an automated 10-scenario invariant test harness. Comprehensive tests verify 100% adherence to mutual exclusion, deadlock prevention, and clean thread lifecycle management.

---

## 3. Introduction

Operating systems are fundamentally responsible for arbitrating hardware resources among concurrent, asynchronously executing tasks. When multiple threads simultaneously request exclusive access to overlapping sets of resources, synchronization discipline is required. Without rigorous protocols, concurrent systems can fall into catastrophic failure modes:

1. **Deadlock:** A state of circular waiting where no thread can proceed.
2. **Race Conditions:** Non-deterministic data corruption caused by unsynchronized concurrent access to shared memory.
3. **Starvation:** Indefinite postponement of a process due to biased scheduling or resource allocation.

The Dining Philosophers problem provides a clear and rigorous conceptual framework to study, demonstrate, and solve these concurrency hazards using standard POSIX synchronization primitives.

---

## 4. Problem Statement

Five philosophers sit around a circular table, with one fork placed between each adjacent pair. Each philosopher alternates between thinking and eating. To eat, a philosopher requires exclusive possession of **both adjacent forks** simultaneously.

If every philosopher simultaneously picks up their left fork and waits indefinitely for their right fork, all philosophers hold one resource while waiting for another in a closed ring. This causes **deadlock**:
$$P_0 \to F_1 \to P_1 \to F_2 \to P_2 \to F_3 \to P_3 \to F_4 \to P_4 \to F_0 \to P_0$$

The objective of this project is to implement a robust, testable, and demonstrable solution in C that enforces **Deadlock Prevention** while maximizing concurrency and preventing system lockups.

---

## 5. Objectives

1. **Deadlock Prevention Implementation:** Implement Dijkstra's $N - 1$ counting semaphore strategy in C using POSIX primitives.
2. **Mutual Exclusion Guarantee:** Ensure no two philosophers can hold the same fork simultaneously.
3. **Circular Wait Elimination:** Provide a mathematical proof and empirical demonstration that circular wait is broken.
4. **Interactive Dashboard:** Build an interactive visual representation of the dining table, fork states, and philosopher transitions.
5. **Real-time Event Streaming:** Implement a safe communication bridge streaming real C pthread events to the frontend via SSE.
6. **Educational Unsafe Demonstration:** Provide a controlled, deterministic circular wait demonstration equipped with a watchdog timer.
7. **Automated Verification:** Build a 10-scenario automated test harness validating all synchronization invariants.

---

## 6. Existing System and Its Limitations

In standard academic textbooks, the Dining Philosophers problem is frequently presented via static pseudocode or high-level JavaScript animations that simulate state machines without executing actual multi-threaded OS primitives.

### Limitations of Existing Approaches:
- **Lack of True Concurrency:** High-level browser animations use `setTimeout` or `async/await` on a single-threaded JavaScript event loop, concealing kernel context switches, thread preemption, and real lock contention.
- **Unsafe Demonstrations:** Naive implementations of the circular wait deadlock freeze the host process or operating system, requiring force-quits.
- **Absence of Quantitative Verification:** Most implementations lack rigorous invariant validation, such as checking whether two threads hold the same lock or verifying clean thread shutdown.

---

## 7. Proposed System

The proposed system addresses these gaps by implementing a decoupled, multi-tiered architecture:
1. **Core Simulation Engine in C:** Uses native POSIX threads, counting semaphores, mutexes, condition variables, and thread barriers.
2. **Safe Integration Layer in Python:** Spawns and manages the C process, providing bidirectional control via REST and streaming telemetry via Server-Sent Events (SSE).
3. **Modern Web Interface:** Provides an interactive circular dining arena, real-time Chart.js graphs, filterable event logs, and an interactive viva preparation bank.
4. **Automated Test Harness:** Runs 10 comprehensive tests directly against the compiled C binary.

---

## 8. Requirements Analysis

### Functional Requirements
- Support configurable philosophers ($N = 2$ to $10$, default $5$).
- Support configurable meal limits ($1$ to $50$).
- Enforce $N - 1$ counting semaphore prevention strategy.
- Provide interactive controls: Start, Pause, Resume, Step, Speed, and Reset with confirmation.
- Emit structured timestamped events for every state transition and lock event.
- Provide safe Unsafe Mode with a watchdog monitor thread.

### Non-Functional Requirements
- **Safety:** Prevent data races by guarding shared state with mutexes.
- **Responsiveness:** Ensure thread cancellation and reset complete in $< 1.0\text{ second}$.
- **Portability:** Compile under GCC on Linux, Ubuntu, and WSL2 on Windows.
- **Usability:** Provide accessible light/dark themes and tooltips.

---

## 9. Hardware and Software Requirements

### Hardware Requirements
- **Processor:** Dual-core 2.0 GHz or higher (x86_64 or ARM64).
- **RAM:** Minimum 2 GB (4 GB recommended).
- **Storage:** 100 MB free disk space.

### Software Requirements
- **Operating System:** Ubuntu Linux 20.04+, Debian, or Windows 10/11 with WSL2.
- **C Compiler:** GCC 9.3+ with POSIX Threads (`-pthread`) and Real-time library (`-lrt`).
- **Build System:** GNU Make 4.0+.
- **Backend:** Python 3.8+ with `Flask` and `flask-cors`.
- **Frontend:** Any modern browser supporting HTML5, CSS Grid, and EventSource (SSE).

---

## 10. System Architecture

The project architecture comprises three decoupled layers:

```
[ Web Browser Dashboard ]
        │  ▲
        │  │ HTTP REST & SSE
        ▼  │
[ Flask Backend Server ]
        │  ▲
        │  │ stdin commands / stdout JSON
        ▼  │
[ POSIX C Simulation Engine ]
  ├─ Philosopher Threads (pthreads)
  ├─ Fork Mutexes (pthread_mutex_t)
  ├─ Permit Semaphore (sem_t)
  └─ Monitor Watchdog Thread
```

---

## 11. Operating Systems Concepts

1. **Process vs. Thread:** The C engine runs as a single OS process hosting $N$ concurrent philosopher worker threads and 1 monitor watchdog thread sharing memory.
2. **Mutual Exclusion:** Enforced on each fork using binary mutex locks (`pthread_mutex_t`), guaranteeing that at most one thread can access the critical section (eating with that fork).
3. **Counting Semaphores:** Maintains an atomic counter initialized to $N - 1$. The `sem_wait()` call decrements the permit counter, blocking if 0 permits remain.
4. **Condition Variables:** Allows threads to suspend execution efficiently (`pthread_cond_wait`) while paused without wasting CPU cycles.
5. **Thread Barriers:** Synchronizes all philosopher threads in Unsafe Mode so they acquire their left fork simultaneously.

---

## 12. Algorithm and Pseudocode

### Philosopher Thread Routine
```c
void* philosopher_thread(void* arg) {
    int id = *(int*)arg;
    int left_fork = id;
    int right_fork = (id + 1) % num_philosophers;

    while (!stop_requested && meals_eaten < max_meals) {
        // Thinking
        transition_state(id, STATE_THINKING);
        sleep_ms(speed_ms);

        // Hungry
        transition_state(id, STATE_HUNGRY);

        // Deadlock Prevention: Acquire permit
        sem_wait(&permit_semaphore);

        // Acquire forks
        pthread_mutex_lock(&fork_mutexes[left_fork]);
        pthread_mutex_lock(&fork_mutexes[right_fork]);

        // Eating (Critical Section)
        transition_state(id, STATE_EATING);
        meals_eaten++;
        sleep_ms(speed_ms);

        // Release forks
        pthread_mutex_unlock(&fork_mutexes[right_fork]);
        pthread_mutex_unlock(&fork_mutexes[left_fork]);

        // Release permit
        sem_post(&permit_semaphore);
    }
    transition_state(id, STATE_COMPLETED);
    return NULL;
}
```

---

## 13. Deadlock Prevention Explanation & Proof

### The Four Coffman Conditions:
1. **Mutual Exclusion:** Necessary for physical forks.
2. **Hold and Wait:** Philosophers hold left fork while requesting right fork.
3. **No Preemption:** Preempting held mutexes in user space risks memory corruption.
4. **Circular Wait:** Successfully eliminated by the $N - 1$ semaphore!

### Mathematical Proof by Contradiction:
1. Assume a deadlock occurs where every competing philosopher holds one fork and is blocked waiting for another.
2. Let $k$ be the number of philosophers permitted to compete. By design, $k \le N - 1$.
3. Since each competing philosopher holds 1 fork, the total number of held forks is at most $N - 1$.
4. There are $N$ total forks on the table. The number of free forks is:
   $$\text{Free Forks} = N - (N - 1) = 1$$
5. At least one fork $F_{\text{free}}$ is unheld. The philosopher waiting for $F_{\text{free}}$ can acquire it, eat, and release both forks.
6. This contradicts the premise of deadlock. Hence, deadlock cannot occur. $\blacksquare$

---

## 14. Implementation Details

- **Language Standard:** C11 (`-std=c11`) with POSIX extensions (`_POSIX_C_SOURCE 200809L`).
- **Thread Management:** Worker threads are created with `pthread_create()` and collected with `pthread_join()`.
- **Event Streaming:** Events are structured as single-line JSON objects with timestamps, philosopher IDs, fork ownership arrays, permit counters, and human-readable messages.
- **Thread-Safe Pause:** Condition variable `pause_cond` checked at safe checkpoints outside fork critical sections.

---

## 15. Source Code Overview

- `src/simulation.h`: Data structures (`SimulationContext`, `PhilStats`), event and state enums, API function signatures.
- `src/simulation.c`: Core implementation of philosopher threads, mutex locking, counting semaphore operations, pause/step condition variable, watchdog monitor, and event formatting.
- `src/dining_philosophers.c`: CLI entry point, argument parsing, interactive stdin command controller, and signal handling.
- `backend/server.py`: Python Flask REST API and SSE event dispatcher.
- `frontend/index.html`: Responsive multi-tab dashboard layout.
- `frontend/style.css`: Modern dark/light theme, circular arena, and state animations.
- `frontend/app.js`: Client-side state manager, SSE listener, Chart.js integrations, and viva accordion.
- `tests/test_suite.c`: Automated test harness testing 10 synchronization invariants.

---

## 16. User Interface Description

- **Navigation Bar:** Tab switching between Dashboard, Algorithm, Deadlock Analysis, Statistics, Test Suite, and Viva Guide. Includes real-time C engine connection indicator and light/dark theme toggle.
- **Control Bar:** Dropdowns for strategy and philosopher count, meal target input, speed slider, and control buttons (Start, Pause, Resume, Step, Reset).
- **Circular Dining Arena:** Renders dynamic philosopher nodes around the table with state colors, avatars, and animated fork items showing ownership or free status.
- **Live Event Stream:** Color-coded terminal display with filters by philosopher and event type, search box, and auto-scroll toggle.
- **Analytics View:** Live Chart.js bar charts showing meals completed and cumulative waiting times per philosopher.

---

## 17. Test Cases and Results

All 10 automated test cases were executed directly against the compiled C binary (`./build/test_suite`):

| Test ID | Test Name | Assertion | Result |
| :---: | :--- | :--- | :---: |
| T01 | Normal Execution | All 5 philosophers complete 2 meals | **PASS** |
| T02 | Deadlock Prevention | Zero deadlocks observed over 4 full meal rounds | **PASS** |
| T03 | Mutual Exclusion | No fork is held by $>1$ philosopher simultaneously | **PASS** |
| T04 | Resource Release | All 5 forks and 4 permits safely released upon completion | **PASS** |
| T05 | Repeated Simulation | 3 consecutive runs complete without resource leaks | **PASS** |
| T06 | Invalid Config Rejection | Correctly rejects $N=1$, $N=15$, and meals $=0$ | **PASS** |
| T07 | Unsafe Scenario Demo | Circular wait detected and safely unblocked by watchdog | **PASS** |
| T08 | Concurrent Activity | Multiple non-adjacent philosophers eat simultaneously | **PASS** |
| T09 | Fast Clean Shutdown | Threads terminated and joined in $< 1000\text{ ms}$ | **PASS** |
| T10 | Stress Testing Matrix | Passed matrix of $N=3, 5, 7, 10$ under high-speed delays | **PASS** |

**Summary: 10 Total | 10 Passed | 0 Failed**

---

## 18. Screenshots of Actual Execution

Below are references to execution views captured during live runs:
1. `screenshots/dashboard_normal_run.png`: Full dashboard showing 5 philosophers dining under Deadlock Prevention mode with 4 semaphore permits.
2. `screenshots/circular_arena_eating.png`: Arena close-up showing non-adjacent philosophers (Phil 0 and Phil 2) eating concurrently with green status.
3. `screenshots/unsafe_deadlock_detected.png`: Warning banner and crimson nodes showing detected circular wait in educational Unsafe Mode.
4. `screenshots/statistics_charts.png`: Chart.js bar charts illustrating meal fairness and waiting time distribution.
5. `screenshots/test_suite_passing.png`: Terminal output showing all 10 automated invariant tests passing.

---

## 19. Advantages

1. **Provable Deadlock Freedom:** Eliminates circular wait mathematically via counting semaphores.
2. **True Kernel Multi-threading:** Uses native POSIX threads and kernel synchronization.
3. **Responsive Controls:** Safe pause, resume, and step without deadlock hazards.
4. **Safe Educational Demonstration:** Deterministic circular wait demo that never freezes host processes.
5. **Zero External Cloud Dependencies:** Runs 100% locally on standard Linux/WSL environments.

---

## 20. Limitations

1. **Conservative Concurrency:** Limiting competing diners to $N - 1$ slightly underutilizes table capacity when table configurations might support $\lfloor N/2 \rfloor$ diners.
2. **Centralized Semaphore:** Requires a global counting semaphore, which can introduce lock contention at very large scale ($N > 1000$).
3. **No Strict FIFO Ordering:** Simple mutexes do not guarantee strict arrival-order fairness, allowing occasional variance in waiting times.

---

## 21. Applications

- **Database Transaction Management:** Strict 2-Phase Locking (2PL) and transaction concurrency control.
- **Operating System Device Drivers:** Managing shared memory ring buffers and hardware DMA channels.
- **Network Routing & Switching:** Preventing packet deadlock in circular buffer rings.
- **Multi-Core Microprocessors:** Cache coherency protocols (MESI/MOESI) arbitrating shared cache lines.

---

## 22. Future Enhancements

1. **Resource Hierarchy Algorithm:** Add Chandy/Misra or asymmetric odd-even philosopher algorithms as selectable options.
2. **Starvation Prevention (Aging):** Implement priority aging where philosophers waiting longest receive priority semaphore grants.
3. **Distributed Actor Model:** Re-implement using Erlang/Elixir or Go channels to demonstrate distributed actor concurrency.

---

## 23. Conclusion

The **Dining Philosophers Problem Using Deadlock Prevention** project successfully implements, validates, and visualizes a classic Operating Systems synchronization challenge. By using POSIX threads, counting semaphores, and mutex locks in C, the system breaks the circular wait condition and guarantees deadlock-free execution. The interactive web dashboard and automated 10-scenario test suite provide students, instructors, and evaluators with a complete, transparent, and rigorous pair of theoretical and practical concurrency tools.

---

## 24. References

1. Dijkstra, E. W. (1965). *Cooperating Sequential Processes*. Technological University, Eindhoven.
2. Coffman, E. G., Elphick, M., & Shoshani, A. (1971). *System Deadlocks*. ACM Computing Surveys (CSUR), 3(2), 67-78.
3. Silberschatz, A., Galvin, P. B., & Gagne, G. (2018). *Operating System Concepts* (10th ed.). Wiley.
4. Tanenbaum, A. S., & Bos, H. (2015). *Modern Operating Systems* (4th ed.). Pearson.
5. IEEE and The Open Group. (2018). *The Open Group Base Specifications Issue 7 / IEEE Std 1003.1-2017 (POSIX.1)*.
