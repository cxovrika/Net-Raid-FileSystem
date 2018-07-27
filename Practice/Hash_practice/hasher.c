#include "md5.h"
#include <stdio.h>

int main(int argc, char const *argv[]) {

  unsigned char hash[16];
  get_hash_from_path("./a.txt", hash);
  int i;
  for (i = 0; i < 16; i++)
    printf("%x", hash[i]);


  // char buf[32] = "c";
  // MD5_CTX cx;
  // MD5_Init(&cx);
  // MD5_Update(&cx, buf, strlen(buf));
  // unsigned char result[64];
  // MD5_Final(result, &cx);
  // int i;
  // for (i = 0; i < 64; i++)
  //   printf("%c", result[i]);
  // printf("\n");
}
