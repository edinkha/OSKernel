/**
 * @file:   queue.h
 * @brief:  Queue header file
 * @author: Nathan Woltman
 * @date:   2014/01/25
 */

#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue_node {
	struct queue_node* next;
} QNode;

typedef struct queue {
	QNode* first;
	QNode* last;
} Queue;

int q_empty(Queue* queue);					// Returns 1 if the queue is empty; else returns 0
void enqueue(Queue* queue, QNode* node);	// Adds the input node to the end of the queue
QNode* dequeue(Queue* queue);				// Removes and returns a pointer to the node at the front of the queue

#endif
