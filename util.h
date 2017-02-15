#ifndef UTIL_H
#define UTIL_H
#ifndef nullptr
#define nullptr NULL
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef bool
#define bool int
#endif

#define BUFFER_SIZE 1024
#define MAX_ARGS_NUMBER 30
#define pipe_out(pipefd) close(pipefd[0]);dup2(pipefd[1],STDOUT_FILENO);close(pipefd[1]);
#define pipe_in(pipefd) close(pipefd[1]);dup2(pipefd[0], STDIN_FILENO);close(pipefd[0]);
#define close_pipe(pipefd) close(pipefd[0]);close(pipefd[1]);
#define EXIT_TYPE -1
#define VIEWTREE_TYPE -2
#define TIMEX_TYPE 1
#define NORMAL_TYPE 0
#define MAX_PROC_FILE_PATH 256
#define MAX_PIPE_NUMBER 5

#include <sys/types.h>

typedef struct Command {
    int argc;
    char * argv[MAX_ARGS_NUMBER];
    struct Command *next;
} Command;

typedef struct Line {
    int type;
    int background;
    Command * head;
} Line;

typedef struct PIDNode {
    pid_t PID;
    pid_t PPID;
    char * name;
    struct PIDNode *next;
    struct PIDNode *child;
} PIDNode;

bool allSpace(char *input);
int split_input(char *input, char **output, char *delimiter, bool flag);
PIDNode * buildPIDNode(pid_t inp);
char * copy(char * buffer,ssize_t i, ssize_t j);
void freeCommand(Command * cmd);
void freeLine(Line * line);
#endif