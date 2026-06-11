#pragma once

#include "game_config.h"
#include "weapons.h"
#include <cstring>

using namespace std;

struct InventorySlot {
    WeaponType weapon;
};

struct LongTermStorage {
    WeaponType weapons[MAX_LTS_SIZE];
    int count;
};

inline void init_inventory(InventorySlot inventory[INVENTORY_SLOTS]) {
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        inventory[i].weapon = WEAPON_NONE;
    }
}

inline void init_lts(LongTermStorage* lts) {
    for (int i = 0; i < MAX_LTS_SIZE; i++) {
        lts->weapons[i] = WEAPON_NONE;
    }
    lts->count = 0;
}

inline int find_contiguous_free(const InventorySlot inventory[INVENTORY_SLOTS], int required_slots) {
    int consecutive = 0;
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (inventory[i].weapon == WEAPON_NONE) {
            consecutive++;
            if (consecutive >= required_slots) {
                return i - required_slots + 1;
            }
        } else {
            consecutive = 0;
        }
    }
    return -1;
}

inline void place_weapon_at(InventorySlot inventory[INVENTORY_SLOTS], 
                            WeaponType weapon, int start_slot) {
    int size = weapon_slot_size(weapon);
    for (int i = start_slot; i < start_slot + size; i++) {
        inventory[i].weapon = weapon;
    }
}

inline void remove_weapon_at(InventorySlot inventory[INVENTORY_SLOTS], int start_slot) {
    WeaponType weapon = inventory[start_slot].weapon;
    if (weapon == WEAPON_NONE) return;
    
    int size = weapon_slot_size(weapon);
    for (int i = start_slot; i < start_slot + size && i < INVENTORY_SLOTS; i++) {
        if (inventory[i].weapon == weapon) {
            inventory[i].weapon = WEAPON_NONE;
        } else {
            break;
        }
    }
}

inline int find_weapon_start(const InventorySlot inventory[INVENTORY_SLOTS], WeaponType weapon) {
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (inventory[i].weapon == weapon) {
            if (i == 0 || inventory[i - 1].weapon != weapon) {
                return i;
            }
        }
    }
    return -1;
}

inline bool move_to_lts(InventorySlot inventory[INVENTORY_SLOTS], 
                        LongTermStorage* lts, int inventory_start_slot) {
    if (lts->count >= MAX_LTS_SIZE) return false;
    
    WeaponType weapon = inventory[inventory_start_slot].weapon;
    if (weapon == WEAPON_NONE) return false;
    
    lts->weapons[lts->count] = weapon;
    lts->count++;
    
    remove_weapon_at(inventory, inventory_start_slot);
    return true;
}

inline int find_in_lts(const LongTermStorage* lts, WeaponType weapon) {
    for (int i = 0; i < lts->count; i++) {
        if (lts->weapons[i] == weapon) return i;
    }
    return -1;
}

inline void remove_from_lts(LongTermStorage* lts, int index) {
    if (index < 0 || index >= lts->count) return;
    for (int i = index; i < lts->count - 1; i++) {
        lts->weapons[i] = lts->weapons[i + 1];
    }
    lts->weapons[lts->count - 1] = WEAPON_NONE;
    lts->count--;
}

inline bool auto_evict_for_space(InventorySlot inventory[INVENTORY_SLOTS], 
                                 LongTermStorage* lts, int required_slots) {
    while (find_contiguous_free(inventory, required_slots) == -1) {
        int best_slot = -1;
        int best_size = INVENTORY_SLOTS + 1;
        
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (inventory[i].weapon != WEAPON_NONE) {
                if (i == 0 || inventory[i - 1].weapon != inventory[i].weapon) {
                    int size = weapon_slot_size(inventory[i].weapon);
                    if (size < best_size) {
                        best_size = size;
                        best_slot = i;
                    }
                }
            }
        }
        
        if (best_slot == -1) return false;
        if (lts->count >= MAX_LTS_SIZE) return false;
        
        move_to_lts(inventory, lts, best_slot);
    }
    
    return find_contiguous_free(inventory, required_slots) != -1;
}

inline int place_weapon(InventorySlot inventory[INVENTORY_SLOTS], 
                        LongTermStorage* lts, WeaponType weapon) {
    int size = weapon_slot_size(weapon);
    
    int slot = find_contiguous_free(inventory, size);
    if (slot != -1) {
        place_weapon_at(inventory, weapon, slot);
        return slot;
    }
    
    if (auto_evict_for_space(inventory, lts, size)) {
        slot = find_contiguous_free(inventory, size);
        if (slot != -1) {
            place_weapon_at(inventory, weapon, slot);
            return slot;
        }
    }
    
    return -1;
}

inline bool swap_in_weapon(InventorySlot inventory[INVENTORY_SLOTS], 
                           LongTermStorage* lts, WeaponType weapon) {
    int lts_idx = find_in_lts(lts, weapon);
    if (lts_idx == -1) return false;
    
    int slot = place_weapon(inventory, lts, weapon);
    if (slot == -1) return false;
    
    remove_from_lts(lts, lts_idx);
    return true;
}

inline int count_weapons_in_inventory(const InventorySlot inventory[INVENTORY_SLOTS]) {
    int count = 0;
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (inventory[i].weapon != WEAPON_NONE) {
            if (i == 0 || inventory[i - 1].weapon != inventory[i].weapon) {
                count++;
            }
        }
    }
    return count;
}

inline int list_inventory_weapons(const InventorySlot inventory[INVENTORY_SLOTS],
                                  WeaponType out_weapons[], int out_starts[], int max_out) {
    int count = 0;
    for (int i = 0; i < INVENTORY_SLOTS && count < max_out; i++) {
        if (inventory[i].weapon != WEAPON_NONE) {
            if (i == 0 || inventory[i - 1].weapon != inventory[i].weapon) {
                out_weapons[count] = inventory[i].weapon;
                out_starts[count] = i;
                count++;
            }
        }
    }
    return count;
}

