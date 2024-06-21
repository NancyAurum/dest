#include "stdint.h"

struct cfax_t { /* return value in CF:AX */
    uint16_t value;
    uint8_t flag;
};

struct far_t { /* off,seg pair */
    uint16_t off;
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

