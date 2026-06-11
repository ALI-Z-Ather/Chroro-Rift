#pragma once

#include "game_config.h"

using namespace std;

struct ArtifactTable {
    int solar_core_holder;
    bool solar_core_held_by_player;

    int lunar_blade_holder;
    bool lunar_blade_held_by_player;

    bool eclipse_relic_exists;
    int eclipse_relic_holder;
    bool eclipse_relic_held_by_player;
};

inline void init_artifact_table(ArtifactTable* table) {
    table->solar_core_holder = -1;
    table->solar_core_held_by_player = false;

    table->lunar_blade_holder = -1;
    table->lunar_blade_held_by_player = false;

    table->eclipse_relic_exists = false;
    table->eclipse_relic_holder = -1;
    table->eclipse_relic_held_by_player = false;
}

inline bool is_solar_core_free(const ArtifactTable* t) {
    return t->solar_core_holder == -1;
}

inline bool is_lunar_blade_free(const ArtifactTable* t) {
    return t->lunar_blade_holder == -1;
}

inline bool is_eclipse_relic_free(const ArtifactTable* t) {
    return t->eclipse_relic_exists && t->eclipse_relic_holder == -1;
}

inline bool acquire_solar_core(ArtifactTable* t, int char_id, bool is_player) {
    if (t->solar_core_holder != -1) return false;
    t->solar_core_holder = char_id;
    t->solar_core_held_by_player = is_player;
    return true;
}

inline void release_solar_core(ArtifactTable* t) {
    t->solar_core_holder = -1;
    t->solar_core_held_by_player = false;
}

inline bool acquire_lunar_blade(ArtifactTable* t, int char_id, bool is_player) {
    if (t->lunar_blade_holder != -1) return false;
    t->lunar_blade_holder = char_id;
    t->lunar_blade_held_by_player = is_player;
    return true;
}

inline void release_lunar_blade(ArtifactTable* t) {
    t->lunar_blade_holder = -1;
    t->lunar_blade_held_by_player = false;
}

inline bool acquire_eclipse_relic(ArtifactTable* t, int char_id, bool is_player) {
    if (!t->eclipse_relic_exists || t->eclipse_relic_holder != -1) return false;
    t->eclipse_relic_holder = char_id;
    t->eclipse_relic_held_by_player = is_player;
    return true;
}

inline void release_eclipse_relic(ArtifactTable* t) {
    t->eclipse_relic_holder = -1;
    t->eclipse_relic_held_by_player = false;
}

inline bool can_use_ultimate(const ArtifactTable* t, int char_id, bool is_player) {
    return (t->solar_core_holder == char_id && t->solar_core_held_by_player == is_player) &&
           (t->lunar_blade_holder == char_id && t->lunar_blade_held_by_player == is_player);
}

