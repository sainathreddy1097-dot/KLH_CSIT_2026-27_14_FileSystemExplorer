#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#include "simulation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

/* Helper: get current monotonic/real timestamp in milliseconds */
uint64_t get_current_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

const char *phil_state_to_string(PhilState state) {
    switch (state) {
        case STATE_THINKING: return "THINKING";
        case STATE_HUNGRY: return "HUNGRY";
        case STATE_ACQUIRING_FORKS: return "ACQUIRING_FORKS";
        case STATE_EATING: return "EATING";
        case STATE_RELEASING_FORKS: return "RELEASING_FORKS";
        case STATE_COMPLETED: return "COMPLETED";
        case STATE_DEADLOCKED: return "DEADLOCKED";
        default: return "UNKNOWN";
    }
}

const char *sim_strategy_to_string(SimulationStrategy strategy) {
    switch (strategy) {
        case STRATEGY_DEADLOCK_PREVENTION: return "DEADLOCK_PREVENTION_N_MINUS_1_SEMAPHORE";
        case STRATEGY_UNSAFE_DEADLOCK: return "UNSAFE_CIRCULAR_WAIT_DEMO";
        default: return "UNKNOWN";
    }
}

static const char *event_type_to_string(SimulationEventType type) {
    switch (type) {
        case EVENT_INIT: return "INIT";
        case EVENT_STATE_CHANGE: return "STATE_CHANGE";
        case EVENT_PERMIT_ACQUIRED: return "PERMIT_ACQUIRED";
        case EVENT_PERMIT_RELEASED: return "PERMIT_RELEASED";
        case EVENT_FORK_ACQUIRED: return "FORK_ACQUIRED";
        case EVENT_FORK_RELEASED: return "FORK_RELEASED";
        case EVENT_DEADLOCK_DETECTED: return "DEADLOCK_DETECTED";
        case EVENT_STEP_COMPLETED: return "STEP_COMPLETED";
        case EVENT_PAUSED: return "PAUSED";
        case EVENT_RESUMED: return "RESUMED";
        case EVENT_RESET: return "RESET";
        case EVENT_FINISHED: return "FINISHED";
        case EVENT_ERROR: return "ERROR";
        default: return "INFO";
    }
}

/* Emit structured event (JSON or Plain text) */
void sim_emit_event(SimulationContext *ctx, SimulationEventType type, int phil_id, int fork_id, const char *custom_msg) {
    if (!ctx) return;

    pthread_mutex_lock(&ctx->state_mutex);
    uint64_t now = get_current_time_ms();
    ctx->total_steps++;

    if (ctx->silent_mode) {
        pthread_mutex_unlock(&ctx->state_mutex);
        return;
    }

    if (ctx->json_mode) {
        printf("{\"timestamp\":%llu,\"step\":%d,\"event\":\"%s\",\"phil_id\":%d,\"fork_id\":%d,\"state\":\"%s\",\"strategy\":\"%s\",\"permits\":%d,\"fork_owners\":[",
               (unsigned long long)now,
               ctx->total_steps,
               event_type_to_string(type),
               phil_id,
               fork_id,
               (phil_id >= 0 && phil_id < ctx->num_philosophers) ? phil_state_to_string(ctx->stats[phil_id].state) : "NONE",
               sim_strategy_to_string(ctx->strategy),
               ctx->current_permits);

        for (int i = 0; i < ctx->num_philosophers; i++) {
            printf("%d%s", ctx->fork_owners[i], (i == ctx->num_philosophers - 1) ? "" : ",");
        }

        printf("],\"philosopher_states\":[");
        for (int i = 0; i < ctx->num_philosophers; i++) {
            printf("\"%s\"%s", phil_state_to_string(ctx->stats[i].state), (i == ctx->num_philosophers - 1) ? "" : ",");
        }

        printf("],\"meals\":[");
        for (int i = 0; i < ctx->num_philosophers; i++) {
            printf("%d%s", ctx->stats[i].meals_eaten, (i == ctx->num_philosophers - 1) ? "" : ",");
        }

        printf("],\"wait_times_ms\":[");
        for (int i = 0; i < ctx->num_philosophers; i++) {
            printf("%llu%s", (unsigned long long)ctx->stats[i].total_wait_time_ms, (i == ctx->num_philosophers - 1) ? "" : ",");
        }

        printf("],\"deadlock\":%s,\"message\":\"%s\"}\n",
               ctx->deadlock_occurred ? "true" : "false",
               custom_msg ? custom_msg : "");
        fflush(stdout);
    } else {
        printf("[%llu ms][Step %-3d] Event: %-18s | Phil %d: %-12s | Permits: %d | %s\n",
               (unsigned long long)(now - ctx->start_time_ms),
               ctx->total_steps,
               event_type_to_string(type),
               phil_id,
               (phil_id >= 0 && phil_id < ctx->num_philosophers) ? phil_state_to_string(ctx->stats[phil_id].state) : "-",
               ctx->current_permits,
               custom_msg ? custom_msg : "");
        fflush(stdout);
    }

    pthread_mutex_unlock(&ctx->state_mutex);
}

/* Pause checkpoint: called before changing states or acquiring locks */
static void check_pause_and_stop(SimulationContext *ctx) {
    pthread_mutex_lock(&ctx->pause_mutex);
    while (ctx->is_paused && !ctx->stop_requested) {
        if (ctx->step_mode) {
            ctx->step_mode = false;
            break;
        }
        pthread_cond_wait(&ctx->pause_cond, &ctx->pause_mutex);
    }
    pthread_mutex_unlock(&ctx->pause_mutex);
}

/* Sleep with responsiveness to stop_requested */
static void responsive_sleep_ms(SimulationContext *ctx, int ms) {
    int chunk = 50;
    while (ms > 0 && !ctx->stop_requested) {
        int sleep_time = (ms > chunk) ? chunk : ms;
        usleep(sleep_time * 1000);
        ms -= sleep_time;
        check_pause_and_stop(ctx);
    }
}

/* Philosopher thread routine */
static void *philosopher_thread(void *arg) {
    int id = *((int *)arg);
    free(arg);

    SimulationContext *ctx = NULL;
    /* Note: Context will be accessed via global/thread parameter passed */
    extern SimulationContext *g_current_sim_ctx;
    ctx = g_current_sim_ctx;
    if (!ctx) return NULL;

    int left_fork = id;
    int right_fork = (id + 1) % ctx->num_philosophers;

    while (!ctx->stop_requested && ctx->stats[id].meals_eaten < ctx->max_meals) {
        /* Check pause before thinking */
        check_pause_and_stop(ctx);
        if (ctx->stop_requested) break;

        /* 1. THINKING STATE */
        pthread_mutex_lock(&ctx->state_mutex);
        ctx->stats[id].state = STATE_THINKING;
        pthread_mutex_unlock(&ctx->state_mutex);

        char msg[128];
        snprintf(msg, sizeof(msg), "Philosopher %d is thinking.", id);
        sim_emit_event(ctx, EVENT_STATE_CHANGE, id, -1, msg);

        responsive_sleep_ms(ctx, ctx->speed_ms);
        if (ctx->stop_requested) break;

        /* 2. HUNGRY STATE */
        check_pause_and_stop(ctx);
        if (ctx->stop_requested) break;

        pthread_mutex_lock(&ctx->state_mutex);
        ctx->stats[id].state = STATE_HUNGRY;
        ctx->stats[id].last_hungry_time_ms = get_current_time_ms();
        ctx->stats[id].waiting_events++;
        pthread_mutex_unlock(&ctx->state_mutex);

        snprintf(msg, sizeof(msg), "Philosopher %d is hungry and waiting for resources.", id);
        sim_emit_event(ctx, EVENT_STATE_CHANGE, id, -1, msg);

        /* STRATEGY 0: DEADLOCK PREVENTION (N - 1 Semaphore) */
        if (ctx->strategy == STRATEGY_DEADLOCK_PREVENTION) {
            /* Acquire semaphore permit (max N-1 philosophers in table room) */
            check_pause_and_stop(ctx);
            if (ctx->stop_requested) break;

            sem_wait(&ctx->permit_semaphore);
            if (ctx->stop_requested) {
                sem_post(&ctx->permit_semaphore);
                break;
            }

            pthread_mutex_lock(&ctx->state_mutex);
            ctx->current_permits--;
            pthread_mutex_unlock(&ctx->state_mutex);

            snprintf(msg, sizeof(msg), "Philosopher %d acquired a semaphore permit (%d permits remaining).",
                     id, ctx->current_permits);
            sim_emit_event(ctx, EVENT_PERMIT_ACQUIRED, id, -1, msg);

            /* Acquire Left Fork */
            pthread_mutex_lock(&ctx->fork_mutexes[left_fork]);
            pthread_mutex_lock(&ctx->state_mutex);
            ctx->fork_owners[left_fork] = id;
            ctx->stats[id].forks_held[0] = left_fork;
            pthread_mutex_unlock(&ctx->state_mutex);

            snprintf(msg, sizeof(msg), "Philosopher %d acquired left fork %d.", id, left_fork);
            sim_emit_event(ctx, EVENT_FORK_ACQUIRED, id, left_fork, msg);

            if (ctx->speed_ms > 200) {
                responsive_sleep_ms(ctx, ctx->speed_ms / 3);
            }
            if (ctx->stop_requested) {
                /* Clean up held left fork and permit */
                pthread_mutex_lock(&ctx->state_mutex);
                ctx->fork_owners[left_fork] = -1;
                ctx->stats[id].forks_held[0] = -1;
                ctx->current_permits++;
                pthread_mutex_unlock(&ctx->state_mutex);
                pthread_mutex_unlock(&ctx->fork_mutexes[left_fork]);
                sem_post(&ctx->permit_semaphore);
                break;
            }

            /* Acquire Right Fork */
            pthread_mutex_lock(&ctx->fork_mutexes[right_fork]);
            pthread_mutex_lock(&ctx->state_mutex);
            ctx->fork_owners[right_fork] = id;
            ctx->stats[id].forks_held[1] = right_fork;
            pthread_mutex_unlock(&ctx->state_mutex);

            snprintf(msg, sizeof(msg), "Philosopher %d acquired right fork %d.", id, right_fork);
            sim_emit_event(ctx, EVENT_FORK_ACQUIRED, id, right_fork, msg);

            /* 3. EATING STATE */
            pthread_mutex_lock(&ctx->state_mutex);
            ctx->stats[id].state = STATE_EATING;
            ctx->current_eating_count++;
            if (ctx->current_eating_count > ctx->max_concurrent_eating) {
                ctx->max_concurrent_eating = ctx->current_eating_count;
            }
            uint64_t wait_duration = get_current_time_ms() - ctx->stats[id].last_hungry_time_ms;
            ctx->stats[id].total_wait_time_ms += wait_duration;
            ctx->stats[id].meals_eaten++;
            ctx->stats[id].successful_acquisitions++;
            pthread_mutex_unlock(&ctx->state_mutex);

            snprintf(msg, sizeof(msg), "Philosopher %d is eating meal %d of %d (waited %llu ms).",
                     id, ctx->stats[id].meals_eaten, ctx->max_meals, (unsigned long long)wait_duration);
            sim_emit_event(ctx, EVENT_STATE_CHANGE, id, -1, msg);

            responsive_sleep_ms(ctx, ctx->speed_ms);

            /* 4. RELEASING FORKS */
            pthread_mutex_lock(&ctx->state_mutex);
            ctx->current_eating_count--;
            ctx->stats[id].state = STATE_RELEASING_FORKS;
            pthread_mutex_unlock(&ctx->state_mutex);

            /* Release right fork */
            pthread_mutex_lock(&ctx->state_mutex);
            ctx->fork_owners[right_fork] = -1;
            ctx->stats[id].forks_held[1] = -1;
            pthread_mutex_unlock(&ctx->state_mutex);
            pthread_mutex_unlock(&ctx->fork_mutexes[right_fork]);

            snprintf(msg, sizeof(msg), "Philosopher %d released right fork %d.", id, right_fork);
            sim_emit_event(ctx, EVENT_FORK_RELEASED, id, right_fork, msg);

            /* Release left fork */
            pthread_mutex_lock(&ctx->state_mutex);
            ctx->fork_owners[left_fork] = -1;
            ctx->stats[id].forks_held[0] = -1;
            pthread_mutex_unlock(&ctx->state_mutex);
            pthread_mutex_unlock(&ctx->fork_mutexes[left_fork]);

            snprintf(msg, sizeof(msg), "Philosopher %d released left fork %d.", id, left_fork);
            sim_emit_event(ctx, EVENT_FORK_RELEASED, id, left_fork, msg);

            /* Release semaphore permit */
            sem_post(&ctx->permit_semaphore);
            pthread_mutex_lock(&ctx->state_mutex);
            ctx->current_permits++;
            pthread_mutex_unlock(&ctx->state_mutex);

            snprintf(msg, sizeof(msg), "Philosopher %d released semaphore permit (%d permits available).",
                     id, ctx->current_permits);
            sim_emit_event(ctx, EVENT_PERMIT_RELEASED, id, -1, msg);

        } else {
            /* STRATEGY 1: UNSAFE DEADLOCK DEMONSTRATION */
            /* Every philosopher grabs their left fork, then waits at barrier so EVERYONE holds their left fork simultaneously */
            pthread_mutex_lock(&ctx->fork_mutexes[left_fork]);
            pthread_mutex_lock(&ctx->state_mutex);
            ctx->fork_owners[left_fork] = id;
            ctx->stats[id].forks_held[0] = left_fork;
            pthread_mutex_unlock(&ctx->state_mutex);

            snprintf(msg, sizeof(msg), "Philosopher %d acquired left fork %d [Unsafe Mode: Hold and Wait].", id, left_fork);
            sim_emit_event(ctx, EVENT_FORK_ACQUIRED, id, left_fork, msg);

            /* Synchronize all threads so every philosopher holds left fork at once */
            pthread_barrier_wait(&ctx->deadlock_barrier);

            /* Now each tries to acquire right fork */
            pthread_mutex_lock(&ctx->state_mutex);
            ctx->stats[id].state = STATE_ACQUIRING_FORKS;
            pthread_mutex_unlock(&ctx->state_mutex);

            snprintf(msg, sizeof(msg), "Philosopher %d is waiting for right fork %d (held by Philosopher %d) -> CIRCULAR WAIT!",
                     id, right_fork, (id + 1) % ctx->num_philosophers);
            sim_emit_event(ctx, EVENT_STATE_CHANGE, id, right_fork, msg);

            /* Try to lock right fork with timeout/trylock to avoid freezing process indefinitely */
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 4; /* 4 seconds deadlock observation period */

            int lock_res = pthread_mutex_timedlock(&ctx->fork_mutexes[right_fork], &ts);
            if (lock_res == ETIMEDOUT) {
                /* Deadlock timed out safely */
                pthread_mutex_lock(&ctx->state_mutex);
                ctx->stats[id].state = STATE_DEADLOCKED;
                ctx->deadlock_occurred = true;
                pthread_mutex_unlock(&ctx->state_mutex);

                snprintf(msg, sizeof(msg), "Deadlock timeout reached for Philosopher %d. Safely unblocking demonstration.", id);
                sim_emit_event(ctx, EVENT_DEADLOCK_DETECTED, id, right_fork, msg);
            } else if (lock_res == 0) {
                /* If acquired, release it */
                pthread_mutex_unlock(&ctx->fork_mutexes[right_fork]);
            }

            /* Release left fork to safely end round */
            pthread_mutex_lock(&ctx->state_mutex);
            ctx->fork_owners[left_fork] = -1;
            ctx->stats[id].forks_held[0] = -1;
            pthread_mutex_unlock(&ctx->state_mutex);
            pthread_mutex_unlock(&ctx->fork_mutexes[left_fork]);

            snprintf(msg, sizeof(msg), "Philosopher %d safely released left fork %d after deadlock detection.", id, left_fork);
            sim_emit_event(ctx, EVENT_FORK_RELEASED, id, left_fork, msg);

            /* End after one deadlock cycle demonstration */
            break;
        }
    }

    /* Completed state */
    pthread_mutex_lock(&ctx->state_mutex);
    if (!ctx->deadlock_occurred) {
        ctx->stats[id].state = STATE_COMPLETED;
    }
    pthread_mutex_unlock(&ctx->state_mutex);

    char final_msg[128];
    snprintf(final_msg, sizeof(final_msg), "Philosopher %d finished simulation (%d meals eaten).",
             id, ctx->stats[id].meals_eaten);
    sim_emit_event(ctx, EVENT_STATE_CHANGE, id, -1, final_msg);

    return NULL;
}

/* Watchdog / monitor thread: detects circular wait and completion */
static void *monitor_thread(void *arg) {
    SimulationContext *ctx = (SimulationContext *)arg;
    if (!ctx) return NULL;

    while (!ctx->stop_requested) {
        usleep(100000); /* 100 ms check */

        pthread_mutex_lock(&ctx->state_mutex);
        bool all_done = true;
        int holding_one_fork_count = 0;

        for (int i = 0; i < ctx->num_philosophers; i++) {
            if (ctx->stats[i].state != STATE_COMPLETED && ctx->stats[i].state != STATE_DEADLOCKED) {
                all_done = false;
            }
            if (ctx->stats[i].forks_held[0] != -1 && ctx->stats[i].forks_held[1] == -1) {
                holding_one_fork_count++;
            }
        }

        /* Check for circular wait in unsafe strategy */
        if (ctx->strategy == STRATEGY_UNSAFE_DEADLOCK && !ctx->deadlock_occurred) {
            if (holding_one_fork_count == ctx->num_philosophers) {
                ctx->deadlock_occurred = true;
                for (int i = 0; i < ctx->num_philosophers; i++) {
                    ctx->stats[i].state = STATE_DEADLOCKED;
                }
                pthread_mutex_unlock(&ctx->state_mutex);

                char deadlock_msg[256];
                snprintf(deadlock_msg, sizeof(deadlock_msg),
                         "CIRCULAR WAIT CONFIRMED: All %d philosophers hold their left fork and wait for their right fork. Deadlock detected!",
                         ctx->num_philosophers);
                sim_emit_event(ctx, EVENT_DEADLOCK_DETECTED, -1, -1, deadlock_msg);
                continue;
            }
        }

        if (all_done && ctx->is_running) {
            ctx->is_running = false;
            pthread_mutex_unlock(&ctx->state_mutex);

            sim_emit_event(ctx, EVENT_FINISHED, -1, -1, "All philosophers completed their configured meals or demonstration.");
            break;
        }

        pthread_mutex_unlock(&ctx->state_mutex);
    }

    return NULL;
}

/* Global context reference for thread worker access */
SimulationContext *g_current_sim_ctx = NULL;

int sim_init(SimulationContext *ctx, int num_philosophers, int max_meals, SimulationStrategy strategy, int speed_ms, bool json_mode) {
    if (!ctx) return -1;
    if (num_philosophers < MIN_PHILOSOPHERS || num_philosophers > MAX_PHILOSOPHERS) return -2;
    if (max_meals < 1) return -3;

    memset(ctx, 0, sizeof(SimulationContext));
    ctx->num_philosophers = num_philosophers;
    ctx->max_meals = max_meals;
    ctx->strategy = strategy;
    ctx->speed_ms = (speed_ms > 0) ? speed_ms : DEFAULT_SPEED_MS;
    ctx->json_mode = json_mode;
    ctx->is_running = false;
    ctx->is_paused = false;
    ctx->step_mode = false;
    ctx->stop_requested = false;
    ctx->deadlock_occurred = false;
    ctx->total_steps = 0;
    ctx->start_time_ms = get_current_time_ms();
    ctx->current_permits = num_philosophers - 1; /* N - 1 permits */

    pthread_mutex_init(&ctx->state_mutex, NULL);
    pthread_mutex_init(&ctx->pause_mutex, NULL);
    pthread_cond_init(&ctx->pause_cond, NULL);

    pthread_barrier_init(&ctx->deadlock_barrier, NULL, num_philosophers);

    /* Initialize counting semaphore with value N - 1 */
    if (sem_init(&ctx->permit_semaphore, 0, num_philosophers - 1) != 0) {
        perror("sem_init failed");
        return -4;
    }

    /* Initialize fork mutexes and philosopher stats */
    for (int i = 0; i < num_philosophers; i++) {
        pthread_mutex_init(&ctx->fork_mutexes[i], NULL);
        ctx->fork_owners[i] = -1;

        ctx->stats[i].id = i;
        ctx->stats[i].state = STATE_THINKING;
        ctx->stats[i].meals_eaten = 0;
        ctx->stats[i].total_wait_time_ms = 0;
        ctx->stats[i].last_hungry_time_ms = 0;
        ctx->stats[i].forks_held[0] = -1;
        ctx->stats[i].forks_held[1] = -1;
        ctx->stats[i].successful_acquisitions = 0;
        ctx->stats[i].waiting_events = 0;
    }

    g_current_sim_ctx = ctx;

    sim_emit_event(ctx, EVENT_INIT, -1, -1, "Simulation context initialized successfully.");
    return 0;
}

int sim_start(SimulationContext *ctx) {
    if (!ctx) return -1;
    ctx->is_running = true;
    ctx->stop_requested = false;
    ctx->start_time_ms = get_current_time_ms();

    /* Create philosopher threads */
    for (int i = 0; i < ctx->num_philosophers; i++) {
        int *id_arg = (int *)malloc(sizeof(int));
        *id_arg = i;
        if (pthread_create(&ctx->phil_threads[i], NULL, philosopher_thread, id_arg) != 0) {
            perror("Failed to create philosopher thread");
            return -1;
        }
    }

    /* Create monitor thread */
    if (pthread_create(&ctx->monitor_thread, NULL, monitor_thread, ctx) != 0) {
        perror("Failed to create monitor thread");
        return -1;
    }

    return 0;
}

void sim_pause(SimulationContext *ctx) {
    if (!ctx) return;
    pthread_mutex_lock(&ctx->pause_mutex);
    ctx->is_paused = true;
    pthread_mutex_unlock(&ctx->pause_mutex);
    sim_emit_event(ctx, EVENT_PAUSED, -1, -1, "Simulation paused by user.");
}

void sim_resume(SimulationContext *ctx) {
    if (!ctx) return;
    pthread_mutex_lock(&ctx->pause_mutex);
    ctx->is_paused = false;
    ctx->step_mode = false;
    pthread_cond_broadcast(&ctx->pause_cond);
    pthread_mutex_unlock(&ctx->pause_mutex);
    sim_emit_event(ctx, EVENT_RESUMED, -1, -1, "Simulation resumed by user.");
}

void sim_step(SimulationContext *ctx) {
    if (!ctx) return;
    pthread_mutex_lock(&ctx->pause_mutex);
    ctx->step_mode = true;
    pthread_cond_broadcast(&ctx->pause_cond);
    pthread_mutex_unlock(&ctx->pause_mutex);
    sim_emit_event(ctx, EVENT_STEP_COMPLETED, -1, -1, "Simulation single step executed.");
}

void sim_set_speed(SimulationContext *ctx, int speed_ms) {
    if (!ctx) return;
    pthread_mutex_lock(&ctx->state_mutex);
    ctx->speed_ms = (speed_ms >= 50) ? speed_ms : 50;
    pthread_mutex_unlock(&ctx->state_mutex);
}

void sim_stop(SimulationContext *ctx) {
    if (!ctx) return;
    ctx->stop_requested = true;

    /* Wake up any threads paused or waiting */
    pthread_mutex_lock(&ctx->pause_mutex);
    ctx->is_paused = false;
    pthread_cond_broadcast(&ctx->pause_cond);
    pthread_mutex_unlock(&ctx->pause_mutex);

    /* Join threads */
    for (int i = 0; i < ctx->num_philosophers; i++) {
        if (ctx->phil_threads[i]) {
            pthread_join(ctx->phil_threads[i], NULL);
            ctx->phil_threads[i] = 0;
        }
    }

    if (ctx->monitor_thread) {
        pthread_join(ctx->monitor_thread, NULL);
        ctx->monitor_thread = 0;
    }

    ctx->is_running = false;
}

void sim_cleanup(SimulationContext *ctx) {
    if (!ctx) return;
    sim_stop(ctx);

    for (int i = 0; i < ctx->num_philosophers; i++) {
        pthread_mutex_destroy(&ctx->fork_mutexes[i]);
    }

    pthread_mutex_destroy(&ctx->state_mutex);
    pthread_mutex_destroy(&ctx->pause_mutex);
    pthread_cond_destroy(&ctx->pause_cond);
    pthread_barrier_destroy(&ctx->deadlock_barrier);
    sem_destroy(&ctx->permit_semaphore);

    if (g_current_sim_ctx == ctx) {
        g_current_sim_ctx = NULL;
    }
}
