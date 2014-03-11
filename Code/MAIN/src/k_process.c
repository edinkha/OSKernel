/**
 * @file:   k_process.c  
 * @brief:  process management C file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/01/17
 * NOTE: The example code shows one way of implementing context switching.
 *       The code only has minimal sanity check. There is no stack overflow check.
 *       The implementation assumes only two simple user processes and NO HARDWARE INTERRUPTS. 
 *       The purpose is to show how context switch could be done under stated assumptions. 
 *       These assumptions are not true in the required RTX Project!!!
 *       If you decide to use this piece of code, you need to understand the assumptions and
 *       the limitations. 
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"
#include "queue.h"

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
	
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[i].m_pid = g_test_procs[i].m_pid;
		g_proc_table[i].m_priority = g_test_procs[i].m_priority;
		g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
		g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
	}
	// null process initialization
	g_proc_table[NUM_TEST_PROCS].m_pid = (U32)(0);
	g_proc_table[NUM_TEST_PROCS].m_priority = 4;
	g_proc_table[NUM_TEST_PROCS].m_stack_size = 0x100;
	g_proc_table[NUM_TEST_PROCS].mpf_start_pc = &nullproc;
  
	/* initilize exception stack frame (i.e. initial context) for each process */
	for ( i = 0; i < NUM_PROCS; i++ ) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
		(gp_pcbs[i])->m_state = NEW;
		
		sp = alloc_stack((g_proc_table[i]).m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		(gp_pcbs[i])->mp_sp = sp;
	}
	
	/* put each process (minus null process) in the ready queue */
	for (i = 0; i < NUM_TEST_PROCS; i++) {
		PCB* process = gp_pcbs[i];
		push(ready_pq, (QNode *)process, process->m_priority);
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
	
	if ((top_pcb != NULL && top_pcb->m_priority <= gp_current_process->m_priority) || gp_current_process->m_state == BLOCKED || gp_current_process->m_state == WAIT_FOR_MSG) {
		next_pcb = (PCB*)pop(ready_pq);
	}
	else {
		next_pcb = gp_current_process;
	}
	
	// if the priority queue is empty, execute the null process; otherwise, execute next highest priority process
	if (next_pcb == NULL) {
		return gp_pcbs[NUM_TEST_PROCS];
	}
	else {
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

	//Set the old process's state to READY and put itback in the ready queue if it's not the null process
	p_pcb_old->m_state = READY;
	if (p_pcb_old != gp_pcbs[NUM_TEST_PROCS]) {
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
	if (gp_current_process->m_state == NEW) {
		if (gp_current_process != p_pcb_old && p_pcb_old->m_state != NEW) {
			p_pcb_old->mp_sp = (U32*)__get_MSP(); //Save the old process's sp
			configure_old_pcb(p_pcb_old); //Configure the old PCB
		}
		gp_current_process->m_state = RUNNING;
		__set_MSP((U32) gp_current_process->mp_sp);
		__rte();  // pop exception stack frame from the stack for a new processes
	}
	
	/* The following will only execute if the if block above is FALSE */
	
	if (gp_current_process != p_pcb_old) {
		//If the new current process isn't in the READY state,
		//revert to the old process and return an error
		if (gp_current_process->m_state != READY) {
			gp_current_process = p_pcb_old;
			return RTX_ERR;
		}
		
		p_pcb_old->mp_sp = (U32*)__get_MSP(); //Save the old process's sp
		configure_old_pcb(p_pcb_old); //Configure the old PCB
		
		//Run the new current process
		gp_current_process->m_state = RUNNING;
		__set_MSP((U32) gp_current_process->mp_sp); //Switch to the new proc's stack
	}
	
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
	
	p_pcb_old = gp_current_process;
	gp_current_process = scheduler();
	
	if (gp_current_process == NULL) {
		gp_current_process = p_pcb_old; // revert back to the old process
		return RTX_ERR;
	}
	if (p_pcb_old == NULL) {
		p_pcb_old = gp_current_process;
	}
	process_switch(p_pcb_old);
	return RTX_OK;
}

/**
 * @brief: Finds the PCB with the given PID and returns a pointer to it
 * @return: A pointer to the PCB with the given PID
 */
PCB* get_proc_by_pid(int pid)
{
	int i;
	for (i = 0; i < NUM_TEST_PROCS; i++) {
		if (gp_pcbs[i]->m_pid == pid) {
			return gp_pcbs[i];
		}
	}
	return NULL; //Error
}

int k_get_process_priority(int pid)
{
	PCB* pcb = get_proc_by_pid(pid);
	if (pcb == NULL) {
		return RTX_ERR;
	}
	return pcb->m_priority;
}

int k_set_process_priority(int pid, int priority)
{
	PCB* pcb;
	
	if (priority < 0 || priority > 3) {
		return RTX_ERR;
	}
	
	pcb = get_proc_by_pid(pid);
	if (pcb == NULL) {
		return RTX_ERR;
	}
	
	if (pcb->m_priority == priority) { //Nothing to change
		return RTX_OK;
	}
	
	switch (pcb->m_state) {
		case NEW:
		case READY:
			//Move the process to its new location in the priority queue based on its new priority
			if (!remove_at_priority(ready_pq, (QNode*)pcb, pcb->m_priority)) {
				return RTX_ERR;
			}
			push(ready_pq, (QNode*)pcb, priority);
		default: //Add other states above if there are states where other work should be done
			pcb->m_priority = priority;
			k_release_processor();
			break;
	}
	
	return RTX_OK;
}

/**
 * @brief: Sends message_envelope to process_id (i.e. add the message defined at message_envelope to process_id's message queue
 * @return: RTX_OK upon success
 *          RTX_ERR upon failure
 */
int send_message(int process_id, void *message_envelope){
	PCB* receiving_proc;
	ENVELOPE_HEADER* header_start;
	
	// atomic(on) -- i.e. prevent interrupts from affecting state
	__disable_irq();
	
	// error checking
	if (message_envelope == NULL || process_id < 0 || process_id > 15) {
		__enable_irq();
		return RTX_ERR;
	}
	
	// set sender and receiver proc_ids in the message_envelope memblock
	header_start = (ENVELOPE_HEADER *)((MSG_ENVELOPE *) message_envelope - sizeof(ENVELOPE_HEADER));
	((MSG_ENVELOPE *) header_start)->header.sender_pid = ((PCB *)gp_current_process)->m_pid;
	((MSG_ENVELOPE *) header_start)->header.destination_pid = process_id;
	
	receiving_proc = get_proc_by_pid(process_id);

	// error checking
	if (receiving_proc == NULL) {
		__enable_irq();
		return RTX_ERR;
	}
	
	// enqueue message_envelope onto the message_q of receiving_proc;
	enqueue(receiving_proc->m_message_q, (QNode *) message_envelope);

	if (receiving_proc->m_state == WAIT_FOR_MSG) {
		receiving_proc->m_state = READY;
		//Move the process from the blocked queue back to the ready queue
		if (!remove_at_priority(blocked_waiting_pq, (QNode*) receiving_proc, receiving_proc->m_priority)) {
			__enable_irq();
			return RTX_ERR;
		}
		push(ready_pq, (QNode *) receiving_proc, receiving_proc->m_priority);
		// handle preemption
		k_release_processor();
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
void *k_receive_message(int *sender_id){	
	void *message_envelope;
	// atomic(on)
	__disable_irq();
	
	while (q_empty(gp_current_process->m_message_q)) {
		gp_current_process->m_state = WAIT_FOR_MSG;
		push(blocked_waiting_pq, (QNode *) gp_current_process, gp_current_process->m_priority);
		k_release_processor();
	}

	message_envelope = dequeue(gp_current_process->m_message_q);
	
	// atomic(off)
	__enable_irq();
	
	return message_envelope;
}





