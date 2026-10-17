#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "simulation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

static SimulationContext g_sim;
static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
    sim_stop(&g_sim);
}

static void print_usage(const char *prog) {
    printf("=====================================================================\n");
    printf("  DINING PHILOSOPHERS SIMULATION - DEADLOCK PREVENTION ENGINE (C/POSIX)\n");
    printf("=====================================================================\n");
    printf("Usage: %s [OPTIONS]\n\n", prog);
    printf("Options:\n");
    printf("  -n <num>         Number of philosophers (2-10, default: 5)\n");
    printf("  -m <meals>       Meals per philosopher (default: 5)\n");
    printf("  -s <strategy>    0: Deadlock Prevention (N-1 Sem), 1: Unsafe Deadlock Demo (default: 0)\n");
    printf("  -d <speed_ms>    Simulation speed / delay in milliseconds (default: 600)\n");
    printf("  --json           Output event stream as JSON (for Web UI integration)\n");
    printf("  --interactive    Enable interactive stdin command controller (PAUSE, RESUME, etc.)\n");
    printf("  -h, --help       Display this help and exit\n\n");
    printf("Interactive Commands (when --interactive is active):\n");
    printf("  PAUSE            Pause the simulation\n");
    printf("  RESUME           Resume the simulation\n");
    printf("  STEP             Execute a single simulation step\n");
    printf("  SPEED <ms>       Change delay in milliseconds (e.g., SPEED 300)\n");
    printf("  STOP             Gracefully stop the simulation\n");
    printf("  STATUS           Print current philosopher & fork states\n");
    printf("=====================================================================\n");
}

/* Command listener thread for interactive CLI / stdin control */
static void *stdin_command_listener(void *arg) {
    SimulationContext *ctx = (SimulationContext *)arg;
    char buffer[256];

    while (g_running && fgets(buffer, sizeof(buffer), stdin) != NULL) {
        /* Strip newline */
        buffer[strcspn(buffer, "\r\n")] = 0;
        if (strlen(buffer) == 0) continue;

        if (strcasecmp(buffer, "PAUSE") == 0) {
            sim_pause(ctx);
        } else if (strcasecmp(buffer, "RESUME") == 0) {
            sim_resume(ctx);
        } else if (strcasecmp(buffer, "STEP") == 0) {
            sim_step(ctx);
        } else if (strncasecmp(buffer, "SPEED", 5) == 0) {
            int new_speed = atoi(buffer + 5);
            if (new_speed >= 20) {
                sim_set_speed(ctx, new_speed);
                char msg[64];
                snprintf(msg, sizeof(msg), "Speed updated to %d ms", new_speed);
                sim_emit_event(ctx, EVENT_STEP_COMPLETED, -1, -1, msg);
            }
        } else if (strcasecmp(buffer, "STOP") == 0 || strcasecmp(buffer, "QUIT") == 0) {
            sim_stop(ctx);
            g_running = 0;
            break;
        } else if (strcasecmp(buffer, "STATUS") == 0) {
            sim_emit_event(ctx, EVENT_STATE_CHANGE, -1, -1, "Status query requested");
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int num_philosophers = DEFAULT_PHILOSOPHERS;
    int max_meals = DEFAULT_MEALS;
    SimulationStrategy strategy = STRATEGY_DEADLOCK_PREVENTION;
    int speed_ms = DEFAULT_SPEED_MS;
    bool json_mode = false;
    bool interactive = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            num_philosophers = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            max_meals = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            strategy = (SimulationStrategy)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            speed_ms = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--json") == 0) {
            json_mode = true;
        } else if (strcmp(argv[i], "--interactive") == 0) {
            interactive = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Set up signal handling */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (!json_mode) {
        printf("\nInitializing Dining Philosophers Simulation...\n");
        printf("Philosophers: %d | Meals: %d | Strategy: %s | Delay: %d ms\n\n",
               num_philosophers, max_meals,
               (strategy == STRATEGY_DEADLOCK_PREVENTION) ? "Deadlock Prevention (N-1 Semaphore)" : "Unsafe Circular Wait Demo",
               speed_ms);
    }

    int init_rc = sim_init(&g_sim, num_philosophers, max_meals, strategy, speed_ms, json_mode);
    if (init_rc != 0) {
        fprintf(stderr, "Error: sim_init failed with code %d. Valid philosopher range: %d to %d.\n",
                init_rc, MIN_PHILOSOPHERS, MAX_PHILOSOPHERS);
        return 1;
    }

    pthread_t listener_tid = 0;
    if (interactive) {
        pthread_create(&listener_tid, NULL, stdin_command_listener, &g_sim);
    }

    if (sim_start(&g_sim) != 0) {
        fprintf(stderr, "Error: sim_start failed.\n");
        sim_cleanup(&g_sim);
        return 1;
    }

    /* Wait for completion or stop signal */
    while (g_running && g_sim.is_running) {
        usleep(100000);
    }

    sim_stop(&g_sim);

    if (listener_tid) {
        pthread_cancel(listener_tid);
        pthread_join(listener_tid, NULL);
    }

    if (!json_mode) {
        printf("\n=======================================================\n");
        printf("               SIMULATION EXECUTION SUMMARY            \n");
        printf("=======================================================\n");
        printf("Mode: %s\n", (strategy == STRATEGY_DEADLOCK_PREVENTION) ? "Deadlock Prevention (N-1 Semaphore)" : "Unsafe Demo");
        printf("Deadlock Occurred: %s\n", g_sim.deadlock_occurred ? "YES (As Expected in Demo)" : "NO (Prevented Successfully)");
        printf("Total Simulation Steps: %d\n", g_sim.total_steps);
        printf("Runtime: %.2f seconds\n", (double)(get_current_time_ms() - g_sim.start_time_ms) / 1000.0);
        printf("Philosopher Statistics:\n");
        for (int i = 0; i < g_sim.num_philosophers; i++) {
            printf("  Phil %d: %d/%d meals | Total Wait: %llu ms | State: %s\n",
                   i, g_sim.stats[i].meals_eaten, g_sim.max_meals,
                   (unsigned long long)g_sim.stats[i].total_wait_time_ms,
                   phil_state_to_string(g_sim.stats[i].state));
        }
        printf("=======================================================\n");
    }

    sim_cleanup(&g_sim);
    return 0;
}
