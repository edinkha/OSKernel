/**
 * @brief: sys_proc.c
 * @author: Justin Gagne
 * @date: 2014/02/28
 */

#include <LPC17xx.h>
#include "uart.h"
#include "k_rtx.h"
#include "sys_proc.h"
#include "string.h"

// KCD command structure
typedef struct cmd {
	char cmd_id[4];  /* command identifier  */
	int reg_proc_id; /* process id of registered process */
} CMD;

CMD registered_commands[10]; // Array of registered commands for KCD
int num_reg_commands = 0;    // Number of currently registered commands


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
			if (registered_commands[i].cmd_id[j - 1] == '\0' && (input_line[j] == '\0' || input_line[j] == ' ')) {
				return registered_commands[i].reg_proc_id;
			}
		}
		while (input_line[j] == registered_commands[i].cmd_id[j - 1]);
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
					registered_commands[num_reg_commands].cmd_id[i - 1] = message_received->mtext[i];
					i++;
				}
				registered_commands[num_reg_commands].cmd_id[i - 1] = '\0';    // Null-terminate the id
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
				
				if (message_received->mtext[0] == '\b' || message_received->mtext[0] == 127) { // The user pressed BACKSPACE
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
 * @brief Prints CRT_DISPLAY message to UART0 by sending message to UART i-proc and toggling interrupt bit
 */
void CRT(void)
{
	MSG_BUF* received_message;
	LPC_UART_TypeDef* pUart = (LPC_UART_TypeDef*) LPC_UART0;
	
	while (1) {
		// grab the message from the CRT proc message queue
		received_message = (MSG_BUF*)receive_message((int*)0);
		if (received_message->mtype == CRT_DISPLAY) {
			send_message(PID_UART_IPROC, received_message);
			// trigger the UART THRE interrupt bit so that UART i-proc runs
			pUart->IER ^= IER_THRE;
		}
		else {
			release_memory_block(received_message);
		}
	}
}
