#include "renderer.h"
#include <cstdio>

using namespace std;

void GameRenderer::renderVictory(const GameState* state, const MenuState& menu) {
    drawGradientRect(sf::Vector2f(0, 0), sf::Vector2f((float)m_width, (float)m_height),
                     sf::Color(5, 15, 5), sf::Color(10, 25, 10),
                     sf::Color(5, 10, 5), sf::Color(10, 20, 10));
    drawParticles();

    float pulse = 0.5f + 0.5f * sinf(menu.anim_timer * 2.5f);
    sf::Uint8 alpha = (sf::Uint8)(180 + 75 * pulse);

    drawCenteredText("VICTORY!", 120.0f, 52, sf::Color(255, 215, 0, alpha));
    drawCenteredText("The Rift has been sealed.", 190.0f, 18, sf::Color(200, 220, 200));

    float pw = 400.0f, ph = 220.0f;
    float px = ((float)m_width - pw) / 2.0f;
    float py = 240.0f;
    drawPanel(sf::Vector2f(px, py), sf::Vector2f(pw, ph),
              sf::Color(20, 30, 20, 200), sf::Color(100, 200, 100));

    if (state) {
        float sx = px + 30.0f, sy = py + 20.0f;
        char buf[64];

        snprintf(buf, sizeof(buf), "Enemies Slain: %d", state->enemies_killed);
        drawText(buf, sf::Vector2f(sx, sy), 16, sf::Color(220, 220, 220));

        snprintf(buf, sizeof(buf), "Total Spawned: %d", state->total_enemies_spawned);
        drawText(buf, sf::Vector2f(sx, sy + 30), 16, sf::Color(200, 200, 200));

        snprintf(buf, sizeof(buf), "Game Ticks: %d", state->game_tick);
        drawText(buf, sf::Vector2f(sx, sy + 60), 16, sf::Color(200, 200, 200));

        snprintf(buf, sizeof(buf), "Floor Reached: %d", state->current_floor);
        drawText(buf, sf::Vector2f(sx, sy + 90), 16, sf::Color(200, 200, 200));

        int alive = 0;
        for (int i = 0; i < state->num_players; i++) {
            if (state->players[i].is_active) alive++;
        }
        snprintf(buf, sizeof(buf), "Survivors: %d/%d", alive, state->num_players);
        drawText(buf, sf::Vector2f(sx, sy + 120), 16,
                 alive > 0 ? sf::Color(100, 255, 100) : sf::Color(255, 100, 100));
    }

    drawCenteredText("Press ENTER to return to menu", (float)m_height - 80.0f, 18,
                     sf::Color(200, 200, 200, (sf::Uint8)(150 + 105 * pulse)));
}

void GameRenderer::handleVictoryInput(const sf::Event& event, MenuState& menu) {
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return) {
        menu.transitioning = true;
        menu.transition_fading_out = true;
        menu.transition_alpha = 0.0f;
        menu.transition_target = SCREEN_MAIN_MENU;
        menu.menu_cursor = 0;
    }
}

void GameRenderer::renderDefeat(const GameState* state, const MenuState& menu) {
    drawGradientRect(sf::Vector2f(0, 0), sf::Vector2f((float)m_width, (float)m_height),
                     sf::Color(20, 5, 5), sf::Color(30, 5, 10),
                     sf::Color(15, 5, 5), sf::Color(25, 5, 10));
    drawParticles();

    float pulse = 0.5f + 0.5f * sinf(menu.anim_timer * 1.5f);
    sf::Uint8 alpha = (sf::Uint8)(180 + 75 * pulse);

    drawCenteredText("DEFEAT", 140.0f, 52, sf::Color(220, 50, 50, alpha));
    drawCenteredText("The Rift consumes all...", 210.0f, 18, sf::Color(200, 160, 160));

    float pw = 400.0f, ph = 160.0f;
    float px = ((float)m_width - pw) / 2.0f;
    float py = 270.0f;
    drawPanel(sf::Vector2f(px, py), sf::Vector2f(pw, ph),
              sf::Color(30, 15, 15, 200), sf::Color(200, 80, 80));

    if (state) {
        float sx = px + 30.0f, sy = py + 20.0f;
        char buf[64];

        snprintf(buf, sizeof(buf), "Enemies Slain: %d/%d", state->enemies_killed, WIN_KILL_COUNT);
        drawText(buf, sf::Vector2f(sx, sy), 16, sf::Color(220, 220, 220));

        snprintf(buf, sizeof(buf), "Game Ticks: %d", state->game_tick);
        drawText(buf, sf::Vector2f(sx, sy + 30), 16, sf::Color(200, 200, 200));

        snprintf(buf, sizeof(buf), "Floor Reached: %d", state->current_floor);
        drawText(buf, sf::Vector2f(sx, sy + 60), 16, sf::Color(200, 200, 200));
    }

    float btn_y = py + ph + 40.0f;
    float cx = (float)m_width / 2.0f - 80.0f;

    drawMenuItem("Retry", sf::Vector2f(cx, btn_y), menu.menu_cursor == 0, true, 22);
    drawMenuItem("Return to Menu", sf::Vector2f(cx, btn_y + 45), menu.menu_cursor == 1, true, 22);
}

void GameRenderer::handleDefeatInput(const sf::Event& event, MenuState& menu) {
    if (event.type != sf::Event::KeyPressed) return;

    switch (event.key.code) {
        case sf::Keyboard::W:
            menu.menu_cursor = 0;
            break;
        case sf::Keyboard::S:
            menu.menu_cursor = 1;
            break;
        case sf::Keyboard::Return:
            if (menu.menu_cursor == 0) {
                menu.transitioning = true;
                menu.transition_fading_out = true;
                menu.transition_alpha = 0.0f;
                menu.transition_target = SCREEN_CITY;
                menu.city_cursor = 0;
                for (int i = 0; i < 4; i++) menu.city_char_selected[i] = false;
                menu.city_party_count = 0;
            } else {
                menu.transitioning = true;
                menu.transition_fading_out = true;
                menu.transition_alpha = 0.0f;
                menu.transition_target = SCREEN_MAIN_MENU;
                menu.menu_cursor = 0;
            }
            break;
        default: break;
    }
}

void GameRenderer::renderPause(const MenuState& menu) {
    sf::RectangleShape dim(sf::Vector2f((float)m_width, (float)m_height));
    dim.setFillColor(sf::Color(0, 0, 0, 160));
    m_window.draw(dim);

    float pw = 300.0f, ph = 250.0f;
    float px = ((float)m_width - pw) / 2.0f;
    float py = ((float)m_height - ph) / 2.0f;
    drawPanel(sf::Vector2f(px, py), sf::Vector2f(pw, ph),
              sf::Color(20, 20, 35, 230), sf::Color(100, 100, 160));

    drawCenteredText("PAUSED", py + 25.0f, 28, sf::Color(255, 215, 0));

    float ix = px + 40.0f;
    float sy = py + 80.0f;
    float spacing = 50.0f;

    drawMenuItem("Resume", sf::Vector2f(ix, sy), menu.menu_cursor == 0, true, 22);
    drawMenuItem("Settings", sf::Vector2f(ix, sy + spacing), menu.menu_cursor == 1, true, 22);
    drawMenuItem("Quit to Menu", sf::Vector2f(ix, sy + spacing * 2), menu.menu_cursor == 2, true, 22);
}

void GameRenderer::handlePauseInput(const sf::Event& event, MenuState& menu) {
    if (event.type != sf::Event::KeyPressed) return;

    switch (event.key.code) {
        case sf::Keyboard::W:
            menu.menu_cursor = (menu.menu_cursor - 1 + 3) % 3;
            break;
        case sf::Keyboard::S:
            menu.menu_cursor = (menu.menu_cursor + 1) % 3;
            break;
        case sf::Keyboard::Return:
            if (menu.menu_cursor == 0) {
                menu.current_screen = menu.previous_screen;
            } else if (menu.menu_cursor == 1) {
                menu.previous_screen = menu.current_screen;
                menu.transitioning = true;
                menu.transition_fading_out = true;
                menu.transition_alpha = 0.0f;
                menu.transition_target = SCREEN_SETTINGS;
                menu.settings_cursor = 0;
            } else if (menu.menu_cursor == 2) {
                menu.transitioning = true;
                menu.transition_fading_out = true;
                menu.transition_alpha = 0.0f;
                menu.transition_target = SCREEN_MAIN_MENU;
                menu.menu_cursor = 0;
            }
            break;
        default: break;
    }
}

