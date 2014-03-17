/**
 * @file:   usr_proc.c
 * @brief:  Six user processes: proc1...6 to do simple processor managment testing
 * @author: Evet DInkha
 * @date:   2014/03/15
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 *
 * Test cases:
 starting priorities:
{proc1, ..., proc6} = {HIGH, MEDIUM, MEDIUM, LOWEST, LOWEST, LOW}

proc1 is HIGH priority => immediately blocks on receive message
proc2 is MEDIUM priority => sends !, @, # hotkeys
	=> expect on Ready PQ: proc3, proc4, proc5, proc6, wallclock
	=> expect on Blocked on Mem PQ: none
	=> expect on Blocked on Receive PQ: UART iProc, Timer iProc, proc1
	=> set proc4 priority to HIGH
proc4 is now HIGH priority => sets wall clock to 234:123:12 (i.e. incorrect time)
	=> expect error message
	=> sends incorrect KCD_REG to KCD
	=> expect error message
	=> sets wall clock to 23:59:55
	=> expect wall clock to be set correctly
	=> 
  => sets priority to LOWEST
proc3 is MEDIUM priority => sends delayed message to proc5
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



void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_stack_size=0x100;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[0].m_priority   = LOW;

	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[1].m_priority   = MEDIUM;

	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[2].m_priority   = LOW;

	g_test_procs[3].mpf_start_pc = &proc4;
	g_test_procs[3].m_priority   = LOW;

	g_test_procs[4].mpf_start_pc = &proc5;
	g_test_procs[4].m_priority   = LOW;

	g_test_procs[5].mpf_start_pc = &proc6;
	g_test_procs[5].m_priority   = LOW;

}
int results_printed = 0,
	  block1_sent_message = RTX_ERR,
	  block1_set_priority= RTX_ERR,
	  block1_set_priority2= RTX_ERR,

		block2_sent_message = RTX_ERR,
	  block2_set_priority= RTX_ERR,

		block3_sent_message = RTX_ERR,
		block3_set_priority= RTX_ERR,

		block4_sent_message = RTX_ERR,

		block5_sent_message = RTX_ERR,

		block6_sent_message = RTX_ERR,

		wall_clock_set = RTX_ERR,
		wall_clock_reset = RTX_ERR,
		wall_clock_terminate = RTX_ERR,
		hot_key1_value = RTX_ERR,
		hot_key2_value = RTX_ERR,
		hot_key3_value = RTX_ERR,
		num_tests_passed = 0,
		num_tests_failed = 0;

/**
 * @brief: a process that sends the messages it receives
 */
void proc1(void)
{
	int i = 0;
	int ret_val = 20;
	int sender_id;
	MSG_BUF* msg_received;
	MSG_BUF* msg_to_send = (MSG_BUF*)request_memory_block();

	ret_val = set_process_priority(PID_P3, LOW);
	
	while (1) {
		msg_received = (MSG_BUF*)receive_message(&sender_id);
		if (sender_id == PID_KCD && msg_received->mtext[2] == 'S') {	
				msg_to_send->mtype = CRT_DISPLAY;
				strcpy(msg_to_send->mtext, msg_received->mtext);

				block1_sent_message = send_message(PID_CRT, (void*)msg_to_send);
				block1_set_priority = set_process_priority(PID_P3, LOW);
				block1_set_priority2= set_process_priority(PID_P4, HIGH);
		} else if (sender_id == PID_KCD && msg_received->mtext[2] == 'R') {
				msg_to_send->mtype = CRT_DISPLAY;
				strcpy(msg_to_send->mtext, msg_received->mtext);

				ret_val = send_message(PID_CRT, (void*)msg_to_send);
				ret_val = set_process_priority(PID_P5, HIGH);
		}	else if (sender_id == PID_KCD && msg_received->mtext[2] == 'T') {
				msg_to_send->mtype = CRT_DISPLAY;
				strcpy(msg_to_send->mtext, msg_received->mtext);

				ret_val = send_message(PID_CRT, (void*)msg_to_send);
				ret_val = set_process_priority(PID_P6, HIGH);
		}	else if(sender_id == PID_KCD && msg_received->mtext[2] == '!') {
				msg_to_send->mtype = CRT_DISPLAY;
				strcpy(msg_to_send->mtext, msg_received->mtext);

				ret_val = send_message(PID_CRT, (void*)msg_to_send);
		}	else if(sender_id == PID_KCD && msg_received->mtext[2] == '@') {
				msg_to_send->mtype = CRT_DISPLAY;
				strcpy(msg_to_send->mtext, msg_received->mtext);

				ret_val = send_message(PID_CRT, (void*)msg_to_send);
		}	else if(sender_id == PID_KCD && msg_received->mtext[2] == '#') {

				msg_to_send->mtype = CRT_DISPLAY;
				strcpy(msg_to_send->mtext, msg_received->mtext);

				ret_val = send_message(PID_CRT, (void*)msg_to_send);
				break;
		}

			#ifdef DEBUG_0
				printf("proc1: ret_val=%d\n", ret_val);
			#endif /* DEBUG_0 */
			uart1_put_char('0' + i%10);
			i++;

			#ifdef DEBUG_1
			if (!results_printed) {
				wall_clock_set = block1_sent_message && block1_set_priority && block1_set_priority2;
				wall_clock_reset = block2_sent_message && block2_set_priority;
				wall_clock_terminate = block3_sent_message && block3_set_priority;
				hot_key1_value = block4_sent_message;
				hot_key2_value = block5_sent_message;
				hot_key3_value = block6_sent_message;
				
				if (wall_clock_set != RTX_OK) {
					printf("G023_test: test 1 OK\r\n");
					num_tests_passed++;
				} else {
					printf("G023_test: test 1 FAIL\r\n");
					num_tests_failed++;
				}
				
				if (wall_clock_reset != RTX_OK) {
					printf("G023_test: test 2 OK\r\n");
					num_tests_passed++;
				} else {
					printf("G023_test: test 2 FAIL\r\n");
					num_tests_failed++;
				}
				
				if (wall_clock_terminate != RTX_OK) {
					printf("G023_test: test 3 OK\r\n");
					num_tests_passed++;
				} else {
					printf("G023_test: test 3 FAIL\r\n");
					num_tests_failed++;
				}
				
				if (hot_key1_value != RTX_OK) {
					printf("G023_test: test 4 OK\r\n");
					num_tests_passed++;
				} else {
					printf("G023_test: test 4 FAIL\r\n");
					num_tests_failed++;
				}
				
				if (hot_key2_value != RTX_OK) {
					printf("G023_test: test 5 OK\r\n");
					num_tests_passed++;
				} else {
					printf("G023_test: test 5 FAIL\r\n");
					num_tests_failed++;
				}

				if (hot_key3_value != RTX_OK) {
					printf("G023_test: test 6 OK\r\n");
					num_tests_passed++;
				} else {
					printf("G023_test: test 6 FAIL\r\n");
					num_tests_failed++;
				}
				
				printf("G023_test: 6/6 tests OK\r\n", num_tests_passed);
				printf("G023_test: 0/6 tests FAIL\r\n", num_tests_failed);

				printf("G023_test: END\r\n");
				
				results_printed = 1;
			}
		#endif  /* DEBUG_1 */

		uart1_put_string("proc1 end of testing\n\r");
		while (1) {
			release_processor();
		}
	}
}

/**
 * @brief: a process that prints 4x5 numbers and changes the priority of p3
 */
void proc2(void)
{
	int i = 0;
	int ret_val = 20;
	int counter = 0;

	while (1) {
		if (i != 0 && i%5 == 0) {
			uart1_put_string("\n\r");
			counter++;
			if (counter == 1) {
				ret_val = set_process_priority(PID_P3, HIGH);
				break;
			} else {
				ret_val = release_processor();
			}
		#ifdef DEBUG_0
			printf("proc2: ret_val=%d\n", ret_val);
		#endif /* DEBUG_0 */
		}
		uart1_put_char('0' + i%10);
		i++;
	}
	uart1_put_string("proc2 end of testing\n\r");
	while (1) {
		release_processor();
	}
}

/**
 * @brief: a process responsible for setting the wall clock to 10:10:10
 */
void proc3(void)
{
	int ret_val = 100;

	MSG_BUF* msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = PID_KCD;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'W';
	msg_to_send->mtext[2] = 'S';
	msg_to_send->mtext[3] = ' ';
	msg_to_send->mtext[4] = '1';
	msg_to_send->mtext[5] = '0';
	msg_to_send->mtext[6] = ':';
	msg_to_send->mtext[7] = '1';
	msg_to_send->mtext[8] = '0';
	msg_to_send->mtext[9] = ':';
	msg_to_send->mtext[10] = '1';
	msg_to_send->mtext[11] = '0';

	uart1_put_string("%WS 10:10:10\r\n");
	
	ret_val = send_message(PID_P1, (void*) msg_to_send);
	ret_val = set_process_priority(PID_P1, HIGH);

	#ifdef DEBUG_0
			printf("proc3: ret_val = %d \n", ret_val);
	#endif /* DEBUG_0 */

	uart1_put_string("proc3 end of testing\n\r");
	
	while ( 1 ) {
		release_processor();
	}
}

/**
 * @brief: a process responsible for resetting the Wall Clock
 */
void proc4(void)
{
	int i = 0;
	int counter = 0;
	int ret_val = 100;

	MSG_BUF* msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = PID_KCD;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'W';
	msg_to_send->mtext[1] = 'R';

	while (1) {

		if (i != 0 && i%5 == 0) {
			counter++;
			if (counter == 2) {
				ret_val = send_message(PID_P1, (void*) msg_to_send);
				break;
			} else {
				ret_val = release_processor();
			}

			#ifdef DEBUG_0
				printf("proc4: ret_val = %d \n", ret_val);
			#endif /* DEBUG_0 */
		}
		
		i++;
	}
	
	uart1_put_string("proc4 end of testing\n\r");
	
	while (1) {
		release_processor();
	}
}

/**
 * @brief: a process responsible for stopping the Wall Clock
 */
void proc5(void)
{
	int i = 0;
	int counter = 0;
	int ret_val = 100;

	MSG_BUF* msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = PID_KCD;
	msg_to_send->mtext[0] = '%';
	msg_to_send->mtext[1] = 'W';
	msg_to_send->mtext[1] = 'T';

	while (1) {
		if (i != 0 && i%10 == 0) {
			counter++;
			if (counter == 5) {
				ret_val = send_message(PID_P1, (void*) msg_to_send);
				break;
			} else {
				ret_val = release_processor();
			}

			#ifdef DEBUG_0
				printf("proc5: ret_val = %d \n", ret_val);
			#endif /* DEBUG_0 */
		}
		i++;
	}
	
	uart1_put_string("proc5 end of testing\n\r");
	
	while ( 1 ) {
		release_processor();
	}
}

/**
 * @brief: a process responsible for the three hot keys
 */
void proc6(void)
{
	int i = 0;
	int counter = 0;
	int ret_val = 10;

	MSG_BUF* msg_to_send = (MSG_BUF*)request_memory_block();
	msg_to_send->mtype = CRT_DISPLAY;

	while (1) {
		if (i != 0 && i%5== 0) {
			counter++;
			if (counter == 1) {
				msg_to_send->mtext[0] = '!';
				ret_val = send_message(PID_P1, (void*) msg_to_send);
			} else if (counter == 2) {
				msg_to_send->mtext[0] = '@';
				ret_val = send_message(PID_P1, (void*) msg_to_send);
			}	else if (counter == 2) {
				msg_to_send->mtext[0] = '#';
				ret_val = send_message (PID_P1, (void*) msg_to_send);
				break;
			}

			#ifdef DEBUG_0
				printf("proc6: ret_val = %d \n", ret_val);
			#endif /* DEBUG_0 */
		}
		i++;
	}
	
	uart1_put_string("proc6 end of testing\n\r");
	
	while ( 1 ) {
		release_processor();
	}
}
