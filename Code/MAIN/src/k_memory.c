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
extern ForwardList* delayed_messages; // List of delayed messages

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
	#ifndef DEBUG_CUSTOM_HEAP
		U32* stack_end_addr;
	#endif
	
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += NUM_PROCS * sizeof(PCB *);
  
	for ( i = 0; i < NUM_PROCS; i++ ) {
		gp_pcbs[i] = (PCB *)p_end;
		p_end += sizeof(PCB);
	}
	
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

	// Allocate memory for the list of delayed messages
	delayed_messages = (ForwardList*)p_end; 
	p_end += sizeof(ForwardList);
	init(delayed_messages);
  
	/* allocate memory for the heap */
	
	// Assign memory for the heap structure (list of memory blocks)
	heap = (ForwardList *)p_end; 
	p_end += sizeof(ForwardList);
	init(heap);
	
	// Build the heap
	#ifdef DEBUG_CUSTOM_HEAP
		for (i = 0; i < NUM_HEAP_BLOCKS; i++) {
	#else
		// Find the address of the end of the stack
		// USR_SZ_STACK + 1 to take into account the 8-byte alignment
		// -1 for extra padding
		stack_end_addr = (U32 *)(RAM_END_ADDR - ((NUM_PROCS) * (USR_SZ_STACK + 1)) - 1);
			
		while (p_end + USR_SZ_MEM_BLOCK < (U8 *)stack_end_addr) {
	#endif
		heap_block = (MEM_BLOCK *)p_end;
		push_front(heap, (ListNode *)heap_block);
		p_end += USR_SZ_MEM_BLOCK;
	}
	
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

void *k_request_memory_block(void)
{
	__disable_irq(); // atomic(on)

	//While there are no memory blocks left on the heap, block the current process
	while (empty(heap)) {
	#ifdef DEBUG_0 
		printf("Process %d blocked \r\n", gp_current_process->m_pid);
	#endif
		gp_current_process->m_state = BLOCKED;
		push(blocked_memory_pq, (QNode *) gp_current_process, gp_current_process->m_priority);
		k_release_processor();
	}
	
	__enable_irq(); // atomic(off)
	
	//Pop a memory block off the heap and return a pointer to its content
	return (void*)((U8*)pop_front(heap) + SZ_MEM_BLOCK_HEADER);
}

/**
 * The non-blocking version of k_request_memory_block for i-processes
 */
void* ki_request_memory_block(void)
{
	// If there are no memory blocks left on the heap, return a null pointer
	if (empty(heap)) {
		return (void*)0;
	}
	// Pop a memory block off the heap and return a pointer to its content
	return (void*)((U8*)pop_front(heap) + SZ_MEM_BLOCK_HEADER);
}

int k_release_memory_block(void *p_mem_blk)
{
	__disable_irq(); // atomic(on)

	//Return an error if the input memory block is not valid
	if (p_mem_blk == NULL) {
		__enable_irq();
		return RTX_ERR;
	}

	// Put the memory block back onto the heap
	push_front(heap, (ListNode*)((U8*)p_mem_blk - SZ_MEM_BLOCK_HEADER));

	// If the blocked queue is not empty, take the first process and put it on the ready queue
	// (since now there is memory available for that process to continue)
	if (!pq_empty(blocked_memory_pq)) {
		PCB* proc_to_unblock = (PCB*)pop(blocked_memory_pq);
		proc_to_unblock->m_state = READY;
	#ifdef DEBUG_0 
		printf("Unblocking process ID %x\r\n", proc_to_unblock->m_pid);
	#endif
		push(ready_pq, (QNode*)proc_to_unblock, proc_to_unblock->m_priority);
		// Only preempt if the current process is not an i-process
		if (!gp_current_process->m_is_iproc) {
			k_release_processor();
		}
	}	
	
	// Only re-enable irq if the current process is not an i-process
	if (!gp_current_process->m_is_iproc) {
		__enable_irq(); // atomic(off)
	}
	
	return RTX_OK;
}
