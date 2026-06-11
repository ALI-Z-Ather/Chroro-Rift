#pragma once

#include "game_config.h"

using namespace std;

enum WeaponType {
    WEAPON_NONE = -1,
    WEAPON_SOLAR_CORE = 0,
    WEAPON_LUNAR_BLADE,
    WEAPON_IRON_HALBERD,
    WEAPON_VENOM_DAGGER,
    WEAPON_THUNDERSTAFF,
    WEAPON_OBSIDIAN_AXE,
    WEAPON_FROSTBOW,
    WEAPON_SPLINTER_STICK,
    WEAPON_ECLIPSE_RELIC,
    WEAPON_COUNT
};

struct WeaponInfo {
    const char* name;
    int slot_size;
    int damage;
    bool is_artifact;
};

inline const WeaponInfo& get_weapon_info(WeaponType type) {
    static const WeaponInfo table[WEAPON_COUNT] = {
        {"Solar Core",     10, 95, true },
        {"Lunar Blade",    10, 90, true },
        {"Iron Halberd",    7, 55, false},
        {"Venom Dagger",    4, 30, false},
        {"Thunderstaff",    6, 50, false},
        {"Obsidian Axe",    5, 45, false},
        {"Frostbow",        6, 48, false},
        {"Splinter Stick",  2, 12, false},
        {"Eclipse Relic",  10, 100, true},
    };
    return table[type];
}

inline const char* weapon_name(WeaponType type) {
    if (type == WEAPON_NONE) return "Empty";
    return get_weapon_info(type).name;
}

inline int weapon_slot_size(WeaponType type) {
    if (type == WEAPON_NONE) return 0;
    return get_weapon_info(type).slot_size;
}

inline int weapon_damage(WeaponType type) {
    if (type == WEAPON_NONE) return 0;
    return get_weapon_info(type).damage;
}

