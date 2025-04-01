#include "threadlib/cpu.h"
#include "debug.hpp"
#include "threadlib/schedulerState.h"
#include "threadlib/thread.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <unistd.h>

// define four states for each thread
#define END_STATE -1
#define NONE 0
#define READY_STATE 1
#define WAIT_STATE 2

// a hashmap that stores ucontext_t pointer and thread id,
// which helps deallocate ucontext_t when it unnormally goes out of scope
FixedHashTable tid_map; // TODO: Replace std hash map

/**
 * this time interrupt function only calls yield
 */
void time_interrupt() { thread::yield(); }

void ipc_interrupt() {}

/**
 * this function deallocate thread's context if it finishes executions
 */
void cpu::thread_handler() {
  if (this->status == END_STATE) {
    tid_map.remove((intptr_t)this->garbageCtx);
    pool.push(stackptr);
    dealloc(this->garbageCtx);
    dealloc(this->wait);
    total_threads--;
  } // else prev is in SchedulerState::scheduler state
  this->status = NONE;
}

/**
 * cpu constructor
 * @param func the function which the new thread will run
 * @param arg the arguments of the function
 */
cpu::cpu() {}
void cpu::run(thread_startfunc_t func, void *arg) {
  printf("start running thread lib\n");
  interrupt_vector_table[0] = time_interrupt;
  interrupt_vector_table[1] = ipc_interrupt;

  // initialize cpu context field
  this->currContext = nullptr;
  this->garbageCtx = nullptr;

  // the special CPU that starts the initial thread
  if (func != nullptr) {
    cpu::interrupt_enable();
    try {
      thread(func, arg);
    } catch (std::bad_alloc) {
      cpu::interrupt_enable();
      throw;
    }
    cpu::interrupt_disable();
  }
  LOCK UNLOCK SchedulerState::scheduler.hasNext();
  DEBUG_STMT(printf("ALLOCATE MAIN\n"));
  LOCK SchedulerState::scheduler.hasNext();
  DEBUG_STMT(printf("RUN AS LOCKED\n"));
  UNLOCK
  // allocate main context
  try {
    this->mainContext = make<ucontext_t>();
  } catch (std::bad_alloc) {
    if (this->mainContext)
      dealloc(this->mainContext);
    cpu::interrupt_enable();
    throw;
  }
  DEBUG_STMT(printf("BEFORE RUN SCHEDULER\n"));
  LOCK SchedulerState::scheduler.hasNext();
  UNLOCK
  // cpu scheduling
  while (1) {
    DEBUG_STMT(printf("RUN SCHEDULER\n"));
    this->status = NONE;
    // run next ready thread on ready queue
    SchedulerState::scheduler.runNextFromKernel();
    DEBUG_STMT(printf("RUN THREAD HANDLER\n"));
    cpu::thread_handler();
    // if the ready queue is empty and cpu is idle
    if (!SchedulerState::scheduler.hasNext()) {
      if (total_threads == 0) {
        printf("PROGRAM DONE\n");
        while(1);
      }
      sleep(1);
      // LOCK
      // // push self back on the cpu queue
      // cpu::cpus.push(this);
      // UNLOCK
      // // enable interrupt and suspend this cpu
      // cpu::interrupt_enable_suspend();
      // cpu::interrupt_disable();
      // LOCK
      // cpu::cpus.pop();
      // UNLOCK
    }
  }
  exit(-1);
}

std::queue<cpu *> cpu::cpus = std::queue<cpu *>(); // initialize cpu queue

/**
 * this function set the cpu done state and update the data field
 */
void cpu::setDone(char *stackptr, waitable *wait, ucontext_t *garbage) {
  this->stackptr = stackptr;
  this->garbageCtx = garbage;
  this->status = END_STATE;
  this->wait = wait;
}

/**
 * this function set the cpu wait state
 */
void cpu::setWait(waitable *wait) {
  this->status = WAIT_STATE;
  this->wait = wait;
}

/**
 * this function set the cpu ready state
 */
void cpu::setReady() { this->status = READY_STATE; }
