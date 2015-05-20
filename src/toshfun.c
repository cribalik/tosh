/*
 * Created by cmarte on 5/20/15.
 */

#define __USE_POSIX
#define _GNU_SOURCE 1
#define _XOPEN_SOURCE 700
#define _POSIX_SOURCE 1

#include "toshfun.h"
#include "checkenv.h"
#include <signal.h>
#include <string.h>

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>

#ifndef SIGDET
#define SIGDET 0
#endif

/*** SIGNAL HANDLERS ***/

void newlineSignalHandler(int s) {
    printf("\n");
}

void ChildExitedSignalHandler(int s) {
    /* putchar('\n'); */
    checkFinishedBackgroundProcesses();
}

void WaitForChildrenSignalHandler(int s) {
    int status;
    pid_t pid;
    while ((pid = wait(&status)) != 0 && !(pid == -1 && errno != EINTR)) {
        printf("%i Terminated\n", pid);
    }
}


/*** SHELL FUNCTIONS ***/

/* wait for any finished children to de-zombify them and print their  */
void checkFinishedBackgroundProcesses() {
    int status;
    pid_t pid;
    while ((pid = waitpid(0, &status, WNOHANG)) != 0 && pid != -1) {
        if (WIFEXITED(status)){
            printf("%i Done. Exit code %i\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)){
            int child_signal = WTERMSIG( status );
            printf( "%i terminated by signal number %d (%s)\n", pid, child_signal, strsignal(child_signal) );
        }
    };
}

void execute(int argc, const char **argv) {

    /*** Shell internal commands ***/

    if (runInternalCommand(argc, argv))
        return;

    /*** Aliases ***/

    else if (strcmp("pager", argv[0]) == 0) {
        page(argc, argv);
    }
    else if (strcmp("checkEnv", argv[0]) == 0) {
        checkenv(argc, argv);
    }

    /*** External command ***/

    else {
        execvp(argv[0], (char * const*) argv);
        fprintf(stderr, "Could not run program %s : %s\n", argv[0], strerror(errno));
        exit(0);
    }
}

void setSignalHandlers() {
    /* set signal handlers */
    {
        struct sigaction s;
        sigemptyset(&s.sa_mask);
        s.sa_flags = 0;

        s.sa_handler = newlineSignalHandler;
        sigaction(SIGINT, &s, NULL);
#if defined(SIGDET) && SIGDET
        s.sa_handler = ChildExitedSignalHandler;
        sigaction(SIGCHLD, &s, NULL);
#endif
        s.sa_handler = WaitForChildrenSignalHandler;
        sigaction(SIGTERM, &s, NULL);
    }
}

void ignoreSomeSignals() {
    signal(SIGINT, SIG_IGN);
}

void defaultSignalHandling() {
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
}

/* Wait for process with process id <child_id> to finish. returns -1 on error, 0 on success */
int waitForChild(pid_t child_id, int* status) {
    pid_t id;
    /* wait for child to finish, keep going at it if we were interrupted */
    while ((id = waitpid(child_id, status, 0)) == -1 && errno == EINTR) {};
    if (id == -1
        && errno != ECHILD) { /* Check if child hasn't been dezombified yet */
        return -1;
    }

    return 0;
}

struct timeval startTimer() {
    struct timeval result;
    gettimeofday(&result, NULL);
    return result;
}

double endTimer(struct timeval* t1) {
    struct timeval t2;
    gettimeofday(&t2, NULL);
    return (double) (t2.tv_sec - t1->tv_sec) + (t2.tv_usec - t1->tv_usec) / 1000000.0f;
}

/* Checks if a command is an internal shell command and, if so, runs it.
 * Returns 0 if command is not an internal command, 1 if it is
 */
int runInternalCommand(int argc, const char **argv) {
    if (strcmp("exit", argv[0]) == 0) {
        printf("Killing children...\n");
#if defined(SIGDET) && SIGDET
            signal(SIGCHLD, SIG_DFL);
#endif
        kill(0, SIGTERM);
        printf("Children killed, Bye!\n");
        exit(0);
    }
    else if (strcmp("cd", argv[0]) == 0) {
        if (chdir(argv[1]) == -1)
            perror("Could not change directory");
        return 1;
    }
    return 0;
}
