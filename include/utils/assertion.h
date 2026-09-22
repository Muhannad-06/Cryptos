/* Assertion functions
*/
#include <stdio.h>

int assertFileEqual(char *msg, FILE *fp1, FILE *fp2); /* Returns FAILURE if the content of both files is not equal. Returns a SUCSESS value if they were equal. */
int assert(char *msg, int st); /* takes a statement and prints fail message if it's not true. */