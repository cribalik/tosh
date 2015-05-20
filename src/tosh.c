/*
 * Created by cmarte on 5/20/15.
 */

#define __USE_POSIX 1
#define __USE_GNU 1
#define _GNU_SOURCE 1
#define _XOPEN_SOURCE 700
#define _POSIX_SOURCE 1


#include <unistd.h>
#include "tosh.h"
#include "toshfun.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include "stringutils.h"

int tosh() {
    static char input[81];
    static char * args[80];

    setSignalHandlers();

    while (1) {

        int backgrnd = 0; /* run in background or not */
        size_t numargs; /* number of arguments */
        struct timeval t;

#if !(defined(SIGDET) && SIGDET)
        /* check for exited background processes */
        checkFinishedBackgroundProcesses();
#endif
        /* print a prompt */
        printf("%s: ", get_current_dir_name());

        /* get command line */
        if (!fgets(input, 81, stdin) && errno == EINTR)
            continue; /* If system call was interrupted for some reason */

        /* split into args */
        numargs = strsplit(input, " \n", args);
        if (numargs == 0) continue; /* empty line */

        /* check if last argument is '&', if so, we run in background */
        if (strcmp("&", args[numargs-1]) == 0) {
            backgrnd = 1;
            --numargs;
        }
        /* null terminate argument list */
        args[numargs] = NULL;

        /* Execute program */
        if (!backgrnd) /* time if foreground */
            t = startTimer();

        /* Check if it is an internal command first */
        if ( !runInternalCommand(numargs, (const char **) args) )
        /* otherwise create child to execute the program */
        {
            int status;
            pid_t child_id = fork();

            if (child_id == 0) {

                /* we want background processes to ignore some signals
                 * but foreground processes should be interruptable */
                if (backgrnd) {
                    ignoreSomeSignals();
                    /* Prevent background processes (for example grep) from listening on our stdin
                     * it's for shell input and foreground process input only */
                    fclose(stdin);
                }
                else
                    defaultSignalHandling();

                execute(numargs, (const char **) args);

            } else if (child_id < 0) {

                perror("Error when calling fork()");

            } else {

                printf("%i Begin\n", child_id);
                /* wait if not in background */
                if (!backgrnd) {
                    if (waitForChild(child_id, &status) == -1)
                        perror("Wait for child to finish failed");

                    /* check how child exited and print result */
                    if (WIFEXITED(status)){
                        printf("%i Done. Exit code %i\n", child_id, WEXITSTATUS(status));
                    } else if (WIFSIGNALED(status)){
                        int child_signal = WTERMSIG( status );
                        printf( "%i terminated by signal number %d (%s)\n", child_id, child_signal, strsignal(child_signal) );
                    }
                }

            }

        }
        if (!backgrnd)
            printf("Ran for %f seconds\n", endTimer(&t));
    }
    return 0;
}

