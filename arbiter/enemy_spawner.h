#pragma once

#include "../shared/game_state.h"

using namespace std;

int determine_enemy_count(unsigned int* seed);

void spawn_enemies(GameState* state, int count, unsigned int* seed);

void respawn_enemy(GameState* state, int slot, unsigned int* seed);

