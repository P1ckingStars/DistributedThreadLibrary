#include "threadlib/cpu.h"
#include "threadlib/cv.h"
#include "threadlib/schedulerState.h"
#include <cstdint>
#include <cstdio>

cv::cv() {}
cv::~cv() {}

/**
 * @param mu mutex that this cv would wait on
*/
void cv::wait(mutex &mu) {
    int a;
    printf("current stack address before: %lx\n", (intptr_t)&a);
    cpu::interrupt_disable();
    LOCK
    try {
        mu.privileged_unlock();
    } catch (std::exception) {
        UNLOCK
        cpu::interrupt_enable();
        throw;
    }
    // waitSelfOn calls switch context, push self on the waiting queue,
    // and run the next thread on the ready queue,
    // or switch to main context if ready queue is empty
    SchedulerState::scheduler.waitSelfOn(&this->q);
    UNLOCK
    cpu::interrupt_enable();
    mu.lock();
    printf("current stack address after: %lx\n", (intptr_t)&a);
}

/**
 * wake up one thread on the waiting queue
*/
void cv::signal() {
    cpu::interrupt_disable();
    LOCK
    // wakeOne function pops one thread from the waiting queue
    // and push it to the back of the ready queue
    SchedulerState::scheduler.wakeOne(&this->q);
    UNLOCK
    cpu::interrupt_enable();
}

/**
 * wake up all the threads on the waiting queue
*/
void cv::broadcast() {
    cpu::interrupt_disable();
    LOCK
    // wakeAll function pops all threads from the waiting queue
    // and push them to the back of the ready queue
    SchedulerState::scheduler.wakeAll(&this->q);
    UNLOCK
    cpu::interrupt_enable();
}
