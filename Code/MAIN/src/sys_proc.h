/**
 * @brief 
 * @author 
 * @date 
 */
#ifndef _SYS_PROC_H_
#define _SYS_PROC_H_

#include "k_rtx.h"

typedef struct delayedMessage {
	struct delayedMessage *next;
	MSG_ENVELOPE* envelope;
	int send_time;
	int test_num;		//== test purpose
} D_MSG;

void iTimer(void);

#endif /* ! _SYS_PROC_H_ */
