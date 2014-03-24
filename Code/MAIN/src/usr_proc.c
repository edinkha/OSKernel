/**
 * @brief: usr_proc.c
 */

#include "k_rtx.h"
#include "usr_proc.h"
#include "i_proc.h"
#include "string.h"
#include "utils.h"


/**
 * @brief The Set Priority Command Process.
 * Allows user to set process priority using messages rather than the user API.
 */
void set_priority_command_proc(void)
{
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
	send_message(PID_KCD, msg_to_send);
	
	while (1) {
		// Initialize pid and priority to error
		pid = RTX_ERR;
		priority = RTX_ERR;
		
		// Receive message from KCD
		msg_received = (MSG_BUF*)receive_message(0);
		
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
			strcpy(msg_to_send->mtext, "ERROR: Invalid input!\r\n");
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
	MSG_BUF* msg_received;
	MSG_BUF* msg_to_send;
	
	// Tell the KCD to register the "%WR" command with the wall clock process
	msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = KCD_REG;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'W';
	msg_to_send->mtext[2] = 'R';
	msg_to_send->mtext[3] = '\0';
	send_message(PID_KCD, msg_to_send);
	
	// Tell the KCD to register the "%WS" command with the wall clock process
	msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = KCD_REG;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'W';
	msg_to_send->mtext[2] = 'S';
	msg_to_send->mtext[3] = '\0';
	send_message(PID_KCD, msg_to_send);
	
	// Tell the KCD to register the "%WT" command with the wall clock process
	msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = KCD_REG;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'W';
	msg_to_send->mtext[2] = 'T';
	msg_to_send->mtext[3] = '\0';
	send_message(PID_KCD, msg_to_send);
	
	while (1) {
		// Receive message from KCD (command input), or self (to display time)
		msg_received = (MSG_BUF*)receive_message(0);
		
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
					strcpy(msg_to_send->mtext, "ERROR: Invalid time!\r\n");
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

void proc_a(void)
{
	int num = 0;
	MSG_BUF* msg_received;
	MSG_BUF* msg_to_send;
	
	// Tell the KCD to register the "%Z" command
	msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = KCD_REG;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'Z';
	msg_to_send->mtext[2] = '\0';
	send_message(PID_KCD, msg_to_send);
	
	// Wait for the "%Z" command and discard any other messages while waiting
	while (1) {
		msg_received = (MSG_BUF*)receive_message(0);
		if (msg_received->mtype == COMMAND) {
			release_memory_block(msg_received);
			break;
		}
		release_memory_block(msg_received);
	}
	
	num = 0;
	while (1) {
		msg_to_send = (MSG_BUF*)request_memory_block();
		msg_to_send->mtype = COUNT_REPORT;
		itoa(num, msg_to_send->mtext);
		send_message(PID_B, msg_to_send);
		num++;
		release_processor();
	}
}

void proc_b (void)
{
	while (1) {
		MSG_BUF* msg_received = (MSG_BUF*)receive_message(0);
		send_message(PID_C, msg_received);
	}
}

void proc_c (void)
{
	MSG_BUF* msg_to_send;
	MSG_BUF* msg_received;
	Queue local_message_q;
	
	init_q(&local_message_q);
	
	while (1) {
		if (q_empty(&local_message_q)) {
			msg_received = (MSG_BUF*)receive_message(0);
		}
		else {
			msg_received = (MSG_BUF*)envelope_to_message((MSG_ENVELOPE*)dequeue(&local_message_q));
		}
		
		if (msg_received->mtype == COUNT_REPORT) {
			int length = strlen(msg_received->mtext);
			if (length > 1 && msg_received->mtext[length - 2] % 2 == 0 && msg_received->mtext[length - 1] == '0') {
				msg_received->mtype = CRT_DISPLAY;
				strcpy(msg_received->mtext, "Process C\r\n");
				send_message(PID_CRT, msg_received);
				
				//hibernate
				msg_to_send = (MSG_BUF*)request_memory_block();
				msg_to_send->mtype = WAKEUP10;
				delayed_send(PID_C, msg_to_send, 10000);
				while (1) {
					msg_received = (MSG_BUF*)receive_message(0);
					if (msg_received->mtype == WAKEUP10) {
						break;
					}
					else {
						enqueue(&local_message_q, (QNode*)message_to_envelope(msg_received));
					}
				}
			}
		}
		release_memory_block(msg_received);
		release_processor();
	}
}
