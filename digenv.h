/*
 * Created by cmarte on 5/11/15.
*/

#ifndef CSHELL_DIGENV_H
#define CSHELL_DIGENV_H

/*
 * programs.h
 *
 * Created on: April 23, 2014
 * By: Christopher M?rtensson
 *
 * Summary: This header includes various functions that can be placed in the
 * 			program_queue queue in digenv.c and will be called by the 'chain' function.
 *			Each function will call a program with exec and terminate even if an error occured.
*/

#define PRINTENV 0
#define GREP 1
#define SORT 2
#define PAGER 3

int printenv (int argc, char * const argv[]);
int grep (int argc, char * const argv[]);
int sort (int argc, char * const argv[]);
int page (int argc, char * const argv[]);

int digenv(int argc, char * const argv[]);

#endif /* CSHELL_DIGENV_H */
