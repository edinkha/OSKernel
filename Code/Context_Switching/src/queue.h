#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

typedef struct queue_node {
	struct queue_node* next;
} QNode;

typedef struct queue {
	QNode* first;
	QNode* last;
} Queue;

bool q_empty(Queue *queue);					// Returns true if the queue is empty; else returns false
void enqueue(Queue *queue, QNode* node);	// Adds the input node to the end of the queue
QNode* dequeue(Queue *queue);				// Removes and returns a pointer to the node at the front of the queue

#endif
