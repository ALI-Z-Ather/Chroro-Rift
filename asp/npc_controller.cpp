// ============================================================================
// npc_controller.cpp — Per-NPC Thread Implementation
//
// NPC threads poll game state for their turn and submit AI decisions directly.
// No semaphore wait needed — the scheduler detects submission via current_action.submitted.
// ============================================================================

#include "npc_controller.h"
#include "ai_strategy.h"
#include "../shared/sync.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

extern GameState* g_state;

void* npc_thread(void* arg) {
    int npc_id = *(int*)arg;
    unsigned int rng_seed = ROLL_NUMBER + npc_id;  // Unique seed per NPC

    printf("[ASP] NPC %d thread started\n", npc_id);

    while (true) {
        usleep(20000);  // Poll every 20ms — fast enough for responsive AI

        MUTEX_LOCK();

        // Exit conditions
        if (g_state->game_over) {
            MUTEX_UNLOCK();
            break;
        }
        if (!g_state->enemies[npc_id].is_active) {
            MUTEX_UNLOCK();
            break;
        }

        // Check if it's THIS NPC's turn and not already submitted
        if (g_state->turn.turn_pending &&
            !g_state->turn.active_is_player &&
            g_state->turn.active_id == npc_id &&
            !g_state->current_action.submitted) {

            g_state->turn.turn_claimed = true;

            // Decide action (mutex held — AI reads state safely)
            ActionRequest action = decide_npc_action(g_state, npc_id, &rng_seed);

            // Submit
            g_state->current_action           = action;
            g_state->current_action.submitted = true;

            MUTEX_UNLOCK();

            // Notify scheduler
            ACTION_SEM_POST();
        } else {
            MUTEX_UNLOCK();
        }
    }

    printf("[ASP] NPC %d thread exiting\n", npc_id);
    return nullptr;
}
