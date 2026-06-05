#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>

typedef struct
{
  char **buf;
  int size;
} parseLineRet;

char *PATH;
int pathSize = 4;

void errorOccured();
parseLineRet *parseLine(char *line, char *delimiter);
void runCommand(int argc, char *argv[]);
void initPath();
void addPath(int pathArgCount, char *pathArgs[]);

int main(int argc, char *argv[])
{
  initPath();
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
        free(PATH);
        exit(0);
      }
      else if (strcmp(tokens[0], "path") == 0)
      {
        //
        // printf("PATH: %s len: %d\n", PATH, strlen(PATH));
        addPath(bufsize, tokens);
        // printf("PATH: %s len: %d\n", PATH, strlen(PATH));
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
    if(strcmp(token, "") == 0) continue;
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
    // checking if executable exists
    // getting path tokens
    char **paths;
    int pathCount;
    parseLineRet *temp = parseLine(PATH, " ");
    paths = temp->buf;
    pathCount = temp->size;


    // now search executable in every path token
    bool pathFound = false;
    char *finalPath;
    for (int i = 0; i < pathCount; i++)
    {
      /*
      when multiple delimiters are present continuously strsep return empty tokens
      although these empty "" are not found in access() , so it works
      but we are skipping it all together

      fixed the parseLine() to skip "" so this is not needed anymore
      */
      // if(strcmp(paths[i],"") == 0){
      //   printf("was here\n");
      //   continue;
      // }
      finalPath = malloc(strlen(paths[i]) + strlen(argv[0]) + 2);
      if (finalPath != NULL)
      {
        strcpy(finalPath, paths[i]);
        strcat(finalPath, "/");
        strcat(finalPath, argv[0]);

        // printf("final path: %s\n", finalPath);

        int executableExist = access(finalPath, X_OK);
        if (executableExist == -1)
        {
          free(finalPath);
          continue; // to next token
        }
        else if (executableExist == 0)
        {
          pathFound = true;
          // printf("final path: %s|\n", finalPath);
          break; // exit loop , since executable found
        }
      }
    }

    if (pathFound)
    {
      execv(finalPath, argv);
      free(temp);
      printf("this shouldnt print if everythings fine\n");
      exit(EXIT_FAILURE);
    }
    else
    {
      fprintf(stderr, "command %s not found\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    int terminatedPid = waitpid(pid, NULL, 0);
    // wait(NULL);
  }
}

void initPath()
{
  PATH = malloc(pathSize + 1); // for \0
  strcpy(PATH, "/bin");
}

void addPath(int pathArgCount, char *pathArgs[])
{
  pathSize = 0;
  free(PATH);
  PATH = NULL;

  if (pathArgCount - 1 > 0)
  {
    for (int i = 1; i < pathArgCount; i++)
    {
      // realloc
      char *temp = realloc(PATH, pathSize + strlen(pathArgs[i]) + 2); // +2 for space in between and \0 at end
      if (temp == NULL)
      {
        perror("realloc");
        exit(EXIT_FAILURE);
      }
      PATH = temp;

      if (i == 1)
      {
        strcpy(PATH, pathArgs[i]);
        continue;
      }

      PATH = strcat(PATH, " ");
      PATH = strcat(PATH, pathArgs[i]);
    }
  }
}
