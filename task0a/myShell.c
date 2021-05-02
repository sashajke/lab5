#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "linux/limits.h"
#include "./LineParser.h"




void execute(cmdLine* pCmdLine)
{
    int res = 1;
    res = execvp(pCmdLine->arguments[0],pCmdLine -> arguments);
    if(res == -1)
    {
        perror("error!!!!");
        exit(EXIT_FAILURE);
    }
}

int main(int argc,char** argv)
{
    while(1)
    {
        const int inputMaxSize = 2048;

        FILE* outpout = stdout;
        FILE* input = stdin;
        cmdLine* command;
        char inputFromUser[inputMaxSize];
        char workingDir[PATH_MAX];

        getcwd(workingDir,PATH_MAX);
        if(errno == ERANGE)
        {   
            printf("%s", "error when reading current directory name");
        }
        fprintf(stdout,"%s\n",workingDir);
        
        fgets(inputFromUser,inputMaxSize,input);
        if(strncmp(inputFromUser,"quit",4) == 0)
        {
            break;
        }
        command = parseCmdLines(inputFromUser);
        execute(command);
        freeCmdLines(command);
    }
    exit(EXIT_SUCCESS);
}