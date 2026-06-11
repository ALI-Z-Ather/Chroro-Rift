#include "scheduler.h"
#include "../shared/sync.h"
#include "../shared/game_screens.h"
#include "combat.h"
#include <cstdio>
#include <unistd.h>


using namespace std;

extern GameState* g_state;

void tick_stamina(GameState* state, float elapsed_fraction) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    for (int i = 0; i < state->num_players; i++) {
        Character& p = state->players[i];
        if (!p.is_active) continue;
        if (p.is_stunned) {
            if (now.tv_sec > p.stun_end.tv_sec || 
               (now.tv_sec == p.stun_end.tv_sec && now.tv_nsec >= p.stun_end.tv_nsec)) {
                p.is_stunned = false;
            } else {
                continue;
            }
        }
        p.stamina += (int)(p.speed * elapsed_fraction);
        if (p.stamina > p.max_stamina) {
            p.stamina = p.max_stamina;
        }
    }

    if (!state->asp_suspended) {
        for (int i = 0; i < state->num_enemies; i++) {
            Character& e = state->enemies[i];
            if (!e.is_active) continue;
            if (e.is_stunned) {
                if (now.tv_sec > e.stun_end.tv_sec || 
                   (now.tv_sec == e.stun_end.tv_sec && now.tv_nsec >= e.stun_end.tv_nsec)) {
                    e.is_stunned = false;
                } else {
                    continue;
                }
            }
            e.stamina += (int)(e.speed * elapsed_fraction);
            if (e.stamina > e.max_stamina) {
                e.stamina = e.max_stamina;
            }
        }
    }

    state->game_tick++;
}

bool find_next_actor(GameState* state, int& out_id, bool& out_is_player) {
    for (int i = 0; i < state->num_players; i++) {
        Character& p = state->players[i];
        if (p.is_active && !p.is_stunned && p.stamina >= p.max_stamina) {
            out_id = i;
            out_is_player = true;
            return true;
        }
    }

    for (int i = 0; i < state->num_enemies; i++) {
        Character& e = state->enemies[i];
        if (e.is_active && !e.is_stunned && e.stamina >= e.max_stamina) {
            out_id = i;
            out_is_player = false;
            return true;
        }
    }

    return false;
}

void dispatch_turn(GameState* state, int actor_id, bool is_player) {
    state->turn.active_id = actor_id;
    state->turn.active_is_player = is_player;
    state->turn.turn_pending = true;
    state->turn.turn_claimed = false;
    clear_action_request(&state->current_action);
}

bool wait_for_action(GameState* state, bool is_player_turn) {
    (void)state;
    
    if (is_player_turn) {
        return true;
    } else {
        return true;
    }
}

void* scheduler_thread(void* arg) {
    GameState* state = (GameState*)arg;
    
    int expected_hips = (state->game_mode == MODE_TWO_PLAYER) ? 2 : 1;
    while (state->hips_connected < expected_hips || !state->asp_ready) {
        if (state->game_over) return nullptr;
        usleep(100000);
    }
    
    while (!state->game_over) {
        MUTEX_LOCK();
        
        float elapsed = (float)SCHEDULER_TICK_US / 1000000.0f;
        tick_stamina(state, elapsed);
        
        int actor_id;
        bool is_player;
        if (!state->turn.turn_pending && find_next_actor(state, actor_id, is_player)) {
            struct timespec turn_start, turn_end;
            clock_gettime(CLOCK_REALTIME, &turn_start);

            dispatch_turn(state, actor_id, is_player);
            
            MUTEX_UNLOCK();

            bool got_action = false;
            int wait_ticks = 0;
            const int ticks_per_sec = 1000000 / SCHEDULER_TICK_US;
            int max_ticks = is_player ? (600 * ticks_per_sec)
                                      : (NPC_TURN_TIMEOUT_SEC * ticks_per_sec);

            while (!got_action && wait_ticks < max_ticks) {
                usleep(SCHEDULER_TICK_US);
                MUTEX_LOCK();
                if (state->game_over) {
                    MUTEX_UNLOCK();
                    return nullptr;
                }
                got_action = state->current_action.submitted;
                MUTEX_UNLOCK();
                wait_ticks++;
            }
            
            clock_gettime(CLOCK_REALTIME, &turn_end);
            double turnaround_ms = (turn_end.tv_sec - turn_start.tv_sec) * 1000.0 +
                                   (turn_end.tv_nsec - turn_start.tv_nsec) / 1000000.0;
            printf("[Arbiter] Turnaround Analysis: %s %d took %.2f ms to submit action.\n",
                   is_player ? "Player" : "Enemy", actor_id, turnaround_ms);
            
            MUTEX_LOCK();
            if (!got_action) {
                state->current_action.type = ACTION_SKIP;
                state->current_action.actor_id = actor_id;
                state->current_action.actor_is_player = is_player;
                state->current_action.submitted = true;
            }
            
            execute_action(state, state->current_action);
            
            state->turn.turn_pending = false;
            state->turn.turn_claimed = false;
            clear_action_request(&state->current_action);
            
            MUTEX_UNLOCK();
        } else {
            MUTEX_UNLOCK();
        }
        
        usleep(SCHEDULER_TICK_US);
    }
    
    return nullptr;
}

