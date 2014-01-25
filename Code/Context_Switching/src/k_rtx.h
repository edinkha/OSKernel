/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H_
#define K_RTX_H_

/*----- Definitations -----*/

#define RTX_ERR -1
#define RTX_OK  0

#define NULL 0
#define NUM_TEST_PROCS 2

#define USR_SZ_MEM_BLOCK 0x80		 /* heap memory block size is 128B     */

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B   */
#define NUM_HEAP_BLOCKS 6         /* TODO: figure out the right number of blocks */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B  */
#define NUM_HEAP_BLOCKS 10         /* TODO: figure out the right number of blocks */
#endif /* DEBUG_0 */


/*----- Types -----*/
typedef unsigned char U8;
typedef unsigned int U32;

/* process states */
typedef enum {
	NEW = 0,
	RDY,
	BLOCKED,
	WAIT_FOR_MSG,
	RUN,
	INTERRUPTED
} PROC_STATE_E;  

/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project 
*/
typedef struct pcb 
{ 
	struct pcb *mp_next;  // next pcb
	U32 *mp_sp;		/* stack pointer of the process */
	U32 m_pid;		/* process id */
	PROC_STATE_E m_state;   /* state of the process */      
} PCB;

/* initialization table item */
typedef struct proc_init
{	
	int m_pid;	        /* process id */ 
	int m_priority;         /* initial priority, not used in this example. */ 
	int m_stack_size;       /* size of stack in words */
	void (*mpf_start_pc) ();/* entry point of the process */    
} PROC_INIT;

typedef struct mem_block
{
	U32 *next_block;
	//some memory
} MEM_BLOCK;

#endif // ! K_RTX_H_
