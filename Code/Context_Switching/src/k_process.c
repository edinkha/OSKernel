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
PCB *gp_current_process = NULL; /* always point to the current RUN process */

/* process initialization table */
PROC_INIT g_proc_table[NUM_TEST_PROCS + 1];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];

/**
 * @biref: initialize all processes in the system
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
	for ( i = 0; i < NUM_TEST_PROCS + 1; i++ ) {
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

/*@brief: scheduler, pick the pid of the next to run process
 *@return: PCB pointer of the next to run process
 *         NULL if error happens
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */

PCB *scheduler(void)
{
	PCB* next_pcb = (PCB *)pop(ready_pq);
	
	// if the priority queue is empty, execute the null process; otherwise, execute next highest priority process
	if (next_pcb == NULL) {
		return gp_pcbs[NUM_TEST_PROCS];
	} else {
		return next_pcb;
	}
}

/* @brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 * @param: p_pcb_old, the old pcb that was in RUN
 * @return: RTX_OK upon success
 *          RTX_ERR upon failure
 * PRE:  p_pcb_old and gp_current_process are pointing to valid PCBs.
 * POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *       No other effect on other global variables.
 */
int process_switch(PCB* p_pcb_old)
{
	if (gp_current_process->m_state == NEW) {
		if (gp_current_process != p_pcb_old && p_pcb_old->m_state != NEW) {
			p_pcb_old->m_state = READY;
			p_pcb_old->mp_sp = (U32*) __get_MSP();
			// put the old process back in the ready queue if it's not the null process
			if (p_pcb_old != gp_pcbs[NUM_TEST_PROCS]) {
				push(ready_pq, (QNode*)p_pcb_old, p_pcb_old->m_priority);
			}
		}
		gp_current_process->m_state = RUN;
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
		
		p_pcb_old->mp_sp = (U32*) __get_MSP();  // save the old process's sp
		
		//If the old process isn't blocked or waiting for a message, set it to ready
		if (p_pcb_old->m_state != BLOCKED && p_pcb_old->m_state != WAIT_FOR_MSG) {
			p_pcb_old->m_state = READY;
			//Put the old process back in the ready queue if it's not the null process
			if (p_pcb_old != gp_pcbs[NUM_TEST_PROCS]) {
				push(ready_pq, (QNode*)p_pcb_old, p_pcb_old->m_priority);
			}
		}

		//Run the new current process
		gp_current_process->m_state = RUN;
		__set_MSP((U32) gp_current_process->mp_sp); //switch to the new proc's stack
	}
	
	return RTX_OK;
}
/**
 * @brief release_processor(). 
 * @return RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */
int k_release_processor(void)
{
	PCB *p_pcb_old = NULL;
	
	p_pcb_old = gp_current_process;
	gp_current_process = scheduler();
	
	if ( gp_current_process == NULL  ) {
		gp_current_process = p_pcb_old; // revert back to the old process
		return RTX_ERR;
	}
  if ( p_pcb_old == NULL ) {
		p_pcb_old = gp_current_process;
	}
	process_switch(p_pcb_old);
	return RTX_OK;
}

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

int get_proc_priority(int pid)
{
	if (get_proc_by_pid(pid) == NULL) {
		return RTX_ERR;
	}
	
	return get_proc_by_pid(pid)->m_priority;
}

int set_proc_priority(int pid, int priority)
{
	PCB* pcb;
	
	if (priority < 0 || priority > 3) {
		return RTX_ERR;
	}
	
	pcb = get_proc_by_pid(pid);
	if (pcb == NULL) {
		return RTX_ERR;
	}
	
	if (pcb->m_priority == priority) {
		return RTX_OK;
	}
	
	switch(pcb->m_state) {
		case NEW:
		case READY:
			// TODO: basically, need to remove from one priority queue and move to another
			push(ready_pq, remove(ready_pq, pcb->m_priority, (void*)pcb), priority);
		default: //Add other states above if there are states where other work should be done
			pcb->m_priority = priority;
			break;
	}
	
	return RTX_OK;
}

void nullproc(void)
{
	while(1) {
		k_release_processor();
	}
}
