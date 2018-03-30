#include "interrupt.h"
#include "libc/uart/uart.h"
#include "libc/debug/debug.h"
#include "stdbool.h"

void display_esr_eln_info(uint64_t esr_eln){
    //Parse ESR_EL1 (see aarch64, exception and interrupt handling)
    uint64_t exception_class = (esr_eln & 0xfc000000) >> 26;
    bool il = (esr_eln & 0x2000000) >> 25;
    uint16_t instr_specific_syndrom = (esr_eln & 0x1fffff);
    uart_printf(
        "ER_ELn info :\nException Class : %d\nIL : %d\n"
         "Instruction Specific Syndrom : 0x%x\n",
        exception_class, il, instr_specific_syndrom);
}

void display_pstate_info(uint64_t pstate){
    bool n  = (pstate & (1 << 31)) >> 31;
    bool z  = (pstate & (1 << 30)) >> 30;
    bool c  = (pstate & (1 << 29)) >> 29;
    bool v  = (pstate & (1 << 28)) >> 28;
    bool ss = (pstate & (1 << 21)) >> 21;
    bool il = (pstate & (1 << 23)) >> 23;
    bool d  = (pstate & (1 <<  9)) >>  9;
    bool a  = (pstate & (1 <<  8)) >>  8;
    bool i  = (pstate & (1 <<  7)) >>  7;
    bool f  = (pstate & (1 <<  6)) >>  6;
    int  m  = (pstate &    0x10  )      ;
    uart_printf(
        "PSTATE info :\n"
        "Negative condition flag : %d\n"
        "Zero     condition flag : %d\n"
        "Carry    condition flag : %d\n"
        "Overflow condition flag : %d\n"
        "Debug mask              : %d\n"
        "System Error mask       : %d\n"
        "IRQ mask                : %d\n"
        "FIQ mask                : %d\n"
        "Software step           : %d\n"
        "Illegal execution state : %d\n"
        "Mode field              : %d\n"
                , n,z,c,v,d,a,i,f,ss,il,m);
}

void c_sync_handler(uint64_t el, uint64_t nb, uint64_t spsr_el, uint64_t elr_el, uint64_t esr_el, uint64_t far_el){
    /* el indicates exception level */
    uart_printf(
        "Sync Interruption :\nAt level : EL%d\nCase nb :%d\n"
        "ELR_EL : 0x%x\nSPSR_EL : 0x%x\nESR_EL : 0x%x\nFAR_EL : 0x%x\n",
        el,nb, elr_el, spsr_el, esr_el, far_el);
    display_esr_eln_info(esr_el);
    display_pstate_info(elr_el);
    abort();
}

void c_serror_handler(uint64_t el, uint64_t nb, uint64_t elr_el, uint64_t spsr_el, uint64_t esr_el, uint64_t far_el){
    uart_printf(
        " System Error :\nAt level : EL%d\nCase nb :%d\n"
        "ELR_EL = 0x%x\nSPSR_EL = 0x%x\nESR_EL = 0x%x\nFAR_EL = 0x%x\n",
        el, nb, elr_el, spsr_el, esr_el, far_el);
    display_esr_eln_info(esr_el);
    display_pstate_info(elr_el);
    abort();
}

/* TODO : get back info on the interrupt from GIC */
void c_irq_handler(uint64_t el, uint64_t nb){
    uart_printf(
        " IRQ :\nAt level : EL%d\nCase nb :%d\n",
        el, nb);
    abort();
}

void c_fiq_handler(uint64_t el, uint64_t nb){
    uart_printf(
        " FIQ :\nAt level : EL%d\nCase nb :%d\n",
        el,nb);
    abort();
}

void c_el2_handler(){
    uart_printf("Interrupt handled at EL2 : non supported\nAborting...\n");
    abort();
}

void c_el1_svc_aarch64_handler(uint64_t x0,uint64_t x1,uint64_t x2,uint64_t x3,
                               uint64_t x4,uint64_t x5,uint64_t x6,uint64_t x7){
    //Syscall arguments
    (void) x0;
    (void) x1;
    (void) x2;
    (void) x3;
    (void) x4;
    (void) x5;
    (void) x6;
    (void) x7;
    uint64_t esr_el1;
    asm ("mrs %0, esr_el1":"=r"(esr_el1)::);
    uint16_t syscall = (esr_el1 & 0x1ffffff); //get back syscall code
    switch(syscall){
        default:
            uart_printf("Error no syscall SVC Aarch64 corresponding to code %d\n", syscall);
            display_esr_eln_info(esr_el1);
            uart_printf("Aborting...\n");
            abort();
    }
}
