/**
 * @file:   usr_proc.c
 * @brief:  Six user processes: proc1...6 to test our RTX
 * @author: Justin Gagne
 * @date:   2014/03/17
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "utils.h"
#include "string.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority = LOWEST;
		g_test_procs[i].m_stack_size = 0x100;
	}  
	g_test_procs[0].mpf_start_pc = &user_test_runner;
	g_test_procs[1].mpf_start_pc = &priority_tests;
	g_test_procs[2].mpf_start_pc = &memory_tests;
	g_test_procs[3].mpf_start_pc = &wall_clock_tests;
	g_test_procs[4].mpf_start_pc = &general_messaging_tests;
	g_test_procs[5].mpf_start_pc = &delayed_messaging_tests;
}
int results_printed = 0;

/**
 * @brief: This is the process that runs all of the user level tests.
 * It calls user processes 2, 3, 4, 5, and 6 to test different areas of the RTX.
 * We test by checking actual results with expected results, then print the results at the end.
 */
void user_test_runner(void)
{
	while (1) {
	
	}
}

/**
 * @brief: a process that prints 4x5 numbers and changes the priority of p3
 */
void priority_tests(void)
{
	
}

/**
 * @brief: a process responsible for setting the wall clock to 10:10:10
 */
void memory_tests(void)
{
	
}

/**
 * @brief: a process responsible for resetting the Wall Clock
 */
void wall_clock_tests(void)
{
	
}

/**
 * @brief: a process responsible for stopping the Wall Clock
 */
void general_messaging_tests(void)
{
	
}

/**
 * @brief: a process responsible for the three hot keys
 */
void delayed_messaging_tests(void)
{
	
}
