/**
 * @file:   usr_proc.c
 * @brief:  Six user processes: proc1...6 to test our RTX
 * @author: Justin Gagne
 * @date:   2014/03/21
 * NOTE: Each process is in an infinite loop. Processes never terminate.
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

	for(i = 0; i < NUM_TEST_PROCS; i++) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority = LOWEST;
		g_test_procs[i].m_stack_size = 0x100;
	}

	g_test_procs[0].mpf_start_pc = &user_test_runner;
	g_test_procs[1].mpf_start_pc = &priority_tests;
	g_test_procs[2].mpf_start_pc = &memory_tests;
	g_test_procs[3].mpf_start_pc = &wall_clock_tests;
	g_test_procs[4].mpf_start_pc = &general_messaging_tests;
	g_test_procs[5].mpf_start_pc = &delayed_messaging_tests;
}

int done_testing = 0;
int priority_tests_pass = 0;
int set_priority_preempt_check_1 = 0;
int set_priority_preempt_check_2 = 0;
int set_priority_preempt_check_3 = 0;
int set_priority_preempt_check_4 = 0;
int set_priority_preempt_check_5 = 0;
int priority_tests_done = 0;
int memory_tests_pass = 0;
int num_tests_failed = 0;

/**
 * @brief: This is the process that runs all of the user level tests.
 * It calls user processes 2, 3, 4, 5, and 6 (by sending messages to them) to test different areas of the RTX.
 * We test by checking actual results with expected results, then print the results at the end.
 */
void user_test_runner(void)
{
	MSG_BUF* message_to_unblock;

	while (!done_testing) {
		// Print introductory test strings
		uart1_put_string("G023_test: START\r\n");
		uart1_put_string("G023_test: Total 6 Tests\r\n");

		message_to_unblock = (MSG_BUF*)request_memory_block();
		
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
	    message_to_unblock->mtype = DEFAULT;
	    message_to_unblock->mtext[0] = 'U';
	    message_to_unblock->mtext[1] = '\0';
	    send_message(PID_P3, message_to_unblock);

	    if (memory_tests_pass) {
	        uart1_put_string("G023_test: Test 2 OK\r\n");
	    } else {
	        uart1_put_string("G023_test: Test 2 FAIL\r\n");
	        num_tests_failed++;
	    }

	    // Print the total number of tests that passed
	    uart1_put_string("G023_test: ");
	    uart1_put_char('0' + 6 - num_tests_failed);
	    uart1_put_string("/6 Tests OK\r\n");

	    // Print the total number of tests that failed
	    uart1_put_string("G023_test: ");
	    uart1_put_char('0' + num_tests_failed);
	    uart1_put_string("/6 Tests FAIL\r\n");

	    uart1_put_string("G023_test: END\r\n");

	    done_testing = 1;
	}
	while (1) {
	
	}
}

/**
 * @brief: A process that thoroughly tests all parts of get/set priority API methods
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
 * @brief: a process responsible for setting the wall clock to 10:10:10
 */
void memory_tests(void)
{
	MSG_BUF* message_to_unblock;
	void* mem_block_1;
	void* mem_block_2;
	int tests_passing = 1;

	// Should only get here once priority_tests changes its priority to LOWEST
	set_priority_preempt_check_1 = 1;

	// Block process by receiving message so that next proc (LOWEST PRIORITY) can run
	// Note: We don't care about who sent the message, but in this case, it should be user_test_runner
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);

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

	// Send test case result back to PID_P1
    // If 0 -> test case failed
    // If 1 -> test case passed
	memory_tests_pass = tests_passing;

	// Release the memory block we used to run these tests
	release_memory_block(message_to_unblock);

	// Request a message to block this process
	// This avoids running into PID_P2 accidentally / anytime after running the priority tests
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);

	// Theoretically should never get here, but just to be safe...
	release_memory_block(message_to_unblock);
}

/**
 * @brief: a process responsible for resetting the Wall Clock
 */
void wall_clock_tests(void)
{
	MSG_BUF* message_to_unblock;


	// Should only get here once priority_tests changes its priority to LOWEST
	set_priority_preempt_check_2 = 1;

	// Block process by receiving message so that next proc (LOWEST PRIORITY) can run
	// Note: We don't care about who sent the message, but in this case, it should be user_test_runner
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);
}

/**
 * @brief: a process responsible for stopping the Wall Clock
 */
void general_messaging_tests(void)
{
	MSG_BUF* message_to_unblock;

	// Should only get here once priority_tests changes its priority to LOWEST
	set_priority_preempt_check_3 = 1;

	// Block process by receiving message so that next proc (LOWEST PRIORITY) can run
	// Note: We don't care about who sent the message, but in this case, it should be user_test_runner
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);
}

/**
 * @brief: a process responsible for the three hot keys
 */
void delayed_messaging_tests(void)
{
	MSG_BUF* message_to_unblock;

	// Should only get here once priority_tests changes its priority to LOWEST
	set_priority_preempt_check_4 = 1;

	// Block process by receiving message so that next proc (LOWEST PRIORITY) can run
	// Note: We don't care about who sent the message, but in this case, it should be user_test_runner
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);
}
