#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

// Fully Windows & Cross-platform safe asprintf
int asprintf(char **strp, const char *fmt, ...) {
    // 1. Immediately set to NULL so we don't return garbage pointers on failure
    *strp = NULL; 
    
    va_list ap;
    va_start(ap, fmt);
    
    // 2. Safely calculate required length
#ifdef _WIN32
    // Windows MinGW safe calculation
    int len = _vscprintf(fmt, ap); 
#else
    // Standard C99 calculation
    int len = vsnprintf(NULL, 0, fmt, ap); 
#endif
    va_end(ap);
    
    if (len < 0) return -1;
    
    // 3. Allocate and format
    *strp = (char*)malloc((size_t)len + 1);
    if (!*strp) return -1;
    
    va_start(ap, fmt);
    int result = vsnprintf(*strp, (size_t)len + 1, fmt, ap);
    va_end(ap);
    
    return result;
}
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

