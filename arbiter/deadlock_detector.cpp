#include "deadlock_detector.h"
#include "../shared/sync.h"
#include <cstdio>
#include <unistd.h>

using namespace std;

extern GameState* g_state;

bool detect_and_resolve_deadlock(GameState* state) {
    Character* all_entities[MAX_PLAYERS + MAX_ENEMIES];
    int count = 0;
    
    for (int i = 0; i < state->num_players; i++) {
        if (state->players[i].is_active) all_entities[count++] = &state->players[i];
    }
    for (int i = 0; i < state->num_enemies; i++) {
        if (state->enemies[i].is_active) all_entities[count++] = &state->enemies[i];
    }

    Character* holder_of_solar = nullptr;
    Character* holder_of_lunar = nullptr;
    Character* holder_of_eclipse = nullptr;

    for (int i = 0; i < count; i++) {
        if (all_entities[i]->holds_solar_core) holder_of_solar = all_entities[i];
        if (all_entities[i]->holds_lunar_blade) holder_of_lunar = all_entities[i];
        if (all_entities[i]->holds_eclipse_relic) holder_of_eclipse = all_entities[i];
    }

    Character* waiting_for[MAX_PLAYERS + MAX_ENEMIES] = {nullptr};
    
    for (int i = 0; i < count; i++) {
        if (all_entities[i]->waiting_for_solar_core && holder_of_solar != nullptr) {
            waiting_for[i] = holder_of_solar;
        } else if (all_entities[i]->waiting_for_lunar_blade && holder_of_lunar != nullptr) {
            waiting_for[i] = holder_of_lunar;
        } else if (all_entities[i]->waiting_for_eclipse_relic && holder_of_eclipse != nullptr) {
            waiting_for[i] = holder_of_eclipse;
        }
    }

    for (int i = 0; i < count; i++) {
        Character* current = all_entities[i];
        Character* next = waiting_for[i];
        int chain_len = 1;
        
        while (next != nullptr && chain_len <= 3) {
            if (next == current) {
                WeaponType forced_drop = WEAPON_NONE;
                if (current->holds_solar_core) {
                    current->holds_solar_core = false;
                    release_solar_core(&state->artifacts);
                    forced_drop = WEAPON_SOLAR_CORE;
                } else if (current->holds_lunar_blade) {
                    current->holds_lunar_blade = false;
                    release_lunar_blade(&state->artifacts);
                    forced_drop = WEAPON_LUNAR_BLADE;
                } else if (current->holds_eclipse_relic) {
                    current->holds_eclipse_relic = false;
                    release_eclipse_relic(&state->artifacts);
                    forced_drop = WEAPON_ECLIPSE_RELIC;
                }
                
                if (forced_drop != WEAPON_NONE) {
                    for (int inv_idx = 0; inv_idx < INVENTORY_SLOTS; inv_idx++) {
                        if (current->inventory[inv_idx].weapon == forced_drop) {
                            remove_weapon_at(current->inventory, inv_idx);
                            break;
                        }
                    }
                    
                    Character* waiter = nullptr;
                    for (int j = 0; j < count; j++) {
                        if (waiting_for[j] == current) {
                            waiter = all_entities[j];
                            break;
                        }
                    }
                    if (waiter) {
                        int slot = find_contiguous_free(waiter->inventory, weapon_slot_size(forced_drop));
                        if (slot != -1) {
                            place_weapon_at(waiter->inventory, forced_drop, slot);
                            bool is_player = (waiter >= &state->players[0] && waiter <= &state->players[MAX_PLAYERS-1]);
                            if (forced_drop == WEAPON_SOLAR_CORE) {
                                waiter->holds_solar_core = true;
                                waiter->waiting_for_solar_core = false;
                                acquire_solar_core(&state->artifacts, waiter->id, is_player);
                            } else if (forced_drop == WEAPON_LUNAR_BLADE) {
                                waiter->holds_lunar_blade = true;
                                waiter->waiting_for_lunar_blade = false;
                                acquire_lunar_blade(&state->artifacts, waiter->id, is_player);
                            } else if (forced_drop == WEAPON_ECLIPSE_RELIC) {
                                waiter->holds_eclipse_relic = true;
                                waiter->waiting_for_eclipse_relic = false;
                                acquire_eclipse_relic(&state->artifacts, waiter->id, is_player);
                            }
                        } else {
                            shared_deposit(&state->shared_lts, forced_drop, -1);
                        }
                    } else {
                        shared_deposit(&state->shared_lts, forced_drop, -1);
                    }
                }
                
                current->waiting_for_solar_core = false;
                current->waiting_for_lunar_blade = false;
                current->waiting_for_eclipse_relic = false;
                
                return true; 
            }
            
            Character* next_next = nullptr;
            for (int j = 0; j < count; j++) {
                if (all_entities[j] == next) {
                    next_next = waiting_for[j];
                    break;
                }
            }
            next = next_next;
            chain_len++;
        }
    }

    return false;
}

void* deadlock_detector_thread(void* arg) {
    GameState* state = (GameState*)arg;
    
    while (!state->game_over) {
        MUTEX_LOCK();
        
        if (detect_and_resolve_deadlock(state)) {
            log_action(&state->action_log, state->game_tick,
                       "DEADLOCK DETECTED AND RESOLVED by Arbiter!");
        }
        
        MUTEX_UNLOCK();
        sleep(1);
    }
    
    return nullptr;
}

