// This file is in public domain --Nancy Sadkov

#include <stdio.h>
#include <string.h>

#include "./common.h"
#include "./x86dis.h"

// following tokens are not part of interface. they are specific to decoder
#define X86_PREFIX_ADROVR			(1<<PREFIX_OFF)
#define X86_PREFIX_OPROVR			(2<<PREFIX_OFF)
#define X86_PREFIX_REPE				(3<<PREFIX_OFF)
#define X86_PREFIX_REPNE			(4<<PREFIX_OFF)
#define X86_PREFIX_SEGOVR			(5<<PREFIX_OFF)
#define X86_PREFIX_0F				(6<<PREFIX_OFF)
#define X86_PREFIX_LOCK				(7<<PREFIX_OFF)

#define X86_SEE						X86_UNUSED

#define SIZE_BITS 4
enum {BYTE=1, WORD=2, DWORD=4, ADDR=5, QWORD=8};

// in decoder we will use higher bits of token to keep info, like flags affected
#define PREFIX_BITS 3
#define PREFIX_OFF (32-PREFIX_BITS)
#define PREFIX_MASK	(MASKN(PREFIX_BITS)<<PREFIX_OFF)

// format of opcode flags descriptor:
// ---------------------------
// ssss suuu uuuu uuuu ffff
// f - flags
// u - unused
// s - transfer size (BYTE/WORD/DWORD/VAR), for immediate
#define OT(a)		((a)<<(32-SIZE_BITS)) /*create operand type everride*/
#define GETTYPE(a)	((a)>>(32-SIZE_BITS)) /*get operand type everride*/

// opcode flags
#define IMM			0x01 /* instruction has immediate value */
#define MODRM		0x02 /* instruction has modrm */
#define SSE			0x04 /* instruction requires SSE support */
#define NDOS		0x08 /* name depends on size */
#define SPEC		0x10 /* instructions behaves in non-standard way */
#define EXT			0x20 /* instruction is extended by modrm */

// format of operand descriptor:
// ---------------------------
// ssss ssuu uuuu uumt tttt
// m - register or mode (REGOP)
// u - unused
// s - transfer size (BYTE/WORD/DWORD), if not set, default to current size
#define OPRTOK_BITS		8							/*operand token bits*/
#define ISREG(a)		((a)<G)
#define ISMODRM(a)		((a)<A)
#define ISMODRM_REG(a)	((a)<M)
#define OPRTOK(a)		((a)&MASKN(OPRTOK_BITS))	/* get operand token from operand descriptor*/

// A direct address, as immediate value
// C modrm::reg selects control register
// D modrm::reg selects debug register
// E modrm::rm specifies operand - gpr or memory(sib)
// G modrm::reg selects gp register
// I operand that follows opcode
// J operand is immediate offset to be added to ip
// M modrm::rm may only refer to memory
// O offset of operand follows instruction
// P modrm::reg selects MMX register
// Q modrm::rm specifies operand - MMX register or memory(sib)
// R modrm::rm may only refer to gpr
// S modrm::reg selects segment register
// T modrm::reg selects test register
// V modrm::reg selects XMM register
// W modrm::rm specifies operand - XMM register or memory(sib)
// X memory adressed by DS:SI pair
// Y memory adressed by ES:DI pair

enum {
	eAX = 1, eCX, eDX, eBX, eSP, eBP, eSI, eDI, ES, CS, SS, DS, GS, FS, eFLAGS,
	AX, CX, DX, BX, SP, BP, SI, DI, AL, CL, DL, BL, AH, CH, DH, BH,
	CR0, CR1, CR2, CR3, CR4, CR5, CR6, CR7,
	TR0, TR1, TR2, TR3, TR4, TR5, TR6, TR7,
	DR0, DR1, DR2, DR3, DR4, DR5, DR6, DR7,
	MMR0, MMR1, MMR2, MMR3, MMR4, MMR5, MMR6, MMR7,
	G=0x80, S, D, C, T, P, V, M, R, E, Q, W,
	A=0x90, I, J, O, X, Y, CONST_0, CONST_LAST=CONST_0+200,
}; // operand types


char *reglut[] = {
	"",
	"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "es", "cs", "ss", "ds", "gs", "fs", "eflags",
	"ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
	"cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7",
	"tr0", "tr1", "tr2", "tr3", "tr4", "tr5", "tr6", "tr7",
	"dr0", "dr1", "dr2", "dr3", "dr4", "dr5", "dr6", "dr7",
	"mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7",
	"xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
};


typedef struct {
	char m[15];			// mnemonic
	u4 t;				// token
	u4 o1;				// 1st operand descriptor
	u4 o2;				// 2nd operand descriptor
	u4 f;				// general flags
	u4 o3;				// 3rd operand descriptor
	int immsz;
	int argc;
} opcode; // opcode descriptor

#define m(a) #a,X86_##a
static opcode map[] = {
#define d(a) {m(a),E|OT(BYTE),G|OT(BYTE)}, {m(a),E,G}, {m(a),G|OT(BYTE),E|OT(BYTE)},\
	{m(a),G,E}, {m(a),eAX|OT(BYTE),I|OT(BYTE)}, {m(a),eAX,I}

	// 00-0F
	d(ADD),	{m(PUSH),ES}, {m(POP),ES}, d(OR), {m(PUSH),CS}, {m(PREFIX_0F)},

	// 10-1F
	d(ADC), {m(PUSH),SS}, {m(POP),SS}, d(SBB), {m(PUSH), DS}, {m(POP), DS},

	// 20-2F
	d(AND), {m(PREFIX_SEGOVR),ES}, {m(DAA)}, d(SUB), {m(PREFIX_SEGOVR),CS}, {m(DAS)},

	// 30-3F
	d(XOR), {m(PREFIX_SEGOVR),SS}, {m(AAA)}, d(CMP), {m(PREFIX_SEGOVR),DS}, {m(AAS)},
#undef d

#define d(a)\
	{m(a),eAX}, {m(a),eCX}, {m(a),eDX}, {m(a),eBX},\
	{m(a),eSP}, {m(a),eBP}, {m(a),eSI}, {m(a),eDI}
	// 40-5F
	d(INC), d(DEC), d(PUSH), d(POP),
#undef d

	// 60-6F
	{m(PUSHA)},							{m(POPA)},
	{m(BOUND),G,M|OT(ADDR)},			{m(ARPL),E|OT(WORD),G|OT(WORD)},
	{m(PREFIX_SEGOVR),FS},				{m(PREFIX_SEGOVR),GS},
	{m(PREFIX_OPROVR)},					{m(PREFIX_ADROVR)},
	// IMUL(G,I,E) is G=E*I
	{m(PUSH),I},						{m(IMUL),G,I,0,E},
	{m(PUSH),I|OT(BYTE)},				{m(IMUL),G,I|OT(BYTE),0,E},
	{m(INSB),Y,eDX}, 					{m(INS),Y,eDX},
	{m(OUTSB),eDX,X},					{m(OUTS),eDX,X},

	// 70-7F
	{m(JO),J|OT(BYTE)}, {m(JNO),J|OT(BYTE)},
	{m(JB),J|OT(BYTE)},  {m(JAE),J|OT(BYTE)},
	{m(JZ),J|OT(BYTE)}, {m(JNZ),J|OT(BYTE)},
	{m(JBE),J|OT(BYTE)}, {m(JA),J|OT(BYTE)}, 
	{m(JS),J|OT(BYTE)}, {m(JNS),J|OT(BYTE)},
	{m(JP),J|OT(BYTE)},  {m(JNP),I|OT(BYTE)},
	{m(JL),J|OT(BYTE)}, {m(JGE),J|OT(BYTE)},
	{m(JLE),J|OT(BYTE)}, {m(JG),J|OT(BYTE)}, 

	// 80-8F
	{"",0,E|OT(BYTE),I|OT(BYTE),IMM|MODRM|EXT,0},	{"",0,E,I,IMM|MODRM|EXT,0},
	{"",0,E|OT(BYTE),I|OT(BYTE),IMM|MODRM|EXT,0},	{"",0,E,I|OT(BYTE),IMM|MODRM|EXT,0},
	{m(TEST),E|OT(BYTE),G|OT(BYTE)},	{m(TEST),E,G},
	{m(XCHG),E|OT(BYTE),G|OT(BYTE)},	{m(XCHG),E,G},

	{m(MOV),E|OT(BYTE),G|OT(BYTE)},		{m(MOV),E,G},
	{m(MOV),G|OT(BYTE),E|OT(BYTE)},		{m(MOV),G,E},
	{m(MOV),E|OT(WORD),S|OT(WORD)},		{m(LEA),G,M},
	{m(MOV),S|OT(BYTE),E|OT(WORD)},		{m(POP),E},
	// 90-9F
	{m(NOP)},			{m(XCHG),eAX,eCX},	{m(XCHG),eAX,eDX}, {m(XCHG),eAX,eBX},
	{m(XCHG),eAX,eSP},	{m(XCHG),eAX,eBP},	{m(XCHG),eAX,eSI}, {m(XCHG),eAX,eDI},
	{m(CBW)},			{m(CWD)},			{m(CALLF),A},	{m(WAIT)},
	{m(PUSH),eFLAGS},	{m(POP),eFLAGS},	{m(SAHF)}, 		{m(LAHF)},
	// A0-AF
	{m(MOV),AL,O|OT(BYTE)},				{m(MOV),eAX,O},
	{m(MOV),O|OT(BYTE),AL},				{m(MOV),O,eAX},
	{m(MOVSB),Y,X,SPEC},				{m(MOVS),Y,X,SPEC|NDOS},
	{m(CMPSB),Y,X,SPEC},				{m(CMPS),X,Y,SPEC|NDOS},
	{m(TEST),AL,I|OT(BYTE)},			{m(TEST),eAX,I},
	{m(STOSB),Y|OT(BYTE),AL,SPEC},		{m(STOS),Y,eAX,SPEC|NDOS},
	{m(LODSB),AL,X|OT(BYTE),SPEC},		{m(LODS),eAX,X,SPEC|NDOS},
	{m(SCASB),AL,Y|OT(BYTE),SPEC},		{m(SCAS),eAX,Y,SPEC|NDOS},
	// B0-BF
	{m(MOV),AL,I|OT(BYTE)},				{m(MOV),CL,I|OT(BYTE)},
	{m(MOV),DL,I|OT(BYTE)},				{m(MOV),BL,I|OT(BYTE)}, 
	{m(MOV),AH,I|OT(BYTE)},				{m(MOV),CH,I|OT(BYTE)},
	{m(MOV),DH,I|OT(BYTE)},				{m(MOV),BH,I|OT(BYTE)}, 
	{m(MOV),eAX,I},						{m(MOV),eCX,I},
	{m(MOV),eDX,I},						{m(MOV),eBX,I}, 
	{m(MOV),eSP,I},						{m(MOV),eBP,I},
	{m(MOV),eSI,I},						{m(MOV),eDI,I}, 
	// C0-CF
	{"",0,E|OT(BYTE),I|OT(BYTE),EXT,1},	{"",0,E,I|OT(BYTE),EXT,1},
	{m(RET),I|OT(WORD)},				{m(RET)},
	{m(LES),G,M},						{m(LDS),G,M},
	{"",0,E|OT(BYTE),I|OT(BYTE),EXT,10},	{"",0,E,I,EXT,10},
	{m(ENTER),0,0,SPEC},				{m(LEAVE)},
	{m(RETF),I|OT(BYTE)},				{m(RETF)},
	{m(INT),(CONST_0+3)|OT(BYTE),0,0},	{m(INT),I,0,1},
	{m(INTO)},							{m(IRET)},
	// D0-DF
	{"",0,E|OT(BYTE),(CONST_0+1)|OT(BYTE),EXT|1,1},	{"",0,E,(CONST_0+1)|OT(BYTE),EXT,1},
	{"",0,E|OT(BYTE),CL,EXT,1},					{"",0,E,eCX|OT(1),EXT,1},
	{m(AAM),I|OT(BYTE)},						{m(AAD),I|OT(BYTE)},
	{m(NH)},									{m(XLAT)},
	// (floating point)
	{m(NH),0,0,MODRM}, {m(NH),0,0,MODRM},		{m(NH),0,0,MODRM}, {m(NH),0,0,MODRM},
	{m(NH),0,0,MODRM}, {m(NH),0,0,MODRM},		{m(NH),0,0,MODRM}, {m(NH),0,0,MODRM},
	// E0-EF
	{m(LOOPNZ),J|OT(BYTE)},				{m(LOOPZ),J|OT(BYTE)},
	{m(LOOP),J|OT(BYTE)},				{m(JCXZ),J|OT(BYTE)},
	{m(IN),AL,I|OT(BYTE)},				{m(IN),eAX,I|OT(BYTE)},
	{m(OUT),I|OT(BYTE),AL},				{m(OUT),I|OT(BYTE),eAX}, 
	{m(CALL),J},						{m(JMP),J},
	{m(JMP),A},							{m(JMP),J|OT(BYTE)},
	{m(IN),AL,DX},						{m(IN),eAX,DX},
	{m(OUT),DX,AL},						{m(OUT),DX,eAX}, 
	// F0-F7
	{m(PREFIX_LOCK)},	{m(NH)},	{m(PREFIX_REPNE)},			{m(PREFIX_REPE)},
	{m(HLT)},			{m(CMC)},	{"",0,E|OT(BYTE),0,EXT,2},	{"",0,E,0,EXT,2},
	// F8-FF
	{m(CLC)}, {m(STC)},	{m(CLI)},			{m(STI)},
	{m(CLD)}, {m(STD)}, {"",0,0,0,EXT,3},	{"",0,0,0,EXT,4}
};

/**********************************************************
 *********************************************************/

/* Secondary map for when first opcode is 0F */
static opcode map0f[] = {
	// 00-0F
	{"",0,0,0,EXT,5}, {"",0,0,0,EXT,6}, {m(LAR),G,E|OT(WORD)}, {m(LSL),G,E|OT(WORD)},
	{m(NH)}, {m(NH)}, {m(CLTS)}, {m(NH)},
	{m(INVD)}, {m(WBINVD)}, {m(NH)}, {m(UD2)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)},
	// 10-1F
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{"",0,0,0,EXT,15}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)},
	// 20-2F
	{m(MOV),R|OT(DWORD),C|OT(DWORD)},		{m(MOV),R|OT(DWORD),D|OT(DWORD)},
	{m(MOV),C|OT(DWORD),R|OT(DWORD)},		{m(MOV),D|OT(DWORD),R|OT(DWORD)}, 
	{m(MOV),R|OT(DWORD),T|OT(DWORD)}, 		{m(NH)},
	{m(MOV),T|OT(DWORD),R|OT(DWORD)},		{m(NH)},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	// 30-3F
	{m(WRMSR)}, {m(RDTSC)}, {m(RDMSR)}, {m(RDPMC)},
	{m(SYSENTER)}, {m(SYSEXIT)}, {m(NH)}, {m(NH)},
	{m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(MOVNTI),G,E}, {m(NH)}, {m(NH)}, {m(NH)},
	// 40-4F
	{m(CMOVO),G,E}, {m(CMOVNO),G,E}, {m(CMOVB),G,E},  {m(CMOVAE),G,E},
	{m(CMOVZ),G,E}, {m(CMOVNZ),G,E}, {m(CMOVBE),G,E}, {m(CMOVA),G,E},
	{m(CMOVS),G,E}, {m(CMOVNS),G,E}, {m(CMOVP),G,E},  {m(CMOVNP),G,E},
	{m(CMOVL),G,E}, {m(CMOVNL),G,E}, {m(CMOVLE),G,E}, {m(CMOVG),G,E},
	// 50-5F
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SEE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	// 60-6F
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SEE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	// 70-7F
	{m(SSE),0,0,SSE}, {"",0,0,0,SSE|EXT,11},
	{"",0,0,0,SSE|EXT,12}, {"",0,0,0,SSE|EXT,13},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(EMMS),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SEE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	// 80-8F
	{m(JO),J}, {m(JNO),J}, {m(JB),J},  {m(JAE),J},
	{m(JZ),J}, {m(JNZ),J}, {m(JBE),J}, {m(JA),J},
	{m(JS),J}, {m(JNS),J}, {m(JP),J},  {m(JNP),J},
	{m(JL),J}, {m(JGE),J}, {m(JLE),J}, {m(JG),J},
	// 90-9F setcc
	{m(SETO),E|OT(BYTE)},			{m(SETNO),E|OT(BYTE)},
	{m(SETB),E|OT(BYTE)},			{m(SETAE),E|OT(BYTE)},
	{m(SETZ),E|OT(BYTE)},			{m(SETNZ),E|OT(BYTE)},
	{m(SETBE),E|OT(BYTE)},			{m(SETA),E|OT(BYTE)},
	{m(SETS),E|OT(BYTE)},			{m(SETNS),E|OT(BYTE)},
	{m(SETP),E|OT(BYTE)}, 			{m(SETNP),E|OT(BYTE)},
	{m(SETL),E|OT(BYTE)},			{m(SETNL),E|OT(BYTE)},
	{m(SETLE),E|OT(BYTE)},			{m(SETG),E|OT(BYTE)},
	// A0-AF
	{m(PUSH),FS}, {m(POP),FS}, {m(CPUID)}, {m(BT),E,G},
	{m(SHLD),E,I|OT(BYTE),0,G}, {m(SHLD),E,CL,0,G}, {m(NH)}, {m(NH)},
	{m(PUSH),GS}, {m(POP),GS}, {m(RSM)}, {m(BTS),E,G},
	{m(SHRD),E,I|OT(BYTE),0,G}, {m(SHRD),E,CL,0,G},
	{"",0,0,0,EXT,14},{m(IMUL),G,E},
	// SHLD(a,b,c) == a<<c|(b>>(NBITSIN(b)-c))
	// B0-BF
	{m(CMPXCHG),E|OT(BYTE),G|OT(BYTE)}, {m(CMPXCHG),E,G}, {m(LSS),M},{m(BTR),E,G},
	{m(LFS),M}, {m(LGS),M}, {m(MOVZX),G,E|OT(BYTE)}, {m(MOVZX),G,E|OT(WORD)},
	{m(NH)}, {"",0,0,0,EXT,9}, {"",0,E,I|OT(BYTE),7}, {m(BTC),E,G},
	{m(BSF),G,E}, {m(BSR),G,E}, {m(MOVSX),G,E|OT(WORD)}, {m(MOVSX),G,E|OT(WORD)},
	// C0-CF
	{m(XADD),E|OT(BYTE),G|OT(BYTE)}, {m(XADD),E,G},
	{m(SSE),0,0,SSE}, {m(MOVNTI),E|OT(DWORD),G|OT(DWORD)},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {"",0,0,0,EXT|SSE,8}, 
	{m(BSWAP),eAX}, {m(BSWAP),eCX}, {m(BSWAP),eDX}, {m(BSWAP),eBX},
	{m(BSWAP),eSP}, {m(BSWAP),eBP}, {m(BSWAP),eSI}, {m(BSWAP),eDI},
	// D0-DF
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	// E0-EF
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	// F0-FF
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE}
};

// opcode map for instructions extended by modrm
static opcode mapext[] = {
	// grp1
	{m(ADD)}, {m(OR)}, {m(ADC)}, {m(SBB)}, {m(AND)}, {m(SUB)}, {m(XOR)}, {m(CMP)},
	// grp2
	{m(ROL)}, {m(ROR)}, {m(RCL)}, {m(RCR)}, {m(SHL)}, {m(SHR)}, {m(NH)}, {m(SAR)},
	// grp3
	{m(TEST),I}, {m(NH)}, {m(NOT)}, {m(NEG)},
	{m(MUL),eAX}, {m(IMUL),eAX}, {m(DIV),eAX}, {m(IDIV),eAX},
	// grp4
	{m(INC),E|OT(BYTE)}, {m(DEC),E|OT(BYTE)}, {m(NH)}, {m(NH)},
	{m(NH)}, {m(NH)}, {m(NH)}, {m(NH)},
	// grp5
	{m(INC),E}, {m(DEC),E}, {m(CALLN),E}, {m(CALLF),E},
	{m(JMPN),E}, {m(JMPF),E}, {m(PUSH),E}, {m(NH)},
	// grp6
	{m(SLDT),E|OT(WORD)}, {m(STR),E}, {m(LLDT),E|OT(WORD)}, {m(LTR),E|OT(WORD)},
	{m(VERR),E|OT(WORD)}, {m(VERW),E|OT(WORD)}, {m(NH)}, {m(NH)},
	// grp7
	{m(SGDT),M}, {m(SIDT),M}, {m(LGDT),M}, {m(LIDT),M},
	{m(SMSW),E|OT(WORD)}, {m(NH)}, {m(LMSW),E|OT(WORD)}, {m(INVLPG),M|OT(BYTE)},
	// grp8
	{m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(BT)}, {m(BTS)}, {m(BTR)}, {m(BTC)},
	// grp9
	{m(NH)}, {m(CMPXCH8B),M|OT(QWORD)}, {m(NH)}, {m(NH)},
	{m(NH)}, {m(NH)}, {m(NH)}, {m(NH)},
	// grp10
	{m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)},
	// grp11
	{m(MOV)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)}, {m(NH)},
	// grp12
	{m(NH)}, {m(NH)}, {m(SSE),0,0,SSE}, {m(NH)},
	{m(SSE),0,0,SSE}, {m(NH)}, {m(SSE),0,0,SSE}, {m(NH)},
	// grp13
	{m(NH)}, {m(NH)}, {m(SSE),0,0,SSE}, {m(NH)},
	{m(SSE),0,0,SSE}, {m(NH)}, {m(SSE),0,0,SSE}, {m(NH)},
	// grp14
	{m(NH)}, {m(NH)}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	{m(NH)}, {m(NH)}, {m(SSE),0,0,SSE}, {m(SSE),0,0,SSE},
	// grp15
	{m(FXSAVE)}, {m(FXRSTOR)}, {m(LDMXCSR)}, {m(STMXCSR)},
	{m(NH)}, {m(NH),0,0}, {m(NH),0,0}, {m(CFLUSH),0,0},
	// grp16
	{m(PREFETCHNTA)}, {m(PREFETCHT0)}, {m(PREFETCHT1)}, {m(PREFETCHT2)},
	{m(NH)}, {m(NH)}, {m(NH)}, {m(NH)},
};
#undef m




#define MOD(a) (((a)&0xc0)>>6)
#define REG(a) (((a)&0x38)>>3)
#define RM(a) ((a)&0x07)

// internal decoding state for one instruction
typedef struct {
	int segovr; 		// segment override
	bool adr32; 		// address override (32 || 48)
	bool opr32; 		// operand size override 
	x86_val imm;		// immediate
	x86_memref mem;
	u1 modrm;			// mod/rm reg

	u4 o1, o2, o3, of;

	u1 *p, *q;
} decstruct;

// decodes operand
static void decopr(x86_opr *out, decstruct *d, u4 o) {
	u4 t = OPRTOK(o);

	if(ISREG(o)) {
isreg:
		if(t < ES) { // gpr
			int s = GETTYPE(o);
			if(!d->opr32 || s == WORD) t += AX-eAX; // 16-bit register
			else if(s == BYTE) t += AL-eAX; // 8-bit register
		}
		out->tok = X86_OREG;
		out->reg = t;
	} else if(ISMODRM(t)) {
		if(ISMODRM_REG(t) || R == t || (M != t && MOD(d->modrm) == 3)) {
			o = ISMODRM_REG(t) ? REG(d->modrm) : RM(d->modrm);
			if(S == t) o += ES; // segment register
			else if(T == t) o += TR0;	// test register
			else if(D == t) o += DR0;	// debug register
			else o += eAX;
			t = o;
			goto isreg;
		}
		// else it is a memory reference
		memcpy(&out->mem, &d->mem, sizeof(d->mem));
		out->tok = X86_OMEM;
		out->mem.size = GETTYPE(o);
	} else switch(t) {
		case I:
			out->tok = X86_OVAL;
			memcpy(&out->val, &d->imm, sizeof(d->imm));
			break;
		case J:
			out->tok = X86_OVAL;
			memcpy(&out->val, &d->imm, sizeof(d->imm));
			out->val.sign = 1;
			break;
		case O: case A: // direct address or ds:offset
			out->tok = X86_OMEM;
			if(t == A) out->mem.disp.val = LSB16(d->q)*16;
			else out->mem.seg = DS; // offset in ds
			d->q += 2;
			if(d->adr32) {
				out->mem.disp.val += LSB32(d->q);
				d->q += 4;
			} else {
				out->mem.disp.val += LSB16(d->q);
				d->q += 2;
			}
			if(!(out->mem.size = GETTYPE(o)))
				out->mem.size = d->opr32 ? DWORD : WORD;
			break;
		case X: case Y: // string instruction
			break;
		default:
			if(t >= CONST_0 && t <= CONST_LAST) {
				out->tok = X86_OVAL;
				out->val.val = t-CONST_0;
				out->val.sign = 0;
				out->val.size = GETTYPE(o);
			} else out->tok = X86_ONON;
			break;
	}
	return;
}

int x86dis(x86_inst *ins, u1 *p, bool bits32) {
	decstruct d;
	opcode *o; // opcode byte

	memset(ins, 0, sizeof(*ins));
	memset(&d, 0, sizeof(d));
	d.opr32 = bits32;
	d.adr32 = bits32;
	d.q = d.p = p;

	o = &map[*d.q++];
	while(o->t&PREFIX_MASK) { // while prefix byte
		switch(o->t) {
			case X86_PREFIX_SEGOVR:
				d.segovr = o->o1;
				break;
			case X86_PREFIX_OPROVR:	d.opr32 = !d.opr32;; break;
			case X86_PREFIX_ADROVR:	d.adr32 = !d.adr32; break;
			case X86_PREFIX_REPE: case X86_PREFIX_REPNE:
						ins->prefix = o->t;
						break;
			default: break;
		}
		if(o->t == X86_PREFIX_0F) o = &map0f[*d.q++];
		else o = &map[*d.q++];
	}

	d.o1 = o->o1;
	d.o2 = o->o2;
	d.o3 = o->o3;
	d.of = o->f;

	if(d.of & MODRM) { // instruction has mod/rm byte
		int mod, rm, reg;
		d.modrm = *d.q++;
		mod = MOD(d.modrm);
		rm = RM(d.modrm);
		reg = REG(d.modrm);
		if(d.of & EXT) { // opcode is extended by modrm
			o = &mapext[(d.o3<<3)|reg];
			d.o1 |= o->o1;
			d.o2 |= o->o2;
			d.o3 = o->o3;
			d.of |= o->f;
		}
		memset(&d.mem, 0, sizeof(d.mem));

		if(mod != 3) {
			d.mem.seg = DS;
			if(d.opr32) { // 32-bit case
				if(rm == 4) { // has sib byte
					u1 sib = *d.q++;
					if(mod == 0 && RM(sib) == 5) { // ds:flat_offset_32bit
						d.mem.disp.val = LSB32(d.q);
						d.mem.disp.sign = 0;
						d.mem.disp.size = 4;
						d.q += 4;
					}
					if(REG(sib) != 4) { // has scaled index
						d.mem.index = REG(sib);
						d.mem.scale = 2<<MOD(sib);
					}
					d.mem.base = RM(sib)+eAX;
				} else if(mod || rm != 5)
					d.mem.base = reg+eAX; //yangzhixuan says "reg should be rm"

				if(mod == 1) {
					d.mem.disp.val = *d.q++;
					d.mem.disp.sign = 1;
					d.mem.disp.size = 1;
				} else if(mod == 2) {
					d.mem.disp.val = LSB32(d.q);
					d.mem.disp.sign = 0;
					d.mem.disp.size = 4;
					d.q += 4;
				} else if(mod == 0 && rm == 5) { // ds:flat_offset_32bit
					d.mem.disp.val = LSB32(d.q);
					d.mem.disp.sign = 0;
					d.mem.disp.size = 4;
					d.q += 4;
				}
			} else { // 16-bit case
				if(mod == 0 && rm == 6) { // ds:flat_offset_16bit
					d.mem.disp.val = LSB16(d.q);
					d.mem.disp.sign = 0;
					d.mem.disp.size = 2;
					d.q += 2;
				} else {
					if(mod == 1) {
						d.mem.disp.val = *d.q++;
						d.mem.disp.sign = 1;
						d.mem.disp.size = 1;
					} else if(mod == 2) {
						d.mem.disp.val = LSB16(d.q);
						d.mem.disp.sign = 0;
						d.mem.disp.size = 2;
						d.q += 2;
					}
					switch(rm) {
						case 0: // [BX+SI]
							d.mem.base = BX;
							d.mem.index = SI;
							break;
						case 1: // [BX+SI]
							d.mem.base = BX;
							d.mem.index = DI;
							break;
						case 2: // [BP+SI]
							d.mem.base = BP;
							d.mem.index = SI;
							break;
						case 3: // [BP+DI]
							d.mem.base = BP;
							d.mem.index = DI;
							break;
						case 4: // [SI]
							d.mem.base = SI;
							break;
						case 5: // [DI]
							d.mem.base = DI;
							break;
						case 6: // [SS:BP]
							d.mem.base = BP;
							d.mem.seg = SS; // BP defaults to SS
							break;
						case 7: // [BX]
							d.mem.base = BX;
							break;
						default:
							break;
					}
					if(d.segovr) d.mem.seg = d.segovr;
				}
			}
		}
	}

	if(o->t == X86_NH) return X86_DIS_NH;

	if(d.of & IMM) { // instruction takes immediate value
		if(BYTE == o->immsz) { // 1-byte immediate
			d.imm.val = *d.q++;
			d.imm.size = 1;
		} else if(d.opr32 && WORD != o->immsz) {
			d.imm.val = LSB32(d.q);
			d.imm.size = 4;
			d.q += 4;
		} else {
			d.imm.val = LSB16(d.q);
			d.imm.size = 2;
			d.q += 2;
		}
	}


	//printf("%s\n", o->m); fflush(stdout);
	//printf("hello\n"); fflush(stdout);
	if(d.of & SPEC) {
		switch(o->t) {
			case X86_ENTER:
				ins->argc = 2;
				ins->argv[0].tok = X86_OVAL;
				ins->argv[0].val.val = LSB16(d.q);
				ins->argv[0].val.size = WORD;
				d.q += 2;
				ins->argv[0].tok = X86_OVAL;
				ins->argv[0].val.val = *d.q++;
				ins->argv[0].val.size = BYTE;
				break;
			default:
				break;
		}
		/*if(strcmp(o->m, "lea")) {

		}	
		else*/;
	} else if(d.o1) {
		decopr(&ins->argv[0], &d, d.o1);
		if(d.o2) {
			decopr(&ins->argv[1], &d, d.o2);
			if(d.o3) {
				decopr(&ins->argv[2], &d, d.o3);
				ins->argc = 3;
			} else ins->argc = 2;
		} else ins->argc = 1;
	} else ins->argc = 0;
	ins->tok = o->t;
	ins->m = o->m;

	return (int)(d.q-d.p);
}

char *val2dec(char *b, x86_val *val) {
	int size = val->size;
	u4 v = val->val;
	if(val->sign || v < 10) {
		if(size == 1) sprintf(b, "%d", (int)(s1)(u1)v);
		else if(size == 2) sprintf(b, "%d", (int)(s2)(u2)v);
		else if(size == 4) sprintf(b, "%d", (int)(s4)(u4)v);
	} else {
		if(size == 1) sprintf(b, "%xh", (uint)(u1)v);
		else if(size == 2) sprintf(b, "%xh", (uint)(u2)v);
		else if(size == 4) sprintf(b, "%xh", (uint)(u4)v);
		if(*b >= 'a' && *b <= 'z') { // add 0, so we wont confuse numver with variable
			char *q = b;
			char c = '0';
			for(; *q; q++) {
				char t = *q;
				*q = c;
				c = t;
			}
		}
	}
	return b;
}

static char *opr2mnemo(char *buf, x86_opr *o, bool att) {
	char *r = buf;
	switch(o->tok) {
	case X86_OREG:
		sprintf(r, att ? "%%%s" : "%s", reglut[o->reg]);
		break;
	case X86_OMEM: {
		char d[20], scale[5], sob[5];
		char *disp = o->mem.disp.val ? val2dec(d, &o->mem.disp) : 0;
		char *base = o->mem.base ? reglut[o->mem.base] : 0;
		char *index = o->mem.index ? reglut[o->mem.index] : 0;
		char *so = "";
		if(o->mem.scale) sprintf(scale, att ? ",%d" : "*%d", o->mem.scale);
		else *scale = 0;
		if(o->mem.seg != DS || o->mem.base == BP) {
			so = o->mem.seg ? reglut[o->mem.seg] : "";
			if(att) {
				sprintf(sob, "%%%s:", so);
				so = sob;
			} else {
				sprintf(sob, "%s:", so);
				so = sob;
			}
		}
		if(att) { // AT&T notation
			if(disp) {
				if(base) {
					if(index) sprintf(r, "%s%s(%%%s,%%%s%s)", so, disp, base,
							index, scale);
					else sprintf(r, "%s%s(%%%s)", so, disp, base);
				} else {
					if(index) sprintf(r, "%s%s(,%%%s%s)", so, disp, index, scale);
					else sprintf(r, "%s%s", so, disp);
				}
			} else {
				if(base) {
					if(index) sprintf(r, "%s(%%%s,%%%s%s)", so, base, index, scale);
					else sprintf(r, "%s(%%%s)", so, base);
				} else {
					if(index) sprintf(r, "%s(,%%%s%s)", so, index, scale);
					else sprintf(r, "%s0", so);
				}
			}
		}
		else { // Intel notation
			if(disp) {
				if(base) {
					if(index) sprintf(r, "[%s%s+%s%s+%s]", so, base, index, scale, disp);
					else sprintf(r, "[%s%s+%s]", so, base, disp);
				} else {
					if(index) sprintf(r, "[%s%s%s+%s]", so, index, scale, disp);
					else sprintf(r, "[%s%s]", so, disp);
				}
			} else {
				if(base) {
					if(index) sprintf(r, "[%s%s+%s%s]", so, base, index, scale);
					else sprintf(r, "[%s%s]", so, base);
				} else {
					if(index) sprintf(r, "[%s%s%s]", so, index, scale);
					else sprintf(r, "[%s0]", so);
				}
			}
		}
		break;
		}
	case X86_OVAL: {
		char d[20];
		sprintf(r, "%s%s", att ? "$" : "", val2dec(d, &o->val));
		break;
	}
	default:
		sprintf(r, "<invalid>");
		break;
	}

	return r;
}

char *x86mnemonize(char *out, x86_inst *inst, bool att) {
	char o1b[32], o2b[32];
	char *prefix = "";
	if(inst->prefix == X86_PREFIX_REPE) prefix = "repe ";
	else if(inst->prefix == X86_PREFIX_REPNE) prefix = "repne ";

	if(!inst->argc)
		sprintf(out, "%s%s", prefix, inst->m);
	else if(inst->argc == 1)
		sprintf(out, "%s%s %s", prefix, inst->m, opr2mnemo(o1b, &inst->argv[0], att));
	else if(att) sprintf(out, "%s%s %s,%s", prefix, inst->m, opr2mnemo(o2b, &inst->argv[1], att),
			opr2mnemo(o1b, &inst->argv[0], att));
	else sprintf(out, "%s%s %s,%s", prefix, inst->m, opr2mnemo(o1b, &inst->argv[0], att),
			opr2mnemo(o2b, &inst->argv[1], att));
	return out;
}

static void lowerize(char *s) {
	//0["abc"] = 'd';
	for(; *s; s++)
		if(*s >= 'A' && *s <= 'Z') *s = (*s -'A') + 'a';
}

#define ISIMM(a) (OPRTOK(a)==I || OPRTOK(a)==J)
#define HASMODRM(a) (OPRTOK(a)>=G && OPRTOK(a)<A)
static void setup_o(opcode *o) {
	lowerize(o->m);
	if(ISIMM(o->o1)) o->f |= IMM, o->immsz = GETTYPE(o->o1);
	else if(ISIMM(o->o2)) o->f |= IMM, o->immsz = GETTYPE(o->o2);
	else if(ISIMM(o->o3)) o->f |= IMM, o->immsz = GETTYPE(o->o3);
	if(HASMODRM(o->o1) || HASMODRM(o->o2)) o->f |= MODRM;
}

void x86init() {
	int i;
	for(i = 0; i < 0x100; i++) {
		if(i < 0x80) setup_o(mapext+i);
		setup_o(map+i);
		setup_o(map0f+i);
	}
}

