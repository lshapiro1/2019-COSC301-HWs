/**
 * A semi-clone of the TabNanny module in Python.
 *
 * As the Python docs state:
 * Tabnanny - The Tab Nanny despises ambiguous indentation.  She knows no mercy.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "stack.h"

/*
 * cleanup (destroy) the stack.
 * arguments: a pointer to the stack
 * returns: nothing
 *
 * Pop elements off the stack until there's nothing left.
 * As a side-effect, this function should print out
 * each value that is popped off the stack.
 */

void cleanstack(struct stack *stack) {
    printf("Stack at the end: \n");
    while (!empty(stack)){
        printf("%d\n", pop(stack));
    }
}



/*
 * first_nonwhitespace
 * arguments: a C string
 * returns: index in the string that contains the first non-whitespace character
 */
int first_nonwhitespace(char *s) {
    for (int i = 0; i < strlen(s); i++){
        if (!isspace(s[i]) && s[i]!='\n' & s[i]!='\0'){
            return i;
        }
    }
    return -1; 
}

/*
 * endsColon
 * arguments: a C string
 * returns: boolean stating if line ends with colon or not
 */

bool endsColon(char *s){
    for (int i = (strlen(s)-1); i>=0; i--){ //iterate from end of char*
        char toCheck = s[i]; //get char at index
        if (toCheck==':'){ //check if colon
            return true;
        }
        else if (!isspace(toCheck)){ //check if not whitespace
            return false;
        }
    }
    return false;
}


void tabnanny(FILE *inputfile) {
    int line_number = 0;
    struct stack intstack;
    init(&intstack);
    int old_indent = -1;
    char line[1024] = {'\0'};  // create and initialize a buffer to 
                               // hold the next line of the file
    char prevLine [1024] = {'\0'}; //sets char array to hold previous line
    while (fgets(line, 1024, inputfile) != NULL) {
        line_number++;
        // get the indentation level of the first line of the Python file
        int current_indent = first_nonwhitespace(line);    
        if (endsColon(prevLine) && current_indent!= (-1) && old_indent!=-1){       
            if (current_indent > old_indent){ //check for correct indentation after colon
                push(&intstack,current_indent);
            }
            else{
                //printf("1");
                printf("Error in indentation on line %d\n", line_number);
                cleanstack(&intstack);
                return;
            }
        }
        else if(current_indent == old_indent){ //check if remaining the same
            push(&intstack,current_indent);
        }
        else if (old_indent>current_indent && current_indent != -1){ //check for dedent
            //printf("Entered Dedent\n");
            bool keep_going = true;
            while (!empty(&intstack) && keep_going){ //searches for dedent, until found or stack is empty
                //pop until empty or indentation same as previous
                int temp_indent = pop(&intstack);
                if(temp_indent == current_indent){ 
                    keep_going = false;
                    //printf("Matching indent found\n");
                }
            }
            if (empty(&intstack)){
                //printf("2");
                printf("Error in indentation on line %d\n", line_number);
                cleanstack(&intstack);
                return;
            }
            push(&intstack,current_indent); 
        }
        else if (current_indent>old_indent && old_indent!=-1){ // check if indented but not after colon
            //printf("%s",prevLine);
            //printf("Current: %d    Old: %d ", current_indent, old_indent);
            //printf("3");
            printf("Error in indentation on line %d\n", line_number);
            cleanstack(&intstack);
            return;
        }
        if (current_indent!=-1){
            old_indent = current_indent;
            strcpy(prevLine,line);
        }
    }
    printf("\nTab Nanny has been satisfied.\n");
    printf("Total lines read: %d\n",line_number);
    cleanstack(&intstack);
    printf("\n\n\n\n");
}




int main(int argc, char **argv) {
    if (argc != 2) {
        printf("One file name required for indentation checking.\n");
        printf("\tInvoke tabnanny like this: %s file.py\n", argv[0]);
        return 0;
    }

    FILE *input = fopen(argv[1], "r");
    if (input == NULL) {
        printf("File couldn't be opened: %s\n", strerror(errno));
        return 0;
    }
    tabnanny(input);
    fclose(input);
    return 0;
}
