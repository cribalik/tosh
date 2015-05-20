/*
 * Created by cmarte on 5/20/15.
 */

#ifndef CSHELL_SHELLFUN_H
#define CSHELL_SHELLFUN_H

#include <sys/types.h>

void setSignalHandlers();
void ignoreSomeSignals();
void defaultSignalHandling();
void checkFinishedBackgroundProcesses();
void execute(int argc, const char ** argv);
struct timeval startTimer();
double endTimer(struct timeval*);
int waitForChild(pid_t child_id, int* status);
int runInternalCommand(int argc, const char ** argv);

#endif /* CSHELL_SHELLFUN_H */
