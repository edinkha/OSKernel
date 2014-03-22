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

/**
 * @brief: This is the process that runs all of the user level tests.
 * It calls user processes 2, 3, 4, 5, and 6 (by sending messages to them) to test different areas of the RTX.
 * We test by checking actual results with expected results, then print the results at the end.
 */
void user_test_runner(void)
{
	MSG_BUF* message_to_unblock;

	while (!done_testing) {
		message_to_unblock = (MSG_BUF*)request_memory_block();

		// Begin by testing priority by sending message to priority_tests, which is blocked on receive
		send_message(PID_P2, message_to_unblock);

		// When we change PID_P2 priority to LOWEST, we will end up back here...
		// Releasing processor so that PID_P2 can resume
		set_priority_preempt_check_5 = 1;
		release_processor();

		// When we change PID_P2 priority from LOWEST -> LOWEST, we should not pre-empt
		// That is, this flag should only be set once priority tests are *actually* done
		priority_tests_done = 1;

	}
	while (1) {
	
	}
}

/**
 * @brief: a process that prints 4x5 numbers and changes the priority of p3
 */
void priority_tests(void)
{
	MSG_BUF* message_to_unblock;
	int tests_passing = 0;

	// Note: We don't care about who sent the message, but in this case, it should be user_test_runner
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);

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

	// Release the memory block used to unblock this process
	release_memory_block(message_to_unblock);

	// Then request another message to block the process again
	// This avoids running into PID_P2 accidentally / anytime after running the priority tests
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);
}

/**
 * @brief: a process responsible for setting the wall clock to 10:10:10
 */
void memory_tests(void)
{
	MSG_BUF* message_to_unblock;

	// Should only get here once priority_tests changes its priority to LOWEST
	set_priority_preempt_check_1 = 1;

	// Block process by receiving message so that next proc (LOWEST PRIORITY) can run
	// Note: We don't care about who sent the message, but in this case, it should be user_test_runner
	message_to_unblock = (MSG_BUF*)receive_message((int*)0);
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
