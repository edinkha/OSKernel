/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H_
#define K_RTX_H_

#include <stdint.h>
#include "forward_list.h"
#include "queue.h"
#include "priority_queue.h"

/*----- Definitations -----*/
#define RTX_ERR -1
#define RTX_OK  0
#define NULL 0
#define NUM_TEST_PROCS 6
#define NUM_STRESS_PROCS 3
#define NUM_PROCS 16

#ifdef DEBUG_CUSTOM_HEAP
#define NUM_HEAP_BLOCKS 9
#endif /* DEBUG_CUSTOM_HEAP */

#define USR_SZ_MEM_BLOCK 0x80    /* heap memory block size is 128 B */
#define SZ_MEM_BLOCK_HEADER 0x10 /* memory block header size is 16 B */
#define USR_SZ_STACK 0x12C  /* user proc stack size 300 B */

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

/* Message Types */
#define DEFAULT 0
#define KCD_REG 1
#define CRT_DISPLAY 2
#define USER_INPUT 3
#define COMMAND 4
#define COUNT_REPORT 5
#define WAKEUP10 6

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
	struct pcb* mp_next;	/* next pcb */
	uint32_t* mp_sp;		/* stack pointer of the process */
	int m_pid;				/* process id */
	int m_priority;
	int m_is_iproc;			/* whether or not PCB is iProc */
	PROC_STATE_E m_state;	/* state of the process */   
	Queue m_message_q;
} PCB;

/* initialization table item */
typedef struct proc_init
{	
	int m_pid;				/* process id */ 
	int m_priority;			/* initial priority */ 
	int m_stack_size;		/* size of stack in words */
	void (*mpf_start_pc)();	/* entry point of the process */    
} PROC_INIT;

typedef struct msg_envelope
{
	struct msg_envelope* next;
	int sender_pid;
	int destination_pid;
	uint32_t send_time;
	int mtype;              /* user defined message type */
	char mtext[100];         /* body of the message */
} MSG_ENVELOPE;

/* message buffer */
typedef struct msgbuf
{
	int mtype;              /* user defined message type */
	char mtext[100];         /* body of the message */
} MSG_BUF;

typedef struct mem_block
{
	U32 *next_block;
} MEM_BLOCK;

/* Global variables */
extern PriorityQueue* blocked_memory_pq;
extern PriorityQueue* blocked_waiting_pq;
extern PriorityQueue* ready_pq;

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

extern int k_set_process_priority(int pid, int prio);
#define set_process_priority(pid, prio) _set_process_priority((U32)k_set_process_priority, pid, prio)
extern int _set_process_priority(U32 p_func, int pid, int prio) __SVC_0;

/* Memory Management */
extern void *ki_request_memory_block(void);
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

extern void *ki_receive_message(int *p_pid);
extern void *k_receive_message(int *p_pid);
#define receive_message(p_pid) _receive_message((U32)k_receive_message, p_pid)
extern void *_receive_message(U32 p_func, void *p_pid) __SVC_0;

extern void *k_message_to_envelope(MSG_BUF* message);
#define message_to_envelope(message) _message_to_envelope((U32)k_message_to_envelope, message)
extern void *_message_to_envelope(U32 p_func, void* message) __SVC_0;

extern void *k_envelope_to_message(MSG_ENVELOPE* envelope);
#define envelope_to_message(envelope) _envelope_to_message((U32)k_envelope_to_message, envelope)
extern void *_envelope_to_message(U32 p_func, void* envelope) __SVC_0;

/* Timing Service */
extern int k_delayed_send(int pid, void *p_msg, int delay);
#define delayed_send(pid, p_msg, delay) _delayed_send((U32)k_delayed_send, pid, p_msg, delay)
extern int _delayed_send(U32 p_func, int pid, void *p_msg, int delay) __SVC_0; 

#endif // ! K_RTX_H_
