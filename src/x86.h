#ifndef _STX86_H
#define _STX86_H

#include <stdint.h>

//#define N2F(p) MK_FP(FP_SEG(p), FP_OFF(p))


typedef struct far_t far_t;
struct far_t { /* off,seg pair */
    uint16_t ofs;
    uint16_t seg;
};

struct DW { /* dword hi:low pair */
    uint16_t lo;
    uint16_t hi;
};

struct LH { /* word hi:low pair */
    uint8_t lo;
    uint8_t hi;
};

struct cfax_t { /* return value in CF:AX */
    uint16_t value;
    uint8_t flag;
};

#endif