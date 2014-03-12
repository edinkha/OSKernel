/**
 * @brief: sys_proc.c
 * @author: Justin Gagne
 * @date: 2014/02/28
 */

#include <LPC17xx.h>
#include "uart.h"
#include "uart_polling.h"
#include "k_process.h"
#include "k_memory.h"
#ifdef DEBUG_0
#include "printf.h"
#endif
#ifdef DEBUG_HK
#include <assert.h>
#endif

uint8_t g_buffer[];
uint8_t *gp_buffer = g_buffer;
uint8_t g_send_char = 0;
uint8_t g_char_in;
uint8_t g_char_out;

extern uint32_t g_switch_flag;
extern PCB *gp_current_process;
extern PCB* get_proc_by_pid(int pid);

LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;

#ifdef DEBUG_HK
/**
 * @brief: Print all of the PCB process_ids in the priority queue, along with their priorities
 */
void print(PriorityQueue* pqueue)
{
	int i;
	QNode* cur_node;
	assert(pqueue != NULL);
	for (i = 0; i < NUM_PRIORITIES; i++) {
		cur_node = pqueue->queues[i].first;
		while (cur_node != NULL) {
			printf("Process ID = %d\n\r", ((PCB*)cur_node)->m_pid);
			printf("Process Priority = %d\n\r", ((PCB*)cur_node)->m_priority);
			cur_node = cur_node->next;
		}
	}
}
#endif

// UART initialized in uart_irq.c
/**
 * @brief: use CMSIS ISR for UART0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine. 
 *       The actual c_UART0_IRQHandler does the rest of irq handling
 */
__asm void UART0_IRQHandler(void)
{
	PRESERVE8
	IMPORT c_UART0_IRQHandler
	; NOTE: remove next line most likely, but will need for timer handler
	IMPORT k_release_processor
	PUSH{r4-r11, lr}
	BL c_UART0_IRQHandler
	; NOTE: Look at code in UART_IRQ folder -- this part can likely be omitted because UART i-process doesnt need preemption
	; However, will likely need it for the timer interrupt handler
	LDR R4, =__cpp(&g_switch_flag)
	LDR R4, [R4]
	MOV R5, #0     
	CMP R4, R5
	BEQ  RESTORE    ; if g_switch_flag == 0, then restore the process that was interrupted
	BL k_release_processor  ; otherwise (i.e g_switch_flag == 1, then switch to the other process)
RESTORE
	; END PART that can be removed
	POP{r4-r11, pc}
} 
/**
 * @brief: c UART0 IRQ Handler
 */
void c_UART0_IRQHandler(void)
{
	uint8_t IIR_IntId;	    // Interrupt ID from IIR 		 
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
	MSG_BUF* received_message;
	MSG_BUF* message_to_send;
	PCB* cur_proc;
	
	__disable_irq();
#ifdef DEBUG_0
	uart1_put_string("Entering c_UART0_IRQHandler\n\r");
#endif // DEBUG_0
	
	cur_proc = get_proc_by_pid(PID_UART_IPROC);
	cur_proc->m_state = RUNNING;
	gp_current_process = cur_proc;

	/* Reading IIR automatically acknowledges the interrupt */
	IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR 
	if (IIR_IntId & IIR_RDA) { // Receive Data Avaialbe
		/* read UART. Read RBR will clear the interrupt */
		g_char_in = pUart->RBR;
#ifdef DEBUG_0
		uart1_put_string("Reading a char = ");
		uart1_put_char(g_char_in);
		uart1_put_string("\n\r");
#endif // DEBUG_0
		
#ifdef DEBUG_HK
		if (g_char_in == '!') {
			uart1_put_string("! hotkey entered - printing processes on ready queue\n\r");
			print(ready_pq);
		}
		else if (g_char_in == '@') {
			uart1_put_string("@ hotkey entered - printing processes on blocked on memory queue\n\r");
			print(blocked_memory_pq);
		}
		else if (g_char_in == '#') {
			uart1_put_string("# hotkey entered - printing processes on blocked on receive queue\n\r");
			print(blocked_waiting_pq);
		}
#endif // DEBUG_HK
		if (g_char_in == '\r') {
			// send g_buffer string to KCD
		}
		else {
			// send char to CRT to echo it back to console
			message_to_send = (MSG_BUF*)k_request_memory_block();
			message_to_send->mtype = CRT_DISPLAY;
			message_to_send->mtext[0] = g_char_in;
			k_send_message(PID_CRT, (void*)message_to_send);
			
			// add character to g_buffer string
			//g_buffer += g_char_in;
		}
		g_send_char = 1;
		
		/* setting the g_switch_flag */
		if ( g_char_in == 'S' ) {
			g_switch_flag = 1; 
		} else {
			g_switch_flag = 0;
		}
	} else if (IIR_IntId & IIR_THRE) {
	/* THRE Interrupt, transmit holding register becomes empty */
		received_message = (MSG_BUF*)k_receive_message((int*)0);
		gp_buffer = received_message->mtext;
		if (*gp_buffer != '\0' ) {
			g_char_out = *gp_buffer;
#ifdef DEBUG_0	
			// you could use the printf instead
			printf("Writing a char = %c \n\r", g_char_out);
#endif // DEBUG_0			
			pUart->THR = g_char_out;
			gp_buffer++;
		} else {
#ifdef DEBUG_0
			uart1_put_string("Finish writing. Turning off IER_THRE\n\r");
#endif // DEBUG_0
			k_release_memory_block((void*)received_message);
			pUart->IER ^= IER_THRE; // toggle the IER_THRE bit 
			pUart->THR = '\0';
			g_send_char = 0;
			gp_buffer = g_buffer;		
		}
	      
	} else {  /* not implemented yet */
#ifdef DEBUG_0
			uart1_put_string("Should not get here!\n\r");
#endif // DEBUG_0
		return;
	}	
	__enable_irq();
}

/**
 * @brief: prints CRT_DISPLAY message to UART0 by sending message to UART i-proc and toggling interrupt bit
 */
void CRT(void)
{
	MSG_BUF* received_message;
	
	while(1) {
		// grab the message from the CRT proc message queue
		received_message = (MSG_BUF*)receive_message((int*)0);
		if (received_message->mtype == CRT_DISPLAY) {
			send_message(PID_UART_IPROC, received_message);
			// trigger the UART THRE interrupt bit so that UART i-proc runs
			pUart->IER ^= IER_THRE;
		} else {
			release_memory_block((void*)received_message);
		}
	}
}
