#pragma once

#define ROLL_NUMBER          682
#define ROLL_LAST_2_DIGITS   82
#define ROLL_LAST_DIGIT      2
#define ROLL_SECOND_LAST     8

#define MAX_PLAYERS          4
#define MAX_ENEMIES          9
#define MIN_PLAYERS          1
#define MIN_ENEMIES          2

#define PLAYER_BASE_HP       ROLL_NUMBER
#define PLAYER_HP_RAND_MIN   100
#define PLAYER_HP_RAND_MAX   1000
#define ENEMY_BASE_HP        ROLL_LAST_2_DIGITS
#define ENEMY_HP_RAND_MIN    50
#define ENEMY_HP_RAND_MAX    200

#define PLAYER_DAMAGE        (ROLL_LAST_DIGIT + 10)
#define ENEMY_DAMAGE         (ROLL_SECOND_LAST + 10)

#define PLAYER_SPEED_BASE    100
#define ENEMY_SPEED_MIN      10
#define ENEMY_SPEED_MAX      30

#define PLAYER_MAX_STAMINA   100
#define ENEMY_MAX_STAMINA    150

#define INVENTORY_SLOTS      20
#define MAX_LTS_SIZE         64

#define STUN_DURATION_SEC    3
#define ULTIMATE_DURATION_SEC 10
#define NPC_TURN_TIMEOUT_SEC 3
#define SCHEDULER_TICK_US    100000

#define WIN_KILL_COUNT       10

#define SHM_NAME             "/chronorift_shm"
#define SEM_MUTEX_NAME       "/chronorift_mutex"
#define SEM_TURN_NAME        "/chronorift_turn"
#define SEM_ACTION_NAME      "/chronorift_action"
#define SEM_ARTIFACT_NAME    "/chronorift_artifact"

#define MAX_LOG_ENTRIES      64
#define MAX_LOG_LENGTH       256

#define HEAL_PERCENT         10

#define SKIP_STAMINA_PERCENT 50

