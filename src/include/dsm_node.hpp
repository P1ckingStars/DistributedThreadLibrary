#ifndef DSM_NODE
#define DSM_NODE

#include "macros.hpp"
#include "rpc/server.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <pthread.h>
#include <rpc/msgpack/adaptor/define_decl.hpp>
#include <sched.h>
#include <vector>
using namespace std;

typedef uint64_t page_id_t;

namespace dsm {
// init seg tree
// setup handler
//

struct private_region {
    int x;
};

struct NodeAddr {
    string ip;
    short port;
    MSGPACK_DEFINE_ARRAY(ip, port);
};

void dsm_init(pid_t child, int * wait_x, int node_id);
char * dsm_init_master(pid_t child, NodeAddr self, char * region, size_t size, int * wait_x);
char * dsm_init_node(pid_t child, NodeAddr self, NodeAddr dst, char * region, size_t size, int * wait_x);

typedef vector<char> page;

class bit_mask {
#define SIZE2BYTE(x) (((x) + 7) >> 3)
#define MASK_OFFSET(x) (1 << (x))
    vector<char> mask;

public:
    bit_mask(size_t size) { mask = vector<char>(SIZE2BYTE(size)); }
    bit_mask(size_t size, bool all_one) {
        mask = vector<char>(SIZE2BYTE(size), all_one ? -1 : 0);
    }
    bit_mask(vector<char> _mask) : mask(_mask) {}
    bool get(int x) { return mask[SIZE2BYTE(x)] & MASK_OFFSET(x % 8); }
    void set(int x) { mask[SIZE2BYTE(x)] |= MASK_OFFSET(x % 8); }
};

class DSMNode {
    char *base;
    pid_t pid;
    pthread_mutex_t mu;
    pthread_mutex_t wr_mu;
    NodeAddr m_addr;
    pthread_t tid;
    vector<NodeAddr> conn;
    vector<char> page_info;
    char *relative_page_id_to_addr(page_id_t page_id) {
        return this->base + VPID2VPADDR(page_id);
    }
    page_id_t relative_page_id_from_addr(void *ptr) {
        return VPADDR2VPID((intptr_t)ptr - (intptr_t)this->base);
    }
    page_id_t relative_page_id_from_page_id(page_id_t page_id) {
        return page_id - VPADDR2VPID((intptr_t)this->base);
    }
    void wait_recv();
    void add(int i, int k);
    void request_hand_shake(NodeAddr my_addr, NodeAddr dst_addr);
    void respond_hand_shake(NodeAddr src_addr);
    vector<NodeAddr> request_join(NodeAddr dst_addr);
    vector<NodeAddr> respond_join();
    page request_write(NodeAddr dst_addr, uint64_t pagenum);
    page response_write(uint64_t pagenum);
    page request_read(NodeAddr dst_addr, uint64_t pagenum);
    page response_read(uint64_t pagenum);
    bool grant_prot(page_id_t relative_page_id, int prot);
    rpc::server *serv;

public:
    DSMNode(NodeAddr m_addr, void *base, size_t len, 
             bool is_master, int swapfd);
    ~DSMNode() {
        if (!!tid)
            pthread_cancel(tid);
        if (!!serv)
            delete serv;
    }
    void sync();
    int prot_check(void *addr);
    void connect(NodeAddr dst_addr);
    bool update_prot(void *addr);
    bool grant_write(char *addr);
    bool grant_read(char *addr);
    bool is_in_range(char *addr);
};

} // namespace dsm
#endif
