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

void remove(ForwardList* list, ListNode* node)
{
	ListNode* iterator;
	
	//If the list is empty, there is nothing to remove, so simply return
	if (empty(list)) {
		return;
	}
	
	iterator = list->front;
	//If the first node is the node to be removed
	if (iterator == node) {
		list->front = list->front->next;
	}
	else {
		//Find the node to be removed in the list
		while (iterator->next != NULL && iterator->next != node) {
			iterator = iterator->next;
		}
		//If the node was found (is not null), remove it
		if (iterator->next != NULL) {
			iterator->next = iterator->next->next;
		}
	}
}
