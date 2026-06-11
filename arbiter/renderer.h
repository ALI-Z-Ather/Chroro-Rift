#pragma once

#include "../shared/game_state.h"
#include "../shared/game_screens.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <cmath>

using namespace std;

struct Particle {
    float x, y;
    float speed;
    float size;
    float alpha;
    float phase;
    float twinkle_speed;
};

class GameRenderer {
public:
    GameRenderer();
    ~GameRenderer();

    bool init(const string& resource_path, int width = 1024, int height = 768);
    bool processEvents(MenuState& menu, GameSettings& settings, GameState* state);
    void render(const GameState* state, MenuState& menu, const GameSettings& settings);
    bool isOpen() const;
    void close();
    sf::RenderWindow& getWindow();

    void updateTransition(MenuState& menu, float dt);
    void updateParticles(float dt);

private:
    sf::RenderWindow m_window;
    sf::Font m_font;
    bool m_font_loaded;
    sf::Clock m_clock;
    int m_width, m_height;
    string m_resource_path;

    sf::Texture m_menu_bg_tex;
    bool m_menu_bg_loaded;
    sf::Texture m_player_textures[MAX_PLAYERS];
    bool m_player_tex_loaded[MAX_PLAYERS];
    sf::Texture m_enemy_texture;
    bool m_enemy_tex_loaded;
    sf::Texture m_battle_bg_tex;
    bool m_battle_bg_loaded;

    vector<Particle> m_particles;
    void initParticles(int count = 120);
    void drawParticles();

    void renderMainMenu(const MenuState& menu);
    void renderModeSelect(const MenuState& menu);
    void renderSettings(const MenuState& menu, const GameSettings& settings);
    void renderCity(const GameState* state, const MenuState& menu);
    void renderBattleIntro(const GameState* state, const MenuState& menu);
    void renderBattle(const GameState* state, MenuState& menu);
    void renderVictory(const GameState* state, const MenuState& menu);
    void renderDefeat(const GameState* state, const MenuState& menu);
    void renderPause(const MenuState& menu);

    void handleMenuInput(const sf::Event& event, MenuState& menu);
    void handleModeSelectInput(const sf::Event& event, MenuState& menu);
    void handleSettingsInput(const sf::Event& event, MenuState& menu, GameSettings& settings);
    void handleCityInput(const sf::Event& event, MenuState& menu);
    void handleBattleInput(const sf::Event& event, MenuState& menu, GameState* state);
    void handleVictoryInput(const sf::Event& event, MenuState& menu);
    void handleDefeatInput(const sf::Event& event, MenuState& menu);
    void handlePauseInput(const sf::Event& event, MenuState& menu);

    void drawTransition(const MenuState& menu);

    void drawGradientRect(sf::Vector2f pos, sf::Vector2f size,
                          sf::Color top_left, sf::Color top_right,
                          sf::Color bot_left, sf::Color bot_right);
    void drawPanel(sf::Vector2f pos, sf::Vector2f size, sf::Color fill,
                   sf::Color outline = sf::Color(80, 80, 120), float thickness = 1.5f);
    void drawMenuItem(const string& text, sf::Vector2f pos,
                      bool selected, bool enabled = true, unsigned int size = 22);
    void drawSlider(const string& label, sf::Vector2f pos, float width,
                    float value, bool selected);
    void drawHPBar(sf::Vector2f pos, float width, float height, int hp, int max_hp);
    void drawStaminaBar(sf::Vector2f pos, float width, float height, int stam, int max_stam);
    void drawText(const string& text, sf::Vector2f pos,
                  unsigned int size, sf::Color color = sf::Color::White);
    void drawCenteredText(const string& text, float y,
                          unsigned int size, sf::Color color = sf::Color::White);
    void drawTitle(const string& text, float y, float timer);

    void drawPlayerPanel(const Character& ch, sf::Vector2f pos, int index, bool is_active_turn);
    void drawEnemyPanel(const Character& ch, sf::Vector2f pos, int index, bool is_active_turn);
    void drawBattleLog(const GameState* state, sf::Vector2f pos, sf::Vector2f size);
    void drawTurnIndicator(const GameState* state);
    void drawGameStatus(const GameState* state);
    void drawArtifactPanel(const GameState* state, sf::Vector2f pos);
    void renderActionMenu(const GameState* state, const BattleInputState& bi);
    void populateBattleInputSnapshot(const GameState* state, int player_id, BattleInputState& bi);
    void submitBattleAction(GameState* state, const BattleInputState& bi);

    sf::Color getHPColor(float ratio);
    sf::Color getStaminaColor(float ratio);
    sf::Color lerpColor(sf::Color a, sf::Color b, float t);
};

