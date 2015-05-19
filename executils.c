/*
 * Created by cmarte on 5/11/15.
*/

#define _GNU_SOURCE
#include "executils.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

int waitForChild(pid_t child_id) {
    int status;
    pid_t id;
    /* wait for child to finish, keep going at it if we were interrupted */
    while ((id = waitpid(child_id, &status, 0)) == -1 && errno == EINTR) {};
    if (id == -1
	&& errno != ECHILD) /* Child wasn't closed by signal handler*/
        perror("Wait for process to finish failed");

    /* check if child finished correctly */
    if (WIFEXITED(status)){
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)){
        /* child exited because of signal, print error and exit */
        int child_signal = WTERMSIG( status );
        fprintf( stderr, "Child pid %li was terminated by signal number %d (%s), exiting...\n", (long int) child_id, child_signal, strsignal(child_signal) );
        return 0;
    }

    /* we should never reach this point */
    perror("### Error ### In executils.h execChildAndWait() : Process neither exited nor was terminated by a signal! Something is very wrong");
    exit(-1);
}

int execChildFile(int argc, char * const argv[], const char* file, int bWait, pid_t* child_pid) {
    pid_t child_id = fork();
    if (child_id == 0) {
        /* background processes ignore user signals, foreground uses default */
        signal(SIGINT, bWait ? SIG_DFL : SIG_IGN);
        /* TODO: More signals */
        execvp(file, argv);
        perror("Could not run program");
        exit(0);
    }

    /* check if fork failed */
    if (child_id < 0){perror("Error when using fork()"); return 0;}

    printf("%i Begin\n", child_id);

    /* provide child process id if requested */
    if (child_pid) *child_pid = child_id;

    /* wait for process to finish if requested */
    if (bWait)
        return waitForChild(child_id);
    return 0;
}

int execChild(int argc, char * const argv[], program p, int bWait, pid_t* child_pid) {
    pid_t child_id = fork();
    if (child_id == 0) {
        /* background processes ignore user signals, foreground uses default */
        signal(SIGINT, bWait ? SIG_DFL : SIG_IGN);
        /* TODO: More signals */
        p(argc, argv);
    }

    /* check if fork failed */
    if (child_id < 0)
        perror("Error when using fork()");

    printf("%i Begin\n", child_id);

    /* provide child process id if requested */
    if (child_pid) *child_pid = child_id;

    /* wait for process to finish if requested */
    if (bWait)
        return waitForChild(child_id);
    return 0;
}
