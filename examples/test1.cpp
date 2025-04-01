#include "dsm_lock.hpp"
#include "queue.hpp"
#include "threadlib/cpu.h"
#include "threadlib/cv.h"
#include "threadlib/mutex.h"
#include "threadlib/thread.h"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <unistd.h>

using std::cout;
using std::endl;

mutex bufferMutex;
cv bufferNotEmpty;
cv bufferNotFull;
Queue<int> buffer;
const size_t bufferSize = 2;
bool x = 1;

void producer(void *arg) {
  char stack_top = 0;
  printf("PRODUCER STACK %lx\n", (intptr_t)&stack_top);
  while (x) {
    dsm::sync();
    printf("x %lx has been set to %d\n", (intptr_t)&x, x);
    sleep(1);
  }
  sleep(1);
  printf("run loop\n");
  for (int i = 0; i < 3; ++i) {
    printf("lock status: %d\n", bufferMutex.status());
    bufferMutex.lock();
    cout << "lock status after aquire: " << bufferMutex.status() << endl;
    while (buffer.size() == bufferSize) {
      printf("producer wait on mutex %lx\n", (intptr_t)&bufferMutex);
      bufferNotFull.wait(bufferMutex);
    }
    cout << "enqueue: " << i << endl;
    buffer.enqueue(i);
    cout << "Produced: " << i << endl;
    bufferNotEmpty.signal();
    bufferMutex.unlock();
  }
}

void consumer(void *arg) {
  char stack_top = 0;
  printf("CONSUMER STACK %lx\n", (intptr_t)&stack_top);
  x = 0;
  printf("x %lx has been set to %d\n", (intptr_t)&x, x);
  for (int i = 0; i < 3; ++i) {
    bufferMutex.lock();
    while (buffer.isEmpty()) {
      printf("consumer wait on mutex %lx\n", (intptr_t)&bufferMutex);
      bufferNotEmpty.wait(bufferMutex);
    }
    printf("front\n");
    int item = buffer.front();
    printf("deque\n");
    buffer.dequeue();
    cout << "Consumed: " << item << endl;
    bufferNotFull.signal();
    bufferMutex.unlock();
  }
}

void dsm_main1(void *arg) {
  printf("---------------run user code now-------------\n");
  thread prod(producer, nullptr);
  thread cons(consumer, nullptr);
  prod.join();
  cons.join(); // In a real scenario, you might need a way to stop the consumer
               // thread gracefully.
  printf("complete!!!\n");
}
