#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "linux/limits.h"
#include "./LineParser.h"


void printDebug(char* command)
{
    fprintf(stderr,"PID: %d\n", getpid());
    fprintf(stderr,"Executing Command: %s\n",command);
}
void handler(int sig)
{
	printf("\nRecieved Signal : %s\n",strsignal(sig));
	if(sig == SIGTSTP)
	{
		signal(SIGCONT,SIG_DFL);
        signal(SIGTSTP,SIG_DFL);
	}
	else if(sig == SIGCONT)
	{
        signal(SIGCONT,SIG_DFL);
		signal(SIGTSTP,SIG_DFL);
	}
	else
	{
		signal(sig,SIG_DFL);
	}
    raise(sig);
}



void execute(cmdLine* pCmdLine,int debug)
{
    int res;
    int childID;
    int chdirRes = 0;
    if(strncmp("cd",pCmdLine->arguments[0],2) == 0)
    {
        if(debug)
        {
            printDebug(pCmdLine->arguments[0]);
        }
        chdirRes = chdir(pCmdLine -> arguments[1]);
        if(chdirRes == -1)
        {
            fprintf(stderr,"error : %d , while trying cd command",errno);
        }
    }
    else
    {
        childID = fork();

        if(childID == 0)
        {
            if(debug)
            {
                printDebug(pCmdLine -> arguments[0]);
            }
            res = execvp(pCmdLine->arguments[0],pCmdLine -> arguments);
            if(res == -1)
            {
                perror("error!!!!");
                _exit(EXIT_FAILURE);
            }   
        }
    }

}

int main(int argc,char** argv)
{
    signal(SIGINT,handler);
    signal(SIGTSTP,handler);
    signal(SIGCONT,handler);
    while(1)
    {

        const int inputMaxSize = 2048;
        int i,debug = 0;
        FILE* outpout = stdout;
        FILE* input = stdin;
        cmdLine* command;
        char inputFromUser[inputMaxSize];
        char workingDir[PATH_MAX];
        
        for(i=1;i<argc;i++)
        {
            if(argv[i][1] == 'd')
                debug = 1;
        }

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
        execute(command,debug);
        freeCmdLines(command);
    }
    exit(EXIT_SUCCESS);
}