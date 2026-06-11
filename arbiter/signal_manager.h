#pragma once

#include "../shared/game_state.h"
#include <signal.h>

using namespace std;

void setup_arbiter_signal_handlers(GameState* state);

void send_stun_signal(pid_t target_pid);

void suspend_asp_process(pid_t asp_pid);

void resume_asp_process(pid_t asp_pid);

void setup_ultimate_timer();

