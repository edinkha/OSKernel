/**
 * @file:   usr_proc.c
 * @brief:  Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date:   2014/02/28
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 
		To demonstrate how interrupt can preempt a current running process
		There are two processes in the system: proc1 and proc2.

		You will need to open two UART terminals.
		The COM0 terminal is interrupt driven and accepts user input. 
		When user types 'S' single char in the terminal, the system switches from the current running process to the other ready process

		The COM1 terminal uses polling mechanism and output the debugging messages.
		The user proc1 and proc2 outputs for now to the debugging terminal for now.

		Expected behavior:

		If user does not type anything from the COM0 or types a character other than 'S', then 
		proc1 and proc2 will switch between each other after six lines are printed.
		proc1 prints six lines of upper case letters.
		proc2 prints six lines of single digits.

		When the user types an 'S' in COM0, then the current running process immediately releases
		the processor and switches the cpu to the other process in the system.

		Note there is a dummy loop added to introduce some delay so that it is easier to type 'S' before a process finishes printing six lines.

		The code has not been tested on the board. To run it on the board, you will need to increase the delay.
*/

#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "utils.h"
#include "string.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void set_test_procs()
{
	int i;
	for (i = 0; i < NUM_TEST_PROCS; i++) {
		g_test_procs[i].m_pid = (U32)(i + 1);
		g_test_procs[i].m_priority = LOWEST;
		g_test_procs[i].m_stack_size = USR_SZ_STACK;
	}
	
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
}


/**
 * @brief: a process that prints 5x6 uppercase letters
 *         and then yields the cpu.
 */
void proc1(void)
{
	int i = 0;
	int ret_val = 10;
	int x = 0;

	while ( 1) {
		if ( i != 0 && i%5 == 0 ) {
			uart1_put_string("\n\r");
			
			if ( i%30 == 0 ) {
				ret_val = release_processor();
#ifdef DEBUG_0
				printf("proc1: ret_val=%d\n", ret_val);
			
#endif /* DEBUG_0 */
			}
			for ( x = 0; x < 500000; x++); // some artifical delay
		}
		uart1_put_char('A' + i%26);
		i++;
		
	}
}

/**
 * @brief: a process that prints 5x6 numbers
 *         and then yields the cpu.
 */
void proc2(void)
{
	int i = 0;
	int ret_val = 20;
	int x = 0;
	while ( 1) {
		if ( i != 0 && i%5 == 0 ) {
			uart1_put_string("\n\r");
			
			if ( i%30 == 0 ) {
				ret_val = release_processor();
#ifdef DEBUG_0
				printf("proc2: ret_val=%d\n", ret_val);
			
#endif /* DEBUG_0 */
			}
			for ( x = 0; x < 500000; x++); // some artifical delay
		}
		uart1_put_char('0' + i%10);
		i++;
		
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
void procWC()
{
	int hours;
	int minutes;
	int seconds;
	int is_running;
	int* sender_id;
	MSG_BUF* msg_received;
	MSG_BUF* msg_to_send = (MSG_BUF*)request_memory_block();
	
	// Tell the KCD to register the "%W" command with the wall clock process
	msg_to_send->mtype = KCD_REG;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'W';
	msg_to_send->mtext[2] = '\0';
	send_message(PID_KCD, (void*)msg_to_send);
	
	// Initialize the is_running variable to 0 (meaning false)
	is_running = 0;
	
	while (1) {
		// Receive message from KCD (command input), or timer (to display time)
		msg_received = (MSG_BUF*)receive_message(sender_id);
		
		if (msg_received->mtext[2] == 'T') {
			is_running = 0;
		}
		else {
			if (msg_received->mtext[2] == 'R') {
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
		}
		
		// Release the memory of the received message
		release_memory_block(msg_received);
	}
}

