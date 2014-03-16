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

char g_buffer[];
char* gp_buffer = g_buffer;
char g_char_in;
char g_char_out;

extern U32 g_switch_flag;
extern PCB *gp_current_process;
extern PCB* get_proc_by_pid(int pid);
extern void configure_old_pcb(PCB* p_pcb_old);
extern int process_switch(PCB* p_pcb_old);
LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;
U8 IIR_IntId;	    // Interrupt ID from IIR 		 
MSG_BUF* received_message;
MSG_BUF* message_to_send;
PCB* unswitched_proc;
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
	
#ifdef DEBUG_0
		uart1_put_string("Entering UART i-proc\n\r");
#endif // DEBUG_0
	
	/* Reading IIR automatically acknowledges the interrupt */
	IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR 
		if (IIR_IntId & IIR_RDA) { // Receive Data Available
			g_switch_flag = 1;
			old_proc = gp_current_process;
			gp_current_process = get_proc_by_pid(PID_UART_IPROC);
			process_switch(old_proc);
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
			// send char to KCD, which will handle parsing and send each character to CRT for printing
			message_to_send = (MSG_BUF*)ki_request_memory_block();
			__disable_irq();
			message_to_send->mtype = USER_INPUT;
			message_to_send->mtext[0] = g_char_in;
			message_to_send->mtext[1] = '\0';
			k_send_message(PID_KCD, (void*)message_to_send);
			__disable_irq();

	} else if (IIR_IntId & IIR_THRE) {
			g_switch_flag = 0;
			old_proc = gp_current_process;
			gp_current_process = get_proc_by_pid(PID_UART_IPROC);
	/* THRE Interrupt, transmit holding register becomes empty */
		received_message = (MSG_BUF*)ki_receive_message((int*)0);
			__disable_irq();
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
			__disable_irq();
			pUart->IER ^= IER_THRE; // toggle the IER_THRE bit 
			//pUart->THR = '\0';
			gp_buffer = g_buffer;		
			unswitched_proc = old_proc;
			old_proc = gp_current_process;
			gp_current_process = unswitched_proc;
	      
	} else {  /* not implemented yet */
#ifdef DEBUG_0
			uart1_put_string("Should not get here!\n\r");
#endif // DEBUG_0
		return;
	}
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

void KCD(void)
{
	typedef struct cmd 
	{ 
		char cmd_id[2];	      /* command identifier  */
		U32 reg_proc_id;			/* process id of registered process */
	} CMD;
	
	MSG_BUF* received_message;
	MSG_BUF* message_to_send;
	char user_input_str[1];
	char* user_input_str_p = user_input_str;
	CMD registered_commands[10];
	U32 current_num_commands = 0;
	int sender_id;
	char to_parse[1];
	char* to_parse_p = to_parse;
	
	while(1) {
		// grab the message from the KCD proc message queue
		received_message = (MSG_BUF*)receive_message(&sender_id);
		if (received_message->mtype == USER_INPUT) {
			if (received_message->mtext[0] == '\r') {
				message_to_send = (MSG_BUF*)request_memory_block();
				message_to_send->mtype = CRT_DISPLAY;
				message_to_send->mtext[0] = '\r';
				message_to_send->mtext[1] = '\n';
				message_to_send->mtext[2] = '\0';
				send_message(PID_CRT, (void*)message_to_send);
				user_input_str_p = user_input_str;
			} else if (received_message->mtext[0] == '\n') {
				// no-op
			} else {
				*user_input_str_p = received_message->mtext[0];
				message_to_send = (MSG_BUF*)request_memory_block();
				message_to_send->mtype = CRT_DISPLAY;
				message_to_send->mtext[0] = *user_input_str_p;
				message_to_send->mtext[1] = '\0';
				send_message(PID_CRT, (void*)message_to_send);
				user_input_str_p++;
			}
		}
		else if (received_message->mtype == KCD_REG) {
			int i = 1;
			if (received_message->mtext[0] != '%') {	
				char error_str[] = "KCD Registration Commands must begin with the % character.\n\r";
				message_to_send = (MSG_BUF*)request_memory_block();
				message_to_send->mtype = CRT_DISPLAY;
				strcpy(message_to_send->mtext, error_str);
				send_message(PID_CRT, (void*)message_to_send);
			}
			while (received_message->mtext[i] != '\0') {
				registered_commands[current_num_commands].cmd_id[i-1] = received_message->mtext[i];
				i++;
			}
			// null-terminate the id
			registered_commands[current_num_commands].cmd_id[i-1] = '\0';
			registered_commands[current_num_commands].reg_proc_id = sender_id;
			current_num_commands++;
		}
		else if (received_message->mtype != KCD_REG) {
			int is_match = 1, num_com;
			if (received_message->mtype == USER_INPUT) {
				to_parse_p = user_input_str;
			} else {
				int i = 0;
				to_parse_p = &received_message->mtext[0];
				message_to_send = (MSG_BUF*)request_memory_block();
				message_to_send->mtype = CRT_DISPLAY;
				while (*to_parse_p != '\0') {
					message_to_send->mtext[i] = *to_parse_p;
					to_parse_p++;
					i++;
				}
				message_to_send->mtext[i] = '\0';
				send_message(PID_CRT, (void*)message_to_send);
				to_parse_p = &received_message->mtext[0];
			}
			for (num_com = 0; num_com < 10; num_com++) {
				int c = 0;
				while (*to_parse_p != '\0') {
					if (registered_commands[num_com].cmd_id[c] != *to_parse_p) {
						is_match = 0;
						break;
					}
					c++;
					to_parse_p++;
				}
				
				if (is_match && *to_parse_p == registered_commands[num_com].cmd_id[c]) {
					int j = 0;
					char* index = to_parse;
					message_to_send = (MSG_BUF*)request_memory_block();
					message_to_send->mtype = DEFAULT;
					while (*index != '\0') {
						message_to_send->mtext[j] = *index;
						index++;
						j++;
					}
					message_to_send->mtext[j] = '\0';
					send_message(registered_commands[num_com].reg_proc_id, (void*)message_to_send);
				}
			}
		}
		release_memory_block((void*)received_message);
	}
}
