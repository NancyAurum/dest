#ifndef _ST_H
#define _ST_H

#include "x86.h"

//silences the `parameter 'PPP' is never used in function FFF` warnings
//alternative is #pragma argsused, which is non-portable
#define UNUSED (void)

/* Ghidra defs */
typedef uint8_t byte;
typedef uint8_t bool;
#define true 1
#define false 0
//#define NULL ((void*)0)
#define CONCAT22(a,b) (((uint32_t)(uint16_t)(a)<<16)|(uint16_t)(b))

//x._1_2_
#define _12(x) ((uint16_t*)(x))[0]
#define _22(x) ((uint16_t*)(x))[1]


//Equals to MAXPATH from stdlib.h
#define STBUFSZ 80

/* Global Code */
void init_game();
extern uint8_t rand8();
extern void srand8(uint32_t seed);
extern char *litoa10(char *buf, int32_t n);
extern void init_check_mem();
extern void print_available_memory();

extern int open_wait_unlock(char *filename);
extern int creat_wait_unlock(char *filename);
extern char *strip_tail_slash(char *s);
extern void cd_to_path(char *path);

extern int biosKeyAvail();
extern uint16_t biosReadKey();


/* Global Data */
extern char GWorkDir[];
extern char *GGameExe;
extern char *GGameArg;

#endif
