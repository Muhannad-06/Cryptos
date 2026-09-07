#include "../../include/types.h"
#include "../../include/utils/assertion.h"
#include <stdio.h>

int assertFileEqual(FILE *fp1, FILE *fp2){
    int c1;
    int c2;

    while( ((c1 = fgetc(fp1)) != EOF) && ((c2 = fgetc(fp2)) != EOF) ){
        if(c1 != c2){
            return FAILURE;
        }
    }
    return SUCCESS;
}

int assert(char *msg, int st){
    if(!st){
        fprintf(stderr, "FAIL: %s\n", msg);
        return FAILURE;
    }
    
    fprintf(stderr, "SUCSESS: %s\n", msg);
    return SUCCESS;
}