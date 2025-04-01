#ifndef LIN_ALLOCATOR_HPP
#define LIN_ALLOCATOR_HPP

#include <stddef.h>
void *alloc(size_t size);
void dealloc(void *addr);

template<class T, typename... Args>
T * make(Args... args) {
    T * res = (T *)alloc(sizeof(T));
    new (res) T(args...);
    return res;
}

#endif
