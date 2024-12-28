#include "./../include/MyError.h"
#include <stdio.h>
#include <stdlib.h>


void errorExit(const bool errorFlag, 
                const char* errMsg, 
                const char* fileName, 
                const int errLine)
{
    if (errorFlag == true)
    {
        fprintf(stderr, "Error %s at %s: %d", errMsg, fileName, errLine);
        exit(EXIT_FAILURE);
    }
}


void errorPrint(bool errorFLag, 
                const char* errMsg, 
                const char* fileName,
                const int errLine)
{
    if (errorFLag == true)
    {
        fprintf(stderr, "Error %s at %s: %d", errMsg, fileName, errLine);
    }
}