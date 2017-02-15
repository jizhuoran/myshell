#include "execute.h"
#include "sig.h"
#include "viewtree.h"
#include <unistd.h>
#include <wait.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

extern sig_atomic_t sigusr1_flag;
extern sig_atomic_t timeX_flag;



void run_command(Command *cmd, int is_background) {
    if (is_background) {
        setpgid(0, 0); //put the child process into another group of processes.
    }
    while(sigusr1_flag == 0);
    execvp(cmd->argv[0], cmd->argv);
    fprintf(stderr, "myshell: '%s': %s\n", cmd->argv[0], strerror(errno));
    exit(EXIT_FAILURE);
}

/*
    If it is a background process, wait_wrapped doesn't wait and returns.
    Else it allows the parent process to ignore SIGINT. 
*/
void wait_wrapped(int pid, int is_background, int flag) {
    if (!is_background) {
        struct sigaction act;
        sigaction(SIGINT,nullptr,&act);
        signal(SIGINT,SIG_IGN);
        waitid(P_PID, pid, NULL, WNOWAIT | WEXITED);
        sigaction(SIGINT,&act,nullptr);
    }
}

/*
    This is a wrapped version of fork().
    It sets sigusr1_flag to 0 (refer to sig.c)
    and execute fork(). If fork fails it 
    prints error and returns -1. If succeeds, the parent
    process sends a SIGUSR1 to pid and then returns the pid. 
*/
int safe_fork() {
    sigusr1_flag = 0;
    int pid = fork();
    if (pid == -1) {
        fprintf(stderr, "can not fork\n");
        return -1;
    }
    if (pid>0) {
        kill(pid, SIGUSR1);
    }
    return pid;
}

/*
    It executes the Line accordingly. If the Line->type is exit, 
    it prints the message, releases memory and exits. If the Line->type
    is viewtree, it releases memory and calls viewTree. If the Line->type
    is TIMEX_TYPE, it sets the timeX_flag to 1. If there's no piping, 
    It forks a new child and executes the command while the parent process will wait for it.
    If there's piping, iterates through all the commands.
*/
void execute(Line *line) {
    if (line->type ==EXIT_TYPE) {
        fprintf(stderr, "myshell: Terminated\n");
        freeLine(line);
        exit(EXIT_SUCCESS);
    } else if (line->type==VIEWTREE_TYPE) {
        freeLine(line);
        viewTree();
    } else {
        if (line->type==TIMEX_TYPE) {
            timeX_flag=1;
        }
        if (line->head->next == NULL) {
            int pid = safe_fork();
            if (pid == 0) {
                run_command(line->head, line->background);
            }else if (pid > 0) {
                wait_wrapped(pid, line->background, line->type);
            }
        } else {
            Command *iterator = line->head;
            int pipefd[MAX_PIPE_NUMBER][2];
            pid_t pid_list[MAX_PIPE_NUMBER] = {0};
            int pipe_number = 0;
            pipe(pipefd[pipe_number]);
            pid_t pid = safe_fork();
            pid_list[pipe_number] = pid;
            if (pid == 0) {                          //piping first command.
                pipe_out(pipefd[pipe_number]);
                run_command(iterator, line->background);
            } 
            while(iterator->next->next != NULL) {  //piping intermediate commands
                iterator = iterator -> next;
                ++pipe_number;
                pipe(pipefd[pipe_number]);
                pid_t pid = safe_fork();
                pid_list[pipe_number] = pid;
                if(pid == 0) {
                    pipe_in(pipefd[pipe_number-1]);
                    pipe_out(pipefd[pipe_number]);
                    run_command(iterator, line->background);
                }else if (pid > 0) {
                    close_pipe(pipefd[pipe_number-1]);
                }
            }
            iterator = iterator->next;
            pid = safe_fork();

            if (pid == 0) { // piping last command.
                pipe_in(pipefd[pipe_number]);
                run_command(iterator, line->background);
            }else if (pid > 0) {
                close_pipe(pipefd[pipe_number]);
            }
            wait_wrapped(pid, line->background, line->type);
           
            for (size_t i = 0; i < pipe_number + 1; i++) {
                wait_wrapped(pid_list[i], line->background, line->type);
            }    
        }
        freeLine(line);
    }
}

/*
    print_timeX reads /proc/pid/stat to get the corresponding
    statistics and prints the required information.
*/
void print_timeX(pid_t pid) {
    int pid_get;
    char cmd[MAX_PROC_FILE_PATH];
    unsigned long ut, st;

    char str[MAX_PROC_FILE_PATH];
    sprintf(str, "/proc/%d/stat", pid);
    FILE *file = fopen(str, "r");
    if (file == NULL) {
        return;
    }
    int z;
    unsigned long h;
    char stat;
    double time_from_boot;
    int efb;

    fscanf(file, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %d %d %d %d %d %d %d", &pid_get, cmd, &stat, &z, &z, &z, &z, &z,
           (unsigned *)&z, &h, &h, &h, &h, &ut, &st, &z, &z, &z, &z, &z, &z, &efb);
    fclose(file);
    cmd[strlen(cmd) - 1] = 0;
    double utime = ut*1.0f/sysconf(_SC_CLK_TCK);
    double stime = st*1.0f/sysconf(_SC_CLK_TCK);
    double exec_from_boot = efb*1.0f/sysconf(_SC_CLK_TCK);

    FILE *uptime = fopen("/proc/uptime", "r");
    if (uptime == NULL) {
	    return;
    }
    fscanf(uptime, "%lf", &time_from_boot);
    fclose(uptime);
    double runtime=time_from_boot-exec_from_boot;
    runtime=runtime<0?0:runtime;

    printf("\n");
    fflush(stdout);
    printf("%-10s%-15s%-10s%-10s%-10s\n", "PID", "CMD", "RTIME", "UTIME", "STIME");
    fflush(stdout);
    printf("%-10d%-15s%-4.2lf%-6s%-4.2lf%-6s%-4.2lf%-6s\n", pid_get, cmd+1,runtime, " s", utime," s", stime," s");
    fflush(stdout);
}
