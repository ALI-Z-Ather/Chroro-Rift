#pragma once

#include "../shared/game_state.h"

using namespace std;

void tick_stamina(GameState* state, float elapsed_fraction);

bool find_next_actor(GameState* state, int& out_id, bool& out_is_player);

void dispatch_turn(GameState* state, int actor_id, bool is_player);

bool wait_for_action(GameState* state, bool is_player_turn);

void* scheduler_thread(void* arg);

