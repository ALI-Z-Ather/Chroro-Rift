#pragma once

#include "game_config.h"
#include "weapons.h"
#include "actions.h"
#include "artifacts.h"
#include "inventory.h"
#include "log_buffer.h"
#include <semaphore.h>
#include <sys/types.h>
#include <time.h>
#include <cstdlib>

using namespace std;

#ifdef __APPLE__
inline int portable_rand(unsigned int* seed) {
    *seed = (*seed) * 1103515245 + 12345;
    return (int)((*seed / 65536) % 32768);
}
#else
inline int portable_rand(unsigned int* seed) {
    return rand_r(seed);
}
#endif

struct Character {
    int id;
    bool is_active;
    bool is_stunned;
    struct timespec stun_end;

    int hp;
    int max_hp;
    int damage;
    int speed;
    int stamina;
    int max_stamina;

    pid_t process_pid;

    InventorySlot inventory[INVENTORY_SLOTS];
    LongTermStorage lts;

    bool holds_solar_core;
    bool holds_lunar_blade;
    bool holds_eclipse_relic;

    bool waiting_for_solar_core;
    bool waiting_for_lunar_blade;
    bool waiting_for_eclipse_relic;

    WeaponType held_weapon;
};

struct TurnControl {
    int active_id;
    bool active_is_player;
    bool turn_pending;
    bool turn_claimed;
};

struct SharedStorage {
    WeaponType weapons[MAX_LTS_SIZE * MAX_PLAYERS];
    int        deposited_by[MAX_LTS_SIZE * MAX_PLAYERS];
    int        count;
};

inline void init_shared_storage(SharedStorage* s) {
    s->count = 0;
    for (int i = 0; i < MAX_LTS_SIZE * MAX_PLAYERS; i++) {
        s->weapons[i]      = WEAPON_NONE;
        s->deposited_by[i] = -1;
    }
}

inline bool shared_deposit(SharedStorage* s, WeaponType w, int player_id) {
    int cap = MAX_LTS_SIZE * MAX_PLAYERS;
    if (s->count >= cap || w == WEAPON_NONE) return false;
    s->weapons[s->count]      = w;
    s->deposited_by[s->count] = player_id;
    s->count++;
    return true;
}

inline bool shared_withdraw(SharedStorage* s, int idx, WeaponType* out) {
    if (idx < 0 || idx >= s->count) return false;
    if (out) *out = s->weapons[idx];
    for (int i = idx; i < s->count - 1; i++) {
        s->weapons[i]      = s->weapons[i + 1];
        s->deposited_by[i] = s->deposited_by[i + 1];
    }
    s->count--;
    return true;
}

struct GameState {
#ifndef __APPLE__
    sem_t mutex;
    sem_t turn_notify;
    sem_t action_ready;
    sem_t mmap_sem;
    sem_t artifact_mutex;
#endif

    Character players[MAX_PLAYERS];
    Character enemies[MAX_ENEMIES];
    int num_players;
    int num_enemies;

    SharedStorage shared_lts;

    int game_mode;
    bool battle_started;
    int current_floor;
    bool boss_active;

    int enemies_killed;
    int total_enemies_spawned;
    bool game_over;
    bool player_won;
    int game_tick;

    TurnControl turn;
    ActionRequest current_action;

    ArtifactTable artifacts;

    pid_t arbiter_pid;
    pid_t hip_pid[2];
    pid_t asp_pid;

    bool asp_suspended;

    ActionLog action_log;

    bool initialized;
    int hips_connected;
    bool asp_ready;
};

inline void init_game_state(GameState* state) {
    memset(state, 0, sizeof(GameState));

#ifndef __APPLE__
    sem_init(&state->mutex, 1, 1);
    sem_init(&state->turn_notify, 1, 0);
    sem_init(&state->action_ready, 1, 0);
    sem_init(&state->artifact_mutex, 1, 1);
#endif

    state->num_players = 0;
    state->num_enemies = 0;

    state->game_mode = 0;
    state->battle_started = false;
    state->current_floor = 1;
    state->boss_active = false;

    state->enemies_killed = 0;
    state->total_enemies_spawned = 0;

    state->game_over = false;
    state->player_won = false;
    state->game_tick = 0;

    state->turn.active_id = -1;
    state->turn.active_is_player = false;
    state->turn.turn_pending = false;
    state->turn.turn_claimed = false;

    clear_action_request(&state->current_action);

    init_artifact_table(&state->artifacts);

    init_shared_storage(&state->shared_lts);

    state->arbiter_pid = 0;
    state->hip_pid[0] = 0;
    state->hip_pid[1] = 0;
    state->asp_pid = 0;
    state->asp_suspended = false;

    init_action_log(&state->action_log);

    state->initialized = false;
    state->hips_connected = 0;
    state->asp_ready = false;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        state->players[i].id = i;
        state->players[i].is_active = false;
        state->players[i].held_weapon = WEAPON_NONE;
        init_inventory(state->players[i].inventory);
        init_lts(&state->players[i].lts);
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {
        state->enemies[i].id = i;
        state->enemies[i].is_active = false;
        state->enemies[i].held_weapon = WEAPON_NONE;
    }
}

inline void init_player(Character* player, int id, int num_players, unsigned int* seed) {
    player->id = id;
    player->is_active = true;
    player->is_stunned = false;

    if (id == 1 || id == 3) {
        player->hp = 661;
    } else {
        player->hp = PLAYER_BASE_HP + (portable_rand(seed) % (PLAYER_HP_RAND_MAX - PLAYER_HP_RAND_MIN + 1)) + PLAYER_HP_RAND_MIN;
    }
    player->max_hp = player->hp;

    player->damage = PLAYER_DAMAGE;
    player->speed = PLAYER_SPEED_BASE / num_players;
    player->stamina = 0;
    player->max_stamina = PLAYER_MAX_STAMINA;

    player->holds_solar_core = false;
    player->holds_lunar_blade = false;
    player->holds_eclipse_relic = false;
    player->waiting_for_solar_core = false;
    player->waiting_for_lunar_blade = false;
    player->waiting_for_eclipse_relic = false;
    player->held_weapon = WEAPON_NONE;

    init_inventory(player->inventory);
    init_lts(&player->lts);

    place_weapon(player->inventory, &player->lts, WEAPON_SPLINTER_STICK);
    place_weapon(player->inventory, &player->lts, WEAPON_VENOM_DAGGER);
    place_weapon(player->inventory, &player->lts, WEAPON_FROSTBOW);
}

inline void init_enemy(Character* enemy, int id, unsigned int* seed) {
    enemy->id = id;
    enemy->is_active = true;
    enemy->is_stunned = false;

    enemy->hp = ENEMY_BASE_HP + (portable_rand(seed) % (ENEMY_HP_RAND_MAX - ENEMY_HP_RAND_MIN + 1)) + ENEMY_HP_RAND_MIN;
    enemy->max_hp = enemy->hp;

    enemy->damage = ENEMY_DAMAGE;
    enemy->speed = ENEMY_SPEED_MIN + (portable_rand(seed) % (ENEMY_SPEED_MAX - ENEMY_SPEED_MIN + 1));
    enemy->stamina = 0;
    enemy->max_stamina = ENEMY_MAX_STAMINA;

    enemy->holds_solar_core = false;
    enemy->holds_lunar_blade = false;
    enemy->holds_eclipse_relic = false;
    enemy->waiting_for_solar_core = false;
    enemy->waiting_for_lunar_blade = false;
    enemy->waiting_for_eclipse_relic = false;
    enemy->held_weapon = WEAPON_NONE;
    
    init_inventory(enemy->inventory);
    init_lts(&enemy->lts);

    int non_artifact_count = WEAPON_SPLINTER_STICK - WEAPON_IRON_HALBERD + 1;
    WeaponType random_w = (WeaponType)(WEAPON_IRON_HALBERD + (portable_rand(seed) % non_artifact_count));
    place_weapon(enemy->inventory, &enemy->lts, random_w);
}

