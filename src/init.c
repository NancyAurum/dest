#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>


//_psp
#include <dos.h>

//getcwd, MAXPATH
#include <dir.h>

//open, creat
#include <fcntl.h>

//clrscr
#include <conio.h>

#include "st.h"
#include "stdos.h"

#define DIRSZ (MAXDIR+2)
char GStaticBuffer[116];
char GOriginalDir[DIRSZ];
char GSaveFileName[] = "strong.sav\\";
char GWorkDir[MAXPATH];


void print_available_memory() {
  char buf[STBUFSZ];
  dos_psp_t _seg *pPSP = (void _seg*)_psp;
  int32_t avail = ((int32_t)(pPSP->himem - _psp) + 1) << 4;
  avail += 0x100;  /* add psp size */
  printf("Conventional memory available: %s\n", litoa10(buf, avail));
}


void init_check_mem() {
  char buf[STBUFSZ];
  dos_psp_t _seg *pPSP = (void _seg*)_psp;
  int32_t avail = ((int32_t)(pPSP->himem - _psp) + 1) << 4;
  avail += 0x100;  /* add psp size */
  if (avail < 585000) {
    printf("Largest executable program size is only %s bytes.\n"
          , litoa10(buf,avail));
    printf("Stronghold requires 585,000 bytes\n");
    exit(0x84);
  } //else print_available_memory();
}

#define SAV_NO_CFG  0
#define SAV_HAS_CFG 1

char cfg_read_char(int handle) {
  char c, cc;
  int nread = read(handle,&c,1);
  if (nread != 1) return 0;
  do { //skip till new line
    nread = read(handle,&cc,1);
    if (nread != 1) return 0;
  } while (cc != '\n');
  return c;
}

//again, crufty code with '}' repeating two times...
//BTW, Borland C++ 3.0 ctype.h had isprint(c)
byte is_printable(char *buf, char c) {
  UNUSED(buf);
  return isalnum(c) || c == ':' || c == '.' || c == '\\' || c == '}' || c == '!'
    || c == '@' || c == '#' || c == '$' || c == '%' || c == '^' || c == '&'
    || c == '(' || c == ')' || c == '-' || c == '_' || c == '{' || c == '}'
    || c == '~' || c == '`';
}


#define KEY_ESCAPE     0x011B
#define KEY_ENTER      0x1C0D
#define KEY_BACKSPACE  0x0E08


#define KEY_ASCII(x) (char)((x)&0xFF)
#define KEY_CODE(x) (((x)>>8)&0xFF)

void input_folder_name(char *result) {
  char c;
  uint16_t key;
  int len;
  
  len = 0;
  result[0] = '\0';
  while( true ) {
    key = biosReadKey();
    if (key == KEY_ESCAPE) {
      printf("\n");
      exit(0);
    }
    if (key == KEY_ENTER) break;
   
    if (key == KEY_BACKSPACE && len) {
      printf("\b \b"); //print backspace
      result[len + -1] = 0;
      return;
    }
    if (len < MAXDIR-1) {
      c = KEY_ASCII(key);
      if (is_printable(result,c)) {
        printf("%c", c); /* Echo the key */
        result[len++] = c;
        result[len] = 0;
      }
    }
  }
  return;
}


int init_strong_sav() {
  int file;
  char *path;
  char path_buffer[MAXPATH];
  char c;
  
  do {
    file = open_wait_unlock("strong.sav\\game.cfg");
    if (file > -1) {
      /* Check if it is a valid cfg file */
      c = cfg_read_char(file);
      if (('1'<=c&&c<='9') || ('A'<=c && c<='B')) {
        c = cfg_read_char(file);
        if (('1'<=c&&c<='9') || ('A'<=c && c<='B')) {
          c = cfg_read_char(file);
          if (('1'<=(c)&&(c)<='9') || ('A'<=c && c<='B')) {
            close(file);
            return SAV_HAS_CFG; 
          }
        }
      }
      close(file);
      break;
    }
    mkdir("strong.sav"); /* create new save folder */

    /* make a temporary file */
    file = creat_wait_unlock("strong.sav\\strong.$$$");
    if (file > -1) {
      close(file);
      unlink("strong.sav\\strong.$$$");
      break;
    }
    clrscr();
    printf("Path to write game data:\n");
    input_folder_name(path_buffer);
    path = strip_tail_slash(path_buffer);
    chpath(path);
  } while ( true );
  return SAV_NO_CFG;
}

//virtual dos machine interface 0x2F:0x1600 interrupt results
#define VDM_NONE      0
#define VDM_OTHER_VM  0x01
#define VDM_SYSTEM_VM 0xFF
#define VDM_HIMEM_SYS 0x80
#define NO_VDM(r) ((r)==VDM_NONE || (r) == VDM_HIMEM_SYS)
uint8_t vdm_query() {
  asm {
    mov ax, 0x1600
    int 0x2f
    xor ah, ah
  }
  return _AL;
}

#define ENTER (char)0x0D
#define ESC (char)0x1B
#define SPACE (char)0x20

//discard all keys in buffer
#define BIOS_CLEAR_KEYS while (biosKeyAvail()) biosReadKey()

#define INPUT_KEY() do {       \
  BIOS_CLEAR_KEYS;             \
  key = biosReadKey();         \
  if (KEY_ASCII(key) == ESC) { \
    printf("\n");              \
    exit(0);                   \
  }                            \
} while (0)

#define KV KEY_ASCII(key)


#define CRD_PCSPK      '1'
#define CRD_SB         '2'
#define CRD_NONE       '3'
#define CRD_PCSPK_VDM  '4'
#define CRD_SB_PRO2    '5' 
#define CRD_SB_PRO1    '6' 
#define CRD_PAS_PLUS   '7'
#define CRD_PAS_16     '8'
#define CRD_ADLIB      '9'
#define CRD_ADLIB_GOLD 'A'
#define CRD_ROLAND     'B'

static char GGCTemplate[] = "0\n";

// creates new game.cfg
bool create_game_cfg(char sfx,char mus,char irq) {
  byte r;
  int handle;
  size_t sz;
  size_t l;
  
  mkdir("strong.sav");
  handle = creat_wait_unlock("strong.sav\\game.cfg");
  if (handle < 0) return false;
  GGCTemplate[0] = sfx;
  sz = strlen(GGCTemplate);
  sz = write(handle,GGCTemplate,sz);
  l = strlen(GGCTemplate);
  if (sz == l) {
    GGCTemplate[0] = mus;
    sz = strlen(GGCTemplate);
    sz = write(handle,GGCTemplate,sz);
    l = strlen(GGCTemplate);
    if (sz == l) {
      GGCTemplate[0] = irq;
      sz = strlen(GGCTemplate);
      sz = write(handle,GGCTemplate,sz);
      l = strlen(GGCTemplate);
      r = close(handle);
      if (sz == l && !r) {
        clrscr();
        return true;
      }
    }
  }
  return false;
}

uint16_t cfg_mice = 1;
char cfg_irq = 0;
char cfg_sfx = 0;
char cfg_mus = 0;

void init_base() {
  char irq,mus,sfx;
  uint16_t key;
  init_check_mem();
  getcwd(GWorkDir,MAXPATH);
  //printf("CWD: %s\n", GWorkDir);
  if (init_strong_sav() == SAV_HAS_CFG) {
    BIOS_CLEAR_KEYS;
    while( true ) {
      clrscr();
      printf(
        "ENTER: use previous configuration, SPACE: reconfigure, ESC: quit"
      );
      key = biosReadKey();
      if (KV == ESC) {
        printf("\n");
        exit(0);
      }
    
      if (KV == SPACE) break;
    
      if (KV == ENTER) { 
        clrscr();
        return;
      }
    }
  }

  clrscr();
  printf("Sound (1. PC Speaker  2. Sound Blaster  3. Pro Audio Spectrum\n");
  printf("       4. Adlib       5. Roland         6. Off):");
  irq = '0';
  while ( true ) {
    INPUT_KEY();
    if (KV == '1') {
      uint8_t r = vdm_query();
      sfx = NO_VDM(r) ? CRD_PCSPK : CRD_PCSPK_VDM;
      mus = CRD_NONE;
      goto pick_irq;
    }
    if (KV == '2') {
      clrscr();
      printf("Sound Blaster (1. Original  2.  Pro Type 2  3. Pro Type 1):");
      irq = '7';
      while( true ) {
        INPUT_KEY();
        if (KV == '1') {
          sfx = CRD_SB;
          mus = CRD_SB;
          goto pick_irq;
        }
        if (KV == '2') {
          sfx = CRD_SB_PRO2;
          mus = CRD_SB_PRO2;
          goto pick_irq;
        }
        if (KV == '3') {
          sfx = CRD_SB_PRO1;
          mus = CRD_SB_PRO1;
          goto pick_irq;
        }
      }
    }
    if (KV == '3') {
      clrscr();
      printf("Pro Audio Spectrum (1. Plus  2. Spectrum 16):");
      while ( true ) {
        INPUT_KEY();
        if (KV == '1') {
          sfx = CRD_PAS_PLUS;
          mus = CRD_PAS_PLUS;
          goto pick_irq;
        }
        if (KV == '2') {
          sfx = CRD_PAS_16;
          mus = CRD_PAS_16;
          irq = '5';
          goto pick_irq;
        }
      }
    }
    if (KV == '4') {
      clrscr();
      printf("Adlib (1. Original  2. Gold):");
      while ( true ) {
        INPUT_KEY();
        if (KV == '1') {
          sfx = CRD_ADLIB;
          mus = CRD_ADLIB;
          goto pick_irq;
        }
        if ( KV == '2') {
          sfx = CRD_ADLIB_GOLD;
          mus = CRD_ADLIB_GOLD;
          goto pick_irq;
        }
      }
    }
    if (KV == '5') {
      mus = CRD_ROLAND;
      clrscr();
      printf(
       "Sound Effects (1. PC Speaker  2. Sound Blaster  3. Pro Audio Spectrum\n"
      );
      printf(
       "               4. Adlib       5. None):");
      while( true ) {
        INPUT_KEY();
        if (KV == '1') {
          uint8_t r = vdm_query();
          sfx = NO_VDM(r) ? CRD_PCSPK : CRD_PCSPK_VDM;
          goto pick_irq;
        }
        if (KV == '2') {
          clrscr();
          printf("Sound Blaster (1. Original  2.  Pro):");
          while (true ) {
            irq = '7';
            INPUT_KEY();
            if (KV == '1') {
              sfx = CRD_SB;
              goto pick_irq;
            }
            if (KV == '2') {
              sfx = CRD_SB_PRO2;
              goto pick_irq;
            }
          }
        }
        if (KV == '3') {
          clrscr();
          printf("Pro Audio Spectrum (1. Plus  2. Spectrum 16):");
          while (KV != '2') {
            INPUT_KEY();
            if (KV == '1') {
              sfx = CRD_PAS_PLUS;
              goto pick_irq;
            }
            if (KV == '2') {
              sfx = CRD_PAS_16;
              irq = '5';
             goto pick_irq;
            }
          }
        }
        if (KV == '4') {
          clrscr();
          printf("Adlib (1. Original  2. Gold):");
          while ( true ) {
            INPUT_KEY();
            if (KV == '1') {
              sfx = CRD_ADLIB;
              goto pick_irq;
            }
            if (KV == '2') {
              sfx = CRD_ADLIB_GOLD;
              goto pick_irq;
            }
          }
        }
        if (KV == '5') {
          sfx = CRD_NONE;
          goto pick_irq;
        }
      }
    }
    if (KV == '6') {
      sfx = CRD_NONE;
      mus = CRD_NONE;
      goto pick_irq;
    }
  }


pick_irq:
  if (irq != '0') {
    clrscr();
    printf("Sound IRQ is %c (1. Don't change  2. IRQ 2  3. IRQ 5\n", irq);
    printf("                                 4. IRQ 7  5. IRQ 10):");
    while ( true ) {
      INPUT_KEY();
      if (KV == '1') break;
      if (KV == '2') { irq = '2'; break; }
      if (KV == '3') { irq = '5'; break; }
      if (KV == '4') { irq = '7'; break; }
      if (KV == '5') { irq = 'A'; break; }
    }
  }
done:
  clrscr();
  if (create_game_cfg(sfx,mus,irq)) return;

  exit(0x83);
}

char *ensure_tail_slash(char *s) {
  size_t l = strlen(s);
  if (!l || s[l-1] != '\\') {
    s[l] = '\\';
    s[l+1] = 0;
  }
  return s;
}


char *append_name_to_path(char *path, char *name) {
  size_t l;  
  strcpy(GStaticBuffer,path);
  l = strlen(GStaticBuffer);
  for (; 0 < l && GStaticBuffer[l-1] != '\\'; l--);
  GStaticBuffer[l] = 0;
  strcat(GStaticBuffer,name);
  return GStaticBuffer;
}


void init_paths(void) {
  char *full_path;
  getcwd(GOriginalDir,DIRSZ);
  strcpy(GWorkDir,GOriginalDir);
  ensure_tail_slash(GWorkDir);
  strcat(GWorkDir,GSaveFileName);
  full_path = append_name_to_path(GGameExe,GSaveFileName + 0xb);
  full_path = strip_tail_slash(full_path);
  chpath(full_path);
  //printf("%s | %s | %s\n", full_path, GGameExe, GWorkDir);
  return;
}

/*
char *stsprintf(char *buf, char *fmt, char *arg1) {
  vsprintf(buf,fmt,&arg1);
  return buf;
}
*/

char *stsprintf (char *buf,  const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsprintf(buf,fmt, args);
  va_end(args);
  return buf;
}

char GCrLf[] = {0x0D,0x0A};

void error(int code) {
  exit(code ? 0x80 : 0);
}

char read_cfg(int handle) {
  char tmp;
  char b;
  
  if (read(handle,&b,1) != 1) return 0;
  do { //cr-lf
    if (read(handle,&tmp,1) != 1) return 0;
  } while (tmp != '\n');
  return b;
}

void init_cfg_mice() {
  cfg_mice = 1;
}


void init_cfg() {
  char *cfgPath;
  int fh;
  char lbuf[80];
  byte goterr;
  uint8_t update;
  uint16_t crlf;
  char mice;
  char c;
  
  update = 0;
  fh = open_wait_unlock_and_disk(stsprintf(lbuf,"%sgame.cfg", GWorkDir));
  if (fh < 0) error(0x80);

  if (!(c = read_cfg(fh))) error(0x80);
  if ((c < '1' || '9' < c) && (c < 'A' || 'B' < c)) {
    if (c < 'a' || 'b' < c) error(0x80);
    c = toupper(c);
  }
  cfg_mus = c;

  if (!(c = read_cfg(fh))) error(0x80);
  if ((c < '1' || '9' < c) && (c < 'A' || 'B' < c)) {
    if (c < 'a' || 'b' < c) error(0x80);
    c = toupper(c);
  }
  cfg_sfx = c;

  if (!(c = read_cfg(fh))) error(0x80);
  if ((c < '0' || '9' < c) && (c < 'A' || 'F' < c)) error(0x80);
  cfg_irq = c;

  mice = read_cfg(fh);
  if (mice < '1' || '2' < mice) {
    init_cfg_mice();
    update = 1;
    mice = cfg_mice ? mice = '1' : '2';
  } else if (mice == '1') cfg_mice = 1;
  else cfg_mice = 0;

  close(fh);

  if (update) {
    fh = creat_wait_unlock_and_disk(stsprintf(lbuf,"%sgame.cfg",GWorkDir));
    if (fh < 0) error(0x83);
    //again, crufty code where |= is missing for two statements
    goterr =  write(fh,&cfg_mus,1) != 1;
    goterr |= write(fh,GCrLf   ,2) != 2;
    goterr =  write(fh,&cfg_sfx,1) != 1;
    goterr |= write(fh,GCrLf   ,2) != 2;
    goterr =  write(fh,&cfg_irq,1) != 1;
    goterr |= write(fh,GCrLf   ,2) != 2;
    goterr |= write(fh,&mice   ,1) != 1;
    goterr |= write(fh,GCrLf   ,2) != 2;
    goterr |= close(fh);
    if (goterr) error(0x80);
    clrscr();
  }
  return;
}


void init_game() {
  //_OvrInitExt(0,0);
  //_OvrInitEms(0,0,0);
  init_base();
  init_paths();
  init_cfg();
}
