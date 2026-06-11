#include "renderer.h"
#include <cstdio>

using namespace std;

void GameRenderer::renderMainMenu(const MenuState& menu) {
    if (m_menu_bg_loaded) {
        sf::Sprite bg(m_menu_bg_tex);
        float sx = (float)m_width / m_menu_bg_tex.getSize().x;
        float sy = (float)m_height / m_menu_bg_tex.getSize().y;
        bg.setScale(sx, sy);
        m_window.draw(bg);
    } else {
        drawGradientRect(sf::Vector2f(0, 0), sf::Vector2f((float)m_width, (float)m_height),
                         sf::Color(10, 10, 26), sf::Color(26, 10, 46),
                         sf::Color(10, 10, 26), sf::Color(26, 10, 46));
    }

    drawPanel(sf::Vector2f(0, 0), sf::Vector2f((float)m_width, (float)m_height),
              sf::Color(0, 0, 0, 100), sf::Color::Transparent, 0);

    drawParticles();

    drawTitle("CHRONO RIFT", 120.0f, menu.anim_timer);

    drawCenteredText("A Turn-Based Chronicle", 185.0f, 16, sf::Color(180, 160, 200));

    float start_y = 320.0f;
    float spacing = 50.0f;
    float center_x = (float)m_width / 2.0f - 100.0f;

    const char* items[] = {"New Game", "Continue Game", "Settings", "Exit"};
    bool enabled[] = {true, menu.continue_available, true, true};

    for (int i = 0; i < 4; i++) {
        drawMenuItem(items[i], sf::Vector2f(center_x, start_y + i * spacing),
                     menu.menu_cursor == i, enabled[i], 22);
    }

    drawCenteredText("Roll: 24i-0682  |  Seed: 682", (float)m_height - 40.0f, 12,
                     sf::Color(100, 100, 120));
}

void GameRenderer::handleMenuInput(const sf::Event& event, MenuState& menu) {
    if (event.type != sf::Event::KeyPressed) return;

    switch (event.key.code) {
        case sf::Keyboard::W:
            menu.menu_cursor = (menu.menu_cursor - 1 + 4) % 4;
            break;
        case sf::Keyboard::S:
            menu.menu_cursor = (menu.menu_cursor + 1) % 4;
            break;
        case sf::Keyboard::Return:
            switch (menu.menu_cursor) {
                case 0:
                    menu.transitioning = true;
                    menu.transition_fading_out = true;
                    menu.transition_alpha = 0.0f;
                    menu.transition_target = SCREEN_MODE_SELECT;
                    menu.mode_cursor = 0;
                    break;
                case 1:
                    break;
                case 2:
                    menu.transitioning = true;
                    menu.transition_fading_out = true;
                    menu.transition_alpha = 0.0f;
                    menu.transition_target = SCREEN_SETTINGS;
                    menu.previous_screen = SCREEN_MAIN_MENU;
                    break;
                case 3:
                    m_window.close();
                    break;
            }
            break;
        case sf::Keyboard::Escape:
            m_window.close();
            break;
        default: break;
    }
}

void GameRenderer::renderModeSelect(const MenuState& menu) {
    if (m_menu_bg_loaded) {
        sf::Sprite bg(m_menu_bg_tex);
        bg.setScale((float)m_width / m_menu_bg_tex.getSize().x,
                     (float)m_height / m_menu_bg_tex.getSize().y);
        m_window.draw(bg);
    }
    drawPanel(sf::Vector2f(0, 0), sf::Vector2f((float)m_width, (float)m_height),
              sf::Color(0, 0, 0, 120), sf::Color::Transparent, 0);
    drawParticles();

    drawCenteredText("SELECT MODE", 140.0f, 32, sf::Color(255, 215, 0));

    float pw = 400.0f, ph = 250.0f;
    float px = ((float)m_width - pw) / 2.0f;
    float py = 230.0f;
    drawPanel(sf::Vector2f(px, py), sf::Vector2f(pw, ph),
              sf::Color(26, 26, 46, 200), sf::Color(80, 80, 120));

    const char* items[] = {"Single Player", "2-Player Mode", "Back"};
    float start_y = py + 40.0f;
    float spacing = 55.0f;
    float ix = px + 50.0f;

    for (int i = 0; i < 3; i++) {
        drawMenuItem(items[i], sf::Vector2f(ix, start_y + i * spacing),
                     menu.mode_cursor == i, true, 22);
    }
}

void GameRenderer::handleModeSelectInput(const sf::Event& event, MenuState& menu) {
    if (event.type != sf::Event::KeyPressed) return;

    switch (event.key.code) {
        case sf::Keyboard::W:
            menu.mode_cursor = (menu.mode_cursor - 1 + 3) % 3;
            break;
        case sf::Keyboard::S:
            menu.mode_cursor = (menu.mode_cursor + 1) % 3;
            break;
        case sf::Keyboard::Return:
            if (menu.mode_cursor == 0) {
                menu.selected_mode = MODE_SINGLE_PLAYER;
                menu.transitioning = true;
                menu.transition_fading_out = true;
                menu.transition_alpha = 0.0f;
                menu.transition_target = SCREEN_CITY;
                menu.city_cursor = 0;
                for (int i = 0; i < 4; i++) menu.city_char_selected[i] = false;
                menu.city_party_count = 0;
            } else if (menu.mode_cursor == 1) {
                menu.selected_mode = MODE_TWO_PLAYER;
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
            }
            break;
        case sf::Keyboard::Escape:
            menu.transitioning = true;
            menu.transition_fading_out = true;
            menu.transition_alpha = 0.0f;
            menu.transition_target = SCREEN_MAIN_MENU;
            break;
        default: break;
    }
}

void GameRenderer::renderSettings(const MenuState& menu, const GameSettings& settings) {
    if (m_menu_bg_loaded) {
        sf::Sprite bg(m_menu_bg_tex);
        bg.setScale((float)m_width / m_menu_bg_tex.getSize().x,
                     (float)m_height / m_menu_bg_tex.getSize().y);
        m_window.draw(bg);
    }
    drawPanel(sf::Vector2f(0, 0), sf::Vector2f((float)m_width, (float)m_height),
              sf::Color(0, 0, 0, 120), sf::Color::Transparent, 0);

    drawCenteredText("SETTINGS", 100.0f, 32, sf::Color(255, 215, 0));

    float pw = 500.0f, ph = 350.0f;
    float px = ((float)m_width - pw) / 2.0f;
    float py = 180.0f;
    drawPanel(sf::Vector2f(px, py), sf::Vector2f(pw, ph),
              sf::Color(26, 26, 46, 200), sf::Color(80, 80, 120));

    float ix = px + 40.0f;
    float start_y = py + 40.0f;
    float spacing = 55.0f;
    float slider_w = pw - 80.0f;

    drawSlider("Master Volume", sf::Vector2f(ix, start_y),
               slider_w, settings.master_volume, menu.settings_cursor == 0);
    drawSlider("Music Volume", sf::Vector2f(ix, start_y + spacing),
               slider_w, settings.music_volume, menu.settings_cursor == 1);
    drawSlider("SFX Volume", sf::Vector2f(ix, start_y + spacing * 2),
               slider_w, settings.sfx_volume, menu.settings_cursor == 2);

    bool mute_sel = (menu.settings_cursor == 3);
    string mute_text = settings.muted ? "Muted: [ON]" : "Muted: [OFF]";
    drawMenuItem(mute_text, sf::Vector2f(ix, start_y + spacing * 3),
                 mute_sel, true, 18);

    drawMenuItem("Back", sf::Vector2f(ix, start_y + spacing * 4),
                 menu.settings_cursor == 4, true, 18);
}

void GameRenderer::handleSettingsInput(const sf::Event& event, MenuState& menu,
                                       GameSettings& settings) {
    if (event.type != sf::Event::KeyPressed) return;

    switch (event.key.code) {
        case sf::Keyboard::W:
            menu.settings_cursor = (menu.settings_cursor - 1 + 5) % 5;
            break;
        case sf::Keyboard::S:
            menu.settings_cursor = (menu.settings_cursor + 1) % 5;
            break;
        case sf::Keyboard::A:
            if (menu.settings_cursor == 0) settings.master_volume = max(0.0f, settings.master_volume - 0.05f);
            if (menu.settings_cursor == 1) settings.music_volume  = max(0.0f, settings.music_volume  - 0.05f);
            if (menu.settings_cursor == 2) settings.sfx_volume    = max(0.0f, settings.sfx_volume    - 0.05f);
            break;
        case sf::Keyboard::D:
            if (menu.settings_cursor == 0) settings.master_volume = min(1.0f, settings.master_volume + 0.05f);
            if (menu.settings_cursor == 1) settings.music_volume  = min(1.0f, settings.music_volume  + 0.05f);
            if (menu.settings_cursor == 2) settings.sfx_volume    = min(1.0f, settings.sfx_volume    + 0.05f);
            break;
        case sf::Keyboard::Return:
            if (menu.settings_cursor == 3) {
                settings.muted = !settings.muted;
            } else if (menu.settings_cursor == 4) {
                menu.transitioning = true;
                menu.transition_fading_out = true;
                menu.transition_alpha = 0.0f;
                menu.transition_target = menu.previous_screen;
            }
            break;
        case sf::Keyboard::Escape:
            menu.transitioning = true;
            menu.transition_fading_out = true;
            menu.transition_alpha = 0.0f;
            menu.transition_target = menu.previous_screen;
            break;
        default: break;
    }
}

void GameRenderer::renderCity(const GameState*, const MenuState& menu) {
    drawGradientRect(sf::Vector2f(0, 0), sf::Vector2f((float)m_width, (float)m_height),
                     sf::Color(10, 10, 26), sf::Color(20, 10, 40),
                     sf::Color(15, 15, 35), sf::Color(26, 10, 46));
    drawParticles();

    drawCenteredText("ASSEMBLE YOUR PARTY", 60.0f, 28, sf::Color(255, 215, 0));
    drawCenteredText("Select 1-4 characters for battle", 100.0f, 14, sf::Color(180, 180, 200));

    float card_w = 200.0f, card_h = 320.0f;
    float total_w = 4 * card_w + 3 * 20.0f;
    float start_x = ((float)m_width - total_w) / 2.0f;
    float card_y = 150.0f;

    const char* names[] = {"Chrono Knight", "Rift Mage", "Shadow Rogue", "Aegis Tank"};
    const char* classes[] = {"Balanced", "High DMG", "Fast", "Durable"};
    sf::Color card_colors[] = {
        sf::Color(60, 100, 200), sf::Color(160, 50, 200),
        sf::Color(50, 180, 100), sf::Color(200, 160, 50)
    };

    for (int i = 0; i < 4; i++) {
        float cx = start_x + i * (card_w + 20.0f);
        bool selected = menu.city_char_selected[i];
        bool hovered = (menu.city_cursor == i);

        sf::Color fill = selected ? sf::Color(40, 60, 80, 220) : sf::Color(26, 26, 46, 200);
        sf::Color outline = hovered ? sf::Color(255, 215, 0) :
                            selected ? sf::Color(100, 200, 100) : sf::Color(60, 60, 90);
        float thick = hovered ? 2.5f : 1.0f;
        drawPanel(sf::Vector2f(cx, card_y), sf::Vector2f(card_w, card_h), fill, outline, thick);

        sf::CircleShape icon(40.0f);
        icon.setPosition(cx + card_w / 2 - 40, card_y + 20.0f);
        icon.setFillColor(card_colors[i]);
        icon.setOutlineColor(sf::Color(255, 255, 255, 80));
        icon.setOutlineThickness(2.0f);
        m_window.draw(icon);

        drawText(names[i], sf::Vector2f(cx + 30.0f, card_y + 115.0f), 16, sf::Color(240, 240, 240));
        drawText(classes[i], sf::Vector2f(cx + 55.0f, card_y + 140.0f), 12, sf::Color(160, 160, 180));

        drawText("HP:  682+", sf::Vector2f(cx + 20.0f, card_y + 175.0f), 12, sf::Color(100, 200, 80));
        drawText("DMG: 12", sf::Vector2f(cx + 20.0f, card_y + 195.0f), 12, sf::Color(255, 130, 130));
        char spd[16];
        snprintf(spd, sizeof(spd), "SPD: %d", 100);
        drawText(spd, sf::Vector2f(cx + 20.0f, card_y + 215.0f), 12, sf::Color(130, 180, 255));

        if (selected) {
            drawText("SELECTED", sf::Vector2f(cx + 55.0f, card_y + card_h - 40.0f),
                     14, sf::Color(100, 255, 100));
        }
    }

    float btn_y = card_y + card_h + 30.0f;
    bool has_party = (menu.city_party_count > 0);

    drawMenuItem("Ready", sf::Vector2f((float)m_width / 2 - 150, btn_y),
                 menu.city_cursor == 4, has_party, 22);
    drawMenuItem("Back", sf::Vector2f((float)m_width / 2 + 50, btn_y),
                 menu.city_cursor == 5, true, 22);

    char party_text[32];
    snprintf(party_text, sizeof(party_text), "Party: %d/4", menu.city_party_count);
    drawCenteredText(party_text, btn_y + 45.0f, 16,
                     has_party ? sf::Color(100, 255, 100) : sf::Color(255, 100, 100));
}

void GameRenderer::handleCityInput(const sf::Event& event, MenuState& menu) {
    if (event.type != sf::Event::KeyPressed) return;

    switch (event.key.code) {
        case sf::Keyboard::A:
            if (menu.city_cursor > 0) menu.city_cursor--;
            break;
        case sf::Keyboard::D:
            if (menu.city_cursor < 5) menu.city_cursor++;
            break;
        case sf::Keyboard::W:
            if (menu.city_cursor >= 4) menu.city_cursor = 0;
            break;
        case sf::Keyboard::S:
            if (menu.city_cursor < 4) menu.city_cursor = 4;
            break;
        case sf::Keyboard::Return:
        case sf::Keyboard::Space:
            if (menu.city_cursor < 4) {
                menu.city_char_selected[menu.city_cursor] = !menu.city_char_selected[menu.city_cursor];
                menu.city_party_count = 0;
                for (int i = 0; i < 4; i++) {
                    if (menu.city_char_selected[i]) menu.city_party_count++;
                }
            } else if (menu.city_cursor == 4 && menu.city_party_count > 0) {
                menu.transitioning = true;
                menu.transition_fading_out = true;
                menu.transition_alpha = 0.0f;
                menu.transition_target = SCREEN_BATTLE_INTRO;
            } else if (menu.city_cursor == 5) {
                menu.transitioning = true;
                menu.transition_fading_out = true;
                menu.transition_alpha = 0.0f;
                menu.transition_target = SCREEN_MODE_SELECT;
            }
            break;
        case sf::Keyboard::Escape:
            menu.transitioning = true;
            menu.transition_fading_out = true;
            menu.transition_alpha = 0.0f;
            menu.transition_target = SCREEN_MODE_SELECT;
            break;
        default: break;
    }
}

