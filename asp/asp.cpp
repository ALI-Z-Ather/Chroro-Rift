// ============================================================================
// asp.cpp — Main Entry Point for the Automated Strategic Process
// Responsibilities:
//   1. Connect to shared memory created by Arbiter
//   2. Read enemy count from GameState
//   3. Create one thread per NPC
//   4. Handle SIGUSR1 (stun), SIGUSR2/SIGSTOP (ultimate suspension)
//   5. Cleanup when game ends
// ============================================================================

#include <iostream>
#include <cstdlib>
#include <pthread.h>

#include "../shared/game_config.h"
#include "../shared/game_state.h"
#include "../shared/sync.h"

#include "npc_controller.h"
#include "ai_strategy.h"

// ============================================================================
// Global State
// ============================================================================
GameState* g_state = nullptr;

#ifdef __APPLE__
sem_t* g_mutex = nullptr;
sem_t* g_turn_notify = nullptr;
sem_t* g_action_ready = nullptr;
sem_t* g_artifact_mutex = nullptr;
#endif

// ============================================================================
// Signal Handlers
// ============================================================================
static void stun_handler(int sig) {
    (void)sig;
    // Stun: sleep for STUN_DURATION_SEC
    sleep(STUN_DURATION_SEC);
}

static void suspend_handler(int sig) {
    (void)sig;
    // Ultimate Ability suspension — handled by SIGSTOP/SIGCONT from Arbiter
    // This handler is for SIGUSR2 if we use that instead
    pause();  // Suspend until another signal arrives
}

static void sigterm_handler(int sig) {
    (void)sig;
    if (g_state) {
        g_state->game_over = true;
    }
}

// ============================================================================
// Main
// ============================================================================
int main() {
    std::cout << "=== Chrono Rift — Automated Strategic Process ===" << std::endl;

    // Register signal handlers
    struct sigaction sa;
    
    sa.sa_handler = stun_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, nullptr);
    
    sa.sa_handler = suspend_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR2, &sa, nullptr);
    
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);

    // -----------------------------------------------------------------------
    // 1. Connect to Shared Memory
    // -----------------------------------------------------------------------
    std::cout << "[ASP] Connecting to shared memory..." << std::endl;
    int shm_fd = -1;
    while ((shm_fd = shm_open(SHM_NAME, O_RDWR, 0666)) == -1) {
        usleep(50000);  // Retry every 50ms
    }

    g_state = (GameState*)mmap(0, sizeof(GameState), PROT_READ | PROT_WRITE,
                                MAP_SHARED, shm_fd, 0);
    if (g_state == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Open semaphores
    MUTEX_OPEN();
    TURN_SEM_OPEN();
    ACTION_SEM_OPEN();
    ARTIFACT_MUTEX_OPEN();

    // Wait for Arbiter to finish initialization
    while (!g_state->initialized) {
        usleep(50000);
    }

    std::cout << "[ASP] Connected to Arbiter." << std::endl;

    // -----------------------------------------------------------------------
    // 2. Read Enemy Count & Register PID
    // -----------------------------------------------------------------------
    MUTEX_LOCK();
    int num_enemies = g_state->num_enemies;
    g_state->asp_pid = getpid();
    
    // Set process PID on all enemies for signal targeting
    for (int i = 0; i < num_enemies; i++) {
        g_state->enemies[i].process_pid = getpid();
    }
    
    g_state->asp_ready = true;
    MUTEX_UNLOCK();

    std::cout << "[ASP] Managing " << num_enemies << " NPC(s)." << std::endl;

    // -----------------------------------------------------------------------
    // 3. Create NPC Threads (One per Enemy)
    // -----------------------------------------------------------------------
    pthread_t npc_threads[MAX_ENEMIES];
    int npc_ids[MAX_ENEMIES];
    
    for (int i = 0; i < num_enemies; i++) {
        npc_ids[i] = i;
        pthread_create(&npc_threads[i], nullptr, npc_thread, &npc_ids[i]);
    }

    // -----------------------------------------------------------------------
    // 4. Wait for All NPC Threads to Complete
    // -----------------------------------------------------------------------
    for (int i = 0; i < num_enemies; i++) {
        pthread_join(npc_threads[i], nullptr);
    }

    // -----------------------------------------------------------------------
    // 5. Cleanup
    // -----------------------------------------------------------------------
    std::cout << "[ASP] All NPC threads exited. Shutting down." << std::endl;
    
    SEM_CLOSE_ALL();
    munmap(g_state, sizeof(GameState));
    
    return 0;
}
