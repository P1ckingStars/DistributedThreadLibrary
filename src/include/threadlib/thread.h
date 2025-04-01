/*
 * thread.h -- interface to the thread library
 *
 * This file should be included by the thread library and by application
 * programs that use the thread library.
 * 
 * You may add new variables and functions to this class.
 *
 * Do not modify any of the given function declarations.
 */
#include <cstddef>
#include <sys/ucontext.h>
#include <ucontext.h>
#include "debug.hpp"
#include "macros.hpp"
#include "waitable.h"
#include "linker_symbol.hpp"

#pragma once

#define TOTAL_STACK_PAGES 10000
#define PAGES_PER_STACK 250
#define TOTAL_POSSIBLE_STACKS (TOTAL_STACK_PAGES/PAGES_PER_STACK)

extern int total_threads;

//static constexpr unsigned int STACK_SIZE=4096; //262144; // size of each thread's stack in bytes

using thread_startfunc_t = void (*)(void*);

class stack_pool {
    char * stacks[TOTAL_POSSIBLE_STACKS];
    int tail;
public:
    void init() {
        DEBUG_STMT(printf("stack init\n"));
        tail = TOTAL_POSSIBLE_STACKS;
        char * mem = STACK_START;
        for (int i = 0; i < TOTAL_POSSIBLE_STACKS; i++) {
            stacks[i] = mem + i * PAGES_PER_STACK * PAGE_SIZE;
        }
    }
    void push(char * addr) {
        ASSERT(tail < TOTAL_POSSIBLE_STACKS, "More stacks than expected");
        stacks[tail++] = addr;
    }
    char * pop() {
        if (tail == 0) return nullptr;
        return stacks[--tail];
    }
    size_t size() {
        return TOTAL_POSSIBLE_STACKS-tail;
    }
};

extern stack_pool pool;
/**
 * a class of shared boolean to store the information that if its owner is alive
*/
class shared_bool {
public:
    bool ownedByStack;
    bool ownedByThread;
    bool val;
};

class thread {
public:
    uint64_t tid; // a unique thread id for each thread
    shared_bool *isDead; // store the information that if this thread is alive
    waitable *wait; // a waitable object that stores other threads
                    // that wait for this thread to exit
    thread(thread_startfunc_t func, void* arg); // create a new thread
    ~thread();

    void join();                                // wait for this thread to finish

    static void yield();                        // yield the CPU
    //static void swapToMaster();

    /*
     * Disable the copy constructor and copy assignment operator.
     */
    thread(const thread&) = delete;
    thread& operator=(const thread&) = delete;

    /*
     * Move constructor and move assignment operator.  Implementing these is
     * optional in Project 2.
     */
    thread(thread&&);
    thread& operator=(thread&&);
};
