/**
 * @file:   k_process.c  
 * @brief:  process management C file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/01/17
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"
#include "queue.h"
#include "uart.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB **gp_pcbs;                  /* array of pcbs */
PCB *gp_current_process = NULL; /* always point to the current RUNNING process */

U32 g_switch_flag = 0;          /* whether to continue to run the process before the UART receive interrupt */
                                /* 1 means to switch to another process, 0 means to continue the current process */
																/* this value will be set by UART handler */

/* process initialization table */
PROC_INIT g_proc_table[NUM_PROCS];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];

// sys procs
extern void CRT(void);
extern void UART_IPROC(void);

extern LPC_UART_TypeDef *pUart;

/* The null process */
void nullproc(void)
{
	while(1) {
		k_release_processor();
	}
}

/**
 * @brief: initialize all processes in the system
 * NOTE: We assume there are only two user processes in the system in this example.
 */
void process_init() 
{
	int i;
	U32 *sp;
  
  /* fill out the initialization table */
	set_test_procs();
	
	// null process initialization
	g_proc_table[0].m_pid = PID_NULL;
	g_proc_table[0].m_priority = LOWEST + 1; // Give the null process a priority lower than the lowest priority
	g_proc_table[0].m_stack_size = USR_SZ_STACK;
	g_proc_table[0].mpf_start_pc = &nullproc;
	
	for (i = 0; i < NUM_TEST_PROCS; i++) {
		g_proc_table[i+1].m_pid = g_test_procs[i].m_pid;
		g_proc_table[i+1].m_priority = g_test_procs[i].m_priority;
		g_proc_table[i+1].m_stack_size = g_test_procs[i].m_stack_size;
		g_proc_table[i+1].mpf_start_pc = g_test_procs[i].mpf_start_pc;
	}
	
	// CRT process initialization
	g_proc_table[NUM_TEST_PROCS + 1].m_pid = PID_CRT;
	g_proc_table[NUM_TEST_PROCS + 1].m_priority = HIGH;
	g_proc_table[NUM_TEST_PROCS + 1].m_stack_size = USR_SZ_STACK;
	g_proc_table[NUM_TEST_PROCS + 1].mpf_start_pc = &CRT;
	
	// UART i-process initialization
	g_proc_table[NUM_TEST_PROCS + 2].m_pid = PID_UART_IPROC;
	g_proc_table[NUM_TEST_PROCS + 2].m_priority = HIGH;
	g_proc_table[NUM_TEST_PROCS + 2].m_stack_size = USR_SZ_STACK;
	g_proc_table[NUM_TEST_PROCS + 2].mpf_start_pc = &UART_IPROC;
  
	/* initialize exception stack frame (i.e. initial context) and memory queue for each process */
	for ( i = 0; i < NUM_PROCS; i++ ) {
		int j; // Used in the loop at the bottom of this loop

		gp_pcbs[i]->m_pid = g_proc_table[i].m_pid;
		gp_pcbs[i]->m_priority = g_proc_table[i].m_priority;
		// If the PID of the process is less than that of the first i-proc, it is not an i-proc
		gp_pcbs[i]->m_is_iproc = g_proc_table[i].m_pid < PID_TIMER_IPROC ? 0 : 1;
		gp_pcbs[i]->m_state = NEW;
		init_q(&gp_pcbs[i]->m_message_q);
		
		sp = alloc_stack(g_proc_table[i].m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)(g_proc_table[i].mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		gp_pcbs[i]->mp_sp = sp;
	}
	
	/* put each process (minus null process and I-processes) in the ready queue */
	for (i = 1; i < NUM_PROCS - 1; i++) {
		PCB* process = gp_pcbs[i];
		push(ready_pq, (QNode *)process, process->m_priority);
	}
	/* set i-proc states to READY */
	for (i = NUM_PROCS - 1; i < NUM_PROCS; i++) {
		gp_pcbs[i]->m_state = READY;
	}
}

/**
 * @brief: scheduler, picks the next to run process
 * @return: PCB pointer of the next to run process
 * POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *       No other effect on other global variables.
 */
PCB* scheduler(void)
{
	PCB* next_pcb;
	PCB* top_pcb = (PCB*)top(ready_pq);
	
	__disable_irq();
	
	if (gp_current_process == NULL || gp_current_process->m_is_iproc == 1 || (top_pcb != NULL && top_pcb->m_priority <= gp_current_process->m_priority) || gp_current_process->m_state == BLOCKED || gp_current_process->m_state == BLOCKED_ON_RECEIVE) {
		next_pcb = (PCB*)pop(ready_pq);
	}
	else {
		next_pcb = gp_current_process;
	}
	
	// if the priority queue is empty, execute the null process; otherwise, execute next highest priority process
	if (next_pcb == NULL) {
		__enable_irq();
		return gp_pcbs[0];
	}
	else {
		__enable_irq();
		return next_pcb;
	}
}

/**
 * @brief: Configures the old PCB if it was RUNNING or INTERRUPTED by making it READY
 */
void configure_old_pcb(PCB* p_pcb_old)
{
	//If the old process wasn't running or interrupted, there is nothing to configure, so return
	if (p_pcb_old->m_state != RUNNING && p_pcb_old->m_state != INTERRUPTED) {
		return;
	}

	//Set the old process's state to READY and put it back in the ready queue if it's not the null process or an i-proc
	p_pcb_old->m_state = READY;
	if (p_pcb_old != gp_pcbs[0] && p_pcb_old->m_is_iproc != 1) {
		push(ready_pq, (QNode*)p_pcb_old, p_pcb_old->m_priority);
	}
	
}

/**
 * @brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 * @param: p_pcb_old, the old pcb that was in RUNNING
 * @return: RTX_OK upon success
 *          RTX_ERR upon failure
 * PRE: p_pcb_old and gp_current_process are pointing to valid PCBs.
 */
int process_switch(PCB* p_pcb_old)
{
	//__disable_irq();
	if (gp_current_process->m_state == NEW) {
		if (gp_current_process != p_pcb_old && p_pcb_old->m_state != NEW) {
			if (p_pcb_old->m_is_iproc != 1) {
				p_pcb_old->mp_sp = (U32*)__get_MSP(); //Save the old process's sp
			}
			configure_old_pcb(p_pcb_old); //Configure the old PCB
		}
		gp_current_process->m_state = RUNNING;
		__enable_irq();
		__set_MSP((U32) gp_current_process->mp_sp);
		__rte();  // pop exception stack frame from the stack for a new processes
	}
	
	/* The following will only execute if the if block above is FALSE */
	
	if (gp_current_process != p_pcb_old) {
		//If the new current process isn't in the READY state,
		//revert to the old process and return an error
		if (gp_current_process->m_state != READY) {
			gp_current_process = p_pcb_old;
			__enable_irq();
			return RTX_ERR;
		}
		if (p_pcb_old->m_is_iproc != 1 && g_switch_flag) {
			p_pcb_old->mp_sp = (U32*)__get_MSP(); //Save the old process's sp
		}
		configure_old_pcb(p_pcb_old); //Configure the old PCB
		
		//Run the new current process
		gp_current_process->m_state = RUNNING;
		if (gp_current_process->m_is_iproc != 1 && g_switch_flag) {
			__enable_irq();
			__set_MSP((U32) gp_current_process->mp_sp); //Switch to the new proc's stack
		}
	}
	
	//__enable_irq();
	return RTX_OK;
}

/**
 * @brief: release_processor(). 
 * @return: RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */
int k_release_processor(void)
{
	PCB* p_pcb_old = NULL;
	__disable_irq();
	
	p_pcb_old = gp_current_process;
	gp_current_process = scheduler();
	
	if (gp_current_process == NULL) {
		gp_current_process = p_pcb_old; // revert back to the old process
		__enable_irq();
		return RTX_ERR;
	}
	if (p_pcb_old == NULL) {
		p_pcb_old = gp_current_process;
	}
	process_switch(p_pcb_old);
	__enable_irq();
	return RTX_OK;
}

/**
 * @brief: Finds the PCB with the given PID and returns a pointer to it
 * @return: A pointer to the PCB with the given PID
 */
PCB* get_proc_by_pid(int pid)
{
	int i;
	__disable_irq();
	for (i = 0; i < NUM_PROCS; i++) {
		if (gp_pcbs[i]->m_pid == pid) {
			__enable_irq();
			return gp_pcbs[i];
		}
	}
	__enable_irq();
	return NULL; //Error
}

int k_get_process_priority(int pid)
{
	PCB* pcb;
	
	__disable_irq();
	pcb = get_proc_by_pid(pid);
	
	if (pcb == NULL) {
		__enable_irq();
		return RTX_ERR;
	}
	__enable_irq();
	return pcb->m_priority;
}

int k_set_process_priority(int pid, int priority)
{
	PCB* pcb;
	
	// atomic(on)
	__disable_irq();
	
	if (priority < 0 || priority > 3) {
		__enable_irq();
		return RTX_ERR;
	}
	
	pcb = get_proc_by_pid(pid);
	if (pcb == NULL) {
		__enable_irq();
		return RTX_ERR;
	}
	
	if (pcb->m_priority == priority) { //Nothing to change
		__enable_irq();
		return RTX_OK;
	}
	
	switch (pcb->m_state) {
		case NEW:
		case READY:
			//Move the process to its new location in the priority queue based on its new priority
			if (!remove_at_priority(ready_pq, (QNode*)pcb, pcb->m_priority)) {
				__enable_irq();
				return RTX_ERR;
			}
			push(ready_pq, (QNode*)pcb, priority);
		default: //Add other states above if there are states where other work should be done
			pcb->m_priority = priority;
			k_release_processor();
			break;
	}
	__enable_irq();
	return RTX_OK;
}

/**
 * @brief: Sends message_envelope to process_id (i.e. add the message defined at message_envelope to process_id's message queue
 * @return: RTX_OK upon success
 *          RTX_ERR upon failure
 */
int k_send_message(int process_id, void *message){
	PCB* receiving_proc;
	MSG_ENVELOPE* envelope;
	
	// atomic(on) -- i.e. prevent interrupts from affecting state
	__disable_irq();
	
	// error checking
	if (message == NULL || process_id < 0) {
		__enable_irq();
		return RTX_ERR;
	}
	
	// TODO: VERIFY THIS WORKS
	// set sender and receiver proc_ids in the message_envelope memblock
	envelope = (MSG_ENVELOPE *)((U8*)message - SZ_MEM_BLOCK_HEADER);
	envelope->sender_pid = ((PCB *)gp_current_process)->m_pid;
	envelope->destination_pid = process_id;
	
	receiving_proc = get_proc_by_pid(process_id);

	// error checking
	if (receiving_proc == NULL) {
		__enable_irq();
		return RTX_ERR;
	}
	
	// TODO: VERIFY THIS WORKS
	// enqueue message_envelope onto the message_q of receiving_proc;
	enqueue(&receiving_proc->m_message_q, (QNode *)envelope);

	if (receiving_proc->m_state == BLOCKED_ON_RECEIVE) {
		receiving_proc->m_state = READY;
		//Move the process from the blocked queue back to the ready queue
		if (!remove_at_priority(blocked_waiting_pq, (QNode*) receiving_proc, receiving_proc->m_priority)) {
			__enable_irq();
			return RTX_ERR;
		}
		push(ready_pq, (QNode *) receiving_proc, receiving_proc->m_priority);
		// handle preemption
		if (gp_current_process->m_is_iproc != 1) {
			k_release_processor();
		}
	}

	// atomic(off)
	__enable_irq();
	
	return RTX_OK;
}

/**
 * NOTE: BLOCKING receive
 * @brief: Returns pointer to waiting message envelope, or blocks until a message is received
 * @return: pointer to message envelope
 */
void *k_receive_message(int* sender_id){	
	void* message;
	// atomic(on)
	__disable_irq();
	
	while (q_empty(&gp_current_process->m_message_q)) {
		gp_current_process->m_state = BLOCKED_ON_RECEIVE;
		push(blocked_waiting_pq, (QNode*)gp_current_process, gp_current_process->m_priority);
		k_release_processor();
	}

	// TODO: VERIFY THIS WORKS -- EDITED: now returns address to msgbuf rather than envelope itself
	message = (void*)((U8*)dequeue(&gp_current_process->m_message_q) + SZ_MEM_BLOCK_HEADER);
	
	// atomic(off)
	__enable_irq();
	
	return message;
}
