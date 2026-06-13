#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/stat.h>

/*
The refactor would be:

Split line on & first → gives you N command strings
For each command string, run your parseLine → gives you tokens
Scan tokens for > → extract output file, strip > and filename from argv
Store result in a Command struct

Then your execution loop is just: for each Command, fork and exec it, with a single unified runCommand(Command *cmd).
*/

typedef struct
{
  char **argv;
  char *outputFile;
  int argc;
} Command;

typedef struct
{
  Command *cmds;
  int cmdsCount;
} getCommandsRet;

getCommandsRet *getCommands(char *line);

typedef struct
{
  char **buf;
  int size;
} parseLineRet;

char **path_dirs;
int pathCount = 0;
int path_dir_size = 0;

// some utils

// this works with any type
#define my_free(ptr) \
  do                 \
  {                  \
    free(ptr);       \
    (ptr) = NULL;    \
  } while (0)

void printPathDirs()
{
  for (int i = 0; i < pathCount; i++)
  {
    printf("%d path: %s\n", i, path_dirs[i]);
  }
}

void freeStringArray(char **buf, int len)
{
  for (int i = 0; i < len; i++)
  {
    my_free(buf[i]);
  }
  my_free(buf);
}

char *preProcessLine(char *line);
void errorOccured();
parseLineRet *parseLine(char *line, char *delimiter);
void runCommand(int argc, char *argv[], char *output);

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
    FILE* fileFD = fopen(argv[1], "r");
    if (fileFD == NULL)
    {
      errorOccured();
      exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    while (getline(&line, &len, fileFD) != -1) // read line from file one by one
    {
      // replace \n caught by getline
      line[strcspn(line, "\n")] = '\0';

      // pre-process
      line = preProcessLine(line);

      // parse commands out of line
      getCommandsRet *temp_commands = getCommands(line);
      Command *commands = temp_commands->cmds;
      int commandCount = temp_commands->cmdsCount;

      for (int i = 0; i < commandCount; i++)
      {
        Command *currCommand = &commands[i];

        if (strcmp(currCommand->argv[0], "bye") == 0)
        { // not checking EOF
          my_free(line);
          my_free(commands);
          my_free(temp_commands);
          freePath_dirs();
          exit(EXIT_SUCCESS);
        }
        else if (strcmp(currCommand->argv[0], "path") == 0)
        {
          //
          // printf("path_dirs: %s len: %d\n", path_dirs, strlen(path_dirs));
          addPath(currCommand->argc, currCommand->argv);
          // printf("path_dirs: %s len: %d\n", path_dirs, strlen(path_dirs));
        }
        else if (strcmp(currCommand->argv[0], "cd") == 0)
        {
          if (currCommand->argc != 2)
          {
            errorOccured();
          }
          else
          {
            int cd = chdir(currCommand->argv[1]);
            if (cd != 0)
              errorOccured();
          }
        }
        else
        {
          char *myArgs[currCommand->argc + 1];
          for (int i = 0; i < currCommand->argc; i++)
          {
            myArgs[i] = currCommand->argv[i];
          }
          myArgs[currCommand->argc] = NULL; // set last as NULL

          runCommand(currCommand->argc, myArgs, currCommand->outputFile);
        }
      }

      while (wait(NULL) > 0); // wait for all process to finish

      my_free(line);
      my_free(commands);
      my_free(temp_commands);
    }

    my_free(line);
    freePath_dirs();
    exit(EXIT_SUCCESS);
  }
  else
  {
    while (1)
    {
      printf("wish> ");
      char *line = NULL;
      size_t len = 0;
      if (getline(&line, &len, stdin) == -1)
      {
        my_free(line);
        freePath_dirs();
        exit(EXIT_SUCCESS);
      }

      // replace \n caught by getline
      line[strcspn(line, "\n")] = '\0';

      // pre-process line
      line = preProcessLine(line);

      // parse commands out of line
      getCommandsRet *temp_commands = getCommands(line);
      Command *commands = temp_commands->cmds;
      int commandCount = temp_commands->cmdsCount;

      // char *delimiter = " \t";
      // char **tokens;

      // parseLineRet *temp = parseLine(line, delimiter);
      // tokens = temp->buf;
      // int bufsize = temp->size;

      for (int i = 0; i < commandCount; i++)
      {
        Command *currCommand = &commands[i];

        /*
        since we are freeing line, and each char* in tokens/temp->buf points to subset of line
        as thats how strsep works, we dont have to free each char* .
        */
        if (strcmp(currCommand->argv[0], "bye") == 0)
        { // not checking EOF
          my_free(line);
          my_free(commands);
          my_free(temp_commands);
          freePath_dirs();
          exit(EXIT_SUCCESS);
        }
        else if (strcmp(currCommand->argv[0], "path") == 0)
        {
          //
          // printf("path_dirs: %s len: %d\n", path_dirs, strlen(path_dirs));
          addPath(currCommand->argc, currCommand->argv);
          // printf("path_dirs: %s len: %d\n", path_dirs, strlen(path_dirs));
        }
        else if (strcmp(currCommand->argv[0], "cd") == 0)
        {
          if (currCommand->argc != 2)
          {
            errorOccured();
          }
          else
          {
            int cd = chdir(currCommand->argv[1]);
            if (cd != 0)
              errorOccured();
          }
        }
        else
        {
          char *myArgs[currCommand->argc + 1];
          for (int i = 0; i < currCommand->argc; i++)
          {
            myArgs[i] = currCommand->argv[i];
          }
          myArgs[currCommand->argc] = NULL; // set last as NULL

          runCommand(currCommand->argc, myArgs, currCommand->outputFile);
        }
      }
      while (wait(NULL) > 0); // wait for all process to finish

      my_free(line);
      my_free(commands);
      my_free(temp_commands);
    }
  }
}

char *preProcessLine(char *line)
{
  int len = strlen(line);

  int charIndex = 0;
  while (charIndex < len)
  {
    if (line[charIndex] == '>')
    {
      char *newLine = malloc((len + 3) * sizeof(char));
      if (newLine == NULL)
      {
        return NULL;
      }
      strncpy(newLine, line, charIndex); // copied till before >
      strcat(newLine, " > ");
      strcat(newLine, &line[charIndex + 1]);

      my_free(line); // free old one
      line = newLine;

      len = strlen(line);
      charIndex += 3;
    }
    else
    {
      charIndex++;
    }
  }
  // printf("%s\n", line);

  return line;
}

void errorOccured()
{
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

/*
I know i am being iconsistent with ownership model , in case with path_dirs i am using strdup() by which
if i am filling char*s in char** i will have to free each char* individually.
but in this case i am using which mdifies line* which it doesnt own, but i am doing it carefully
knowing that i am not going to free(line) before freeing buf/temp->buff/tokens.
*/
parseLineRet *parseLine(char *line, char *delimiter) // doesnt own line
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

    buf[offset++] = strdup(token);
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

void runCommand(int argc, char *argv[], char *output)
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
      if (output != NULL)
      {
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        open(output, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU);
      }
      execv(finalPath, argv);
      // printf("this shouldnt print if everythings fine\n");
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
    // int terminatedPid = waitpid(pid, NULL, 0);
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

getCommandsRet *getCommands(char *line)
{
  // first split on &
  char **commandStrings;
  int commandStringCount;

  parseLineRet *temp_commandString = parseLine(line, "&");
  if (temp_commandString == NULL)
    return NULL;

  commandStrings = temp_commandString->buf;
  commandStringCount = temp_commandString->size;

  // now split commandStrings into tokens
  Command *commands = malloc(commandStringCount * sizeof(Command));

  for (int cmdNum = 0; cmdNum < commandStringCount; cmdNum++)
  {
    parseLineRet *temp_tokens = parseLine(commandStrings[cmdNum], " \t");
    if (temp_tokens == NULL)
    {
      freeStringArray(commandStrings, commandStringCount);
      my_free(temp_commandString);
      my_free(commands);
      return NULL;
    }

    // default value for command outputfile and argc
    commands[cmdNum].outputFile = NULL;
    commands[cmdNum].argc = (temp_tokens->size);

    for (int tknNum = 0; tknNum < temp_tokens->size; tknNum++)
    {
      if (strcmp(temp_tokens->buf[tknNum], ">") == 0)
      {
        if (tknNum != (temp_tokens->size - 2) || tknNum == 0) // if > is not second last or in first
        {
          errorOccured();
          freeStringArray(commandStrings, commandStringCount);
          my_free(temp_commandString);
          my_free(commands);
          freeStringArray(temp_tokens->buf, temp_tokens->size);
          my_free(temp_tokens);
          return NULL;
        }
        if (strcmp(temp_tokens->buf[tknNum + 1], ">") == 0) // if there are consecutive >
        {
          errorOccured();
          freeStringArray(commandStrings, commandStringCount);
          my_free(temp_commandString);
          my_free(commands);
          freeStringArray(temp_tokens->buf, temp_tokens->size);
          my_free(temp_tokens);
          return NULL;
        }
        // now re-write the respective command data
        commands[cmdNum].outputFile = temp_tokens->buf[tknNum + 1]; // not using strdup here
        commands[cmdNum].argc = (temp_tokens->size) - 2;
      }
    }

    // now fill argv of command
    commands[cmdNum].argv = malloc((commands[cmdNum].argc) * sizeof(char *));

    for (int argNum = 0; argNum < commands[cmdNum].argc; argNum++)
    {
      commands[cmdNum].argv[argNum] = temp_tokens->buf[argNum]; // not using strdup here
    }
  }

  // for (int i = 0; i < commandStringCount; i++)
  // {
  //   printf("command no. %d\n", i + 1);
  //   for (int j = 0; j < commands[i].argc; j++)
  //   {
  //     printf("arg %d: %s\n", j + 1, commands[i].argv[j]);
  //   }
  //   printf("output file: %s\n", commands[i].outputFile);
  // }

  /*
  since we used parseline to get temp_tokens which uses strdup for each token in temp_tokens->buf so we can free
  commandStrings which was passed in parseline for tokenization of each command
  */
  freeStringArray(commandStrings, commandStringCount);
  my_free(temp_commandString);

  getCommandsRet *temp = malloc(sizeof(getCommandsRet));
  temp->cmds = commands;
  temp->cmdsCount = commandStringCount;
  return temp;
}