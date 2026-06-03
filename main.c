#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

      if(strcmp(line,"bye\n") == 0){
        exit(0);
      }
    }
  }
}