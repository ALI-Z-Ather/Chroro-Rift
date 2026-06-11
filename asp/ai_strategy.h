#pragma once

// ============================================================================
// ai_strategy.h — NPC Decision-Making Logic
// Implements the automated strategic behavior for enemy NPCs.
// ============================================================================

#include "../shared/game_state.h"

// Decide what action an NPC should take on its turn.
// Uses a probability-based strategy:
//   70% Strike the weakest player
//   20% Strike a random player
//   10% Skip
// Returns a filled ActionRequest.
ActionRequest decide_npc_action(GameState* state, int npc_id, unsigned int* seed);

// Find the player with the lowest HP (primary target).
int find_weakest_player(GameState* state);

// Find a random alive player.
int find_random_player(GameState* state, unsigned int* seed);
