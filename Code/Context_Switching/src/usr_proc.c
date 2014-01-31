/*
STEP-THROUGH OF TEST PROCS:

        ! - each of the blocks starts off with the lowest priority
        ! - proc1 requests 2 blocks and gets them
        ! - proc2 requests 2 blocks and gets them
		! - proc2 sets proc1 priority to next highest (low)
		! - proc1 requests another block and gets blocked
		! - proc3 gets run and sets proc2 priority to 2nd highest (medium)
		! - proc2 gets all proc priorities -- should be: pid1 => 2, pid2 => 1, pid3 => 3, pid4 => 3
		! - proc2 releases a memblock
		! - proc1 gets unblocked and releases processor
		! - proc2 releases another memblock
		! - since nothing is blocked, still running proc2
		! - proc2 sets proc4 priority to highest (high)
		! - proc4 gets all priorities -- should be: pid1 => 2, pid2 => 1, pid3 => 3, pid4 => 0
		! - proc4 releases processor
		! - proc2 requests 2 memblocks -> gets first, then blocked on second
		! - proc4 is run again, requests a memblock and gets blocked
		! - proc1 is run, releases memblock
		! - since proc2 was blocked first, proc2 gets unblocked
		! - proc2 releases memblock
		! - proc4 is unblocked and sets proc1 priority to highest (high)
		- proc1 prints results
		-- TESTS END
		
TOTAL TEST CHECKS:

		- MEMORY ALLOC (block1-5_received)
		- MEMORY DEALLOC (block1-5_released)
		- PRIORITY CHECKS (all_proc_value_check_1&2)
		- PRIORITY SET CHECK (proc_1&2&4_priority_change_ret_val)
		- GENERAL SWITCH CHECK (switched_to_proc_2)
*/


#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */
#ifdef DEBUG_1
#include "printf.h"
#endif /* DEBUG_1 */

// HAVE NUM_TEST_PROCS = 4
// HAVE NUM_HEAP_BLOCKS = 4
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void set_test_procs()
{
	int i;
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid = (U32)(i + 1);
		g_test_procs[i].m_stack_size = 0x100;
	}
	
	//each memory block has a different priority
	g_test_procs[0].m_priority = LOWEST;
	g_test_procs[1].m_priority = LOWEST;
	g_test_procs[2].m_priority = LOWEST;
	g_test_procs[3].m_priority = LOWEST;
	
	//assign each test to a procedure
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[3].mpf_start_pc = &proc4;
}

int results_printed = 0,
	    block1_received = RTX_ERR,
	    block1_released = RTX_ERR,
			block2_received = RTX_ERR,
	    block2_released = RTX_ERR,
			block3_received = RTX_ERR,
	    block3_released = RTX_ERR,
			block4_received = RTX_ERR,
	    block4_released = RTX_ERR,
			proc_1_priority_change_ret_val = RTX_ERR,
			proc_2_priority_change_ret_val = RTX_ERR,
			all_proc_value_check_1 = RTX_ERR,
			block5_received = RTX_ERR,
			block5_released = RTX_ERR,
			switched_to_proc_2 = RTX_ERR,
			proc_4_priority_change_ret_val = RTX_ERR,
			all_proc_value_check_2 = RTX_ERR,
			mem_alloc_result = RTX_ERR,
			mem_dealloc_result = RTX_ERR,
			priority_check_result = RTX_ERR,
			priority_set_check_result = RTX_ERR,
			num_tests_passed = 0,
			num_tests_failed = 0;

//Have proc1 ask for two memory blocks upfront, another one to block later
void proc1(void)
{
	void* memblock1 = NULL;
	void* memblock2 = NULL;
	void* memblock3 = NULL;
	
	// print test init statements
	if (!results_printed) {
#ifdef DEBUG_1
		printf("G023_test: START\r\n");
		printf("G023_test: total 5 tests\r\n");
#endif  /* DEBUG_1 */
	}
	
	while (1) {
#ifdef DEBUG_0
		printf("proc1 requesting 2 memory blocks\r\n");
#endif /* DEBUG_0 */
		
		// request 2 blocks, check that blocks 1 and 2 are allocated successfully and set the flags
		memblock1 = request_memory_block();
		block1_received = RTX_OK;
		memblock2 = request_memory_block();
		block2_received = RTX_OK;
		
		// pass control to proc2
		release_processor();
		
		// proc2 changed priority -- now request another block and get blocked
		// control switched to proc3
#ifdef DEBUG_0
		printf("proc1 requesting 1 memory block\r\n");
#endif /* DEBUG_0 */
		memblock3 = request_memory_block();
		
		// just got assigned memblock3 after proc2 released block3
		block5_received = RTX_OK;
		
		// release the processor -> should switch to proc2
		release_processor();
		
		// proc2 and proc4 blocked, proc4 just got blocked
		// release a memory block -> should switch to proc2 seeing as it got in blocked queue first
#ifdef DEBUG_0
		printf("proc1 releasing memory\r\n");
#endif /* DEBUG_0 */
		block5_released = release_memory_block(memblock3);

#ifdef DEBUG_1
		if (!results_printed) {
			mem_alloc_result = block1_received && block2_received && block3_received && block4_received && block5_received;
			mem_dealloc_result = block1_released && block2_released && block3_released && block4_released && block5_released;
			priority_check_result = all_proc_value_check_1 && all_proc_value_check_2;
			priority_set_check_result = proc_1_priority_change_ret_val && proc_2_priority_change_ret_val && proc_4_priority_change_ret_val;
			
			if (mem_alloc_result == RTX_OK) {
				printf("G023_test: test 1 OK\r\n");
				num_tests_passed++;
			} else {
				printf("G023_test: test 1 FAIL\r\n");
				num_tests_failed++;
			}
			
			if (mem_dealloc_result == RTX_OK) {
				printf("G023_test: test 2 OK\r\n");
				num_tests_passed++;
			} else {
				printf("G023_test: test 2 FAIL\r\n");
				num_tests_failed++;
			}
			
			if (priority_check_result == RTX_OK) {
				printf("G023_test: test 3 OK\r\n");
				num_tests_passed++;
			} else {
				printf("G023_test: test 3 FAIL\r\n");
				num_tests_failed++;
			}
			
			if (priority_set_check_result == RTX_OK) {
				printf("G023_test: test 4 OK\r\n");
				num_tests_passed++;
			} else {
				printf("G023_test: test 4 FAIL\r\n");
				num_tests_failed++;
			}
			
			if (switched_to_proc_2 == RTX_OK) {
				printf("G023_test: test 5 OK\r\n");
				num_tests_passed++;
			} else {
				printf("G023_test: test 5 FAIL\r\n");
				num_tests_failed++;
			}
			
			printf("G023_test: %d/5 tests OK\r\n", num_tests_passed);
			printf("G023_test: %d/5 tests FAIL\r\n", num_tests_failed);

			printf("G023_test: END\r\n");
			
			results_printed = 1;
		}
#endif  /* DEBUG_1 */
	}
}

//proc 2 will ask for 2 blocks of memory, then another 2
void proc2(void)
{
	void* memblock1 = NULL;
	void* memblock2 = NULL;
	
	while ( 1) {
	
#ifdef DEBUG_0
		printf("proc2 requesting 2 memory blocks\r\n");
#endif /* DEBUG_0 */
		
		// request 2 blocks, check that blocks 1 and 2 are allocated successfully and set the flags
		memblock1 = request_memory_block();
		block3_received = RTX_OK;
		memblock2 = request_memory_block();
		block4_received = RTX_OK;
		
		// set proc1 priority to a higher priority than the rest (lowest->low)
		// should change back to proc1 after running this
		proc_1_priority_change_ret_val = set_process_priority(1, LOW);
		
		// return here after setting proc2 priority to MEDIUM
		// verify that proc1 and proc2 priorities changed, proc3 and proc4 priorities are still LOWEST
		if (get_process_priority(1) == LOW && get_process_priority(2) == MEDIUM && get_process_priority(3) == LOWEST && get_process_priority(4) == LOWEST) {
			all_proc_value_check_1 = RTX_OK;
		}
		
		// release a memblock --> should unblock proc1
#ifdef DEBUG_0
		printf("proc2 releasing memory\r\n");
#endif /* DEBUG_0 */
		block3_released = release_memory_block(memblock1);
		
		// proc1 just released processor
		switched_to_proc_2 = RTX_OK;
		
		// release another memblock
#ifdef DEBUG_0
		printf("proc2 releasing memory\r\n");
#endif /* DEBUG_0 */
		block4_released = release_memory_block(memblock2);
		
		// set proc4 priority to highest (HIGH)
		// should switch to proc4
		proc_4_priority_change_ret_val = set_process_priority(4, HIGH);
		
		// proc4 just released processor
		// request 2 blocks of memory -> get first, blocked on second
		// since proc2 is blocked, return to proc4 (highest priority)
#ifdef DEBUG_0
		printf("proc2 requesting 2 memory blocks\r\n");
#endif /* DEBUG_0 */
		memblock1 = request_memory_block();
		memblock2 = request_memory_block();
		
		// proc1 released a block so proc2 just got unblocked
		// release a memory block
		// proc4 gets unblocked and called
#ifdef DEBUG_0
		printf("proc2 releasing memory\r\n");
#endif /* DEBUG_0 */
		release_memory_block(memblock1);
	}
}

//proc 3 will not ask for any memory
void proc3(void)
{
	while (1) {
		// just arriving from proc1 getting blocked
		// set proc2 priority to MEDIUM
		// proc2 should be called next
#ifdef DEBUG_0
		printf("changing priority of proc2 to MEDIUM\r\n");
#endif /* DEBUG_0 */
		proc_2_priority_change_ret_val = set_process_priority(2, MEDIUM);
	}
}

//proc 4 asking for one block of memory -> get blocked
void proc4(void)
{
	void* memblock1 = NULL;
	
	while ( 1) {
	
		// should be executing just after switching priority of proc4 to HIGH -- check all priorities again
		if (get_process_priority(1) == LOW && get_process_priority(2) == MEDIUM && get_process_priority(3) == LOWEST && get_process_priority(4) == HIGH) {
			all_proc_value_check_2 = RTX_OK;
		}
		
		// release the processor --> should switch to proc2, which has priority MEDIUM
		release_processor();
		
		// proc2 just blocked itself
		// request a memory block -> get blocked at this point
#ifdef DEBUG_0
		printf("proc4 requesting memory\r\n");
#endif /* DEBUG_0 */
		memblock1 = request_memory_block();
		
		// unblocked after proc2 releases a memory block
		// set priority of proc1 to HIGH --> switch to proc1 so we can print test results
		set_process_priority(1, HIGH);
	}
}
