#include "../libk/debug.h"
#include "../libk/uart.h"
#include "../libk/sys.h"
#include "../test/test.h"
#include "../proc/proc.h"
#include "mem_manager.h"
#include "io.h"

void listen_shutdown(){
    int pid;
    /* we want the proc to send a code, o/w we reject */
    int code = 1;
    while(1){
        pid = receive(&code, sizeof(int));
        switch(code){
        case 0:
            uart_info("System halting at the request of process %d\r\n", pid);
            halt();
            break;
        default:
            /* Unknown signal */
            (void) acknowledge(-1, NULL, 0);
        }
    }
}

void start_mem_manager_process(){
    int ret = fork(15);
    assert(ret != -1);
    if(ret == 0)
        main_mem_manager();
    /* Make sur it has pid 2 (ie don't create processes before calling this !)*/
    assert(ret == MEM_MANAGER_PID);
    return;
}

void start_io_manager_process(){
    int ret = fork(15);
    assert(ret != -1);
    if(ret == 0)
        main_io_manager();
    /* Make sur it has pid 3 (ie don't create processes before calling this !)*/
    assert(ret == IO_MANAGER_PID);
    return;
}


void start_test_process(){
    int ret = fork(15);
    assert(ret != -1);
    if (ret != 0) return;
    else {
        /* fork_test1(); */
        /* fork_test2(); */
        /* fork_test2bis(); */
        /* fork_test4(); */
        /* fork_test4bis(); */
        /* sched_test1(); */
        /* io_simple_test(); */
        print_io_formatting_tests();
        test_io_get_string();
        uart_info("TEST SUCCESS\r\n");
        exit(0, 0);
    }
}



void main_init(){
    uart_info("Init process running\r\n");
    start_mem_manager_process();
    start_io_manager_process();
    start_test_process();
    listen_shutdown();
}

