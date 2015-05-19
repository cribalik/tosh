/*
 * digenv.c
 *
 * Created on: April 23, 2014
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
#include <sys/types.h>
#include <sys/wait.h>

#include "executils.h"
#include "digenv.h" /* contains all the functions (program executors) that run the programs whose stdin and stdout will be piped together */


#define PIPE_READ 0 /* read end of pipe */
#define PIPE_WRITE 1 /* write end of pipe */

static const program programs[4] = {printenv, grep, sort, page};

/* executes the program 'printenv' with no arguments but the program name */
int printenv (int argc, char * const argv[]) {
    execlp("printenv",argv[0],(char*) NULL);
    /* if we reach this point, execlp failed */
    perror("Could not execute 'printenv'");
    exit(1);}


/* executes the program 'grep' with the given arguments */
int grep (int argc, char * const argv[]) {
    execvp("grep",argv);
    /* if we reach this point, execvp failed */
    perror("Could not execute 'grep'");
    exit(1);}


/* executes the program 'sort' with no arguments but the program name */
int sort (int argc, char * const argv[]) {
    execlp("sort",argv[0],(char *) NULL);
    /* if we reach this point, execlp failed */
    perror("Could not execute 'sort'");
    exit(1);
}

int page(int argc, char * const argv[]) {
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
 * and then calls the next function/program executer that 'job' points to, until *job is a null pointer */
int chain(int argc, char * const argv[], program* job){
    pid_t child_id, id;
    int status;
    int pipedesc[2];
    /* someone called chain on an empty queue */
    if (*job == NULL){fprintf(stderr, "Chain call on empty program queue!\n"); exit(1);}
    /* if last in queue, don't create child, just do job and end recursion */
    if (*(job+1) == NULL) { (*job)(argc,argv); }
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
        chain(argc,argv,job+1);
    }

    /* this part will only be executed by parent */

    /* check if fork failed */
    if (child_id < 0){perror("Error when using fork()"); exit(1);}
    /* duplicate write end of pipe to stdout */
    if (dup2(pipedesc[PIPE_READ], STDIN_FILENO) == -1) {perror("Cannot duplicate read pipe to stdin"); exit(1);}
    /* close pipe */
    if (close(pipedesc[PIPE_WRITE]) == -1) {perror("Cannot close write end of pipe"); exit(1);}
    if (close(pipedesc[PIPE_READ]) == -1) {perror("Cannot close read end of pipe"); exit(1);}

    /* wait for child to finish */
    /* TODO: Don't wait for child, pipe like ordinary shells do. Makes it easier on us as well */
    while ((id = waitpid(child_id, &status, 0)) == -1 && errno == EINTR) {};
    if (id == -1 
	&& errno != ECHILD) /* child wasnt closed by signal handler */
        perror("Wait for process to finish failed");

    /* check if child finished correctly */
    if (WIFEXITED(status)){
        int child_status = WEXITSTATUS(status);
        if (child_status != 0){
            /* there was an error, exit and pass on the error code to parent */
            exit(child_status);
        }
    } else if (WIFSIGNALED(status)){
        /* child exited because of signal, print error and exit */
        int child_signal = WTERMSIG( status );
        fprintf( stderr, "Child pid %li was terminated by signal %s, exiting...\n", (long int) child_id, strsignal(child_signal) );
        exit(1);
    }

    /* child finished without errors, now execute own program */
    (*job)(argc,argv);

    /* we should never reach this point */
    return -1;
}

/* 	initializes the program_queue, and calls chain to start piping together
	the programs in program_queue */
int digenv(int argc, char * const argv[]) {

    program program_queue[5]; /* the queue of programs to be piped together */

    /* initialize program queue depending on number of parameters */
    if (argc == 1){
        program_queue[0] = programs[PAGER];
        program_queue[1] = programs[SORT];
        program_queue[2] = programs[PRINTENV];
        program_queue[3] = NULL;
    } else {
        program_queue[0] = programs[PAGER];
        program_queue[1] = programs[SORT];
        program_queue[2] = programs[GREP];
        program_queue[3] = programs[PRINTENV];
        program_queue[4] = NULL;
    }

    /* begin recursive piping */
    chain(argc,argv,program_queue);

    return 0;
}
