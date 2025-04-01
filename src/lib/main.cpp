#include "debug.hpp"
#include "dsm_lock.hpp"
#include "dsm_node.hpp"
#include "threadlib/cpu.h"
#include "threadlib/thread.h"
#include "util/lin_allocator.hpp"
#include <alloca.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <malloc.h>
#include <memory>
#include <sched.h>
#include <signal.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include <vector>

#define NUM_NODE 2

using namespace dsm;

int dsm_main(char *mem_region, size_t length, int argc, char *argv[]);
void dsm_main1(void *args);

extern char __bss_start;
extern int __private_region_start;

int main(int argc, char *argv[]) {
  int a;
  printf("stack begin at %lx\n", (intptr_t)&a);
  printf("mem region begin at %lx\n", (intptr_t)&__bss_start);
  char *mem_region = (char *)&__bss_start;
  char *mem_end = (char *)((intptr_t)&__bss_start + 45000 * PAGE_SIZE);
  brk(mem_end);
  pid_t child;
  size_t size = mem_end - mem_region;
  printf("mem size %lx, %lx\n", size, (intptr_t)mem_end);
  bool is_master = atoi(argv[1]) == 0;
  if (is_master) {
    pool.init();
  }
  // int x = 0;
  private_region *pr = (private_region *)&__private_region_start;
  pr->x = -1;
  dsm::total_page = 25000;
  if ((child = fork()) == 0) {
    ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
    // if (1) {
    printf("wait on x: %lx, %d\n", (intptr_t)(&pr->x), pr->x);
    while (pr->x == -1)
      ;
    printf("wait on x: %lx, %d\n", (intptr_t)(&pr->x), pr->x);
    printf("cpu id %lx\n", (intptr_t)&cpuid);
    cpuid = &pr->x;
    printf("start dsm main %lx\n", (intptr_t)&cpu_list);
    printf("start dsm main %lx\n", (intptr_t)cpu_list);
    if (is_master) {
      cpu_list = (cpu **)alloc(sizeof(cpu *) * NUM_NODE);
      printf("cpu list addr %lx\n", (intptr_t)cpu_list);
      cpu_list[pr->x] = make<cpu>();
      cpu_list[pr->x]->run(dsm_main1, nullptr);
    } else {
      printf("cpu list addr reference %lx\n", (intptr_t)&cpu_list);
      printf("cpu list addr %lx\n", (intptr_t)cpu_list);
      printf("x: %d\n", pr->x);
      cpu_list[pr->x] = make<cpu>();
      printf("cpu: %lx\n", (intptr_t)cpu_list[pr->x]);
      cpu_list[pr->x]->run(nullptr, nullptr);
    }
  } else {
    if (is_master) {
      printf("create master\n");
      NodeAddr addr;
      addr.ip = string(argv[3]);
      addr.port = stoi(argv[4]);
      dsm_init_master(child, addr, mem_region, size, &pr->x);
    } else {
      printf("create node\n");
      NodeAddr addr;
      addr.ip = string(argv[3]);
      addr.port = stoi(argv[4]);
      NodeAddr dst_addr;
      dst_addr.ip = string(argv[5]);
      dst_addr.port = stoi(argv[6]);
      dsm_init_node(child, addr, dst_addr, mem_region, size, &pr->x);
    }
  }
  while (1)
    ;
  return 0;
}
