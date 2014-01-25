#ifndef FORWARD_LIST_H
#define FORWARD_LIST_H

#include <stdbool.h>

typedef struct list_node {
	struct list_node* next;
} ListNode;

typedef struct forward_list {
	ListNode* front;
} ForwardList;

bool empty(ForwardList* list);						// Returns true if the list is empty; else returns false
ListNode* pop_front(ForwardList* list);				// Removes and returns a pointer to the first node in the list
void push_front(ForwardList* list, ListNode* node);	// Adds the input node to the front of the list
void remove(ForwardList* list, ListNode* node);		// Removes the input node from the list

#endif
