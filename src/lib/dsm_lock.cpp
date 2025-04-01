
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cwctype>
#include <sched.h>
#include <sys/mman.h>

#include "dsm_lock.hpp"
#include "macros.hpp"

using namespace dsm;

size_t dsm::total_page;
extern char __bss_start;

void dsm::sync() {
    mprotect(&__bss_start, total_page * PAGE_SIZE, PROT_NONE);
    char a = __bss_start;
}

inline bool test_and_set(int * mu) {
    int test = 1;
    return xchgl(mu, test);
}

void dsm::dsm_mutex_lock(dsm_mutex * mu) {
    while (test_and_set(mu));
    sync();   
}

void dsm::dsm_mutex_unlock(dsm_mutex * mu) {
    *mu = 0;
}

