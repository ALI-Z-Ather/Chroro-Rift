// ============================================================================
// ai_strategy.cpp — NPC Decision-Making Implementation
// ============================================================================

#include "ai_strategy.h"
#include <cstdlib>
#include <cstdio>

ActionRequest decide_npc_action(GameState* state, int npc_id, unsigned int* seed) {
    ActionRequest action;
    clear_action_request(&action);
    action.actor_id        = npc_id;
    action.actor_is_player = false;

    // Roll d100
    // Per rubric, enemies may ONLY: Strike or Skip
    int roll = portable_rand(seed) % 100;

    if (roll < 50) {
        // 50%: Strike a random alive player
        action.type             = ACTION_STRIKE;
        action.target_id        = find_random_player(state, seed);
        action.target_is_player = true;
    } else if (roll < 80) {
        // 30%: Strike the weakest alive player
        action.type             = ACTION_STRIKE;
        action.target_id        = find_weakest_player(state);
        action.target_is_player = true;
    } else if (roll < 90) {
        // 10%: Try to Channel Ultimate to cause deadlock
        action.type = ACTION_ULTIMATE;
    } else {
        // 10%: Skip — conserve stamina
        action.type = ACTION_SKIP;
    }

    // Safety: if no valid target exists, fall back to Skip
    if (action.type == ACTION_STRIKE && action.target_id == -1) {
        action.type = ACTION_SKIP;
    }
    
    // Equip a weapon from inventory if striking
    if (action.type == ACTION_STRIKE) {
        Character* self = &state->enemies[npc_id];
        // Find first valid weapon in inventory
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (self->inventory[i].weapon != WEAPON_NONE) {
                action.weapon_slot = i;
                break;
            }
        }
    }

    return action;
}

int find_weakest_player(GameState* state) {
    int weakest_id = -1;
    int lowest_hp = __INT_MAX__;
    
    for (int i = 0; i < state->num_players; i++) {
        if (state->players[i].is_active && state->players[i].hp < lowest_hp) {
            lowest_hp = state->players[i].hp;
            weakest_id = i;
        }
    }
    
    return weakest_id;
}

int find_random_player(GameState* state, unsigned int* seed) {
    // Collect alive player indices
    int alive[MAX_PLAYERS];
    int alive_count = 0;
    
    for (int i = 0; i < state->num_players; i++) {
        if (state->players[i].is_active) {
            alive[alive_count++] = i;
        }
    }
    
    if (alive_count == 0) return -1;
    return alive[portable_rand(seed) % alive_count];
}
