/**
 * @brief timer.h - Timer header file
 * @date 2013/02/12
 */

#ifndef I_PROC_H_
#define I_PROC_H_
 
#include <stdint.h>

extern uint32_t timer_init (uint8_t n_timer);  /* initialize timer n_timer */
extern uint32_t get_current_time(void);
extern uint32_t get_current_bench_time(void);
extern void timer_i_process(void);
extern void UART0_IRQHandler(void);

#endif /* ! I_PROC_H_ */
