#include "util.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



/*
    If the last element of buffer is not \n, i.e., 
    reading is incomplete, getLastOne returns the 
    last incomplete part of PID.
*/
char * getLastOne(char * buffer) {
    ssize_t i=BUFFER_SIZE-1;
    while (buffer[i]!='\n') {
        --i;
    }
    ssize_t size=BUFFER_SIZE-i-1;
    char * lastOne=(char*)malloc(sizeof(char)*(size+1));
    ssize_t j=0;
    while (j!=size) {
        lastOne[j]=buffer[i+j+1];
        ++j;
    }
    lastOne[j]='\0';
    return lastOne;
}



/*
    If there's a separated PID due to the BUFFER_SIZE, combine uses 
    the lastOne part of last buffer and current buffer to create a 
    new complete PID string.
*/
char* combine(char * buffer,char *lastOne) {
    ssize_t i=0;
    while (buffer[i]!='\n') {
        ++i;
    }
    ssize_t lastSize=strlen(lastOne);
    ssize_t size=lastSize+i;
    char * result=(char*)malloc(sizeof(char)*(size+1));
    ssize_t j=0;
    while (j!=lastSize) {
        result[j]=lastOne[j];
        ++j;
    }
    j=0;
    while (j!=i) {
        result[lastSize+j]=buffer[j];
        buffer[j]='\n'; 
        ++j;
    }
    result[lastSize+j]='\0';
    free(lastOne);
    return result;
}


/*
    build a linked list from buffer with length size while updating iterator
    and returns the head Node. It goes through the buffer and uses \n as its
    delimiter. for each pattern, call buildPIDNode to create PIDNode.
*/
PIDNode * buildFromBuffer(char * buffer, PIDNode** iterator,ssize_t size) {
    PIDNode * head=nullptr;
    if (size==BUFFER_SIZE&&buffer[size]!='\n') {
        while(buffer[size]!='\n') {
            --size;
        }
    }
    ssize_t i=0;
    ssize_t j=0;
    while (j!=size) {
        i=j;
        while(buffer[j]!='\n') {
            ++j;
        }
        char * pid=copy(buffer,i,j);
        if ((*iterator)==nullptr) {
            (*iterator)=buildPIDNode(atoi(pid));
            head=*iterator;
        } else {
            (*iterator)->next=buildPIDNode(atoi(pid));
            if ((*iterator)->next!=nullptr) {
                (*iterator)=(*iterator)->next;
            }
        }
        ++j;
    }
    return head;
}


/*
    getChildren returns a list of PIDNodes, which are
    the children of process specified by pid. It forks 
    a child process to execute pgrep -P pid and uses pipe
    to get the result. The parent process reads all input
    from child process and generates a list of PIDNodes using
    buildFromBuffer;
*/

PIDNode * getChildren(pid_t pid) {
    int pfd[2];
    pipe(pfd);
    pid_t pgrepPID=0;
    if ((pgrepPID=fork())==0) {
        close(pfd[0]);
        dup2(pfd[1],1);
        char * PID=(char *)malloc(sizeof(char)*MAX_PROC_FILE_PATH);
        sprintf(PID,"%d",pid);
        char *command[4]={(char*)"pgrep",(char*)"-P",PID,nullptr};
        execvp(command[0],command);
    } else if (pgrepPID>0){
        close(pfd[1]);
        char *lastOne=nullptr;
        char buffer[BUFFER_SIZE];
        char * combined=nullptr;
        ssize_t readSize=0;
        PIDNode * pidList=nullptr;
        PIDNode * iterator=nullptr;
        do {
            memset(buffer,0,BUFFER_SIZE*sizeof(char));
            readSize=read(pfd[0],buffer,BUFFER_SIZE);
            if (readSize!=0) {
                if (lastOne!=nullptr) {
                    combined=buffer[0]=='\n'?lastOne:combine(buffer,lastOne);
                    lastOne=nullptr;
                    iterator->next=buildPIDNode(atoi(combined));
                    if (iterator->next!=nullptr) {
                        iterator=iterator->next;
                    }
                    free(combined);
                    combined=nullptr;
                }
                if (pidList==nullptr) {
                    pidList=buildFromBuffer(buffer,&iterator,readSize);
                } else {
                    buildFromBuffer(buffer,&iterator,readSize);
                }
                if (readSize==BUFFER_SIZE &&buffer[readSize-1]!='\n') {
                    lastOne=getLastOne(buffer);
                } else {
                    lastOne=nullptr;
                }
            }
        } while (readSize==BUFFER_SIZE);
        return pidList;
    }
}

/*
    buildTree builds the process tree in a top-down order.
    It calls getCHildren to generate the root's children and
    for each child calls buildTree recursively'
*/
PIDNode * buildTree(PIDNode* root) {
    root->child=getChildren(root->PID);
    PIDNode * iterator=root->child;
    while (iterator!=nullptr) {
        iterator=buildTree(iterator);
        iterator=iterator->next;
    }
    return root;
}


/*
    print the process tree recursively.
    First print the current process name,
    then iterate through its children and call 
    printTree recursively.
*/
void printTree(PIDNode * root) {
    printf("%s",root->name);
    PIDNode * iterator=root->child;
    if (iterator!=nullptr) {
        printf(" - ");
    } else {
        printf("\n");
    }
    if (iterator!=nullptr) {
        printTree(iterator);
        iterator=iterator->next;
    }
    while (iterator!=nullptr) {
        ssize_t j=0;
        for(j=0;j!=strlen(root->name);++j) {
            printf(" ");
        }
        printf(" - ");
        printTree(iterator);
        iterator=iterator->next;
    }
}


/*
    use post-order traversal to free the whole tree.
*/
PIDNode * freeTree(PIDNode * root) {
    if (root!=nullptr) {
        freeTree(root->next);
        freeTree(root->child);
        free(root->name);
        free(root);
    }
    return nullptr;
}

/*
    viewTree firstly builds a PIDNode using current pid.
    Then it builds the whole process tree using this node.
    After calling the printTree, it frees the memory allocated
    and return.
*/
void viewTree() {
    PIDNode * root=buildPIDNode(getpid());
    root = buildTree(root);
    printTree(root);
    root=freeTree(root);
    return;
}
