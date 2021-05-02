#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include "linux/limits.h"
#include "./LineParser.h"
#include "sys/wait.h"

#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0

typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                  /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;

void freeProcessList(process* processList)
{
    if(processList != NULL)
    {
        freeCmdLines(processList -> cmd);
        freeProcessList(processList -> next);
        free(processList);
        processList = NULL;
    }
}

int getStatus(pid_t pid,int prevStatus)
{
    int status,res;
    res = waitpid(pid,&status,WNOHANG | WUNTRACED | WCONTINUED);
    if(res == -1)
    {
        // NO CHILD PROCESS , ASSUME WE FINISHED ALREADY SO ITS TERMINATED
        //return TERMINATED;
    }
    if(res == 0)
        return prevStatus; // IF WE DIDN'T CHANGE STATUS SINCE LAST CALL
    else if(WIFEXITED(status))
        return TERMINATED; // IF WE EXITED
    else if(WIFSIGNALED(status))
        return TERMINATED;
    else if(WIFSTOPPED(status))
        return SUSPENDED; // IF WE WERE STOPPED
    else if(WIFCONTINUED(status))
        return RUNNING; // IF WE RESUMED AFTER STOP
    return RUNNING; // BY DEFAULT WE RETURN RUNNING ALTHOUGH WE ARE NOT SUPPOSED TO GET HERE BECAUSE ONE OF THE OPTIONS WOULD HAVE HAPPEND
}

void removeTerminatedProcesses(process** processList)
{
    process* currProcess = *(processList);
    process* previousProcess = NULL;
    process* temp;
    while(currProcess != NULL)
    {
        if(currProcess -> status == TERMINATED)
        {
            freeCmdLines(currProcess -> cmd);
            if(previousProcess != NULL)
            {
                previousProcess -> next = currProcess -> next;
                free(currProcess);
                currProcess = previousProcess -> next;
            }
            else // if we delete the first link
            {
                temp = currProcess -> next;
                free(currProcess);
                currProcess = temp;
                *(processList) = currProcess;
            }           
        }
        else
        { 
            previousProcess = currProcess;
            currProcess = currProcess -> next;        
        }

    }
    if(previousProcess == NULL && currProcess == NULL) // if we deleted the last one(empty list)
    {
        *(processList) = NULL;
    }
}

void updateProcessList(process **process_list)
{
    process* currProcess = *(process_list);
    while(currProcess != NULL)
    {
        currProcess -> status = getStatus(currProcess -> pid,currProcess -> status);
        currProcess = currProcess -> next;
    }
}
void addToEnd(process** process_list, cmdLine* cmd, pid_t pid)
{
    process* currProc = *(process_list);
    process* toAdd = (process*)malloc(sizeof(process));
    toAdd -> cmd = cmd;
    toAdd -> pid = pid;
    toAdd -> status = getStatus(pid,RUNNING);
    toAdd -> next = NULL;
    
    while(currProc -> next != NULL )
        currProc = currProc -> next;

    currProc -> next = toAdd;
}
void addProcess(process** process_list, cmdLine* cmd, pid_t pid)
{
    process* currProc = *(process_list);

    if(currProc == NULL)
    {
        currProc = malloc(sizeof(process));
        currProc -> cmd = cmd;
        currProc -> pid = pid;
        currProc -> status = getStatus(pid,RUNNING);
        currProc -> next = NULL;
        *(process_list) = currProc;
    }
    else
    {
        addToEnd(process_list,cmd,pid);
    }
}

void printProcessList(process** process_list)
{
    updateProcessList(process_list);
    process* currProcess = *(process_list);
    char* currStatus;
    printf("PID           Command           STATUS\n");
    while(currProcess != NULL)
    {
        currStatus = currProcess -> status == 0 ? "SUSPENDED" : currProcess -> status == -1 ? "TERMINATED" : "RUNNING";       
        printf("%d           %s           %s\n\n",currProcess->pid,currProcess -> cmd ->arguments[0],currStatus);
        currProcess = currProcess -> next;
    }
    removeTerminatedProcesses(process_list);
}

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



void execute(cmdLine* pCmdLine,int debug,process** processList)
{
    int chdirRes = 0,childID,res,pidOfProcess,
    procs = strncmp("procs",pCmdLine->arguments[0],5),
    cd = strncmp("cd",pCmdLine->arguments[0],2),
    suspend = strncmp("suspend",pCmdLine->arguments[0],7),
    killCommand = strncmp("kill",pCmdLine->arguments[0],4);
    if(procs == 0) // if we recieved procs command
    {
        printProcessList(processList);
        freeCmdLines(pCmdLine);
    }
    else if(cd == 0) //  if we recieved cd command
    {
        chdirRes = chdir(pCmdLine -> arguments[1]);
        if(chdirRes == -1)
        {
            fprintf(stderr,"error : %d , while trying cd command",errno);
        }
        freeCmdLines(pCmdLine);

    }
    else if(killCommand == 0) // if we recived kill comamnd
    {
        pidOfProcess = atoi(pCmdLine -> arguments[1]);
        printf("sending signal: SIGINT    to: %d\n",pidOfProcess);
        kill(pidOfProcess,SIGINT);
    }
    else
    {    
        childID = fork();
        if(childID == 0)
        {
            if(suspend == 0) // if we recieved suspend command
            {
                pidOfProcess = atoi(pCmdLine -> arguments[1]);
                printf("sending signal: SIGTSTP    to: %d\n",pidOfProcess);
                kill(pidOfProcess,SIGTSTP); // send sigtstp to the process
                sleep(atoi(pCmdLine -> arguments[2])); // sleep for t seconds
                printf("sending signal: SIGCONT    to: %d\n",pidOfProcess);
                kill(pidOfProcess,SIGCONT);
            }
            else
            {
                res = execvp(pCmdLine->arguments[0],pCmdLine -> arguments);
                if(res == -1)
                {
                    perror("error!!!!");
                    freeProcessList(*processList); 
                    freeCmdLines(pCmdLine); 
                    _exit(EXIT_FAILURE);
                        
                }
                if(debug)
                {
                    printDebug(pCmdLine -> arguments[0]);
                } 
                
            }  
            _exit(EXIT_SUCCESS);
        }
        if(pCmdLine ->blocking == 1 && suspend != 0) // if we at suspend to be able to perfrom actions while sleeping
            waitpid(childID,NULL,0);
        if(suspend != 0) // to not add suspend command
            addProcess(processList,pCmdLine,childID);
    }
}

void clearBuf(char* buf)
{
    memset(buf,'\0',2048);
}

int main(int argc,char** argv)
{
    const int inputMaxSize = 2048;
    int i,debug = 0;
    FILE* outpout = stdout;
    FILE* input = stdin;
    cmdLine* command;
    char inputFromUser[inputMaxSize];
    char workingDir[PATH_MAX];
    process* processList = NULL;
    signal(SIGINT,handler);
    signal(SIGTSTP,handler);
    signal(SIGCONT,handler);
    char* res;
    while(1)
    {
        for(i=1;i<argc;i++)
        {
            if(argv[i][1] == 'd')
                debug = 1;
        }

        res = getcwd(workingDir,PATH_MAX);
        if(errno == ERANGE)
        {   
            printf("%s", "error when reading current directory name");
        }
        fprintf(stdout,"%s\n",workingDir);
        fgets(inputFromUser,inputMaxSize,input);

        if(strstr(inputFromUser,"quit") != NULL)
        {
            freeProcessList(processList);
            break;
        }
        command = parseCmdLines(inputFromUser);
        execute(command,debug,&processList);
    }
    return 0;
}