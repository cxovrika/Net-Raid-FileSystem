#include <stdio.h>
#include "message.h"

int main(int argc, char const *argv[]) {
  printf("This message is from main.c\n");
  print_message();
  return 0;
}
