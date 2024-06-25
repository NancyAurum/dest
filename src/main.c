#include <stdio.h>

//_psp, _heaplen, _osmajor, _osminor, _stklen, etc..
#include <dos.h>

#include "st.h"
#include "stdos.h"

char *GGameExe;
char *GGameArg;

int main(int argc, char **argv) {
  GGameExe = argv[0];
  GGameArg = argc < 2 ? "" : argv[1];
  init_game();
  return 0;
}
