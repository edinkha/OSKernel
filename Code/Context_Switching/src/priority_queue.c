/**
 * @file:   priority_queue.c
 * @brief:  Priority Queue C file
 * @author: Nathan Woltman
 * @date:   2014/01/26
 */

#ifndef DEBUG_0
#define NDEBUG //Disable assertions
#endif

#include <assert.h>
#include <stddef.h>
#include "priority_queue.h"


QNode* pop(PriorityQueue* pqueue)
{
	int i;
	Queue queue;
	
	assert(pqueue != NULL);
	
	//Loop through the queues by priority (0 is highest)
	//and dequeue the first non-empty queue
	for (i = 0; i < NUM_PRIORITIES; i++) {
		queue = pqueue->queues[i];
		if (!q_empty(&queue)) {
			return dequeue(&queue);
		}
	}
	//All queues were empty so return a null pointer
	return NULL;
}

void push(PriorityQueue* pqueue, QNode* node, int priority)
{
	assert(pqueue != NULL && priority < NUM_PRIORITIES);
	
	//Simply add the node to the queue with the specified priority
	enqueue(&(pqueue->queues[priority]), node);
}
