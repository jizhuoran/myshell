#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/*
    it examines the line from argument begin to argument end
    or when it meets |.
    If there's a non-space string between begin and end and 
    the string does not contains &, there's a command between
    begin and end and returns the length of that command. Else
    it means that there's an illegal command between begin and end
    and thus returns -1.
*/
int hasCmd(const char * line, const int begin, const int end) {
    int i=begin;
    bool hascmd=false;
    while (i<end &&line[i] != '|') {
        if (line[i]!=' '&&line[i]!='&') {
            hascmd=true;
        }
        ++i;
    }
    if (hascmd) {
        return i;
    } else {
        return -1;
    }
}


/*
    It checks whether the use of pipe and background is correct.
    It returns -1 if it is incorrect and BACKGROUND_MODE if it is in
    background or FOREGROUND_MODE it if is in foreground.
*/
int syntaxCheck(char * line) {
    ssize_t size = strlen(line);
    if(split_input(line, nullptr," ",false)==0) {
        return -1;
    }
    char * background = strchr(line,'&');

    // & check
    if (background) {
        if (background==line) {
            fprintf(stderr,"myshell: syntax error near unexpected token '&'\n");
            return -1;
        }
        int before=(int)(background-line)-1;
        bool emptyBefore=true;
        while (before>=0) {
            if (line[before]!=' ') {
                emptyBefore=false;
                break;
            }
            --before;
        }
        if (emptyBefore) {
            fprintf(stderr,"myshell: syntax error near unexpected token '&'\n");
            return -1;
        }
        int after = (int)(background-line)+1;
        while (after<size) {
            if (line[after]!=' ') {
                fprintf(stderr,"myshell: '&' should not appear in the middle of the command line\n");
                return -1;
            }
            ++after;
        }
    }

    // | check
    char * pipe = strchr(line, '|');
    if (pipe!=nullptr) {
        int itr = hasCmd(line,0,(int)(pipe-line));
        if (itr==-1||line[size-1]=='|') {
            fprintf(stderr,"myshell: Incomplete '|' sequence\n");
            return -1;
        }
        while (itr!=size)  {
            itr = hasCmd(line,itr+1,(int)size);
            if (itr==-1) {
                fprintf(stderr,"myshell: Incomplete '|' sequence\n");
                return -1;
            }
        }
    }
    if (background!=nullptr) {
        return BACKGROUND_MODE;
    } else {
        return FOREGROUND_MODE;
    }
}

/*
    It checks whether the use of exit and viewtree is correct.
    They cannot have arguments or pipe or be run in background.
    If it is incorrect, print corresponding error message to stderr
    and returns nullptr. Else returns line directly.
*/
Line * process(Line * line, char * message) {
    if (line->head->argc!=1||line->head->next!=nullptr||line->background) {
        fprintf(stderr,"myshell: \"%s\" with other arguments!!!\n",message);
        freeLine(line);
        return nullptr;
    } else {
        return line;
    }
}

/*
    It checks the use of built-in command and set the
    corresponding type. If there' illegal usage, returns
    nullptr else returns the line with line->type set.
*/
Line * processBuiltin(Line * line) {
    Command * first = line->head;
    if (strcmp(first->argv[0],"exit\0")==0) { // exit built-in
        line->type=EXIT_TYPE;
        return process(line,"exit\0");
    } else if (strcmp(first->argv[0],"viewtree\0")==0) { //viewtree built-in
        line->type=VIEWTREE_TYPE;
        return process(line,"viewtree\0");
    }
    Command * iterator=line->head;
    if (strcmp(iterator->argv[0],"timeX\0")==0) { //timeX built-in
        if (line->background) {
            fprintf(stderr,"myshell: \"timeX\" cannot be run in background mode\n");
            freeLine(line);
            return nullptr;
        }
        if (iterator->argc==1) {
            fprintf(stderr,"myshell: \"timeX\" cannot be a standalone command\n");
            freeLine(line);
            return nullptr;
        }
        --iterator->argc;
        int i=0;
        free(iterator->argv[0]);
        for (i=0;i!=iterator->argc;++i) {
            iterator->argv[i]=iterator->argv[i+1];
        }
        iterator->argv[i]=nullptr;
        line->type=TIMEX_TYPE;
    } else { // no built-in function.
        line->type=NORMAL_TYPE;
    }
    return line;
}


/*
    It parses a command from input, splits it input argv and 
    set the argc using space as its delimiter. 
    It removes & if encounters and return the parsed command;
*/
Command * parseCommand(char * input) {
    Command * result = (Command *)malloc(sizeof(Command));
    result->argc=split_input(input,result->argv," ",true);
    result->argv[result->argc]=nullptr;
    if (strcmp(result->argv[result->argc-1],"&\0")==0) {
        free(result->argv[--result->argc]);
        result->argv[result->argc]=nullptr;
    } else {
        char * last = result->argv[result->argc-1];
        size_t size = strlen(last);
        if (last[size-1]=='&') {
            last[size-1]='\0';
        }
    }
    result->next=nullptr;
    return result;
}


/*
    It parses a line into a parsed Line structure.
    Firstly it does syntax check to check the usage of
    pipe and background and set the flag correspondingly.
     secondly it splits the line into
    5 or fewer raw commands and uses these raw commands to create
    a Command linked list. Finally it processes the built-in function
    and returns the result.
*/
Line * parse(char * line) {
    int i=0;
    int syntaxResult = syntaxCheck(line);
    if (syntaxResult==-1) {
        return nullptr;
    }
    Line * result = (Line*)malloc(sizeof(Line));
    result->type=0;
    result->background=syntaxResult;
    char * rawCmd[5];
    int cmdNumber = split_input(line,rawCmd,"|",true);
    Command *iterator = nullptr;
    while (i<cmdNumber) {
        if (iterator==nullptr) {
            result->head=parseCommand(rawCmd[i]);
            iterator=result->head;
        } else {
            iterator->next=parseCommand(rawCmd[i]);
            iterator=iterator->next;
        }
        free(rawCmd[i]);
        rawCmd[i]=nullptr;
        ++i;
    }
    result = processBuiltin(result);
    return result;
}