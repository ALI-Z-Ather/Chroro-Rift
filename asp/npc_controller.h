#pragma once

// ============================================================================
// npc_controller.h — Per-NPC Thread Logic
// Each NPC runs in its own dedicated thread within the ASP process.
// ============================================================================

#include "../shared/game_state.h"

// NPC thread function (passed to pthread_create).
// arg = pointer to int (npc_id).
void* npc_thread(void* arg);
