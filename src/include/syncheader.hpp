#ifndef SYNC_HPP
#define SYNC_HPP

#include <pthread.h>

#include "config.hpp"

// #define SHOW_LOCK
#ifdef SHOW_LOCK

#define LOCK(mu)                                                               \
  {                                                                            \
    printf("try lock at %s::%d\n", __FILE__, __LINE__);                        \
    pthread_mutex_lock(&(mu));                                                 \
  }
#define UNLOCK(mu)                                                             \
  {                                                                            \
    printf("unlock at %s::%d\n", __FILE__, __LINE__);                          \
    pthread_mutex_unlock(&(mu));                                               \
  }
#define WAIT(cond, mu)                                                         \
  {                                                                            \
    printf("wait at %s::%d\n", __FILE__, __LINE__);                            \
    pthread_cond_wait(&(cond), &mu);                                           \
  }
#define SIGNAL(cond)                                                           \
  {                                                                            \
    printf("signal at %s::%d\n", __FILE__, __LINE__);                          \
    pthread_cond_signal(&(cond));                                              \
  }
#else
#define LOCK(mu)            { pthread_mutex_lock(&(mu)); }
#define UNLOCK(mu)          { pthread_mutex_unlock(&(mu)); }
#define WAIT(cond, mu)      { pthread_cond_wait(&(cond), &mu); }
#define SIGNAL(cond)        { pthread_cond_signal(&(cond)); }
#endif

#endif
