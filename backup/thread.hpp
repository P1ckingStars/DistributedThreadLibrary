#ifndef THREAD_HPP
#define THREAD_HPP

#include "dsm_lock.hpp"
#include "macros.hpp"

#include <deque>
#include <sys/ucontext.h>
#include <ucontext.h>

namespace dsm {

typedef int (*funcptr)(void *);

struct ThreadPack {
  bool valid;
  funcptr fp;
  void *args;
  ucontext_t *ctx;
  char *stack;
};

class Thread {
  static class {
    dsm_mutex mu = 0;
    int x;

  public:
    void inc() {
      dsm_mutex_lock(&mu);
      x++;
      dsm_mutex_unlock(&mu);
    }
    void dec() {
      dsm_mutex_lock(&mu);
      x--;
      dsm_mutex_unlock(&mu);
    }
    int get_x() {
      dsm_mutex_lock(&mu);
      int res = x;
      dsm_mutex_unlock(&mu);
      return res;
    }
  } counter;

public:
  static void entry();
  Thread(funcptr fp, void *arg);
  static int get_num_living_thread() { return counter.get_x(); }
};

class ThreadQueue {
  dsm_mutex mu = 0;
  dsm_mutex pack_mu = 0;
  std::deque<ucontext_t *> threads;
  ucontext_t *temp;
  ucontext_t *pop();
  ThreadPack pack_ = {0, 0, 0, 0, 0};
  void clean_req(ThreadPack const &pack);
  void clean_rsps();

public:
  void push(ucontext_t *ctx);
  void swap();
  void run_next(ThreadPack const &pack);
  void thread_entry_inv();
};

extern ThreadQueue ready_queue;

} // namespace dsm

#endif
