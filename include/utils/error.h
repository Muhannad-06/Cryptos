/* functions for error reporting */

/* errorp: print error message in stderr */
void errorp(char* fmt, ...);

/* error: print error message and terminate*/
void error(char* fmt, ...);

int asprintf(char **strp, const char *fmt, ...);