#include "renderer.h"
#include "../shared/actions.h"
#include "../shared/sync.h"
#include <cstdio>
#include <string>

using namespace std;

#ifdef __APPLE__
extern sem_t* g_mutex;
extern sem_t* g_turn_notify;
extern sem_t* g_action_ready;
extern sem_t* g_artifact_mutex;
#endif

void GameRenderer::populateBattleInputSnapshot(const GameState* state, int player_id,
                                               BattleInputState& bi) {
    const Character& player = state->players[player_id];

    bi.inv_count = list_inventory_weapons(player.inventory,
                                          bi.inv_weapons, bi.inv_starts, 20);
    bi.lts_count = 0;
    for (int i = 0; i < state->shared_lts.count && i < MAX_LTS_SIZE; i++)
        bi.lts_weapons[bi.lts_count++] = state->shared_lts.weapons[i];

    bi.active_enemy_count = 0;
    for (int i = 0; i < state->num_enemies && bi.active_enemy_count < 10; i++)
        if (state->enemies[i].is_active)
            bi.active_enemy_ids[bi.active_enemy_count++] = i;

    bi.can_use_ultimate = true;

    bi.action_cursor = bi.target_cursor = bi.weapon_cursor = bi.lts_cursor = 0;
    bi.chosen_action = bi.chosen_target_id = bi.chosen_weapon_slot = bi.chosen_lts_index = -1;
}

void GameRenderer::submitBattleAction(GameState* state, const BattleInputState& bi) {
    ActionRequest action;
    clear_action_request(&action);
    action.actor_id        = state->turn.active_id;
    action.actor_is_player = true;

    switch (bi.chosen_action) {
        case ACTION_STRIKE:
            action.type = ACTION_STRIKE;
            action.target_id = bi.chosen_target_id;
            action.target_is_player = false;
            break;
        case ACTION_EXHAUST:
            action.type = ACTION_EXHAUST;
            action.target_id = bi.chosen_target_id;
            action.target_is_player = false;
            break;
        case ACTION_USE_WEAPON:
            action.type = ACTION_USE_WEAPON;
            action.weapon_slot = bi.chosen_weapon_slot;
            action.target_id = bi.chosen_target_id;
            action.target_is_player = false;
            break;
        case ACTION_SWAP_IN:
            action.type = ACTION_SWAP_IN;
            action.swap_weapon = bi.lts_weapons[bi.chosen_lts_index];
            break;
        case ACTION_PUT_STORAGE:
            action.type = ACTION_PUT_STORAGE;
            action.weapon_slot = bi.chosen_weapon_slot;
            break;
        case ACTION_HEAL:   action.type = ACTION_HEAL;    break;
        case ACTION_SKIP:   action.type = ACTION_SKIP;    break;
        case ACTION_ULTIMATE: action.type = ACTION_ULTIMATE; break;
        default:            action.type = ACTION_SKIP;    break;
    }

    MUTEX_LOCK();
    state->current_action           = action;
    state->current_action.submitted = true;
    MUTEX_UNLOCK();

    ACTION_SEM_POST();
}

void GameRenderer::renderActionMenu(const GameState* state, const BattleInputState& bi) {
    if (bi.phase == BATTLE_INPUT_NONE || bi.phase == BATTLE_INPUT_CONFIRM) return;

    const float PW    = 370.0f;
    const float PX    = (float)m_width / 2.0f - PW / 2.0f;
    const float PY    = 185.0f;
    const float ROW_H = 32.0f;
    const float PAD   = 14.0f;

    if (bi.phase == BATTLE_INPUT_ACTION) {
        const char* labels[8] = {
            "Strike         (reduce enemy HP)",
            "Exhaust        (reduce enemy Stamina)",
            "Use Weapon     (inventory)",
            "Swap In        (team storage)",
            "Put to Storage (inventory)",
            "Heal           (restore 10% HP)",
            "Skip           (restore 50% Stamina)",
            nullptr
        };
        char ult_buf[48] = "Ultimate       (hold both artifacts)";
        if (bi.can_use_ultimate) labels[7] = ult_buf;
        int n = bi.can_use_ultimate ? 8 : 7;

        float ph = PAD + 20.0f + n * ROW_H + PAD;
        drawPanel({PX, PY}, {PW, ph}, sf::Color(10,10,30,235), sf::Color(255,215,0,200), 2.0f);

        char hdr[48];
        snprintf(hdr, sizeof(hdr), "— Player %d: Choose Action —", state->turn.active_id);
        drawText(hdr, {PX + PAD, PY + 6.0f}, 13, sf::Color(255,215,0));

        for (int i = 0; i < n; i++) {
            if (!labels[i]) continue;
            float ry  = PY + PAD + 18.0f + i * ROW_H;
            bool  sel = (bi.action_cursor == i);
            bool  dis = (i == 2 && bi.inv_count == 0) || (i == 3 && bi.lts_count == 0) || (i == 4 && bi.inv_count == 0);

            if (sel) {
                sf::RectangleShape bar({PW - 4.0f, ROW_H - 2.0f});
                bar.setPosition(PX + 2.0f, ry); bar.setFillColor(sf::Color(255,215,0,40));
                m_window.draw(bar);
            }
            sf::Color c = dis ? sf::Color(70,70,70) : (sel ? sf::Color(255,215,0) : sf::Color(220,220,220));
            drawText((sel ? "> " : "  ") + string(labels[i]), {PX + PAD, ry + 5.0f}, 13, c);
        }
        drawText("Up/Down: Navigate    Enter: Select    Esc: Cancel",
                 {PX + PAD, PY + ph + 4.0f}, 10, sf::Color(120,120,140));
        return;
    }

    if (bi.phase == BATTLE_INPUT_TARGET) {
        float ph = PAD + 20.0f + bi.active_enemy_count * ROW_H + PAD;
        drawPanel({PX, PY}, {PW, ph}, sf::Color(10,10,30,235), sf::Color(255,80,80,200), 2.0f);
        drawText("— Select Target —", {PX + PAD, PY + 6.0f}, 13, sf::Color(255,100,100));

        for (int i = 0; i < bi.active_enemy_count; i++) {
            int eid = bi.active_enemy_ids[i];
            float ry = PY + PAD + 18.0f + i * ROW_H;
            bool  sel = (bi.target_cursor == i);
            const Character& e = state->enemies[eid];
            char row[80];
            snprintf(row, sizeof(row), "%sEnemy %d   HP:%d/%d  SPD:%d",
                     sel ? "> " : "  ", eid, e.hp, e.max_hp, e.speed);
            if (sel) {
                sf::RectangleShape bar({PW-4.0f, ROW_H-2.0f});
                bar.setPosition(PX+2.0f, ry); bar.setFillColor(sf::Color(255,80,80,40));
                m_window.draw(bar);
            }
            drawText(row, {PX+PAD, ry+5.0f}, 13, sel ? sf::Color(255,215,0) : sf::Color(220,220,220));
        }
        drawText("Up/Down: Navigate    Enter: Confirm    Esc: Back",
                 {PX+PAD, PY+ph+4.0f}, 10, sf::Color(120,120,140));
        return;
    }

    if (bi.phase == BATTLE_INPUT_WEAPON || bi.phase == BATTLE_INPUT_PUT_STORAGE) {
        int   total = bi.inv_count + 1;
        float ph    = PAD + 20.0f + total * ROW_H + PAD;
        drawPanel({PX, PY}, {PW, ph}, sf::Color(10,10,30,235), sf::Color(100,160,255,200), 2.0f);
        if (bi.phase == BATTLE_INPUT_WEAPON)
            drawText("— Select Weapon to Use —", {PX+PAD, PY+6.0f}, 13, sf::Color(120,180,255));
        else
            drawText("— Select Weapon to Store —", {PX+PAD, PY+6.0f}, 13, sf::Color(120,180,255));

        for (int i = 0; i < bi.inv_count; i++) {
            float ry  = PY + PAD + 18.0f + i * ROW_H;
            bool  sel = (bi.weapon_cursor == i);
            char  row[80];
            snprintf(row, sizeof(row), "%s%-16s  DMG:%-3d  Slot:%d",
                     sel?"> ":"  ", weapon_name(bi.inv_weapons[i]),
                     weapon_damage(bi.inv_weapons[i]), bi.inv_starts[i]);
            if (sel) {
                sf::RectangleShape bar({PW-4.0f, ROW_H-2.0f});
                bar.setPosition(PX+2.0f, ry); bar.setFillColor(sf::Color(100,160,255,40));
                m_window.draw(bar);
            }
            drawText(row, {PX+PAD, ry+5.0f}, 13, sel?sf::Color(255,215,0):sf::Color(220,220,220));
        }
        float cy  = PY + PAD + 18.0f + bi.inv_count * ROW_H;
        bool  csel = (bi.weapon_cursor == bi.inv_count);
        drawText((csel?"> ":"  ") + string("Cancel"),
                 {PX+PAD, cy+5.0f}, 13, csel?sf::Color(255,100,100):sf::Color(140,140,140));
        drawText("Up/Down: Navigate    Enter: Select    Esc: Back",
                 {PX+PAD, PY+ph+4.0f}, 10, sf::Color(120,120,140));
        return;
    }

    if (bi.phase == BATTLE_INPUT_LTS) {
        int   total = bi.lts_count + 1;
        float ph    = PAD + 20.0f + total * ROW_H + PAD;
        drawPanel({PX, PY}, {PW, ph}, sf::Color(10,10,30,235), sf::Color(180,100,255,200), 2.0f);
        drawText("— Swap In (Long-Term Storage) —", {PX+PAD, PY+6.0f}, 13, sf::Color(200,140,255));

        for (int i = 0; i < bi.lts_count; i++) {
            float ry  = PY + PAD + 18.0f + i * ROW_H;
            bool  sel = (bi.lts_cursor == i);
            char  row[80];
            snprintf(row, sizeof(row), "%s%-16s  DMG:%-3d  Size:%d",
                     sel?"> ":"  ", weapon_name(bi.lts_weapons[i]),
                     weapon_damage(bi.lts_weapons[i]), weapon_slot_size(bi.lts_weapons[i]));
            if (sel) {
                sf::RectangleShape bar({PW-4.0f, ROW_H-2.0f});
                bar.setPosition(PX+2.0f, ry); bar.setFillColor(sf::Color(180,100,255,40));
                m_window.draw(bar);
            }
            drawText(row, {PX+PAD, ry+5.0f}, 13, sel?sf::Color(255,215,0):sf::Color(220,220,220));
        }
        float cy  = PY + PAD + 18.0f + bi.lts_count * ROW_H;
        bool  csel = (bi.lts_cursor == bi.lts_count);
        drawText((csel?"> ":"  ") + string("Cancel"),
                 {PX+PAD, cy+5.0f}, 13, csel?sf::Color(255,100,100):sf::Color(140,140,140));
        drawText("Up/Down: Navigate    Enter: Select    Esc: Back",
                 {PX+PAD, PY+ph+4.0f}, 10, sf::Color(120,120,140));
    }
}

void GameRenderer::handleBattleInput(const sf::Event& event, MenuState& menu, GameState* state) {
    if (!state || event.type != sf::Event::KeyPressed) return;

    BattleInputState& bi = menu.battle_input;

    MUTEX_LOCK();
    bool my_turn = state->turn.turn_pending &&
                   state->turn.active_is_player &&
                   bi.phase == BATTLE_INPUT_NONE;
    int  player_id = state->turn.active_id;
    MUTEX_UNLOCK();

    if (my_turn) {
        MUTEX_LOCK();
        populateBattleInputSnapshot(state, player_id, bi);
        MUTEX_UNLOCK();
        bi.phase = BATTLE_INPUT_ACTION;
        return;
    }
    if (bi.phase == BATTLE_INPUT_NONE) return;

    int n_actions = bi.can_use_ultimate ? 8 : 7;

    if (bi.phase == BATTLE_INPUT_ACTION) {
        if (event.key.code == sf::Keyboard::W)
            bi.action_cursor = (bi.action_cursor - 1 + n_actions) % n_actions;
        else if (event.key.code == sf::Keyboard::S)
            bi.action_cursor = (bi.action_cursor + 1) % n_actions;
        else if (event.key.code == sf::Keyboard::Return) {
            bool dis = (bi.action_cursor == 2 && bi.inv_count == 0) ||
                       (bi.action_cursor == 3 && bi.lts_count == 0) ||
                       (bi.action_cursor == 4 && bi.inv_count == 0);
            if (dis) return;
            switch (bi.action_cursor) {
                case 0: bi.chosen_action = ACTION_STRIKE;   bi.phase = BATTLE_INPUT_TARGET; bi.target_cursor = 0; break;
                case 1: bi.chosen_action = ACTION_EXHAUST;  bi.phase = BATTLE_INPUT_TARGET; bi.target_cursor = 0; break;
                case 2: bi.chosen_action = ACTION_USE_WEAPON; bi.phase = BATTLE_INPUT_WEAPON; bi.weapon_cursor = 0; break;
                case 3: bi.chosen_action = ACTION_SWAP_IN;  bi.phase = BATTLE_INPUT_LTS;    bi.lts_cursor    = 0; break;
                case 4: bi.chosen_action = ACTION_PUT_STORAGE; bi.phase = BATTLE_INPUT_PUT_STORAGE; bi.weapon_cursor = 0; break;
                case 5: bi.chosen_action = ACTION_HEAL;     submitBattleAction(state, bi); bi.phase = BATTLE_INPUT_NONE; break;
                case 6: bi.chosen_action = ACTION_SKIP;     submitBattleAction(state, bi); bi.phase = BATTLE_INPUT_NONE; break;
                case 7: bi.chosen_action = ACTION_ULTIMATE; submitBattleAction(state, bi); bi.phase = BATTLE_INPUT_NONE; break;
            }
        } else if (event.key.code == sf::Keyboard::Escape)
            bi.phase = BATTLE_INPUT_NONE;
        return;
    }

    if (bi.phase == BATTLE_INPUT_TARGET) {
        if (event.key.code == sf::Keyboard::W)
            bi.target_cursor = (bi.target_cursor - 1 + bi.active_enemy_count) % bi.active_enemy_count;
        else if (event.key.code == sf::Keyboard::S)
            bi.target_cursor = (bi.target_cursor + 1) % bi.active_enemy_count;
        else if (event.key.code == sf::Keyboard::Return) {
            bi.chosen_target_id = bi.active_enemy_ids[bi.target_cursor];
            submitBattleAction(state, bi);
            bi.phase = BATTLE_INPUT_NONE;
        } else if (event.key.code == sf::Keyboard::Escape)
            bi.phase = BATTLE_INPUT_ACTION;
        return;
    }

    if (bi.phase == BATTLE_INPUT_WEAPON || bi.phase == BATTLE_INPUT_PUT_STORAGE) {
        int total = bi.inv_count + 1;
        if (event.key.code == sf::Keyboard::W)
            bi.weapon_cursor = (bi.weapon_cursor - 1 + total) % total;
        else if (event.key.code == sf::Keyboard::S)
            bi.weapon_cursor = (bi.weapon_cursor + 1) % total;
        else if (event.key.code == sf::Keyboard::Return) {
            if (bi.weapon_cursor == bi.inv_count) {
                bi.phase = BATTLE_INPUT_ACTION;
            } else {
                bi.chosen_weapon_slot = bi.inv_starts[bi.weapon_cursor];
                if (bi.phase == BATTLE_INPUT_WEAPON) {
                    bi.phase = BATTLE_INPUT_TARGET;
                    bi.target_cursor = 0;
                } else {
                    submitBattleAction(state, bi);
                    bi.phase = BATTLE_INPUT_NONE;
                }
            }
        } else if (event.key.code == sf::Keyboard::Escape)
            bi.phase = BATTLE_INPUT_ACTION;
        return;
    }

    if (bi.phase == BATTLE_INPUT_LTS) {
        int total = bi.lts_count + 1;
        if (event.key.code == sf::Keyboard::W)
            bi.lts_cursor = (bi.lts_cursor - 1 + total) % total;
        else if (event.key.code == sf::Keyboard::S)
            bi.lts_cursor = (bi.lts_cursor + 1) % total;
        else if (event.key.code == sf::Keyboard::Return) {
            if (bi.lts_cursor == bi.lts_count) { bi.phase = BATTLE_INPUT_ACTION; }
            else { bi.chosen_lts_index = bi.lts_cursor; submitBattleAction(state, bi); bi.phase = BATTLE_INPUT_NONE; }
        } else if (event.key.code == sf::Keyboard::Escape)
            bi.phase = BATTLE_INPUT_ACTION;
    }
}

