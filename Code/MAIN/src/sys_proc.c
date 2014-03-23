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

// KCD command structure
typedef struct cmd 
{ 
	char cmd_id[4];  /* command identifier  */
	int reg_proc_id; /* process id of registered process */
} CMD;

CMD registered_commands[10]; // Array of registered commands for KCD
int num_reg_commands = 0;    // Number of currently registered commands


/**
 * @brief The Set Priority Command Process.
 * Allows user to set process priority using messages rather than the user API.
 */
void set_priority_command_proc(void)
{
	int sender_id;
	int pid;
	int priority;
	MSG_BUF* msg_received;
	MSG_BUF* msg_to_send;
	
	// Tell the KCD to register the "%C" command with the set priority command process
	msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = KCD_REG;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'C';
	msg_to_send->mtext[2] = '\0';
	send_message(PID_KCD, (void*)msg_to_send);
	
	while(1) {
		// Initialize pid and priority to error
		pid = RTX_ERR;
		priority = RTX_ERR;
		
		// Receive message from KCD
		msg_received = (MSG_BUF*)receive_message(&sender_id);
		
		if (msg_received->mtype == COMMAND) {
			// We're looking for a string of the following form: %C {1,...,13} {0,...,3}
			if (msg_received->mtext[2] == ' ') {
				// CASE: PID between 1 and 9
				if (msg_received->mtext[4] == ' ' 
					&& msg_received->mtext[3] >= '1' && msg_received->mtext[3] <= '9'
					&& msg_received->mtext[5] >= '0' && msg_received->mtext[5] <= '3'
					&& hasWhiteSpaceToEnd(msg_received->mtext, 6)) {
						pid = ctoi(msg_received->mtext[3]);
						priority = ctoi(msg_received->mtext[5]);
				}
				// CASE: PID between 10 and 13
				else if (msg_received->mtext[5] == ' ' 
							&& msg_received->mtext[3] == '1' 
							&& msg_received->mtext[4] >= '0' && msg_received->mtext[4] <= '3'
							&& msg_received->mtext[6] >= '0' && msg_received->mtext[6] <= '3'
							&& hasWhiteSpaceToEnd(msg_received->mtext, 7)) {
					pid = 10 + ctoi(msg_received->mtext[4]);
					priority = ctoi(msg_received->mtext[6]);
				}
			}
		}
		
		if (pid != RTX_ERR && priority != RTX_ERR) {
			set_process_priority(pid, priority);
		}
		else {
			msg_to_send = (MSG_BUF*)request_memory_block();
			msg_to_send->mtype = CRT_DISPLAY;
			strcpy(msg_to_send->mtext, "ERROR: Incorrect Input! Please ensure that the PID is between 1 and 13 and that the priority is between 0 and 3.\r\n");
			send_message(PID_CRT, msg_to_send);
		}
		
		release_memory_block(msg_received);
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
	int mtype_validator = -1; // Validates messages to determine if the wall clock should run
	int sender_id;
	MSG_BUF* msg_received;
	MSG_BUF* msg_to_send;
	
	// Tell the KCD to register the "%WR" command with the wall clock process
	msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = KCD_REG;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'W';
	msg_to_send->mtext[2] = 'R';
	msg_to_send->mtext[3] = '\0';
	send_message(PID_KCD, (void*)msg_to_send);

	// Tell the KCD to register the "%WS" command with the wall clock process
	msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = KCD_REG;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'W';
	msg_to_send->mtext[2] = 'S';
	msg_to_send->mtext[3] = '\0';
	send_message(PID_KCD, (void*)msg_to_send);

	// Tell the KCD to register the "%WT" command with the wall clock process
	msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = KCD_REG;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'W';
	msg_to_send->mtext[2] = 'T';
	msg_to_send->mtext[3] = '\0';
	send_message(PID_KCD, (void*)msg_to_send);

	while (1) {
		// Receive message from KCD (command input), or self (to display time)
		msg_received = (MSG_BUF*)receive_message(&sender_id);
		
		if (msg_received->mtype == COMMAND) {
			/* NOTE:
			 * Changing the message type validator invalidates previous delay-sent messages
			 * so only new messages are used to run the clock.
			 */

			if (msg_received->mtext[2] == 'R') { // Reset and run clock
				// Reset the time
				hours = minutes = seconds = 0;

				mtype_validator = get_current_time(); // Change the message type validator
				// If the message type validator happens to have been set to the COMMAND type, change it
				if (mtype_validator == COMMAND) {
					++mtype_validator;
				}
				msg_received->mtype = mtype_validator; // Set the message's type to the validator so the clock will run
			}
			else if (msg_received->mtext[2] == 'S') { // Set clock running starting at a specified time
				if (msg_received->mtext[3] == ' '
					&& msg_received->mtext[4] >= '0' && msg_received->mtext[4] <= '2'
					&& msg_received->mtext[5] >= '0' && msg_received->mtext[5] <= '3'
					&& msg_received->mtext[6] == ':'
					&& msg_received->mtext[7] >= '0' && msg_received->mtext[7] <= '5'
					&& msg_received->mtext[8] >= '0' && msg_received->mtext[8] <= '9'
					&& msg_received->mtext[9] == ':'
					&& msg_received->mtext[10] >= '0' && msg_received->mtext[10] <= '5'
					&& msg_received->mtext[11] >= '0' && msg_received->mtext[11] <= '9'
					&& hasWhiteSpaceToEnd(msg_received->mtext, 12)) {
					// Use the input time to set the current time variables and set the message's type to the validator so the clock will run
					hours = ctoi(msg_received->mtext[4]) * 10 + ctoi(msg_received->mtext[5]);
					minutes = ctoi(msg_received->mtext[7]) * 10 + ctoi(msg_received->mtext[8]);
					seconds = ctoi(msg_received->mtext[10]) * 10 + ctoi(msg_received->mtext[11]);

					mtype_validator = get_current_time(); // Change the message type validator
					// If the message type validator happens to have been set to the COMMAND type, change it
					if (mtype_validator == COMMAND) {
						++mtype_validator;
					}
					msg_received->mtype = mtype_validator; // Set the message's type to the validator so the clock will run
				}
				else { // Input was invalid
					// Send a message to the CRT to display an error message
					msg_to_send = (MSG_BUF*)request_memory_block();
					msg_to_send->mtype = CRT_DISPLAY;
					strcpy(msg_to_send->mtext, "ERROR: The input time was invalid!\r\n");
					send_message(PID_CRT, msg_to_send);
				}
			}
			else if (msg_received->mtext[2] == 'T') { // Stop clock
				mtype_validator = get_current_time(); // Change the message type validator so the clock will stop running
				// If the message type validator happens to have been set to the COMMAND type, change it
				if (mtype_validator == COMMAND) {
					++mtype_validator;
				}
			}
		}

		if (msg_received->mtype == mtype_validator) {
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
			msg_to_send->mtext[8] = '\r';
			msg_to_send->mtext[9] = '\n';
			msg_to_send->mtext[10] = '\0';
			
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

			// Send a message to self in exactly 1 second to update and display the time
			msg_to_send = (MSG_BUF*)request_memory_block();
			msg_to_send->mtype = mtype_validator;
			delayed_send(PID_CLOCK, msg_to_send, 1000);
		}
		
		// Release the memory of the received message
		release_memory_block(msg_received);
	}
}

/**
 * Gets the PID of the process that registered the input command
 * @returns The PID of the process that registered the input command,
 *          or if there is no process registered with the command, returns RTX_ERR
 */
int get_command_proc_id(const char input_line[50])
{
	int i;

	if (input_line[0] != '%') {
		return RTX_ERR;
	}

	for (i = 0; i < num_reg_commands; ++i) {
		int j = 0;
		do {
			j++;
			if (registered_commands[i].cmd_id[j-1] == '\0' && (input_line[j] == '\0' || input_line[j] == ' ')) {
				return registered_commands[i].reg_proc_id;
			}
		} while (input_line[j] == registered_commands[i].cmd_id[j-1]);
	}

	return RTX_ERR;
}

void KCD(void)
{	
	MSG_BUF* message_received;
	MSG_BUF* message_to_send;
	char str_user_input[50] = "";
	int idx_user_input = 0;
	int command_line_received = 0;
	int sender_id;
	
	while (1) {
		// Receive a message
		message_received = (MSG_BUF*)receive_message(&sender_id);

		if (message_received->mtype == KCD_REG) { // Register a command with KCD
			if (message_received->mtext[0] != '%' || message_received->mtext[1] == '\0') {
				message_to_send = (MSG_BUF*)request_memory_block();
				message_to_send->mtype = CRT_DISPLAY;
				strcpy(message_to_send->mtext, "ERROR: Commands must begin with the %% character, followed by at least one letter.\n\r");
				send_message(PID_CRT, message_to_send);
			}
			else {
				int i = 1;
				while (message_received->mtext[i] != '\0') {
					registered_commands[num_reg_commands].cmd_id[i-1] = message_received->mtext[i];
					i++;
				}
				registered_commands[num_reg_commands].cmd_id[i-1] = '\0';      // Null-terminate the id
				registered_commands[num_reg_commands].reg_proc_id = sender_id; // Set the registered proc id
				num_reg_commands++; // Increment the number of registered commands
			}
		}
		else if (message_received->mtype == USER_INPUT) { // User input a character
			// Build message to send to CRT to output the input character
			message_to_send = (MSG_BUF*)request_memory_block();
			message_to_send->mtype = CRT_DISPLAY;
			message_to_send->mtext[0] = message_received->mtext[0];

			if (message_received->mtext[0] == '\r') { // The user pressed ENTER
				message_to_send->mtext[1] = '\n';
				message_to_send->mtext[2] = '\0';
				
				// A full command line has been received, so put the input string into the received message's text
				// and set the flag so that it may be forwarded
				strcpy(message_received->mtext, str_user_input);
				command_line_received = 1;

				// Reset the user input string and input string index
				str_user_input[0] = '\0';
				idx_user_input = 0;
			}
			else {
				message_to_send->mtext[1] = '\0';

				if (message_received->mtext[0] == '\b') { // The user pressed BACKSPACE
					if (idx_user_input > 0) {
						// Subtract a character from the user input string
						str_user_input[--idx_user_input] = '\0';
					}
				}
				else {
					// Add the character to the user input string
					str_user_input[idx_user_input] = message_received->mtext[0];
					str_user_input[++idx_user_input] = '\0';
				}
			}

			send_message(PID_CRT, message_to_send); // Send message to CRT
		}
		else { // We have received a normal message from a test process that contains a command line
			command_line_received = 1;
		}

		if (command_line_received) { // We have received a full command line and its text is located in message_received->mtext
			// Retreive the id of the process that registered the command
			int reg_id = get_command_proc_id(message_received->mtext);
			command_line_received = 0; // Reset the flag
			if (reg_id != RTX_ERR) {
				// The line was a valid command and we have retreived the id of the process registered for that command
				message_received->mtype = COMMAND;
				send_message(reg_id, message_received); // Forward the message to the registered process
				continue; // Go back to the start of the loop (so that we don't release the memory of the forwarded message)
			}
		}

		release_memory_block(message_received);
	}
}

/**
 * @brief: prints CRT_DISPLAY message to UART0 by sending message to UART i-proc and toggling interrupt bit
 */
void CRT(void)
{
	MSG_BUF* received_message;
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;	 
	
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
	MSG_BUF* received_message;
	MSG_BUF* message_to_send;
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;
	U8 IIR_IntId;	    // Interrupt ID from IIR

	// Save the previously running proccess and set the current process to this i-proc 
	PCB* old_proc = gp_current_process;
	gp_current_process = get_proc_by_pid(PID_UART_IPROC);
	
	g_switch_flag = 0;  // Reset the switch flag 

#ifdef DEBUG_0
	uart1_put_string("Entering UART i-proc\n\r");
#endif // DEBUG_0
	
	/* Reading IIR automatically acknowledges the interrupt */
	IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR 
	
	if (IIR_IntId & IIR_RDA) { // Receive Data Available
		/* read UART. Read RBR will clear the interrupt */
		g_char_in = pUart->RBR;
		
	#ifdef DEBUG_0
		uart1_put_string("Reading a char = ");
		uart1_put_char(g_char_in);
		uart1_put_string("\r\n");
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
		if (message_to_send) {
			message_to_send->mtype = USER_INPUT;
			message_to_send->mtext[0] = g_char_in;
			message_to_send->mtext[1] = '\0';
			k_send_message(PID_KCD, (void*)message_to_send);
		}
	}
	else if (IIR_IntId & IIR_THRE) {
		/* THRE Interrupt, transmit holding register becomes empty */
		received_message = (MSG_BUF*)ki_receive_message((int*)0);
		gp_buffer = received_message->mtext;
		
		pUart->IER ^= IER_THRE; // toggle the IER_THRE bit
		//pUart->THR = '\0';
		
		while (*gp_buffer != '\0' ) {
			g_char_out = *gp_buffer;
			pUart->THR = g_char_out;
			gp_buffer++;
			uart1_put_string("Printing a character to UART0...\r\n");
		}
		
		gp_buffer = g_buffer;	
		k_release_memory_block((void*)received_message);	
	}
	else {  /* not implemented yet */
	#ifdef DEBUG_0
		uart1_put_string("Should not get here!\n\r");
	#endif // DEBUG_0
	}
	
	//Restore the current process
	gp_current_process = old_proc;
}
