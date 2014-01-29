/*
HOPEFUL OUTPUT:

        - each of the blocks has a different priority set 
        - proc1 requests 1 block and gets it
        - proc2 requests 2 blocks and gets them
        - proc3 requests 2 blocks and gets one of them while 
        mem block2 is blocked
        - proc 4 requests 2 block and gets blocked

        - proc1 releases its memory block
        - proc4 gets unblocked
        - proc 2 releases 2 mem blocks
        - proc 3 gets unblocked

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

void set_test_procs() {


        int i;
        for( i = 0; i < NUM_TEST_PROCS; i++ ) {
           g_test_procs[0].m_pid=(U32)(i+1);
           g_test_procs[0].m_stack_size=0x100;     
        }

        //each memory block has a different priority
        g_test_procs[0].m_priority=LOWEST;
        g_test_procs[1].m_priority=LOW;
        g_test_procs[2].m_priority=MEDIUM;
        g_test_procs[3].m_priority=HIGH;
        
        //assign each test to a procedure
        g_test_procs[0].mpf_start_pc = &proc1;
        g_test_procs[1].mpf_start_pc = &proc2;
        g_test_procs[2].mpf_start_pc = &proc3;
        g_test_procs[3].mpf_start_pc = &proc4;
}

//Have proc1 ask for one memory block
void proc1(void)
{
        void* memblock1 = NULL;

        int ret_val = 20;
        while ( 1) {

#ifdef DEBUG_0
        printf("proc1 requesting 1 memory block\n");
#endif /* DEBUG_0 */

        memblock1 = request_memory_block();
        ret_val = release_processor();

#ifdef DEBUG_0
        printf("proc1 releasing memory\n");
#endif /* DEBUG_0 */

        release_memory_block(memblock1);
        
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
        printf("proc2 requesting 2 memory blocks\n");
#endif /* DEBUG_0 */

        memblock1 = request_memory_block();
        memblock2 = request_memory_block();
        ret_val = release_processor();

#ifdef DEBUG_0
        printf("proc2 releasing memory\n");
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
        printf("proc3 requesting 2 memory blocks\n");
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

//proc 4 asking for one block of memory
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
