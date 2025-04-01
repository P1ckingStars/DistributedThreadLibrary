#include "cpu.h"
#include "debug.hpp"
#include "queue.hpp"
#include "waitable.h"
#include <cstdint>
#include <cstdio>
#include <sys/ucontext.h>
#include <ucontext.h>

#ifndef SCHEDULER_STATE_H
#define SCHEDULER_STATE_H
#define SS "SchedulerState.cpp"

/**
 * send ipi if there's at least one cpu running
 */
inline void ipi_send() {
  if (!cpu::cpus.empty()) {
    cpu::cpus.front()->interrupt_send();
  }
}

/**
 * set cpu current context to nullptr if next is main context,
 * or set to next otherwise, then call setcontext to run next
 */
inline void ctx_set(ucontext_t *next) {
  cpu::self()->currContext = next == cpu::self()->mainContext ? nullptr : next;
  setcontext(next);
}

/**
 * set cpu current context to nullptr if next is main context,
 * or set to next otherwise, then call swap context to run next.
 * call thread handler to make sure deallocate in time
 * if the thread finishes executions
 */
inline void ctx_switch(ucontext_t *current, ucontext_t *next) {
  DEBUG_STMT(printf("ctx_switch %lx, %lx\n", (intptr_t)&cpu::self()->currContext,
         (intptr_t)&cpu::self()->mainContext));
  cpu::self()->currContext = next == cpu::self()->mainContext ? nullptr : next;
  DEBUG_STMT(printf("%lx, %lx\n", (intptr_t)current, (intptr_t)next));
  swapcontext(current, next);
  cpu::self()->thread_handler();
}

/**
 * a class of scheduler
 */
class SchedulerState {
  Queue<ucontext_t *> readyQueue;

  /**
   * this function pop the next thread on the ready queue
   * @return next ready thread
   */
  ucontext_t *getNext() {
    auto currContext = readyQueue.front();
    readyQueue.dequeue();
    printf("DEQUEUE %lx\n", (intptr_t)currContext);
    if (hasNext())
      ipi_send();
    return currContext;
  }

public:
  /**
   * suspend running main context,
   * run next thread on the ready queue if it's not empty
   */
  void runNextFromKernel() {
    DEBUG_STMT(printf("RUN NEXT FORM KERNEL %lx\n", (intptr_t)this));
    LOCK;
    if (this->hasNext()) {
      DEBUG_STMT(printf("CPU %lx\n", (intptr_t)cpu::self()));
      ucontext_t *nextContext = this->getNext();
      DEBUG_STMT(printf("SWAP %lx, %lx\n", (intptr_t)cpu::self()->mainContext,
                        (intptr_t)nextContext));
      ctx_switch(cpu::self()->mainContext, nextContext);
    }
    UNLOCK
    DEBUG_STMT(printf("RUN MAIN CTX\n"));
  }

  /**
   * run next thread on the ready queue if it's not empty,
   * or run main context otherwise
   */
  void runNextFromUser() {
    LOCK;
    if (this->hasNext()) {
      ucontext_t *nextContext = this->getNext();
      ctx_set(nextContext);
    } else {
      ctx_set(cpu::self()->mainContext);
    }
    UNLOCK
  }

  /**
   * push the given context to the back of the ready queue,
   * send ipi to efficiently use cpu
   * @param ctx the context that need to be push to the ready queue
   */
  void putInReady(ucontext_t *ctx) {
    DEBUG_STMT(printf("TRY ADD NEW CTX TO QUEUE\n"));
    printf("ENQUEUE %lx\n", (intptr_t)ctx);
    readyQueue.enqueue(ctx);
    DEBUG_STMT(printf("ADDED NEW CTX TO QUEUE %d\n", readyQueue.isEmpty()));
    DEBUG_STMT(printf("ADDED NEW CTX TO QUEUE %d\n", hasNext()));
    ipi_send();
  }

  /**
   * run the next thread on the ready queue if it's not empty,
   * put self on the the back of the ready queue
   */
  void putSelfInReady() {
    LOCK;
    if (this->hasNext() && cpu::self()->currContext) {
      ucontext_t *nextContext = this->getNext();
      scheduler.putInReady(cpu::self()->currContext);
      ctx_switch(cpu::self()->currContext, nextContext);
    }
    UNLOCK
  }

  /**
   * run the next thread on the ready queue if it's not empty,
   * or switch back to the main context if the ready queue is empty,
   * and add self to the waiting queue of the mutex
   * @param wait the waiting queue of the mutex
   */
  void waitSelfOn(waitable *wait) {
    if (this->hasNext()) {
      ucontext_t *nextContext = this->getNext();
      wait->enqueue(cpu::self()->currContext);
      DEBUG_STMT(printf("swap with next\n"));
      ctx_switch(cpu::self()->currContext, nextContext);
    } else {
      wait->enqueue(cpu::self()->currContext);
      DEBUG_STMT(printf("swap with kernel\n"));
      ctx_switch(cpu::self()->currContext, cpu::self()->mainContext);
    }
  }

  /**
   * if the waiting queue of the mutex is not empty,
   * pop one thread from the waiting queue,
   * and push it to the back of the ready queue
   */
  void wakeOne(waitable *wait) {
    if (!wait->empty()) {
      putInReady(wait->dequeue());
    }
  }

  /**
   * if the waiting queue of the mutex is not empty,
   * pop all threads from the waiting queue,
   * and push them to the back of the ready queue
   */
  void wakeAll(waitable *wait) {
    while (!wait->empty()) {
      putInReady(wait->dequeue());
    }
  }

  /**
   * @return true if ready queue is not empty, or false otherwise.
   */
  bool hasNext() {
    DEBUG_STMT(printf("has next\n"));
    auto res = !readyQueue.isEmpty();
    DEBUG_STMT(printf("done has next\n"));
    return res;
  }

  static SchedulerState scheduler; // declare a scheduler object
};

#endif
