/* Additional DOS structures, not defined by <dos.h> */

#ifndef STDOS_H
#define STDOS_H

#include "x86.h"



typedef struct dos_psp_t dos_psp_t;
struct dos_psp_t {
  uint16_t cpm_call0; /* CP/M-80-like termination (int 20h) */
  uint16_t himem; /* memory above the relocated EXE */
  uint8_t unused1;
  uint8_t cpm_call5[5];
  struct far_t termhndlr; /* parent's terminate handler */
  struct far_t breakhndlr; /* parent's crtl/break handler */
  struct far_t crithndlr; /* parent's critical error handler */
  uint16_t parent_psp;
  uint8_t jft[20]; /* job file table entries (FF = available) */
  uint16_t envseg; /* segment with environment vars */
  struct far_t int21_ss_sp;
  uint16_t jft_size; /* DOS 3+ number of entries in JFT (default 20) */
  struct far_t jft_pointer; /* DOS 3+ pointer to JFT (default PSP:0018h) */
  struct far_t previous_psp;
  uint8_t call_int21[3];
  uint8_t unused2;
  uint8_t dosver[2]; /* DOS 5+ version to return on INT 21/AH=30h  */
  uint16_t win3x_pdb_next_psp;
  uint16_t win3x_pdb_partition;
  uint16_t win3x_pdb_next_pdb;
  uint8_t win3x_winoldapp;
  uint8_t unused3[3];
  uint16_t win3x_pdb_entry_stack;
  uint8_t unused4[2];
  uint8_t unix_call50[3];
  uint8_t unused5[2];
  uint8_t efcb1[7]; /* used to extend fcb1 */
  uint8_t fcb1[16]; /* 1st CP/M-style FCB */
  uint8_t fcb2[16]; /* 2nd CP/M-style FCB */
  uint8_t extra[4]; /* overflow from FCBs */
  uint8_t cmdlen; /* length of command line parameters */
  uint8_t cmdline[127]; /* command line parameters */
};

typedef struct dos_open_t dos_open_t;
struct dos_open_t { /* CF:AX result of calling DOS_OpenFile */
  uint16_t handle;
  uint8_t error;
};

typedef struct dos_read_t dos_read_t;
struct dos_read_t { /* result of calling DOS_ReadFile */
  uint16_t readed;
  uint8_t error;
};


typedef struct dos_setfileposrslt_t dos_setfileposrslt_t;
struct dos_setfileposrslt_t { /* result of calling DOS_SetFilePosition */
  int32_t position;
  uint8_t error;
};

typedef struct dosver_t dosver_t;
struct dosver_t {
  uint8_t major;
  uint8_t minor;
};



#endif

