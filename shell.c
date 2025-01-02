#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "array.h"
#include "errno.h"

int argAnzahl;
int stopWait = 0;

//READ CHARACTERS
#define LSH_RL_BUFSIZE 1024
char *lsh_read_line(void)
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}


//SPLIT CHARACTERS
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  argAnzahl = position;
  return tokens;
}
//##########################################EINLESEN ENDE


void sigHandler(int sig_num){
extern int stopWait;
stopWait=1;
}


void waitSig(char **args){
extern int argAnzahl;
extern int stopWait;
int status;

for(int i = 1; i < argAnzahl; i++){
  pid_t pid= atoi(args[i]);

  signal(SIGINT, sigHandler);
  if(stopWait==1){
    stopWait=0;
    break;
  }
  waitpid(pid, &status, 0);
        if (WIFEXITED(status)){
            printf("[%d] terminated normally.\nExit Status: %d \n", pid, WEXITSTATUS(status));
        }else if (WIFSIGNALED(status)){

            if (WTERMSIG(status) == SIGTERM){
                printf("[%d] got terminated \nExit Status: %d \n", pid, WEXITSTATUS(status));
                }

            if (WTERMSIG(status) == SIGKILL){
                printf("[%d] got killed \nExit Status: %d \n", pid, WEXITSTATUS(status));
                }
        }
}
}

void launchNormal(char **args){
  pid_t pid;

  pid = fork();
  if (pid == 0) {
    // Child process
    execvp(args[0], args);
  }
  //parent
  wait(NULL);
  }

  void launchAND(char **args){
  pid_t pid;

  pid = fork();
  if (pid == 0) {
    // Child process
    execvp(args[0], args);
    printf("Terminated with following error code: [%d]", errno);
  }
  //parent
  printf("[%d] \n", pid);
  sleep(1);
  }

  int checkPipe(char **args){
  extern int argAnzahl;
  for(int i=0; i < argAnzahl; i++){
    if(strcmp(args[i], "|")==0){
      return i;
    }
  }
  return 0;
  }


  void launchPipe(char **args, int pipeNumb){
  //Aufteilen des Eingabe Token in zwei Eingaben
  extern int argAnzahl;
  char *args1 [pipeNumb+1];
  char *args2 [argAnzahl-pipeNumb-1+1];

  for(int i =0; i < pipeNumb; i++) {args1[i]=args[i];}
  for(int j =0; j < argAnzahl-(pipeNumb+1); j++) {args2[j]=args[pipeNumb+1+j];}
  args1[pipeNumb]=NULL;
  args2[argAnzahl-pipeNumb-1]=NULL;
  
  //Erstellen einer Pipe mit Fehlermeldung
  int filedeskriptor[2];
  if(pipe(filedeskriptor)==-1){printf("error: pipe \n");}

  int pid1 = fork();
  if (pid1 == 0){
    //Kind 1
    dup2(filedeskriptor[1], STDOUT_FILENO);
    close(filedeskriptor[0]);
    close(filedeskriptor[1]);
    execvp(args1[0], args1);
  }
  
  int pid2 = fork();
    if (pid2 == 0){
    //Kind 2
    dup2(filedeskriptor[0], STDIN_FILENO);
    close(filedeskriptor[0]);
    close(filedeskriptor[1]);
    execvp(args2[0], args2);
  }

  //Schliessen der Pipe zum Elternprozess
  close(filedeskriptor[0]);
  close(filedeskriptor[1]);

  waitpid(pid1, NULL, 0);
  waitpid(pid2, NULL, 0);
  }


void launch(char **args){
extern int argAnzahl;
int pipe = checkPipe(args);

if(strcmp(args[argAnzahl -1], "&")==0){
  launchAND(args);
  }else if(pipe != 0){
  launchPipe(args, pipe);
  }else{
  launchNormal(args);
  }

}

int main(void) {

  char *line;
  char **args;

  while(1){

    //WD anzeigen (https://iq.opengenus.org/chdir-fchdir-getcwd-in-c/)
    char cwd[256];
    if (getcwd(cwd, sizeof(cwd)) == NULL){
      perror("getcwd() error");
    }else{
      printf("%s >", cwd);
    }

    //Eingabe einlesen (https://brennan.io/2015/01/16/write-a-shell-in-c/)
    line = lsh_read_line();
    args = lsh_split_line(line);

    //Initialisierung exit, cd, wait 
    if (args[0]== NULL){

    }else if (strcmp(args[0], "exit")==0){
      break;
    }else if (strcmp(args[0], "cd")==0){
      int ch = chdir(args[1]);
      if(ch<0) {printf("error: cd \n");}
    }else if (strcmp(args[0], "wait")==0){
    waitSig(args);
    }else{
      launch(args);
    }



    free(line);
    free(args);
  };

	return 0;
}
