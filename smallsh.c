/*--------------------------------------------------------------------------
Student: Michael Kochka
Assignment2: smallsh
username: kochkam
Date Due: 7/21/2020
Description: This program mirrors the bash shell but with limited functionality.
The cd, exit and status commands are built in and all other commands are run
using execlp or execvp within child process. Signal handlers have been defined
both for SIGINT and SIGTSTP for the parent process. These handlers are 
ignored by the children. This shell has the capability to redirect output and 
also run process in the background and the foreground and tracking their
exit status. 
----------------------------------------------------------------------------*/
#define _GNU_SOURCE
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>

int childStatus = 0; //track child status
pid_t backPid = 0; //track backround PID
int backStatus = 0; //track background process status
int runBackground = 0; //specify if new process should be 1 or 0 : 1 meaning a process should run in background
pid_t foregroundPID = -2;//tracks PID of latestest foregroundPID
pid_t idArray[50]; 
int STOP_MODE = 0; 
int IGNORE_BACKGROUND = 0; 
char *argv[508];

/***************************************************************************************
 * checkString(): Examine if string has $$ sign and then expand variable with pid
 * 
 * 
*****************************************************************************************/
char * checkString(char * string){
    char variable[2048];
    char Tempstring[10];
    char * newString; 
    int counter = 0; 
    pid_t expansion = getpid(); //get pid
    strcpy(variable, string); //copy pointer into array

    for (int i = 0; i < sizeof(variable); i++) //search for $$ sign and track occurences
    {
        if (variable[i] == '$')
        {
            counter++; 
            if (counter == 2) //if counter equals two break
            {
                break;
            }  
        }
        if(variable[i] == '\0') //break if null terminatro encounterd
            break; 
    }

    if(counter == 2) //if $$ occured twice in string 
    {

        sprintf(Tempstring, "%d", expansion);//write output from expansion to string
        for (int i = 0; i < sizeof(variable); i++) //replace $$ with null terminator
        {
            if(variable[i] == '$')
                variable[i] = '\0';
            if(variable[i] == '\0')
                break;   
        }
        strcat(variable, Tempstring);//add PID to string 
        newString = variable; 
        return newString;
    }
    else
        return string;

}


/***************************************************************************************
 * checkSignals: checks if background process terminated and displays info about process
 * 
 * 
*****************************************************************************************/
void checkSignals()
{
    for (int i = 0; i < 50; i++) //read idarray
    {
        if (idArray[i] != 0) //if id is not 0
        {
            backPid = idArray[i]; //put id in pid variable

            if (waitpid(backPid, &backStatus, WNOHANG) != 0) //check if state has changed
            {

                printf("background pid %d is done: teminated by signal %d\n", backPid, backStatus);
                fflush(stdout);
                //fflush(stdin);
                idArray[i] = 0; //replace id with 0 as precess has terminated
            }
        }
    }


}

/***************************************************************************************
 * 
 * getEntry: get entry from user 
 * 
*****************************************************************************************/
char * getEntry(){

    char entry[2048];
    char * pointer = entry; 
    int dumb; 
    int length; 
    printf(":"); //prompt user
    //fflush(stdout);
    fgets (entry, 2048, stdin); //get input
    length = sizeof(entry);   //Sources Cited: https://cboard.cprogramming.com/c-programming/70320-how-remove-newline-string.html#:~:text=All%20you%20have%20to%20do,last%20character%20is%20a%20newline.&text=char%20str%5B80%5D%3B%20int,2005%20at%2004%3A33%20AM.
    for (int i = 0; i <= length; i++)
    {
        if (entry[i] == '\n') //remove newline and replace it with null terminator 
        {
            entry[i] = '\0';
            break; 
        }
    }
    
    return pointer; // return the pointer 
}
/***************************************************************************************
 * builtCommand(): determines what command to execute if it is a built in command
 * for this program. 
 * 
*****************************************************************************************/
int builtCommand(char * something)
{
    char altTerm[] = "exit";
    char newChange[] = "cd";
    char newStat[] = "status";
    int commandType  = 0;
    char tempString[2048];
    strcpy(tempString, something); //copy the users input into the string 
    char * temp = strtok(tempString, " "); //get first arguement and check if it is one of the following 
    if(strcmp(temp, altTerm) == 0) //check if exit
        commandType = 0; 
    else if(strcmp(temp, newChange) == 0) //check if cd
        commandType = 1; 
    else if(strcmp(temp, newStat) == 0) //check if status
        commandType = 2; 
    else if (tempString[0] == '#') //check if comment 
        commandType = 3;
    else
        commandType = 4; //else it is not a built in command
    
    return commandType;
}
/***************************************************************************************
 * terminate(): terminates the shell 
 * 
 * 
*****************************************************************************************/
void terminate()
{
     for (int i = 0; i < 50; i++) //read idarray
    {
        if (idArray[i] != 0) //if id is not 0
        {
            backPid = idArray[i]; //put id in pid variable

            waitpid(backPid, &backStatus,0); //check if state has changed
        }
    }
    exit(0); //terminate shell 
}
/***************************************************************************************
 * changeDir(): change the directory to directory specified by user 
 * 
 * 
*****************************************************************************************/
void changeDir(char * entry){
    char change[] = "cd";
    int open = 0; 
    char newString[2048];
    strcpy(newString, checkString(entry)); 
    if(strcmp(newString, change) == 0)
    {
        char * home = getenv("HOME"); //get home directory 
        open = chdir(home); //change directory to home

    }
    else
    {
        entry = strtok(newString, " ");
        entry = strtok(NULL, " "); //get filename
        open = chdir(entry); //change directory to specified name
    }
    
}
/***************************************************************************************
 * status(): determines the exit status of the last exited process 
 * 
 * 
*****************************************************************************************/
void status(){

    if(WIFEXITED(childStatus)) //determine if process exited normally 
    {
        childStatus = WEXITSTATUS(childStatus); //get exit status of child 
        printf("exit value %d\n", childStatus);
        fflush(stdin);
        fflush(stdout);
    }
    else
    {
        childStatus = WTERMSIG(childStatus);//determine what signal chaused the child process to exit
        printf("exit value %d\n", childStatus);
        fflush(stdin);
        fflush(stdout);
    }

}
/***************************************************************************************
 * noBuiltCommand(): Run child process specified by user direct input and output
 * 
 * 
*****************************************************************************************/
int getArgs(char * String, char * File){
    int i = 1; 
    int args; 
    argv[0] = File; //get file name on path
    char * parser = strtok(String, " "); //get first arg
    while (parser != NULL) //Sources Cited:https://stackoverflow.com/questions/27541910/how-to-use-execvp
    {
        argv[i] = parser;
        parser = strtok(NULL, " ");
        i++;
    }
    argv[i] = NULL;//put null terminator at the end of array
    args = i; 
    return args; 
    
} 

/***************************************************************************************
 * noBuiltCommand(): Run child process specified by user direct input and output
 * 
 * 
*****************************************************************************************/
  
void nonBuiltCommand(char * var){

    char *temp; //temp pointer to hold a string temporarily 
    char search[2048]; //array to hold strings 
    char newS[2048];
    char * output = NULL; //hold char string to open file for output 
    char * input = NULL; //hold char string to open file for input 
    int targetFD; // hold file descriptor  
    int altFD; // hold file descriptor  
    int result; //hold result of dup2()
    int saveFD; //save orginal File descriptor before modifying 
    int altSaveFD; //save orginal File descriptor before modifying 
    int altresult; //hold result of dup2()
    int childPID;  //child process ID 
    int count = 0; 
    int args = 0; 

    runBackground = 0; //reset runbackground process to 0 commands default to foreground unless specified with &
    foregroundPID = -2; // reset foregroundPID to -2 

    strcpy(newS, checkString(var)); //check string for $$ to expand symbols to PID
    strcpy(search, newS); //copy pointer to array 


    int length = sizeof(search);
    for (int i = 0; i < length; i++) //determine if & background operator exists 
    {
        if (search[i] == '&')
        {
            search[i - 1] = '\0'; //insert null terminatory at & and before in the " "
            search[i] = '\0';
            if(IGNORE_BACKGROUND == 0)
                runBackground = 1; //set runBackground to one to indicate process should be run in background and not waitpid()
            break;

        }
        if (search[i + 1] == '\0') //if null terminator is next character break out of loop 
            break;
    }

    temp = strtok(search, " "); //get command
    var = strtok(NULL, "\0"); //get arguement

    if (var != NULL) //if an argument exists in string do this
    {
        output = strstr(var, ">"); //if this character exits intialize output var
        input = strstr(var, "<");  //if this character exits intialize input var

        if (input == NULL && output == NULL)
        {
            args = getArgs(var, temp); //getargs from string and add them to array
        }
    }
    else
    {
        argv[0] = NULL;
    }
    
    pid_t spawnPid = fork(); //fork 

    switch (spawnPid)
    {
    case -1:
        perror("Error:fork()\n"); //if fork fails
        exit(1);
        break;
    case 0:
        // In the child process
        
        if (runBackground == 1)
        {
            backPid = getpid(); //getpid and store in background PID variable 
            signal(SIGINT, SIG_IGN); //ignore signal if SIGINT
            signal(SIGTSTP, SIG_IGN);//ignore signal if SIGTSTP
        }
        if (runBackground == 0)
            signal(SIGTSTP, SIG_IGN); //ignore signal if SIGTSTP


        if (output != NULL && input != NULL) //if input file and ouput file is specified do this
        {
            output = strtok(output, " ");
            output = strtok(NULL, "\0"); //get input and output files to open the appropriate files based on arguements
            input = strtok(input, " ");
            input = strtok(NULL, " ");
            saveFD = dup(STDIN_FILENO); //initialzed saveFD with value 0
            altSaveFD = dup(STDOUT_FILENO); //initialzed saveFD with value 1

            altFD = open(input, O_RDONLY);//open file for readonly in alt FD 
            if (altFD == -1) //throw error if this
            {
                perror("Error:open()");
                exit(1);
            }
            altresult = dup2(altFD, 0); //copy file descriptor
            if (altresult == -1) //throw error if -1
            {
                perror("Error:dup2");
                exit(1);
            }
            close(altFD); //close old file descriptor
            targetFD = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0664); //open file for writing

            if (targetFD == -1) //throw error if file does not exist
            {
                perror("Error:open()");
                exit(1);
            }
            result = dup2(targetFD, 1); //copy file descriptor 
            if (result == -1)
            {
                perror("Error:dup2");
                exit(1);
            }
            close(targetFD); //close old file descriptor

            
        }
        else if (output != NULL) //if output is specified 
        {
            output = strtok(output, " "); //get filename
            output = strtok(NULL, "\0");
            saveFD = dup(STDOUT_FILENO);
            targetFD = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0664); //open file
            if (targetFD == -1) //throw error if file does not exist
            {
                perror("Error:open()");
                exit(1);
            }
            result = dup2(targetFD, 1); //copy file descriptor 
            if (result == -1)
            {
                perror("Error:dup2");
                exit(1);
            }
            close(targetFD); //close old file descriptor 
        }
        else if (input != NULL) //if input is specified 
        {
            input = strtok(input, " "); //get file for input 
            input = strtok(NULL, "\0");
            saveFD = dup(STDIN_FILENO); //initalize file descriptor 
            targetFD = open(input, O_RDONLY);

            if (targetFD == -1) //target 
            {
                perror("Error:open()");
                exit(1);
            }
            result = dup2(targetFD, 0); // copy file descriptor
            if (result == -1)
            {
                perror("Error:dup2");
                exit(1);
            }
            close(targetFD); //close old file descriptor

        }
        if( runBackground == 1 && input == NULL && output == NULL) //if no output orinput is specified and background is specified
        {
            
            saveFD = dup(STDIN_FILENO); //create new FD for input
            targetFD = open("/dev/null", O_RDONLY); //direct input to null

             if (targetFD == -1) //if failed throw error
            {
                perror("Error:open()");
                exit(1);
            }
            result = dup2(targetFD, 0); //copy FD
            if (result == -1) //if failed throw error
            {
                perror("Error:dup2");
                exit(1);
            }
            close(targetFD); //close old FD

            altSaveFD = dup(STDOUT_FILENO); //save FD for OUTPUT 

            altFD = open("/dev/null", O_WRONLY); //open null for writing 
            if (altFD == -1) //throw error if failed to open
            {
                perror("Error:open()");
                exit(1);
            }
            altresult = dup2(altFD, 1); //copy FD
            if (altresult == -1)
            {
                perror("Error:dup2");
                exit(1);
            }
            close(altFD); //close old FD
            fflush(stdout);
        }

        if (output || input != NULL || argv[0] == NULL) //if input or output is specified
        {
            fflush(stdout);
            execlp(temp, temp, NULL); //execut file on path 
            perror("Error: execlp");
            exit(1);
        }
        else //no output or input was specified 
        {
            fflush(stdout);
            execvp(temp, argv); //execute file on path and 
            perror("Error:execvp");
            exit(1);

        }
        if(output != NULL && input != NULL) //if output and input were specified 
        {
            fflush(stdout);
            fflush(stdin); //flush input and output 
            close(result);
            close(altresult); //close file Desc
            dup2(altSaveFD, STDOUT_FILENO);
            dup2(saveFD, STDIN_FILENO); //restore original input and output FD
            close(saveFD); //close FD for reuse 
            close(altSaveFD);
        }
        else if(output != NULL) //if output was specifed 
        {
            fflush(stdout); //flush input and output
            fflush(stdin);
            close(result);  //close file Desc
            dup2(saveFD, STDOUT_FILENO); // Sources Cited: https://stackoverflow.com/questions/34945049/restoring-stdout-after-using-dup
            close(saveFD);  //close FD 
        }
        else if(input != NULL) //if input was specifed 
        {
            fflush(stdout); //flush input and output
            fflush(stdin);
            close(result); //close file Desc
            dup2(saveFD, STDIN_FILENO);
            close(saveFD); //close FD 
        }
        else if (runBackground == 1 && input == NULL && output == NULL) //if background process specifeied and no input or output was specified 
        {

            fflush(stdout);   //flush input and output
            fflush(stdin);
            close(result);
            dup2(saveFD, STDIN_FILENO); //restore orifinal FD
            close(saveFD); //close old FD
            close(altresult);
            dup2(altSaveFD, STDOUT_FILENO); //restore org FD 
            close(altSaveFD); //close old fd

        }
            // exec only returns if there is an error
        //exit(0); 
        break;
    
    default:
        // In the parent process
        temp = NULL;
        var = NULL; 
        if(runBackground == 0) //if background flag is not specified 
        {
            foregroundPID = spawnPid; 
            spawnPid = waitpid(spawnPid, &childStatus, 0); //wait for child process to terminate 
            //fflush(stdin); 
            fflush(stdout);
        }
        else
        {
            for(int i = 0; i < 50; i++) //read array for empty slot and put background id inside array
            {
                if(idArray[i] == 0)
                {
                    idArray[i] = spawnPid; //put spawnID in array 
                    break; 
                }
            }
            printf("began background process id: %d\n",spawnPid);//print id of background process
            fflush(stdout);
            //fflush(stdin);
        }
 
        break;
    }
}
/***************************************************************************************
 * handle_SIGINT(): specifies how to handle SIGINT signal 
 * 
 * 
*****************************************************************************************/
void handle_SIGINT(int signo){
    char *message = "\nTerminated by signal 2\n";

    if (foregroundPID != -2)
        if (waitpid(foregroundPID, &childStatus, WNOHANG) == 0) //kill foreground process
        {
            kill(foregroundPID, SIGTERM);
            write(STDOUT_FILENO, message, 27); //display terminating signal
            fflush(stdout);
            foregroundPID = -2;
        }
}
/***************************************************************************************
 * handle_SIGTSTP: specifies how to handle SIGTSTP which displays forground mode
 * and toggles background &
 * 
*****************************************************************************************/
void handle_SIGTSTP(int signo){
    char *message = "\nEntering Foreground Only Mode(& is now ignored)\n";
    char *exit = "\nExiting Foreground Only Mode\n";

    if(STOP_MODE == 0)
    {
        write(STDOUT_FILENO, message, 52); //display  signal message
        fflush(stdout);
        STOP_MODE = 1; //trigger foreground only mode
        IGNORE_BACKGROUND = 1; 
    }
    else
    {
        write(STDOUT_FILENO, exit, 33); //display signal message 
        fflush(stdout);
        STOP_MODE = 0; //disable forground only mode
        IGNORE_BACKGROUND = 0; 
    }
    

}



int main()
{
    char userEntry[2048]; //holds user entry
    int selectCommand;    //selects command specified by user

    struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0}; //initialize handlers
    SIGINT_action.sa_handler = handle_SIGINT;                   //specify function to handle action
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;              //specify flags
    sigaction(SIGINT, &SIGINT_action, NULL); //set action

    SIGTSTP_action.sa_handler = handle_SIGTSTP; //specify function to handle action
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;               //specify flags
    sigaction(SIGTSTP, &SIGTSTP_action, NULL); //set action
    

    do
    {
        checkSignals();           //check if background process terminated and display info
        char *entry = getEntry(); //get user entry
        strcpy(userEntry, entry); //copy string
        char *selection = userEntry;
        selectCommand = builtCommand(entry); //get built in command from user

        if (selectCommand == 0) //terminate program
            terminate();
        else if (selectCommand == 1) //change directory specified by user
            changeDir(userEntry);
        else if (selectCommand == 2) //get status of latest terminated process
            status();
        else if (selectCommand == 3)
        {
            //do nothing because user entered comment
        }
        else
        {
            nonBuiltCommand(selection); //execute not specified command in child process
        }
    } while (selectCommand != 0);

    return 0;
}