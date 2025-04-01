/*
 * mutex.h -- interface to the mutex class
 *
 * You may add new variables and functions to this class.
 *
 * Do not modify any of the given function declarations.
 */

#pragma once

#include <sys/ucontext.h>
#include <ucontext.h>
#include "waitable.h"

#define INUSE true
#define FREE  false

class mutex: public waitable {
public:
    mutex();
    ~mutex();

    void lock();
    void unlock();
    void privileged_lock();
    void privileged_unlock();
    bool status();
    
    /*
     * Disable the copy constructor and copy assignment operator.
     */
    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;

    /*
     * Move constructor and move assignment operator.  Implementing these is
     * optional in Project 2.
     */
    mutex(mutex&&);
    mutex& operator=(mutex&&);

private:
    bool testAndSet();
    uint64_t owner_id; // id of thread that owns this lock
    bool isLocked = false; // lock state
};
