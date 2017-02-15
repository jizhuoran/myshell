#ifndef SIG_H
#define SIG_H
#include <signal.h>

void SIGCHLD_handler(int signum, siginfo_t * info, void *context);
void SIGINT_handler(int signum);
void SIGUSR1_handler(int signum);
void SIGCHLD_handler_wrapper();
void SIGINT_handler_wrapper();
void SIGUSR1_handler_wrapper();
void cleanup_wrapper();
#endif