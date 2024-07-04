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
} __attribute__ ((packed)) wavHeader;


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


