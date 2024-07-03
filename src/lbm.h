#ifndef LBM_H
#define LBM_H

#include "x86.h"


typedef struct { //all fields are big-endian, because Amiga.
  uint16_t w;          //00 width
  uint16_t h;          //02 height
  int16_t x;           //04 0 unless LBM is part of a larger image
  int16_t y;           //06 0 unless LBM is part of a larger image
  uint8_t nplanes;     //08 1=monochrome, 4=16 colors, 8=256 colors
  uint8_t mask;        //09 1=masked, 2=transparent color, 3=lasso (for MacPaint).
                       //   Mask data is not considered a bit plane.
  uint8_t enc;         //0A compression: 0=plain. 1=RLE
  uint8_t pad1;        //0B padding for Amiga's CPU. should be 0
  uint16_t bg;         //0C Transparent colour, useful only when mask >= 2
  uint8_t xAspect;     //0E
  uint8_t yAspect;     //0F
  int16_t pageWidth;   //10 The size of the screen the image is to
                       //   be displayed on in pixels, usually 320×200 
  int16_t pageHeight;  //12
} lbm_bmhd_t


typedef struct { //temporary representation of decoded LBM
  uint8_t cmap[256*3];
  far_t aptr; //segment aligned pointer to the pixels buffer
  far_t bptr; //the ptr used to free the aptr buffer
} gfx_tmp_t;

#endif
