/**
 * @file:   queue.c
 * @brief:  Queue C file
 * @author: Nathan Woltman
 * @date:   2014/01/25
 */

#ifndef DEBUG_0
#define NDEBUG //Disable assertions
#endif

#include <assert.h>
#include <stddef.h>
#include "queue.h"


int q_empty(Queue* queue)
{
	assert(queue != NULL);
	return queue->first == NULL;
}

void enqueue(Queue* queue, QNode* node)
{
	assert(queue != NULL);
	
	if (q_empty(queue)) {
		queue->first = node;
	}
	else {
		queue->last->next = node;
	}
	queue->last = node;
}

QNode* dequeue(Queue* queue)
{
	QNode* firstNode;
	assert(queue != NULL);
	
	firstNode = queue->first;
	//If the front node is not null, set the next node to the front node
	// NOTE: This currently doesn't work with implementation of priority queue...
	// "queue->first = " changes pointer of local variable, but not the queue in the priority queue
	if (firstNode != NULL) {
		queue->first = firstNode->next;
	}
	return firstNode;
}
