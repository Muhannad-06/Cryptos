#include "assertion.h"
#include <stdio.h>

int assertFileEqual(FILE *fp1, FILE *fp2){
    int c1;
    int c2;

    while( ((c1 = fgetc(fp1)) != EOF) && ((c2 = fgetc(fp2)) != EOF) ){
        if(c1 != c2){
            return 0;
        }
    }
    return 1;
}