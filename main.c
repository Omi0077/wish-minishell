#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

typedef struct{
  char** buf;
  int size;
} parseLineRet;


parseLineRet* parseLine(char* line, char* delimiter){
  char** buf;
  int bufsize = 4;
  int offset = 0;
  char* token;

  buf = malloc(bufsize*sizeof(char*));
  if(buf == NULL){
    return NULL;
  }

  while(token = strsep(&line, delimiter)){
    //null-terminate token
    // printf("%s is %d long\n", token, strlen(token));
    // already null terminated

    // check if buffer is full
    if(offset == bufsize -1){
      bufsize *= 2;
      char** new_buf = realloc(buf, bufsize*sizeof(char*));
      if(new_buf == NULL){
        free(buf);
        return NULL;
      }
      buf = new_buf;
    }

    buf[offset++] = token;
  }

  // shrinking buffer to fit
  if(offset < bufsize-1){
    char** new_buf = realloc(buf, (offset+1)*sizeof(char*));
    if(new_buf != NULL){
      buf = new_buf;
    }
  }

  parseLineRet *temp = malloc(sizeof(parseLineRet));
  temp->buf = buf;
  temp->size = offset;
  return temp;
}

void runCommand(int argc, char* argv[]){
  if(argc < 1){
    exit(EXIT_FAILURE);
  }
  int pid = fork();
  if(pid == -1){
    perror("fork");
    exit(EXIT_FAILURE);
  }
  else if(pid == 0){
    //child process
    execvp(argv[0], argv);
    printf("this shouldnt print if everythings fine\n");
    exit(EXIT_FAILURE);
  }
  else{
    int terminatedPid = waitpid(pid, NULL, 0);
    // wait(NULL);
  }
}

int main(int argc, char* argv[]){
  if(argc > 1){
    fprintf(stdout, "%s %s\n", argv[0], argv[1]);
  }
  else{
    while(1){
      fprintf(stdout, "wish> ");
      char* line = NULL;
      size_t len = 0;
      getline(&line, &len, stdin);

      char* delimiter = " ";
      char** tokens;

      if(strcmp(line,"bye\n") == 0){ // not checking EOF
        free(line);
        free(tokens);
        exit(0);
      } 

      line[strcspn(line, "\n")] = '\0';
      // replace \n caught by getline

      parseLineRet* temp = parseLine(line, delimiter);
      tokens = temp->buf;
      int bufsize = temp->size;

      char* args[bufsize+1];
      for(int i=0; i<bufsize; i++){
        args[i] = tokens[i];
      }
      args[bufsize] = NULL;
      // args[0] = "ls";
      // args[1] = NULL;
      runCommand(bufsize,args);

      free(temp);
    }
  }
}