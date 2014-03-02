/**
 * @file:   priority_queue.h
 * @brief:  Priority Queue header file
 * @author: Nathan Woltman
 * @author: Justin Gagne
 * @date:   2014/01/26
 *
 * NOTE:
 * This is a very specific implementaion of a priority queue. It is used for placing process
 * control blocks (PCBs) into a priority queue (either a ready queue or a blocked queue).
 * There are only 4 priorities, so the priority queue will always have exactly 4 queues.
 * This static number of queues will also help with defining a size for a chunk of memory in
 * which the priority queue will be stored.
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include "queue.h"

#define NUM_PRIORITIES 4

typedef struct priority_queue {
	Queue queues[NUM_PRIORITIES];
} PriorityQueue;


/**
 * @brief: Initializes the given PriorityQueue
 */
void init_pq(PriorityQueue* pqueue);

/**
 * @brief: Determines whether or not the priority queue is empty
 * @return: 1 if the priority queue is empty, 0 otherwise
 */
int pq_empty(PriorityQueue* pqueue);

/**
 * @brief: Gets the highest priority node in the queue without removing it from the queue
 * @return: QNode pointer of the top node
						NULL if the queue is empty
 */
QNode* top(PriorityQueue* pqueue);

/**
 * @brief: Gets the highest priority node in the queue and removes the node from the queue
 * @return: QNode pointer of the popped node
 *          NULL if the queue is empty
 */
QNode* pop(PriorityQueue* pqueue);

/**
 * @brief: Adds the input node to the end of the queue with the given priority
 */
void push(PriorityQueue* pqueue, QNode* node, int priority);

/**
 * @brief: Removes a specific node from the queue with the given priority
 * @return: 1 if the node was found and removed; else returns 0
 * PRE: The given node has the given priority
 */
int remove_at_priority(PriorityQueue* pqueue, QNode* node, int priority);

#endif
