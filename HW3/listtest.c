#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "list.h"

bool ok = true;

#define XTEST(expr, errmsg) \
  if (!expr) { \
    printf("Failure: %s on line %d of %s", errmsg, __LINE__, __FILE__); \
    ok = false; \
  } \

int main() {
    struct proclist *head = proclist_new();
    XTEST((head->length == 0), "List length not initialized to 0.");
    XTEST((head->head == NULL), "List head not initially NULL.");

    char xname[] = "test";
    proclist_add(head, 13, xname);
    XTEST((head->length == 1), "List length not incremented to 1 after adding a node.");
    XTEST((head->head != NULL), "List head shouldn't be NULL after adding one element.");
    XTEST((head->head->next == NULL), "With one node on the list, its next should point to NULL but doesn't.");
    XTEST((head->head->pid == 13), "pid value on new node should be 13 but isn't");
    XTEST((!strcmp(head->head->cmd, xname)), "cmd name on new node should be \"test\" but isn't");
    XTEST((head->head->cmd != xname), "It doesn't look like the cmd in the node was allocated on the heap.");

    struct procnode *first = proclist_find(head, 13);
    XTEST((first != NULL), "When finding a node that should exist, NULL was returned instead.");
    XTEST((first->pid == 13), "Node found should have pid 13 but doesn't\n");

    char xname2[] = "second";
    proclist_add(head, 42, xname2);
    XTEST((head->length == 2), "List length not incremented to 2 after adding a second node.");
    struct procnode *second = proclist_find(head, 42);
    XTEST((second != NULL), "When finding a node that should exist, NULL was returned instead.");
    XTEST((second->pid == 42), "Node found should have pid 42 but doesn't\n");
    XTEST((!strcmp(second->cmd, xname2)), "Node found should have cmd \"second\"but doesn't\n");

    struct procnode *invalid = proclist_find(head, 7);
    XTEST((invalid == NULL), "proclist_find called for a non-existent pid; should return NULL but didn't");

    proclist_remove(head, 13);
    XTEST((head->length == 1), "List length not decremented to 1 after removing a node.");
    XTEST((proclist_find(head, 13) == NULL), "After removing node with pid 13, trying to find it should result in NULL but doesn't");
    XTEST((proclist_find(head, 42) != NULL), "After removing node with pid 13, trying to find the other node remaining (with pid 42) should not return NULL but does");
    proclist_remove(head, 42);
    XTEST((head->length == 0), "List length should be zero after removing all nodes.");
    XTEST((head->head == NULL), "List head should be NULL after removing all nodes.");

    printf("Tests complete.\n");
    if (!ok) {
      printf("At least one test failed.\n");
    } else {
      printf("All tests passed.\n");
    }
    return 0;
}
