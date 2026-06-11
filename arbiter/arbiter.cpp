#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <SFML/Graphics.hpp>

#include "../shared/game_config.h"
#include "../shared/game_state.h"
#include "../shared/game_screens.h"
#include "../shared/sync.h"

#include "scheduler.h"
#include "combat.h"
#include "deadlock_detector.h"
#include "renderer.h"
#include "signal_manager.h"
#include "enemy_spawner.h"

using namespace std;

GameState* g_state = nullptr;

#ifdef __APPLE__
sem_t* g_mutex = nullptr;
sem_t* g_turn_notify = nullptr;
sem_t* g_action_ready = nullptr;
sem_t* g_artifact_mutex = nullptr;
#endif

static string resolve_binary(const char* name) {
    char buf[512];
    const char* patterns[] = {"./%s", "./%s/%s", "../%s/%s", nullptr};
    struct stat st;
    for (int i = 0; patterns[i]; i++) {
        if (i == 0)
            snprintf(buf, sizeof(buf), patterns[i], name);
        else
            snprintf(buf, sizeof(buf), patterns[i], name, name);
        
        if (stat(buf, &st) == 0 && S_ISREG(st.st_mode) && access(buf, X_OK) == 0) {
            return string(buf);
        }
    }
    snprintf(buf, sizeof(buf), "./%s", name);
    return string(buf);
}

static string resolve_resources() {
    const char* candidates[] = {
        "Resources",
        "../Resources",
        "../../Resources",
        nullptr
    };
    struct stat st;
    for (int i = 0; candidates[i]; i++) {
        if (stat(candidates[i], &st) == 0 && S_ISDIR(st.st_mode))
            return string(candidates[i]);
    }
    return "Resources";
}

static pthread_t g_scheduler_tid = 0;
static pthread_t g_deadlock_tid = 0;
static bool g_battle_threads_active = false;

static void start_battle(MenuState& menu) {
    SEM_CLEANUP_ALL();
    init_game_state(g_state);
    MUTEX_INIT();
    TURN_SEM_INIT();
    ACTION_SEM_INIT();
    ARTIFACT_MUTEX_INIT();
    g_state->arbiter_pid = getpid();
    
    cout << "[Arbiter] Starting battle — Floor " << g_state->current_floor << endl;

    g_state->game_mode = (int)menu.selected_mode;
    g_state->battle_started = true;
    g_state->game_over = false;
    g_state->player_won = false;

    g_state->num_players = menu.city_party_count;
    unsigned int rng_seed = ROLL_NUMBER;

    int player_idx = 0;
    for (int i = 0; i < 4; i++) {
        if (menu.city_char_selected[i]) {
            init_player(&g_state->players[player_idx], player_idx, 
                       g_state->num_players, &rng_seed);
            player_idx++;
        }
    }

    int enemy_count = determine_enemy_count(&rng_seed);
    spawn_enemies(g_state, enemy_count, &rng_seed);

    shared_deposit(&g_state->shared_lts, WEAPON_SOLAR_CORE, -1);
    shared_deposit(&g_state->shared_lts, WEAPON_LUNAR_BLADE, -1);
    shared_deposit(&g_state->shared_lts, WEAPON_IRON_HALBERD, -1);
    shared_deposit(&g_state->shared_lts, WEAPON_THUNDERSTAFF, -1);
    shared_deposit(&g_state->shared_lts, WEAPON_OBSIDIAN_AXE, -1);

    g_state->initialized = true;

    setup_arbiter_signal_handlers(g_state);

    pthread_create(&g_scheduler_tid, nullptr, scheduler_thread, g_state);
    pthread_create(&g_deadlock_tid, nullptr, deadlock_detector_thread, g_state);
    g_battle_threads_active = true;

    string hip_path = resolve_binary("hip");
    pid_t hip_pid_1 = fork();
    if (hip_pid_1 == 0) {
        if (g_state->game_mode == MODE_TWO_PLAYER) {
            execl(hip_path.c_str(), "hip", "0", (char*)NULL);
        } else {
            execl(hip_path.c_str(), "hip", "-1", (char*)NULL);
        }
        fprintf(stderr, "[Arbiter] execl hip 1 failed (%s): %s\n", hip_path.c_str(), strerror(errno));
        exit(1);
    }
    g_state->hip_pid[0] = hip_pid_1;
    cout << "[Arbiter] Forked HIP 1 pid=" << hip_pid_1 << " (" << hip_path << ")" << endl;

    if (g_state->game_mode == MODE_TWO_PLAYER) {
        pid_t hip_pid_2 = fork();
        if (hip_pid_2 == 0) {
            execl(hip_path.c_str(), "hip", "1", (char*)NULL);
            fprintf(stderr, "[Arbiter] execl hip 2 failed (%s): %s\n", hip_path.c_str(), strerror(errno));
            exit(1);
        }
        g_state->hip_pid[1] = hip_pid_2;
        cout << "[Arbiter] Forked HIP 2 pid=" << hip_pid_2 << " (" << hip_path << ")" << endl;
    }

    string asp_path = resolve_binary("asp");
    pid_t asp_pid = fork();
    if (asp_pid == 0) {
        execl(asp_path.c_str(), "asp", (char*)NULL);
        fprintf(stderr, "[Arbiter] execl asp failed (%s): %s\n", asp_path.c_str(), strerror(errno));
        exit(1);
    }
    g_state->asp_pid = asp_pid;
    cout << "[Arbiter] Forked ASP pid=" << asp_pid << " (" << asp_path << ")" << endl;

    cout << "[Arbiter] Battle started with " << g_state->num_players 
              << " players vs " << g_state->num_enemies << " enemies." << endl;
}

static void end_battle(MenuState& menu) {
    cout << "[Arbiter] Battle ended." << endl;

    g_state->battle_started = false;

    for (int i = 0; i < 2; i++) {
        if (g_state->hip_pid[i] > 0) {
            kill(g_state->hip_pid[i], SIGTERM);
            waitpid(g_state->hip_pid[i], NULL, 0);
            g_state->hip_pid[i] = 0;
        }
    }
    if (g_state->asp_pid > 0) {
        kill(g_state->asp_pid, SIGTERM);
        waitpid(g_state->asp_pid, NULL, 0);
        g_state->asp_pid = 0;
    }

    if (g_battle_threads_active) {
        pthread_join(g_scheduler_tid, nullptr);
        pthread_join(g_deadlock_tid, nullptr);
        g_battle_threads_active = false;
    }

    if (g_state->player_won) {
        menu.transitioning = true;
        menu.transition_fading_out = true;
        menu.transition_alpha = 0.0f;
        menu.transition_target = SCREEN_VICTORY;
    } else {
        menu.transitioning = true;
        menu.transition_fading_out = true;
        menu.transition_alpha = 0.0f;
        menu.transition_target = SCREEN_DEFEAT;
        menu.menu_cursor = 0;
    }
}

int main() {
    cout << "=== Chrono Rift — Game Arbiter ===" << endl;
    cout << "Roll Number Seed: 24I0682 and 24I0661"<< endl;

    shm_unlink(SHM_NAME);
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(shm_fd, sizeof(GameState)) == -1) {
        perror("ftruncate");
        return 1;
    }

    g_state = (GameState*)mmap(0, sizeof(GameState), PROT_READ | PROT_WRITE,
                                MAP_SHARED, shm_fd, 0);
    if (g_state == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    init_game_state(g_state);
    MUTEX_INIT();
    TURN_SEM_INIT();
    ACTION_SEM_INIT();
    ARTIFACT_MUTEX_INIT();
    g_state->arbiter_pid = getpid();

    MenuState menu;
    init_menu_state(&menu);

    GameSettings settings;
    init_game_settings(&settings);

    GameRenderer renderer;
    string resource_path = resolve_resources();
    cout << "[Arbiter] Resources path: " << resource_path << endl;
    renderer.init(resource_path);

    sf::Clock frame_clock;
    bool battle_active = false;
    float intro_timer = 0.0f;

    while (renderer.isOpen()) {
        float dt = frame_clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        menu.anim_timer += dt;

        if (!renderer.processEvents(menu, settings, g_state)) {
            MUTEX_LOCK();
            g_state->game_over = true;
            MUTEX_UNLOCK();
            break;
        }

        renderer.updateTransition(menu, dt);

        renderer.updateParticles(dt);

        if (menu.current_screen == SCREEN_BATTLE_INTRO && !menu.transitioning) {
            intro_timer += dt;
            if (intro_timer >= 2.5f) {
                intro_timer = 0.0f;
                
                if (!battle_active) {
                    start_battle(menu);
                    battle_active = true;
                }

                menu.transitioning = true;
                menu.transition_fading_out = true;
                menu.transition_alpha = 0.0f;
                menu.transition_target = SCREEN_BATTLE;
            }
        }

        if (battle_active && (menu.current_screen == SCREEN_BATTLE || 
                              menu.current_screen == SCREEN_BOSS_BATTLE)) {
            MUTEX_LOCK();
            bool over = g_state->game_over;
            MUTEX_UNLOCK();

            if (over) {
                end_battle(menu);
                battle_active = false;
            }
        }

        if (menu.current_screen == SCREEN_MAIN_MENU && !menu.transitioning) {
            if (battle_active) {
                MUTEX_LOCK();
                g_state->game_over = true;
                MUTEX_UNLOCK();
                end_battle(menu);
                battle_active = false;
            }
        }

        renderer.render(g_state, menu, settings);
    }

    renderer.close();

    cout << "[Arbiter] Shutting down..." << endl;

    if (battle_active) {
        MUTEX_LOCK();
        g_state->game_over = true;
        MUTEX_UNLOCK();

        for (int i = 0; i < 2; i++) {
            if (g_state->hip_pid[i] > 0) {
                kill(g_state->hip_pid[i], SIGTERM);
                waitpid(g_state->hip_pid[i], NULL, 0);
            }
        }
        if (g_state->asp_pid > 0) {
            kill(g_state->asp_pid, SIGTERM);
            waitpid(g_state->asp_pid, NULL, 0);
        }

        if (g_battle_threads_active) {
            pthread_join(g_scheduler_tid, nullptr);
            pthread_join(g_deadlock_tid, nullptr);
        }
    }

    SEM_CLEANUP_ALL();
    munmap(g_state, sizeof(GameState));
    shm_unlink(SHM_NAME);

    cout << "[Arbiter] Shutdown complete." << endl;
    return 0;
}

