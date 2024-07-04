/*
  UnGrab
  A command line utility which should ideally reverse the result of the
    st.exe grab 

  In other words will hopefully unpack files from the STRONG.DAT,
  recovering names, types and folder structure.

  Example use:
    ./build.bat && ./ungrab.exe ../../../dest_data/STRONG.DAT ./out


                                         -Nancy Sadkov

*/

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "nap.h"
#include "pic.h"


#define NG_UTIL_IMPLEMENTATION
#include "util.h"


#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

//ensures compiler wont add gaps between the fields.
#define PACKED __attribute__((packed)) 

//FIXME: map file hash to filename, in case we encounter STRONG.DAT with
//unknown entries.

typedef struct { //content table entry
  uint8_t state;     //0=disk, 1=loaded
  uint32_t ofs;      //offset inside STRONG.DAT
  uint32_t sz;       //size of the file
  uint8_t info[12];  //0s when in STRONG.DAT
} PACKED cte_t; //21 bytes in size

typedef struct { //stronghold graphics
  uint8_t type; //0=disk, 1=loaded
  uint32_t sz;
  uint16_t w;
  uint16_t h;
  //uint8 data[sz]; //RLE-packed pixels
                    //the first data byte could be either pixel or
                    //additional info
} PACKED stg_t;

void fail(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int l = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  char *s = NULL;
  arrsetlen(s, l+1);
  va_start(ap, fmt);
  vsprintf(s, fmt, ap);
  va_end(ap);
  fprintf(stderr, "%s\n", s);
  exit(-1);
}


void usage() {
#define P printf
  P("UnGrab v0.1 by Nancy Sadkov\n");
  P("Usage: ungrab path/to/STRONG.DAT output/folder/\n");
  P("\n");
  exit(0);
#undef P
}

typedef struct { //extended information
  int ext;
  int16_t sx;
  int16_t sy;
  int16_t ex; //inclusive
  int16_t ey;
  char *name;
} einfo_t;

einfo_t einfo[] = {
  {0,  0,  0,  10, 15,"mouse03.lbm"},
  {0, 77,  11, 93, 27,"mouse03.lbm"},
  {0, 96,  55,109, 62,"mouse03.lbm"},
  {0, 159, 22,171, 32,"mouse03.lbm"},

  //bigcomp.lbm is 320x200 lbm with smaller items
  //the first row elements are 50x42 in size
  //3 in total
  //original LBM had grid lines between them
  //i.e. these were 52x44
  {0, 159, 22,171, 32,"bigcomp.lbm"},

  {0,0,0,0,0,0}
};


int rle_decode(uint8_t *src, uint8_t *dst, int sz) {
  uint8_t *q;
  uint8_t b;
  int i;
  uint8_t *end = dst + sz;
  uint8_t *ps;
  uint8_t *s = src;

  do {
    do {
      ps = s;
      s = ps + 1;
      b = *ps;
    } while (b == 0x80);
    if (b < 0x80) {
      i = (uint8_t)(b + 1);
      do {
        q = s;
        s = s + 1;
        if (*q != 0) {
          *dst = *q;
        }
        dst = dst + 1;
        i = i - 1;
      } while (i != 0);
    }
    else {
      i = (uint8_t)((b ^ 0xff) + 2);
      q = s;
      s = ps + 2;
      b = *q;
      if (b == 0) {
        dst = dst + i;
      }
      else {
        for (; i != 0; i = i - 1) {
          q = dst;
          dst = dst + 1;
          *q = b;
        }
      }
    }
  } while (dst < end);
  return s - src;
}



int main(int argc, char **argv) {
  int i, j;

  NAP_INTRO
  NAP_IF("h") usage(); NAP_FI
  NAP_OUTRO(2,2)

  char *outpath = fargv[1];
  if (outpath[strlen(outpath)-1] != '/') outpath = cat(outpath,"/");

  if (!file_exists(fargv[0])) fail("File doesn't exist: %s", fargv[0]);
  
  int64_t file_size;
  uint8_t *file = read_whole_file(fargv[0], &file_size);
  uint32_t ctofs = *(uint32_t*)file; //files table offset
  uint32_t ctsz = file_size - ctofs;
  int nitems = ctsz/sizeof(cte_t);

  printf("Content Table Offset: %x\n", ctofs);
  printf("Content Table Size: %x\n", ctsz);
  printf("Number of entries: %d\n", nitems);

  //do sanity check
  if (ctofs+sizeof(cte_t)*100 > file_size
      || nitems < 100 || nitems > 0x8000
      || file_size < ctsz*10
      || ctsz%sizeof(cte_t)
      ) {
    fail("Invalid STRONG.DAT");
  }

  cte_t *ct = (cte_t*)(file+ctofs);

#if 0
  for (i = 0; i < nitems; i++) {
    printf("%04d: Ofs=%x Sz=%x\n", i, ct[i].ofs, ct[i].sz);
    if (ct[i].state) fail("entry %d has non-null state", i);
    for (j = 0; j < 12; j++)
      if (ct[i].info[j]) fail("entry %d has non-null info", i);
  }
#endif



#if 0
  for (i = 0; i < nitems; i++) {
    printf("Dumping entry %d...\n", i);
    write_whole_file_path(fmt("%s%04d",outpath, i), file+ct[i].ofs, ct[i].sz);
  }
  printf("Done!\n");
#endif

  

#if 1
  for (i = 0; i < nitems; i++) {
    if (i == 0) {
      einfo_t *ei = &einfo[i];
      stg_t *stg = (stg_t*)(file+ct[i].ofs);
      printf("%dx%d %d bytes\n", stg->w, stg->h, stg->sz);
      pic_t *pic = picNew(stg->w, stg->h, 8);
      pic->P = new(uint8_t,4*256);
      for (j = 0; j < 256; j++) {
        pic->P[j*4+0] = rand()%256;
        pic->P[j*4+1] = rand()%256;
        pic->P[j*4+2] = rand()%256;
        pic->P[j*4+3] = 0;
      }
      //picClear(pic, 0xFF00FF);
      rle_decode((uint8_t*)(stg+1), pic->D, stg->w*stg->h);
      pngSave("./out/test.png", pic);
    }
    break;
    //printf("Dumping entry %d...\n", i);
    //write_whole_file_path(fmt("%s%04d",outpath, i), file+ct[i].ofs, ct[i].sz);
  }
  printf("Done!\n");
#endif

  return 0;
}
