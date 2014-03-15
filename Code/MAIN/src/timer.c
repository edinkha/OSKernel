/**
 * @brief timer.c - Timer example code. Timer IRQ is invoked every 1ms
 * @author T. Reidemeister
 * @author Y. Huang
 * @author NXP Semiconductors
 * @date 2012/02/12
 */

#include <LPC17xx.h>
#include "k_rtx.h"
#include "timer.h"

#define BIT(X) (1<<X)

extern PCB* gp_current_process;

volatile uint32_t g_timer_count = 0; // increment every 1 ms


/**
 * @brief: initialize timer. Only timer 0 is supported
 */
uint32_t timer_init(uint8_t n_timer) 
{
	LPC_TIM_TypeDef *pTimer;
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
		pTimer = (LPC_TIM_TypeDef *) LPC_TIM0;

	} else { /* other timer not supported yet */
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
	pTimer->MCR = BIT(0) | BIT(1);

	g_timer_count = 0;

	/* Step 4.4: CSMSIS enable timer0 IRQ */
	NVIC_EnableIRQ(TIMER0_IRQn);

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
 * @brief: use CMSIS ISR for TIMER0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine. 
 *       The actual c_TIMER0_IRQHandler does the rest of irq handling
 */
__asm void TIMER0_IRQHandler(void)
{
	PRESERVE8
	IMPORT c_TIMER0_IRQHandler
	PUSH{r4-r11, lr}
	BL c_TIMER0_IRQHandler
	POP{r4-r11, pc}
}

/**
 * @brief: C TIMER0 IRQ Handler
 */
void c_TIMER0_IRQHandler(void)
{
	int cur_pid;

	/* acknowledge interrupt */
	LPC_TIM0->IR = BIT(0);

	__disable_irq(); // Prevent any other interrupts while we handle this one

	g_timer_count++; // Increment the time

	/* Do a mock process switch by giving the current process variable some of the i-proc's data
	 * (the only reason this has to be done is because the messaging functions use data from gp_curent_process)
	 * NOTE: This looks hacky, and it is; but it is not worse than saving the gp_current_proc pointer, getting the
	 * timer i-proc's pointer and setting that to gp_current_proc because that would take way more time and not
	 * be any different than this (since the only current process data we care about is the current process's pid
	 * and whether or not it is an i-process).
	 */
	cur_pid = gp_current_process->m_pid; // Save the current process's PID
	gp_current_process->m_pid = PID_TIMER_IPROC;
	gp_current_process->m_is_iproc = 1;

	timer_i_process(); // Call the timer i-process

	// Restore the current process's data
	gp_current_process->m_pid = cur_pid;
	gp_current_process->m_is_iproc = 0;

	__enable_irq();    // Re-enable interrupts
}

/**
 * @brief The timer i-process
 */
void timer_i_process()
{
	// If a second has passed, send a message to the wall clock
	if (g_timer_count % 1000 == 0) {
		MSG_BUF* message = (MSG_BUF*)request_memory_block();
		message->mtype = DEFAULT;
		message->mtext[0] = '\0';
		send_message(PID_CLOCK, message);
	}

	// Remove expired messages from the list of delayed messages and send them
	while (((MSG_ENVELOPE*)delayed_messages->front)->send_time <= g_timer_count) {
		MSG_ENVELOPE* envelope = (MSG_ENVELOPE*)pop_front(delayed_messages);
		send_message(envelope->destination_pid, (void*)((U8*)envelope + SZ_MEM_BLOCK_HEADER));
	}
}
