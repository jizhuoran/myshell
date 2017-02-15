/*
Authors: CHEN, Zhihan UID: 3035142261
         JI, Zhuoran  UID: 3035139915
Development platform: Ubuntu 14.04 x64
Last modified date: 20/10/2016
Compilation: make myshell
*/
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "execute.h"
#include "sig.h"
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
extern sig_atomic_t timeX_flag;



/*
    get_command reads contents from stdin and stores it to buffer.
    If there's no input or the number of arguments is greater than
    the maximum arguments number, it returns false.
*/
bool get_command(char * buffer) {
    memset(buffer, 0,BUFFER_SIZE * sizeof(char));
    char * input = fgets(buffer,BUFFER_SIZE,stdin);
    if (input == nullptr) {
        return false;
    }
    if (split_input(input,nullptr," ",false)>MAX_ARGS_NUMBER) {
        fprintf(stderr,"myshell: Too many arguments\n");
        return false;
    }
    return true;
}

/*
    the entry point of myshell.
    It initializes signal handlers and then enters the while loop 
    reading from buffer, parse the input and if the input is valid,
    execute the line.
*/
int main(int argc, char const *argv[]) {

    SIGINT_handler_wrapper();
    SIGCHLD_handler_wrapper();
    SIGUSR1_handler_wrapper();

    char buffer[BUFFER_SIZE];
    while (true) {
        fprintf(stdout, "## myshell $ ");
        if (get_command(buffer)) {
            char * input = strndup(buffer,strlen(buffer) - 1);
            Line * line = parse(input);
            if (line) {
                timeX_flag=0;
                execute(line);
            }
            free(input);
        }
        cleanup_wrapper();
    }
}
