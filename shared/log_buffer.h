#pragma once

#include "game_config.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>

using namespace std;

struct ActionLogEntry {
    char message[MAX_LOG_LENGTH];
    int game_tick;
};

struct ActionLog {
    ActionLogEntry entries[MAX_LOG_ENTRIES];
    int head;
    int count;
};

inline void init_action_log(ActionLog* log) {
    memset(log, 0, sizeof(ActionLog));
    log->head = 0;
    log->count = 0;
}

inline void log_action(ActionLog* log, int game_tick, const char* fmt, ...) {
    ActionLogEntry* entry = &log->entries[log->head];
    entry->game_tick = game_tick;
    
    va_list args;
    va_start(args, fmt);
    vsnprintf(entry->message, MAX_LOG_LENGTH, fmt, args);
    va_end(args);
    
    log->head = (log->head + 1) % MAX_LOG_ENTRIES;
    if (log->count < MAX_LOG_ENTRIES) {
        log->count++;
    }
}

inline int get_recent_logs(const ActionLog* log, ActionLogEntry* out, int max_entries) {
    int num = (log->count < max_entries) ? log->count : max_entries;
    int start = (log->head - num + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
    
    for (int i = 0; i < num; i++) {
        out[i] = log->entries[(start + i) % MAX_LOG_ENTRIES];
    }
    return num;
}

