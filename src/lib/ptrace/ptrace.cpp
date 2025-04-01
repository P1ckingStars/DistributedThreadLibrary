#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#define IGNORED 0

#define ALIGN(x) ((x >> 12) << 12)

int x[4096 * 10];

void injection();
void injection3() { printf("how things works?\n"); }
void injection2() {
  // mprotect();
  int *xptr = x; // + 2 * 4096;
  mprotect((void *)ALIGN((intptr_t)xptr), 4096, PROT_WRITE | PROT_READ);
  printf("mprotect %lx, %d\n", (intptr_t)xptr, *xptr);
  int *c = 0;
  *c = 0;
  printf("shouldn't print\n");
}

void user_mprotect(pid_t pid, void *addr, int prot) {
  printf("user mprotect start\n");
  user_regs_struct regs;
  user_regs_struct saved_regs;
  ptrace(PTRACE_GETREGS, pid, NULL, &saved_regs);
  regs.rax = 10;
  regs.rdi = ALIGN((intptr_t)addr);
  regs.rsi = 4096;
  regs.rdx = prot;
  regs.rip = (intptr_t)injection;
  __ptrace_syscall_info info;
  int err;
  ptrace(PTRACE_SETREGS, pid, NULL, &regs);
  ptrace(PTRACE_CONT, pid, NULL, NULL);
  wait(NULL);
  siginfo_t sig;
  ptrace(PTRACE_GETSIGINFO, pid, NULL, &sig);
  // reg_err = ptrace(PTRACE_PEEKDATA, child, reg_err, &sig);
  printf("sig num: %d, addr: %lx, pc: %llx\n", sig.si_signo, sig.si_addr,
         saved_regs.rip);
  ptrace(PTRACE_SETREGS, pid, NULL, &saved_regs);
  printf("user mprotect end\n");
}

static void handler(int sig, siginfo_t *si, void *unused) {
  char top;
  mprotect((void *)ALIGN((intptr_t)si->si_addr), 4096, PROT_READ | PROT_WRITE);
  printf("addr: %lx, offset: %ld\n", (intptr_t)si->si_addr,
         (intptr_t)unused - (intptr_t)(&top));
  // if (si->si_addr == nullptr)
  exit(-1);
}

int main() {
  pid_t child = 0;
  uint64_t pc;
  child = fork();
  char stack[1024];
  if (child == 0) {
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
  } else {
    while (1) {
      wait(NULL);
      pc = ptrace(PTRACE_PEEKUSER, child, 8 * 16, NULL);
      uint64_t rsp = ptrace(PTRACE_PEEKUSER, child, 8 * 4, NULL);
      uint64_t inst = ptrace(PTRACE_PEEKTEXT, child, pc, NULL);
      siginfo_t sig;
      ptrace(PTRACE_GETSIGINFO, child, NULL, &sig);
      if (sig.si_signo == 11) {
        handler(11, &sig, nullptr);
      }
      // printf("sig num: %d, addr: %lx, pc: %llx, inst: %lx, rsp: %lx\n",
      //        sig.si_signo, sig.si_addr, saved_regs.rip, *(uint64_t *)pc,
      //        rsp);
      user_mprotect(child, sig.si_addr, PROT_READ | PROT_WRITE);
      ptrace(PTRACE_CONT, child, NULL, NULL);
    }
  }
  return 0;
}
