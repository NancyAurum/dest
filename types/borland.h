#include "stdint.h"
#include "x86.h"

struct bofh_t { // Bolrand FB File Header
    uint16_t id0; /* magic id, always FB */
    uint16_t id1; /* magic id, usually OV */
    uint32_t size; /* size in bytes of the FBOV section excluding its header */
    uint32_t stofs; /* offset to the boseg_t array from the start of MZ header */
    int32_t nsegs; /* number of the boseg_t entries at stofs */
};


struct boseg_t { /* Borland __SEGTABLE__ entry */
    uint16_t seg; /* segment (coincides with the MZ reloc table segments) */
    uint16_t maxoff; /* -1 - Ignored by the linker's OvrCodeReloc */
    uint16_t flags; /* 1=CODE,2=OVR,4=DATA */
    uint16_t minoff;
};

struct bosh_t { /* Borland Segment Header */
    uint16_t code;
    uint16_t saveret;
    int32_t fileofs;
    uint16_t codesz;
    uint16_t fixupsz;
    uint16_t jmpcnt;
    uint16_t link;
    uint16_t bufseg;
    uint16_t retrycnt;
    uint16_t next;
    uint16_t ems_page;
    uint16_t ems_ofs;
    uint8_t user[6];
};

struct boinit_t { /* Entry in the Borlands's startup's table of init functions */
    uint8_t calltype; /* 0=near,1=far,ff=not used */
    uint8_t priority; /* 0=highest,ff=lowest */
    pointer32 addr;
};

struct FILE {
    int16_t level; /* fill/empty level of buffer */
    uint16_t flags; /* File status flags */
    int8_t fd; /* File descriptor */
    uint8_t hold; /* Ungetc char if no buffer */
    uint16_t bsize; /* Buffer size */
    uint32_t buffer; /* Data transfer buffer */
    uint32_t curp; /* Current active pointer */
    uint16_t istemp; /* Temporary file indicator */
    int16_t token; /* Used for validity checking */
};


struct VIDEOREC { /* current video state */
    uint8_t windowx1;
    uint8_t windowy1;
    uint8_t windowx2;
    uint8_t windowy2;
    uint8_t attribute;
    uint8_t normattr;
    uint8_t currmode;
    uint8_t screenheight;
    uint8_t screenwidth;
    uint8_t graphicsmode;
    uint8_t snow;
    struct far_t displayptr;
};

