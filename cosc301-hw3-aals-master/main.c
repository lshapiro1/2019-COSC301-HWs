#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "list.h"


/*
 * The tokenify() function from lab 2, with one modification,
 * for you to use.
 *
 * The modification is that it also takes a second parameter,
 * which is the set of characters on which to "split" the input.
 * In lab 2, this was always " \t\n", but you can call tokenify
 * to split on other characters, too, with the implementation below.
 * Hint: you can split on ";" in order to separate the commands on 
 * a single command line.
 */
char** tokenify(const char *s, const char *sep) {
    //printf("entered tokenify");
    char *word = NULL;

    // make a copy of the input string, because strtok
    // likes to mangle strings.  
    char *s1 = strdup(s);

    // find out exactly how many tokens we have
    int words = 0;
    for (word = strtok(s1, sep);
         word;
         word = strtok(NULL, sep)) words++ ;
    free(s1);

    s1 = strdup(s);

    // allocate the array of char *'s, with one additional
    char **array = (char **)malloc(sizeof(char *)*(words+1));
    int i = 0;
    for (word = strtok(s1, sep);
         word;
         word = strtok(NULL, sep)) {
        array[i] = strdup(word);
        i++;
    }
    array[i] = NULL;
    free(s1);
    return array;
}

void free_tokens(char **tokens) {
    //printf("entered free_tokens");
    int i = 0;
    while (tokens[i] != NULL) {
        free(tokens[i]); // free each string
        i++;
    }
    free(tokens); // then free the array
}

void test_print_tokenify(char **toCheck){
    int i = 0;
    while (toCheck[i] != NULL) {
        printf("String%d:  %s\n",i, toCheck[i]); // free each string
        fflush(stdout);
        i++;
    }
}



int is_builtIn(char *toCheck, struct proclist *plist){ //checks if a built in function we need to deal with 
    if (strcmp(toCheck,"exit")==0){ //if user types exit
        printf("Exiting process\n");
        exit(0);
        return 1;
    }
    else if(strcmp(toCheck,"status")==0){ //prints out time and proclist
        //need to print usage --> see handout for how to do it
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        printf("%ld.%06d\n", usage.ru_stime.tv_sec, (int)usage.ru_stime.tv_usec);
        printf("%ld.%06d\n", usage.ru_utime.tv_sec, (int)usage.ru_utime.tv_usec);
        proclist_print(plist);
        return 2;
    }
    else if (strcmp(toCheck,"print")==0){ //complete
        //not built in function but helps debug
        proclist_print(plist);
        return 3;
    }
    return -1;
}

void execute_commands(struct proclist* plist, char **toExecute){
    bool background = false;
    int i = 0;
    while (toExecute[i]!=NULL){
        char ** broken_up = tokenify(toExecute[i], " ");  
        int z = strcmp(broken_up[0],"exit");          
        if ( z == 0){      // checking before fork occurs to exit parent
            if(is_builtIn(broken_up[0], plist)==1){
                if (plist->length==1 &&  z == 1){
                    exit(EXIT_SUCCESS);
                } else if (z==1){
                    printf("Other Processes Still Running, Cannot Exit!");
                }
            }
        }
        pid_t pid = fork();
        char* ampersand_location = strchr(toExecute[i],'&');
        if (ampersand_location != NULL){
            ampersand_location[0] = ' ';
            background = true;   
            //printf("%s\n",toExecute[i]);
        } else{
            background = false;
        }
        if (pid == -1 ){ //means error
            perror("Error in Child Creation!\n");
        } else if (pid ==0){ //means within child
            //printf("Entered CHILD\n\n");
            pid_t child_pid = getpid();
            proclist_add(plist,child_pid,toExecute[i]);
            char ** command_broken_up = tokenify(toExecute[i], " ");
            int k = is_builtIn(command_broken_up[0],plist);
            if (k==-1){
                //printf("Executing Command: %s\n",command_broken_up[0]);
                int j = execv(command_broken_up[0],command_broken_up);
                if (j==-1){
                    printf("%s : is not a valid command!\n",command_broken_up[0]);
                    proclist_remove(plist,pid);
                }
            }
            
            free_tokens(command_broken_up);
        } else if (background == false && pid>0) { //for parent when not a background command
            int* j = NULL;
            waitpid(pid,j,0);
            proclist_remove(plist,pid);
        }
        
        i++;
    }
    free_tokens(toExecute);
}

int main(int argc, char **argv) {
    struct proclist *plist = proclist_new(); // you'll need this eventually
    printf("--->");
    fflush(stdout);
    // main loop for the shell
    while(true) {
        char buffer[1024];
        char newCommand[1024];
        // get command input from user, could contain mutiple commands
        while (fgets(buffer, 1024, stdin) != NULL){
            //checks if user has hit enter, then breaks loop
            if (buffer[strlen(buffer)-1]=='\n'){
                buffer[strlen(buffer)-1] ='\0';
                char *whereComment = strchr(buffer, '#');
                if (whereComment!= NULL){
                    whereComment[0] = '\0';
                }
                strcpy(newCommand,buffer);
                //printf("Exiting FGETS w/ command: %s\n",newCommand);
                break;
            }
        }
        // must use tokenify() to break up string into commands for processing
        char **commands = tokenify(newCommand,";");
        //test_print_tokenify(commands);

        //logic to execute each command
        execute_commands(plist,commands);
        
        printf("--->");
        fflush(stdout);
        
    }

    proclist_free(plist); // clean up the process list
    return EXIT_SUCCESS;
}
