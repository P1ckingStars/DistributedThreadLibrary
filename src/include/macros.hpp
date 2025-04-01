#ifndef MACROS_HPP
#define MACROS_HPP

#define RELEASE_CONSISTANCY
#define PAGE_OFFSET_BIT 12
#define PAGE_ALIGNED_ADDR(x) ((x >> PAGE_OFFSET_BIT) << PAGE_OFFSET_BIT)
#define PAGE_SIZE (1 << PAGE_OFFSET_BIT)
#define VPID2VPADDR(vpid) ((vpid) << PAGE_OFFSET_BIT)
#define VPADDR2VPID(vpaddr) ((vpaddr) >> PAGE_OFFSET_BIT)

#define ASSERT(EXP, MSG)                                                       \
{                                                                            \
    if (!(EXP)) {                                                              \
        printf("ASSERTION FAILED at %s:%d: %s\n", __FILE__, __LINE__, MSG);      \
        exit(-1);                                                                \
    }                                                                          \
}

#define ASSERT_PAGE_ALIGN(addr)                                                \
ASSERT(((intptr)addr) % PAGE_SIZE == 0, "addr page align")
#define ASSERT_NOT_NULL(ptr) ASSERT(ptr, "null ptr error")
#define ASSERT_NOT_NULL_MSG(ptr, MSG) ASSERT(ptr, MSG)
#define ASSERT_POSIX_STATUS(status) ASSERT(status != -1, "posix error")
#define ASSERT_PERROR(err_no)                                                         \
{                                                                            \
    if ((int64_t)(err_no) == -1) {                                             \
        perror("POSIX ");                                                        \
        exit(-1);                                                                \
    }                                                                          \
}


#endif
