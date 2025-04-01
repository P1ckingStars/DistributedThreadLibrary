/*
 * cpu.h -- interface to the simulated CPU
 *
 * This interface is used mainly by the thread library.
 * The only part that is used by application programs is cpu::boot().
 *
 * This class is implemented almost entirely by the infrastructure.  The
 * responsibilities of the thread library are to implement the cpu constructor
 * (and any functions you choose to add) and manage interrupt_vector_table
 * (and any variables you choose to add).
 *
 * You may add new variables and functions to this class.  If you add variables
 * to the cpu class, add them at the *end* of the class definition, and do not
 * exceed the 1 KB total size limit for the class.
 *
 * Do not modify any of the given variable and function declarations.
 */

#pragma once
#include "dsm_lock.hpp"
#ifndef CPU_HPP
#define CPU_HPP

#include "threadlib/thread.h"
#include "util/fixed_hashmap.hpp"
#include <atomic>
#include <queue>
#include <unordered_map>

using interrupt_handler_t = void (*)();

class syslock {
  dsm::dsm_mutex mu = 0;

public:
  void lock() { dsm::dsm_mutex_lock(&mu); }
  void unlock() { dsm::dsm_mutex_unlock(&mu); }
};

class cpu {
public:
  /*
   * cpu::boot() starts all CPUs in the system.  The number of CPUs is
   * specified by num_cpus.
   * One CPU will call cpu::cpu(func, arg); the other CPUs will call
   * cpu::cpu(nullptr, nullptr).
   *
   * On success, cpu::boot() does not return.
   *
   * async, sync, random_seed configure each CPU's generation of timer
   * interrupts (which are only delivered if interrupts are enabled).
   * Timer interrupts in turn cause the current thread to be preempted.
   *
   * The sync and async parameters can generate several patterns of interrupts:
   *
   *     1. async = true: generate interrupts asynchronously every 1 ms.
   *        These are non-deterministic.
   *
   *     2. sync = true: generate synchronous, pseudo-random interrupts on each
   *        CPU.  You can generate different (but repeatable) interrupt
   *        patterns by changing random_seed.
   *
   * An application will be deterministic if num_cpus=1 && async=false.
   */

  /*
   * interrupt_disable() disables interrupts on the executing CPU.  Any
   * interrupt that arrives while interrupts are disabled will be saved
   * and delivered when interrupts are re-enabled.
   *
   * interrupt_enable() and interrupt_enable_suspend() enable interrupts
   * on the executing CPU.
   *
   * interrupt_enable_suspend() atomically enables interrupts and suspends
   * the executing CPU until it receives an interrupt from another CPU.
   * The CPU will ignore timer interrupts while suspended.
   *
   * These functions should only be called by the thread library (not by
   * an application).  They are static member functions because they always
   * operate on the executing CPU.
   *
#include <memory>
   * Each CPU starts with interrupts disabled.
   */
  static void interrupt_disable();
  static void interrupt_enable();
  static void interrupt_enable_suspend();

  /*
   * interrupt_send() sends an inter-processor interrupt to the specified CPU.
   * E.g., c_ptr->interrupt_send() sends an IPI to the CPU pointed to by c_ptr.
   */
  void interrupt_send();

  /*
   * The interrupt vector table specifies the functions that will be called
   * for each type of interrupt.  There are two interrupt types: TIMER and
   * IPI.
   */
  static constexpr unsigned int TIMER = 0;
  static constexpr unsigned int IPI = 1;
  interrupt_handler_t interrupt_vector_table[IPI + 1];

  static cpu *self(); // returns pointer to the cpu that
                      // the calling thread is running on

  /*
   * The infrastructure provides an atomic guard variable, which thread
   * libraries should use to provide mutual exclusion on multiprocessors.
   * The switch invariant for multiprocessors specifies that this guard variable
   * must be true when calling swapcontext.  guard is initialized to false.
   */
  static syslock guard;

  /*
   * Disable the default copy constructor, copy assignment operator,
   * move constructor, and move assignment operator.
   */
  cpu(const cpu &) = delete;
  cpu &operator=(const cpu &) = delete;
  cpu(cpu &&) = delete;
  cpu &operator=(cpu &&) = delete;

  /*
   * The cpu constructor initializes a CPU.  It is provided by the thread
   * library and called by the infrastructure.  After a CPU is initialized, it
   * should run user threads as they become available.  If func is not
   * nullptr, cpu::cpu() also creates a user thread that executes func(arg).
   *
   * On success, cpu::cpu() should not return to the caller.
   */
  cpu();
  void run(thread_startfunc_t func, void *arg);

  /************************************************************
   * Add any variables you want here (do not add them above   *
   * interrupt_vector_table).  Do not exceed the 2 KB size    *
   * limit for this class.  Do not add any private variables. *
   ************************************************************/

  // ------------ Added variables and functions below --------------
  void setDone(char *stackptr, waitable *wait, ucontext_t *garbage);
  void setReady();
  void setWait(waitable *wait);
  void thread_handler();
  ucontext_t *garbageCtx;  // point to the context to be deallocated
  ucontext_t *mainContext; // Scheduler/master context
  ucontext_t *currContext; // current context
  int status;              // cpu status
  char *stackptr;
  waitable *wait;
  static std::queue<cpu *> cpus;
};

// define macro for lock and unlock
#define LOCK                                                                   \
  {                                                                            \
    DEBUG_STMT(printf("sys lock at: %s, %d\n", __FILE__, __LINE__));                       \
    cpu::guard.lock();                                                         \
  }
#define UNLOCK                                                                 \
  {                                                                            \
    DEBUG_STMT(printf("sys unlock at: %s, %d\n", __FILE__, __LINE__));                     \
    cpu::guard.unlock();                                                       \
  }

// a hashmap that stores ucontext_t pointer and thread id,
// which helps deallocate ucontext_t when it unnormally goes out of scope
extern FixedHashTable tid_map;

/*
 * assert_interrupts_disabled() and assert_interrupts_enabled() can be used
 * as error checks inside the thread library.  They will assert (i.e. abort
 * the program and dump core) if the condition they test for is not met.
 */
/*
 * assert_interrupts_private() is a private function for the interrupt
 * functions.  Your thread library should not call it directly.
 */

extern int *cpuid;
extern cpu **cpu_list;

#endif
