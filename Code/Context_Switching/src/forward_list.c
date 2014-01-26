/**
 * @file:   forward_list.c
 * @brief:  Forward List C file
 * @auther: Nathan Woltman
 * @date:   2014/01/25
 */

#ifndef DEBUG_0
#define NDEBUG //Disable assertions
#endif

#include <assert.h>
#include <stddef.h>
#include "forward_list.h"


int empty(ForwardList* list)
{
	assert(list != NULL);
	return list->front == NULL;
}

ListNode* pop_front(ForwardList* list)
{
	ListNode* frontNode;
	assert(list != NULL);
	
	frontNode = list->front;
	//If the front node is not null, set the next node to the front node
	if (frontNode != NULL) {
		list->front = frontNode->next;
	}
	return frontNode;
}

void push_front(ForwardList* list, ListNode* node)
{
	assert(list != NULL);
	node->next = list->front;
	list->front = node;
}
