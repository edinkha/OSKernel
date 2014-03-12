/** 
 * @file:   k_rtx_init.c
 * @brief:  Kernel initialization C file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_rtx_init.h"
#include "uart.h"
#include "uart_polling.h"
#include "timer.h"
#include "k_memory.h"
#include "k_process.h"

extern volatile uint32_t g_timer_count;

void k_rtx_init(void)
{
				volatile uint8_t sec = 0;
	
        __disable_irq();
				uart_irq_init(0); // uart0, interrupt-driven
        uart1_init();     // uart1, polling
				//timer_init(0);    // initialize timer 0; might need to be moved down later
        memory_init();
        process_init();
        __enable_irq();
	
				/* 
					uart polling is used in this example only to help
					demonstrate timer interrupt programming.
					In your final project, polling is NOT ALLOWED in i-processes.
					Refer to the UART_irq example for interrupt driven UART.
				*/

// 				while (1) {
// 					/* g_timer_count gets updated every 1ms */
// 					if (g_timer_count == 1000) { 
// 						uart1_put_char('0'+ sec);
// 						sec = (++sec)%10;
// 						g_timer_count = 0; /* reset the counter */
// 					}     
// 				}
	
				uart1_put_string("Type 'S' in COM0 terminal to switch between proc1 and proc2 or wait for them to switch between themselves\n\r");
				uart1_put_string("An input other than 'S' in COM0 terminal will be have no effect.\n\r"); 
	
				/* start the first process */
	
        k_release_processor();
}
