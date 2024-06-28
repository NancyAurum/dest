#include <stdio.h>

//_psp, _heaplen, _osmajor, _osminor, _stklen, etc..
#include <dos.h>

#include "st.h"
#include "stdos.h"

char *GExeCmd;
char *GExeArg;

int main(int argc, char **argv) {
  GExeCmd = argv[0];
  GExeArg = argc < 2 ? "" : argv[1];
  init_game();
  return 0;
}
