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

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x100;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;

	initWC();
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
 * @brief: A process that initially registers the values of the wall clock to the 
 *         KCD. 
 */
void initWC(void){

	MSG_BUF* message_to_register;

	// send "%WR" message to the KCD to register
	message_to_register = (MSG_BUF*)k_request_memory_block();
	message_to_register->mtype = KCD_REGISTER;
	message_to_register->mtext[0] = '%';
	message_to_register->mtext[1] = 'W';
	message_to_register->mtext[2] = 'R';
	k_send_message(PID_KCD, (void*)message_to_register);


	// send "%WS" message to the KCD to register
	message_to_register->mtype = KCD_REGISTER;
	message_to_register->mtext[0] = '%';
	message_to_register->mtext[1] = 'W';
	message_to_register->mtext[2] = 'R';
	k_send_message(PID_KCD, (void*)message_to_register);

	// send "%WT" message to the KCD to register
	message_to_register->mtype = KCD_REGISTER;
	message_to_register->mtext[0] = '%';
	message_to_register->mtext[1] = 'W';
	message_to_register->mtext[2] = 'R';
	k_send_message(PID_KCD, (void*)message_to_register);

}






