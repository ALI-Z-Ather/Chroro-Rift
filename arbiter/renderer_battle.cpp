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

void GameRenderer::renderBattleIntro(const GameState* state, const MenuState& menu) {
    drawGradientRect(sf::Vector2f(0, 0), sf::Vector2f((float)m_width, (float)m_height),
                     sf::Color(5, 5, 15), sf::Color(15, 5, 25),
                     sf::Color(5, 5, 15), sf::Color(15, 5, 25));

    int floor = state ? state->current_floor : 1;
    char floor_text[32];
    snprintf(floor_text, sizeof(floor_text), "FLOOR %d", floor);

    float pulse = 0.5f + 0.5f * sinf(menu.anim_timer * 3.0f);
    sf::Uint8 alpha = (sf::Uint8)(150 + 105 * pulse);

    drawCenteredText(floor_text, (float)m_height / 2 - 60, 48,
                     sf::Color(255, 215, 0, alpha));

    if (state && state->boss_active) {
        drawCenteredText("BOSS ENCOUNTER", (float)m_height / 2 + 10, 24,
                         sf::Color(255, 50, 50, alpha));
    } else {
        drawCenteredText("Prepare for Battle", (float)m_height / 2 + 10, 20,
                         sf::Color(200, 200, 220, alpha));
    }

    drawParticles();
}

void GameRenderer::renderBattle(const GameState* state, MenuState& menu) {
    if (m_battle_bg_loaded) {
        sf::Sprite bg(m_battle_bg_tex);
        float sx = (float)m_width / m_battle_bg_tex.getSize().x;
        float sy = (float)m_height / m_battle_bg_tex.getSize().y;
        bg.setScale(sx, sy);
        m_window.draw(bg);
    } else {
        drawGradientRect(sf::Vector2f(0, 0), sf::Vector2f((float)m_width, (float)m_height),
                         sf::Color(20, 15, 30), sf::Color(25, 20, 40),
                         sf::Color(15, 10, 25), sf::Color(20, 15, 35));
    }

    if (state == nullptr) {
        drawCenteredText("Waiting for game state...", 350.0f, 24);
        return;
    }

    float player_start_y = 80.0f;
    float player_spacing = 160.0f;
    for (int i = 0; i < state->num_players; i++) {
        bool is_turn = state->turn.turn_pending &&
                       state->turn.active_is_player &&
                       state->turn.active_id == i;
        drawPlayerPanel(state->players[i], sf::Vector2f(30, player_start_y + i * player_spacing),
                        i, is_turn);
    }

    float enemy_start_y = 80.0f;
    float enemy_spacing = 70.0f;
    for (int i = 0; i < state->num_enemies; i++) {
        if (!state->enemies[i].is_active) continue;
        bool is_turn = state->turn.turn_pending &&
                       !state->turn.active_is_player &&
                       state->turn.active_id == i;
        drawEnemyPanel(state->enemies[i], sf::Vector2f(600, enemy_start_y + i * enemy_spacing),
                       i, is_turn);
    }

    drawBattleLog(state, sf::Vector2f(10, 560), sf::Vector2f(1004, 200));

    drawTurnIndicator(state);

    drawGameStatus(state);

    drawArtifactPanel(state, sf::Vector2f(350, 10));

    if (state->turn.turn_pending && state->turn.active_is_player &&
        menu.battle_input.phase == BATTLE_INPUT_NONE) {
        populateBattleInputSnapshot(state, state->turn.active_id, menu.battle_input);
        menu.battle_input.phase = BATTLE_INPUT_ACTION;
    }
    if (!state->turn.turn_pending || !state->turn.active_is_player) {
        menu.battle_input.phase = BATTLE_INPUT_NONE;
    }
    renderActionMenu(state, menu.battle_input);
}

void GameRenderer::drawPlayerPanel(const Character& ch, sf::Vector2f pos,
                                   int index, bool is_active_turn) {
    float pw = 250.0f, ph = 140.0f;

    sf::RectangleShape panel(sf::Vector2f(pw, ph));
    panel.setPosition(pos);

    if (!ch.is_active) {
        panel.setFillColor(sf::Color(40, 40, 40, 150));
    } else if (is_active_turn) {
        panel.setFillColor(sf::Color(60, 60, 100, 200));
        panel.setOutlineColor(sf::Color(255, 215, 0));
        panel.setOutlineThickness(2.0f);
    } else {
        panel.setFillColor(sf::Color(30, 30, 50, 180));
        panel.setOutlineColor(sf::Color(80, 80, 120));
        panel.setOutlineThickness(1.0f);
    }
    m_window.draw(panel);

    if (!ch.is_active) {
        drawText("DEFEATED", sf::Vector2f(pos.x + 10, pos.y + 55), 16, sf::Color(150, 50, 50));
        return;
    }

    string name = "Player " + to_string(index);
    drawText(name, sf::Vector2f(pos.x + 10, pos.y + 5), 14, sf::Color(100, 200, 255));

    if (ch.is_stunned) {
        drawText("[STUNNED]", sf::Vector2f(pos.x + pw - 90, pos.y + 5), 12, sf::Color(255, 255, 0));
    }

    drawHPBar(sf::Vector2f(pos.x + 10, pos.y + 25), pw - 20, 12, ch.hp, ch.max_hp);

    drawStaminaBar(sf::Vector2f(pos.x + 10, pos.y + 42), pw - 20, 8, ch.stamina, ch.max_stamina);

    char stats[64];
    snprintf(stats, sizeof(stats), "DMG:%d  SPD:%d  HP:%d/%d",
             ch.damage, ch.speed, ch.hp, ch.max_hp);
    drawText(stats, sf::Vector2f(pos.x + 10, pos.y + 55), 11, sf::Color(200, 200, 200));

    int wcount = count_weapons_in_inventory(ch.inventory);
    char inv[32];
    snprintf(inv, sizeof(inv), "Inventory: %d weapons", wcount);
    drawText(inv, sf::Vector2f(pos.x + 10, pos.y + 72), 11, sf::Color(180, 180, 180));

    float badge_y = pos.y + 92;
    if (ch.holds_solar_core)
        drawText("[Solar Core]", sf::Vector2f(pos.x + 10, badge_y), 10, sf::Color(255, 200, 50));
    if (ch.holds_lunar_blade)
        drawText("[Lunar Blade]", sf::Vector2f(pos.x + 120, badge_y), 10, sf::Color(150, 200, 255));
}

void GameRenderer::drawEnemyPanel(const Character& ch, sf::Vector2f pos,
                                  int index, bool is_active_turn) {
    float pw = 180.0f, ph = 55.0f;

    sf::RectangleShape panel(sf::Vector2f(pw, ph));
    panel.setPosition(pos);

    if (!ch.is_active) {
        panel.setFillColor(sf::Color(40, 40, 40, 150));
    } else if (is_active_turn) {
        panel.setFillColor(sf::Color(100, 40, 40, 200));
        panel.setOutlineColor(sf::Color(255, 100, 100));
        panel.setOutlineThickness(2.0f);
    } else {
        panel.setFillColor(sf::Color(30, 30, 50, 180));
        panel.setOutlineColor(sf::Color(80, 80, 120));
        panel.setOutlineThickness(1.0f);
    }
    m_window.draw(panel);

    if (!ch.is_active) {
        drawText("DEAD", sf::Vector2f(pos.x + 10, pos.y + 15), 12, sf::Color(150, 50, 50));
        return;
    }

    string name = "Enemy " + to_string(index);
    drawText(name, sf::Vector2f(pos.x + 10, pos.y + 3), 12, sf::Color(255, 100, 100));

    if (ch.is_stunned) {
        drawText("[S]", sf::Vector2f(pos.x + pw - 30, pos.y + 3), 10, sf::Color(255, 255, 0));
    }

    drawHPBar(sf::Vector2f(pos.x + 10, pos.y + 20), pw - 20, 10, ch.hp, ch.max_hp);
    drawStaminaBar(sf::Vector2f(pos.x + 10, pos.y + 35), pw - 20, 6, ch.stamina, ch.max_stamina);
}

void GameRenderer::drawBattleLog(const GameState* state, sf::Vector2f pos, sf::Vector2f size) {
    sf::RectangleShape panel(size);
    panel.setPosition(pos);
    panel.setFillColor(sf::Color(10, 10, 20, 220));
    panel.setOutlineColor(sf::Color(60, 60, 90));
    panel.setOutlineThickness(1.0f);
    m_window.draw(panel);

    drawText("— Battle Log —", sf::Vector2f(pos.x + 10, pos.y + 5), 12, sf::Color(200, 200, 255));

    ActionLogEntry recent[12];
    int count = get_recent_logs(&state->action_log, recent, 12);

    for (int i = 0; i < count; i++) {
        float alpha = 255.0f - (float)(count - 1 - i) * 15.0f;
        if (alpha < 80) alpha = 80;
        drawText(recent[i].message,
                 sf::Vector2f(pos.x + 15, pos.y + 22 + i * 15),
                 11, sf::Color(220, 220, 220, (sf::Uint8)alpha));
    }
}

void GameRenderer::drawTurnIndicator(const GameState* state) {
    string text;
    sf::Color color;

    if (state->game_over) {
        text = state->player_won ? "VICTORY!" : "DEFEAT!";
        color = state->player_won ? sf::Color(255, 215, 0) : sf::Color(255, 50, 50);
    } else if (state->turn.turn_pending) {
        if (state->turn.active_is_player) {
            text = "Player " + to_string(state->turn.active_id) + "'s Turn";
            color = sf::Color(100, 200, 255);
        } else {
            text = "Enemy " + to_string(state->turn.active_id) + "'s Turn";
            color = sf::Color(255, 100, 100);
        }
    } else {
        text = "Charging...";
        color = sf::Color(150, 150, 150);
    }

    drawText(text, sf::Vector2f(10, 10), 18, color);
}

void GameRenderer::drawGameStatus(const GameState* state) {
    char status[64];
    snprintf(status, sizeof(status), "Kills: %d/%d  |  Tick: %d",
             state->enemies_killed, WIN_KILL_COUNT, state->game_tick);
    drawText(status, sf::Vector2f(10, 40), 12, sf::Color(180, 180, 180));
}

void GameRenderer::drawArtifactPanel(const GameState* state, sf::Vector2f pos) {
    drawText("Artifacts:", sf::Vector2f(pos.x, pos.y), 12, sf::Color(255, 215, 0));

    const char* sc_s = is_solar_core_free(&state->artifacts) ? "Free" : "Held";
    char sc[64]; snprintf(sc, sizeof(sc), "Solar Core: %s", sc_s);
    drawText(sc, sf::Vector2f(pos.x, pos.y + 18), 11,
             is_solar_core_free(&state->artifacts) ? sf::Color(100, 255, 100) : sf::Color(255, 150, 50));

    const char* lb_s = is_lunar_blade_free(&state->artifacts) ? "Free" : "Held";
    char lb[64]; snprintf(lb, sizeof(lb), "Lunar Blade: %s", lb_s);
    drawText(lb, sf::Vector2f(pos.x + 180, pos.y + 18), 11,
             is_lunar_blade_free(&state->artifacts) ? sf::Color(100, 255, 100) : sf::Color(255, 150, 50));

    if (state->artifacts.eclipse_relic_exists) {
        const char* er_s = is_eclipse_relic_free(&state->artifacts) ? "Free" : "Held";
        char er[64]; snprintf(er, sizeof(er), "Eclipse Relic: %s", er_s);
        drawText(er, sf::Vector2f(pos.x + 360, pos.y + 18), 11,
                 is_eclipse_relic_free(&state->artifacts) ? sf::Color(100, 255, 100) : sf::Color(255, 150, 50));
    }
}

