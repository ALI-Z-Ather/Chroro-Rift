#pragma once

#include "../shared/game_state.h"

using namespace std;

ActionRequest get_player_action(GameState* state, int player_id);

int select_target(GameState* state, bool select_enemy);

int select_weapon_from_inventory(GameState* state, int player_id);

WeaponType select_weapon_from_lts(GameState* state, int player_id);

