#include "stack.h"
#include "stdio.h"
/*
 * push a value onto the stack.
 * arguments: pointer to the stack, value to push
 * returns: nothing
 */
void push(struct stack *stack, int value) {
    struct stack_node *new = malloc(sizeof(struct stack_node)); // create a new node in memory
    new->value = value;         //assign new value to new node
    new->next = stack->top;      // have new node point to old head
    stack->top = new;           //assign head to new node
}


/*
 * pop a value from the stack.
 * arguments: pointer to the stack
 * returns: int value that got popped
 *
 * Note: if there are no values on the stack, it is the fault of the *user*
 * of the stack, not the stack itself.  That is, in this function you should
 * just assume that there is a value to be popped.  If there isn't, the
 * program will crash (which is expected behavior).
 */
int pop(struct stack *stack) {
    int toReturn;               //initalize return value
    toReturn = stack->top->value;       //get value
    struct stack_node *toFree = stack -> top; //get node to free later
    stack -> top = stack -> top -> next; // changing the head to point to the value next, aka the one behind it
    free(toFree);
    return toReturn;
}


/*
 * peek at the top of the stack.
 * arguments: pointer to the stack
 * returns: the int value at the top of the stack
 */
int peek(struct stack *stack) {
    //struct stack_node *p = malloc(sizeof(struct stack));
    if (stack->top != NULL){                   // checking to see if the stack is empty, if not, returns the head. Does not change anything.
        return stack->top->value; //returns value if one
    }
    return -1; // null otherwise

}


/*
 * is the stack empty?
 * arguments: pointer to the stack
 * returns: boolean indicating whether there is at least one value on the stack
 */
bool empty(struct stack *stack) {
    if (stack->top == NULL){
        return true;
    }
    return false;
}


/*
 * initialize the stack
 * arguments: pointer to the stack
 * returns: nothing
 */
void init(struct stack *stack) {
    stack->top = NULL;
}
