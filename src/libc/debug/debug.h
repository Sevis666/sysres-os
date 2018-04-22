#ifndef DEBUG_H
#define DEBUG_H

#include "stdint.h"

#define print_reg(reg) \
  uint64_t __variable__print__reg__ = 0;\
  asm volatile("MRS %0, " #reg : "=r"(__variable__print__reg__) : :);\
  uart_info("Reg " #reg " : 0x%x\r\n", __variable__print__reg__);

#define assert(cond) \
    if((cond) == 0){ \
        uart_error("Assertion \"" #cond "\" failed at %s:%d\r\n",\
                   __FILE__, __LINE__);\
        abort(); \
    }; \


void abort();

#endif
