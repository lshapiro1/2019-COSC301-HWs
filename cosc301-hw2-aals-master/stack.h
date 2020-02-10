#ifndef __STACK_H__
#define __STACK_H__

#include <stdlib.h>
#include <stdbool.h>

struct stack_node {
    int value;
    struct stack_node *next;
};

struct stack {
    struct stack_node *top;
};


void push(struct stack *, int);
int pop(struct stack *);
int peek(struct stack *);
bool empty(struct stack *);
void init(struct stack *);

#endif // __STACK_H__
