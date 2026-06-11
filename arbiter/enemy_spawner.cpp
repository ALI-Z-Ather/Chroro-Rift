#include "enemy_spawner.h"
#include <cstdlib>
#include <cstdio>

using namespace std;

int determine_enemy_count(unsigned int* seed) {
    return MIN_ENEMIES + (portable_rand(seed) % (MAX_ENEMIES - MIN_ENEMIES + 1));
}

void spawn_enemies(GameState* state, int count, unsigned int* seed) {
    state->num_enemies = count;
    
    for (int i = 0; i < count; i++) {
        init_enemy(&state->enemies[i], i, seed);
        state->total_enemies_spawned++;
    }
    
    printf("[Arbiter] Spawned %d enemies\n", count);
    for (int i = 0; i < count; i++) {
        printf("  Enemy %d: HP=%d, DMG=%d, SPD=%d\n",
               i, state->enemies[i].hp, state->enemies[i].damage, state->enemies[i].speed);
    }
}

void respawn_enemy(GameState* state, int slot, unsigned int* seed) {
    if (state->total_enemies_spawned >= WIN_KILL_COUNT + MAX_ENEMIES) {
        return;
    }
    
    init_enemy(&state->enemies[slot], slot, seed);
    state->total_enemies_spawned++;
    
    log_action(&state->action_log, state->game_tick,
               "A new enemy %d appears! HP=%d SPD=%d",
               slot, state->enemies[slot].hp, state->enemies[slot].speed);
}

