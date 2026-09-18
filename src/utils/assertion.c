#include "../../include/types.h"
#include "../../include/utils/assertion.h"
#include <stdio.h>

#include "utils/error.h"

int assertFileEqual(char *msg, FILE *fp1, FILE *fp2){
    if (!fp1 || !fp2 || !msg) {
        error("assertFileEqual: null pointer");
    }
    // return cursor to each file's beginning.
    rewind(fp1);
    rewind(fp2);

    int c1;
    int c2;

    for(;;){
        c1 = getc(fp1);
        c2 = getc(fp2);
        if (c1 == c2) {
            if (c1 == EOF) {
                fprintf(stderr, "SUCCESS(files are equal): %s\n", msg);
                return SUCCESS;
            }
        } else {
            break;
        }
    }
    fprintf(stderr, "FAILURE(files are NOT equal): %s\n", msg);
    return FAILURE;
}

int assert(char *msg, int st){
    if(!st){
        fprintf(stderr, "FAIL: %s\n", msg);
        return FAILURE;
    }
    
    fprintf(stderr, "SUCCESS: %s\n", msg);
    return SUCCESS;
}