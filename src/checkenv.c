
/*
 * checkenv.c
 *
 * Created on: May 20, 2015
 * By: Christopher M?rtensson
 *
 * Summary: Given no parameters, this program emulates the behaviour of the command "printenv | sort | pager" in a shell,
 *			where 'pager' is either the pager specified by the PAGER environment variable, 'less' or 'more' in that order
 *			depending on availability.
 *			Given a parameter list 'parameters', the program emulates instead the behaviour of the command "printenv | grep 'parameters' | sort | pager".
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "checkenv.h"
#include "toshfun.h"

#define PIPE_READ 0 /* read end of pipe */
#define PIPE_WRITE 1 /* write end of pipe */

/* executes a pager with no arguments but the program name. The pager chosen is either
 * the one specified by the environment variable PAGER, less, or more, in order of availability */
int page(int argc, const char **argv) {
    const char* pager = NULL; /* a string containing the name of the pager to be used */
    /* get the pager to use from the PAGER environment variable */
    pager = getenv("PAGER");
    /* if not specified, use 'less' */
    if (pager == NULL)
        pager = "less";
    /* try preferred pager first */
    execlp(pager,argv[0],(char *) NULL);
    /* Check reason for error, if simply file not found, continue, otherwise exit */
    if (errno != ENOENT) {
        perror("Could not open pager");
        exit(1);}
    /* then try less, if we haven't already */
    if (strcmp(pager,"less") != 0){
        execlp("less",argv[0],(char *) NULL);
        /* Check reason for error, if simply file not found, continue, otherwise exit */
        if (errno != ENOENT) {
            perror("Could not open pager");
            exit(1);}
    }
    /* lastly try more */
    execlp("more",argv[0],(char *) NULL);
    if (errno != ENOENT) {
        perror("Could not open pager");
        exit(1);}
    if (strcmp(pager,"less") == 0) fprintf(stderr, "Could not find a pager to use (tried 'less' and 'more')\n");
    else fprintf(stderr, "Could not find a pager to use (tried '%s', 'less' and 'more')\n",pager);
    exit(1);
}

/* chain recursively creates children, creating pipes between child stdout and parent stdin,
 * and then runs the program specified in the current position in the vector of argv's */
void chain(int* argc, char const *** argv) {
    pid_t child_id;
    int pipedesc[2];
    /* someone called chain on an empty queue */
    if (*argv == NULL){fprintf(stderr, "Chain call on empty program queue!\n"); exit(1);}
    /* while not last in queue, create children */
    while (*(argv+1) != NULL) {

        /* create pipe */
        if (pipe(pipedesc) == -1) { perror("Cannot create pipe"); exit(1); }
        /* create child */
        child_id = fork();
        if (child_id == 0){
            /* duplicate write end of pipe to stdin */
            if (dup2(pipedesc[PIPE_WRITE], STDOUT_FILENO) == -1) {perror("Cannot duplicate write pipe to stdout"); exit(1);}
            /* close ends of pipe */
            if (close(pipedesc[PIPE_READ]) == -1) {perror("Cannot close read end of pipe"); exit(1);}
            if (close(pipedesc[PIPE_WRITE]) == -1) {perror("Cannot close write end of pipe"); exit(1);}
            /* continue chaining with next job */
            ++argv;
            ++argc;
            continue; /* continue creating children */
        }
        else {
            /* this part will only be executed by parent */

            /* check if fork succeeded */
            if (child_id < 0) {
                perror("Error when using fork()");
                exit(1);
            }
            /* duplicate write end of pipe to stdout */
            if (dup2(pipedesc[PIPE_READ], STDIN_FILENO) == -1) {
                perror("Cannot duplicate read pipe to stdin");
                exit(1);
            }
            /* close pipe */
            if (close(pipedesc[PIPE_WRITE]) == -1) {
                perror("Cannot close write end of pipe");
                exit(1);
            }
            if (close(pipedesc[PIPE_READ]) == -1) {
                perror("Cannot close read end of pipe");
                exit(1);
            }

            /* We could wait here for children to finish and check exit codes,
             * but then the pipes could be filled before we get to reading from it,
             * deadlocking the application. Instead we just run our program.
             */
            /* (*job)(argc,argv); */
            execute(*argc, *argv);

            /* we should never reach this point */
            fprintf(stderr, "Error in function chain in %s line %d : job didn't exit : %s", __FILE__, __LINE__, strerror(errno));
            exit(1);
        }
    }
    /* if last in queue, don't create child, just do job and end recursion */
    /* (*job)(argc,argv); */
    execute(*argc, *argv);

    /* we should never reach this point */
    fprintf(stderr, "Error in function chain in %s line %d : job didn't exit : %s", __FILE__, __LINE__, strerror(errno));
    exit(1);
}

/* 	initializes the queue, and calls chain to start piping together
	the programs in argv */
int checkenv(int argc, char const **argv) {

    /* program program_queue[5]; */ /* the queue of programs to be piped together */
    int _argc[4] = {1,1,1,1};
    char const * argv_page[] = {"pager", NULL};
    char const * argv_sort[] = {"sort", NULL};
    char const * argv_printenv[] = {"printenv", NULL};
    char const ** argv_grep = argv;
    char const ** _argv[5];
    argv_grep[0] = "grep";

    /* initialize program queue depending on number of parameters */
    if (argc == 1){
        _argv[0] = argv_page;
        _argv[1] = argv_sort;
        _argv[2] = argv_printenv;
        _argv[3] = NULL;
    } else {
        _argv[0] = argv_page;
        _argv[1] = argv_sort;
        _argv[2] = argv_grep;
        _argv[3] = argv_printenv;
        _argv[4] = NULL;
    }

    /* begin recursive piping */
    chain(_argc,_argv);

    return 0;
}
