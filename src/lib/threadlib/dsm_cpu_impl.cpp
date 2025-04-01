#include "debug.hpp"
#include "threadlib/cpu.h"
#include <cstdint>
#include <cstdio>


syslock cpu::guard;

int * cpuid;
cpu ** cpu_list;

void cpu::interrupt_disable() {}
void cpu::interrupt_enable() {}
void cpu::interrupt_enable_suspend() {}
void cpu::interrupt_send() {}
cpu *cpu::self() {
    DEBUG_STMT(printf("CPU SELF %lx, %lx, %d\n", (intptr_t)cpu_list, (intptr_t)cpuid, *cpuid));
    DEBUG_STMT(printf("CPU SELF %lx, %lx, %d, %lx\n", (intptr_t)cpu_list, (intptr_t)cpuid, *cpuid, (intptr_t)cpu_list[*cpuid]));
    return cpu_list[*cpuid];
}
