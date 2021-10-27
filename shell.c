#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>  //for size_t
#include <sys/wait.h>   //for wait
#include <stdio.h>      //for input and output
#include <string.h>     //for string functions
#include <stdbool.h>    //for boolean
#include <fcntl.h>      //for O_CREAT...

#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_RESET    "\x1b[0m"
#define MAXLEN              80
#define MAXARGS             10

void navigateToUserDir();

void printPrompt();

void runBuiltInCommand(char**);

void runCommand(char**, int);

void redirCommand(char**, int, char*, bool);

void bgCommand(char**, int);
 
void pipeCommand(char**, int, int);


char workDir[MAXLEN]; // working directory
char displayDir[MAXLEN]; // directory as to be displayed
char hostName[20]; // hostname of system to be displayed at prompt
char usrDir[15]; // location of user directory, /~
size_t usrDirSize; //size of user directory


/**
 * urbanShell: A shell application to complete tasks as assigned by hw2.
 * Should be able to:
 * -1: Run built in commands cd & exit - works
 * -2: Run simple unix commands - works
 * -3: Run commands in the background using & - does not work, cannot figure
 *      out how to run exec in background
 * -4: Output redirection using > and >> - works
 * -5: Commands with a simple pipe - does not work yet, trying to work 
 *      implementation but I am not sure
 * 
 * @author Chase Urban
 */
int main(){

    char input[MAXLEN]; // user input line
    char *args[MAXARGS]; // tokenized array of strings from input
    char **arg; // pointer to beginning of args array, used for creation
    size_t argsCount; // length of args array
    char fileName[MAXLEN]; // filename, used for redirection
    size_t pipeAt; // location of pipe in string
    bool pipe = false, redir = false, append = false, bg = false; 
    // flags for execution

    gethostname(hostName, sizeof(hostName)); //gets host name
    //prints welcome message
    printf(" * Welcome to urbanShell v2.1\n * By Chase Urban, March 2021\n");
    //navigates to user directory
    navigateToUserDir();
    
    //infinite loop to catch more commands and execute
    while (true){       
        printPrompt(); // prints the prompt
        fgets(input, MAXLEN, stdin); // gets the user input line

        if (input[0] == '\n'){ // checks to see if input is null, else continue
            
        }
        else{

            //resets flags
            redir = false;
            append = false;
            pipe = false;

            arg = args; // resets arg location
            *arg++ = strtok(input, " \n"); // creates tokenizer
            while ((*arg++ = strtok(NULL, " \n"))); // tokenizes input to args

            // finds number of arguments
            argsCount = 0;
            for (size_t i = 0; i < MAXARGS; i++){
                    if(args[i] != NULL)
                        argsCount++;
                    else
                        break;
            }

            // checks for >, >>, or | to set flags
            for(int i = 0; i < argsCount; i++){
                if(strstr(args[i], ">") != 0){
                    redir = true;
                    strcpy(fileName, args[i+1]);
                    if (strstr(args[i], ">>") != 0){
                        append = true;
                    }
                    args[i+1] = NULL;
                    args[i] = NULL;
                    argsCount = argsCount - 2;
                }
                else if(strstr(args[i], "|") != 0){
                    pipeAt = i;
                    printf("pipe at %zu\\n", pipeAt);
                    pipe = true;
                }
            }

            // if exit or cd, runs built in command function
            if (strcmp(args[0],"exit") == 0 || strcmp(args[0], "cd") == 0){
                runBuiltInCommand(args);
            }
            else{
                if(redir){ // else if redirect flag set, runs redirect funct
                    redirCommand(args, argsCount, fileName, append);

                }
                else if(pipe){ // else if pipe flag, runs pipe function
                    pipeCommand(args, argsCount, pipeAt);
                }
                else{ // else runs as basic command
                    runCommand(args, argsCount);
                }
            }
        }
    }
    return 0; // should never be reached
}

/**
 * Navigates the proccesses working directory to the user directory
 */
void navigateToUserDir(){
    
    bool atHome = false;
    char testDir[MAXLEN];
    while (!atHome)
    {
        // goes to .., checks if current directory is /home/, as that is
        // common, then returns to last visited directory, which in theory
        // should be the user directory
        getcwd(workDir, sizeof(workDir));
        chdir("..");
        getcwd(testDir, sizeof(testDir));
        if (strlen(testDir) == 5)   
        {
            chdir(workDir);
            getcwd(usrDir, sizeof(usrDir));
            atHome = true;
        }
    }

}

/**
 * Prints the prompt, using colors and if at the user directory or a subfolder,
 * replaces /home/USER with ~
 */
void printPrompt(){
    printf(ANSI_COLOR_RED"urbanShell[on]%s"ANSI_COLOR_RESET,hostName);
    getcwd(workDir, sizeof(workDir));

    //if working dir is or begins with user dir, replaces user dir with ~ for
    //display purposes
    if(strncmp(workDir, usrDir, strlen(usrDir)) == 0){
        usrDirSize = strlen(usrDir); 
        strcpy(displayDir, "~");
        strncpy(&displayDir[1], &workDir[usrDirSize], MAXLEN-usrDirSize-1);
    }
    else
        strcpy(displayDir, workDir);

    printf(":"ANSI_COLOR_CYAN"%s"ANSI_COLOR_RESET"$ ", displayDir);
}

/**
 * Runs the built in commands of exit and cd, by using exit() and chdir()
 */
void runBuiltInCommand(char** arguments){

    if (strcmp(arguments[0], "exit") == 0){
        exit(0);
    }
    else if(strcmp(arguments[0], "cd") == 0){
        chdir(arguments[1]);
    }
    return;    

}

/**
 * forks and runs any command able to be found by PATH
 */
void runCommand(char** argv, int argc){

    if (fork() == 0){
        execvp(argv[0], argv);
        printf("%s: command not found\n", argv[0]);
        exit(0); //Runs only when execvp was not successful, kills child
    }

    wait(NULL);
}

/**
 * forks and redirects output of executed file to file specified
 */
void redirCommand(char** argv, int argc, char* fName, bool append){
    
    if (fork() == 0){
        int file;
        
        //if append flag is false, open a write only file
        if (!append){
            file = open(fName, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        }
        //if append flag is true, open a read/write file
        else{
            file = open(fName, O_CREAT | O_RDWR | O_APPEND, 0666);
        }

        close(1);
        dup(file);
        close(file);
        
        // runs the
        execvp(argv[0], argv);
        printf("%s: command not found\n", argv[0]);
        exit(0); //Runs only when execvp was not successful, kills child
    }
    wait(NULL);
    return;
}

/**
 * pipe command, does not work as it is not yet completed
 */
void pipeCommand(char** argv, int argc, int pipeAt){
    int fd[2];
    pipe(fd);

    if (fork() == 0){
        close(fd[0]);
        close(1);
        dup(fd[1]);
        //execvp
    }

}