/*
 * Created by cmarte on 5/11/15.
 */

#ifndef CSHELL_EXECUTILS_H
#define CSHELL_EXECUTILS_H

#include <sys/types.h>

typedef int (*program) (int argc, char * const argv[]);

int waitForChild(pid_t child_id);
int execChild(int argc, char * const argv[], program p, int bWait, pid_t* child_pid);
int execChildFile(int argc, char * const argv[], const char* file, int bWait, pid_t* child_pid);

#endif /* CSHELL_EXECUTILS_H */
