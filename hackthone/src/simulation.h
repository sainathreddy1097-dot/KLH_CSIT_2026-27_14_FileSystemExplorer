#ifndef SIMULATION_H
#define SIMULATION_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>

#define MIN_PHILOSOPHERS 2
#define MAX_PHILOSOPHERS 10
#define DEFAULT_PHILOSOPHERS 5
#define DEFAULT_MEALS 5
#define DEFAULT_SPEED_MS 600

typedef enum {
    STATE_THINKING = 0,
    STATE_HUNGRY,
    STATE_ACQUIRING_FORKS,
    STATE_EATING,
    STATE_RELEASING_FORKS,
    STATE_COMPLETED,
    STATE_DEADLOCKED
} PhilState;

typedef enum {
    STRATEGY_DEADLOCK_PREVENTION = 0, /* N-1 counting semaphore */
    STRATEGY_UNSAFE_DEADLOCK = 1      /* Demonstrates circular wait */
} SimulationStrategy;

typedef enum {
    EVENT_INIT,
    EVENT_STATE_CHANGE,
    EVENT_PERMIT_ACQUIRED,
    EVENT_PERMIT_RELEASED,
    EVENT_FORK_ACQUIRED,
    EVENT_FORK_RELEASED,
    EVENT_DEADLOCK_DETECTED,
    EVENT_STEP_COMPLETED,
    EVENT_PAUSED,
    EVENT_RESUMED,
    EVENT_RESET,
    EVENT_FINISHED,
    EVENT_ERROR
} SimulationEventType;

/* Statistics tracked per philosopher */
typedef struct {
    int id;
    PhilState state;
    int meals_eaten;
    uint64_t total_wait_time_ms;
    uint64_t last_hungry_time_ms;
    int forks_held[2]; /* [left_fork_id, right_fork_id], -1 if not held */
    int successful_acquisitions;
    int waiting_events;
} PhilStats;

/* Overall simulation configuration and state */
typedef struct {
    int num_philosophers;
    int max_meals;
    SimulationStrategy strategy;
    int speed_ms; /* base delay in ms */
    bool json_mode; /* emit JSON to stdout */
    bool interactive_cli; /* CLI command parser on stdin */

    /* Runtime state */
    bool is_running;
    bool is_paused;
    bool step_mode;
    bool stop_requested;
    bool deadlock_occurred;
    bool silent_mode; /* Suppress terminal prints in unit tests */
    uint64_t start_time_ms;
    int total_steps;
    int current_eating_count;
    int max_concurrent_eating;

    /* Synchronization primitives */
    pthread_mutex_t fork_mutexes[MAX_PHILOSOPHERS];
    int fork_owners[MAX_PHILOSOPHERS]; /* -1 if free, else phil id */
    sem_t permit_semaphore;            /* Initialized to N-1 for prevention */
    int current_permits;               /* Tracked permits */
    pthread_mutex_t state_mutex;       /* Protects shared simulation state & stats */

    /* Pause / step synchronization */
    pthread_mutex_t pause_mutex;
    pthread_cond_t pause_cond;

    /* Barrier for deterministic unsafe deadlock scenario */
    pthread_barrier_t deadlock_barrier;

    /* Threads */
    pthread_t phil_threads[MAX_PHILOSOPHERS];
    pthread_t monitor_thread;
    PhilStats stats[MAX_PHILOSOPHERS];
} SimulationContext;

/* API functions */
int sim_init(SimulationContext *ctx, int num_philosophers, int max_meals, SimulationStrategy strategy, int speed_ms, bool json_mode);
int sim_start(SimulationContext *ctx);
void sim_pause(SimulationContext *ctx);
void sim_resume(SimulationContext *ctx);
void sim_step(SimulationContext *ctx);
void sim_set_speed(SimulationContext *ctx, int speed_ms);
void sim_stop(SimulationContext *ctx);
void sim_cleanup(SimulationContext *ctx);
void sim_emit_event(SimulationContext *ctx, SimulationEventType type, int phil_id, int fork_id, const char *custom_msg);
uint64_t get_current_time_ms(void);
const char *phil_state_to_string(PhilState state);
const char *sim_strategy_to_string(SimulationStrategy strategy);

#endif /* SIMULATION_H */
