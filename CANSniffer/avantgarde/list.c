#include "list.h"

void init_list(struct list *_list, uint32_t _ID)
{
	// Initialize the list
	_list->root = NULL;
	
	// Set the ID for the list
	_list->ArbID = _ID;
}

void push(struct list *_list, uint8_t data[8])
{
	// Check to see if the root node is NULL before doing
	// a recursive push.
	if(_list->root == NULL)
	{
		// Copy the data to the root
		memcpy(_list->root->data, data, sizeof(uint8_t [8]));
		return;
	}
	// Start the recursive push
	push_r(_list->root, data);
}

void push_r(struct node *_node, uint8_t data[8])
{
	// Check to see if the next node is NULL
	// if it is, then we need to add the new node
	// to the next node.
	if(_node->next == NULL)
	{
		// allocate space for a new node
		struct node *new_node = (struct node*)malloc(sizeof(struct node));
		
		// copy the contiants from the input data to the
		// node's data array.
		memcpy(new_node->data, data, sizeof(uint8_t[8]));
		
		// set the next pointer to NULL signifying the end
		// of the list
		new_node->next = NULL;
		
		// Set the previous node to point to the new node
		_node->next = new_node;
		
		// Return through the recursion
		return;
	}
	// Step to the next node
	push_r(_node->next, data);
}

void pop(struct list *_list, uint8_t data[8])
{
	// Check to see if the root node is NULL before doing
	// a recursive pop.
	if(_list->root == NULL)
	{
		return;
	}
	// check to see if the root node is the only node in the list
	else if(_list->root->next == NULL)
	{
		// Copy the data to the data array
		memcpy(data, _list->root->data, sizeof(uint8_t [8]));
		
		// Set root to NULL to signify the list is empty
		_list->root = NULL;
		
		// return to 
		return;
	}
	// Start the recursive pop
	pop_r(_list->root, data);
}

void pop_r(struct node *_node, uint8_t data[8])
{
	// Check to see if the 
	if(_node->next->next == NULL)
	{
		// Copy the data to the data array
		memcpy(data, _node->next->data, sizeof(uint8_t[8]));
		
		// Free the memory
		free(_node->next);
		
		// Set the new end node as the current node
		_node->next = NULL;
		return;
	}
	// Step to the next node
	pop_r(_node->next, data);
}

