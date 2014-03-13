/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H_
#define K_RTX_H_

#include "queue.h"
#include "priority_queue.h"

/*----- Definitations -----*/

#define RTX_ERR -1
#define RTX_OK  0

#define NULL 0
#define NUM_TEST_PROCS 2
#define NUM_PROCS 5

#define USR_SZ_MEM_BLOCK 0x80	/* heap memory block size is 128B     */
#define SZ_MEM_BLOCK_HEADER 0x0C /* memory block header size is 12B */

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200		/* user proc stack size 512B   */
#define NUM_HEAP_BLOCKS 4		/* Blocks just for debugging */
#else
#define USR_SZ_STACK 0x100		/* user proc stack size 218B  */
#endif /* DEBUG_0 */

/* Process Priority. The bigger the number is, the lower the priority is*/
#define HIGH    0
#define MEDIUM  1
#define LOW     2
#define LOWEST  3

/* Process IDs */
#define PID_NULL 0
#define PID_P1   1
#define PID_P2   2
#define PID_P3   3
#define PID_P4   4
#define PID_P5   5
#define PID_P6   6
#define PID_A    7
#define PID_B    8
#define PID_C    9
#define PID_SET_PRIO     10
#define PID_CLOCK        11
#define PID_KCD          12
#define PID_CRT          13
#define PID_TIMER_IPROC  14
#define PID_UART_IPROC   15

/* Proc Types */
#define REG_PROC 0
#define TIMER_IPROC 14
#define UART_IPROC 15

/* Message Types */
#define DEFAULT 0
#define KCD_REG 1
#define CRT_DISPLAY 2

/*----- Types -----*/
typedef unsigned char U8;
typedef unsigned int U32;

/* process states */
typedef enum {
	NEW = 0,
	READY,
	BLOCKED,
	BLOCKED_ON_RECEIVE,
	RUNNING,
	INTERRUPTED
} PROC_STATE_E;  

/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project 
*/
typedef struct pcb 
{ 
	struct pcb *mp_next;	/* next pcb */
	U32 *mp_sp;				/* stack pointer of the process */
	U32 m_pid;				/* process id */
	U32 m_priority;
	PROC_STATE_E m_state;	/* state of the process */   
	Queue m_message_q;
} PCB;

/* initialization table item */
typedef struct proc_init
{	
	int m_pid;				/* process id */ 
	int m_priority;			/* initial priority, not used in this example. */ 
	int m_stack_size;		/* size of stack in words */
	void (*mpf_start_pc)();	/* entry point of the process */    
} PROC_INIT;

typedef struct msg_envelope
{
	struct msg_envelope *next;
	U32 sender_pid;
	U32 destination_pid;
	int mtype;              /* user defined message type */
	U8 mtext[1];            /* body of the message */
} MSG_ENVELOPE;

/* message buffer */
typedef struct msgbuf
{
	int mtype;              /* user defined message type */
	U8 mtext[1];            /* body of the message */
} MSG_BUF;

typedef struct mem_block
{
	U32 *next_block;
} MEM_BLOCK;

extern PriorityQueue *blocked_memory_pq;
extern PriorityQueue *blocked_waiting_pq;
extern PriorityQueue *ready_pq;

/* ----- RTX User API ----- */
#define __SVC_0  __svc_indirect(0)

/* RTX initialization */
extern void k_rtx_init(void);
#define rtx_init() _rtx_init((U32)k_rtx_init)
extern void __SVC_0 _rtx_init(U32 p_func);

/* Processor Management */
extern int k_release_processor(void);
#define release_processor() _release_processor((U32)k_release_processor)
extern int __SVC_0 _release_processor(U32 p_func);

extern int k_get_process_priority(int pid);
#define get_process_priority(pid) _get_process_priority((U32)k_get_process_priority, pid)
extern int _get_process_priority(U32 p_func, int pid) __SVC_0;
/* __SVC_0 can also be put at the end of the function declaration */

extern int k_set_process_priority(int pid, int prio);
#define set_process_priority(pid, prio) _set_process_priority((U32)k_set_process_priority, pid, prio)
extern int _set_process_priority(U32 p_func, int pid, int prio) __SVC_0;

/* Memeory Management */
extern void *k_request_memory_block(void);
#define request_memory_block() _request_memory_block((U32)k_request_memory_block)
extern void *_request_memory_block(U32 p_func) __SVC_0;


extern int k_release_memory_block(void *);
#define release_memory_block(p_mem_blk) _release_memory_block((U32)k_release_memory_block, p_mem_blk)
extern int _release_memory_block(U32 p_func, void *p_mem_blk) __SVC_0;

/* IPC Management */
extern int k_send_message(int pid, void *p_msg);
#define send_message(pid, p_msg) _send_message((U32)k_send_message, pid, p_msg)
extern int _send_message(U32 p_func, int pid, void *p_msg) __SVC_0;

extern void *k_receive_message(int *p_pid);
#define receive_message(p_pid) _receive_message((U32)k_receive_message, p_pid)
extern void *_receive_message(U32 p_func, void *p_pid) __SVC_0;

/* Timing Service */
extern int k_delayed_send(int pid, void *p_msg, int delay);
#define delayed_send(pid, p_msg, delay) _delayed_send((U32)k_delayed_send, pid, p_msg, delay)
extern int _delayed_send(U32 p_func, int pid, void *p_msg, int delay) __SVC_0; 

#endif // ! K_RTX_H_
