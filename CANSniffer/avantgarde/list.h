#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// struct list nameOfList;
// init_list(nameOfList, ID);

struct node
{
	uint8_t data[8];
	struct node* next;
};

struct list
{
	uint32_t ArbID;
	struct node* root;
};

void init_list(struct list *_list, uint32_t _ID);

void push(struct list *_list, uint8_t *data);

void push_r(struct node *_node, uint8_t data[8]);

void pop(struct list *_list, uint8_t data[8]);

void pop_r(struct node *_node, uint8_t data[8]);
