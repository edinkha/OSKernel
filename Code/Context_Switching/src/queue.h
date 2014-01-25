#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct queue_node {
	struct queue_node* next;
} QueueNode;

typedef struct queue {
	QueueNode* first;
	QueueNode* last;
} Queue;

bool q_empty(Queue *queue);						// Returns true if the queue is empty; else returns false
void enqueue(Queue *queue, QueueNode* node);	// Adds the input node to the end of the queue
QueueNode* dequeue(Queue *queue);				// Removes and returns a pointer to the node at the front of the queue

#endif
