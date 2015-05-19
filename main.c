/*
 * Created by cmarte on 5/11/15.
*/

#define __USE_POSIX
#define _XOPEN_SOURCE 700
#define _POSIX_SOURCE 1

#include "executils.h"
#include "digenv.h"
#include <signal.h>

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "stringutils.h"
#include <errno.h>
#include <sys/wait.h>

#define TRUE 1
#define FALSE 0

#ifndef SIGDET
#define SIGDET 0
#endif

void checkBackgroundProcesses() {
    int status;
    pid_t pid;
    while ((pid = waitpid(0, &status, WNOHANG)) != 0 
        && !(pid == -1 && errno != EINTR)) /* Try agian if we where interrupted */
    {
        printf("%i Done\n", pid);
    };
}

void newlineSignalHandler(int s) {
    printf("\n");
}

void ChildExitedSignalHandler(int s) {
    putchar('\n');
    checkBackgroundProcesses();
}

double gettimediff(struct timeval* t2, struct timeval* t1) {
    return (double) (t2->tv_sec - t1->tv_sec) + (t2->tv_usec - t1->tv_usec) / 1000000.0f;
}

void runbackground(int argc, char * const argv[], program comm) {
    execChild(argc, argv, comm, FALSE, NULL);
}

void runforeground(int argc, char * const argv[], program comm) {
    struct timeval t1,t2;
    int status;
    pid_t child_pid;
    gettimeofday(&t1, NULL);
    status = execChild(argc, argv, comm, TRUE, &child_pid);
    if (status)
        fprintf(stderr, "%i exited with exit status %i\n", child_pid, status);
    gettimeofday(&t2, NULL);
    printf("%i Done after %f seconds\n", child_pid, gettimediff(&t2,&t1));
}

void runbackgroundFile(int argc, char * const argv[], const char* file) {
    execChildFile(argc, argv, file, FALSE, NULL);
}

void runforegroundFile(int argc, char * const argv[], const char* file) {
    struct timeval t1,t2;
    int status;
    pid_t child_pid;
    gettimeofday(&t1, NULL);
    status = execChildFile(argc, argv, file, TRUE, &child_pid);
    if (status)
        fprintf(stderr, "%i exited with exit status %i\n", child_pid, status);
    gettimeofday(&t2, NULL);
    printf("%i Done after %f seconds\n", child_pid, gettimediff(&t2,&t1));
}

int main (int argc, char * const argv[]) {
    static char input[81];
    static char* args[80];

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
        /* TODO: Block more signals */
    }

    while (1) {
        int backgrnd = 0; /* run in background or not */
        size_t numargs; /* number of arguments */
        printf("cshell: ");
        /* get line */
        if (!fgets(input, 81, stdin) && errno == EINTR)
            continue; /* If system call was interrupted for some reason */

#if !(defined(SIGDET) && SIGDET)
        /* check for exited background processes */
        checkBackgroundProcesses();
#endif

        numargs = strsplit(input, "\n ", args); /* split into args */
        if (numargs == 0) continue;
        /* check if last argument is '&' */
        if (strcmp("&", args[numargs-1]) == 0) {
            backgrnd = 1;
            --numargs;
        }
        args[numargs] = NULL;

        /* Find program to execute */
        if (strcmp("exit", args[0]) == 0) {
            puts("Bye!");
            return 0;
        }
        else if (strcmp("cd", args[0]) == 0) {
            if (chdir(args[1]) == -1)
                perror("Could not change directory");
        }
        else if (strcmp("checkEnv", args[0]) == 0) {
            if (backgrnd)
                runbackground(numargs, args, digenv);
            else
                runforeground(numargs, args, digenv);
        } else {
            if (backgrnd)
                runbackgroundFile(numargs, args, args[0]);
            else
                runforegroundFile(numargs, args, args[0]);
        }
    }
}
