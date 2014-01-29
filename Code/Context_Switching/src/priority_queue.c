/**
 * @file:   priority_queue.c
 * @brief:  Priority Queue C file
 * @author: Nathan Woltman
 * @author: Justin Gagne
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
	Queue* queue;
	
	assert(pqueue != NULL);
	
	//Loop through the queues by priority (0 is highest)
	//and dequeue the first non-empty queue
	for (i = 0; i < NUM_PRIORITIES; i++) {
		queue = &(pqueue->queues[i]);
		if (!q_empty(queue)) {
			return dequeue(queue);
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

QNode* remove(PriorityQueue* pqueue, int old_priority, void* pcb_address)
{
	QNode* node;

	assert(pqueue != NULL);
	
	// Assign the first node in the old priority queue's to node
	node = (QNode*)pqueue->queues[old_priority].first;
	
	// loop through the queue until we reach the end or find the matching PCB
	while(node != NULL && (void*)node != pcb_address) {
		node = node->next;
	}
	
	return node;
}
