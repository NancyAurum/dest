#include "png/png.h"
#include "util.h"
#include "pic.h"


pic_t *picNew(int W, int H, int BPP) {
  int I, X,Y;
  pic_t *P = new(pic_t,1);
  P->W = W;
  P->H = H;
  P->B = BPP;
  P->I = (P->W*BPP+7)/8;
  P->S = P->H*P->I;
  P->D = new(u1, P->S);
  P->K = P->BK = P->SK = -1;
  if (P->B <= 8) {
    P->P = new(u1,256*4);
    memset(P->P, 0, 256*4);
    times(Y,16) times(X,16) {
      I = (Y*16+X)*4;
      P->P[I+0] = (X<<4)|Y;
      P->P[I+1] = (Y<<4)|X;
      P->P[I+2] = X*Y;
    }
  }
  return P;
}

void picDel(pic_t *P) {
  if (!P->Proxy) {
    if (P->D) free(P->D);
    if (P->N) free(P->N);
    if (P->B <= 8 && !P->SharedPalette && P->P) free(P->P);
  }
  free(P);
}

pic_t *picDup(pic_t *Src) {
  int X, Y;
  pic_t *P = picNew(Src->W,Src->H,Src->B);
  if (Src->P) memcpy(P->P, Src->P, 256*4);
  picBlt(P,Src,0,0,0,0,0,P->W,P->H);
  P->X = Src->X;
  P->Y = Src->Y;
  return P;
}

pic_t *picProxy(pic_t *Src, int X, int Y, int W, int H) {
  pic_t *P = new(pic_t,1);
  memcpy(P, Src, sizeof(pic_t));
  P->D += Y*Src->I + X*Src->B/8;
  P->Proxy = Src;
  P->X = X;
  P->Y = Y;
  P->W = W;
  P->H = H;
  return P;
}

void picPut(pic_t *P, int X, int Y, u4 C) {
  if (0 <= X && X<P->W && 0 <= Y && Y<P->H) {
    if (P->Proxy) picPut(P->Proxy, P->X+X, P->Y+Y, C);
    else P->D[Y*P->I + X] = C;
  }
}

void picPut24(pic_t *P, int X, int Y, u4 C) {
  if (0 <= X && X<P->W && 0 <= Y && Y<P->H) {
    if (P->Proxy) picPut24(P->Proxy, P->X+X, P->Y+Y, C);
    else {
      fromR8G8B8(P->D[Y*P->I+X*3+0],P->D[Y*P->I+X*3+1],P->D[Y*P->I+X*3+2],C);
    }
  }
}

void picPut32(pic_t *P, int X, int Y, u4 C) {
  if (0 <= X && X<P->W && 0 <= Y && Y<P->H) {
    if (P->Proxy) picPut32(P->Proxy, P->X+X, P->Y+Y, C);
    else ((u4*)P->D)[Y*P->W + X] = C;
  }
}

u4 picGet(pic_t *P, int X, int Y) {
  if (P->Proxy) return picGet(P->Proxy, P->X+X, P->Y+Y);
  else if (0 <= X && X<P->W && 0 <= Y && Y<P->H) {
    return P->D[Y*P->I + X];
  }
  return P->K;
}

u4 picGet24(pic_t *P, int X, int Y) {
  if (P->Proxy) return picGet24(P->Proxy, P->X+X, P->Y+Y);
  else if (0 <= X && X<P->W && 0 <= Y && Y<P->H) {
    return R8G8B8(P->D[Y*P->I+X*3+0], P->D[Y*P->I+X*3+1], P->D[Y*P->I+X*3+2]);
  }
  return P->K;
}

u4 picGet32(pic_t *P, int X, int Y) {
  if (P->Proxy) return picGet32(P->Proxy, P->X+X, P->Y+Y);
  else if (0 <= X && X<P->W && 0 <= Y && Y<P->H) {
    return ((u4*)P->D)[Y*P->W + X];
  }
  return P->K;
}

pic_t *picClear(pic_t *P, u4 V) {
  int X, Y;
  if (P->B == 8) {
    times (Y, P->H)
      times (X, P->W)
        picPut(P, X, Y, V);
  } else if (P->B == 24) {
    times (Y, P->H)
      times (X, P->W)
        picPut24(P, X, Y, V);
  } else if (P->B == 32) {
    times (Y, P->H)
      times (X, P->W)
        picPut32(P, X, Y, V);
  } else {
    printf("cant clear %d-bit image\n", P->B);
    abort();
  }
  return P;
}

void picPalSet(pic_t *P, int Index, u4 Color) {
  if (P->B == 8) {
    ((u4*)P->P)[Index] = Color;
  } else {
    printf("cant set palette for %d-bit image\n", P->B);
    abort();
  }
}

void picBlt(pic_t *B, pic_t *A, int Flags, int DX, int DY
           ,int SX, int SY, int W, int H)
{
  int BX=0, BY=0, XI=1, YI=1, X, Y;
  u4 *T = new(u4, W*H);

  if (A->B==8) {
    // blocks can overlap, so we use temporary buffer
    times (Y, H) times (X, W) T[Y*W + X] = picGet(A, SX+X, SY+Y);

    if (Flags & PIC_FLIP_X) {
      BX = W-1;
      XI = -1;
    }

    if (Flags & PIC_FLIP_Y) {
      BY = H-1;
      YI = -1;
    }

    for (SY = BY, Y=0; Y < H; SY+=YI, Y++) {
      for (SX = BX, X=0; X < W; SX+=XI, X++) {
        unless (T[SY*W + SX]!=B->K) continue;
        picPut(B, DX+X, DY+Y, T[SY*W + SX]);
      }
    }
    //times (Y, H) times (X, W) picPut(B, DX+X, DY+Y, T[Y*W + X]);

    free(T);
  } else if (A->B==24) {
    // blocks can overlap, so we use temporary buffer

    times (Y, H) times (X, W) T[Y*W + X] = picGet24(A, SX+X, SY+Y);

    if (Flags & PIC_FLIP_X) {
      BX = W-1;
      XI = -1;
    }

    if (Flags & PIC_FLIP_Y) {
      BY = H-1;
      YI = -1;
    }

    for (SY = BY, Y=0; Y < H; SY+=YI, Y++) {
      for (SX = BX, X=0; X < W; SX+=XI, X++) {
        unless (T[SY*W + SX]&0xFF000000) continue;
        picPut24(B, DX+X, DY+Y, T[SY*W + SX]);
      }
    }
    //times (Y, H) times (X, W) picPut(B, DX+X, DY+Y, T[Y*W + X]);

    free(T);
  } else if (A->B==32) {
    // blocks can overlap, so we use temporary buffer

    times (Y, H) times (X, W) T[Y*W + X] = picGet32(A, SX+X, SY+Y);

    if (Flags & PIC_FLIP_X) {
      BX = W-1;
      XI = -1;
    }

    if (Flags & PIC_FLIP_Y) {
      BY = H-1;
      YI = -1;
    }

    for (SY = BY, Y=0; Y < H; SY+=YI, Y++) {
      for (SX = BX, X=0; X < W; SX+=XI, X++) {
        unless (T[SY*W + SX]&0xFF000000) continue;
        picPut32(B, DX+X, DY+Y, T[SY*W + SX]);
      }
    }
    //times (Y, H) times (X, W) picPut(B, DX+X, DY+Y, T[Y*W + X]);

    free(T);
  } else {
    printf("cant blit %d-bit image\n", A->B);
    abort();
  }
}

void pngSave(char *Output, pic_t *P) {
  png_structp Png;
  png_infop Info;
  int I, X, Y, BPR;
  u1 *Q, **Rows;
  FILE *F;
  png_color Pal[256];

  write_whole_file_path(Output, "", 0); //ensure path exists
  F = fopen(Output, "wb");
  if (!F) {
    printf("cant create %s\n", Output);
    abort();
  }
  Png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  Info = png_create_info_struct(Png);
  png_set_IHDR(Png,
               Info,
               P->W,
               P->H,
               P->B<8 ? P->B : 8, // depth of color channel
               P->B <= 8  ? PNG_COLOR_TYPE_PALETTE :
               P->B == 32 ? PNG_COLOR_TYPE_RGB_ALPHA :
                            PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  BPR = (P->W*P->B + 7)/8;
  Rows = png_malloc(Png, P->H * sizeof(png_byte *));
  if (P->B==8) {
    times (I, 256) {
      Pal[I].red = P->P[I*4+0];
      Pal[I].green = P->P[I*4+1];
      Pal[I].blue = P->P[I*4+2];
    }
    png_set_PLTE(Png, Info, Pal, 256);
    if (P->K!=-1) { // that is little ticky
      png_color_16 CK;
      CK.index = P->K; //just in case png uses it
      u1 Alpha[256];
      times (I, 256) Alpha[I] = I==P->K ? 0 : 0xFF;
      png_set_tRNS(Png, Info, Alpha, 256, &CK);
    }
    times (Y, P->H) {
      Rows[Y] = Q = png_malloc(Png, BPR);
      times (X, P->W) *Q++ = picGet(P,X,Y);
    }
  } else if (P->B==24) {
    times (Y, P->H) {
      Rows[Y] = Q = png_malloc(Png, BPR);
      times (X, P->W) {
        fromR8G8B8(Q[0],Q[1],Q[2], picGet24(P,X,Y));
        Q += 3;
      }
    }
  } else if (P->B==32) {
    times (Y, P->H) {
      Rows[Y] = Q = png_malloc(Png, BPR);
      times (X, P->W) {
        fromR8G8B8A8(Q[0],Q[1],Q[2],Q[3], picGet32(P,X,Y));
        Q += 4;
      }
    }
  } else {
    printf("  Cant save %d-bit PNGs\n", P->B);
    abort();
  }

  png_init_io(Png, F);
  png_set_rows(Png, Info, Rows);
  png_write_png(Png, Info, PNG_TRANSFORM_IDENTITY, NULL);

  times (Y, P->H) png_free(Png, Rows[Y]);
  png_free(Png, Rows);

  png_destroy_write_struct(&Png, &Info);
  fclose(F);
}







spr_t *sprNew() {
  spr_t *S = new(spr_t,1);
  S->ColorKey = -1;
  return S;
}

void sprDel(spr_t *S) {
  int I, J, K;
  if (S->Palette) free(S->Palette);
  times(I, S->NAnis) {
    times(J, S->Anis[I].NFacs) {
      times(K, S->Anis[I].Facs[J].NPics) {
        unless (S->Anis[I].Facs[J].Pics[K]) continue;
        picDel(S->Anis[I].Facs[J].Pics[K]);
      }
      if (S->Anis[I].Facs[J].Pics) free(S->Anis[I].Facs[J].Pics);
    }
    if (S->Anis[I].Facs) free(S->Anis[I].Facs);
    if (S->Anis[I].Name) free(S->Anis[I].Name);
  }
  if (S->Anis) free(S->Anis);
  free(S);
}
















#define CHK_FORM	0x464F524D
#define CHK_PBM		0x50424D20
#define CHK_BMHD	0x424D4844
#define CHK_CMAP	0x434D4150
#define CHK_DPPS	0x44505053
#define CHK_CRNG	0x43524E47
#define CHK_TINY	0x54494E59
#define CHK_BODY	0x424F4459

#define cmpNone         0
#define cmpByteRun1     1

#define mskNone                0
#define mskHasMask             1
#define mskHasTransparentColor 2
#define mskLasso               3

#define RowBytes(cols) ((((cols) + 15) / 16) * 2)


#if 1
#define MAX_RUN 128
int lbmLineRLE(uint8_t *dst, uint8_t *src, uint16_t length) {
  uint8_t *s = src;
  uint8_t *d = dst;
  uint8_t *end = src + length;
  uint8_t b;

  while (s < end) {
    uint8_t *p = s;
    uint16_t run_length = end - s;
    if (run_length > MAX_RUN) run_length = MAX_RUN;

    uint8_t *run_end = s + run_length;
    b = *s++;
    while (s < run_end && *s == b) s++;

    uint16_t run_size = s - p;
    if (run_size < 3) { //literal run?
#if 0
      while (s < run_end) {
        if (s+1<run_end && s[0] == s[1]) break;
        s++;
      }
#else
      while (s < run_end) {
        if (s+2<run_end && s[0] == s[1] && s[0] == s[2]) break;
        s++;
      }
#endif
      *d++ = s - p - 1;
      while (p < s) *d++ = *p++;
    } else  { //replicate run
      *d++ = (int)0xFF - run_size + 2; // Store run-length
      *d++ = b; // Store the byte
    }
  }
  //Some versions of Adobe Photoshop incorrectly use the n=128 no-op as
  //a repeat code, which breaks strictly conforming readers.
  //To read Photoshop ILBMs, allow the use of n=128 as a repeat.
  //This is pretty safe, since no known program writes real no-ops into
  //their ILBMs. The reason n=128 is a no-op is historical:
  //the Mac Packbits buffer was only 128 bytes, and a repeat code of
  //128 generates 129 bytes.
  if ((d-dst) & 1) *d++ = 0x80; //align with NOP
  return d - dst;
}
#endif

#if 0
#define MAX_RUN 128
int lbmLineRLE(uint8_t *dst, uint8_t *src, uint16_t length) {
  uint8_t *s = src;
  uint8_t *d = dst;
  uint8_t *end = src + length;
  uint8_t b;

  while (s < end) {
    uint8_t *p = s;
    uint16_t run_length = end - s;
    if (run_length > MAX_RUN) run_length = MAX_RUN;

    uint8_t *run_end = s + run_length;
    b = *s++;
    while (s < run_end && *s == b) s++;

    uint16_t run_size = s - p;
    if (run_size < 3) { //literal run?
#if 0
      while (s < run_end) {
        if (s+1<run_end && s[0] == s[1]) break;
        s++;
      }
#else
      while (s < run_end) {
        if (s+2<run_end && s[0] == s[1] && s[0] == s[2]) break;
        s++;
      }
#endif
      *d++ = s - p - 1;
      while (p < s) *d++ = *p++;
    } else  { //replicate run
      *d++ = (int)0xFF - run_size + 2; // Store run-length
      *d++ = b; // Store the byte
    }
  }
  if ((d-dst) & 1) *d++ = 0x80; //align with NOP
  return d - dst;
}
#endif

#if 0
static int lbmLineRLE(u1 *Dst, u1 *Src, int N) {
  u1 *D = Dst;
  u1 *S = Src;
  u1 *E = S+N;
  while (S < E) {
#if 0
    //*D++ = 0;
    //*D++ = *S++;
    int len = E - S;
    if (len > 128) len = 128;
    *D++ = len-1;
    while (len--) *D++ = *S++;
#else
    uint8_t *R = S;
    while (R < E && *R == *S && R-S < 128) R++;
    if (R - S >= 2) {
      *D++ = (uint8_t)(int8_t)-(R-S - 1);
      *D++ = *S;
      S = R;
    } else {
      R = S;
    }
    while (R < E && R-S < 128) {
      if (R+2<E && R[0] == R[1] && R[0] == R[2]) break;
      R++;
    }
    if (R - S) {
      *D++ = R - S - 1;
      while (S < R) *D++ = *S++;
    }
#endif
  }
  if ((D-Dst) & 1) *D++ = 0x80; //align with NOP
  return D - Dst;
}
#endif



// FIXME: align chunks to 16-bit words
void lbmSave(char *FileName, pic_t *P) {
  int uncomp = 0;
  u1 *D = new(u1, 1024*1024*8);
  int B, I, J, L = 0;

  // we don't really know how the origianl RLE worked
  if (P->W*P->H <= 64000) uncomp = 1;

  s4bePut(D+L, CHK_FORM); L+=4;
  s4bePut(D+L, 0); L+=4;

  // PBM is chunky pixel type LBM, made by Deluxe Paint for DOS
  s4bePut(D+L, CHK_PBM); L+=4;

  s4bePut(D+L, CHK_BMHD); L+=4;
  s4bePut(D+L, 0x14); L+=4; //BMHD size
  s2bePut(D+L, P->W); L+=2;
  s2bePut(D+L, P->H); L+=2;
  s2bePut(D+L, 0); L+=2; // x-origin
  s2bePut(D+L, 0); L+=2; // y-origin
  D[L++] = P->B; // number of color planes
  D[L++] = 0; // color key type (mask)
  if (uncomp) {
    D[L++] = cmpNone; // compression type
  } else {
    D[L++] = cmpByteRun1; // compression type
  }
  D[L++] = 0; // color map flags
  s2bePut(D+L, 0xFF); L+=2; // color key
#if 1
  s2bePut(D+L, 0x0506); L+=2; // aspect ratio
  s2bePut(D+L, 320); L+=2; // page width
  s2bePut(D+L, 200); L+=2; // page height
#else
  s2bePut(D+L, 0x0101); L+=2; // aspect ratio
  s2bePut(D+L, 640); L+=2; // page width
  s2bePut(D+L, 480); L+=2; // page height
#endif

  s4bePut(D+L, CHK_CMAP); L+=4;
  s4bePut(D+L, 3*256); L+=4;
  times(I, 256) {
    D[L++] = P->P[I*4+0];
    D[L++] = P->P[I*4+1];
    D[L++] = P->P[I*4+2];
  }


  s4bePut(D+L, CHK_DPPS); L+=4; //Deluxe Pain Page Settings
  s4bePut(D+L, 110); L+=4;
  s2bePut(D+L+0, 2);
  s2bePut(D+L+14, 360);
  s2bePut(D+L+18, 320);
  s2bePut(D+L+20, 240);
  s2bePut(D+L+22, 2);
  s2bePut(D+L+24, 15);
  s2bePut(D+L+26, 1);
  s2bePut(D+L+30, 1);
  s2bePut(D+L+34, 1);

  s2bePut(D+L+74, 1);
  s2bePut(D+L+90, 1);
  s2bePut(D+L+106, 1);

  L += 110;

  //Deluxe Paint normally writes 4 CRNG chunks in an ILBM
  //when the user asks it to "Save Picture". 
  times (I,16) {
    s4bePut(D+L, CHK_CRNG); L+=4;
    s4bePut(D+L, 8); L+=4;

    s2bePut(D+L, 0); L+=2; // padding;
    s2bePut(D+L, 0); L+=2; // color cycle rate
    s2bePut(D+L, 0); L+=2; // cycle colors, if nonzero

    // here we select upper and lower color registers
    if (I == 0) {s2bePut(D+L, 0x101F); L+=2;}
    else if (I == 1) {s2bePut(D+L, 0x202F); L+=2;}
    else if (I == 2) {s2bePut(D+L, 0x606F); L+=2;}
    else if (I == 3) {s2bePut(D+L, 0x909F); L+=2;}
    else {s2bePut(D+L, 0x0000); L+=2;}
  }

#if 0
  s4bePut(D+L, CHK_TINY); L+=4;
  s4bePut(D+L, 0x100B); L+=4;
  L += 0x100B+1;
#endif

  s4bePut(D+L, CHK_BODY); L+=4;
  s4bePut(D+L, 0); L+=4;
  B = L;

  if (uncomp) {
    times(I, P->H) {
      memcpy(D+L, P->D + I*P->I, P->W);
      L += P->W;
      if (L&1) L++;
    }
  } else {
    times(I, P->H) {
      L += lbmLineRLE(D+L, P->D+I*P->I, (P->W+1)/2*2);
    }
  }

  s4bePut(D+4, L-8);
  s4bePut(D+B-4, L-B);

  if(L&1) D[L++] = 0;

  write_whole_file_path(FileName, D, L);

  free(D);
}

static void rev(void *X, int N) {
  int T, I;
  u1 *P = (u1*)X;
  times (I, N/2) {T = P[I]; P[I] = P[N-I-1]; P[N-I-1] = T;}
}


typedef struct {
  u4 Len;
  u2 W;
  u2 H;
  u2 X;
  u2 Y;
  u1 BPP;
  u1 ColorKeyType;
  u1 Enc;
  u1 CMAPFlags;
  u2 ColorKey;
  u2 AspectRatio;
  u2 PageWidth;
  u2 PageHeight;
} PACKED bmhd;


static u1 *decodeRLE(u1 *D, u1 *S, int ULen) {
  int C;
  u1 *End = D+ULen;

  while (D < End) {
    C = *S++;
    if (C < 128) {
      C += 1;
      while (C--) *D++ = *S++;
    } else if (C > 128) {
      C = 257-C;
      while (C--) *D++ = *S;
      *S++;
    } else {
       //printf("NOOP\n");
    }
  }

  return S;
}

#if 0
int lbm_unrle(uint8_t *src, uint8_t *dst, uint16_t len) {
  uint8_t *ps;
  uint8_t val;
  uint8_t run_val;
  uint8_t *pd;
  int i;
  uint8_t *end;
  uint8_t *pps;
  uint8_t *s;
  uint8_t *d;

  d = dst;
  s = src;
  do {
    do {
      pps = s++;
      val = *pps;
    } while (val == 0x80);
    if (val < 0x80) { /* copy as is */
      for (i = val + 1; i != 0; i--) *d++ = *s++;
    } else { /* duplicate */
      ps = s;
      s = pps + 2;
      run_val = *ps;
      for (i = (val^0xff) + 2; i != 0; i--) {
        ps = d;
        d = d + 1;
        *ps = run_val;
      }
    }
  } while (d < end);
  return s - src;
}
#endif

static const u1 bit_mask[] = {1, 2, 4, 8, 16, 32, 64, 128};
static u1 *decodeRow(u4 *D, u1 *S, bmhd *H) {
  int plane, col, cols, cbit, bytes, nPlanes;
  u1 *ilp;
  u1 *Line = new(u1,H->W*5);
  u4 *chp;

  cols = H->W;
  bytes = RowBytes(cols);
  nPlanes = H->BPP + (H->ColorKeyType==mskHasMask ? 1 : 0);
  for (plane = 0; plane < nPlanes; plane++) {
    int mask = 1 << plane;
    if (H->Enc == 1) S = decodeRLE(Line, S, bytes);
    else if (H->Enc == 2) {
      memcpy(Line, S, bytes);
      S += bytes;
    } else {
      printf("  Unknown encoding (%d)\n", H->Enc);
      abort();
    }

    ilp = Line;
    chp = D;

    cbit = 7;
    for(col = 0; col < cols; col++, cbit--, chp++) {
      if (cbit < 0) {
        cbit = 7;
        ilp++;
      }
      if(*ilp & (1<<cbit)) *chp |= mask;
      else *chp &= ~mask;
    }
  }

  return S;
}

// ILBMs are made for Amiga and have interleaved bitplanes
// PBMs made for PC and have all planes combined into bytes
pic_t *lbmLoad(char *Input) {
  int64_t L;
  u1 *M = read_whole_file(Input, &L);
  pic_t *P;
  u1 *Q, *Z, *End=M+L;
  u4 *Line;
  int I, J, K, N, X, Y, C, Len, NColors=0;
  u1 Type[5],*CMAP=0,*BMHD=0,*BODY=0,*CRNG=0,*TINY=0,*DPPS=0, *CAMG=0, *Pal=0;
  bmhd *H=0;

  if (L<16 || strncmp(M,"FORM",4)) {
    printf ("  Not an IFF file\n");
    abort();
  }

  for (Q = M+12; Q+8 < End; Q += Len) {
    memcpy(Type, Q, 4);
    Type[4] = 0;
    Q += 4;
    if      (!strcmp(Type,"BMHD")) BMHD = Q; // Bitmap Header
    else if (!strcmp(Type,"BODY")) BODY = Q; // Pixels
    else if (!strcmp(Type,"CMAP")) CMAP = Q; // Color Map
    else if (!strcmp(Type,"CRNG")) CRNG = Q; // Color Cycling Ranges
    else if (!strcmp(Type,"CAMG")) CAMG = Q; // Amiga Display Mode
    else if (!strcmp(Type,"TINY")) TINY = Q; // Thumbnail
    else if (!strcmp(Type,"DPPS")) DPPS = Q; // Deluxe Paint Program Settings
    else {
      printf ("  Unknown IFF chunk (%s)\n", Type);
      //break;
    }
    Len = ru4(Q);
    rev(&Len, 4);
    if (Len%2) Len++; // odd-sized chunks have a padding byte
    if (Len < 0) break;
  }

  if (CMAP) {
    Q = CMAP;
    Len = ru4(Q);
    rev(&Len, 4);
    NColors = Len/3;
    Pal = new(u1, max(256, NColors)*4);
    times (I, NColors) {
      Pal[I*4+0] = *Q++;
      Pal[I*4+1] = *Q++;
      Pal[I*4+2] = *Q++;
      Pal[I*4+3] = 0;
    }
  }

  if (BMHD) {
    H = (bmhd*)BMHD;
    rev(&H->Len,4);
    rev(&H->W,2);
    rev(&H->H,2);
    rev(&H->X,2);
    rev(&H->Y,2);
    rev(&H->ColorKey,2);

    printf("  %dx%dx%d:%d,%d:%d\n", H->W, H->H, H->BPP, H->X, H->Y, H->Enc);

    unless (H->BPP > 8 || Pal) {
      printf("  CMAP is missing\n");
      abort();
    }

    if (H->ColorKeyType == mskHasMask) {
      printf("  cant handle masked LBMs\n");
      abort();
    }

    P = picNew(H->W, H->H, H->BPP > 8 ? 32 : 8);

    // if images you decode gets garbled, use the following instead
    //P = picNew(H->W+H->W%2, H->H, H->BPP > 8 ? 32 : 8);

    unless (H->BPP > 8) memcpy(P->P, Pal, 256*4);
    Line = new(u4,H->W*4);
  } else {
    printf("  BMHD is missing\n");
    abort();
  }

  unless (BODY) {
    printf("  BODY is missing\n");
    abort();
  }


  if (NColors > 256) {
    printf("  Cant handle %d-color images\n", NColors);
    abort();
  }


  memcpy(Type, M+8, 4);
  Type[4] = 0;
  if (!strcmp(Type,"PBM ")) {
    Z = P->D;
    Q = BODY;
    Len = ru4(Q);
    if (H->BPP != 8) {
      printf("  Cant handle %d-bit PBM files\n", H->BPP);
      abort();
    } else {
      if (H->Enc == cmpNone) {
        times (Y, H->H) {
          memcpy(Z+Y*P->W, Q, H->W);
          Q += H->W + H->W%2; //every row is aligned
        }
      } else if (H->Enc == cmpByteRun1) {
        decodeRLE(P->D, Q, P->W*P->H);
      } else {
        printf("  Unknown encoding (%d)\n", H->Enc);
        abort();
      }
    }
  } else if (!strcmp(Type,"ILBM")) {
    Z = P->D;
    Q = BODY;
    Len = ru4(Q);
    if (H->BPP > 8) {
      printf("  Cant handle %d-bit ILBM files\n", H->BPP);
      abort();
    } else {
      times (Y, H->H) {
        Q = decodeRow(Line, Q, H);
        times (X, H->W) picPut(P, X, Y, Line[X]);
        //skip mask here
        Z += H->W;
      }
    }
  } else {
    printf("  Unsupported IFF file type (%s)\n", Type);
    abort();
  }

  return P;
}











#define WAVE_FORMAT_G723_ADPCM  	0x0014  	/* Antex Electronics Corporation */
#define WAVE_FORMAT_ANTEX_ADPCME 	0x0033 	/* Antex Electronics Corporation */
#define WAVE_FORMAT_G721_ADPCM 	0x0040 	/* Antex Electronics Corporation */
#define WAVE_FORMAT_APTX 	0x0025 	/* Audio Processing Technology */
#define WAVE_FORMAT_AUDIOFILE_AF36 	0x0024 	/* Audiofile, Inc. */
#define WAVE_FORMAT_AUDIOFILE_AF10 	0x0026 	/* Audiofile, Inc. */
#define WAVE_FORMAT_CONTROL_RES_VQLPC 	0x0034 	/* Control Resources Limited */
#define WAVE_FORMAT_CONTROL_RES_CR10 	0x0037 	/* Control Resources Limited */
#define WAVE_FORMAT_CREATIVE_ADPCM 	0x0200 	/* Creative Labs, Inc */
#define WAVE_FORMAT_DOLBY_AC2 	0x0030 	/* Dolby Laboratories */
#define WAVE_FORMAT_DSPGROUP_TRUESPEECH 	0x0022 	/* DSP Group, Inc */
#define WAVE_FORMAT_DIGISTD 	0x0015 	/* DSP Solutions, Inc. */
#define WAVE_FORMAT_DIGIFIX 	0x0016 	/* DSP Solutions, Inc. */
#define WAVE_FORMAT_DIGIREAL 	0x0035 	/* DSP Solutions, Inc. */
#define WAVE_FORMAT_DIGIADPCM 	0x0036 	/* DSP Solutions, Inc. */
#define WAVE_FORMAT_ECHOSC1 	0x0023 	/* Echo Speech Corporation */
#define WAVE_FORMAT_FM_TOWNS_SND 	0x0300 	/* Fujitsu Corp. */
#define WAVE_FORMAT_IBM_CVSD 	0x0005 	/* IBM Corporation */
#define WAVE_FORMAT_OLIGSM 	0x1000 	/* Ing C. Olivetti & C., S.p.A. */
#define WAVE_FORMAT_OLIADPCM 	0x1001 	/* Ing C. Olivetti & C., S.p.A. */
#define WAVE_FORMAT_OLICELP 	0x1002 	/* Ing C. Olivetti & C., S.p.A. */
#define WAVE_FORMAT_OLISBC 	0x1003 	/* Ing C. Olivetti & C., S.p.A. */
#define WAVE_FORMAT_OLIOPR 	0x1004 	/* Ing C. Olivetti & C., S.p.A. */
#define WAVE_FORMAT_IMA_ADPCM 	(WAVE_FORM_DVI_ADPCM) 	/* Intel Corporation */
#define WAVE_FORMAT_DVI_ADPCM 	0x0011 	/* Intel Corporation */
#define WAVE_FORMAT_UNKNOWN 	0x0000 	/* Microsoft Corporation */
#define WAVE_FORMAT_PCM 	0x0001 	/* Microsoft Corporation */
#define WAVE_FORMAT_ADPCM 	0x0002 	/* Microsoft Corporation */
#define WAVE_FORMAT_ALAW 	0x0006 	/* Microsoft Corporation */
#define WAVE_FORMAT_MULAW 	0x0007 	/* Microsoft Corporation */
#define WAVE_FORMAT_GSM610 	0x0031 	/* Microsoft Corporation */
#define WAVE_FORMAT_MPEG 	0x0050 	/* Microsoft Corporation */
#define WAVE_FORMAT_NMS_VBXADPCM 	0x0038 	/* Natural MicroSystems */
#define WAVE_FORMAT_OKI_ADPCM 	0x0010 	/* OKI */
#define WAVE_FORMAT_SIERRA_ADPCM 	0x0013 	/* Sierra Semiconductor Corp */
#define WAVE_FORMAT_SONARC 	0x0021 	/* Speech Compression */
#define WAVE_FORMAT_MEDIASPACE_ADPCM 	0x0012 	/* Videologic */
#define WAVE_FORMAT_YAMAHA_ADPCM 	0x0020 	/* Yamaha Corporation of America */

typedef struct {
  u1 Id[4]; //RIFF
  u4 Len;
  u1 Type[4]; //WAVE

  u1 FmtId[4];  //fmt
  u4 FmtLen;
  u2 Format;  //1=PCM
  u2 Chns; // channels
  u4 Freq; // sample rate
  u4 ByteRate;
  u2 Align;
  u2 Depth; // bits per sample

  u1 DataId[4]; //data
  u4 DataLen;
  //u1 Data[DataLen];
} PACKED wavHeader;


void wavSave(char *Output, u1 *Q, int L, int Bits, int Chns, int Freq) {
  int I;
  wavHeader W;
  FILE *OF;
  memcpy(W.Id, "RIFF", 4);
  W.Len = 36 + L;
  memcpy(W.Type, "WAVE", 4);
  memcpy(W.FmtId, "fmt ", 4);
  W.FmtLen = 16;
  W.Format = 1;
  W.Chns = Chns;
  W.Freq = Freq;
  W.Depth = Bits;
  W.ByteRate = W.Freq*W.Chns*W.Depth/8;
  W.Align = W.Chns*W.Depth/8;
  memcpy(W.DataId, "data", 4);
  W.DataLen = L;

  write_whole_file_path(Output, "", 0);
  OF = fopen(Output, "wb");
  fwrite(&W, 1, sizeof(wavHeader), OF);
  fwrite(Q, 1, L, OF);
  fclose(OF);
}


