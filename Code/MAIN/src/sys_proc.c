/**
 * @brief: sys_proc.c
 * @author: Justin Gagne
 * @date: 2014/02/28
 */

#include <LPC17xx.h>
#include "timer.h"
#include "sys_proc.h"
#include "uart.h"
#include "uart_polling.h"
#include "k_process.h"
#include "k_memory.h"
#include "string.h"
#include "utils.h"
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
	U8 IIR_IntId;	    // Interrupt ID from IIR 		 
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

/**
 * @brief The Wall Clock process.
 *
 * The first time the wall clock is run, it registers itself with the KCD, then goes into
 * the eternal loop where it gets blocked waiting for a message. Each time the wall clock
 * gets a new message, it performs the necessary actions and if it is in a "running" state,
 * it sends a message to the CRT to display the current time. Finally, the wall clock releases
 * the memory block of the message it received.
 */
void proc_wall_clock()
{
	int hours;
	int minutes;
	int seconds;
	int is_running = 0; // Initialize to 0 (meaning false)
	int sender_id;
	MSG_BUF* msg_received;
	MSG_BUF* msg_to_send = (MSG_BUF*)request_memory_block();
	
	// Tell the KCD to register the "%W" command with the wall clock process
	msg_to_send->mtype = KCD_REG;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'W';
	msg_to_send->mtext[2] = '\0';
	send_message(PID_KCD, (void*)msg_to_send);

	while (1) {
		// Receive message from KCD (command input), or timer (to display time)
		msg_received = (MSG_BUF*)receive_message(&sender_id);
		
		if (sender_id == PID_KCD) {
			if (msg_received->mtext[2] == 'T') {
				is_running = 0;
			}
			else if (msg_received->mtext[2] == 'R') {
				// Reset the time and set the wall clock to running
				hours = minutes = seconds = 0;
				is_running = 1;
			}
			else if (msg_received->mtext[2] == 'S'
			         && msg_received->mtext[3] == ' '
			         && msg_received->mtext[4] >= '0' && msg_received->mtext[4] <= '2'
			         && msg_received->mtext[5] >= '0' && msg_received->mtext[5] <= '3'
			         && msg_received->mtext[6] == ':'
			         && msg_received->mtext[7] >= '0' && msg_received->mtext[7] <= '5'
			         && msg_received->mtext[8] >= '0' && msg_received->mtext[8] <= '9'
			         && msg_received->mtext[9] == ':'
			         && msg_received->mtext[10] >= '0' && msg_received->mtext[10] <= '5'
			         && msg_received->mtext[11] >= '0' && msg_received->mtext[11] <= '9') {
				// Use the input time to set the current time variables and set the wall clock to running
				hours = ctoi(msg_received->mtext[4]) * 10 + ctoi(msg_received->mtext[5]);
				minutes = ctoi(msg_received->mtext[7]) * 10 + ctoi(msg_received->mtext[8]);
				seconds = ctoi(msg_received->mtext[10]) * 10 + ctoi(msg_received->mtext[11]);
				is_running = 1;
			}
		}

		if (is_running) {
			// Send a message to the CRT to display the current time
			msg_to_send = (MSG_BUF*)request_memory_block();
			msg_to_send->mtype = CRT_DISPLAY;
			
			msg_to_send->mtext[0] = itoc(hours / 10);
			msg_to_send->mtext[1] = itoc(hours % 10);
			msg_to_send->mtext[2] = ':';
			msg_to_send->mtext[3] = itoc(minutes / 10);
			msg_to_send->mtext[4] = itoc(minutes % 10);
			msg_to_send->mtext[5] = ':';
			msg_to_send->mtext[6] = itoc(seconds / 10);
			msg_to_send->mtext[7] = itoc(seconds % 10);
			msg_to_send->mtext[8] = '\0';
			
			send_message(PID_CRT, (void*)msg_to_send);
			
			// Set the time for when the wall clock is run again (which should be in exactly 1 second)
			if (++seconds >= 60) {
				seconds = 0;
				if (++minutes >= 60) {
					minutes = 0;
					if (++hours >= 24) {
						hours = 0;
					}
				}
			}
		}
		
		// Release the memory of the received message
		release_memory_block(msg_received);
	}
}
