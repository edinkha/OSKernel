#ifndef DEBUG_0
//Disable assertions
#define NDEBUG
#endif

#include <assert.h>
#include <stdlib.h>
#include "queue.h"


bool q_empty(Queue* queue)
{
	assert(queue != NULL);

	return queue->first == NULL;
}

void enqueue(Queue* queue, QueueNode* node)
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

QueueNode* dequeue(Queue* queue)
{
	QueueNode* firstNode;
	assert(queue != NULL);

	firstNode = queue->first;
	//If the front node is not null, set the next node to the front node
	if (firstNode != NULL) {
		queue->first = firstNode->next;
	}
	return firstNode;
}
