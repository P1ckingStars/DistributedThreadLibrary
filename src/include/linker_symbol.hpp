#ifndef LINKER_SYMBOL_HPP
#define LINKER_SYMBOL_HPP

extern char __bss_start;
extern char __stack_start;

#define DYNAMIC_START (&__bss_start)
#define STACK_START (&__stack_start)

#endif
