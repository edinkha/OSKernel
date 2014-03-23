/**
 * @file:   usr_proc.c
 * @brief:  Six user processes: proc1...6 to test our RTX
 * @author: Justin Gagne
 * @date:   2014/03/21
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#ifdef DEBUG_SIM_TIME
#define ONE_SECOND 30
#else
#define ONE_SECOND 1000
#endif

#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "utils.h"
#include "string.h"
#include "forward_list.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void set_test_procs() {
	int i;

	for(i = 0; i < NUM_TEST_PROCS; i++) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority = LOWEST;
		g_test_procs[i].m_stack_size = 0x100;
	}

	g_test_procs[0].mpf_start_pc = &user_test_runner;
	g_test_procs[1].mpf_start_pc = &priority_tests;
	g_test_procs[2].mpf_start_pc = &memory_tests;
	g_test_procs[3].mpf_start_pc = &general_messaging_tests;
	g_test_procs[4].mpf_start_pc = &delayed_messaging_tests;
	g_test_procs[5].mpf_start_pc = &set_priority_command_tests;
}

int done_testing = 0;
int priority_tests_pass = 0;
int set_priority_preempt_check_1 = 0;
int set_priority_preempt_check_2 = 0;
int set_priority_preempt_check_3 = 0;
int set_priority_preempt_check_4 = 0;
int set_priority_preempt_check_5 = 0;
int priority_tests_done = 0;
int block_check_1 = 0;
int block_check_2 = 0;
int memory_tests_pass = 0;
int send_check_1 = 0;
int general_messaging_tests_pass = 0;
int delayed_send_check_1 = 0;
int delayed_messaging_tests_pass = 0;
int num_tests_failed = 0;

/**
 * @brief: This is the process that runs all of the user level tests.
 * It calls user processes 2, 3, 4, 5, and 6 (by sending messages to them) to test different areas of the RTX.
 * We test by checking actual results with expected results, then print the results at the end.
 */
void user_test_runner(void)
{
	MSG_BUF* message_to_unblock;
	void* mem_block_to_unblock;
	MSG_BUF* message_from_PID4;
	MSG_BUF* message_from_PID5;
	int sender_id;

	while (!done_testing) {
		// Print introductory test strings
		uart1_put_string("G023_test: START\r\n");
		uart1_put_string("G023_test: Total 6 Tests\r\n");
		
		// PID_2 ... PID_6 haven't run yet (i.e. they're not blocked yet)
		// So let's release processor so that proc2 can actually run
		release_processor();

		// When we change PID_P2 priority to LOWEST, we will end up back here...
		// Releasing processor so that PID_P2 can resume
		set_priority_preempt_check_5 = 1;
		release_processor();

		// When we change PID_P2 priority from LOWEST -> LOWEST, we should not pre-empt
		// That is, this flag should only be set once priority tests are *actually* done
		priority_tests_done = 1;

		if (priority_tests_pass) {
	        uart1_put_string("G023_test: Test 1 OK\r\n");
	    } else {
	        uart1_put_string("G023_test: Test 1 FAIL\r\n");
	        num_tests_failed++;
	    }

	    // Unblock PID_P3 so we can test memory primitives
	    message_to_unblock = (MSG_BUF*)request_memory_block();
	    message_to_unblock->mtype = DEFAULT;
	    message_to_unblock->mtext[0] = 'U';
	    message_to_unblock->mtext[1] = '\0';
	    send_message(PID_P3, message_to_unblock);

	    // PID_P3 just released processor, so we end up back here
	    // Request a memory block and release processor
	    mem_block_to_unblock = request_memory_block();
	    release_processor();

	    // PID_P3 just got blocked
	    block_check_1 = 1;

	    // Release memory block so that PID_P3 gets unblocked
	    release_memory_block(mem_block_to_unblock);
	    
	    if (memory_tests_pass && block_check_2) {
	        uart1_put_string("G023_test: Test 2 OK\r\n");
	    } else {
	        uart1_put_string("G023_test: Test 2 FAIL\r\n");
	        num_tests_failed++;
	    }

	    // Unblock PID_P4 so we can test messaging primitives
	    message_to_unblock = (MSG_BUF*)request_memory_block();
	    message_to_unblock->mtype = DEFAULT;
	    message_to_unblock->mtext[0] = 'U';
	    message_to_unblock->mtext[1] = '\0';
	    send_message(PID_P4, message_to_unblock);

	    // PID_P4 releases processor so that PID_P1 can be blocked on receive
	    message_from_PID4 = (MSG_BUF*)receive_message(&sender_id);
	    if (sender_id == PID_P4 && release_memory_block(message_from_PID4) != RTX_ERR) {
	    	send_check_1 = 1;
	    }
	    // Let PID_P4 run again
	    release_processor();

	    if (general_messaging_tests_pass) {
	        uart1_put_string("G023_test: Test 3 OK\r\n");
	    } else {
	        uart1_put_string("G023_test: Test 3 FAIL\r\n");
	        num_tests_failed++;
	    }

	    // Unblock PID_P5 so we can test delayed messaging primitive
	    message_to_unblock = (MSG_BUF*)request_memory_block();
	    message_to_unblock->mtype = DEFAULT;
	    message_to_unblock->mtext[0] = 'U';
	    message_to_unblock->mtext[1] = '\0';
	    send_message(PID_P5, message_to_unblock);

	    // PID_P5 releases processor so that PID_P1 can be blocked on receive
	    message_from_PID5 = (MSG_BUF*)receive_message(&sender_id);
	    if (sender_id == PID_P5 && release_memory_block(message_from_PID5) != RTX_ERR) {
					delayed_send_check_1 = 1;
	    }
	    // Let PID_P5 run again
	    message_to_unblock = (MSG_BUF*)request_memory_block();
	    message_to_unblock->mtype = DEFAULT;
	    message_to_unblock->mtext[0] = 'U';
	    message_to_unblock->mtext[1] = '\0';
	    send_message(PID_P5, message_to_unblock);

	    if (delayed_messaging_tests_pass) {
	        uart1_put_string("G023_test: Test 4 OK\r\n");
	    } else {
	        uart1_put_string("G023_test: Test 4 FAIL\r\n");
	        num_tests_failed++;
	    }

	    // Unblock PID_P6 so we can test set priority command process
	    message_to_unblock = (MSG_BUF*)request_memory_block();
	    message_to_unblock->mtype = DEFAULT;
	    message_to_unblock->mtext[0] = 'U';
	    message_to_unblock->mtext[1] = '\0';
	    send_message(PID_P6, message_to_unblock);

	    // Print the total number of tests that passed
	    uart1_put_string("G023_test: ");
	    uart1_put_char('0' + 5 - num_tests_failed);
	    uart1_put_string("/5 Tests OK\r\n");

	    // Print the total number of tests that failed
	    uart1_put_string("G023_test: ");
	    uart1_put_char('0' + num_tests_failed);
	    uart1_put_string("/5 Tests FAIL\r\n");

	    uart1_put_string("G023_test: END\r\n");

	    done_testing = 1;
	}
	while (1) {
	
	}
}

/**
 * @brief: A process that tests the get and set process priority primitives
 */
void priority_tests(void)
{
	MSG_BUF* message_to_unblock;
	int tests_passing = 1;

	// Failure if not all user test procs current priority is set to LOWEST
	// or there's an issue with get_process_priority
	if (get_process_priority(PID_P1) != LOWEST 
		|| get_process_priority(PID_P2) != LOWEST
		|| get_process_priority(PID_P3) != LOWEST
		|| get_process_priority(PID_P4) != LOWEST
		|| get_process_priority(PID_P5) != LOWEST
		|| get_process_priority(PID_P6) != LOWEST) {
		tests_passing = 0;
	}

	// Should be allowed to set this process's priority to HIGH
	if (set_process_priority(PID_P2, HIGH) == RTX_ERR) {
        tests_passing = 0;
    }

    // The previous priority change should have succeeded
    // Should *not* cause pre-emption because all of the other higher priority procs should be blocked
    if (get_process_priority(PID_P2) != HIGH
    	|| set_priority_preempt_check_1
		|| set_priority_preempt_check_2
		|| set_priority_preempt_check_3
		|| set_priority_preempt_check_4
		|| set_priority_preempt_check_5) {
        tests_passing = 0;
    }

    // Set current proc's priority to MEDIUM
    if (set_process_priority(PID_P2, MEDIUM) == RTX_ERR) {
        tests_passing = 0;
    }

    // Priority change should have been successful
    // Should *not* cause pre-emption because all of the other higher priority procs should be blocked
    if (get_process_priority(PID_P2) != MEDIUM
    	|| set_priority_preempt_check_1
		|| set_priority_preempt_check_2
		|| set_priority_preempt_check_3
		|| set_priority_preempt_check_4
		|| set_priority_preempt_check_5) {
        tests_passing = 0;
    }

    // Set current proc's priority to LOW
    if (set_process_priority(PID_P2, LOW) == RTX_ERR) {
        tests_passing = 0;
    }

    // Priority change should have been successful
    // Should *not* cause pre-emption because all of the other higher priority procs should be blocked
    if (get_process_priority(PID_P2) != LOW
    	|| set_priority_preempt_check_1
		|| set_priority_preempt_check_2
		|| set_priority_preempt_check_3
		|| set_priority_preempt_check_4
		|| set_priority_preempt_check_5) {
        tests_passing = 0;
    }

    // Set current proc's priority to LOWEST, giving other procs a chance to run and get blocked
    if (set_process_priority(PID_P2, LOWEST) == RTX_ERR) {
        tests_passing = 0;
    }

    // Contrary to above, setting priority to LOWEST *SHOULD* cause pre-emption
    // PID_P3, PID_P4, PID_P5, and PID_P6 will all run and get blocked on receive
    // and PID_P1 will release_processor so that we end up here
    if (get_process_priority(PID_P2) != LOWEST
    	|| !set_priority_preempt_check_1
    	|| !set_priority_preempt_check_2
    	|| !set_priority_preempt_check_3
    	|| !set_priority_preempt_check_4
    	|| !set_priority_preempt_check_5) {
        tests_passing = 0;
    }

    // Should *NOT* allow setting the null process's priority
    if (set_process_priority(PID_NULL, HIGH) != RTX_ERR) {
        tests_passing = 0;
    }

    // Should *NOT* allow setting the priority to anything below 0
    if (set_process_priority(PID_P2, -1) != RTX_ERR) {
        tests_passing = 0;
    }

    // Should *NOT* allow setting the priority to anything above 3
    if (set_process_priority(PID_P2, 4) != RTX_ERR) {
        tests_passing = 0;
    }

    // Should *NOT* allow setting the priority of an iproc
    if (set_process_priority(PID_UART_IPROC, HIGH) != RTX_ERR) {
        tests_passing = 0;
    }

    // Should *allow* setting priority to current priority
    if (set_process_priority(PID_P2, LOWEST) == RTX_ERR) {
        tests_passing = 0;
    }

    // This *should NOT* cause pre-emption using our design choice
    if (get_process_priority(PID_P2) != LOWEST
    	|| priority_tests_done) {
        tests_passing = 0;
    }

    // Send test case result back to PID_P1
    // If 0 -> test case failed
    // If 1 -> test case passed
	priority_tests_pass = tests_passing;

	// Request a message to block this process
	// This avoids running into PID_P2 accidentally / anytime after running the priority tests
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);

	// Theoretically should never get here, but just to be safe...
	release_memory_block(message_to_unblock);
}

/**
 * @brief: a process responsible for testing the request and release memory block primitives
 */
void memory_tests(void)
{
	MSG_BUF* message_to_unblock;
	void* mem_block_1;
	void* mem_block_2;
	ForwardList all_mem_blocks;
	int tests_passing = 1;

	// Should only get here once priority_tests changes its priority to LOWEST
	set_priority_preempt_check_1 = 1;

	// Block process by receiving message so that next proc (LOWEST PRIORITY) can run
	// Note: We don't care about who sent the message, but in this case, it should be user_test_runner
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);
	// We can safely release the memory block used to get this far
	release_memory_block(message_to_unblock);

	// We should get unblocked by user_test_runner after running priority tests
	// Test that requesting a block, releasing it, then requesting it again gets the same block
	mem_block_1 = request_memory_block();
	mem_block_2 = mem_block_1;

	// Test basic memory block release
	if (release_memory_block(mem_block_1) == RTX_ERR) {
		tests_passing = 0;
	}

	mem_block_1 = (void*)0;
	if (mem_block_1 == mem_block_2) {
		tests_passing = 0;
	}

	mem_block_1 = request_memory_block();
	
	if (mem_block_1 != mem_block_2) {
		tests_passing = 0;
	}

	mem_block_2 = request_memory_block();

	// After requesting 2nd memblock, addresses should be different
	if (mem_block_1 == mem_block_2) {
		tests_passing = 0;
	}

	// Release two blocks at once
	if (release_memory_block(mem_block_1) == RTX_ERR
		|| release_memory_block(mem_block_2) == RTX_ERR) {
		tests_passing = 0;
	}

	// For this section, we want the following to happen:
	// Release processor so that PID_P1 can run
	release_processor();
	// Then, back in PID_P1, we request a memory block and release processor
	// We end up back here, and we want to request every memory block available (i.e. block on memory)
	init(&all_mem_blocks);
	while (!block_check_1) {
		push_front(&all_mem_blocks, (ListNode*)request_memory_block());
	}
	// We should end up at PID_P1 again, where we release the previously requested memory block
	// We should now be unblocked, and we should release all of the memory we just requested
	block_check_2 = 1;

	while (!empty(&all_mem_blocks)) {
		if (release_memory_block(pop_front(&all_mem_blocks)) == RTX_ERR) {
			tests_passing = 0;
		}
	}

	// Send test case result back to PID_P1
    // If 0 -> test case failed
    // If 1 -> test case passed
	memory_tests_pass = tests_passing;

	// Request a message to block this process
	// This avoids running into PID_P2 accidentally / anytime after running the priority tests
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);

	// Theoretically should never get here, but just to be safe...
	release_memory_block(message_to_unblock);
}

/**
 * @brief: a process responsible for stopping the Wall Clock
 */
void general_messaging_tests(void)
{
	MSG_BUF* message_to_unblock;
	MSG_BUF* message_to_send;
	MSG_BUF* message_received;
	int tests_passing = 1;
	int sender_id;

	// Should only get here once priority_tests changes its priority to LOWEST
	set_priority_preempt_check_2 = 1;

	// Block process by receiving message so that next proc (LOWEST PRIORITY) can run
	// Note: We don't care about who sent the message, but in this case, it should be user_test_runner
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);
	// We can safely release the memory block used to get this far
	release_memory_block(message_to_unblock);

	// Send message to self
	message_to_send = (MSG_BUF*)request_memory_block();
	message_to_send->mtype = DEFAULT;
	message_to_send->mtext[0] = 'S';
	message_to_send->mtext[1] = '\0';
	send_message(PID_P4, message_to_send);

	// Ensure that address of message received is the same as that of the message sent 
	// and that sender_id is PID_P4
	message_received = (MSG_BUF*)receive_message(&sender_id);
	if (message_received != message_to_send
		|| sender_id != PID_P4) {
		tests_passing = 0;
	}
	if (release_memory_block(message_received) == RTX_ERR) {
		tests_passing = 0;
	}

	// Ensure that sending a message to a blocked on receive process (PID_P1) unblocks it 
	// immediately if it's supposed to
	release_processor();
	message_to_send = (MSG_BUF*)request_memory_block();
	message_to_send->mtype = DEFAULT;
	message_to_send->mtext[0] = 'S';
	message_to_send->mtext[1] = '\0';
	send_message(PID_P1, message_to_send);

	// Should have switched to P1, set flag, and come back here
	if (!send_check_1) {
		tests_passing = 0;
	}

	// Send test case result back to PID_P1
    // If 0 -> test case failed
    // If 1 -> test case passed
	general_messaging_tests_pass = tests_passing;

	// Request a message to block this process
	// This avoids running into PID_P2 accidentally / anytime after running the priority tests
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);

	// Theoretically should never get here, but just to be safe...
	release_memory_block(message_to_unblock);
}

/**
 * @brief: a process responsible for testing delayed messaging
 */
void delayed_messaging_tests(void)
{
	MSG_BUF* message_to_unblock;
	MSG_BUF* message_to_send;
	MSG_BUF* message_received;
	int tests_passing = 1;
	int sender_id;

	// Should only get here once priority_tests changes its priority to LOWEST
	set_priority_preempt_check_3 = 1;

	// Block process by receiving message so that next proc (LOWEST PRIORITY) can run
	// Note: We don't care about who sent the message, but in this case, it should be user_test_runner
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);
	// We can safely release the memory block used to get this far
	release_memory_block(message_to_unblock);
	
	// Send message to self after 2 seconds
	message_to_send = (MSG_BUF*)request_memory_block();
	message_to_send->mtype = DEFAULT;
	message_to_send->mtext[0] = 'S';
	message_to_send->mtext[1] = '\0';
	delayed_send(PID_P5, message_to_send, 2 * ONE_SECOND);

	// Ensure that address of message received is the same as that of the message sent 
	// and that sender_id is PID_P5
	message_received = (MSG_BUF*)receive_message(&sender_id);
	if (message_received != message_to_send
		|| sender_id != PID_P5) {
		tests_passing = 0;
	}
	if (release_memory_block(message_received) == RTX_ERR) {
		tests_passing = 0;
	}

	// Ensure that sending a message to a blocked on receive process (PID_P1) unblocks it 
	// after about 3 seconds
	release_processor();
	message_to_send = (MSG_BUF*)request_memory_block();
	message_to_send->mtype = DEFAULT;
	message_to_send->mtext[0] = 'S';
	message_to_send->mtext[1] = '\0';
	delayed_send(PID_P1, message_to_send, 3 * ONE_SECOND);
	
	// Request a message to block this process until PID_P1 received the delayed message
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);
	release_memory_block(message_to_unblock);

	// Should have switched to P1, set flag, and come back here
	if (!delayed_send_check_1) {
		tests_passing = 0;
	}

	// Send test case result back to PID_P1
    // If 0 -> test case failed
    // If 1 -> test case passed
	delayed_messaging_tests_pass = tests_passing;

	// Request a message to block this process
	// This avoids running into PID_P5 accidentally / anytime after running the priority tests
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);

	// Theoretically should never get here, but just to be safe...
	release_memory_block(message_to_unblock);

}

/**
 * @brief: a process responsible for testing the set priority command process
 */
void set_priority_command_tests(void)
{
	MSG_BUF* message_to_unblock;


	// Should only get here once priority_tests changes its priority to LOWEST
	set_priority_preempt_check_4 = 1;

	// Block process by receiving message so that next proc (LOWEST PRIORITY) can run
	// Note: We don't care about who sent the message, but in this case, it should be user_test_runner
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);
}
