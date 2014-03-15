/**
 * @file:   k_memory.c
 * @brief:  kernel memory management routines
 */

#include "k_memory.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

extern int k_release_processor(void);

/* ----- Global Variables ----- */
U32 *gp_stack; /* The last allocated stack low address. 8 bytes aligned */
               /* The first stack starts at the RAM high address */
	       /* stack grows down. Fully decremental stack */
ForwardList* heap; // Pointer to the heap
PriorityQueue* ready_pq; // Ready queue to hold the PCBs
PriorityQueue* blocked_memory_pq; // Blocked priority queue to hold PCBs blocked due to memory
PriorityQueue* blocked_waiting_pq; // Blocked priority queue to hold PCBs blocked due to waiting for a message

/**
 * @brief: Initialize RAM as follows:

0x10008000+---------------------------+ High Address
          |    Proc 1 STACK           |
          |---------------------------|
          |    Proc 2 STACK           |
          |---------------------------|<--- gp_stack
          |                           |
          |        HEAP               |
          |                           |
          |---------------------------|<--- p_end (before heap alloc)
          |        PCB 2              |
          |---------------------------|
          |        PCB 1              |
          |---------------------------|
          |        PCB pointers       |
          |---------------------------|<--- gp_pcbs
          |        Padding            |
          |---------------------------|  
          |Image$$RW_IRAM1$$ZI$$Limit |
          |...........................|          
          |       RTX  Image          |
          |                           |
0x10000000+---------------------------+ Low Address

*/

void memory_init(void)
{
	U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
	int i;
	MEM_BLOCK* heap_block;
	U32* stack_end_addr;
	
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += NUM_PROCS * sizeof(PCB *);
  
	for ( i = 0; i < NUM_PROCS; i++ ) {
		gp_pcbs[i] = (PCB *)p_end;
		p_end += sizeof(PCB);
	}
#ifdef DEBUG_0  
	printf("gp_pcbs[0] = 0x%x \r\n", gp_pcbs[0]);
	printf("gp_pcbs[1] = 0x%x \r\n", gp_pcbs[1]);
#endif
	
	/* prepare for alloc_stack() to allocate memory for stacks */	
	gp_stack = (U32 *)RAM_END_ADDR;
	if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
		--gp_stack; 
	}
	
	/* allocate memory for ready and blocked queues */
	ready_pq = (PriorityQueue *)p_end;
	p_end += sizeof(PriorityQueue);
	init_pq(ready_pq);
	
	blocked_memory_pq = (PriorityQueue *)p_end;
	p_end += sizeof(PriorityQueue);
	init_pq(blocked_memory_pq);
	
	blocked_waiting_pq = (PriorityQueue *)p_end;
	p_end += sizeof(PriorityQueue);
	init_pq(blocked_waiting_pq);
  
	/* allocate memory for the heap */
#ifdef DEBUG_0
	printf("p_end = 0x%x \r\n", p_end);
#endif
	
	// Assign memory for the heap struct
	heap = (ForwardList *)p_end; 
	p_end += sizeof(ForwardList);
	init(heap);
	
	//Find the address of the end of the stack
	//USR_SZ_STACK + 1 to take into account the 8-byte alignment
	//-1 for extra padding
	stack_end_addr = (U32 *)(RAM_END_ADDR - ((NUM_PROCS) * (USR_SZ_STACK + 1)) - 1);
	
	// Build the heap
#ifdef DEBUG_0
	for (i = 0; i < NUM_HEAP_BLOCKS; i++) {
#else
	while (p_end + USR_SZ_MEM_BLOCK < (U8 *)stack_end_addr) {
#endif
		heap_block = (MEM_BLOCK *)p_end;
		push_front(heap, (ListNode *)heap_block);
		p_end += USR_SZ_MEM_BLOCK;
	}
	
#ifdef DEBUG_0
	printf("p_end = 0x%x \r\n", p_end);
#endif
	
}

/**
 * @brief: allocate stack for a process, align to 8 bytes boundary
 * @param: size, stack size in bytes
 * @return: The top of the stack (i.e. high address)
 * POST:  gp_stack is updated.
 */

U32 *alloc_stack(U32 size_b) 
{
	U32 *sp;
	sp = gp_stack; /* gp_stack is always 8 bytes aligned */
	
	/* update gp_stack */
	gp_stack = (U32 *)((U8 *)sp - size_b);
	
	/* 8 bytes alignement adjustment to exception stack frame */
	if ((U32)gp_stack & 0x04) {
		--gp_stack; 
	}
	return sp;
}

void *k_request_memory_block(void) {
	// atomic(on)
	__disable_irq();
#ifdef DEBUG_0 
	printf("k_request_memory_block: entering...\r\n");
#endif
	//While there are no memory blocks left on the heap, block the current process
	while (empty(heap)) {
		if (gp_current_process->m_is_iproc) {
			__enable_irq();
			return (void*)0;
		}
	#ifdef DEBUG_0 
		printf("process %d blocked \r\n", gp_current_process->m_pid);
	#endif
		gp_current_process->m_state = BLOCKED;
		push(blocked_memory_pq, (QNode *) gp_current_process, gp_current_process->m_priority);
		k_release_processor();
	}
	#ifdef DEBUG_0 
		printf("k_request_memory_block: returning new block...\r\n");
	#endif
	
	// atomic(off)
	__enable_irq();
	
	// TODO: VERIFY THIS WORKS
	//Pop a memory block off the heap and return a pointer to its content
	return (void*)((U8*)pop_front(heap) + SZ_MEM_BLOCK_HEADER);
}

int k_release_memory_block(void *p_mem_blk) {
	QNode* to_unblock;
	
	// atomic(on)
	__disable_irq();
#ifdef DEBUG_0 
	printf("k_release_memory_block: releasing block @ 0x%x\r\n", p_mem_blk);
#endif
	//Return an error if the input memory block is not valid
	if (p_mem_blk == NULL) {
		__enable_irq();
		return RTX_ERR;
	}

	// TODO: VERIFY THIS WORKS
	//Put the memory block back onto the heap
	push_front(heap, (ListNode*)((U8*)p_mem_blk - SZ_MEM_BLOCK_HEADER));

	//If the blocked queue is not empty, take the first process and put it on the ready queue
	//(since now there is memory available for that process to continue)
	if (!pq_empty(blocked_memory_pq)) {
		to_unblock = pop(blocked_memory_pq);
		((PCB *)to_unblock)->m_state = READY;
	#ifdef DEBUG_0 
		printf("unblocking process ID %x\r\n", ((PCB *)to_unblock)->m_pid);
	#endif
		push(ready_pq, to_unblock, ((PCB *)to_unblock)->m_priority);
		// handle preemption
		k_release_processor();
	}	
	
	//atomic(off)
	__enable_irq();
	
	return RTX_OK;
}
