#pragma once

#include "game_config.h"
#include "weapons.h"

using namespace std;

enum ActionType {
    ACTION_NONE = 0,
    ACTION_STRIKE,
    ACTION_EXHAUST,
    ACTION_USE_WEAPON,
    ACTION_SWAP_IN,
    ACTION_PUT_STORAGE,
    ACTION_HEAL,
    ACTION_SKIP,
    ACTION_ULTIMATE
};

inline const char* action_name(ActionType type) {
    static const char* names[] = {
        "None",
        "Strike",
        "Exhaust",
        "Use Weapon",
        "Swap In",
        "Put to Storage",
        "Heal",
        "Skip",
        "Ultimate"
    };
    return (type <= ACTION_ULTIMATE) ? names[type] : "Unknown";
}

struct ActionRequest {
    ActionType type;
    int actor_id;
    bool actor_is_player;
    int target_id;
    bool target_is_player;
    int weapon_slot;
    WeaponType swap_weapon;
    bool submitted;
    bool processed;
};

inline void clear_action_request(ActionRequest* req) {
    req->type = ACTION_NONE;
    req->actor_id = -1;
    req->actor_is_player = false;
    req->target_id = -1;
    req->target_is_player = false;
    req->weapon_slot = -1;
    req->swap_weapon = WEAPON_NONE;
    req->submitted = false;
    req->processed = false;
}

