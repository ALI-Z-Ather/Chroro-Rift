#include "renderer.h"
#include <cstdio>
#include <cstdlib>
#include <sstream>

using namespace std;

GameRenderer::GameRenderer()
    : m_font_loaded(false)
    , m_width(1024)
    , m_height(768)
    , m_menu_bg_loaded(false)
    , m_enemy_tex_loaded(false)
    , m_battle_bg_loaded(false)
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        m_player_tex_loaded[i] = false;
    }
}

GameRenderer::~GameRenderer() {
    if (m_window.isOpen()) {
        m_window.close();
    }
}

bool GameRenderer::init(const string& resource_path, int width, int height) {
    m_width = width;
    m_height = height;
    m_resource_path = resource_path;

    m_window.create(sf::VideoMode(width, height), "Chrono Rift",
                    sf::Style::Titlebar | sf::Style::Close);
    m_window.setFramerateLimit(60);

    string font_path = resource_path + "/fonts/game_font.ttf";
    if (!m_font.loadFromFile(font_path)) {
#ifdef __APPLE__
        if (!m_font.loadFromFile("/System/Library/Fonts/Menlo.ttc")) {
            fprintf(stderr, "Warning: Could not load any font\n");
        } else {
            m_font_loaded = true;
        }
#else
        if (!m_font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf")) {
            fprintf(stderr, "Warning: Could not load any font\n");
        } else {
            m_font_loaded = true;
        }
#endif
    } else {
        m_font_loaded = true;
    }

    m_menu_bg_loaded = m_menu_bg_tex.loadFromFile(resource_path + "/backgrounds/menu_bg.png");
    m_battle_bg_loaded = m_battle_bg_tex.loadFromFile(resource_path + "/backgrounds/battle_bg.png");

    for (int i = 0; i < MAX_PLAYERS; i++) {
        string path = resource_path + "/sprites/player_" + to_string(i) + ".png";
        m_player_tex_loaded[i] = m_player_textures[i].loadFromFile(path);
    }
    m_enemy_tex_loaded = m_enemy_texture.loadFromFile(resource_path + "/sprites/enemy_default.png");

    initParticles(120);

    m_clock.restart();
    return true;
}

bool GameRenderer::isOpen() const {
    return m_window.isOpen();
}

void GameRenderer::close() {
    m_window.close();
}

sf::RenderWindow& GameRenderer::getWindow() {
    return m_window;
}

bool GameRenderer::processEvents(MenuState& menu, GameSettings& settings, GameState* state) {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
            return false;
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            if (menu.current_screen == SCREEN_BATTLE || menu.current_screen == SCREEN_BOSS_BATTLE) {
                menu.previous_screen = menu.current_screen;
                menu.current_screen = SCREEN_PAUSE;
                continue;
            } else if (menu.current_screen == SCREEN_PAUSE) {
                menu.current_screen = menu.previous_screen;
                continue;
            }
        }

        if (menu.transitioning) continue;

        switch (menu.current_screen) {
            case SCREEN_MAIN_MENU:   handleMenuInput(event, menu); break;
            case SCREEN_MODE_SELECT: handleModeSelectInput(event, menu); break;
            case SCREEN_SETTINGS:    handleSettingsInput(event, menu, settings); break;
            case SCREEN_CITY:        handleCityInput(event, menu); break;
            case SCREEN_BATTLE:
            case SCREEN_BOSS_BATTLE: handleBattleInput(event, menu, state); break;
            case SCREEN_VICTORY:     handleVictoryInput(event, menu); break;
            case SCREEN_DEFEAT:      handleDefeatInput(event, menu); break;
            case SCREEN_PAUSE:       handlePauseInput(event, menu); break;
            default: break;
        }
    }
    return true;
}

void GameRenderer::render(const GameState* state, MenuState& menu, const GameSettings& settings) {
    m_window.clear(sf::Color(10, 10, 26));

    switch (menu.current_screen) {
        case SCREEN_MAIN_MENU:    renderMainMenu(menu); break;
        case SCREEN_MODE_SELECT:  renderModeSelect(menu); break;
        case SCREEN_SETTINGS:     renderSettings(menu, settings); break;
        case SCREEN_CITY:         renderCity(state, menu); break;
        case SCREEN_BATTLE_INTRO: renderBattleIntro(state, menu); break;
        case SCREEN_BATTLE:
        case SCREEN_BOSS_BATTLE:  renderBattle(state, menu); break;
        case SCREEN_VICTORY:      renderVictory(state, menu); break;
        case SCREEN_DEFEAT:       renderDefeat(state, menu); break;
        case SCREEN_PAUSE:        renderPause(menu); break;
        default: break;
    }

    drawTransition(menu);

    m_window.display();
}

void GameRenderer::initParticles(int count) {
    m_particles.resize(count);
    for (int i = 0; i < count; i++) {
        m_particles[i].x = (float)(rand() % m_width);
        m_particles[i].y = (float)(rand() % m_height);
        m_particles[i].speed = 5.0f + (float)(rand() % 16);
        m_particles[i].size = 1.0f + (float)(rand() % 3);
        m_particles[i].phase = (float)(rand() % 628) / 100.0f;
        m_particles[i].twinkle_speed = 1.0f + (float)(rand() % 30) / 10.0f;
        m_particles[i].alpha = 80.0f + (float)(rand() % 176);
    }
}

void GameRenderer::updateParticles(float dt) {
    float time = m_clock.getElapsedTime().asSeconds();
    for (auto& p : m_particles) {
        p.y -= p.speed * dt;
        if (p.y < -5.0f) {
            p.y = (float)m_height + 5.0f;
            p.x = (float)(rand() % m_width);
        }
        p.alpha = 80.0f + 175.0f * (0.5f + 0.5f * sinf(time * p.twinkle_speed + p.phase));
    }
}

void GameRenderer::drawParticles() {
    for (const auto& p : m_particles) {
        sf::CircleShape dot(p.size);
        dot.setPosition(p.x, p.y);
        dot.setFillColor(sf::Color(220, 220, 255, (sf::Uint8)p.alpha));
        m_window.draw(dot);
    }
}

void GameRenderer::updateTransition(MenuState& menu, float dt) {
    if (!menu.transitioning) return;

    float speed = 600.0f;
    if (menu.transition_fading_out) {
        menu.transition_alpha += speed * dt;
        if (menu.transition_alpha >= 255.0f) {
            menu.transition_alpha = 255.0f;
            menu.current_screen = menu.transition_target;
            menu.transition_fading_out = false;
        }
    } else {
        menu.transition_alpha -= speed * dt;
        if (menu.transition_alpha <= 0.0f) {
            menu.transition_alpha = 0.0f;
            menu.transitioning = false;
        }
    }
}

void GameRenderer::drawTransition(const MenuState& menu) {
    if (menu.transition_alpha <= 0.0f) return;
    sf::RectangleShape overlay(sf::Vector2f((float)m_width, (float)m_height));
    overlay.setFillColor(sf::Color(0, 0, 0, (sf::Uint8)menu.transition_alpha));
    m_window.draw(overlay);
}

void GameRenderer::drawText(const string& text, sf::Vector2f pos,
                            unsigned int size, sf::Color color) {
    if (!m_font_loaded) return;
    sf::Text t;
    t.setFont(m_font);
    t.setString(text);
    t.setCharacterSize(size);
    t.setFillColor(color);
    t.setPosition(pos);
    m_window.draw(t);
}

void GameRenderer::drawCenteredText(const string& text, float y,
                                    unsigned int size, sf::Color color) {
    if (!m_font_loaded) return;
    sf::Text t;
    t.setFont(m_font);
    t.setString(text);
    t.setCharacterSize(size);
    t.setFillColor(color);
    sf::FloatRect bounds = t.getLocalBounds();
    t.setPosition((float)m_width / 2.0f - bounds.width / 2.0f, y);
    m_window.draw(t);
}

void GameRenderer::drawTitle(const string& text, float y, float timer) {
    if (!m_font_loaded) return;

    float glow_alpha = 60.0f + 40.0f * sinf(timer * 2.0f);
    sf::Text glow;
    glow.setFont(m_font);
    glow.setString(text);
    glow.setCharacterSize(52);
    glow.setFillColor(sf::Color(255, 215, 0, (sf::Uint8)glow_alpha));
    sf::FloatRect gb = glow.getLocalBounds();
    glow.setPosition((float)m_width / 2.0f - gb.width / 2.0f - 2.0f, y - 2.0f);
    m_window.draw(glow);

    sf::Text title;
    title.setFont(m_font);
    title.setString(text);
    title.setCharacterSize(48);
    title.setFillColor(sf::Color(255, 215, 0));
    title.setOutlineColor(sf::Color(180, 120, 0));
    title.setOutlineThickness(2.0f);
    sf::FloatRect tb = title.getLocalBounds();
    title.setPosition((float)m_width / 2.0f - tb.width / 2.0f, y);
    m_window.draw(title);
}

void GameRenderer::drawPanel(sf::Vector2f pos, sf::Vector2f size, sf::Color fill,
                             sf::Color outline, float thickness) {
    sf::RectangleShape panel(size);
    panel.setPosition(pos);
    panel.setFillColor(fill);
    panel.setOutlineColor(outline);
    panel.setOutlineThickness(thickness);
    m_window.draw(panel);
}

void GameRenderer::drawMenuItem(const string& text, sf::Vector2f pos,
                                bool selected, bool enabled, unsigned int size) {
    if (!m_font_loaded) return;

    if (selected) {
        sf::RectangleShape bar(sf::Vector2f(300.0f, (float)size + 12.0f));
        bar.setPosition(pos.x - 10.0f, pos.y - 2.0f);
        bar.setFillColor(sf::Color(255, 215, 0, 30));
        bar.setOutlineColor(sf::Color(255, 215, 0, 80));
        bar.setOutlineThickness(1.0f);
        m_window.draw(bar);
    }

    sf::Color color;
    if (!enabled)      color = sf::Color(85, 85, 85);
    else if (selected) color = sf::Color(255, 215, 0);
    else               color = sf::Color(220, 220, 220);

    string display = selected ? "> " + text : "  " + text;
    drawText(display, pos, size, color);
}

void GameRenderer::drawSlider(const string& label, sf::Vector2f pos, float width,
                              float value, bool selected) {
    sf::Color label_color = selected ? sf::Color(255, 215, 0) : sf::Color(200, 200, 200);
    drawText(label, pos, 18, label_color);

    float track_x = pos.x + 180.0f;
    float track_y = pos.y + 5.0f;
    float track_w = width - 180.0f;
    float track_h = 12.0f;

    sf::RectangleShape track(sf::Vector2f(track_w, track_h));
    track.setPosition(track_x, track_y);
    track.setFillColor(sf::Color(40, 40, 60));
    track.setOutlineColor(selected ? sf::Color(255, 215, 0, 120) : sf::Color(80, 80, 120));
    track.setOutlineThickness(1.0f);
    m_window.draw(track);

    sf::RectangleShape fill(sf::Vector2f(track_w * value, track_h));
    fill.setPosition(track_x, track_y);
    fill.setFillColor(selected ? sf::Color(255, 215, 0, 180) : sf::Color(100, 100, 200));
    m_window.draw(fill);

    char val_text[8];
    snprintf(val_text, sizeof(val_text), "%d%%", (int)(value * 100));
    drawText(val_text, sf::Vector2f(track_x + track_w + 10.0f, pos.y), 16, label_color);
}

void GameRenderer::drawHPBar(sf::Vector2f pos, float width, float height,
                             int hp, int max_hp) {
    float ratio = (max_hp > 0) ? (float)hp / max_hp : 0;

    sf::RectangleShape bg(sf::Vector2f(width, height));
    bg.setPosition(pos);
    bg.setFillColor(sf::Color(40, 10, 10));
    bg.setOutlineColor(sf::Color(80, 80, 80));
    bg.setOutlineThickness(1.0f);
    m_window.draw(bg);

    sf::RectangleShape fill(sf::Vector2f(width * ratio, height));
    fill.setPosition(pos);
    fill.setFillColor(getHPColor(ratio));
    m_window.draw(fill);

    char hp_text[32];
    snprintf(hp_text, sizeof(hp_text), "%d/%d", hp, max_hp);
    drawText(hp_text, sf::Vector2f(pos.x + width / 2 - 20, pos.y - 1), 10);
}

void GameRenderer::drawStaminaBar(sf::Vector2f pos, float width, float height,
                                  int stam, int max_stam) {
    float ratio = (max_stam > 0) ? (float)stam / max_stam : 0;

    sf::RectangleShape bg(sf::Vector2f(width, height));
    bg.setPosition(pos);
    bg.setFillColor(sf::Color(10, 10, 40));
    m_window.draw(bg);

    sf::RectangleShape fill(sf::Vector2f(width * ratio, height));
    fill.setPosition(pos);
    fill.setFillColor(getStaminaColor(ratio));
    m_window.draw(fill);
}

void GameRenderer::drawGradientRect(sf::Vector2f pos, sf::Vector2f size,
                                    sf::Color tl, sf::Color tr,
                                    sf::Color bl, sf::Color br) {
    sf::VertexArray quad(sf::Quads, 4);
    quad[0].position = sf::Vector2f(pos.x, pos.y);
    quad[1].position = sf::Vector2f(pos.x + size.x, pos.y);
    quad[2].position = sf::Vector2f(pos.x + size.x, pos.y + size.y);
    quad[3].position = sf::Vector2f(pos.x, pos.y + size.y);
    quad[0].color = tl;
    quad[1].color = tr;
    quad[2].color = br;
    quad[3].color = bl;
    m_window.draw(quad);
}

sf::Color GameRenderer::getHPColor(float ratio) {
    if (ratio > 0.6f) return sf::Color(50, 200, 80);
    if (ratio > 0.3f) return sf::Color(230, 180, 30);
    return sf::Color(220, 50, 50);
}

sf::Color GameRenderer::getStaminaColor(float ratio) {
    if (ratio >= 1.0f) return sf::Color(255, 215, 0);
    return sf::Color(60, 120, 255);
}

sf::Color GameRenderer::lerpColor(sf::Color a, sf::Color b, float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return sf::Color(
        (sf::Uint8)(a.r + (b.r - a.r) * t),
        (sf::Uint8)(a.g + (b.g - a.g) * t),
        (sf::Uint8)(a.b + (b.b - a.b) * t),
        (sf::Uint8)(a.a + (b.a - a.a) * t)
    );
}
