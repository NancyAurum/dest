// This file is in public domain --Nancy Sadkov

#ifndef X86DIS_H
#define X86DIS_H

#include "./common.h"

// structure for keeping decoded values
typedef struct {
	u4 val;
	int size;	// byte-size of a value (1, 2, or 4)
	bool sign; 	// signed/unsigned
} x86_val;


// keeps reference to memory from instruction [seg:base+index*scale+disp]
typedef struct {
	u4 seg;			// base segment
	u4 base;		// base register keeps start of data array
	u4 index;		// this register keeps index in data array at base
	u4 scale;		// scale for index (2, 4, 8, or 16)
	x86_val disp;	// displacement
	int size;		// size of referenced value in bytes
	bool sign;		// is referenced value signed
} x86_memref;

typedef enum {
	X86_ONON=0,		// not used
	X86_OREG,		// register
	X86_OFL4,		// 4-byte IEEE float
	X86_OFL8,		// 8-byte IEEE float
	X86_OVAL,		// value
	X86_OMEM		// memory reference
} x86_opr_types;

typedef struct {
	x86_opr_types tok;
	union {
		x86_memref mem;	// memory reference
		x86_val val;	// value
		u4 reg;			// register
	};
} x86_opr; // keeps decoded operand for x86 instruction (register/memory/immediate)

// structure for keeping decoded instruction (we can produce mnemo code from this one)
typedef struct {
	u4 tok;					// symbolic token (pointer into mnemonic table)
	u4 prefix;				// prefix token
	char *m;				// mnemonic
	int argc;				// number of operands to this instruction
	x86_opr argv[3];		// instruction operands
} x86_inst;

typedef enum {
	X86_UNUSED,
	X86_AAD,
	X86_AAM,
	X86_ADC,
	X86_ADD,
	X86_AND,
	X86_ARPL,
	X86_BOUND,
	X86_BSF,
	X86_BSR,
	X86_BSWAP,
	X86_BT,
	X86_BTC,
	X86_BTR,
	X86_BTS,
	X86_CALL,
	X86_CALLF,
	X86_CALLN,
	X86_CBW,
	X86_CFLUSH,
	X86_CLC,
	X86_CLD,
	X86_CLI,
	X86_CLTS,
	X86_CMC,
	X86_CMOVL, X86_CMOVNL, X86_CMOVLE, X86_CMOVG,
	X86_CMOVO, X86_CMOVNO, X86_CMOVB, X86_CMOVAE,
	X86_CMOVS, X86_CMOVNS, X86_CMOVP, X86_CMOVNP,
	X86_CMOVZ, X86_CMOVNZ, X86_CMOVBE, X86_CMOVA,
	X86_CMP,
	X86_CMPS,
	X86_CMPSB,
	X86_CMPXCH8B,
	X86_CMPXCHG,
	X86_CPUID,
	X86_CWD,
	X86_AAS,
	X86_AAA,
	X86_DAA,
	X86_DAS,
	X86_DEC,
	X86_DIV,
	X86_EMMS,
	X86_ENTER,
	X86_FXRSTOR,
	X86_FXSAVE,
	X86_HLT,
	X86_IDIV,
	X86_IMUL,
	X86_IN,
	X86_INC,
	X86_INS,
	X86_INSB,
	X86_INT,
	X86_INTO,
	X86_INVD,
	X86_INVLPG,
	X86_IRET,
	X86_JCXZ,
	X86_LAHF,
	X86_LAR,
	X86_LDMXCSR,
	X86_LDS,
	X86_LEA,
	X86_LEAVE,
	X86_LES,
	X86_LFS,
	X86_LGDT,
	X86_LGS,
	X86_LIDT,
	X86_LLDT,
	X86_LMSW,
	X86_LODS,
	X86_LODSB,
	X86_LOOP,
	X86_LOOPNZ,
	X86_LOOPZ,
	X86_LSL,
	X86_LSS,
	X86_LTR,
	X86_MOV,
	X86_MOVNTI,
	X86_MOVS,
	X86_MOVSB,
	X86_MOVSX,
	X86_MOVZX,
	X86_MUL,
	X86_NEG,
	X86_NOP,
	X86_NOT,
	X86_OR,
	X86_OUT,
	X86_OUTS,
	X86_OUTSB,
	X86_POP,
	X86_POPA,
	X86_PREFETCHNTA,
	X86_PREFETCHT0,
	X86_PREFETCHT1,
	X86_PREFETCHT2,
	X86_PUSH,
	X86_PUSHA,
	X86_RCL,
	X86_RCR,
	X86_RDMSR,
	X86_RDPMC,
	X86_RDTSC,
	X86_RET,
	X86_RETF,
	X86_ROL,
	X86_ROR,
	X86_RSM,
	X86_SAHF,
	X86_SAR,
	X86_SBB,
	X86_SCAS,
	X86_SCASB,
	X86_SETL, X86_SETNL, X86_SETLE, X86_SETG,
	X86_SETO, X86_SETNO, X86_SETB, X86_SETAE,
	X86_SETS, X86_SETNS, X86_SETP, X86_SETNP,
	X86_SETZ, X86_SETNZ, X86_SETBE, X86_SETA,
	X86_SGDT,
	X86_SHL,
	X86_SHLD,
	X86_SHR,
	X86_SHRD,
	X86_SIDT,
	X86_SLDT,
	X86_SMSW,
	X86_STC,
	X86_STD,
	X86_STI,
	X86_STMXCSR,
	X86_STOS,
	X86_STOSB,
	X86_STR,
	X86_SUB,
	X86_SYSENTER,
	X86_SYSEXIT,
	X86_TEST,
	X86_UD2,
	X86_VERR,
	X86_VERW,
	X86_WAIT,
	X86_WBINVD,
	X86_WRMSR,
	X86_XADD,
	X86_XCHG,
	X86_XLAT,
	X86_XOR,

	// goto
	X86_JMPF,
	X86_JMP,
	X86_JMPN,

	// if(cond) goto...
	X86_JL, X86_JGE, X86_JLE, X86_JG,
	X86_JO, X86_JNO, X86_JB,  X86_JAE,
	X86_JS, X86_JNS, X86_JP,  X86_JNP,
	X86_JZ, X86_JNZ, X86_JBE, X86_JA,

	X86_SSE,
	X86_NH,
} x86_instoks;

#define X86_DIS_NH		-1

// decodes one x86 instruction into 'ins' and returns its size
void x86init(); // setup local opcode tables
int x86dis(x86_inst *ins, u1 *p, bool bits32);
// converts x86_inst to mnemonic
char *x86mnemonize(char *out, x86_inst *inst, bool att);

#endif

