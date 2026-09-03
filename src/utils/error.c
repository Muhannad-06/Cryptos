#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void errorp(char *fmt, ...){
    va_list args;

    va_start(args, fmt);

    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt , args);
    fprintf(stderr, "\n");

    va_end(args);
}

void error(char* fmt, ...){
    va_list args;

    va_start(args, fmt);

    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt , args);
    fprintf(stderr, "\n");

    va_end(args);
    
    exit(1);
}

