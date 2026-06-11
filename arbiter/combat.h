#pragma once

#include "../shared/game_state.h"

using namespace std;

void execute_action(GameState* state, const ActionRequest& action);

void execute_strike(GameState* state, Character& actor, Character& target);
void execute_exhaust(GameState* state, Character& actor, Character& target);
void execute_use_weapon(GameState* state, Character& actor, Character& target, int weapon_slot);
void execute_swap_in(GameState* state, Character& actor, WeaponType weapon);
void execute_put_storage(GameState* state, Character& actor, int weapon_slot);
void execute_heal(GameState* state, Character& actor);
void execute_skip(GameState* state, Character& actor);
void execute_ultimate(GameState* state, Character& actor);

void check_entity_death(GameState* state, Character& target, bool target_is_player);
void handle_weapon_drop(GameState* state, Character& defeated_enemy);
void check_win_lose_conditions(GameState* state);
