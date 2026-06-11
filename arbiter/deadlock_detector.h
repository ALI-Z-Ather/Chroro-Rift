#pragma once

#include "../shared/game_state.h"

using namespace std;

void* deadlock_detector_thread(void* arg);

bool detect_and_resolve_deadlock(GameState* state);

