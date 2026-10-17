# Application Execution Screenshots & Visual Demonstration Guide

This directory contains placeholders and documentation for live application execution screenshots captured from the **Dining Philosophers Deadlock Prevention System**.

---

## Screenshot Inventory

### 1. `dashboard_normal_run.png`
- **Description:** Complete view of the interactive web dashboard during active simulation under **Deadlock Prevention (N-1 Semaphore)** mode.
- **Key Visual Elements:**
  - Control bar configured with 5 philosophers and target 5 meals.
  - Semaphore permits counter showing active permit acquisition (`3 / 4` or `2 / 4`).
  - Circular arena showing active philosophers with state badges (`THINK`, `HUNGRY`, `EATING`).
  - Real-time POSIX event stream in the terminal window.

---

### 2. `circular_arena_eating.png`
- **Description:** High-resolution close-up of the circular dining table showing **concurrent eating**.
- **Key Visual Elements:**
  - Philosopher 0 and Philosopher 2 simultaneously in the `EATING` state (highlighted in emerald green with eating glow).
  - Forks 0, 1, 2, and 3 held (crimson indicator with active owner pointer).
  - Fork 4 available on the table (metallic gray).
  - Proves that non-adjacent philosophers eat in parallel without contention.

---

### 3. `unsafe_deadlock_detected.png`
- **Description:** Visual representation of the educational **Unsafe Scenario (Hold and Wait Circular Deadlock)**.
- **Key Visual Elements:**
  - Red warning banner: `CIRCULAR WAIT DEADLOCK OCCURRED!`.
  - All 5 philosopher nodes in the `DEADLOCKED` state (crimson alert pulse).
  - Each philosopher holding their left fork while waiting for the right fork.
  - Watchdog timer notification in the event log showing automated safe resolution.

---

### 4. `statistics_charts.png`
- **Description:** The **Live Statistics** tab displaying real-time Chart.js analytics.
- **Key Visual Elements:**
  - Bar chart showing meals completed per philosopher, verifying fairness.
  - Bar chart showing cumulative waiting times in milliseconds.
  - Detailed resource utilization table with average wait per meal and acquisition counts.

---

### 5. `test_suite_passing.png`
- **Description:** Terminal view showing execution of the automated test suite (`make test`).
- **Key Visual Elements:**
  - All 10 invariant tests displaying green `[PASS]` tags.
  - Test summary line: `10 Total | 10 Passed | 0 Failed`.

---

## How to Capture Screenshots

1. Launch the web application:
   ```bash
   python backend/server.py
   ```
2. Open your browser to `http://localhost:5000`.
3. Use your operating system's screenshot utility (e.g. `Win + Shift + S` on Windows, or `gnome-screenshot` on Linux) to capture each view.
4. Save the images into this directory using the names listed above.
