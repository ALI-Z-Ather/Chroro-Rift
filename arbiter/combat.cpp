#include "combat.h"
#include "signal_manager.h"
#include "../shared/sync.h"
#include "../shared/game_screens.h"
#include "enemy_spawner.h"
#include <cstdio>
#include <algorithm>

using namespace std;

extern GameState* g_state;

void execute_action(GameState* state, const ActionRequest& action) {
    Character& actor = action.actor_is_player 
        ? state->players[action.actor_id] 
        : state->enemies[action.actor_id];
    
    switch (action.type) {
        case ACTION_STRIKE: {
            Character& target = action.target_is_player
                ? state->players[action.target_id]
                : state->enemies[action.target_id];
            execute_strike(state, actor, target);
            check_entity_death(state, target, action.target_is_player);
            break;
        }
        case ACTION_EXHAUST: {
            Character& target = action.target_is_player
                ? state->players[action.target_id]
                : state->enemies[action.target_id];
            execute_exhaust(state, actor, target);
            break;
        }
        case ACTION_USE_WEAPON: {
            Character& target = action.target_is_player
                ? state->players[action.target_id]
                : state->enemies[action.target_id];
            execute_use_weapon(state, actor, target, action.weapon_slot);
            check_entity_death(state, target, action.target_is_player);
            break;
        }
        case ACTION_SWAP_IN:
            execute_swap_in(state, actor, action.swap_weapon);
            break;
        case ACTION_PUT_STORAGE:
            execute_put_storage(state, actor, action.weapon_slot);
            break;
        case ACTION_HEAL:
            execute_heal(state, actor);
            break;
        case ACTION_SKIP:
            execute_skip(state, actor);
            break;
        case ACTION_ULTIMATE:
            execute_ultimate(state, actor);
            break;
        default:
            break;
    }
    
    check_win_lose_conditions(state);
}

void execute_strike(GameState* state, Character& actor, Character& target) {
    target.hp -= actor.damage;
    if (target.hp < 0) target.hp = 0;
    actor.stamina = 0;

    bool actor_is_p  = (&actor  >= &state->players[0] && &actor  <= &state->players[MAX_PLAYERS-1]);
    bool target_is_p = (&target >= &state->players[0] && &target <= &state->players[MAX_PLAYERS-1]);

    log_action(&state->action_log, state->game_tick,
               "%s %d strikes %s %d for %d damage!",
               actor_is_p  ? "Player" : "Enemy", actor.id,
               target_is_p ? "Player" : "Enemy", target.id,
               actor.damage);
}

void execute_exhaust(GameState* state, Character& actor, Character& target) {
    target.stamina -= actor.damage;
    if (target.stamina < 0) target.stamina = 0;
    actor.stamina = 0;

    bool actor_is_p  = (&actor  >= &state->players[0] && &actor  <= &state->players[MAX_PLAYERS-1]);
    bool target_is_p = (&target >= &state->players[0] && &target <= &state->players[MAX_PLAYERS-1]);
    log_action(&state->action_log, state->game_tick,
               "%s %d exhausts %s %d's stamina by %d!",
               actor_is_p ? "Player" : "Enemy", actor.id,
               target_is_p ? "Player" : "Enemy", target.id,
               actor.damage);
}

void execute_use_weapon(GameState* state, Character& actor, Character& target, int weapon_slot) {
    WeaponType weapon = actor.inventory[weapon_slot].weapon;
    if (weapon == WEAPON_NONE) {
        execute_strike(state, actor, target);
        return;
    }

    int dmg = weapon_damage(weapon);
    target.hp -= dmg;
    if (target.hp < 0) target.hp = 0;
    actor.stamina = 0;

    bool actor_is_p  = (&actor  >= &state->players[0] && &actor  <= &state->players[MAX_PLAYERS-1]);
    bool target_is_p = (&target >= &state->players[0] && &target <= &state->players[MAX_PLAYERS-1]);
    
    bool stunned = false;
    if (weapon == WEAPON_THUNDERSTAFF) {
        stunned = true;
        target.is_stunned = true;
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        target.stun_end.tv_sec = now.tv_sec + STUN_DURATION_SEC;
        target.stun_end.tv_nsec = now.tv_nsec;

        pid_t target_pid;
        if (target_is_p) {
            if (state->game_mode == MODE_TWO_PLAYER) {
                target_pid = (target.id == 1) ? state->hip_pid[1] : state->hip_pid[0];
            } else {
                target_pid = state->hip_pid[0];
            }
        } else {
            target_pid = state->asp_pid;
        }
        send_stun_signal(target_pid);
    }
    
    if (stunned) {
        log_action(&state->action_log, state->game_tick,
                   "%s %d uses %s on %s %d for %d damage and STUNS them!",
                   actor_is_p  ? "Player" : "Enemy", actor.id,
                   weapon_name(weapon),
                   target_is_p ? "Player" : "Enemy", target.id,
                   dmg);
    } else {
        log_action(&state->action_log, state->game_tick,
                   "%s %d uses %s on %s %d for %d damage!",
                   actor_is_p  ? "Player" : "Enemy", actor.id,
                   weapon_name(weapon),
                   target_is_p ? "Player" : "Enemy", target.id,
                   dmg);
    }
}

void execute_swap_in(GameState* state, Character& actor, WeaponType weapon) {
    int lts_idx = -1;
    for (int i = 0; i < state->shared_lts.count; i++) {
        if (state->shared_lts.weapons[i] == weapon) {
            lts_idx = i;
            break;
        }
    }

    bool actor_is_p = (&actor >= &state->players[0] && &actor <= &state->players[MAX_PLAYERS-1]);
    const char* label = actor_is_p ? "Player" : "Enemy";

    if (lts_idx != -1) {
        int size = weapon_slot_size(weapon);
        int slot = find_contiguous_free(actor.inventory, size);

        while (slot == -1) {
            int best_slot = -1;
            int best_size = INVENTORY_SLOTS + 1;
            for (int i = 0; i < INVENTORY_SLOTS; i++) {
                if (actor.inventory[i].weapon != WEAPON_NONE) {
                    if (i == 0 || actor.inventory[i-1].weapon != actor.inventory[i].weapon) {
                        int sz = weapon_slot_size(actor.inventory[i].weapon);
                        if (sz < best_size) { best_size = sz; best_slot = i; }
                    }
                }
            }
            if (best_slot == -1) break;

            WeaponType evict_w = actor.inventory[best_slot].weapon;
            if (shared_deposit(&state->shared_lts, evict_w, actor.id)) {
                remove_weapon_at(actor.inventory, best_slot);
                
                if (evict_w == WEAPON_SOLAR_CORE) {
                    release_solar_core(&state->artifacts);
                    actor.holds_solar_core = false;
                } else if (evict_w == WEAPON_LUNAR_BLADE) {
                    release_lunar_blade(&state->artifacts);
                    actor.holds_lunar_blade = false;
                } else if (evict_w == WEAPON_ECLIPSE_RELIC) {
                    release_eclipse_relic(&state->artifacts);
                    actor.holds_eclipse_relic = false;
                }
                
                log_action(&state->action_log, state->game_tick, "%s %d automatically moved %s to storage to make room.", label, actor.id, weapon_name(evict_w));
            } else {
                break;
            }
            slot = find_contiguous_free(actor.inventory, size);
        }

        if (slot != -1) {
            bool can_acquire = true;
            if (weapon == WEAPON_SOLAR_CORE) {
                if (!acquire_solar_core(&state->artifacts, actor.id, actor_is_p)) {
                    can_acquire = false;
                    actor.waiting_for_solar_core = true;
                    log_action(&state->action_log, state->game_tick, "%s %d tried to swap Solar Core but it's locked! Waiting...", label, actor.id);
                } else {
                    actor.holds_solar_core = true;
                }
            } else if (weapon == WEAPON_LUNAR_BLADE) {
                if (!acquire_lunar_blade(&state->artifacts, actor.id, actor_is_p)) {
                    can_acquire = false;
                    actor.waiting_for_lunar_blade = true;
                    log_action(&state->action_log, state->game_tick, "%s %d tried to swap Lunar Blade but it's locked! Waiting...", label, actor.id);
                } else {
                    actor.holds_lunar_blade = true;
                }
            } else if (weapon == WEAPON_ECLIPSE_RELIC) {
                if (!acquire_eclipse_relic(&state->artifacts, actor.id, actor_is_p)) {
                    can_acquire = false;
                    actor.waiting_for_eclipse_relic = true;
                    log_action(&state->action_log, state->game_tick, "%s %d tried to swap Eclipse Relic but it's locked! Waiting...", label, actor.id);
                } else {
                    actor.holds_eclipse_relic = true;
                }
            }
            
            if (can_acquire) {
                place_weapon_at(actor.inventory, weapon, slot);
                shared_withdraw(&state->shared_lts, lts_idx, nullptr);
                actor.waiting_for_solar_core = false;
                actor.waiting_for_lunar_blade = false;
                actor.waiting_for_eclipse_relic = false;
                log_action(&state->action_log, state->game_tick, "%s %d swaps in %s from team storage.", label, actor.id, weapon_name(weapon));
            }
        } else {
            log_action(&state->action_log, state->game_tick, "%s %d could not fit %s (storage full).", label, actor.id, weapon_name(weapon));
        }
    } else {
        log_action(&state->action_log, state->game_tick, "%s %d failed to swap in %s (not found).", label, actor.id, weapon_name(weapon));
    }
    actor.stamina = 0;
}

void execute_put_storage(GameState* state, Character& actor, int weapon_slot) {
    WeaponType weapon = actor.inventory[weapon_slot].weapon;
    bool actor_is_p = (&actor >= &state->players[0] && &actor <= &state->players[MAX_PLAYERS-1]);
    const char* label = actor_is_p ? "Player" : "Enemy";

    if (weapon == WEAPON_NONE) {
        log_action(&state->action_log, state->game_tick, "%s %d tried to store empty slot!", label, actor.id);
        actor.stamina = 0;
        return;
    }

    if (shared_deposit(&state->shared_lts, weapon, actor.id)) {
        remove_weapon_at(actor.inventory, weapon_slot);
        
        if (weapon == WEAPON_SOLAR_CORE) {
            release_solar_core(&state->artifacts);
            actor.holds_solar_core = false;
        } else if (weapon == WEAPON_LUNAR_BLADE) {
            release_lunar_blade(&state->artifacts);
            actor.holds_lunar_blade = false;
        } else if (weapon == WEAPON_ECLIPSE_RELIC) {
            release_eclipse_relic(&state->artifacts);
            actor.holds_eclipse_relic = false;
        }
        
        log_action(&state->action_log, state->game_tick, "%s %d put %s into shared team storage.", label, actor.id, weapon_name(weapon));
    } else {
        log_action(&state->action_log, state->game_tick, "Team storage is full! %s %d kept %s.", label, actor.id, weapon_name(weapon));
    }
    actor.stamina = 0;
}

void execute_heal(GameState* state, Character& actor) {
    int heal_amount = actor.max_hp * HEAL_PERCENT / 100;
    actor.hp += heal_amount;
    if (actor.hp > actor.max_hp) actor.hp = actor.max_hp;
    actor.stamina = 0;
    bool actor_is_p = (&actor >= &state->players[0] && &actor <= &state->players[MAX_PLAYERS-1]);
    log_action(&state->action_log, state->game_tick,
               "%s %d heals for %d HP! (HP: %d/%d)",
               actor_is_p ? "Player" : "Enemy",
               actor.id, heal_amount, actor.hp, actor.max_hp);
}

void execute_skip(GameState* state, Character& actor) {
    actor.stamina = actor.max_stamina * SKIP_STAMINA_PERCENT / 100;
    bool actor_is_p = (&actor >= &state->players[0] && &actor <= &state->players[MAX_PLAYERS-1]);
    log_action(&state->action_log, state->game_tick,
               "%s %d skips their turn. (Stamina: %d%%)",
               actor_is_p ? "Player" : "Enemy",
               actor.id, SKIP_STAMINA_PERCENT);
}

void execute_ultimate(GameState* state, Character& actor) {
    bool actor_is_p = (&actor >= &state->players[0] && &actor <= &state->players[MAX_PLAYERS-1]);
    const char* label = actor_is_p ? "Player" : "Enemy";

    if (!actor.holds_solar_core) {
        if (!acquire_solar_core(&state->artifacts, actor.id, actor_is_p)) {
            actor.waiting_for_solar_core = true;
            actor.stamina = 0;
            log_action(&state->action_log, state->game_tick, "%s %d channeling Ultimate but Solar Core is locked! Waiting...", label, actor.id);
            return;
        } else {
            actor.waiting_for_solar_core = false;
            actor.holds_solar_core = true;
            
            int slot = find_contiguous_free(actor.inventory, 10);
            while (slot == -1) {
                int best_slot = -1; int best_size = 100;
                for(int i=0; i<INVENTORY_SLOTS; i++) {
                    if(actor.inventory[i].weapon != WEAPON_NONE && (i==0 || actor.inventory[i-1].weapon != actor.inventory[i].weapon)) {
                        int sz = weapon_slot_size(actor.inventory[i].weapon);
                        if(sz < best_size) { best_size = sz; best_slot = i; }
                    }
                }
                if(best_slot == -1) break;
                WeaponType evict_w = actor.inventory[best_slot].weapon;
                shared_deposit(&state->shared_lts, evict_w, actor.id);
                remove_weapon_at(actor.inventory, best_slot);
                slot = find_contiguous_free(actor.inventory, 10);
            }
            if(slot != -1) {
                place_weapon_at(actor.inventory, WEAPON_SOLAR_CORE, slot);
                for(int i=0; i<state->shared_lts.count; i++) {
                    if(state->shared_lts.weapons[i] == WEAPON_SOLAR_CORE) {
                        shared_withdraw(&state->shared_lts, i, nullptr);
                        break;
                    }
                }
            }
            actor.stamina = 0;
            log_action(&state->action_log, state->game_tick, "%s %d locked Solar Core for Ultimate.", label, actor.id);
            return;
        }
    }

    if (!actor.holds_lunar_blade) {
        if (!acquire_lunar_blade(&state->artifacts, actor.id, actor_is_p)) {
            actor.waiting_for_lunar_blade = true;
            actor.stamina = 0;
            log_action(&state->action_log, state->game_tick, "%s %d channeling Ultimate but Lunar Blade is locked! Waiting...", label, actor.id);
            return;
        } else {
            actor.waiting_for_lunar_blade = false;
            actor.holds_lunar_blade = true;
            
            int slot = find_contiguous_free(actor.inventory, 10);
            while (slot == -1) {
                int best_slot = -1; int best_size = 100;
                for(int i=0; i<INVENTORY_SLOTS; i++) {
                    if(actor.inventory[i].weapon != WEAPON_NONE && (i==0 || actor.inventory[i-1].weapon != actor.inventory[i].weapon)) {
                        int sz = weapon_slot_size(actor.inventory[i].weapon);
                        if(sz < best_size && actor.inventory[i].weapon != WEAPON_SOLAR_CORE) { best_size = sz; best_slot = i; }
                    }
                }
                if(best_slot == -1) break;
                WeaponType evict_w = actor.inventory[best_slot].weapon;
                shared_deposit(&state->shared_lts, evict_w, actor.id);
                remove_weapon_at(actor.inventory, best_slot);
                slot = find_contiguous_free(actor.inventory, 10);
            }
            if(slot != -1) {
                place_weapon_at(actor.inventory, WEAPON_LUNAR_BLADE, slot);
                for(int i=0; i<state->shared_lts.count; i++) {
                    if(state->shared_lts.weapons[i] == WEAPON_LUNAR_BLADE) {
                        shared_withdraw(&state->shared_lts, i, nullptr);
                        break;
                    }
                }
            }
            actor.stamina = 0;
            log_action(&state->action_log, state->game_tick, "%s %d locked Lunar Blade for Ultimate. Both locked!", label, actor.id);
            return;
        }
    }

    suspend_asp_process(state->asp_pid);
    state->asp_suspended = true;
    
    log_action(&state->action_log, state->game_tick,
               "Player %d activates ULTIMATE ABILITY! ASP suspended for %d seconds!",
               actor.id, ULTIMATE_DURATION_SEC);
    
    for (int i = 0; i < state->num_enemies; i++) {
        if (state->enemies[i].is_active) {
            state->enemies[i].hp -= 80;
            if (state->enemies[i].hp < 0) state->enemies[i].hp = 0;
            
            log_action(&state->action_log, state->game_tick,
                       "Enemy %d takes 80 damage from Ultimate!", state->enemies[i].id);
            
            check_entity_death(state, state->enemies[i], false);
        }
    }
    
    setup_ultimate_timer();
    
    send_stun_signal(state->asp_pid);
    
    release_solar_core(&state->artifacts);
    actor.holds_solar_core = false;
    release_lunar_blade(&state->artifacts);
    actor.holds_lunar_blade = false;
    
    for(int i=0; i<INVENTORY_SLOTS; i++) {
        if(actor.inventory[i].weapon == WEAPON_SOLAR_CORE || actor.inventory[i].weapon == WEAPON_LUNAR_BLADE) {
            WeaponType w = actor.inventory[i].weapon;
            remove_weapon_at(actor.inventory, i);
            shared_deposit(&state->shared_lts, w, -1);
        }
    }
    
    actor.stamina = 0;
}

void check_entity_death(GameState* state, Character& target, bool target_is_player) {
    if (target.hp > 0) return;
    
    target.is_active = false;
    
    if (!target_is_player) {
        state->enemies_killed++;
        log_action(&state->action_log, state->game_tick,
                   "Enemy %d has been defeated! (Total kills: %d/%d)",
                   target.id, state->enemies_killed, WIN_KILL_COUNT);
        
        handle_weapon_drop(state, target);
        
        if (!state->artifacts.eclipse_relic_exists) {
            bool drop_eclipse = false;
            if (state->enemies_killed >= 3) {
                drop_eclipse = true;
            } else if (rand() % 10 < 3) {
                drop_eclipse = true;
            }
            if (drop_eclipse) {
                state->artifacts.eclipse_relic_exists = true;
                log_action(&state->action_log, state->game_tick, "The enemy's death has revealed the Eclipse Relic!");
                if (shared_deposit(&state->shared_lts, WEAPON_ECLIPSE_RELIC, -1)) {
                    log_action(&state->action_log, state->game_tick, "The Eclipse Relic was placed in the team's shared storage.");
                } else {
                    log_action(&state->action_log, state->game_tick, "The team storage is full! The Eclipse Relic was lost.");
                }
            }
        }
    } else {
        log_action(&state->action_log, state->game_tick,
                   "Player %d has fallen!", target.id);
    }
}

void handle_weapon_drop(GameState* state, Character& defeated_enemy) {
    if (rand() % 2 == 0) {
        int non_artifact_count = WEAPON_SPLINTER_STICK - WEAPON_IRON_HALBERD + 1;
        WeaponType dropped = (WeaponType)(WEAPON_IRON_HALBERD + (rand() % non_artifact_count));
        
        log_action(&state->action_log, state->game_tick,
                   "Enemy %d dropped a %s!",
                   defeated_enemy.id, weapon_name(dropped));
                   
        if (shared_deposit(&state->shared_lts, dropped, -1)) {
            log_action(&state->action_log, state->game_tick,
                       "The %s was placed in the team's shared storage.", weapon_name(dropped));
        } else {
            log_action(&state->action_log, state->game_tick,
                       "The team storage is full! The %s was lost.", weapon_name(dropped));
        }
    }
}

void check_win_lose_conditions(GameState* state) {
    if (state->enemies_killed >= WIN_KILL_COUNT) {
        state->game_over = true;
        state->player_won = true;
        log_action(&state->action_log, state->game_tick,
                   "VICTORY! All enemies have been vanquished!");
        return;
    }
    
    bool any_enemy_alive = false;
    for (int i = 0; i < state->num_enemies; i++) {
        if (state->enemies[i].is_active) {
            any_enemy_alive = true;
            break;
        }
    }
    
    if (!any_enemy_alive && state->enemies_killed < WIN_KILL_COUNT) {
        int needed = WIN_KILL_COUNT - state->total_enemies_spawned;
        if (needed > 0) {
            int spawn_count = (needed > state->num_enemies) ? state->num_enemies : needed;
            unsigned int seed = (unsigned int)state->game_tick;
            int spawned = 0;
            for (int j = 0; j < state->num_enemies && spawned < spawn_count; j++) {
                if (!state->enemies[j].is_active) {
                    respawn_enemy(state, j, &seed);
                    spawned++;
                }
            }
        }
    }

    bool any_player_alive = false;
    for (int i = 0; i < state->num_players; i++) {
        if (state->players[i].is_active) {
            any_player_alive = true;
            break;
        }
    }
    
    if (!any_player_alive) {
        state->game_over = true;
        state->player_won = false;
        log_action(&state->action_log, state->game_tick,
                   "DEFEAT! All player characters have fallen...");
    }
}

