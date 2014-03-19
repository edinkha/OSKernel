/**
 * @file sys_proc.h
 * @brief System Processes
 */
 
#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_


extern void UART0_IRQHandler(void);
extern void CRT(void);
extern void KCD(void);
extern void proc_wall_clock(void);
extern void set_priority_command_proc(void);

#endif /* ! _SYS_PROC_H_ */
