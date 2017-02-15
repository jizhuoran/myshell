#include "sig.h"
#include "execute.h"
#include "util.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>

/*
	sigusr1_flag is a flag that tested by the child process
	in a while loop, when sigusr1_flag became 1, child begin
	to execuate(finished busy waiting). It is set to 0 before
	fork the child process.

	timeX_flag is a flag that tested by the SIGCHLD handler
	if it is 1, then this command line is timex type, the time
	infomation will be print. It is set to 0 for every main loop.

*/
volatile sig_atomic_t sigusr1_flag =0;
volatile sig_atomic_t timeX_flag=0;



/*
    SIGCHLD_handler will test whether the signal sender
    is running in background mode, if it is, print the info
    of the pid, name when it is done.
    And if it is foreground process, test whether timeX
    flag is set, if it is, print the timeX info.
    Finally, remove the zombie.
*/
void SIGCHLD_handler(int signum, siginfo_t * info, void *context) {
    int pid = info->si_pid;
    int gid = getpgid(pid);
    if(gid == -1) {
        errno = 0;
    }
    if (pid == gid) { // if pid == gid, it is background process
        PIDNode *pnode = buildPIDNode(pid);
        printf("[%d] %s Done\n",pid, pnode->name);
        fflush(stdout);
    } else if (timeX_flag) {
        print_timeX(pid);
    }
    waitpid(pid, NULL, 0);// clean up
}


/*
	We handle the SIGINT signal, hence the process
	wii not terminate when receive it. And '\n' will
	be printed to stdout.
*/
void SIGINT_handler(int signum) {
    printf("\n");
}


/*
	parent process will send the SIGUSR1 to child, when
	receive this signal, child will begin to execuate.
	In this handler, sigusr1_flag will be set to 1.
*/
void SIGUSR1_handler(int signum) {
    sigusr1_flag = 1;
}

/*
	SA_NOCLDSTOP: If signum is SIGCHLD, do not receive 
	notification when child processes stop. Since
	the SIGCHLD_handler only deal with child which is terminate.

	SA_SIGINFO: Since we need to use siginfo_t, this flag
	should be set.

	SA_RESTART: system call should be resarted after
	this handler.
*/
void SIGCHLD_handler_wrapper() {
    struct sigaction act;
    sigaction(SIGCHLD, NULL, &act);
    act.sa_sigaction = SIGCHLD_handler;
    act.sa_flags |= SA_NOCLDSTOP;
    act.sa_flags |= SA_SIGINFO;
    act.sa_flags |= SA_RESTART;
    sigaction(SIGCHLD, &act, NULL);
}

/*
	SA_RESTART: system call should be resarted after
	this handler.

	SA_NODEFER: do not mask this signal in the signal handler

*/

void SIGUSR1_handler_wrapper() {
    struct sigaction act;
    sigaction(SIGUSR1, NULL, &act);
    act.sa_handler = SIGUSR1_handler;
    act.sa_flags |= SA_RESTART;
    act.sa_flags |= SA_NODEFER;
    sigaction(SIGUSR1, &act, NULL);
}


/*
	SA_RESTART: Never restart the system call, just let
	the read() return NULL.
*/
void SIGINT_handler_wrapper() {
    struct sigaction act;
    sigaction(SIGCHLD, NULL, &act);
    act.sa_handler = SIGINT_handler;
    act.sa_flags &=~SA_RESTART;
    sigaction(SIGINT, &act, NULL);
}


/*
	cleanup_wrapper() will clean up the infomatin
	of the terminated child process. In other word,
	it will clean up the zombie process.
	Same as SIGCHLD_handler
*/
void cleanup_wrapper() {
    for(int i = 0 ; i < 256; ++i){
        siginfo_t tmp;
        int i = waitid(P_ALL, getpid(), &tmp, WNOWAIT | WNOHANG | WEXITED);
        if (i == -1) {
            break;
        } else {
            int pid = tmp.si_pid;
            int gid = getpgid(pid);
            if(gid == -1) {
                errno = 0;
            }
            if (pid == gid) {
                PIDNode *pnode = buildPIDNode(pid);
                printf("[%d] %s Done\n",pid, pnode->name);
                fflush(stdout);
            } else if (timeX_flag) {
                print_timeX(pid);
            }
            waitpid(pid, NULL, WNOHANG);
        }
    }
}
