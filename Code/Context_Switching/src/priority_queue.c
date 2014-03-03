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


void init_pq(PriorityQueue* pqueue)
{
	int i;
	assert(pqueue != NULL);
	for (i = 0; i < NUM_PRIORITIES; i++) {
		pqueue->queues[i].first = pqueue->queues[i].last = NULL;
	}
}

int pq_empty(PriorityQueue* pqueue)
{
	int i;
	assert(pqueue != NULL);
	for (i = 0; i < NUM_PRIORITIES; i++) {
		if (pqueue->queues[i].first != NULL) {
			return 0;
		}
	}
	return 1;
}

QNode* top(PriorityQueue* pqueue)
{
	int i;
	Queue* queue;
	
	assert(pqueue != NULL);
	
	//Loop through the queues by priority (0 is highest)
	//and return the first element of the first non-empty queue
	for (i = 0; i < NUM_PRIORITIES; i++) {
		queue = &(pqueue->queues[i]);
		if (!q_empty(queue)) {
			return queue->first;
		}
	}
	//All queues were empty so return a null pointer
	return NULL;
}

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

int remove_at_priority(PriorityQueue* pqueue, QNode* node, int priority)
{
	Queue* queue;
	assert(pqueue != NULL);
	queue = &(pqueue->queues[priority]);
	
	//If the queue is empty the node cannot be found
	if (q_empty(queue)) {
		return 0;
	}
	
	//Remove the node from its current queue
	if (queue->first == node) {
		queue->first = node->next;
		//If the new first node is null, the queue is now empty so set the last node to null as well
		if (queue->first == NULL) {
			queue->last = NULL;
		}
	}
	else {
		QNode* iterator = queue->first;
		while (iterator->next != node) {
			if (iterator->next == NULL) {
				return 0;
			}
			iterator = iterator->next;
		}
		//Getting here means the node was found and it is equal to iterator->next
		iterator->next = node->next; //Replace the found/input node with the next node
		if (queue->last == node) {
			//The iterator node is now the last node in the queue
			queue->last = iterator;
		}
	}
	
	return 1; //Success
}
