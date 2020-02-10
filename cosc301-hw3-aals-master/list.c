#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "list.h"

/*
 * Return a pointer to a newly-heap-allocated struct proclist.
 * Don't forget to initialize the contents of the struct!
 */
struct proclist* proclist_new() {
       struct proclist *new = malloc(sizeof(struct proclist));
       new->head = NULL;
       new->length = 0;
       return new;
}

/*
 * Completely free the memory consumed by a struct proclist,
 * including each node (and each cmd in each node, which should
 * also be heap-allocated), and the struct proclist itself.
 */
void proclist_free(struct proclist *head) {
       struct procnode *upNext = head->head;
       for (int i =0; i < head->length; i++){
              struct procnode *toDelete = upNext;
              upNext = upNext->next;
              free(toDelete->cmd);
              free(toDelete);
       }
       free(head);
}

/*
 * Create a new heap-allocated struct procnode, initialize it
 * with the pid and cmd passed as parameters, and add it to the
 * struct proclist.  
 * 
 * The cmd string *must* be copied to a newly heap-allocated
 * location.  (Hint: use strdup.)
 */
void proclist_add(struct proclist* head, pid_t pid, char *cmd) {
       struct procnode *new = malloc(sizeof(struct procnode));
       new->next = head->head;
       new->cmd = strdup(cmd);
       new->pid = pid;
       head->length = head->length+1;
       head->head = new;
}

/*
 * Search the struct proclist for a struct procnode with a particular
 * process id (pid).  Return NULL if the pid isn't anywhere on the proclist,
 * or a pointer to the struct procnode for that pid.
 */
struct procnode* proclist_find(struct proclist* head, pid_t pid) {
       struct procnode *toSearch = head->head;
       while (toSearch!=NULL){
              if (toSearch->pid == pid){
                     return toSearch;
              }
              toSearch = toSearch->next;
       }
       return NULL;
}

/*
 * Remove the procnode within the proclist that has a particular
 * pid.  If a procnode with the pid doesn't exist on the proclist, do
 * nothing.
 * When removing, don't forget to deallocate any heap memory used
 * by the struct procnode being removed (including the cmd, which should
 * have been heap-allocated).
 */
void proclist_remove(struct proclist *head, pid_t pid) {
       struct procnode *toSearch = head->head;
       struct procnode *previous = NULL;
       while (toSearch!=NULL){
              if (toSearch->pid == pid){
                     if (previous == NULL){
                            head->head = toSearch->next;
                            free(toSearch);
                            head->length = head->length - 1;
                            return;
                     }
                     else{
                            previous->next= toSearch->next;
                            free(toSearch);
                            head->length = head->length - 1;
                            return;
                     }
              }
              previous = toSearch;
              toSearch = toSearch->next;
       }
}


/*
 * Print some representation of the (active) processes on the proclist.
 * For example, if no processes are running, you can print something like:
 *
 Processes currently active: none
 *
 * If there are any processes active, you output might look like:
 Processes currently active:
        [30]: /bin/sleep
        [29]: /bin/sleep
        [28]: /bin/sleep
 */
void proclist_print(struct proclist *head) {
       int active = 0;
       printf("Processes currently active: \n");   
       struct procnode *toCheck = head->head;
       while (toCheck != NULL){
              printf("[%d] : %s\n",toCheck->pid ,toCheck->cmd);
              toCheck = toCheck->next;
              active++;
       }
       if (active == 0){
              printf("none\n");
       }
}