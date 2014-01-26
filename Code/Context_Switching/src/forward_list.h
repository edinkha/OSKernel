/**
 * @file:   forward_list.h
 * @brief:  Forward List header file
 * @author: Nathan Woltman
 * @date:   2014/01/25
 */

#ifndef FORWARD_LIST_H
#define FORWARD_LIST_H

typedef struct list_node {
	struct list_node* next;
} ListNode;

typedef struct forward_list {
	ListNode* front;
} ForwardList;

int empty(ForwardList* list);						// Returns 1 if the list is empty; else returns 0
ListNode* pop_front(ForwardList* list);				// Removes and returns a pointer to the first node in the list
void push_front(ForwardList* list, ListNode* node);	// Adds the input node to the front of the list

#endif
