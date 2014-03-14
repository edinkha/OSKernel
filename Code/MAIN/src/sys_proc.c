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

U8 g_buffer[];
U8 *gp_buffer = g_buffer;
U8 g_send_char = 0;
U8 g_char_in;
U8 g_char_out;

extern U32 g_switch_flag;
extern PCB *gp_current_process;
extern PCB* get_proc_by_pid(int pid);
extern void configure_old_pcb(PCB* p_pcb_old);
extern int process_switch(PCB* p_pcb_old);

LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;
U8 IIR_IntId;	    // Interrupt ID from IIR 		 
MSG_BUF* received_message;
MSG_BUF* message_to_send;
//PCB* cur_proc;
PCB* old_proc;

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
void UART_IPROC(void)
{
	old_proc = gp_current_process;
	gp_current_process = get_proc_by_pid(PID_UART_IPROC);
	process_switch(old_proc);
	
	#ifdef DEBUG_0
		uart1_put_string("Entering UART i-proc\n\r");
	#endif // DEBUG_0

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
			g_switch_flag = 1;
			
		} else if (IIR_IntId & IIR_THRE) {
		/* THRE Interrupt, transmit holding register becomes empty */
			received_message = (MSG_BUF*)k_receive_message((int*)0);
			gp_buffer = received_message->mtext;
			while (*gp_buffer != '\0' ) {
				g_char_out = *gp_buffer;
	#ifdef DEBUG_0	
				// you could use the printf instead
				printf("Writing a char = %c \n\r", g_char_out);
	#endif // DEBUG_0			
				pUart->THR = g_char_out;
				gp_buffer++;
			}
	#ifdef DEBUG_0
			uart1_put_string("Finish writing. Turning off IER_THRE\n\r");
	#endif // DEBUG_0
			k_release_memory_block((void*)received_message);
			pUart->IER ^= IER_THRE; // toggle the IER_THRE bit 
			pUart->THR = '\0';
			g_send_char = 0;
			g_switch_flag = 0;
			gp_buffer = g_buffer;		
					
		} else {  /* not implemented yet */
	#ifdef DEBUG_0
				uart1_put_string("Should not get here!\n\r");
	#endif // DEBUG_0
			return;
		}
		old_proc = gp_current_process;
		// Use scheduler to determine next proc to run, then process_switch to run it
		gp_current_process = scheduler();
		process_switch(old_proc);
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
