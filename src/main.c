#include <stdio.h>
#include <stdint.h>

extern unsigned _stklen = 0x1400
extern unsigned _ovrbuffer = 0x1100;

int main(int argc, char **argv) {
  int i;
  srand8(100);
  for (i = 0; i < 300; i++) printf("%03d: %d\n", i, rand8());
  return 0;
}
