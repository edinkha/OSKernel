/**
 * @brief timer.h - Timer header file
 * @date 2013/02/12
 */

#ifndef _TIMER_H_
#define _TIMER_H_

extern uint32_t timer_init (uint8_t n_timer);  /* initialize timer n_timer */
extern uint32_t get_current_time(void);
extern void timer_i_process(void);

#endif /* ! _TIMER_H_ */
