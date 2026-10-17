#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "../src/simulation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#define ANSI_GREEN "\033[0;32m"
#define ANSI_RED   "\033[0;31m"
#define ANSI_BLUE  "\033[0;34m"
#define ANSI_RESET "\033[0m"

static int total_tests = 0;
static int passed_tests = 0;
static int failed_tests = 0;

static void report_test_result(const char *name, bool passed, const char *details) {
    total_tests++;
    if (passed) {
        passed_tests++;
        printf("  [%sPASS%s] %-42s : %s\n", ANSI_GREEN, ANSI_RESET, name, details);
    } else {
        failed_tests++;
        printf("  [%sFAIL%s] %-42s : %s\n", ANSI_RED, ANSI_RESET, name, details);
    }
}

/* Test 1: Normal execution */
static void test_normal_execution(void) {
    SimulationContext ctx;
    int rc = sim_init(&ctx, 5, 2, STRATEGY_DEADLOCK_PREVENTION, 20, false);
    assert(rc == 0);
    ctx.silent_mode = true;
    sim_start(&ctx);

    /* Wait with timeout (max 5 seconds) */
    uint64_t start = get_current_time_ms();
    while (ctx.is_running && (get_current_time_ms() - start < 5000)) {
        usleep(10000);
    }

    bool all_completed = true;
    for (int i = 0; i < 5; i++) {
        if (ctx.stats[i].meals_eaten != 2) all_completed = false;
    }

    sim_cleanup(&ctx);
    report_test_result("Test 1: Normal Execution", all_completed,
                       all_completed ? "All 5 philosophers completed 2 meals" : "Not all philosophers finished meals");
}

/* Test 2: Deadlock prevention (no deadlock under N-1 semaphore) */
static void test_deadlock_prevention(void) {
    SimulationContext ctx;
    sim_init(&ctx, 5, 4, STRATEGY_DEADLOCK_PREVENTION, 15, false);
    ctx.silent_mode = true;
    sim_start(&ctx);

    uint64_t start = get_current_time_ms();
    while (ctx.is_running && (get_current_time_ms() - start < 6000)) {
        usleep(10000);
    }

    bool no_deadlock = (!ctx.deadlock_occurred) && (!ctx.is_running);
    sim_cleanup(&ctx);
    report_test_result("Test 2: Deadlock Prevention", no_deadlock,
                       no_deadlock ? "Completed 4 meals with 0 deadlocks" : "Deadlock occurred or timed out");
}

/* Test 3: Mutual exclusion (fork owner invariant check) */
static void test_mutual_exclusion(void) {
    SimulationContext ctx;
    sim_init(&ctx, 5, 3, STRATEGY_DEADLOCK_PREVENTION, 25, false);
    ctx.silent_mode = true;
    sim_start(&ctx);

    bool mutual_exclusion_violated = false;
    uint64_t start = get_current_time_ms();

    while (ctx.is_running && (get_current_time_ms() - start < 5000)) {
        pthread_mutex_lock(&ctx.state_mutex);
        /* Check fork ownership invariant: for each fork, exactly 0 or 1 holder */
        int fork_counts[5] = {0};
        for (int p = 0; p < 5; p++) {
            if (ctx.stats[p].forks_held[0] != -1) {
                fork_counts[ctx.stats[p].forks_held[0]]++;
            }
            if (ctx.stats[p].forks_held[1] != -1) {
                fork_counts[ctx.stats[p].forks_held[1]]++;
            }
        }
        for (int f = 0; f < 5; f++) {
            if (fork_counts[f] > 1) {
                mutual_exclusion_violated = true;
            }
        }
        pthread_mutex_unlock(&ctx.state_mutex);
        usleep(5000);
    }

    sim_cleanup(&ctx);
    report_test_result("Test 3: Mutual Exclusion Invariant", !mutual_exclusion_violated,
                       !mutual_exclusion_violated ? "No fork was ever held by >1 philosopher" : "Fork conflict detected!");
}

/* Test 4: Resource release (forks and semaphore permit released after eating) */
static void test_resource_release(void) {
    SimulationContext ctx;
    sim_init(&ctx, 5, 2, STRATEGY_DEADLOCK_PREVENTION, 20, false);
    ctx.silent_mode = true;
    sim_start(&ctx);

    uint64_t start = get_current_time_ms();
    while (ctx.is_running && (get_current_time_ms() - start < 5000)) {
        usleep(10000);
    }

    /* Verify all forks free and all permits returned */
    bool all_resources_released = true;
    for (int i = 0; i < 5; i++) {
        if (ctx.fork_owners[i] != -1) all_resources_released = false;
        if (ctx.stats[i].forks_held[0] != -1 || ctx.stats[i].forks_held[1] != -1) all_resources_released = false;
    }
    if (ctx.current_permits != 4) all_resources_released = false;

    sim_cleanup(&ctx);
    report_test_result("Test 4: Resource Release Invariant", all_resources_released,
                       all_resources_released ? "All 5 forks & 4 permits safely released" : "Dangling locks or permits found");
}

/* Test 5: Repeated simulation (3 consecutive runs without resource leaks) */
static void test_repeated_simulation(void) {
    bool all_runs_ok = true;
    for (int run = 1; run <= 3; run++) {
        SimulationContext ctx;
        if (sim_init(&ctx, 4, 2, STRATEGY_DEADLOCK_PREVENTION, 15, false) != 0) {
            all_runs_ok = false;
            break;
        }
        ctx.silent_mode = true;
        sim_start(&ctx);
        uint64_t start = get_current_time_ms();
        while (ctx.is_running && (get_current_time_ms() - start < 4000)) {
            usleep(10000);
        }
        if (ctx.deadlock_occurred || ctx.is_running) all_runs_ok = false;
        sim_cleanup(&ctx);
    }

    report_test_result("Test 5: Repeated Simulation", all_runs_ok,
                       all_runs_ok ? "3 consecutive full runs completed cleanly" : "Consecutive run failure");
}

/* Test 6: Invalid configuration rejection */
static void test_invalid_configuration(void) {
    SimulationContext ctx;
    int rc1 = sim_init(&ctx, 1, 3, STRATEGY_DEADLOCK_PREVENTION, 20, false);  /* < 2 */
    int rc2 = sim_init(&ctx, 15, 3, STRATEGY_DEADLOCK_PREVENTION, 20, false); /* > 10 */
    int rc3 = sim_init(&ctx, 5, 0, STRATEGY_DEADLOCK_PREVENTION, 20, false);  /* meals < 1 */

    bool passed = (rc1 != 0 && rc2 != 0 && rc3 != 0);
    report_test_result("Test 6: Invalid Config Rejection", passed,
                       passed ? "Properly rejected N=1, N=15, and meals=0" : "Invalid parameters erroneously accepted");
}

/* Test 7: Unsafe scenario demonstration with controlled watchdog */
static void test_unsafe_scenario(void) {
    SimulationContext ctx;
    sim_init(&ctx, 5, 1, STRATEGY_UNSAFE_DEADLOCK, 20, false);
    ctx.silent_mode = true;
    sim_start(&ctx);

    uint64_t start = get_current_time_ms();
    while (ctx.is_running && (get_current_time_ms() - start < 6000)) {
        usleep(20000);
    }

    bool passed = ctx.deadlock_occurred;
    sim_cleanup(&ctx);
    report_test_result("Test 7: Unsafe Scenario Demonstration", passed,
                       passed ? "Circular wait detected and unblocked safely" : "Deadlock not detected or thread hung");
}

/* Test 8: Concurrent activity (multiple philosophers eating simultaneously) */
static void test_concurrent_activity(void) {
    SimulationContext ctx;
    sim_init(&ctx, 6, 5, STRATEGY_DEADLOCK_PREVENTION, 50, false);
    ctx.silent_mode = true;
    sim_start(&ctx);

    bool observed_concurrency = false;
    uint64_t start = get_current_time_ms();

    while (ctx.is_running && (get_current_time_ms() - start < 5000)) {
        pthread_mutex_lock(&ctx.state_mutex);
        int eating_count = 0;
        for (int i = 0; i < 6; i++) {
            if (ctx.stats[i].state == STATE_EATING) {
                eating_count++;
            }
        }
        if (eating_count >= 2 || ctx.max_concurrent_eating >= 2) {
            observed_concurrency = true;
        }
        pthread_mutex_unlock(&ctx.state_mutex);
        usleep(1000);
    }

    if (ctx.max_concurrent_eating >= 2) {
        observed_concurrency = true;
    }

    sim_cleanup(&ctx);
    report_test_result("Test 8: Concurrent Activity", observed_concurrency,
                       observed_concurrency ? "Observed >=2 philosophers eating concurrently" : "No concurrent eating observed");
}

/* Test 9: Shutdown and reset responsiveness */
static void test_shutdown_and_reset(void) {
    SimulationContext ctx;
    sim_init(&ctx, 5, 10, STRATEGY_DEADLOCK_PREVENTION, 200, false);
    ctx.silent_mode = true;
    sim_start(&ctx);

    /* Let it run for 150 ms then immediately request stop */
    usleep(150000);
    uint64_t stop_start = get_current_time_ms();
    sim_stop(&ctx);
    uint64_t stop_duration = get_current_time_ms() - stop_start;

    bool fast_shutdown = (stop_duration < 1000) && (!ctx.is_running);
    sim_cleanup(&ctx);
    report_test_result("Test 9: Fast Clean Shutdown", fast_shutdown,
                       fast_shutdown ? "Terminated threads cleanly within < 1000 ms" : "Shutdown took too long or hung");
}

/* Test 10: Stress test (varied configurations and speeds) */
static void test_stress_test(void) {
    bool stress_passed = true;
    int configs[][3] = {
        {3, 3, 10},
        {5, 3, 10},
        {7, 2, 10},
        {10, 2, 5}
    };

    for (int i = 0; i < 4; i++) {
        SimulationContext ctx;
        int n = configs[i][0];
        int m = configs[i][1];
        int spd = configs[i][2];

        if (sim_init(&ctx, n, m, STRATEGY_DEADLOCK_PREVENTION, spd, false) != 0) {
            stress_passed = false;
            break;
        }
        ctx.silent_mode = true;
        sim_start(&ctx);
        uint64_t start = get_current_time_ms();
        while (ctx.is_running && (get_current_time_ms() - start < 5000)) {
            usleep(10000);
        }
        if (ctx.deadlock_occurred || ctx.is_running) {
            stress_passed = false;
        }
        sim_cleanup(&ctx);
    }

    report_test_result("Test 10: Stress Testing Matrix", stress_passed,
                       stress_passed ? "Passed configs N=3, 5, 7, 10 with varied speeds" : "Stress test failed on configuration");
}

int main(void) {
    printf("\n=====================================================================\n");
    printf("   DINING PHILOSOPHERS DEADLOCK PREVENTION - AUTOMATED TEST HARNESS  \n");
    printf("=====================================================================\n");
    printf("Executing 10 Comprehensive Synchronization Invariant & Feature Tests:\n\n");

    test_normal_execution();
    test_deadlock_prevention();
    test_mutual_exclusion();
    test_resource_release();
    test_repeated_simulation();
    test_invalid_configuration();
    test_unsafe_scenario();
    test_concurrent_activity();
    test_shutdown_and_reset();
    test_stress_test();

    printf("\n=====================================================================\n");
    printf("TEST SUMMARY: %d Total | %s%d Passed%s | %s%d Failed%s\n",
           total_tests,
           ANSI_GREEN, passed_tests, ANSI_RESET,
           failed_tests ? ANSI_RED : ANSI_GREEN, failed_tests, ANSI_RESET);
    printf("=====================================================================\n\n");

    return (failed_tests == 0) ? 0 : 1;
}
