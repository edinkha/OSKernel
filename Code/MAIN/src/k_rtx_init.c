/** 
 * @file:   k_rtx_init.c
 * @brief:  Kernel initialization C file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_rtx_init.h"
#include "uart.h"
#include "uart_polling.h"
#include "i_proc.h"
#include "k_memory.h"
#include "k_process.h"

void k_rtx_init(void)
{
	__disable_irq();  // atomic(on)
	uart_irq_init(0); // uart0, interrupt-driven
	uart1_init();     // uart1, polling
	timer_init(0);    // initialize timer 0
	memory_init();    // initialize memory
	process_init();   // initialize processes (system, user, and interrupt)
	__enable_irq();   // atomic(off)
	
	// uart1_put_string("Type 'S' in COM0 terminal to switch between proc1 and proc2 or wait for them to switch between themselves\n\r");
	// uart1_put_string("An input other than 'S' in COM0 terminal will be have no effect.\n\r"); 
	
	/* start the first process */
	k_release_processor();
}
