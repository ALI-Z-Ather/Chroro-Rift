#pragma once

#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "game_config.h"

using namespace std;

struct GameState;
extern GameState* g_state;

#ifdef __APPLE__

extern sem_t* g_mutex;
extern sem_t* g_turn_notify;
extern sem_t* g_action_ready;
extern sem_t* g_artifact_mutex;

#define MUTEX_INIT() do { \
    sem_unlink(SEM_MUTEX_NAME); \
    g_mutex = sem_open(SEM_MUTEX_NAME, O_CREAT, 0666, 1); \
} while(0)

#define MUTEX_OPEN() do { \
    g_mutex = sem_open(SEM_MUTEX_NAME, 0); \
} while(0)

#define MUTEX_LOCK()    sem_wait(g_mutex)
#define MUTEX_UNLOCK()  sem_post(g_mutex)

#define MUTEX_DESTROY() do { \
    sem_close(g_mutex); \
    sem_unlink(SEM_MUTEX_NAME); \
} while(0)

#define MUTEX_CLOSE()   sem_close(g_mutex)

#define TURN_SEM_INIT() do { \
    sem_unlink(SEM_TURN_NAME); \
    g_turn_notify = sem_open(SEM_TURN_NAME, O_CREAT, 0666, 0); \
} while(0)

#define TURN_SEM_OPEN() do { \
    g_turn_notify = sem_open(SEM_TURN_NAME, 0); \
} while(0)

#define TURN_SEM_POST()    sem_post(g_turn_notify)
#define TURN_SEM_WAIT()    sem_wait(g_turn_notify)
#define TURN_SEM_TRYWAIT() sem_trywait(g_turn_notify)

#define TURN_SEM_DESTROY() do { \
    sem_close(g_turn_notify); \
    sem_unlink(SEM_TURN_NAME); \
} while(0)

#define TURN_SEM_CLOSE()   sem_close(g_turn_notify)

#define ACTION_SEM_INIT() do { \
    sem_unlink(SEM_ACTION_NAME); \
    g_action_ready = sem_open(SEM_ACTION_NAME, O_CREAT, 0666, 0); \
} while(0)

#define ACTION_SEM_OPEN() do { \
    g_action_ready = sem_open(SEM_ACTION_NAME, 0); \
} while(0)

#define ACTION_SEM_POST()    sem_post(g_action_ready)
#define ACTION_SEM_WAIT()    sem_wait(g_action_ready)
#define ACTION_SEM_TRYWAIT() sem_trywait(g_action_ready)

#define ACTION_SEM_DESTROY() do { \
    sem_close(g_action_ready); \
    sem_unlink(SEM_ACTION_NAME); \
} while(0)

#define ACTION_SEM_CLOSE()   sem_close(g_action_ready)

#define ARTIFACT_MUTEX_INIT() do { \
    sem_unlink(SEM_ARTIFACT_NAME); \
    g_artifact_mutex = sem_open(SEM_ARTIFACT_NAME, O_CREAT, 0666, 1); \
} while(0)

#define ARTIFACT_MUTEX_OPEN() do { \
    g_artifact_mutex = sem_open(SEM_ARTIFACT_NAME, 0); \
} while(0)

#define ARTIFACT_LOCK()    sem_wait(g_artifact_mutex)
#define ARTIFACT_UNLOCK()  sem_post(g_artifact_mutex)

#define ARTIFACT_MUTEX_DESTROY() do { \
    sem_close(g_artifact_mutex); \
    sem_unlink(SEM_ARTIFACT_NAME); \
} while(0)

#define ARTIFACT_MUTEX_CLOSE() sem_close(g_artifact_mutex)

#define SEM_CLEANUP_ALL() do { \
    MUTEX_DESTROY(); \
    TURN_SEM_DESTROY(); \
    ACTION_SEM_DESTROY(); \
    ARTIFACT_MUTEX_DESTROY(); \
} while(0)

#define SEM_CLOSE_ALL() do { \
    MUTEX_CLOSE(); \
    TURN_SEM_CLOSE(); \
    ACTION_SEM_CLOSE(); \
    ARTIFACT_MUTEX_CLOSE(); \
} while(0)

#else

#define MUTEX_INIT()    
#define MUTEX_OPEN()    
#define MUTEX_LOCK()    sem_wait(&g_state->mutex)
#define MUTEX_UNLOCK()  sem_post(&g_state->mutex)
#define MUTEX_DESTROY() sem_destroy(&g_state->mutex)
#define MUTEX_CLOSE()   

#define TURN_SEM_INIT()    
#define TURN_SEM_OPEN()    
#define TURN_SEM_POST()    sem_post(&g_state->turn_notify)
#define TURN_SEM_WAIT()    sem_wait(&g_state->turn_notify)
#define TURN_SEM_TRYWAIT() sem_trywait(&g_state->turn_notify)
#define TURN_SEM_DESTROY() sem_destroy(&g_state->turn_notify)
#define TURN_SEM_CLOSE()   

#define ACTION_SEM_INIT()    
#define ACTION_SEM_OPEN()    
#define ACTION_SEM_POST()    sem_post(&g_state->action_ready)
#define ACTION_SEM_WAIT()    sem_wait(&g_state->action_ready)
#define ACTION_SEM_TRYWAIT() sem_trywait(&g_state->action_ready)
#define ACTION_SEM_DESTROY() sem_destroy(&g_state->action_ready)
#define ACTION_SEM_CLOSE()   

#define ARTIFACT_MUTEX_INIT()    
#define ARTIFACT_MUTEX_OPEN()    
#define ARTIFACT_LOCK()          sem_wait(&g_state->artifact_mutex)
#define ARTIFACT_UNLOCK()        sem_post(&g_state->artifact_mutex)
#define ARTIFACT_MUTEX_DESTROY() sem_destroy(&g_state->artifact_mutex)
#define ARTIFACT_MUTEX_CLOSE()   

#define SEM_CLEANUP_ALL() do { \
    MUTEX_DESTROY(); \
    TURN_SEM_DESTROY(); \
    ACTION_SEM_DESTROY(); \
    ARTIFACT_MUTEX_DESTROY(); \
} while(0)

#define SEM_CLOSE_ALL() 

#endif

inline int sem_timed_wait_sec(sem_t* sem, int timeout_sec) {
#ifdef __APPLE__
    int elapsed_ms = 0;
    int timeout_ms = timeout_sec * 1000;
    while (elapsed_ms < timeout_ms) {
        if (sem_trywait(sem) == 0) {
            return 0;  
        }
        usleep(50000);  
        elapsed_ms += 50;
    }
    return -1;  
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout_sec;
    
    int ret;
    do {
        ret = sem_timedwait(sem, &ts);
    } while (ret == -1 && errno == EINTR);
    
    return ret;  
#endif
}

