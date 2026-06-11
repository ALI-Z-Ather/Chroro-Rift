#include "input_handler.h"
#include "../shared/sync.h"
#include <cstdio>
#include <unistd.h>

using namespace std;

extern GameState* g_state;

void* player_thread(void* arg) {
    int player_id = *(int*)arg;
    printf("[HIP] Player %d thread started\n", player_id);

    while (true) {
        usleep(50000);

        MUTEX_LOCK();

        if (g_state->game_over) {
            MUTEX_UNLOCK();
            break;
        }

        if (g_state->turn.turn_pending &&
            g_state->turn.active_is_player &&
            g_state->turn.active_id == player_id &&
            !g_state->turn.turn_claimed) {
            g_state->turn.turn_claimed = true;
        }

        MUTEX_UNLOCK();
    }

    printf("[HIP] Player %d thread exiting\n", player_id);
    return nullptr;
}

