
#ifndef DSM_LOCK_HPP
#define DSM_LOCK_HPP

#include <cstddef>
namespace dsm {

extern size_t total_page;

typedef int dsm_mutex;

inline int
xchgl(volatile int *addr, int newval)
{   
  int result;
  asm volatile("lock; xchgl %0, %1" :
               "+m" (*addr), "=a" (result) :
               "1" (newval) :
               "cc");
  return result;
}

void sync();

void dsm_mutex_lock(dsm_mutex * mu);
void dsm_mutex_unlock(dsm_mutex * mu);

class Guard {
    dsm_mutex * mu_;
public:
    Guard(dsm_mutex * mu): mu_(mu) {
        dsm_mutex_lock(mu);
    }
    ~Guard() {
        dsm_mutex_unlock(mu_);
    }
};

}

#endif
