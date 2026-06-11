#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <string>

#include "../shared/game_config.h"
#include "../shared/game_state.h"
#include "../shared/sync.h"

#include "input_handler.h"
#include "player_actions.h"

using namespace std;

GameState* g_state = nullptr;

#ifdef __APPLE__
sem_t* g_mutex = nullptr;
sem_t* g_turn_notify = nullptr;
sem_t* g_action_ready = nullptr;
sem_t* g_artifact_mutex = nullptr;
#endif

static void stun_handler(int sig) {
    (void)sig;
    sleep(STUN_DURATION_SEC);
}

static void sigterm_handler(int sig) {
    (void)sig;
    if (g_state) {
        g_state->game_over = true;
    }
}

int main(int argc, char** argv) {
    cout << "=== Chrono Rift — Human Interfacing Process ===" << endl;

    struct sigaction sa;
    
    sa.sa_handler = stun_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, nullptr);
    
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);

    cout << "[HIP] Connecting to shared memory..." << endl;
    int shm_fd = -1;
    while ((shm_fd = shm_open(SHM_NAME, O_RDWR, 0666)) == -1) {
        usleep(50000);
    }

    g_state = (GameState*)mmap(0, sizeof(GameState), PROT_READ | PROT_WRITE,
                                MAP_SHARED, shm_fd, 0);
    if (g_state == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    MUTEX_OPEN();
    TURN_SEM_OPEN();
    ACTION_SEM_OPEN();
    ARTIFACT_MUTEX_OPEN();

    while (!g_state->initialized) {
        usleep(50000);
    }

    cout << "[HIP] Connected to Arbiter." << endl;

    int hip_index = -1;
    if (argc > 1) {
        hip_index = atoi(argv[1]);
    }

    MUTEX_LOCK();
    int num_players = g_state->num_players;
    
    int start_idx = 0;
    int end_idx = num_players;
    
    if (hip_index == 0) {
        end_idx = num_players / 2;
        if (end_idx == 0) end_idx = 1;
    } else if (hip_index == 1) {
        start_idx = num_players / 2;
        if (start_idx == 0 && num_players > 1) start_idx = 1;
        if (num_players == 1) {
            start_idx = 0;
            end_idx = 0;
        }
    }
    
    for (int i = start_idx; i < end_idx; i++) {
        g_state->players[i].process_pid = getpid();
    }
    
    g_state->hips_connected++;
    MUTEX_UNLOCK();

    if (start_idx < end_idx) {
        cout << "[HIP " << hip_index << "] Managing players " << start_idx << " to " << (end_idx - 1) << "." << endl;
    } else {
        cout << "[HIP " << hip_index << "] No players to manage." << endl;
    }

    pthread_t player_threads[MAX_PLAYERS];
    int player_ids[MAX_PLAYERS];
    
    for (int i = start_idx; i < end_idx; i++) {
        player_ids[i] = i;
        pthread_create(&player_threads[i], nullptr, player_thread, &player_ids[i]);
    }

    for (int i = start_idx; i < end_idx; i++) {
        pthread_join(player_threads[i], nullptr);
    }

    cout << "[HIP] All player threads exited. Shutting down." << endl;
    
    SEM_CLOSE_ALL();
    munmap(g_state, sizeof(GameState));
    
    return 0;
}

