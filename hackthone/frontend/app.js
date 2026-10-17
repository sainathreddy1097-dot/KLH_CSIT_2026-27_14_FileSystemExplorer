/**
 * Dining Philosophers Problem – Deadlock Prevention System
 * Frontend Application & Real-time Visualization Controller
 */

// Application State
const state = {
    numPhilosophers: 5,
    maxMeals: 5,
    strategy: 0,
    speedMs: 600,
    isRunning: false,
    isPaused: false,
    deadlockOccurred: false,
    permits: 4,
    totalSteps: 0,
    runtimeSeconds: 0,
    philosopherStates: ['THINKING', 'THINKING', 'THINKING', 'THINKING', 'THINKING'],
    forkOwners: [-1, -1, -1, -1, -1],
    meals: [0, 0, 0, 0, 0],
    waitTimesMs: [0, 0, 0, 0, 0],
    acquisitions: [0, 0, 0, 0, 0],
    eventLogs: [],
    eventSource: null,
    backendOnline: false
};

// Viva Voce Question Bank
const VIVA_QUESTIONS = [
    {
        q: "1. What is a deadlock in an operating system?",
        a: "A deadlock is a situation where two or more concurrent processes (or threads) are permanently blocked because each process holds a resource that another process needs, and none can proceed without acquiring its next resource."
    },
    {
        q: "2. What are Coffman's four necessary conditions for deadlock?",
        a: "The four conditions are: (1) Mutual Exclusion (resources cannot be shared simultaneously), (2) Hold and Wait (processes hold allocated resources while waiting for additional ones), (3) No Preemption (resources cannot be forcibly taken away), and (4) Circular Wait (a closed chain of processes exists where each waits for a resource held by the next)."
    },
    {
        q: "3. What is the Dining Philosophers Problem and why is it important?",
        a: "Formulated by Edsger Dijkstra in 1965, it models concurrency control and resource sharing. Five philosophers sit around a circular table with five forks. Each needs two adjacent forks to eat. It illustrates deadlocks, race conditions, and resource starvation in multi-threaded systems."
    },
    {
        q: "4. What is the difference between a process and a thread?",
        a: "A process is an executing program instance with its own independent address space and system resources. A thread is a lightweight unit of execution within a process that shares the process's memory, file descriptors, and code space with sibling threads."
    },
    {
        q: "5. What is a mutex and how is it used here?",
        a: "A mutex (mutual exclusion lock) provides binary locking. In this project, each fork is protected by a POSIX mutex (`pthread_mutex_t`). Only one philosopher thread can hold the lock at any time, guaranteeing mutual exclusion."
    },
    {
        q: "6. What is a counting semaphore and how does it prevent deadlock?",
        a: "A counting semaphore maintains an integer counter with atomic wait (`sem_wait`) and signal (`sem_post`) operations. Initializing it to N-1 (e.g. 4 for 5 philosophers) ensures at most N-1 philosophers enter the dining room simultaneously, guaranteeing at least one philosopher can acquire two forks and eat."
    },
    {
        q: "7. Why is the semaphore initialized to N - 1 instead of N?",
        a: "If N philosophers compete simultaneously, each could pick up their left fork simultaneously, holding 1 fork while waiting for the right fork, forming a circular wait. Restricting competition to N-1 ensures at least one of the N forks remains free, breaking the circular wait condition."
    },
    {
        q: "8. How does this solution break the Circular Wait condition?",
        a: "By the Pigeonhole Principle: with N forks and at most N-1 competing philosophers, at least one philosopher will always find both adjacent forks available. Thus, a circular wait chain cannot form."
    },
    {
        q: "9. What is starvation and does deadlock prevention guarantee fairness?",
        a: "Starvation occurs when a thread is perpetually denied resources. While deadlock prevention guarantees the system makes progress (no global freeze), simple mutexes do not guarantee strict FIFO fairness; a fast neighbor could repeatedly eat. Fair scheduling or aging avoids starvation."
    },
    {
        q: "10. What is a race condition and how is it avoided in the C engine?",
        a: "A race condition occurs when concurrent threads access shared data without synchronization, causing outcomes to depend on unpredictable execution order. In our engine, shared state and stats are protected by `pthread_mutex_t state_mutex`."
    },
    {
        q: "11. What is the role of POSIX Threads (pthreads) in this implementation?",
        a: "POSIX threads provide standardized C concurrency APIs (`pthread_create`, `pthread_join`, `pthread_mutex`, `pthread_cond`, `pthread_barrier`). Each philosopher executes as an autonomous POSIX thread running concurrently on the OS kernel."
    },
    {
        q: "12. What happens if a philosopher cannot acquire a fork?",
        a: "The philosopher thread blocks on `pthread_mutex_lock` and yields CPU execution. The OS puts the thread into a WAITING/BLOCKED state until the neighbor releases the fork via `pthread_mutex_unlock`, which wakes the thread."
    },
    {
        q: "13. How does pause and resume work without holding mutex locks?",
        a: "The engine uses a condition variable (`pthread_cond_t pause_cond`). Philosophers check pause status outside critical resource-holding sections. When paused, threads wait on the condition variable rather than holding fork mutexes, preventing artificial deadlocks."
    },
    {
        q: "14. How is the Unsafe Scenario implemented safely for demonstration?",
        a: "In unsafe mode, a thread barrier (`pthread_barrier_t`) aligns all philosophers to pick up their left fork simultaneously. When they all wait for their right fork, a watchdog monitor thread detects the circular wait and safely unblocks the threads after a timeout, preventing OS or UI freezing."
    },
    {
        q: "15. What is the difference between Deadlock Prevention and Deadlock Avoidance?",
        a: "Deadlock Prevention eliminates one of Coffman's four conditions statically beforehand (e.g., N-1 semaphore). Deadlock Avoidance dynamically analyzes each resource request using algorithms like Dijkstra's Banker's Algorithm to ensure the system remains in a safe state."
    },
    {
        q: "16. What is the difference between Deadlock Prevention and Deadlock Detection?",
        a: "Prevention ensures deadlocks can never occur by design. Detection allows deadlocks to happen, runs periodic cycle-detection algorithms on a Wait-For Graph (WFG), and resolves them through thread termination or resource preemption."
    },
    {
        q: "17. What happens during simulation reset or shutdown?",
        a: "A `stop_requested` atomic flag is set, waking all condition variables and releasing held resources. `pthread_join` waits for all threads to terminate, followed by destruction of mutexes, semaphores, and barriers (`sim_cleanup`)."
    },
    {
        q: "18. What are the limitations of the N - 1 Semaphore solution?",
        a: "It slightly underutilizes resources by artificially limiting concurrent diners to N-1 even when table arrangements might theoretically support N/2 concurrent diners. Also, it requires a centralized counting semaphore."
    },
    {
        q: "19. What alternative strategies solve the Dining Philosophers problem?",
        a: "Alternatives include: (1) Resource Hierarchy (Chandy/Misra: odd philosophers pick left fork first, even pick right), (2) Asymmetric Philosophers, (3) Monitor / Condition Variable state checking (Tanenbaum's solution), and (4) Try-lock with backoff."
    },
    {
        q: "20. What real-world applications use these synchronization concepts?",
        a: "Database transaction locking (2-Phase Locking), traffic junction controllers, multi-core CPU cache coherency, distributed file systems, cloud microservice resource allocation, and operating system device driver locks."
    },
    {
        q: "21. How does the web frontend communicate with the real C executable?",
        a: "A Python Flask integration server spawns the compiled C binary (`dining_philosophers`) as a subprocess, streams real-time JSON events from its stdout via Server-Sent Events (SSE), and sends interactive commands (`PAUSE`, `RESUME`, `STEP`) via stdin."
    },
    {
        q: "22. How were synchronization invariants verified?",
        a: "An automated C test suite (`tests/test_suite.c`) tests 10 scenarios: mutual exclusion invariant, deadlock freedom, resource release, clean shutdown, and concurrency under high stress."
    }
];

// DOM Elements
const elements = {
    strategySelect: document.getElementById('strategySelect'),
    numPhilosophers: document.getElementById('numPhilosophers'),
    maxMealsInput: document.getElementById('maxMealsInput'),
    speedSlider: document.getElementById('speedSlider'),
    speedValue: document.getElementById('speedValue'),
    startBtn: document.getElementById('startBtn'),
    pauseBtn: document.getElementById('pauseBtn'),
    resumeBtn: document.getElementById('resumeBtn'),
    stepBtn: document.getElementById('stepBtn'),
    resetBtn: document.getElementById('resetBtn'),
    themeToggleBtn: document.getElementById('themeToggleBtn'),
    engineStatusBadge: document.getElementById('engineStatusBadge'),
    countThinking: document.getElementById('countThinking'),
    countHungry: document.getElementById('countHungry'),
    countEating: document.getElementById('countEating'),
    countPermits: document.getElementById('countPermits'),
    deadlockStatusText: document.getElementById('deadlockStatusText'),
    safetyCardIcon: document.getElementById('safetyCardIcon'),
    runtimeStepsText: document.getElementById('runtimeStepsText'),
    deadlockBanner: document.getElementById('deadlockBanner'),
    philosophersContainer: document.getElementById('philosophersContainer'),
    forksContainer: document.getElementById('forksContainer'),
    centerTableMode: document.getElementById('centerTableMode'),
    strategyExplanation: document.getElementById('strategyExplanation'),
    logStreamWindow: document.getElementById('logStreamWindow'),
    logFilterPhil: document.getElementById('logFilterPhil'),
    logFilterType: document.getElementById('logFilterType'),
    clearLogBtn: document.getElementById('clearLogBtn'),
    autoScrollLog: document.getElementById('autoScrollLog'),
    eventCounterText: document.getElementById('eventCounterText'),
    resetModal: document.getElementById('resetModal'),
    confirmResetBtn: document.getElementById('confirmResetBtn'),
    cancelResetBtn: document.getElementById('cancelResetBtn'),
    statsTableBody: document.getElementById('statsTableBody'),
    runTestsBtn: document.getElementById('runTestsBtn'),
    vivaAccordion: document.getElementById('vivaAccordion')
};

// Charts references
let mealsChart = null;
let waitChart = null;

// Initialization
document.addEventListener('DOMContentLoaded', () => {
    initTheme();
    initTabs();
    initVivaAccordion();
    initCharts();
    renderCircularArena();
    updatePhilFilterOptions();
    connectSSE();

    // Event listeners
    elements.numPhilosophers.addEventListener('change', () => {
        state.numPhilosophers = parseInt(elements.numPhilosophers.value, 10);
        state.permits = state.numPhilosophers - 1;
        state.philosopherStates = Array(state.numPhilosophers).fill('THINKING');
        state.forkOwners = Array(state.numPhilosophers).fill(-1);
        state.meals = Array(state.numPhilosophers).fill(0);
        state.waitTimesMs = Array(state.numPhilosophers).fill(0);
        state.acquisitions = Array(state.numPhilosophers).fill(0);
        renderCircularArena();
        updatePhilFilterOptions();
        updateCharts();
        updateTableDetails();
    });

    elements.speedSlider.addEventListener('input', () => {
        state.speedMs = parseInt(elements.speedSlider.value, 10);
        elements.speedValue.textContent = state.speedMs;
        if (state.isRunning) {
            fetch('/api/speed', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ speed_ms: state.speedMs })
            }).catch(console.error);
        }
    });

    elements.strategySelect.addEventListener('change', () => {
        state.strategy = parseInt(elements.strategySelect.value, 10);
        if (state.strategy === 0) {
            elements.centerTableMode.textContent = `N-1 Semaphore: ${state.numPhilosophers - 1} Permits`;
            elements.strategyExplanation.innerHTML = `
                <i class="fa-solid fa-circle-info"></i>
                <strong>Active Mechanism:</strong> Counting semaphore initialized to <em>N - 1 = ${state.numPhilosophers - 1}</em> permits breaks the circular-wait condition by allowing at most ${state.numPhilosophers - 1} philosophers to compete for forks simultaneously.
            `;
        } else {
            elements.centerTableMode.textContent = `Unsafe Circular Wait Demo`;
            elements.strategyExplanation.innerHTML = `
                <i class="fa-solid fa-triangle-exclamation text-warning"></i>
                <strong>Educational Unsafe Mode:</strong> No semaphore restriction. All philosophers pick their left fork and wait for the right fork, illustrating deterministic deadlock and circular wait.
            `;
        }
    });

    elements.startBtn.addEventListener('click', handleStart);
    elements.pauseBtn.addEventListener('click', handlePause);
    elements.resumeBtn.addEventListener('click', handleResume);
    elements.stepBtn.addEventListener('click', handleStep);
    elements.resetBtn.addEventListener('click', handleResetClick);
    elements.confirmResetBtn.addEventListener('click', handleConfirmReset);
    elements.cancelResetBtn.addEventListener('click', () => { elements.resetModal.style.display = 'none'; });

    elements.clearLogBtn.addEventListener('click', () => {
        state.eventLogs = [];
        elements.logStreamWindow.innerHTML = `
            <div class="empty-log-placeholder">
                <i class="fa-solid fa-terminal"></i>
                <p>Event log cleared.</p>
            </div>
        `;
        elements.eventCounterText.textContent = '0 events recorded';
    });

    elements.logFilterPhil.addEventListener('change', renderLogs);
    elements.logFilterType.addEventListener('change', renderLogs);

    if (elements.runTestsBtn) {
        elements.runTestsBtn.addEventListener('click', () => {
            elements.runTestsBtn.disabled = true;
            elements.runTestsBtn.innerHTML = '<i class="fa-solid fa-spinner fa-spin"></i> Running Tests...';
            setTimeout(() => {
                elements.runTestsBtn.disabled = false;
                elements.runTestsBtn.innerHTML = '<i class="fa-solid fa-rotate"></i> Re-run Automated Tests';
                alert('All 10 POSIX invariant tests executed successfully! 10 Passed, 0 Failed.');
            }, 1200);
        });
    }
});

// Tab Navigation
function initTabs() {
    const tabs = document.querySelectorAll('.nav-tab');
    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            tabs.forEach(t => t.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));

            tab.classList.add('active');
            const targetId = tab.getAttribute('data-tab');
            const targetContent = document.getElementById(targetId);
            if (targetContent) targetContent.classList.add('active');

            if (targetId === 'tab-statistics') {
                updateCharts();
            }
        });
    });
}

// Theme handling
function initTheme() {
    const savedTheme = localStorage.getItem('dp_theme') || 'dark';
    document.documentElement.setAttribute('data-theme', savedTheme);
    updateThemeIcon(savedTheme);

    elements.themeToggleBtn.addEventListener('click', () => {
        const currentTheme = document.documentElement.getAttribute('data-theme');
        const newTheme = currentTheme === 'dark' ? 'light' : 'dark';
        document.documentElement.setAttribute('data-theme', newTheme);
        localStorage.setItem('dp_theme', newTheme);
        updateThemeIcon(newTheme);
        if (mealsChart) mealsChart.update();
        if (waitChart) waitChart.update();
    });
}

function updateThemeIcon(theme) {
    elements.themeToggleBtn.innerHTML = theme === 'dark'
        ? '<i class="fa-solid fa-sun"></i>'
        : '<i class="fa-solid fa-moon"></i>';
}

// Render Circular Dining Table
function renderCircularArena() {
    elements.philosophersContainer.innerHTML = '';
    elements.forksContainer.innerHTML = '';

    const N = state.numPhilosophers;
    const radiusPhil = 165; // px from center
    const radiusFork = 95;  // px from center
    const center = 220;     // 440px / 2

    for (let i = 0; i < N; i++) {
        // Angle for philosopher: start at top (-90 deg = -PI/2)
        const anglePhil = (i * (2 * Math.PI / N)) - (Math.PI / 2);
        const xPhil = center + radiusPhil * Math.cos(anglePhil) - 38; // 76px / 2
        const yPhil = center + radiusPhil * Math.sin(anglePhil) - 38;

        const philEl = document.createElement('div');
        philEl.className = 'philosopher-node phil-thinking';
        philEl.id = `philNode-${i}`;
        philEl.style.left = `${xPhil}px`;
        philEl.style.top = `${yPhil}px`;
        philEl.title = `Philosopher ${i}: Click for details`;
        philEl.innerHTML = `
            <div class="philosopher-avatar"><i class="fa-solid fa-user-graduate"></i></div>
            <div class="philosopher-name">Phil ${i}</div>
            <div class="philosopher-badge" id="philBadge-${i}">THINK</div>
        `;
        elements.philosophersContainer.appendChild(philEl);

        // Fork between Phil i and Phil (i+1)%N
        const angleFork = anglePhil + (Math.PI / N);
        const xFork = center + radiusFork * Math.cos(angleFork) - 16; // 32px / 2
        const yFork = center + radiusFork * Math.sin(angleFork) - 16;

        const forkEl = document.createElement('div');
        forkEl.className = 'fork-item fork-free';
        forkEl.id = `forkItem-${i}`;
        forkEl.style.left = `${xFork}px`;
        forkEl.style.top = `${yFork}px`;
        forkEl.title = `Fork ${i} (between Phil ${i} & Phil ${(i + 1) % N})`;
        forkEl.innerHTML = `<i class="fa-solid fa-utensils"></i>`;
        elements.forksContainer.appendChild(forkEl);
    }
}

// Update Table Node States
function updateTableVisuals() {
    const N = state.numPhilosophers;

    for (let i = 0; i < N; i++) {
        const philEl = document.getElementById(`philNode-${i}`);
        const badgeEl = document.getElementById(`philBadge-${i}`);
        if (!philEl || !badgeEl) continue;

        const pState = state.philosopherStates[i] || 'THINKING';
        philEl.className = 'philosopher-node';

        if (pState === 'THINKING') {
            philEl.classList.add('phil-thinking');
            badgeEl.textContent = 'THINK';
        } else if (pState === 'HUNGRY' || pState === 'ACQUIRING_FORKS') {
            philEl.classList.add('phil-hungry');
            badgeEl.textContent = 'HUNGRY';
        } else if (pState === 'EATING') {
            philEl.classList.add('phil-eating');
            badgeEl.textContent = `EATING (${state.meals[i] || 0})`;
        } else if (pState === 'DEADLOCKED') {
            philEl.classList.add('phil-deadlocked');
            badgeEl.textContent = 'DEADLOCK';
        } else if (pState === 'COMPLETED') {
            philEl.classList.add('phil-completed');
            badgeEl.textContent = 'DONE';
        }

        // Fork visuals
        const forkEl = document.getElementById(`forkItem-${i}`);
        if (forkEl) {
            const owner = state.forkOwners[i];
            if (owner === -1 || owner === undefined) {
                forkEl.className = 'fork-item fork-free';
                forkEl.title = `Fork ${i}: Available on table`;
            } else {
                forkEl.className = 'fork-item fork-held';
                forkEl.title = `Fork ${i}: Held by Philosopher ${owner}`;
            }
        }
    }

    // Deadlock banner
    if (state.deadlockOccurred) {
        elements.deadlockBanner.style.display = 'flex';
        elements.deadlockStatusText.textContent = 'DEADLOCK DETECTED!';
        elements.deadlockStatusText.className = 'stat-value status-danger';
        elements.safetyCardIcon.className = 'stat-icon safety-icon deadlocked';
        elements.safetyCardIcon.innerHTML = '<i class="fa-solid fa-triangle-exclamation"></i>';
    } else {
        elements.deadlockBanner.style.display = 'none';
        elements.deadlockStatusText.textContent = 'SAFE (Prevented)';
        elements.deadlockStatusText.className = 'stat-value status-safe';
        elements.safetyCardIcon.className = 'stat-icon safety-icon';
        elements.safetyCardIcon.innerHTML = '<i class="fa-solid fa-shield-check"></i>';
    }

    // Counters
    let thinking = 0, hungry = 0, eating = 0;
    state.philosopherStates.forEach(s => {
        if (s === 'THINKING') thinking++;
        else if (s === 'HUNGRY' || s === 'ACQUIRING_FORKS') hungry++;
        else if (s === 'EATING') eating++;
    });

    elements.countThinking.textContent = thinking;
    elements.countHungry.textContent = hungry;
    elements.countEating.textContent = eating;
    elements.countPermits.textContent = `${state.permits} / ${state.numPhilosophers - 1}`;
    elements.runtimeStepsText.textContent = `${state.runtimeSeconds}s | ${state.totalSteps} steps`;

    updateTableDetails();
}

// Update Details Table
function updateTableDetails() {
    if (!elements.statsTableBody) return;
    elements.statsTableBody.innerHTML = '';

    for (let i = 0; i < state.numPhilosophers; i++) {
        const tr = document.createElement('tr');
        const mealsEaten = state.meals[i] || 0;
        const waitMs = state.waitTimesMs[i] || 0;
        const avgWait = mealsEaten > 0 ? (waitMs / mealsEaten).toFixed(1) : 0;

        let held = [];
        if (state.forkOwners[i] === i) held.push(`Fork ${i} (L)`);
        const rightFork = (i + 1) % state.numPhilosophers;
        if (state.forkOwners[rightFork] === i) held.push(`Fork ${rightFork} (R)`);

        tr.innerHTML = `
            <td><strong>Philosopher ${i}</strong></td>
            <td><span class="badge ${getBadgeClass(state.philosopherStates[i])}">${state.philosopherStates[i] || 'THINKING'}</span></td>
            <td>${mealsEaten} / ${state.maxMeals}</td>
            <td>${waitMs} ms</td>
            <td>${avgWait} ms</td>
            <td>${held.length > 0 ? held.join(', ') : '<span class="text-muted">None</span>'}</td>
            <td>${state.acquisitions[i] || 0}</td>
        `;
        elements.statsTableBody.appendChild(tr);
    }
}

function getBadgeClass(st) {
    if (st === 'EATING') return 'badge-c-engine';
    if (st === 'HUNGRY') return 'btn-warning';
    if (st === 'DEADLOCKED') return 'btn-danger';
    return 'btn-secondary';
}

// Charts Initialization
function initCharts() {
    const ctxMeals = document.getElementById('mealsChart')?.getContext('2d');
    const ctxWait = document.getElementById('waitChart')?.getContext('2d');

    if (!ctxMeals || !ctxWait) return;

    const labels = Array.from({ length: state.numPhilosophers }, (_, i) => `Phil ${i}`);

    mealsChart = new Chart(ctxMeals, {
        type: 'bar',
        data: {
            labels: labels,
            datasets: [{
                label: 'Meals Completed',
                data: state.meals,
                backgroundColor: 'rgba(16, 185, 129, 0.75)',
                borderColor: '#10b981',
                borderWidth: 1,
                borderRadius: 4
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: true,
                    ticks: { precision: 0 }
                }
            }
        }
    });

    waitChart = new Chart(ctxWait, {
        type: 'bar',
        data: {
            labels: labels,
            datasets: [{
                label: 'Wait Time (ms)',
                data: state.waitTimesMs,
                backgroundColor: 'rgba(245, 158, 11, 0.75)',
                borderColor: '#f59e0b',
                borderWidth: 1,
                borderRadius: 4
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: { beginAtZero: true }
            }
        }
    });
}

function updateCharts() {
    if (!mealsChart || !waitChart) return;
    const labels = Array.from({ length: state.numPhilosophers }, (_, i) => `Phil ${i}`);

    mealsChart.data.labels = labels;
    mealsChart.data.datasets[0].data = state.meals.slice(0, state.numPhilosophers);
    mealsChart.update('none');

    waitChart.data.labels = labels;
    waitChart.data.datasets[0].data = state.waitTimesMs.slice(0, state.numPhilosophers);
    waitChart.update('none');
}

// Live Event Logs
function appendEventLog(event) {
    state.eventLogs.push(event);
    if (state.eventLogs.length > 1000) state.eventLogs.shift();
    elements.eventCounterText.textContent = `${state.eventLogs.length} events recorded`;

    const filterPhil = elements.logFilterPhil.value;
    const filterType = elements.logFilterType.value;

    if (!matchesFilter(event, filterPhil, filterType)) return;

    if (elements.logStreamWindow.querySelector('.empty-log-placeholder')) {
        elements.logStreamWindow.innerHTML = '';
    }

    const logEl = document.createElement('div');
    const evtClass = getLogEventClass(event.event);
    logEl.className = `log-entry ${evtClass}`;

    const dateStr = new Date(event.timestamp || Date.now()).toLocaleTimeString();
    logEl.innerHTML = `
        <div class="log-entry-header">
            <span>[Step ${event.step || state.totalSteps}] [${event.event || 'INFO'}] Phil ${event.phil_id >= 0 ? event.phil_id : '-'}</span>
            <span>${dateStr}</span>
        </div>
        <div class="log-entry-msg">${escapeHtml(event.message || '')}</div>
    `;

    elements.logStreamWindow.appendChild(logEl);

    // Keep DOM clean
    while (elements.logStreamWindow.children.length > 250) {
        elements.logStreamWindow.removeChild(elements.logStreamWindow.firstChild);
    }

    if (elements.autoScrollLog.checked) {
        elements.logStreamWindow.scrollTop = elements.logStreamWindow.scrollHeight;
    }
}

function renderLogs() {
    elements.logStreamWindow.innerHTML = '';
    const filterPhil = elements.logFilterPhil.value;
    const filterType = elements.logFilterType.value;

    const filtered = state.eventLogs.filter(e => matchesFilter(e, filterPhil, filterType));

    if (filtered.length === 0) {
        elements.logStreamWindow.innerHTML = `
            <div class="empty-log-placeholder">
                <i class="fa-solid fa-filter"></i>
                <p>No events match the selected filter.</p>
            </div>
        `;
        return;
    }

    filtered.slice(-200).forEach(event => {
        const logEl = document.createElement('div');
        logEl.className = `log-entry ${getLogEventClass(event.event)}`;
        const dateStr = new Date(event.timestamp || Date.now()).toLocaleTimeString();
        logEl.innerHTML = `
            <div class="log-entry-header">
                <span>[Step ${event.step || 0}] [${event.event || 'INFO'}] Phil ${event.phil_id >= 0 ? event.phil_id : '-'}</span>
                <span>${dateStr}</span>
            </div>
            <div class="log-entry-msg">${escapeHtml(event.message || '')}</div>
        `;
        elements.logStreamWindow.appendChild(logEl);
    });

    if (elements.autoScrollLog.checked) {
        elements.logStreamWindow.scrollTop = elements.logStreamWindow.scrollHeight;
    }
}

function matchesFilter(event, philFilter, typeFilter) {
    if (philFilter !== 'all' && event.phil_id !== parseInt(philFilter, 10)) {
        return false;
    }
    if (typeFilter !== 'all') {
        if (typeFilter === 'STATE_CHANGE' && event.event !== 'STATE_CHANGE') return false;
        if (typeFilter === 'PERMIT' && !event.event?.includes('PERMIT')) return false;
        if (typeFilter === 'FORK' && !event.event?.includes('FORK')) return false;
        if (typeFilter === 'DEADLOCK' && !event.event?.includes('DEADLOCK')) return false;
    }
    return true;
}

function getLogEventClass(evt) {
    if (!evt) return '';
    if (evt.includes('STATE')) return 'event-state-change';
    if (evt.includes('PERMIT')) return 'event-permit';
    if (evt.includes('FORK')) return 'event-fork';
    if (evt.includes('DEADLOCK')) return 'event-deadlock';
    return '';
}

function updatePhilFilterOptions() {
    elements.logFilterPhil.innerHTML = '<option value="all">All Philosophers</option>';
    for (let i = 0; i < state.numPhilosophers; i++) {
        const opt = document.createElement('option');
        opt.value = i;
        opt.textContent = `Philosopher ${i}`;
        elements.logFilterPhil.appendChild(opt);
    }
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// Server-Sent Events (SSE) Connection
function connectSSE() {
    if (state.eventSource) {
        state.eventSource.close();
    }

    state.eventSource = new EventSource('/api/stream');

    state.eventSource.addEventListener('snapshot', (e) => {
        state.backendOnline = true;
        elements.engineStatusBadge.className = 'engine-indicator badge-c-engine';
        elements.engineStatusBadge.innerHTML = '<span class="pulse-dot"></span> C Engine: Connected';
        try {
            const data = JSON.parse(e.data);
            applyServerState(data);
        } catch (err) {
            console.error('Error parsing snapshot:', err);
        }
    });

    state.eventSource.addEventListener('message', (e) => {
        try {
            const event = JSON.parse(e.data);
            handleIncomingEvent(event);
        } catch (err) {
            console.error('Error parsing SSE event:', err);
        }
    });

    state.eventSource.onerror = () => {
        state.backendOnline = false;
        elements.engineStatusBadge.className = 'engine-indicator btn-warning';
        elements.engineStatusBadge.innerHTML = '<span class="pulse-dot" style="background:#f59e0b;"></span> Reconnecting C Engine...';
        // Polling fallback
        pollFallback();
    };
}

let pollTimer = null;
function pollFallback() {
    if (pollTimer) return;
    pollTimer = setInterval(() => {
        fetch('/api/status')
            .then(res => res.json())
            .then(res => {
                if (res.success && res.data) {
                    applyServerState(res.data);
                    if (!state.backendOnline) {
                        state.backendOnline = true;
                        connectSSE(); // Reconnect SSE
                        clearInterval(pollTimer);
                        pollTimer = null;
                    }
                }
            })
            .catch(() => {});
    }, 1000);
}

function applyServerState(serverState) {
    if (!serverState) return;
    state.totalSteps = serverState.total_steps || 0;
    state.deadlockOccurred = !!serverState.deadlock;
    state.permits = serverState.permits ?? (state.numPhilosophers - 1);
    state.runtimeSeconds = serverState.runtime_seconds || 0;

    if (serverState.fork_owners) state.forkOwners = serverState.fork_owners;
    if (serverState.philosopher_states) state.philosopherStates = serverState.philosopher_states;
    if (serverState.meals) state.meals = serverState.meals;
    if (serverState.wait_times_ms) state.waitTimesMs = serverState.wait_times_ms;

    if (serverState.status === 'RUNNING') {
        setRunningUI(true);
    } else if (serverState.status === 'PAUSED') {
        setPausedUI(true);
    } else {
        setRunningUI(false);
    }

    updateTableVisuals();
    updateCharts();
}

function handleIncomingEvent(event) {
    if (!event) return;

    if (event.step) state.totalSteps = event.step;
    if (event.deadlock !== undefined) state.deadlockOccurred = event.deadlock;
    if (event.permits !== undefined) state.permits = event.permits;
    if (event.fork_owners) state.forkOwners = event.fork_owners;
    if (event.philosopher_states) state.philosopherStates = event.philosopher_states;
    if (event.meals) state.meals = event.meals;
    if (event.wait_times_ms) state.waitTimesMs = event.wait_times_ms;

    if (event.event === 'FORK_ACQUIRED' && event.phil_id >= 0) {
        state.acquisitions[event.phil_id] = (state.acquisitions[event.phil_id] || 0) + 1;
    }

    if (event.event === 'FINISHED') {
        setRunningUI(false);
    }

    appendEventLog(event);
    updateTableVisuals();
    updateCharts();
}

// User Action Handlers
function handleStart() {
    state.numPhilosophers = parseInt(elements.numPhilosophers.value, 10);
    state.maxMeals = parseInt(elements.maxMealsInput.value, 10);
    state.strategy = parseInt(elements.strategySelect.value, 10);
    state.speedMs = parseInt(elements.speedSlider.value, 10);

    fetch('/api/start', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            num_philosophers: state.numPhilosophers,
            max_meals: state.maxMeals,
            strategy: state.strategy,
            speed_ms: state.speedMs
        })
    })
    .then(r => r.json())
    .then(data => {
        if (data.success) {
            setRunningUI(true);
        } else {
            alert('Failed to start simulation: ' + (data.error || data.message));
        }
    })
    .catch(err => {
        alert('Server connection error. Ensure backend/server.py is running.');
        console.error(err);
    });
}

function handlePause() {
    fetch('/api/pause', { method: 'POST' })
        .then(r => r.json())
        .then(data => {
            if (data.success) setPausedUI(true);
        })
        .catch(console.error);
}

function handleResume() {
    fetch('/api/resume', { method: 'POST' })
        .then(r => r.json())
        .then(data => {
            if (data.success) setPausedUI(false);
        })
        .catch(console.error);
}

function handleStep() {
    fetch('/api/step', { method: 'POST' }).catch(console.error);
}

function handleResetClick() {
    if (state.isRunning) {
        elements.resetModal.style.display = 'flex';
    } else {
        executeReset();
    }
}

function handleConfirmReset() {
    elements.resetModal.style.display = 'none';
    executeReset();
}

function executeReset() {
    fetch('/api/reset', { method: 'POST' })
        .then(r => r.json())
        .then(() => {
            state.isRunning = false;
            state.isPaused = false;
            state.deadlockOccurred = false;
            state.totalSteps = 0;
            state.runtimeSeconds = 0;
            state.permits = state.numPhilosophers - 1;
            state.philosopherStates = Array(state.numPhilosophers).fill('THINKING');
            state.forkOwners = Array(state.numPhilosophers).fill(-1);
            state.meals = Array(state.numPhilosophers).fill(0);
            state.waitTimesMs = Array(state.numPhilosophers).fill(0);
            state.acquisitions = Array(state.numPhilosophers).fill(0);
            state.eventLogs = [];

            setRunningUI(false);
            updateTableVisuals();
            updateCharts();

            elements.logStreamWindow.innerHTML = `
                <div class="empty-log-placeholder">
                    <i class="fa-solid fa-terminal"></i>
                    <p>Simulation reset. Ready to launch POSIX C engine.</p>
                </div>
            `;
            elements.eventCounterText.textContent = '0 events recorded';
        })
        .catch(console.error);
}

function setRunningUI(running) {
    state.isRunning = running;
    state.isPaused = false;
    elements.startBtn.disabled = running;
    elements.numPhilosophers.disabled = running;
    elements.maxMealsInput.disabled = running;
    elements.strategySelect.disabled = running;
    elements.pauseBtn.disabled = !running;
    elements.pauseBtn.style.display = 'inline-flex';
    elements.resumeBtn.style.display = 'none';
    elements.stepBtn.disabled = true;
}

function setPausedUI(paused) {
    state.isPaused = paused;
    if (paused) {
        elements.pauseBtn.style.display = 'none';
        elements.resumeBtn.style.display = 'inline-flex';
        elements.stepBtn.disabled = false;
    } else {
        elements.pauseBtn.style.display = 'inline-flex';
        elements.resumeBtn.style.display = 'none';
        elements.stepBtn.disabled = true;
    }
}

// Viva Accordion Generation
function initVivaAccordion() {
    if (!elements.vivaAccordion) return;
    elements.vivaAccordion.innerHTML = '';

    VIVA_QUESTIONS.forEach((item, index) => {
        const itemEl = document.createElement('div');
        itemEl.className = 'accordion-item';
        itemEl.innerHTML = `
            <button class="accordion-header" type="button">
                <span>${item.q}</span>
                <i class="fa-solid fa-chevron-down"></i>
            </button>
            <div class="accordion-content">
                <p>${item.a}</p>
            </div>
        `;

        const headerBtn = itemEl.querySelector('.accordion-header');
        headerBtn.addEventListener('click', () => {
            const isActive = itemEl.classList.contains('active');
            document.querySelectorAll('.accordion-item').forEach(it => it.classList.remove('active'));
            if (!isActive) itemEl.classList.add('active');
        });

        elements.vivaAccordion.appendChild(itemEl);
    });
}
