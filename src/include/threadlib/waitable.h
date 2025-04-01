
#include <sys/ucontext.h>
#include <ucontext.h>
#include <memory>
#include "queue.hpp"

#ifndef WAITABLE_H
#define WAITABLE_H

/**
 * waitable class contains a uncontext_t pointers queue and related functions
*/
class waitable {
protected:
    Queue<ucontext_t*> waitingQueue;
public:
    /**
     * @param ctx a ucontext_t pointer which need to be enqueued
    */
    inline void enqueue(ucontext_t *ctx)
    {
        waitingQueue.enqueue(ctx);
    }

    /**
     * @return true if the queue is empty, or false otherwise
    */
    inline bool empty()
    {
        return waitingQueue.isEmpty();
    }

    /**
     * pop the front ucontext_t pointer out of the queue
     * @return the front ucontext_t pointer in the queue that popped out
    */
    inline ucontext_t *dequeue()
    {
        auto ctx = waitingQueue.front();
        waitingQueue.dequeue();
        return ctx;
    }
};

#endif
