#pragma once

enum GameScreen {
    SCREEN_MAIN_MENU = 0,
    SCREEN_MODE_SELECT,
    SCREEN_SETTINGS,
    SCREEN_CITY,
    SCREEN_BATTLE_INTRO,
    SCREEN_BATTLE,
    SCREEN_BOSS_INTRO,
    SCREEN_BOSS_BATTLE,
    SCREEN_VICTORY,
    SCREEN_DEFEAT,
    SCREEN_PAUSE,
    SCREEN_COUNT
};

enum GameMode {
    MODE_NONE = 0,
    MODE_SINGLE_PLAYER,
    MODE_TWO_PLAYER
};

enum BattleInputPhase {
    BATTLE_INPUT_NONE = 0,
    BATTLE_INPUT_ACTION,
    BATTLE_INPUT_TARGET,
    BATTLE_INPUT_WEAPON,
    BATTLE_INPUT_LTS,
    BATTLE_INPUT_PUT_STORAGE,
    BATTLE_INPUT_CONFIRM
};

struct BattleInputState {
    BattleInputPhase phase;
    int action_cursor;
    int target_cursor;
    int weapon_cursor;
    int lts_cursor;

    int chosen_action;
    int chosen_weapon_slot;
    int chosen_lts_index;
    int chosen_target_id;

    WeaponType inv_weapons[20];
    int inv_starts[20];
    int inv_count;

    WeaponType lts_weapons[64];
    int lts_count;

    int active_enemy_ids[10];
    int active_enemy_count;

    bool can_use_ultimate;
};

inline void init_battle_input(BattleInputState* b) {
    b->phase = BATTLE_INPUT_NONE;
    b->action_cursor = 0;
    b->target_cursor = 0;
    b->weapon_cursor = 0;
    b->lts_cursor = 0;
    b->chosen_action = -1;
    b->chosen_weapon_slot = -1;
    b->chosen_lts_index = -1;
    b->chosen_target_id = -1;
    b->inv_count = 0;
    b->lts_count = 0;
    b->active_enemy_count = 0;
    b->can_use_ultimate = false;
}

inline const char* screen_name(GameScreen s) {
    static const char* names[] = {
        "Main Menu", "Mode Select", "Settings", "City",
        "Battle Intro", "Battle", "Boss Intro", "Boss Battle",
        "Victory", "Defeat", "Pause"
    };
    return (s < SCREEN_COUNT) ? names[s] : "Unknown";
}

struct GameSettings {
    float master_volume;
    float music_volume;
    float sfx_volume;
    bool muted;
};

inline void init_game_settings(GameSettings* s) {
    s->master_volume = 0.8f;
    s->music_volume = 0.7f;
    s->sfx_volume = 0.9f;
    s->muted = false;
}

struct MenuState {
    GameScreen current_screen;
    GameScreen previous_screen;
    GameMode selected_mode;

    int menu_cursor;
    bool continue_available;

    int mode_cursor;

    int settings_cursor;

    int city_cursor;
    bool city_char_selected[4];
    int city_party_count;

    BattleInputState battle_input;

    bool transitioning;
    float transition_alpha;
    GameScreen transition_target;
    bool transition_fading_out;

    float anim_timer;
};

inline void init_menu_state(MenuState* m) {
    m->current_screen = SCREEN_MAIN_MENU;
    m->previous_screen = SCREEN_MAIN_MENU;
    m->selected_mode = MODE_NONE;
    m->menu_cursor = 0;
    m->continue_available = false;
    m->mode_cursor = 0;
    m->settings_cursor = 0;
    m->city_cursor = 0;
    for (int i = 0; i < 4; i++) m->city_char_selected[i] = false;
    m->city_party_count = 0;
    init_battle_input(&m->battle_input);
    m->transitioning = false;
    m->transition_alpha = 0.0f;
    m->transition_target = SCREEN_MAIN_MENU;
    m->transition_fading_out = true;
    m->anim_timer = 0.0f;
}
