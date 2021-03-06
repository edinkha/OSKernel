/**
 * @brief: uart_irq.c
 * @author: NXP Semiconductors
 * @author: Y. Huang
 * @date: 2014/02/28
 */

#include <LPC17xx.h>
#include "uart.h"
#include "uart_polling.h"
#include "k_rtx.h"
#include "i_proc.h"
#include "k_process.h"
#ifdef DEBUG_0
#include "printf.h"
#endif
#ifdef DEBUG_HK
#include <assert.h>
#endif

#define BIT(X)(1 << (X))

extern U32 g_switch_flag;
extern PCB* gp_current_process;
extern PCB* get_proc_by_pid(int pid);

volatile uint32_t g_timer_count = 0; // increment every 1 ms
volatile uint32_t g_bench_timer_count = 0;

ForwardList* delayed_messages; // List of delayed messages
PCB* timer_proc;

char g_buffer[];
char* gp_buffer = g_buffer;
char g_char_in;
char g_char_out;


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

/**
 * @brief: initialize timer. Only timer 0 is supported
 */
uint32_t timer_init(uint8_t n_timer)
{
	LPC_TIM_TypeDef* pTimer;
	if (n_timer == 0) {
		/*
		Steps 1 & 2: system control configuration.
		Under CMSIS, system_LPC17xx.c does these two steps
		
		-----------------------------------------------------
		Step 1: Power control configuration.
		        See table 46 pg63 in LPC17xx_UM
		-----------------------------------------------------
		Enable UART0 power, this is the default setting
		done in system_LPC17xx.c under CMSIS.
		Enclose the code for your refrence
		//LPC_SC->PCONP |= BIT(1);
		
		-----------------------------------------------------
		Step2: Select the clock source,
		       default PCLK=CCLK/4 , where CCLK = 100MHZ.
		       See tables 40 & 42 on pg56-57 in LPC17xx_UM.
		-----------------------------------------------------
		Check the PLL0 configuration to see how XTAL=12.0MHZ
		gets to CCLK=100MHZ in system_LPC17xx.c file.
		PCLK = CCLK/4, default setting in system_LPC17xx.c.
		Enclose the code for your reference
		//LPC_SC->PCLKSEL0 &= ~(BIT(3)|BIT(2));
		
		-----------------------------------------------------
		Step 3: Pin Ctrl Block configuration.
		        Optional, not used in this example
		        See Table 82 on pg110 in LPC17xx_UM
		-----------------------------------------------------
		*/
		
		pTimer = (LPC_TIM_TypeDef*) LPC_TIM0;
		
	}
	else if (n_timer == 1) {
		
		pTimer = (LPC_TIM_TypeDef*) LPC_TIM1;
		
	}
	else {   /* other timers not supported */
		return 1;
	}
	
	/*
	-----------------------------------------------------
	Step 4: Interrupts configuration
	-----------------------------------------------------
	*/
	
	/* Step 4.1: Prescale Register PR setting
	   CCLK = 100 MHZ, PCLK = CCLK/4 = 25 MHZ
	   2*(12499 + 1)*(1/25) * 10^(-6) s = 10^(-3) s = 1 ms
	   TC (Timer Counter) toggles b/w 0 and 1 every 12500 PCLKs
	   see MR setting below
	*/
	pTimer->PR = 12499;
	
	/* Step 4.2: MR setting, see section 21.6.7 on pg496 of LPC17xx_UM. */
	pTimer->MR0 = 1;
	
	/* Step 4.3: MCR setting, see table 429 on pg496 of LPC17xx_UM.
	   Interrupt on MR0: when MR0 mathches the value in the TC,
	                     generate an interrupt.
	   Reset on MR0: Reset TC if MR0 mathches it.
	*/
 	pTimer->MCR = BIT(0) | BIT(1); // MR0
	
	/* Step 4.4: CSMSIS enable timer IRQ */
	if (n_timer == 0) {
		g_timer_count = 0;
		NVIC_EnableIRQ(TIMER0_IRQn);
	}
	else {
		g_bench_timer_count = 0;
		NVIC_EnableIRQ(TIMER1_IRQn);
	}
	
	/* Step 4.5: Enable the TCR. See table 427 on pg494 of LPC17xx_UM. */
	pTimer->TCR = 1;
	
	return 0;
}

/**
 * Simply returns the time
 */
uint32_t get_current_time(void)
{
	return g_timer_count;
}

/**
 * Simply returns the benchmark time
 */
uint32_t get_current_bench_time(void)
{
	return g_bench_timer_count;
}

/**
 * @brief: use CMSIS ISR for TIMER0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine.
 *       The actual c_TIMER0_IRQHandler does the rest of irq handling
 */
__asm void TIMER0_IRQHandler(void)
{
	CPSID I ; disable interrupts
	PRESERVE8
	IMPORT c_TIMER0_IRQHandler
	IMPORT k_release_processor
	PUSH {r4-r11, lr}
	BL c_TIMER0_IRQHandler
	LDR R4, = __cpp(&g_switch_flag)
	LDR R4, [R4]
	MOV R5, #0
	CMP R4, R5
	BEQ RESTORE			; if g_switch_flag == 0, then restore the process that was interrupted
	BL k_release_processor	; otherwise (i.e g_switch_flag == 1, so switch to the another process)
RESTORE
	CPSIE I ; enable interrupts
	POP {r4-r11, pc}
}

/**
 * @brief C TIMER0 IRQ Handler
 */
void c_TIMER0_IRQHandler(void)
{
	PCB* cur_proc;
	
	LPC_TIM0->IR = BIT(0); // acknowledge interrupt
	g_switch_flag = 0;     // Reset the switch flag
	
	g_timer_count++; // Increment the time
	
	cur_proc = gp_current_process;   // Save the actual current process
	gp_current_process = timer_proc; // Set the timer as the current process
	
	timer_i_process(); // Call the timer i-process
	
	gp_current_process = cur_proc;   // Restore the current process
}

/**
 * @brief The timer i-process
 */
void timer_i_process()
{
	MSG_BUF* message;
	
	// Insert the envelopes of any received messages into the list of delayed messages
	while (message = (MSG_BUF*)ki_receive_message(0)) {
		// Get the pointer to the envelope from the message
		MSG_ENVELOPE* envelope = (MSG_ENVELOPE*)k_message_to_envelope(message);
		
		/* Insert the envelope into the delayed messages list based on the send time.
		 * The later the send time, the farther to the back of the list the message will be inserted.
		 */
		if (empty(delayed_messages) || ((MSG_ENVELOPE*)delayed_messages->front)->send_time > envelope->send_time) {
			push_front(delayed_messages, (ListNode*)envelope);
		}
		else {
			MSG_ENVELOPE* iter = (MSG_ENVELOPE*)delayed_messages->front;
			while (iter->next && iter->next->send_time <= envelope->send_time) {
				iter = iter->next;
			}
			// Perform the insertion
			envelope->next = iter->next;
			iter->next = envelope;
		}
	}
	
	// Remove expired messages from the list of delayed messages and send them
	while (!empty(delayed_messages) && ((MSG_ENVELOPE*)delayed_messages->front)->send_time <= g_timer_count) {
		MSG_ENVELOPE* envelope = (MSG_ENVELOPE*)pop_front(delayed_messages);
		k_send_message(envelope->destination_pid, envelope);
	}
}

__asm void TIMER1_IRQHandler(void)
{
	CPSID i ; disable interrupts
	PRESERVE8
	IMPORT c_TIMER1_IRQHandler
	PUSH {r4-r11, lr}
  BL c_TIMER1_IRQHandler
	LDR R4, = __cpp(&g_bench_timer_count)
	LDR R5, [R4]
	ADD R5, #1
	STR R5, [R4]
	CPSIE i ; enable interrupts
	POP {r4-r11, pc}
}

/**
 * @brief C TIMER1 IRQ Handler
 */
void c_TIMER1_IRQHandler(void)
{
	LPC_TIM1->IR = BIT(0); // acknowledge interrupt
}

/**
 * @brief: initialize the n_uart
 * NOTES: It only supports UART0. It can be easily extended to support UART1 IRQ.
 * The step number in the comments matches the item number in Section 14.1 on pg 298
 * of LPC17xx_UM
 */
int uart_irq_init(int n_uart)
{

	LPC_UART_TypeDef* pUart;
	
	if ( n_uart == 0 ) {
		/*
		Steps 1 & 2: system control configuration.
		Under CMSIS, system_LPC17xx.c does these two steps
		
		-----------------------------------------------------
		Step 1: Power control configuration.
		        See table 46 pg63 in LPC17xx_UM
		-----------------------------------------------------
		Enable UART0 power, this is the default setting
		done in system_LPC17xx.c under CMSIS.
		Enclose the code for your refrence
		//LPC_SC->PCONP |= BIT(3);
		
		-----------------------------------------------------
		Step2: Select the clock source.
		       Default PCLK=CCLK/4 , where CCLK = 100MHZ.
		       See tables 40 & 42 on pg56-57 in LPC17xx_UM.
		-----------------------------------------------------
		Check the PLL0 configuration to see how XTAL=12.0MHZ
		gets to CCLK=100MHZin system_LPC17xx.c file.
		PCLK = CCLK/4, default setting after reset.
		Enclose the code for your reference
		//LPC_SC->PCLKSEL0 &= ~(BIT(7)|BIT(6));
		
		-----------------------------------------------------
		Step 5: Pin Ctrl Block configuration for TXD and RXD
		        See Table 79 on pg108 in LPC17xx_UM.
		-----------------------------------------------------
		Note this is done before Steps3-4 for coding purpose.
		*/
		
		/* Pin P0.2 used as TXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 4);
		
		/* Pin P0.3 used as RXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 6);
		
		pUart = (LPC_UART_TypeDef*) LPC_UART0;
		
	}
	else if ( n_uart == 1) {
	
		/* see Table 79 on pg108 in LPC17xx_UM */
		/* Pin P2.0 used as TXD1 (Com1) */
		LPC_PINCON->PINSEL4 |= (2 << 0);
		
		/* Pin P2.1 used as RXD1 (Com1) */
		LPC_PINCON->PINSEL4 |= (2 << 2);
		
		pUart = (LPC_UART_TypeDef*) LPC_UART1;
		
	}
	else {
		return 1; /* not supported yet */
	}
	
	/*
	-----------------------------------------------------
	Step 3: Transmission Configuration.
	        See section 14.4.12.1 pg313-315 in LPC17xx_UM
	        for baud rate calculation.
	-----------------------------------------------------
	    */
	
	/* Step 3a: DLAB=1, 8N1 */
	pUart->LCR = UART_8N1; /* see uart.h file (0x83) */
	
	/* Step 3b: 115200 baud rate @ 25.0 MHZ PCLK */
	pUart->DLM = 0; /* see table 274, pg302 in LPC17xx_UM */
	pUart->DLL = 9;	/* see table 273, pg302 in LPC17xx_UM */
	
	/* FR = 1.507 ~ 1/2, DivAddVal = 1, MulVal = 2
	   FR = 1.507 = 25MHZ/(16*9*115200)
	   see table 285 on pg312 in LPC_17xxUM
	*/
	pUart->FDR = 0x21;
	
	
	
	/*
	-----------------------------------------------------
	Step 4: FIFO setup.
	       see table 278 on pg305 in LPC17xx_UM
	-----------------------------------------------------
	    enable Rx and Tx FIFOs, clear Rx and Tx FIFOs
	Trigger level 0 (1 char per interrupt)
	*/
	
	pUart->FCR = 0x07;
	
	/* Step 5 was done between step 2 and step 4 a few lines above */
	
	/*
	-----------------------------------------------------
	Step 6 Interrupt setting and enabling
	-----------------------------------------------------
	*/
	/* Step 6a:
	   Enable interrupt bit(s) wihtin the specific peripheral register.
	       Interrupt Sources Setting: RBR, THRE or RX Line Stats
	   See Table 50 on pg73 in LPC17xx_UM for all possible UART0 interrupt sources
	   See Table 275 on pg 302 in LPC17xx_UM for IER setting
	*/
	/* disable the Divisior Latch Access Bit DLAB=0 */
	pUart->LCR &= ~(BIT(7));
	
	//pUart->IER = IER_RBR | IER_THRE | IER_RLS;
	pUart->IER = IER_RBR | IER_RLS;
	
	/* Step 6b: enable the UART interrupt from the system level */
	
	if ( n_uart == 0 ) {
		NVIC_EnableIRQ(UART0_IRQn); /* CMSIS function */
	}
	else if ( n_uart == 1 ) {
		NVIC_EnableIRQ(UART1_IRQn); /* CMSIS function */
	}
	else {
		return 1; /* not supported yet */
	}
	pUart->THR = '\0';
	return 0;
}

/**
 * @brief: use CMSIS ISR for UART0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine.
 *       The actual c_UART0_IRQHandler does the rest of irq handling
 */
__asm void UART0_IRQHandler(void)
{
	CPSID I ; disable interrupts
	PRESERVE8
	IMPORT UART_IPROC
	IMPORT k_release_processor
	PUSH {r4-r11, lr}
	BL UART_IPROC
	LDR R4, = __cpp(&g_switch_flag)
	LDR R4, [R4]
	MOV R5, #0
	CMP R4, R5
	BEQ RESTORE2			; if g_switch_flag == 0, then restore the process that was interrupted
	BL k_release_processor	; otherwise (i.e g_switch_flag == 1, then switch to the other process)
RESTORE2
	CPSIE I ; enable interrupts
	POP {r4-r11, pc}
}

// UART initialized in uart_irq.c
void UART_IPROC(void)
{
	MSG_BUF* received_message;
	MSG_BUF* message_to_send;
	LPC_UART_TypeDef* pUart = (LPC_UART_TypeDef*) LPC_UART0;
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
			k_send_message(PID_KCD, message_to_send);
		}
		else {
			uart0_put_char(g_char_in);
			if (g_char_in == '\r') {
				uart0_put_char('\n');
				uart0_put_string("ERROR: System out of memory!\r\n");
			}
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
			uart1_put_string(".");
		}
		uart1_put_string("\r\n");
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
