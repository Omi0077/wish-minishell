#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

typedef struct
{
  char **buf;
  int size;
} parseLineRet;

char* PATH = "/bin";

void errorOccured();
parseLineRet *parseLine(char *line, char *delimiter);
void runCommand(int argc, char *argv[]);
void addPath(int argc, char *argv[]);


int main(int argc, char *argv[])
{
  if (argc > 1)
  {
    fprintf(stdout, "%s %s\n", argv[0], argv[1]);
  }
  else
  {
    while (1)
    {
      fprintf(stdout, "wish> ");
      char *line = NULL;
      size_t len = 0;
      getline(&line, &len, stdin);

      char *delimiter = " ";
      char **tokens;

      line[strcspn(line, "\n")] = '\0';
      // replace \n caught by getline

      parseLineRet *temp = parseLine(line, delimiter);
      tokens = temp->buf;
      int bufsize = temp->size;

      if (strcmp(tokens[0], "bye") == 0)
      { // not checking EOF
        free(line);
        free(tokens);
        exit(0);
      }
      else if (strcmp(tokens[0], "path") == 0)
      {
        //
      }
      else
      {
        char *myArgs[bufsize + 1];
        for (int i = 0; i < bufsize; i++)
        {
          myArgs[i] = tokens[i];
        }
        myArgs[bufsize] = NULL; // set last as NULL

        runCommand(bufsize, myArgs);
      }

      free(temp);
    }
  }
}






void errorOccured()
{
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

parseLineRet *parseLine(char *line, char *delimiter)
{
  char **buf;
  int bufsize = 4;
  int offset = 0;
  char *token;

  buf = malloc(bufsize * sizeof(char *));
  if (buf == NULL)
  {
    return NULL;
  }

  while (token = strsep(&line, delimiter))
  {
    // null-terminate token
    //  printf("%s is %d long\n", token, strlen(token));
    //  already null terminated

    // check if buffer is full
    if (offset == bufsize - 1)
    {
      bufsize *= 2;
      char **new_buf = realloc(buf, bufsize * sizeof(char *));
      if (new_buf == NULL)
      {
        free(buf);
        return NULL;
      }
      buf = new_buf;
    }

    buf[offset++] = token;
  }

  // shrinking buffer to fit
  if (offset < bufsize - 1)
  {
    char **new_buf = realloc(buf, (offset + 1) * sizeof(char *));
    if (new_buf != NULL)
    {
      buf = new_buf;
    }
  }

  parseLineRet *temp = malloc(sizeof(parseLineRet));
  temp->buf = buf;
  temp->size = offset;
  return temp;
}

void runCommand(int argc, char *argv[])
{
  if (argc < 1)
  {
    exit(EXIT_FAILURE);
  }
  int pid = fork();
  if (pid == -1)
  {
    perror("fork");
    exit(EXIT_FAILURE);
  }
  else if (pid == 0)
  {
    // child process
    // check if executable exists
    int executableExist = access(argv[0], X_OK);
    if (executableExist == -1)
    {
      errorOccured();
      perror("executable not found");
      exit(EXIT_FAILURE);
    }
    execvp(argv[0], argv);
    printf("this shouldnt print if everythings fine\n");
    exit(EXIT_FAILURE);
  }
  else
  {
    int terminatedPid = waitpid(pid, NULL, 0);
    // wait(NULL);
  }
}

void addPath(int argc, char *argv[]){
  if(argc == 0){
    PATH = NULL;
  }
  else if(argc > 0){
    
  }
}
