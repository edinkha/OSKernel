/**
 * @file:   usr_proc.c
 * @brief:  Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date:   2014/01/17
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS - 1; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x100;
	}
	
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[3].mpf_start_pc = &proc4;
	g_test_procs[4].mpf_start_pc = &proc5;
}


/**
 * @brief: a process that prints five uppercase letters
 *         and then yields the cpu.
 */
void proc1(void)
{
	int i = 0;
	int ret_val = 10;
	while ( 1) {
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
			ret_val = release_processor();
#ifdef DEBUG_0
			printf("proc1: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
		}
		uart0_put_char('A' + i%26);
		i++;
	}
}

/**
 * @brief: a process that prints five numbers
 *         and then yields the cpu.
 */
void proc2(void)
{
	int i = 0;
	int ret_val = 20;
	while ( 1) {
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
			ret_val = release_processor();
#ifdef DEBUG_0
			printf("proc2: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
		}
		uart0_put_char('0' + i%10);
		i++;
	}
}

void proc3(void)
{
	void* memblock1 = NULL;
	void* memblock2 = NULL;

	int ret_val = 20;
	while ( 1) {

#ifdef DEBUG_0
			printf("proc3 requesting memory\n");
#endif /* DEBUG_0 */
		memblock1 = request_memory_block();
		memblock2 = request_memory_block();
		ret_val = release_processor();
#ifdef DEBUG_0
			printf("proc3 releasing memory\n");
#endif /* DEBUG_0 */
		release_memory_block(memblock1);
		release_memory_block(memblock2);
	}
}

void proc4(void)
{
	void* memblock1 = NULL;

	int ret_val = 20;
	while ( 1) {

#ifdef DEBUG_0
			printf("proc4 requesting memory\n");
#endif /* DEBUG_0 */
		memblock1 = request_memory_block();
		ret_val = release_processor();
#ifdef DEBUG_0
			printf("proc4 releasing memory\n");
#endif /* DEBUG_0 */
		release_memory_block(memblock1);
	}
}

void proc5(void)
{
	void* memblock1 = NULL;

	int ret_val = 20;
	while ( 1) {

#ifdef DEBUG_0
			printf("proc5 requesting memory\n");
#endif /* DEBUG_0 */
		memblock1 = request_memory_block();
		ret_val = release_processor();
#ifdef DEBUG_0
			printf("proc5 releasing memory\n");
#endif /* DEBUG_0 */
		release_memory_block(memblock1);
	}
}

void proc6(void)
{
	int i = 0;
	int ret_val = 20;
	while ( 1) {
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
			ret_val = release_processor();
#ifdef DEBUG_0
			printf("proc2: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
		}
		uart0_put_char('0' + i%10);
		i++;
	}
}
