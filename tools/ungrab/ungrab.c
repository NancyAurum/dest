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
} PACKED cte_t;

typedef struct { //stronghold graphics
  uint8_t type; //0=disk, 1=loaded
  uint32_t sz;
  uint16_t w;
  uint16_t h;
  //uint8 data[sz]; //RLE-packed pixels
} PACKED gfx_t;

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

  for (i = 0; i < nitems; i++) {
    printf("Dumping entry %d...\n", i);
    write_whole_file_path(fmt("%s%04d",outpath, i), file+ct[i].ofs, ct[i].sz);
  }
  printf("Done!\n");

  return 0;
}
