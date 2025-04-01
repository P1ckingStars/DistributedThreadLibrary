#include "dsm_node.hpp"
#include "debug.hpp"
#include "linker_symbol.hpp"
#include "macros.hpp"
#include "rpc/client.h"
#include "rpc/rpc_error.h"
#include "syncheader.hpp"
#include "threadlib/thread.h"
#include "user_mprotect.hpp"
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/ucontext.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace dsm;

#define RPC_HAND_SHAKE "hand_shake"
#define RPC_JOIN "join"
#define RPC_READ "read"
#define RPC_WRITE "write"

#define DSM_PROT_READ 0b1
#define DSM_PROT_WRITE 0b111
#define DSM_PROT_OWNED 0b101
#define DSM_PROT_INVALID 0

#define DSM_PROT_CHECK(INFO, PROT) (((INFO) & PROT) == PROT)
#define READABLE(pinfo) DSM_PROT_CHECK(pinfo, DSM_PROT_READ)
#define WRITABLE(pinfo) DSM_PROT_CHECK(pinfo, DSM_PROT_WRITE)
#define OWNERSHIP(pinfo) DSM_PROT_CHECK(pinfo, DSM_PROT_OWNED)

#define FLOOR(addr) ((addr) / PAGE_SIZE * PAGE_SIZE)

static DSMNode *dsm_singleton;

static struct sigaction old_sa;

static void handler(int sig, siginfo_t *si, void *unused) {
  DEBUG_STMT(printf("This %lx RUN handler at address %lx\n",
                    (intptr_t)dsm_singleton, (intptr_t)si->si_addr));
  if (!dsm_singleton->is_in_range((char *)si->si_addr)) {
    printf("Not in range: 0x%lx\n", (long)si->si_addr);
    exit(-1);
  }
  int prot = dsm_singleton->prot_check(si->si_addr);
  bool is_write = prot & DSM_PROT_READ;
  if (OWNERSHIP(prot)) {
    DEBUG_STMT(printf("owned: 0x%lx\n", (long)si->si_addr));
    dsm_singleton->update_prot(si->si_addr);
    return;
  }
  DEBUG_STMT(printf("Got SIGSEGV at address: 0x%lx\n", (long)si->si_addr));
  if (!dsm_singleton->is_in_range((char *)si->si_addr)) {
    DEBUG_STMT(printf("address: 0x%lx is not in range\n", (long)si->si_addr));
    old_sa.sa_sigaction(sig, si, unused);
  }
  if (is_write) {
    DEBUG_STMT(printf("GRANT write at address %lx, %ld\n",
                      (intptr_t)si->si_addr, std::time(0)));
    dsm_singleton->grant_write((char *)si->si_addr);
  } else {
    DEBUG_STMT(printf("GRANT read at address %lx, %ld\n", (intptr_t)si->si_addr,
                      std::time(0)));
    dsm_singleton->grant_read((char *)si->si_addr);
  }
}

int DSMNode::prot_check(void *addr) {
  return this->page_info[relative_page_id_from_addr(addr)];
}

void dsm::dsm_init(pid_t child, int *wait_x, int node_id) {
  DEBUG_STMT(printf("setup handler\n"));
  while (1) {
    DEBUG_STMT(printf("WAIT FOR SIGNALS\n"));
    wait(NULL);
    siginfo_t sig;
    ptrace(PTRACE_GETSIGINFO, child, NULL, &sig);
    DEBUG_STMT(printf("signal received %d\n", sig.si_signo));
    if (sig.si_signo == 11) {
      DEBUG_STMT(printf("SIGSEGV\n"));
      handler(11, &sig, nullptr);
    } else if (sig.si_signo == SIGUSR2) {
      DEBUG_STMT(printf("SIGUSR2\n"));
      user_mprotect_respond();
    } else if (sig.si_signo == SIGUSR1) {
      DEBUG_STMT(printf("SIGUSR1\n"));
      struct iovec local[1];
      struct iovec remote[1];
      *wait_x = node_id;
      local[0].iov_base = wait_x;
      local[0].iov_len = sizeof(int);
      remote[0].iov_base = wait_x;
      remote[0].iov_len = sizeof(int);
      DEBUG_STMT(printf("!!!WRITE at %lx, %d\n", (intptr_t)(wait_x), *wait_x));
      int err = process_vm_writev(child, local, 1, remote, 1, 0);
      ASSERT_PERROR(err);
    }
    // printf("sig num: %d, addr: %lx, pc: %llx, inst: %lx, rsp: %lx\n",
    //        sig.si_signo, sig.si_addr, saved_regs.rip, *(uint64_t *)pc,
    //        rsp);
    if (ptrace(PTRACE_CONT, child, NULL, NULL) < 0) {
      perror("ptrace");
      exit(0);
    }
  }
}
struct dsm_init_args {
  pid_t child;
  NodeAddr self;
  NodeAddr dst;
  char *region;
  size_t size;
};
char *dsm::dsm_init_master(pid_t child, NodeAddr self, char *region,
                           size_t size, int *wait_x) {
  user_mprotect_init();
  pthread_t tid;
  dsm_init_args args = {child, self, NodeAddr(), region, size};
  pthread_create(
      &tid, NULL,
      [](void *input) -> void * {
        dsm_init_args *args = (dsm_init_args *)input;
        DEBUG_STMT(
            printf("addr of mem_region: 0x%lx\n", ((intptr_t)args->region)));
        DSMNode *node;
        DEBUG_STMT(printf("create master\n"));
        DEBUG_STMT(printf("make new node\n"));
        node = new DSMNode(args->self, args->region, args->size, true,
                           args->child);
        kill(args->child, SIGUSR1);
        DEBUG_STMT(printf("finish make new node\n"));
        return nullptr;
      },
      &args);
  dsm_init(child, wait_x, 0);
  return region;
}
char *dsm::dsm_init_node(pid_t child, NodeAddr self, NodeAddr dst, char *region,
                         size_t size, int *wait_x) {
  user_mprotect_init();
  pthread_t tid;
  dsm_init_args args = {child, self, dst, region, size};
  pthread_create(
      &tid, NULL,
      [](void *input) -> void * {
        dsm_init_args *args = (dsm_init_args *)input;
        DEBUG_STMT(
            printf("addr of mem_region: 0x%lx\n", ((intptr_t)args->region)));
        DSMNode *node;
        DEBUG_STMT(printf("create process\n"));
        DEBUG_STMT(printf("make new node\n"));
        node = new DSMNode(args->self, args->region, args->size, false,
                           args->child);
        NodeAddr dst_addr;
        DEBUG_STMT(printf("try connect\n"));
        node->connect(args->dst);
        kill(args->child, SIGUSR1);
        DEBUG_STMT(printf("finish make new node\n"));
        return nullptr;
      },
      &args);
  dsm_init(child, wait_x, 1);
  return region;
}

bool DSMNode::is_in_range(char *addr) {
  int idx = ((intptr_t)addr - (intptr_t)STACK_START) / PAGE_SIZE;
  DEBUG_STMT(printf("idx %d, base: %lx, end: %lx, violate 1: %d\n", idx,
                    (intptr_t)this->base,
                    ((intptr_t)this->base) + PAGE_SIZE * this->page_info.size(),
                    !(idx >= 0 && idx < TOTAL_POSSIBLE_STACKS &&
                      idx % PAGES_PER_STACK == 0)));

  return !(idx >= 0 && idx < TOTAL_POSSIBLE_STACKS &&
           idx % PAGES_PER_STACK == 0) &&
         ((intptr_t)addr >= (intptr_t)this->base) &&
         ((intptr_t)addr <
          ((intptr_t)this->base) + PAGE_SIZE * this->page_info.size());
}

void DSMNode::request_hand_shake(NodeAddr my_addr, NodeAddr dst_addr) {
  DEBUG_STMT(
      printf("HANDSHAKE REQ TO %s:%d\n", dst_addr.ip.c_str(), dst_addr.port));
  rpc::client cli(dst_addr.ip, dst_addr.port);
  cli.call(RPC_HAND_SHAKE, m_addr);
}

void DSMNode::respond_hand_shake(NodeAddr src_addr) {
  DEBUG_STMT(printf("Received hand shake from %s:%d\n", src_addr.ip.c_str(),
                    src_addr.port));
  this->conn.push_back(src_addr);
}

vector<NodeAddr> DSMNode::request_join(NodeAddr dst_addr) {
  DEBUG_STMT(printf("JOIN REQ TO %s:%d\n", dst_addr.ip.c_str(), dst_addr.port));
  try {
    rpc::client cli(dst_addr.ip, dst_addr.port);
    return cli.call(RPC_JOIN).as<vector<NodeAddr>>();
  } catch (rpc::rpc_error err) {
    DEBUG_STMT(printf("%s\n", err.what()));
    return vector<NodeAddr>();
  }
}
vector<NodeAddr> DSMNode::respond_join() {
  DEBUG_STMT(printf("received join\n"));
  return this->conn;
}

page DSMNode::request_write(NodeAddr dst_addr, uint64_t pagenum) {
  DEBUG_STMT(
      printf("WRITE REQ TO %s:%d\n", dst_addr.ip.c_str(), dst_addr.port));
  rpc::client cli(dst_addr.ip, dst_addr.port);
  return cli.call(RPC_WRITE, pagenum).as<page>();
}

page DSMNode::response_write(uint64_t relative_page_id) {
  DEBUG_STMT(printf("recieved write req %lx\n", relative_page_id));
  page res;
  res.clear();
  LOCK(this->mu)
  if (this->page_info.size() > relative_page_id &&
      OWNERSHIP(this->page_info[relative_page_id])) {
    this->page_info[relative_page_id] = DSM_PROT_READ;
    UNLOCK(this->mu)
    auto addr = relative_page_id_to_addr(relative_page_id);
    res.resize(PAGE_SIZE);
    this_thread::sleep_for(std::chrono::milliseconds(random() % 500));
    user_mprotect_req(this->pid, addr, PAGE_SIZE, PROT_READ, true, &res[0]);
    // remote_mempage_read(this->pid, &res[0], addr); merged into previous call
    DEBUG_STMT(printf("res %d, %d, %lx\n", res[0], res[1], (intptr_t)addr));
    DEBUG_STMT(printf("setup result page\n"));
  } else {
    UNLOCK(this->mu)
  }
  return res;
}

page DSMNode::request_read(NodeAddr dst_addr, uint64_t pagenum) {
  DEBUG_STMT(printf("READ REQ TO %s:%d\n", dst_addr.ip.c_str(), dst_addr.port));
  rpc::client cli(dst_addr.ip, dst_addr.port);
  return cli.call(RPC_READ, pagenum).as<page>();
}

page DSMNode::response_read(uint64_t relative_page_id) {
  page res;
  res.clear();
  DEBUG_STMT(printf("recieved read req %lx\n", relative_page_id));
  LOCK(this->mu)
  if (this->page_info.size() > relative_page_id &&
      OWNERSHIP(this->page_info[relative_page_id])) {
    DEBUG_STMT(printf("master respond %lx\n",
                      (intptr_t)relative_page_id_to_addr(relative_page_id)));
#ifndef RELEASE_CONSISTANCY
    this->page_info[relative_page_id] = DSM_PROT_OWNED;
#endif
    res.resize(PAGE_SIZE);
    auto addr = relative_page_id_to_addr(relative_page_id);
    this_thread::sleep_for(std::chrono::milliseconds(random() % 500));
    user_mprotect_req(this->pid, addr, PAGE_SIZE,
                      OWNERSHIP(this->page_info[relative_page_id])
                          ? PROT_READ | PROT_WRITE
                          : PROT_READ,
                      true, &res[0]);
    // remote_mempage_read(this->pid, &res[0], addr); //merged into previous
    // call
    //  remote_mempage_read(this->pid, &res[0],
    //                      relative_page_id_to_addr(relative_page_id));
    DEBUG_STMT(printf("master respond done\n"));
  }
  UNLOCK(this->mu)
  return res;
}

void DSMNode::connect(NodeAddr dst_addr) {
  this->conn = request_join(dst_addr);
  this->conn.push_back(dst_addr);
  DEBUG_STMT(printf("connect %s:%d\n", dst_addr.ip.c_str(), dst_addr.port));
  struct thread_arg {
    DSMNode *self;
    int idx;
  };
  pthread_t *threads =
      (pthread_t *)malloc(sizeof(pthread_t) * this->conn.size());
  thread_arg *args = (thread_arg *)malloc(this->conn.size());
  for (int i = 0; i < this->conn.size(); i++) {
    args[i].self = this;
    args[i].idx = i;
    pthread_create(
        &threads[i], NULL,
        [](void *input) -> void * {
          thread_arg *arg = (thread_arg *)input;
          DSMNode *self = (DSMNode *)arg->self;
          self->request_hand_shake(self->m_addr, self->conn[arg->idx]);
          return NULL;
        },
        &args[i]);
  }
  for (int i = 0; i < this->conn.size(); i++) {
    pthread_join(threads[i], NULL);
  }
  free(args);
  free(threads);
}

struct thread_arg_content {
  DSMNode *self;
  page data;
  pthread_mutex_t mu;
  pthread_cond_t cond;
  int count;
};
struct thread_arg {
  thread_arg_content *content;
  int idx;
  int relative_page_id;
  int prot;
  bool done;
};

bool DSMNode::grant_prot(page_id_t relative_page_id, int prot) {
  DEBUG_STMT(printf("requesting protection \n"));
  page res;
  pthread_t *threads =
      (pthread_t *)malloc(sizeof(pthread_t) * this->conn.size());
  thread_arg_content *arg_content = new thread_arg_content();
  arg_content->self = this;
  arg_content->data.clear();
  arg_content->count = this->conn.size();
  thread_arg *args = new thread_arg[this->conn.size()];

  pthread_mutex_init(&arg_content->mu, NULL);
  pthread_cond_init(&arg_content->cond, NULL);
  DEBUG_STMT(printf("sending rpc... %zu\n", conn.size()));
  for (int i = 0; i < this->conn.size(); i++) {
    args[i].content = arg_content;
    args[i].idx = i;
    args[i].relative_page_id = relative_page_id;
    args[i].prot = prot;
    args[i].done = false;
    DEBUG_STMT(printf("relative_page_id %x\n", args[i].relative_page_id));
    pthread_create(
        &threads[i], NULL,
        [](void *input) -> void * {
          thread_arg *arg = (thread_arg *)input;
          DSMNode *self = arg->content->self;
          auto res = arg->prot == DSM_PROT_READ
                         ? self->request_read(self->conn[arg->idx],
                                              arg->relative_page_id)
                         : self->request_write(self->conn[arg->idx],
                                               arg->relative_page_id);
          LOCK(arg->content->mu)
          DEBUG_STMT(printf("received page with size %ld\n", res.size()));
          if (res.size() == PAGE_SIZE) {
            arg->content->data = res;
          }
          arg->content->count--;
          arg->done = true;
          // SIGNAL(arg->content->cond)
          UNLOCK(arg->content->mu)
          return NULL;
        },
        &args[i]);
  }
#ifdef RELEASE_CONSISTANCY
  // while (arg_content->data.size() != PAGE_SIZE && arg_content->count) {
  //     WAIT(arg_content->cond, arg_content->mu)
  // }
  for (int i = 0; i < this->conn.size(); i++) {
    while (!args[i].done) {
      // Handle mprotect request to solve dead lock when two threads get page
      // fault at the same time
      user_mprotect_respond();
    }
    pthread_join(threads[i], nullptr);
  }
#else
  for (int i = 0; i < this->conn.size(); i++) {
    pthread_join(threads[i]);
  }
#endif
  bool succeed = false;
  if (arg_content->data.size() == PAGE_SIZE) {
    LOCK(this->wr_mu)
    user_mprotect(this->pid, relative_page_id_to_addr(relative_page_id),
                  PAGE_SIZE, PROT_READ | PROT_WRITE);
    int err = remote_mempage_write(this->pid, &arg_content->data[0],
                                   relative_page_id_to_addr(relative_page_id));
    UNLOCK(this->wr_mu)
    DEBUG_STMT(printf("write %d, %d, %d, %lx\n", arg_content->data[0],
                      arg_content->data[1], err,
                      (intptr_t)relative_page_id_to_addr(relative_page_id)));
    if (err < 0) {
      perror("ERR remote page write");
    }
    succeed = true;
  }
  DEBUG_STMT(printf("got rpc response %d\n", succeed));
  delete threads;
  delete arg_content;
  delete[] args;
  return succeed;
}

bool DSMNode::update_prot(void *addr) {
  page_id_t relative_page_id = relative_page_id_from_addr(addr);
  if (OWNERSHIP(this->page_info[relative_page_id])) {
    this->page_info[relative_page_id] = DSM_PROT_WRITE;
    user_mprotect(this->pid, (void *)FLOOR((intptr_t)addr), PAGE_SIZE,
                  PROT_READ | PROT_WRITE);
    return true;
  }
  return false;
}

bool DSMNode::grant_write(char *addr) {
  page_id_t relative_page_id = relative_page_id_from_addr(addr);
  bool res = this->grant_prot(relative_page_id, DSM_PROT_WRITE);
  if (res) {
    while (pthread_mutex_trylock(&this->mu)) {
      user_mprotect_respond();
    }
    this->page_info[relative_page_id] = DSM_PROT_WRITE;
    UNLOCK(this->mu)
  }
  return res;
}

bool DSMNode::grant_read(char *addr) {
  page_id_t relative_page_id = relative_page_id_from_addr(addr);
  bool res = this->grant_prot(relative_page_id, DSM_PROT_READ);
  if (res) {
    LOCK(this->mu)
    this->page_info[relative_page_id] = DSM_PROT_READ;
    user_mprotect(this->pid, (void *)FLOOR((intptr_t)addr), PAGE_SIZE,
                  PROT_READ);
    UNLOCK(this->mu)
  }
  return res;
}

void DSMNode::sync() {
  mprotect(this->base, this->page_info.size() * PAGE_SIZE, PROT_NONE);
}

DSMNode::DSMNode(NodeAddr m_addr, void *_base, size_t _len, bool is_master,
                 pid_t pid) {

  this->pid = pid;
  DEBUG_STMT(printf("is master %d\n", is_master));
  int pages = VPADDR2VPID(_len);
  page_info.resize(pages);
  for (int i = 0; i < pages; i++) {
    this->page_info[i] = is_master ? DSM_PROT_WRITE : DSM_PROT_INVALID;
  }
  this->base = (char *)_base;
  DEBUG_STMT(printf("check mmap equal %lx\n", (intptr_t)this->base));
  ASSERT_PERROR(this->base);
  if (!is_master) {
    user_mprotect_req(pid, _base, _len, PROT_NONE, 0, 0);
    DEBUG_STMT(printf("disable from %lx to %lx\n", (intptr_t)this->base,
                      (intptr_t)this->base + _len));
  }

  DEBUG_STMT(printf("setup mem\n"));
  this->m_addr = m_addr;
  if (pthread_mutex_init(&this->mu, NULL)) {
    exit(-1);
  }
  serv = new rpc::server(m_addr.port);
  serv->bind(RPC_HAND_SHAKE, [this](NodeAddr src_addr) -> void {
    return this->respond_hand_shake(src_addr);
  });
  serv->bind(RPC_JOIN,
             [this]() -> vector<NodeAddr> { return this->respond_join(); });
  serv->bind(RPC_WRITE, [this](uint64_t pagenum) -> page {
    return this->response_write(pagenum);
  });
  serv->bind(RPC_READ, [this](uint64_t pagenum) -> page {
    return this->response_read(pagenum);
  });
  dsm_singleton = this;
  pthread_create(
      &this->tid, NULL,
      [](void *input) -> void * {
        rpc::server *serv = (rpc::server *)input;
        serv->async_run(4);
        return nullptr;
      },
      serv);
  DEBUG_STMT(printf("run server\n"));
}
