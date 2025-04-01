# TODO 14: Remove mysqrt.cxx from the list of sources

# TODO 1: Add a library called MathFunctions with sources MathFunctions.cxx
# and mysqrt.cxx

add_library(DSMLib
    dsm_lock.cpp
    main.cpp
    dsm_node.cpp
    injection.s 
    user_mprotect.cpp
    threadlib/cpu.cpp
    threadlib/cv.cpp
    threadlib/dsm_cpu_impl.cpp
    threadlib/mutex.cpp
    threadlib/schedulerState.cpp
    threadlib/thread.cpp
    util/lin_allocator.cpp
)

add_compile_options(-fno-stack-protector)

target_include_directories(DSMLib PUBLIC
    "${CMAKE_SOURCE_DIR}/src/include"
)

