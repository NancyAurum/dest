#include <string.h>
#include <stdlib.h>

//MAXDIR, getcurdir, etc...
#include <dir.h>

//open, creat
#include <fcntl.h>

#include <dos.h>

#include "st.h"
#include "stdos.h"

/* Don't ask me about memmove; - just preserving the original algorithm.
                                                             --Nancy */
char *litoa10(char *buf, int32_t n) {
  bool sign;
  size_t len;
  char digit;
  
  if ((0xffff < n) || ((-1 < n && (true)))) {
    sign = false;
  } else {
    sign = true;
    n = CONCAT22(-(n != 0)-_22(n),-n);
  }
  digit = n%10;
  *buf = digit + '0';
  buf[1] = 0;
  while( true ) {
    digit = n/10;
    if (digit == 0) break;
    len = strlen(buf);
    if ((len & 3) == 3) {
      len = strlen(buf);
      memmove(buf + 1,buf,len + 1);
      *buf = ',';
    }
    len = strlen(buf);
    memmove(buf + 1,buf,len + 1);
    n = n/10;
    digit = n%10;
    *buf = digit + '0';
  }
  if (sign) {
    len = strlen(buf);
    memmove(buf + 1,buf,len + 1);
    *buf = '-';
  }
  return buf;
}

/*
unsigned _bios_keybrd(unsigned cmd)
{
//  Clear zero flag
    asm xor  al, al

    asm mov  ah, cmd
    asm int  16h

//  If zero flag set then no key is waiting
    asm jz   nokey

//  If we aren't checking status, just return key
    asm test byte ptr (cmd), 1
    asm jnz  keydone

//  Here we have status command and key waiting
//  If keycode is zero (control-break) then signal with 0FFFFh
    asm or   ax, ax
    asm jnz  keydone
    asm mov  ax, 0FFFFh
    asm jmp  keydone

nokey:
//  Zero flag wasn't set, if not checking status just return key
    asm test byte ptr (cmd), 1
    asm jz   keydone

//  Here we have status command and no key waiting
    asm xor  ax, ax

keydone:
    return _AX;
}

*/

uint16_t biosReadKey() {
  asm xor ah,ah
  asm int 16h
  return _AX;
}

int biosKeyAvail() {
  asm mov ah, 1
  asm int 16h
  asm mov ax, 0
  asm jz  nokey
  asm mov ax, 1
nokey:
  return _AX;
}

//DOS_GetExtendedErrorInfo return codes, set for int 24h handler
#define ERROR_SHARING_VIOLATION 0x20
#define ERROR_DEV_NOT_EXIST     0x37


#define DOS_GetExtendedErrorInfo  0x59
#define DOS_GetInterruptHandler   0x3524
#define DOS_SetInterruptHandler   0x2524

//void (*far pfn)();
//pfn = &ioErrorHndlr;

far_t GSavedErrorHandler;

uint16_t GIOErrorCode = 0x1234;
uint8_t GIOErrorRetryCount = 1;


extern void ioErrorHndlr();

//replace critical error handler to handle our file errors
void ioErrorHndlrInstall() {
  //far_t hndlr;
  //hndlr.ofs = (uint16_t)(void*)&ioErrorHndlr;
  //hndlr.seg = (uint16_t)(void _seg*)ioErrorHndlr;

  //save old handler
  asm {
    MOV        AX,DOS_GetInterruptHandler
    INT        21h
    MOV        word ptr [GSavedErrorHandler.ofs],BX
    MOV        word ptr [GSavedErrorHandler.seg],ES
  }
  //GSavedErrorHandler.ofs = _BX;
  //GSavedErrorHandler.seg = _ES;

  //install our handler
  asm {
    PUSH      DS
    PUSH      ES
    MOV       DX,word ptr ioErrorHndlr
    MOV       DS,word ptr seg ioErrorHndlr
    //MOV       DX,word ptr [hndlr.ofs]
    //MOV       DS,word ptr [hndlr.seg]
    MOV       AX,DOS_SetInterruptHandler
    INT       0x21
    POP       ES
    POP       DS
  }
  //printf("%04X:%04X\n", GSavedErrorHandler.ofs, GSavedErrorHandler.seg); for(;;);
}

int ioErrorHndlrUninstall() {
  asm {
    PUSH      DS
    PUSH      ES
    MOV       DX,word ptr [GSavedErrorHandler.ofs]
    MOV       DS,word ptr [GSavedErrorHandler.seg]
    MOV       AX,DOS_SetInterruptHandler
    INT       0x21
    POP       ES
    POP       DS
  }
  return GIOErrorCode;
}



//waits when file is unlocked and opens it
//currently ioErrorHndlrInstall and ioErrorHndlrUninstall and not decompiled
int open_wait_unlock(char *filename) {
  int handle;
  uint16_t ioerr;
  do {
    ioErrorHndlrInstall();
    /* O_DENYNONE = Allow concurrent access  */
    handle = _open(filename,O_RDONLY|O_DENYNONE);
    ioerr = ioErrorHndlrUninstall();
    if (handle != -1) return handle;
    /* while sharing violation error */
  } while (ioerr == ERROR_SHARING_VIOLATION);
  return -1;
}

int open_wait_unlock_and_disk(char *filename) {
  int handle;
  uint16_t ioerr;
  
  do {
    ioErrorHndlrInstall();
    /* O_DENYNONE = Allow concurrent access  */
    handle = _open(filename,O_RDONLY|O_DENYNONE);
    ioerr = ioErrorHndlrUninstall();
    if (handle != -1) return handle;
  } while (ioerr == ERROR_SHARING_VIOLATION || ioerr == ERROR_DEV_NOT_EXIST);
  return -1;
}

int creat_wait_unlock(char *filename) {
  int handle;
  uint16_t ioerr;

  do {
    ioErrorHndlrInstall();
    // _creat is basically open with O_CREAT, O_TRUNC and O_WRONLY flags.
    handle = _creat(filename,_A_NORMAL); //create file with default permissions
    ioerr = ioErrorHndlrUninstall();
    if (handle != -1) return handle;
  } while (ioerr == ERROR_SHARING_VIOLATION);
  return -1;
}


int creat_wait_unlock_and_disk(char *filename) {
  int handle;
  uint16_t ioerr;
  
  do {
    ioErrorHndlrInstall();
    handle = _creat(filename, _A_NORMAL);
    ioerr = ioErrorHndlrUninstall();
    if (handle != -1) return handle;
  } while (ioerr == ERROR_SHARING_VIOLATION || ioerr == ERROR_DEV_NOT_EXIST);
  return -1;
}

char *strip_tail_slash(char *s) {
  size_t l = strlen(s);
  if ((3 < l) && (s[l-1] == '\\')) s[l-1] = 0;
  return s;
}

void chpath(char *path) { //cuz chdir doesn't do setdisk
  if (chdir(path) == 0) {
    char c = toupper(*path);
    if (path[1] == ':' && 'A' <= c && (c <= 'Z')) setdisk(c - 'A');
  }
  return;
}



// ChatGPT claims rand8 resembles the:
// 8-bit Maxim Integrated (formerly Dallas Semiconductor) one-wire CRC algorithm

//48-bit shift register
static uint8_t s[6]; //state

uint8_t rand8() { //1000:0730
  int i,c;
  uint8_t *src, *dst;
  //sum 1st, 4th and 5th bytes into the 0th byte
  c = s[1] + s[4] + 1;
  if (c&0x100) c = (c&0xFF) + 1;
  s[0] = (c + s[5])&0xFF;

  //shift right by 8 bytes
  src = s + 4;
  dst = s + 5;
  for (i = 5; i != 0; i = --i) *dst-- = *src--;

  return s[0];
}

void srand8(uint32_t seed) { //1000:0730
  int i;
  uint8_t *pseed = (uint8_t*)&seed;
  s[0] = 0;
  s[1] = pseed[0] ^ 0x55;
  s[2] = pseed[1] ^ 0x55;
  s[3] = pseed[2] ^ 0x55;
  s[4] = pseed[3] ^ 0x55;
  s[5] = 0;
  for (i = 0; i < 200; i++) rand8();
  return;
}

#if 0
#include <stdio.h>
int test_rand8() {
  int i;
  srand8(100);
  for (i = 0; i < 300; i++) printf("%03d: %d\n", i, rand8());
  return 0;
}
#endif
