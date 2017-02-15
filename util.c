#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


/*
    returns true if the input is all space and false if not.
*/
bool allSpace(char * input) {
    int i=0;
    while (input[i]!='\0') {
        if (input[i]!=' ') {
            return false;
        }
        ++i;
    }
    return true;
}

/*
    It returns the number of non-space string in inp with delimiter.
    If flag is true, the separated strings will be stored into output.
*/
int split_input(char *inp, char **output, char *delimiter, bool flag) {
    int i = 0;
    char *input = strdup(inp);
    char *tmp = strtok(input, delimiter);
    while (tmp) {
        if (!allSpace(tmp)) {
            if (flag) {
                output[i] = strdup(tmp);
            }
            ++i;
        }
        tmp = strtok(nullptr, delimiter);
    }
    free(input);
    return i;
}

/*
    It creates a new string which is a copy of buffer from i to j.
*/
char * copy(char * buffer,ssize_t i, ssize_t j) {
    char * result=(char*)malloc(sizeof(char)*(j-i+1));
    ssize_t c=0;
    while (c!=j-i) {
        result[c]=buffer[i+c];
        ++c;
    }
    result[c]='\0';
    return result;
}

/*
    It reads /proc/inp/stat to get the statistics of process inp.
    And then it create a new PIDNode to store the relevant information
    and returns it.
*/
PIDNode * buildPIDNode(pid_t inp) {
    pid_t pid=0;
    char * name=(char*)malloc(sizeof(char)*MAX_PROC_FILE_PATH);
    unsigned long ut, st;
    pid_t ppid;

    char str[MAX_PROC_FILE_PATH];
    sprintf(str, "/proc/%d/stat",inp);
    FILE *file = fopen(str, "r");
    if (file == NULL) {
        return nullptr;
    }
    int z;
    unsigned long h;
    char stat;
    fscanf(file, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu", &pid, name, &stat, &ppid, &z, &z, &z, &z,
           (unsigned *)&z, &h, &h, &h, &h, &ut, &st);
    fclose(file);
    char * newName=copy(name,1,strlen(name)-1);
    free(name);
    PIDNode * result = (PIDNode*)(malloc(sizeof(PIDNode)));
    result->PPID=ppid;
    result->PID=pid;
    result->name=newName;
    result->child=nullptr;
    result->next=nullptr;
    return result;
}

/*
    release all memory allocated for cmd.
*/
void freeCommand(Command * cmd) {
    int i=0;
    for (;i!=cmd->argc;++i) {
        free(cmd->argv[i]);
    }
    free(cmd);
}

/*
    release all memory allocated for line.
*/
void freeLine(Line * line) {
    Command * iterator = line->head;
    Command * temp=nullptr;
    while (iterator) {
        temp = iterator;
        iterator=iterator->next;
        freeCommand(temp);
    }
    free(line);
}