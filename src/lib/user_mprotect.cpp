
#include "user_mprotect.hpp"
#include "debug.hpp"
#include "macros.hpp"
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <sched.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>

#define REQ_INCOMPLETE 1
#define REQ_COMPLETE 0

ssize_t remote_mempage_write(pid_t pid, char *local, char *remote) {
  struct iovec liov[1];
  struct iovec riov[1];
  liov[0].iov_base = local;
  liov[0].iov_len = PAGE_SIZE;
  riov[0].iov_base = remote;
  riov[0].iov_len = PAGE_SIZE;
  DEBUG_STMT(printf("!!!WRITE at addr %lx\n", riov[0].iov_base));
  int res = process_vm_writev(pid, liov, 1, riov, 1, 0);
  if (res == -1) {
    printf("ERROR when vm write %lx\n", (intptr_t)remote);
    exit(-1);
  }
  return res;
}
ssize_t remote_mempage_read(pid_t pid, char *local, char *remote) {
  struct iovec liov[1];
  struct iovec riov[1];
  liov[0].iov_base = local;
  liov[0].iov_len = PAGE_SIZE;
  riov[0].iov_base = remote;
  riov[0].iov_len = PAGE_SIZE;
  int res = process_vm_readv(pid, liov, 1, riov, 1, 0);
  if (res == -1) {
    printf("ERROR when vm read %lx\n", (intptr_t)remote);
    exit(-1);
  }
  return res;
}

class {
  pthread_mutex_t mu_;
  pid_t pid_;
  void *addr_;
  size_t size_;
  int prot_;
  uint8_t status_ = REQ_COMPLETE;
  bool read_page_flag_ = 0;
  char *page_ = 0;

public:
  void init() { pthread_mutex_init(&mu_, NULL); }
  bool empty() { return status_ == REQ_COMPLETE; }
  void produce(pid_t pid, void *addr, size_t size, int prot,
               bool read_page_flag, char *page) {
    pthread_mutex_lock(&mu_);
    this->pid_ = pid;
    this->addr_ = addr;
    this->size_ = size;
    this->prot_ = prot;
    this->status_ = REQ_INCOMPLETE;
    this->read_page_flag_ = read_page_flag;
    this->page_ = page;
  }
  void consume(pid_t *pid, void **addr, size_t *size, int *prot,
               bool *read_page_flag, char **page) {
    *pid = this->pid_;
    *addr = this->addr_;
    *size = this->size_;
    *prot = this->prot_;
    *read_page_flag = this->read_page_flag_;
    *page = this->page_;
  }
  void compelete() { this->status_ = REQ_COMPLETE; }
  void wait_to_compelete(pid_t pid) {
    int i = 0;
    while (this->status_ == REQ_INCOMPLETE) {
      if ((i++)==10000000) {
        kill(pid, SIGUSR2);
        i = 0;
      }
    }
    pthread_mutex_unlock(&mu_);
  }
} mprotect_req;

void injection();
void injection2() {
  printf("inject\n");
  int *x = 0;
  int y = *x;
}

void user_mprotect_init() { mprotect_req.init(); }

void user_mprotect_req(pid_t pid, void *addr, size_t size, int prot,
                       bool read_page_flag, char *page) {
  DEBUG_STMT(printf("try user mprotect\n"));
  mprotect_req.produce(pid, addr, size, prot, read_page_flag, page);
  DEBUG_STMT(printf("rsps sent\n"));
  kill(pid, SIGUSR2);
  DEBUG_STMT(printf("wait to complete\n"));
  mprotect_req.wait_to_compelete(pid);
}

void user_mprotect_respond() {
  if (mprotect_req.empty()) {
    return;
  }
  pid_t pid;
  void *addr;
  size_t size;
  int prot;
  bool read_page_flag;
  char *page;
  mprotect_req.consume(&pid, &addr, &size, &prot, &read_page_flag, &page);
  user_mprotect(pid, addr, size, prot, read_page_flag, page);
  mprotect_req.compelete();
  DEBUG_STMT(printf("complete\n"));
}

void user_mprotect(pid_t pid, void *addr, size_t size, int prot,
                   bool read_page_flag, char *page) {
  DEBUG_STMT(printf("user mprotect BEGIN at ADDR: %lx, PROT: %d\n",
                    (intptr_t)addr, prot));
  user_regs_struct regs;
  user_regs_struct saved_regs;
  user_fpregs_struct saved_fp_regs;
  ptrace(PTRACE_GETREGS, pid, 0, &saved_regs);
  ptrace(PTRACE_GETFPREGS, pid, 0, &saved_fp_regs);
  bzero(&regs, sizeof(regs));
  regs.rax = 10;
  regs.rdi = (intptr_t)addr;
  regs.rsi = size;
  regs.rdx = prot;
  regs.rip = (intptr_t)injection;
  int err = ptrace(PTRACE_SETREGS, pid, NULL, &regs);
  // if (err != 0) {
  //   perror("ptrace");
  // }
  ptrace(PTRACE_CONT, pid, NULL, NULL);
  DEBUG_STMT(printf("continue\n"));
  wait(NULL);
  siginfo_t sig;
  ptrace(PTRACE_GETSIGINFO, pid, NULL, &sig);
  DEBUG_STMT(printf("user mprotect recv sig: %d\n",
                    sig.si_signo)); // TODO BUG showed in the output here, when
                                    // it get SIGUSR instead
  if (sig.si_signo != SIGSEGV) {
    ptrace(PTRACE_CONT, pid, NULL, NULL);
    wait(NULL);
    kill(pid, sig.si_signo);
  }
  // reg_err = ptrace(PTRACE_PEEKDATA, child, reg_err, &sig);
  ptrace(PTRACE_SETREGS, pid, 0, &saved_regs);
  ptrace(PTRACE_SETFPREGS, pid, 0, &saved_fp_regs);
  DEBUG_STMT(
      printf("user mprotect FINISHED at ADDR: %lx, PROT: %d, PROT read: %d\n",
             (intptr_t)addr, prot, PROT_READ));
  if (read_page_flag) {
    remote_mempage_read(pid, page, (char *)addr);
  }
}
