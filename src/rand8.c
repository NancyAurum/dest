#include <stdio.h>
#include <stdint.h>

// ChatGPT claims it resembles the:
// 8-bit Maxim Integrated (formerly Dallas Semiconductor) one-wire CRC algorithm


//48-bit shift register
static uint8_t s[6]; //state

uint8_t rand8() { //1000:0730
  int i,c;
  uint8_t *src, *dst;
  //sum 1st, 4th and 5th bytes into the 0th byte
  c = s[1] + s[4] + 1;
  if (c&0x100) c = (c&0xFF) + 1;
  s[0] = (c + s[5])&0xFF;

  //shieft right by 8 bites
  src = s + 4;
  dst = s + 5;
  for (i = 5; i != 0; i = --i) *dst-- = *src--;

  return s[0];
}

void srand8(uint32_t seed) { //1000:0730
  int i;
  uint8_t *pseed;

  seed ^= 0x55555555;
  pseed = (uint8_t*)&seed;
  s[0] = 0;
  s[1] = pseed[0];
  s[2] = pseed[1];
  s[3] = pseed[2];
  s[4] = pseed[3];
  s[5] = 0;
  for (i = 0; i < 200; i++) rand8();
  return;
}

int main(int argc, char **argv) {
  int i;
  srand8(100);
  for (i = 0; i < 300; i++) printf("%03d: %d\n", i, rand8());
  return 0;
}
