#include "signal_manager.h"
#include "../shared/sync.h"
#include <cstdio>
#include <unistd.h>

using namespace std;

static GameState* s_signal_state = nullptr;

static void sigterm_handler(int sig) {
    (void)sig;
    if (s_signal_state) {
        s_signal_state->game_over = true;
    }
}

static void sigalrm_handler(int sig) {
    (void)sig;
    if (s_signal_state && s_signal_state->asp_pid > 0) {
        resume_asp_process(s_signal_state->asp_pid);
        s_signal_state->asp_suspended = false;
        
        log_action(&s_signal_state->action_log, s_signal_state->game_tick,
                   "Ultimate Ability window expired. ASP resumed.");
    }
}

void setup_arbiter_signal_handlers(GameState* state) {
    s_signal_state = state;
    
    struct sigaction sa;
    
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    
    sa.sa_handler = sigalrm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, nullptr);
    
    signal(SIGPIPE, SIG_IGN);
}

void send_stun_signal(pid_t target_pid) {
    if (target_pid > 0) {
        kill(target_pid, SIGUSR1);
    }
}

void suspend_asp_process(pid_t asp_pid) {
    if (asp_pid > 0) {
        kill(asp_pid, SIGSTOP);
    }
}

void resume_asp_process(pid_t asp_pid) {
    if (asp_pid > 0) {
        kill(asp_pid, SIGCONT);
    }
}

void setup_ultimate_timer() {
    alarm(ULTIMATE_DURATION_SEC);
}

