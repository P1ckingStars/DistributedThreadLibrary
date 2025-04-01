#include "util/lin_allocator.hpp"
#include "dsm_lock.hpp"
#include "linker_symbol.hpp"
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define DYNAMIC_SIZE (4096 * 10000)
#define MIN_CHUNK 128
#define LINEAR_ALLOCATOR
#define ALIGN64_FLOOR(x) (((x) >> 3) << 3)
#define ALIGN64_CEIL(x) (((x + 7) >> 3) << 3)
#define REGION_END_NODE ((ListNode *)((DYNAMIC_START) + DYNAMIC_SIZE))

dsm::dsm_mutex alloc_mu = 0;

typedef struct ListNode {
  struct ListNode *prev;
  struct ListNode *next;
  bool valid;
  bool in_use;
  short alloc_size;
} ListNode;

size_t chunk_size(ListNode const *node) {
  return (intptr_t)(node->next) - (intptr_t)(node + 1);
}

void *alloc(size_t size) {
  dsm::Guard lock(&alloc_mu);
  size = ALIGN64_CEIL(size); // Align the requested size
  ListNode *root = (ListNode *)DYNAMIC_START;
  if (!root->valid) {
    root->prev = 0;
    root->next = REGION_END_NODE;
    root->valid = true;
    root->in_use = false;
    root->alloc_size = 0;
  }
  for (ListNode *curr = root; curr != REGION_END_NODE; curr = curr->next) {
    if (curr->in_use || chunk_size(curr) < size)
      continue;
    if (chunk_size(curr) - size > MIN_CHUNK) {
      ListNode *new_node = (ListNode *)((intptr_t)(curr + 1) + size);
      new_node->next = curr->next;
      new_node->prev = curr;
      new_node->valid = true;
      new_node->in_use = false;
      curr->next = new_node;
    }
    curr->in_use = true;
    curr->alloc_size = size;
    return curr + 1;
  }
  return 0;
}

size_t capacity(void *addr) {
  ListNode *node = (ListNode *)addr;
  node--;
  return (node->valid && node->in_use) ? ALIGN64_FLOOR(chunk_size(node)) : 0;
}

size_t alloc_size(void *addr) {
  ListNode *node = (ListNode *)addr;
  node--;
  return (node->valid && node->in_use) ? node->alloc_size : 0;
}

void dealloc(void *addr) {
  dsm::Guard lock(&alloc_mu);
  ListNode *node = (ListNode *)addr;
  node--;
  node->in_use = false;

  // Merge with previous chunk if free
  if (node->prev && !node->prev->in_use) {
    node->prev->next = node->next;
    if (node->next != REGION_END_NODE) {
      node->next->prev = node->prev;
    }
    node = node->prev;
  }

  // Merge with next chunk if free
  if (node->next != REGION_END_NODE && !node->next->in_use) {
    node->next = node->next->next;
    if (node->next != REGION_END_NODE) {
      node->next->prev = node;
    }
  }
}
