/* Assertion functions
*/
#include <stdio.h>

int assertFileEqual(FILE *fp1, FILE *fp2); /* Returns 0 if the content of both files is not equal. Returns a non-negative value if they were equal. */