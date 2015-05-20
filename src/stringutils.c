/*
 * Created by cmarte on 5/11/15.
*/

#include "stringutils.h"
#include <string.h>

/* Splits string on a string of delimiters.
 * Inserts null characters into <str> and fills <out> with pointers into <str>. */
size_t strsplit(char* str, const char* delim, char** out) {
    char** o = out;
    char* p = str;
    while (1) {
        while (*p && strchr(delim, *p)) ++p;
        if (!*p) break;
        *o++ = p;
        while (*p && !strchr(delim, *p)) ++p;
        if (!*p) break;
        *p++ = '\0';
    }
    return o - out;
}
