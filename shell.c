/*
  Copyright Heydar Gerayzade 2017
  
  Shell Reloaded
  
  Credits to Malcolm X and linux.die.net
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <pwd.h>
#include <time.h>
#include <dirent.h>
#include <signal.h>
#include <wait.h>

/* file descriptors for i/o */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1

/* font styles */
#define BOLD "\033[1m"
#define BLUE "\033[94m"
#define RED "\033[91m"
#define YELLOW "\033[93m"
#define DARKCYAN "\033[36m"
#define GREEN "\033[92m"
#define PURPLE "\033[95m"
#define CYAN "\033[96m"
#define BOLDGREEN "\033[1m\033[92m"
#define BOLDBLUE "\033[1m\033[94m"
#define RESET "\033[0m"

/* splitting the string into char array */
char splitStr(char array, int arrSpace, char * str, char * div, int lastNull) {
  char * tokenizator;
  tokenizator = strtok(str, div);
  while (tokenizator != NULL) {
    array[arrSpace] = tokenizator;
    arrSpace++;
    tokenizator = strtok(NULL, div);
  }
  if (lastNull == 1) {
    array[arrSpace] = tokenizator;
    array = realloc(array, sizeof(char*) * arrSpace);
  }

  return array;
}

/* getting number of elements in char array */
size_t elementsNum(char ** array) {
  size_t i;
  for (i = 0; array[i]; i++);
  return i;
}

/* signal handling */
pid_t pid;
int exitStatus = 0;
int commandId = 0;

static void handler_sigmsg(int signum) {
  if(signum == SIGCHLD) {
    waitpid(pid, &exitStatus, 0);
    if(WEXITSTATUS(exitStatus) == 0) {
      fprintf(stderr, YELLOW"Process %d is done\n"RESET, commandId);
    }
  }
}

/* Backbone of the shell */
int sh() {  
  bool ifExit = false;
    
  /* initial directory (where shell is located) */
  char shellDir[512];
  getcwd(shellDir, sizeof(shellDir));
 
  /* getting some enviroment variables */
  char * envUser = getenv("USER");
  char * envPath = getenv("PATH");    
  char ** envDirs = malloc(1024);
  envDirs[0] = "";
  envDirs = splitStr(envDirs, 1, envPath, ":", 0);

  /* here comes the magic */
  while(!ifExit){
    /* Decorating command line */
    char * commandLine = malloc(512);
    char curDir[512];
    getcwd(curDir, sizeof(curDir));
    sprintf(commandLine, BOLDGREEN"%s@shell"RESET":"BOLDBLUE"%s"RESET"$ ", envUser, curDir);
    write(STDOUT_FILENO, commandLine, strlen(commandLine));
    
    /* new command */
    char tempCommand[512];
    char * command = malloc(1024);
    /* read next line if "\" is found in the end of current one */
    while(fgets(tempCommand, 512, stdin)) {
      if(tempCommand[strlen(tempCommand) - 2] == '\\') {
        write(STDOUT_FILENO, "> ", 2);
        tempCommand[strlen(tempCommand) - 2] = '\0';
        sprintf(command, "%s%s", command, tempCommand);
      } else {
        sprintf(command, "%s%s", command, tempCommand);
        break;
      }
    }
    /* removing "\n" */
    for(int i = 0; i < strlen(command); i++) {
      if(command[i] == '\n') command[i] = '\0';
    }
   
    /* separating commands with "&&" */
    char ** superSet = malloc(1024);
    superSet = splitStr(superSet, 0, command, "&&", 0);
      
    for (int i = 0; i < elementsNum(superSet); i++) {
        
      commandId++;
      
      if(!strcmp(superSet[i], "exit")) {
        /* end process */
        exit(0);
        
        break;
      } else {
        bool emptyCmd = false;
        if (!strcmp(superSet[i], "")) emptyCmd = true;
     
        bool cmdNotFound = false;
      
        /* getting arguments from the command */
        char * binFile = malloc(1024);
        char ** cmdParts = malloc(1024);
        cmdParts = splitStr(cmdParts, 0, superSet[i], " ", 1);
        
        if (!emptyCmd) {
          /* searching for the executable file */
          for (int j = 0; j < elementsNum(envDirs); j++) {
            char * dirSlash;
            dirSlash = "/";
            if(j == 0) dirSlash = "";
            sprintf(binFile, "%s%s%s", envDirs[j], dirSlash, cmdParts[0]);

            FILE *fp = fopen(binFile, "r");
            if(fp) { 
              fclose(fp);
              cmdNotFound = false;

              break;
            } else if(strcmp(cmdParts[0], "cd")) { 
              cmdNotFound = true;
            }
          }
        }
        
        if(cmdNotFound) { 
          /* command not found */
          fprintf(stderr, "shell: %d: %s: command not found\n", commandId, superSet[i]);
          exit(127);
        } else {   
          /* new process */
          bool cmdSuccess = true;
          
          if (!strcmp(cmdParts[0], "cd")) {
            if(chdir(cmdParts[1]) < 0) { 
              cmdSuccess = false;
              fprintf(stderr, "shell: %d: %s: %s: No such file or directory\n", commandId, cmdParts[0], cmdParts[1]);
              exit(1);
            }
          } else {
            pid = fork();
            signal(SIGCHLD, handler_sigmsg);
            if (pid == 0) { 
              /* child process */
              if(execv(binFile, cmdParts) < 0) { 
                cmdSuccess = false;
                fprintf(stderr, "shell: %d: %s: Something went wrong\n", commandId, cmdParts[0]);
                exit(-1);
              }
            } else { 
              /* parent process */
              int waitst;
              waitpid(pid, &waitst, 0);
              /* terminating the sequence of commands in case of an error */
              if(waitst != 0) exit(WEXITSTATUS(waitst));
              exitStatus = WEXITSTATUS(waitst); 
            }
          }
        }   
      }
    }
  }

  /* return with exit status */
  return WEXITSTATUS(exitStatus);
}

/* main function */
int main(int argc, char const *argv[]) {
  /* intro quote */
  char * quote = malloc(300 * sizeof(char));
  quote = "\n"BOLDGREEN"###############################################"RESET"\n"BOLDGREEN"### Nobody can give you freedom ###############"RESET"\n"BOLDGREEN"### Nobody can give you equality or justice ###"RESET"\n"BOLDGREEN"### If you are a man, you take it #############"RESET"\n"BOLDGREEN"####### -Malcolm X- ###########################"RESET"\n\n";

  write(STDOUT_FILENO, quote, strlen(quote));
  /* start shelling */
  return sh();
}