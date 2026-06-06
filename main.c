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

char **path_dirs;
int pathCount = 0;
int path_dir_size = 0;

// some utils

//this works with any type
#define my_free(ptr) do { free(ptr); (ptr) = NULL; } while(0)

void printPathDirs(){
  for(int i=0; i<pathCount; i++){
    printf("%d path: %s\n", i, path_dirs[i]);
  }
}

void errorOccured();
parseLineRet *parseLine(char *line, char *delimiter);
void runCommand(int argc, char *argv[]);

// path related functons
void initPath();
void addPath(int pathArgCount, char *pathArgs[]);
void freePath_dirs();

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

      char *delimiter = " \t";
      char **tokens;

      line[strcspn(line, "\n")] = '\0';
      // replace \n caught by getline

      parseLineRet *temp = parseLine(line, delimiter);
      tokens = temp->buf;
      int bufsize = temp->size;

      if (strcmp(tokens[0], "bye") == 0)
      { // not checking EOF
        my_free(line);
        my_free(tokens);
        my_free(path_dirs);
        exit(0);
      }
      else if (strcmp(tokens[0], "path") == 0)
      {
        //
        // printf("path_dirs: %s len: %d\n", path_dirs, strlen(path_dirs));
        addPath(bufsize, tokens);
        // printf("path_dirs: %s len: %d\n", path_dirs, strlen(path_dirs));
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

      my_free(temp->buf);
      my_free(temp);
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
    if (strcmp(token, "") == 0)
      continue;
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
        my_free(buf);
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
  pid_t pid = fork();
  if (pid == -1)
  {
    perror("fork");
    exit(EXIT_FAILURE);
  }
  else if (pid == 0)
  {
    // checking if executable exists

    // now search executable in every path token
    bool pathFound = false;
    char *finalPath;
    for (int i = 0; i < pathCount; i++)
    {
      finalPath = malloc(strlen(path_dirs[i]) + strlen(argv[0]) + 2);
      if (finalPath != NULL)
      {
        strcpy(finalPath, path_dirs[i]);
        strcat(finalPath, "/");
        strcat(finalPath, argv[0]);

        // printf("final path: %s\n", finalPath);

        int executableExist = access(finalPath, X_OK);
        if (executableExist == -1)
        {
          my_free(finalPath);
          continue; // to next token
        }
        else if (executableExist == 0)
        {
          pathFound = true;
          break; // exit loop , since executable found
        }
      }
    }

    if (pathFound)
    {
      execv(finalPath, argv);
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
  path_dirs = malloc(++path_dir_size * sizeof(char *));
  // increment path_dir_size then allocate
  if (path_dirs != NULL)
  {
    path_dirs[pathCount++] = strdup("/bin");
  }
  else
  {
    errorOccured();
    perror("malloc");
    exit(EXIT_FAILURE);
  }
}

void freePath_dirs()
{
  if (path_dir_size == 0)
    return;
  // my_free all char* first
  for (int i = 0; i < path_dir_size; i++)
  {
    if (path_dirs[i] != NULL)
    {
      my_free(path_dirs[i]);
    }
  }
  path_dir_size = 0;
  pathCount = 0;
  // now my_free char**
  if (path_dirs != NULL)
  {
    my_free(path_dirs);
  }
}

void addPath(int pathArgCount, char *pathArgs[]) // doesnt own char *pathArgs[]
{
  freePath_dirs();

  if (pathArgCount - 1 > 0)
  {
    // pathArgCount-1 no. of char* to be added in Char** path_dirs
    path_dir_size += pathArgCount - 1;
    char **temp = malloc(path_dir_size * sizeof(char *));
    if (temp == NULL)
    {
      errorOccured();
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    path_dirs = temp;

    // now iterate over all the pathArgs and strdup it in path_dirs
    for (int i = 1; i < pathArgCount; i++)
    {
      path_dirs[pathCount++] = strdup(pathArgs[i]);
    }
  }
}