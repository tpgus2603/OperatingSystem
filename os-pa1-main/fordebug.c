#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include<string.h>
#include "parser.h"
int main() {
  char a [10000];
  strcat(a,"abdc");
  printf("%s",a);

}