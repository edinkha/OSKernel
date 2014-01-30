// #include "rtx.h"
// #include "uart_polling.h"
// #include "usr_proc.h"

// #ifdef DEBUG_0
// #include "printf.h"
// #endif /* DEBUG_0 */

// /* initialization table item */
// PROC_INIT g_test_procs[NUM_TEST_PROCS];

// void set_test_procs() {
// 	int i;
//
// 	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
// 		g_test_procs[i].m_pid=(U32)(i+1);
// 		g_test_procs[i].m_priority=LOWEST;
// 		g_test_procs[i].m_stack_size=0x100;
// 	}
//
// 	g_test_procs[0].mpf_start_pc = &proc1;
// 	g_test_procs[1].mpf_start_pc = &proc2;
// // 	g_test_procs[2].mpf_start_pc = &proc3;
// // 	g_test_procs[3].mpf_start_pc = &proc4;
// // 	g_test_procs[4].mpf_start_pc = &proc5;
// }


// /**
//  * @brief: a process that prints five uppercase letters
//  *         and then yields the cpu.
//  */
// void proc1(void)
// {
// 	void* memblock1 = NULL;
// 	void* memblock2 = NULL;

// 	int ret_val = 20;
// 	while ( 1) {

// 		#ifdef DEBUG_0
// 			printf("proc1 requesting memory block 1\r\n");
// 		#endif /* DEBUG_0 */
// 		memblock1 = request_memory_block();
//
// 		#ifdef DEBUG_0
// 			printf("proc1 requesting memory block 2\r\n");
// 		#endif /* DEBUG_0 */
// 		memblock2 = request_memory_block();
//
// 		// Let process 2 execute
// 		ret_val = release_processor();
//
// 		// Come back here once process 2 gets blocked
// 		#ifdef DEBUG_0
// 			printf("proc1 releasing memory block 1\r\n");
// 		#endif /* DEBUG_0 */
// 		release_memory_block(memblock1);
//
// 		// Process 2 gets to run now because it's unblocked... Therefore it runs, then execution returns here
// 		// after it obtains a mem block
// 		#ifdef DEBUG_0
// 			printf("proc1 releasing memory block 2\r\n");
// 		#endif /* DEBUG_0 */
// 		release_memory_block(memblock2);
// 	}
// }

// /**
//  * @brief: a process that prints five numbers
//  *         and then yields the cpu.
//  */
// void proc2(void)
// {
// 	void* memblock3 = NULL;

// 	int ret_val = 20;
// 	while ( 1) {

// 		// No memory blocks available, proc 2 gets blocked here...
// 		#ifdef DEBUG_0
// 			printf("proc2 requesting memory block 3\r\n");
// 		#endif /* DEBUG_0 */
// 		memblock3 = request_memory_block();
//
// 		// Once it gets unblocked and takes the memory block it requested above, proc1 gets to run again
// 		ret_val = release_processor();
//
// 		#ifdef DEBUG_0
// 			printf("proc2 releasing memory block 3\r\n");
// 		#endif /* DEBUG_0 */
// 		release_memory_block(memblock3);
// 	}
// }

/*
HOPEFUL OUTPUT:

        - each of the blocks has a different priority set
        - proc1 requests 1 block and gets it
        - proc2 requests 2 blocks and gets them
        - proc3 requests 2 blocks and gets one of them while
        mem block2 is blocked
        - proc 4 requests 2 block and gets blocked

        - proc1 releases its memory block
        - proc3 gets unblocked
        - proc 2 releases 1 mem block
        - proc 4 gets unblocked

        - proc 4 gets a memory block
        - proc 3 gets a memory block
*/


#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

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
	g_test_procs[0].m_priority = HIGH;
	g_test_procs[1].m_priority = HIGH;
	g_test_procs[2].m_priority = HIGH;
	g_test_procs[3].m_priority = HIGH;
	
	//assign each test to a procedure
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[3].mpf_start_pc = &proc4;
}

//Have proc1 ask for one memory block
void proc1(void)
{
	int results_printed = 0,
	    block_received = RTX_ERR,
	    block_released = RTX_ERR;
	void* memblock1 = NULL;
	
	int ret_val = 20;
	while (1) {
#ifdef DEBUG_0
		printf("proc1 requesting 1 memory block\r\n");
#endif /* DEBUG_0 */
		
		memblock1 = request_memory_block();
		block_received = RTX_OK;
		ret_val = release_processor();
		
#ifdef DEBUG_0
		printf("proc1 releasing memory\r\n");
#endif /* DEBUG_0 */
		
		block_released = release_memory_block(memblock1);
		
		if (!results_printed) {
			if (block_received == RTX_OK && block_released == RTX_OK) {
#ifdef DEBUG_1
				printf("Test 2/2 Passed\r\n");
#endif  /* DEBUG_1 */
			}
			else if (block_received == RTX_ERR && block_released == RTX_ERR) {
#ifdef DEBUG_1
				printf("Test 0/2 Passed\r\n");
#endif  /* DEBUG_1 */
			}
			else {
#ifdef DEBUG_1
				printf("Test 1/2 Passed\r\n");
#endif  /* DEBUG_1 */
			}
		}
	}
}

//proc 2 will ask for 2 blocks of memory
void proc2(void)
{
	void* memblock1 = NULL;
	void* memblock2 = NULL;
	
	int ret_val = 20;
	while ( 1) {
	
#ifdef DEBUG_0
		printf("proc2 requesting 2 memory blocks\r\n");
#endif /* DEBUG_0 */
		
		memblock1 = request_memory_block();
		memblock2 = request_memory_block();
		ret_val = release_processor();
		
#ifdef DEBUG_0
		printf("proc2 releasing memory\r\n");
#endif /* DEBUG_0 */
		
		release_memory_block(memblock1);
		release_memory_block(memblock2);
	}
}

//proc 3 will also ask for 2 blocks of memory
void proc3(void)
{
	void* memblock1 = NULL;
	void* memblock2 = NULL;
	
	int ret_val = 20;
	while (1) {
	
#ifdef DEBUG_0
		printf("proc3 requesting 2 memory blocks\r\n");
#endif /* DEBUG_0 */
		
		memblock1 = request_memory_block();
		memblock2 = request_memory_block();
		ret_val = release_processor();
		
#ifdef DEBUG_0
		printf("proc3 releasing memory\r\n");
#endif /* DEBUG_0 */
		
		release_memory_block(memblock1);
		release_memory_block(memblock2);
	}
}

//proc 4 asking for one block of memory
void proc4(void)
{
	void* memblock1 = NULL;
	int ret_val = 20;
	
	while ( 1) {
	
#ifdef DEBUG_0
		printf("proc4 requesting memory\r\n");
#endif /* DEBUG_0 */
		
		memblock1 = request_memory_block();
		ret_val = release_processor();
		
#ifdef DEBUG_0
		printf("proc4 releasing memory\r\n");
#endif /* DEBUG_0 */
		
		release_memory_block(memblock1);
	}
}