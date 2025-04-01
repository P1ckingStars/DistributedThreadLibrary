#ifndef USER_MPROTECT_HPP
#define USER_MPROTECT_HPP

#include "macros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <sys/types.h>

#define STOPPED 1
#define RUNNING 0

class ProcessState {
    pthread_mutex_t mu;
    bool stopped = false;
public:
    void init() {
        pthread_mutex_init(&mu, NULL);
    }
    void stop() {
        pthread_mutex_lock(&mu);
        ASSERT((stopped == RUNNING), "The process should run but didn't");
        stopped = STOPPED;
        pthread_mutex_unlock(&mu);
    }
    void resume() {
        pthread_mutex_lock(&mu);
        ASSERT((stopped == STOPPED), "The process should stop but didn't");
        stopped = RUNNING;
        pthread_mutex_unlock(&mu);
    }
    bool get_state() {
        return stopped;
    }
    void lock_state() {
        pthread_mutex_lock(&mu);
    }
    void unlock_state() {
        pthread_mutex_unlock(&mu);
    }
};

extern ProcessState process_state_info;

ssize_t remote_mempage_read(pid_t pid, char *local, char *remote);
ssize_t remote_mempage_write(pid_t pid, char *local, char *remote);
void user_mprotect_init();
void user_mprotect_req(pid_t pid, void *addr, size_t size, int prot, bool read_page_flag, char * page);
void user_mprotect_respond();
void user_mprotect(pid_t pid, void *addr, size_t size, int prot, bool read_page_flag, char *page);
inline void user_mprotect(pid_t pid, void *addr, size_t size, int prot) {
    user_mprotect(pid, addr, size, prot, 0, 0);
}

#endif
