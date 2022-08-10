#define _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>

/* DEFINING THE MAXIMUM ARGUMENTS, BUILT-IN ARGUMENTS
 * MAXIMUM BACKGROUND PROCESSES */
#define MAXCMDLEN 2048
#define MAXARGS 512
#define STATUSPROMPT "status"
#define EXITPROMPT "exit"
#define CD "cd"
#define MAXBACKGROUND 1000

// DEFINES THE SIGNAL STRUCT AND FLAG FOR SIGTSTP
volatile sig_atomic_t backMode = 1;
struct commandLineStruct {
  char* command;
  char* arguments[MAXARGS];
  char* input_file;
  char* output_file;
  bool  background;
};

/* EXIT PROGRAM FUNCTION:
 *  int* backgroundArr:   array holding PIDs of active background processes;
 *  int  backTrack:       counter of background processes;
 */
int exit_prog(int* backgroundArr, int backTrack) {
  for(int n = 0; n < backTrack; n++){
    kill(backgroundArr[n], SIGTERM);
  }
  exit(0);
}

/* GET STATUS FUNCTION:
 *  int statusTrack:  counter of background processes;
 */
int get_status(int statusTrack) {
  printf("exited with status %d\n", statusTrack);
  return 0;
}

/* CHECK EXPANSION FUNCTION:
 *  char* command:  the command that was entered in the cmd line;
 */
bool check_expansion(char* command){
  char* expandToken = "$";
  int expandCount = 0;

  // ITERATES OVER THE COMMAND STRING CHECKING FOR EXPAND TOKEN
  for(size_t i = 0; i < (strlen(command));) {
    
    // CHECKS FOR FIRST "$"
    if (*(command + i) == *expandToken) {

      // CHECKS FOR THE SECOND "$"
      if (*(command + i + 1) == *expandToken) {
        expandCount += 1;
        size_t j = 0;

        /* IF THIS IS THE FIRST EXPANSION, WRITE THE COMMAND INPUT
         * FROM START TO ONE LESS THAN INDEX WHERE FIRST "$" WAS FOUND
         */
        if (expandCount == 1) {
          while (j < i) {
            printf("%c", *(command + j));
            j++;
          }
          printf("%d", getpid());
          // MOVES UP INDEX 2 TO SKIP "$$"
          i += 2;
          continue;
        }
        else {
          // PRINT THE PID
          printf("%d", getpid());
          i += 2;
          continue;
        }
      }
      // ONLY CONTINUE PRINTING IF THERE HAS ALREADY BEEN "$$" FOUND
      else if (expandCount > 0) {
        printf("%c", *(command+i));
        i++;
      }
      // OTHERWISE MOVE UP THE INDEX OF CMDLINE
      else { 
        i++; 
        continue;
      }
    }
    /* PRINTS ONLY IF THERE HAS ALREADY BEEN "$$" FOUND:
     *   this case is on the outside loop and does not
     *   depend on a first "$" being found. */
    else if (expandCount > 0) {
      printf("%c", *(command+i));
      i++;
    }
    else { i++; }
  }
  // PRINTS A NEWLINE IF NECESSARY
  if (expandCount > 0){
    printf("\n");
    fflush(stdout);
    return 0;
  } else {
    return 0;
  }
}

/* CHANGE DIRECTORY FUNCTION:
 *  char* path:     path to target directory;
 *  int   argTrack: counter of arguments in the argument array;
 */
int change_directory(char* path, int argTrack) {
  // CHANGES TO HOME DIRECTORY IN HOME PATH VAR IF ONLY "CD" ENTERED;
  if (argTrack < 2) {
    char* homePath = getenv("HOME");
    int cdsuccess = chdir(homePath);
    if (cdsuccess == -1) {
      perror("Error in changing directory");
      return 1;
    }
    return 0;
  // CHANGES TO DIRECTORY FOUND IN ARGUMENTS[1]
  } else { 
    int cdsuccess = chdir(path);
    if (cdsuccess == -1){
      perror("Error in changing directory");
      return 1;
    }
    return 0;
  }
}

/* FUNCTION HANDLING NON-BUILT-IN COMMANDS:
 *  char*  command:   command that was given in cmd line;
 *  char** arguments: arguments that will be given to execvp;
 *  int    argTrack:  counter of arguments given;
 */
int nonbuiltin(char* command, char** arguments, int argTrack) {
  // ADDS THE COMMAND AND NULL TO THE ARGUMENTS ARRAY
  arguments[0] = command;
  arguments[argTrack] = NULL;
  execvp(command, arguments);
  perror("execv");
  exit(EXIT_FAILURE);
}

/* REDIRECT INPUT FUNCTION:
 *  char* input_file: input file given in cmd line;
 */
int redirect_input(char* input_file){
  int sourceFD = open(input_file, O_RDONLY, 0440);
  if (sourceFD == -1){
    perror("open input file failed");
    exit(1);
  }

  // REDIRECT INPUT TO NEW FILE DISCRIPTOR
  int result = dup2(sourceFD, 0);
  if (result == -1) {
    perror("source dup2() failed");
    exit(2);
  }
  close(sourceFD);
  return 0;
}

/* REDIRECT OUTPUT FUNCTION:
 *  char* output_file: string containing name of output file;
 */
int redirect_output(char* output_file){
  int targetFD = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
  if (targetFD == -1) {
    perror("open output file failed");
    exit(1);
  }
  // REDIRECT OUTPUT TO NEW FILE DISCRIPTOR
  int result = dup2(targetFD, 1);
  if (result == -1) {
    perror("source dup2() failed");
    exit(2);
  }
  close(targetFD);
  return 0;
}

/* HANDLER FOR SIGINT:
 *  int signo:  signal number;
 */
void SIGINT_handler(int signo) {
  exit(signo);
  return;
}

/* HANDLE WHEN SIGTSTP ENTERS FOREGROUND-ONLY MODE:
 *  int signo: signal number;
 */
void SIGTSTP_handler_to0(int signo) {
  // CHANGES BACKMODE TO 0 AND WRITES MESSAGE
  backMode = 0;
  char* message = "\nEntering foreground-only mode (& is now ignored)\n";
  write(STDOUT_FILENO, message, 50);
  return;
}

/* HANDLE WHEN SIGTSTP EXITS FOREGROUND-ONLY MODE:
 *  int signo:  signal number;
 */
void SIGTSTP_handler_to1(int signo){
  // CHANGES BACKMODE TO 1 AND WRITES MESSAGE
  backMode = 1;
  char* message = "\nExiting foreground-only mode\n";
  write(STDOUT_FILENO, message, 30);
  return;
}

// START OF MAIN FUNCTION
int main(void) {

  // DEFINES SIGNAL STRUCTS
  struct sigaction ignore_action = {0};
  struct sigaction SIGTSTP_action = {0};
  struct sigaction SIGINT_handler = {0};

  // PARENT PROCESS IGNORES SIGINT
  ignore_action.sa_handler = SIG_IGN;
  sigaction(SIGINT, &ignore_action, NULL);

  // FILLS SIGTSTP_action STRUCT WITH PARAMETERS
  sigfillset(&SIGTSTP_action.sa_mask);
  SIGTSTP_action.sa_flags = 0;
  SIGTSTP_action.sa_handler = SIGTSTP_handler_to0;

  // REGISTERS SIGTSTP WITH SIGTSTP_action STRUCT
  sigaction(SIGTSTP, &SIGTSTP_action, NULL); 

  // ALLOCATE MEMORY FOR BACKGROUND ARRAY
  int* backgroundArr = malloc(sizeof(int) * MAXBACKGROUND);

  // INITIALIZES PIDS AND TRACKER VARIABLES
  int childPid = 0;
  pid_t spawnPid = -2;
  int statusTrack = 0;
  int backTrack = 0;

  // ALLOCATE HEAP MEMORY FOR CMD LINE INPUT
  char* userInputPtr = calloc(MAXCMDLEN, sizeof(char));
  
  // THIS LOOP ENSURES THE USER GETS RE-PROMPTED
  while(1){
    
    /* THIS LOOP ITERATES THROUGH BACKGROUND ARRAY TO REAP ANY FINISHED
     *  CHILD PROCESSES
     */
    for (int n = 0; n < backTrack; n++){
      if (backgroundArr[n] == 0){
        continue;
      }
      childPid = waitpid(backgroundArr[n], &statusTrack, WNOHANG);
      if (childPid != 0) {
        if (childPid == -1){
          perror("Error on waitpid()");
          exit(1);
        }
        if(WIFEXITED(statusTrack)){
          // IF CHILD PROCESS WAS REAPED, TAKE IT OUT OF BACKGROUND ARRAY
          backgroundArr[n] = 0;
          backTrack--;
          statusTrack = WEXITSTATUS(statusTrack);
          printf("background pid %d is done: exit value %d\n", childPid, statusTrack);
          fflush(stdout);
        } else {
          // IF CHILD WAS TERMINATED BY SIGNAL, DISPLAY MESSAGE AND TAKE IT OUT OF ARRAY
          statusTrack = WTERMSIG(statusTrack);
          printf("background pid %d is done: terminated by signal %d\n", childPid, statusTrack);
          fflush(stdout);
          backgroundArr[n] = 0;
          backTrack--;
        }
      }
    }

    // REGISTER THE SIGNAL HANDLER FOR SIGTSTP THAT WILL FLIP FLAG IF CALLED
    if(backMode == 0){
      SIGTSTP_action.sa_handler = SIGTSTP_handler_to1;
    } else if (backMode == 1){
      SIGTSTP_action.sa_handler = SIGTSTP_handler_to0;
    }
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    
    // PRINT PROMPT FOR USER AND COLLECTS INPUT
    memset(userInputPtr, 0, sizeof(userInputPtr)); 
    printf(":");
    fflush(stdout);
    fgets(userInputPtr, MAXCMDLEN, stdin);

    /* IGNORE A BLANK LINE AND COMMENT LINE: 
     *  userInputPtr will have only newline character */
    if (strlen(userInputPtr) <= 1){
      continue;
    }
    char* commentToken = "#";
    if (strncmp(userInputPtr, commentToken, 1) == 0) {
      continue;
    }

    // CREATE AND STORE PARAMETERS IN A STRUCT THAT HOLD IMPORTANT CMD LINE VALUES
    struct commandLineStruct userEntry;
    userEntry.command = strtok(userInputPtr, " \n");
    userEntry.arguments[0] = userEntry.command;
    userEntry.input_file = NULL;
    userEntry.output_file = NULL;
    userEntry.background = false;
    char* token;
    int argTrack = 1;

    // TOKENIZE EACH CMD LINE STRING
    while ((token = strtok(NULL, " \n")) != NULL) {

      if (strcmp(token, "&") == 0) {

        // CHECKS IF "&" WAS AT VERY END OF CMD LINE INPUT
        if ((token = strtok(NULL, " \n")) == NULL) {
          // IF BACKGROUND MODE IS TRUE, SET FLAG IN STRUCT
          if (backMode == 1){
            userEntry.background = true;
            break;
          } else {
            // DOES NOT SET BACKGROUND MODE AND IGNORES "&"
            break;
          }
        } else {
          // IF & IS NOT AT END OF CMD LINE INPUT, ADDS IT TO ARGUMENTS
          userEntry.arguments[argTrack] = "&";
          argTrack++;
        }
      }

      // ADDS REDIRECT FILES TO STRUCT IF NECESSARY
      if (strcmp(token, "<") == 0) {
        userEntry.input_file = strtok(NULL, " \n");
        continue;
      }
      if (strcmp(token, ">") == 0) {
        userEntry.output_file = strtok(NULL, " \n");
        continue;
      } else {

        // REPLACES "$$" WITH PID IF GIVEN AS AN ARGUMENT
        char* expandTokens = "$$";
        if (strcmp(token, expandTokens) == 0) {
          /* CONVERTS PID TO A STRING AND STORES INTO ARGUMENT ARRAY
           *  Citation for code lines 366 - 370:
           *  Date: 5/9/2022
           *  Based on: https://edstem.org/us/courses/21025/idscussion/1437434
           */
          pid_t pid = getpid();
          char* pidstr;
          int n = snprintf(NULL, 0, "%jd", pid);
          pidstr = malloc((n+1) * sizeof *pidstr);
          sprintf(pidstr,"%jd", pid);

          userEntry.arguments[argTrack] = pidstr;
          argTrack++;
        } else {
          userEntry.arguments[argTrack] = token;
          argTrack++;
        }
      }
    }

    // IF REPLACED INSTANCES OF "$$" IN THE COMMAND WITH PID OF SHELL, REPROMPT
    if (check_expansion(userEntry.command)){
      continue;
    }
 
    // IF COMMAND IS STATUSPROMPT, DISPLAY LAST EXIT STATUS / TERMINATING SIGNAL
    else if (strcmp(userEntry.command, STATUSPROMPT) == 0) {
      get_status(statusTrack);
      fflush(stdout);
      continue;
    }

    // IF COMMAND IS EXITPROMPT, KILL ALL PROCESSES AND TERMINATE SHELL
    else if (strcmp(userEntry.command, EXITPROMPT) == 0) {
      exit_prog(backgroundArr, backTrack);
    }
    
    // IF COMMAND IS CD, CHANGES DIRECTORY
    else if (strcmp(userEntry.command, CD) == 0) {
      change_directory(userEntry.arguments[1], argTrack);
      continue;
    }

    else {
      // SPAWNS NEW CHILD PROCESS TO RUN BUILD-IN COMMAND
      spawnPid = fork();
      if (spawnPid == -1) {
        perror("fork() failed");
        exit(1);
      } else if (spawnPid == 0){
       
        // IF IN CHILD PROCESS, REGISTER CORRECT HANDLER STRUCTS
        sigaction(SIGTSTP, &ignore_action, NULL);
        sigaction(SIGINT, &SIGINT_handler, NULL);

        // REDIRECTS INPUT AND OUTPUT
        // IF NO REDIRECTS, AND RUNNING IN BACKGROUND, POINT STDIN AND STDOUT TO NULL
        char* nullBin = "/dev/null";
        if (userEntry.input_file != NULL) {
          redirect_input(userEntry.input_file);
        } else if (userEntry.background == true){
          redirect_input(nullBin);
        }
        if (userEntry.output_file != NULL) {
          redirect_output(userEntry.output_file);
        } else if (userEntry.background == true){
          redirect_output(nullBin);
        }
        
        // RUNS NON-BUILD-IN COMMAND
        nonbuiltin(userEntry.command, userEntry.arguments, argTrack);
        perror("execution failed");
        exit(EXIT_FAILURE);
      } else {

        // REGISTERS CORRECT SIGNAL STRUCTS FOR PARENT PROCESS
        sigaction(SIGTSTP, &SIGTSTP_action, NULL);
        sigaction(SIGINT, &ignore_action, NULL);

        // DOES NOT WAIT FOR BACKGROUND PROCESS IF FLAG IS TRUE
        if (userEntry.background == true) {
         printf("background pid is %d\n", spawnPid);
         // ADDS BACKGROUND PID TO BACKGROUND ARRAY
         backgroundArr[backTrack] = spawnPid;
         backTrack++;
         continue;
        }
        
        // PARENT DOES WAIT FOR CHILD IF BACKGROUND FLAG IS FALSE
        childPid = waitpid(spawnPid, &statusTrack, 0);

        // SETS STATUS SIGNAL FOR FOREGROUND PROCESS
        if(WIFEXITED(statusTrack)){
          statusTrack = WEXITSTATUS(statusTrack);
        } else {
          statusTrack = WTERMSIG(statusTrack);
          if (statusTrack == 2) {
            printf("terminated by signal 2\n");
            fflush(stdout);
          }
        }
        continue;
      }
    }
  }
}
