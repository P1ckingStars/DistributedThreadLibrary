#include "thread.hpp"
#include "dsm_lock.hpp"
#include "macros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <sys/ucontext.h>
#include <ucontext.h>

using namespace dsm;

ThreadQueue read_queue;

void ThreadQueue::push(ucontext_t *ctx) {
  Guard lock(&this->mu);
  this->threads.push_back(ctx);
}

void ThreadQueue::swap() {
  dsm_mutex_lock(&this->mu);
  if (this->threads.empty()) {
    dsm_mutex_unlock(&this->mu);
    return;
  }
  auto next = this->threads.front();
  this->threads.pop_front();
  auto oucp = temp;
  temp = next;
  this->threads.push_back(oucp);
  swapcontext(oucp, next);
  this->thread_entry_inv();
}

void ThreadQueue::clean_req(ThreadPack const &pack) {
  dsm_mutex_lock(&this->pack_mu);
  while (pack_.valid) {
    dsm_mutex_unlock(&this->pack_mu);
    dsm_mutex_lock(&this->pack_mu);
  }
  this->pack_ = pack;
  dsm_mutex_unlock(&this->pack_mu);
}

void ThreadQueue::clean_rsps() {
  Guard g(&this->pack_mu);
  if (pack_.valid) {
    delete pack_.stack;
    delete pack_.ctx;
    pack_.valid = false;
  }
}

void ThreadQueue::run_next(ThreadPack const &pack) {
  clean_req(pack);
  if (auto ctx = this->pop()) {
    this->
  }
}

ucontext_t *ThreadQueue::pop() {
  while (Thread::get_num_living_thread()) {
    Guard lock(&this->mu);
    if (!this->threads.empty()) {
      ucontext_t *ctx = threads.front();
      threads.pop_front();
      return ctx;
    }
  }
  return nullptr;
}

void Thread::entry() {
  ThreadPack pack;
  pack.fp(pack.args);
  counter.dec();
  ready_queue.run_next(pack);
}

void ThreadQueue::thread_entry_inv() {
  dsm_mutex_unlock(&this->mu);
  this->clean_rsps();
}

Thread::Thread(funcptr fp, void *args) {
  ucontext_t *ctx = new ucontext_t;
  char *stack = new char[STACK_SIZE];
  getcontext(ctx);
  intptr_t addr = (intptr_t)stack;
  addr += STACK_SIZE - sizeof(intptr_t) -
          sizeof(ThreadPack); // Stack push rbp, push ThreadPack
  *(ThreadPack *)addr = ThreadPack{1, fp, args, ctx, stack};
  ctx->uc_stack.ss_sp = stack;
  ctx->uc_stack.ss_size = STACK_SIZE;
  ctx->uc_link = NULL;
  makecontext(ctx, entry, 0);
  read_queue.push(ctx);
  counter.inc();
}
