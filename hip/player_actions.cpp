#include "player_actions.h"
#include "../shared/sync.h"
#include <cstdio>
#include <iostream>
#include <string>

using namespace std;

extern GameState* g_state;

ActionRequest get_player_action(GameState* state, int player_id) {
    ActionRequest action;
    clear_action_request(&action);
    action.actor_id = player_id;
    action.actor_is_player = true;
    
    Character& player = state->players[player_id];
    
    printf("\n===== Player %d's Turn =====\n", player_id);
    printf("HP: %d/%d  |  Stamina: %d/%d  |  DMG: %d\n",
           player.hp, player.max_hp, player.stamina, player.max_stamina, player.damage);
    printf("\nActions:\n");
    printf("  1. Strike (Attack - reduce enemy HP)\n");
    printf("  2. Exhaust (Attack - reduce enemy Stamina)\n");
    printf("  3. Use Weapon\n");
    printf("  4. Swap In (from Long-Term Storage)\n");
    printf("  5. Heal (restore 10%% HP)\n");
    printf("  6. Skip Turn\n");
    
    bool can_ult = can_use_ultimate(&state->artifacts, player_id, true);
    if (can_ult) {
        printf("  7. ULTIMATE ABILITY (Solar Core + Lunar Blade)\n");
    }
    
    printf("  0. Quit Game\n");
    printf("\nChoice: ");
    
    int choice = -1;
    while (choice < 0 || choice > 7) {
        string input;
        getline(cin, input);
        try {
            choice = stoi(input);
        } catch (...) {
            choice = -1;
        }
        
        if (choice == 7 && !can_ult) {
            printf("You don't hold both artifacts! Choose again: ");
            choice = -1;
        }
        
        if (choice < 0 || choice > 7) {
            printf("Invalid choice. Try again: ");
        }
    }
    
    switch (choice) {
        case 0:
            printf("Sending quit signal...\n");
            if (state->arbiter_pid > 0) {
                kill(state->arbiter_pid, SIGTERM);
            }
            action.type = ACTION_SKIP;
            break;
            
        case 1:
            action.type = ACTION_STRIKE;
            action.target_id = select_target(state, true);
            action.target_is_player = false;
            break;
            
        case 2:
            action.type = ACTION_EXHAUST;
            action.target_id = select_target(state, true);
            action.target_is_player = false;
            break;
            
        case 3:
            action.type = ACTION_USE_WEAPON;
            action.weapon_slot = select_weapon_from_inventory(state, player_id);
            if (action.weapon_slot == -1) {
                printf("No weapon selected. Skipping turn.\n");
                action.type = ACTION_SKIP;
            } else {
                action.target_id = select_target(state, true);
                action.target_is_player = false;
            }
            break;
            
        case 4:
            action.type = ACTION_SWAP_IN;
            action.swap_weapon = select_weapon_from_lts(state, player_id);
            if (action.swap_weapon == WEAPON_NONE) {
                printf("No weapon selected. Skipping turn.\n");
                action.type = ACTION_SKIP;
            }
            break;
            
        case 5:
            action.type = ACTION_HEAL;
            break;
            
        case 6:
            action.type = ACTION_SKIP;
            break;
            
        case 7:
            action.type = ACTION_ULTIMATE;
            break;
    }
    
    return action;
}

int select_target(GameState* state, bool select_enemy) {
    printf("\nSelect target:\n");
    
    if (select_enemy) {
        for (int i = 0; i < state->num_enemies; i++) {
            if (state->enemies[i].is_active) {
                printf("  %d. Enemy %d (HP: %d/%d)\n", 
                       i, i, state->enemies[i].hp, state->enemies[i].max_hp);
            }
        }
    } else {
        for (int i = 0; i < state->num_players; i++) {
            if (state->players[i].is_active) {
                printf("  %d. Player %d (HP: %d/%d)\n",
                       i, i, state->players[i].hp, state->players[i].max_hp);
            }
        }
    }
    
    printf("Target: ");
    int target = -1;
    int max_targets = select_enemy ? state->num_enemies : state->num_players;
    
    while (target < 0 || target >= max_targets) {
        string input;
        getline(cin, input);
        try {
            target = stoi(input);
        } catch (...) {
            target = -1;
        }
        
        if (target >= 0 && target < max_targets) {
            Character& t = select_enemy ? state->enemies[target] : state->players[target];
            if (!t.is_active) {
                printf("That target is defeated. Choose another: ");
                target = -1;
            }
        } else {
            printf("Invalid target. Try again: ");
            target = -1;
        }
    }
    
    return target;
}

int select_weapon_from_inventory(GameState* state, int player_id) {
    Character& player = state->players[player_id];
    
    WeaponType weapons[INVENTORY_SLOTS];
    int starts[INVENTORY_SLOTS];
    int count = list_inventory_weapons(player.inventory, weapons, starts, INVENTORY_SLOTS);
    
    if (count == 0) {
        printf("Inventory is empty!\n");
        return -1;
    }
    
    printf("\nSelect weapon:\n");
    for (int i = 0; i < count; i++) {
        printf("  %d. %s (DMG: %d, Slot: %d)\n",
               i, weapon_name(weapons[i]), weapon_damage(weapons[i]), starts[i]);
    }
    printf("  -1. Cancel\n");
    printf("Weapon: ");
    
    int choice = -2;
    while (choice < -1 || choice >= count) {
        string input;
        getline(cin, input);
        try {
            choice = stoi(input);
        } catch (...) {
            choice = -2;
        }
        if (choice < -1 || choice >= count) {
            printf("Invalid choice. Try again: ");
        }
    }
    
    return (choice >= 0) ? starts[choice] : -1;
}

WeaponType select_weapon_from_lts(GameState* state, int player_id) {
    Character& player = state->players[player_id];
    
    if (player.lts.count == 0) {
        printf("Long-term storage is empty!\n");
        return WEAPON_NONE;
    }
    
    printf("\nLong-Term Storage:\n");
    for (int i = 0; i < player.lts.count; i++) {
        printf("  %d. %s (DMG: %d, Slots: %d)\n",
               i, weapon_name(player.lts.weapons[i]),
               weapon_damage(player.lts.weapons[i]),
               weapon_slot_size(player.lts.weapons[i]));
    }
    printf("  -1. Cancel\n");
    printf("Weapon: ");
    
    int choice = -2;
    while (choice < -1 || choice >= player.lts.count) {
        string input;
        getline(cin, input);
        try {
            choice = stoi(input);
        } catch (...) {
            choice = -2;
        }
        if (choice < -1 || choice >= player.lts.count) {
            printf("Invalid choice. Try again: ");
        }
    }
    
    return (choice >= 0) ? player.lts.weapons[choice] : WEAPON_NONE;
}

