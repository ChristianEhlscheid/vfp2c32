#ifndef _VFP2CASSEMBLY_H__
#define _VFP2CASSEMBLY_H__

inline bool InByteRange(int nValue) { return (nValue > 0 && nValue < 127) || (nValue < 0 && nValue > -128); }
inline bool InUByteRange(unsigned  int nValue) { return nValue < 256; }

typedef void (_stdcall *FUNCPTR)();
typedef unsigned char *LPCODE;
typedef unsigned char CODE;
typedef void* AVALUE;

static const int ASM_IDENTIFIER_LEN	= 64;
static const int ASM_MAX_PARAMS		= 32;
static const int ASM_MAX_VARS			= 64;
static const int ASM_MAX_LABELS		= 32;
static const int ASM_MAX_JUMPS			= 64;
static const int ASM_MAX_CODE_BUFFER	= 2048;

typedef enum _REGISTER {
// 32bit Registers
	EAX = 0,
	ECX,
	EDX,
	EBX,
	ESP,
	EBP,
	ESI,
	EDI,
// 16bit Registers
	AX,
	CX, 
	DX,
	BX,
	SP,
	BP,
	SI,
	DI,
// 8bit Registers
	AL,
	BL,
	CL,
	DL,
	DH,
	BH,
	CH,
	AH,
} REGISTER;

typedef enum _RELREGISTER {
	REAX = 0,
	RECX,
	REDX,
	REBX,
	RESP,
	REBP,
	RESI,
	REDI,
} RELREGISTER;

typedef enum _PARAMNO {
	parm1 = 1,
	parm2,
	parm3,
	parm4,
	parm5,
	parm6,
	parm7,
	parm8,
	parm9,
	parm10,
	parm11,
	parm12,
	parm13,
	parm14,
	parm15,
	parm16,
	parm17,
	parm18,
	parm19,
	parm20,
	parm21,
	parm22,
	parm23,
	parm24,
	parm25,
	parm26,
	parm27,
} PARAMNO;

typedef enum _TYPE {
	T_CHAR = 0,
	T_UCHAR,
	T_SHORT,
	T_USHORT,
	T_INT,
	T_UINT,
	T_INT64,
	T_UINT64,
	T_FLOAT,
	T_DOUBLE,
} TYPE;

typedef struct _CTYPEINFO {
	int nSize;
	int nAlign;
	int nOffset;
	BOOL bSign;
	TYPE nType;
	char aName[ASM_IDENTIFIER_LEN];
} CTYPEINFO, *LPCTYPEINFO;

typedef struct _LABEL {
		char aName[ASM_IDENTIFIER_LEN];
		LPCODE pLocation;
} LABEL, *LPLABEL;

class RuntimeAssembler
{
public:
	RuntimeAssembler();

	void WriteCode(void *lpAddress);
	int CodeSize();
	void Prolog();
	void Epilog(bool bCDecl = false);

	void Parameter(TYPE nType);
	void Parameter(char *pParameter, TYPE nType);

	void LocalVar(char *pVariable, TYPE nType);
	void LocalVar(char *pVariable, int nSize, int nAlignment);
	
	void Label(char *pLabel);
	void* LabelAddress(char *pLabel);
	void Jump(char *pLabel);
	void Patch();
	void BreakPoint();

	void Push(REGISTER nReg);
	void Push(AVALUE nValue);
	void Push(int nValue);
	void Push(PARAMNO nParmNo);
	void Push(char *pParmOrVar);
	void Push(PARAMNO nParmNo, TYPE nType, int nOffset = 0);
	void Push(char *pParmOrVar, TYPE nType, int nOffset = 0);

	void Pop(REGISTER nReg);

	void Sub(REGISTER nReg, int nBytes);
	void Sub(REGISTER nReg, unsigned int nBytes);
	void Sub(REGISTER nReg, REGISTER nReg2);
	void Add(REGISTER nReg, int nBytes);
	void Add(REGISTER nReg, unsigned int nBytes);
	void Add(REGISTER nReg, REGISTER nReg2);
	void Dec(REGISTER nReg);
	void Inc(REGISTER nReg);
	void And(REGISTER nReg, int nValue);
	void And(REGISTER nReg, REGISTER nReg2);
	void Or(REGISTER nReg, int nValue);
	void Or(REGISTER nReg, REGISTER nReg2);
	void Xor(REGISTER nReg, int nValue);
	void Xor(REGISTER nReg, REGISTER nReg2);

	void Cdq();

	void Sar(REGISTER nReg, int nBits);
	void Sal(REGISTER nReg, int nBits);
	void Shl(REGISTER nReg, int nBits);
	void Shr(REGISTER nReg, int nBits);

	void Lea(REGISTER nReg, PARAMNO nParmNo, int nOffset = 0);
	void Lea(REGISTER nReg, char *pParmOrVar, int nOffset = 0);

	void Mov(REGISTER nReg, PARAMNO nParmNo);
	void Mov(REGISTER nReg, char *pParmOrVar);
	void Mov(REGISTER nReg, PARAMNO nParmNo, TYPE nType, int nOffset = 0);
	void Mov(REGISTER nReg, char *pParmOrVar, TYPE nType, int nOffset = 0);
	void Mov(char *pParmOrVar, unsigned int nValue);
	void Mov(char *pParmOrVar, REGISTER nReg);
	void Mov(REGISTER nReg, AVALUE nValue);
	void Mov(REGISTER nRegDest, REGISTER nRegSource);
	void Mov(RELREGISTER nRelReg, int nSize, int nValue);

	void MovZX(REGISTER nReg, PARAMNO nParmNo);
	void MovZX(REGISTER nReg, char *pParmOrVar);
	void MovZX(REGISTER nReg, PARAMNO nParmNo, TYPE nType, int nOffset = 0);
	void MovZX(REGISTER nReg, char *pParmOrVar, TYPE nType, int nOffset = 0);
	void MovZX(char *pParmOrVar, REGISTER nReg);
	void MovZX(REGISTER nRegDest, REGISTER nRegSource);

	void MovSX(REGISTER nReg, PARAMNO nParmNo);
	void MovSX(REGISTER nReg, char *pParmOrVar);
	void MovSX(REGISTER nReg, PARAMNO nParmNo, TYPE nType, int nOffset = 0);
	void MovSX(REGISTER nReg, char *pParmOrVar, TYPE nType, int nOffset = 0);
	void MovSX(char *pParmOrVar, REGISTER nReg);
	void MovSX(PARAMNO nParmNo, REGISTER nReg);
	void MovSX(REGISTER nRegDest, REGISTER nRegSource);

// FLOATING POINT functions
	// Fld -  Pushes a Float Number from the source onto the top of the FPU Stack.
	void Fld(PARAMNO nParmNo);
	void Fld(char *pParmOrVar);
	void Fld(PARAMNO nParmNo, TYPE nType, int nOffset = 0);
	void Fld(char *pParmOrVar, TYPE nType, int nOffset = 0);

	void Cmp(REGISTER nReg, int nValue);
	void Cmp(REGISTER nReg, REGISTER nReg2);

	// conditional jumps
	void Je(char *pLabel);
	void Jmp(char *pLabel);
	void Jmp(REGISTER nReg, AVALUE pLocation);
	void Jmp(REGISTER nReg);

	// function invocation
	void Call(REGISTER nReg, FUNCPTR pFunction);
	void Call(FUNCPTR pFunction);
	void Call(REGISTER nReg);
	void Ret();

private:
	void ParameterEx(LPCTYPEINFO lpType, TYPE nType);
	LPCTYPEINFO ParameterRef(char *pParameter);
	LPCTYPEINFO ParameterRef(PARAMNO nParmNo);
	int ParameterSize();
	LPCTYPEINFO LocalVarRef(char *pVariable);
	LPCTYPEINFO ParmOrVarRef(char *pVariable);
	LPLABEL LabelRef(char *pLabel);

	void Push_Ex(TYPE nType, int nOffset);
	void Shift_Ex(REGISTER nReg, int nBits, int nOpcode);
	void Lea_Ex(REGISTER nReg, int nOffset);
	void Mov_Ex(REGISTER nReg, TYPE nType, int nOffset);
	void MovZX_Ex(REGISTER nReg, TYPE nType, int nOffset);
	void MovSX_Ex(REGISTER nReg, TYPE nType, int nOffset);
	void Fld_Ex(TYPE nType, int nOffset);
	void Ret(int nBytes);

	LPCODE m_CS;
	LPCODE m_CSEx;
	CTYPEINFO m_Parms[ASM_MAX_PARAMS];
	int m_ParmCount;
	CTYPEINFO m_Vars[ASM_MAX_VARS];
	int m_VarCount;
	LABEL m_Labels[ASM_MAX_LABELS];
	int m_LabelCount;
	LABEL m_Jumps[ASM_MAX_LABELS];
	int m_JumpCount;
	CODE m_CodeBuffer[ASM_MAX_CODE_BUFFER];

	CODE MAKE_REG_CODE(REGISTER nReg) { return (CODE)(nReg <= EDI ? nReg : nReg <= DI ? nReg - AX : nReg - AL); }
	CODE MODRM_REL_REG(REGISTER nReg) { return (MODRM_MOD_REL | MAKE_REG_CODE(nReg)); }
	CODE MODRM_REL_REG(RELREGISTER nReg) { return (MODRM_MOD_REL | MAKE_REG_CODE((REGISTER)nReg)); }
	CODE MODRM_REG_REG(REGISTER nReg) { return (MODRM_MOD_REG | MAKE_REG_CODE(nReg)); }
	CODE MODRM_REG_OP_REG(CODE nOpcode, REGISTER nReg) { return (MODRM_MOD_REG | (nOpcode << MODRM_OP_OFFSET) | MAKE_REG_CODE(nReg)); }
	CODE MODRM_REG_REG_REG(REGISTER nDest, REGISTER nSource)	{ return (MODRM_MOD_REG | MAKE_REG_CODE(nSource) | (MAKE_REG_CODE(nDest) << MODRM_OP_OFFSET)); }

	CODE MODRM_DISP32_OP_REG(CODE nOpcode, REGISTER nReg) { return (MODRM_MOD_DISP32 | (nOpcode << MODRM_OP_OFFSET) | MAKE_REG_CODE(nReg)); }
	CODE MODRM_DISP32_STACK() { return (MODRM_MOD_DISP32 | EBP); }
	CODE MODRM_DISP32_OP_STACK(CODE nOpcode) { return (MODRM_MOD_DISP32 | (nOpcode << MODRM_OP_OFFSET) | EBP); }
	CODE MODRM_DISP32_REG_STACK(REGISTER nReg) { return (MODRM_MOD_DISP32 | (MAKE_REG_CODE(nReg) << MODRM_OP_OFFSET) | EBP); }

	CODE MODRM_DISP8_OP_REG(CODE nOpcode, REGISTER nReg) { return (MODRM_MOD_DISP8 | (nOpcode << MODRM_OP_OFFSET) | MAKE_REG_CODE(nReg)); }
	CODE MODRM_DISP8_STACK() { return (MODRM_MOD_DISP8 | EBP); }
	CODE MODRM_DISP8_OP_STACK(CODE nOpcode) { return (MODRM_MOD_DISP8 | (nOpcode << MODRM_OP_OFFSET) | EBP); }
	CODE MODRM_DISP8_REG_STACK(REGISTER nReg) { return (MODRM_MOD_DISP8 | (MAKE_REG_CODE(nReg) << MODRM_OP_OFFSET) | EBP); }
	CODE MAKE_OP(CODE nOpcode, REGISTER nReg) { return (nOpcode + MAKE_REG_CODE(nReg)); }

	static const int Asm_Types[10][3];  // size, alignment & sign of C types

	// count of registers
	static const int R_COUNT	= 24;
	static const int R_16COUNT	= 16;
	static const int R_32COUNT	= 8;
	static const int R_32BIT	= 7;
	static const int R_16BIT	= 15;

	static const int CTYPE_SIZE	= 0;
	static const int CTYPE_ALIGN	= 1;
	static const int CTYPE_SIGN	= 2;

	// ModR/M byte encoding
	// 7   6  5        3  2    0
	// Mod    Reg/Opcode  R/M
	static const CODE MODRM_MOD_REL		= 0;		// 00000000
	static const CODE MODRM_MOD_DISP8		= 64;		// 01000000
	static const CODE MODRM_MOD_DISP32		= 128;		// 10000000
	static const CODE MODRM_MOD_REG		= 192;		// 11000000

	// SIB byte (Scale Index Register) encoding
	// 7   6  5    3  2   0
	// Scale  Index   Base
	static const CODE SIB_SCALE0	= 0;	// 00000000
	static const CODE SIB_SCALE2	= 64;	// 01000000
	static const CODE SIB_SCALE4	= 128;	// 10000000
	static const CODE SIB_SCALE8	= 192;	// 11000000

	// defines to make opcodes
	static const int MODRM_OP_OFFSET	= 3;

	/* Opcodes
	r					= any register
	r/m					= register or memory operand
	imm8,imm16,imm32	= immediate operand of size X
	*/
	static const CODE OP_ADRSIZE_16		= 0x66;		// immediate/register 16 bit prefix

	static const CODE OP_PUSH_R32			= 0x50;		// PUSH r16 | r32
	static const CODE OP_PUSH_IMM8			= 0x6A;		// PUSH imm8 - Push sign-extended imm8. Stack pointer is incremented by the size of stack pointer.
	static const CODE OP_PUSH_IMM32		= 0x68;		// PUSH imm16 | imm32 - Push sign-extended imm16. Stack pointer is incremented by the size of stack pointer.
	static const CODE OP_PUSH_RM32			= 0xFF;		// PUSH r/m16 | r/m32 - Push r/m16 | r/m32.
	static const CODE MODRM_OP_PUSH		= 6;		// additional opcode for PUSH in ModRM byte

	static const CODE OP_POP_R32			= 0x58;		// pop to register

	static const CODE OP_DEC_RM8			= 0xFE;		// DEC r/m8 - Decrement r/m8 by 1.
	static const CODE OP_DEC_RM32			= 0xFF;		// DEC r/m16 | r/m32 - Decrement r/m16 by 1.
	static const CODE OP_DEC_R32			= 0x48;		// DEC r16 | r32 - Decrement r16 by 1.
	static const CODE MODRM_OP_DEC			= 1;		// additional opcode for DEC in ModRM byte

	static const CODE OP_INC_RM8			= 0xFE;		// INC r/m8 - Increment r/m byte by 1.
	static const CODE OP_INC_RM32			= 0xFF;		// INC r/m16 | r/m32 - Increment word/doubleword by 1.
	static const CODE OP_INC_R32			= 0x40;		// INC r16 | r32 - Increment word/doubleword register by 1.

	static const CODE OP_ADD_RM8_R8		= 0x00;		// ADD r/m8, r8
	static const CODE OP_ADD_RM32_R32		= 0x01;		// ADD r/m32, r32
	static const CODE OP_ADD_R8_RM8		= 0x02;		// ADD r8, r/m8
	static const CODE OP_ADD_R32_RM32		= 0x03;		// ADD r32, r/m32 | ADD r16, r/m16
	static const CODE OP_ADD_RM8_IMM8		= 0x80;		// ADD r/m8, imm8 
	static const CODE OP_ADD_RM32_IMM32	= 0x81;		// ADD r/m32, imm32 | ADD r/m16, imm16
	static const CODE OP_ADD_R32_IMM8		= 0x83;		// ADD r/m16, imm8 | ADD r/m32, imm8 - Add sign-extended imm8 to r/m16 | r/m32.
	static const CODE OP_ADD_AL_IMM8		= 0x04;		// ADD AL, imm8
	static const CODE OP_ADD_AX_IMM16		= 0x05;		// ADD AX, imm16
	static const CODE OP_ADD_EAX_IMM32		= 0x05;		// ADD EAX, imm32

	static const CODE OP_SUB_AL_IMM8		= 0x2C;		// SUB AL, imm8 - Subtract imm8 from AL.
	static const CODE OP_SUB_AX_IMM16		= 0x2D;		// SUB AX, imm16 - Subtract imm16 from AX.
	static const CODE OP_SUB_EAX_IMM32		= 0x2D;		// SUB EAX, imm32 - Subtract imm32 from EAX.
	static const CODE OP_SUB_RM8_IMM8		= 0x80;		// SUB r/m8, imm8 - Subtract imm8 from r/m8.
	static const CODE OP_SUB_R32_IMM32		= 0x81;		// SUB r/m16, imm16 | r/m32, imm32 - Subtract imm16 from r/m16 | imm32 from r/m32.
	static const CODE OP_SUB_R32_IMM8		= 0x83;		// SUB r/m16, imm8 | r/m32, imm8 - Subtract sign-extended imm8 from r/m16 | r/m32.
	static const CODE OP_SUB_RM8_R8		= 0x28;		// SUB r/m8, r8 - Subtract r8 from r/m8.
	static const CODE OP_SUB_RM32_R32		= 0x29;		// SUB r/m16, r16 | r/m32, r32 - Subtract r16 from r/m16 | r32 from r/m32.
	static const CODE OP_SUB_R8_RM8		= 0x2A;		// SUB r8, r/m8 - Subtract r/m8 from r8.
	static const CODE OP_SUB_R32_RM32		= 0x2B;		// SUB r16, r/m16 | r/m32 from r32 - Subtract r/m16 from r16 | r/m32 from r32.
	static const CODE MODRM_OP_SUB			= 5;		// additional opcode for SUB in ModRM byte

	static const CODE OP_AND_AL_IMM8		= 0x24;		// AND AL, imm8 - AL AND imm8.
	static const CODE OP_AND_EAX_IMM32		= 0x25;		// AND EAX, imm32 | AND AX, imm16 - EAX AND imm32.
	static const CODE OP_AND_RM8_IMM8		= 0x80;		// AND r/m8, imm8 - r/m8 AND imm8.
	static const CODE OP_AND_RM32_IMM32	= 0x81;		// AND r/m16, imm16 | AND r/m32, imm32 - r/m16 AND imm16.
	static const CODE OP_AND_RM32_IMM8		= 0x83;		// AND r/m16, imm8 | AND r/m32, imm8 - r/m16 AND imm8 (signextended).
	static const CODE OP_AND_RM8_R8		= 0x20;		// AND r/m8, r8 - r/m8 AND r8.
	static const CODE OP_AND_RM32_R32		= 0x21;		// AND r/m16, r16 | AND r/m32, r32 - r/m16 AND r16.
	static const CODE OP_AND_R8_RM8		= 0x22;		// AND r8, r/m8 - r8 AND r/m8.
	static const CODE OP_AND_R32_RM32		= 0x23;		// AND r16, r/m16 | AND r32, r/m32 - r16 AND r/m16.
	static const CODE MODRM_OP_AND			= 4;		// additional opcode for AND in ModRM byte

	static const CODE OP_OR_AL_IMM8		= 0x0C;		// OR AL, imm8 - AL OR imm8.
	static const CODE OP_OR_EAX_IMM32		= 0x0D;		// OR AX, imm16 | OR EAX, imm32 - EAX OR imm32.
	static const CODE OP_OR_RM8_IMM8		= 0x80;		// OR r/m8, imm8 - r/m8 OR imm8.
	static const CODE OP_OR_RM32_IMM32		= 0x81;		// OR r/m16, imm16 | OR r/m32, imm32 - r/m32 OR imm32.
	static const CODE OP_OR_RM32_IMM8		= 0x83;		// OR r/m16, imm8 | OR r/m32, imm8 - r/m16 OR imm8 (signextended).
	static const CODE OP_OR_RM8_R8			= 0x08;		// OR r/m8, r8 - r/m8 OR r8.
	static const CODE OP_OR_RM32_R32		= 0x09;		// OR r/m16, r16 | OR r/m32, r32 - r/m16 OR r16.
	static const CODE OP_OR_R8_RM8			= 0x0A;		// OR r8, r/m8 - r8 OR r/m8.
	static const CODE OP_OR_R32_RM32		= 0x0B;		// OR r16, r/m16 | OR r32, r/m32 - r16 OR r/m16.
	static const CODE MODRM_OP_OR			= 1;		// additional opcode for OR in ModRM byte

	static const CODE OP_XOR_AL_IMM8		= 0x34;		// XOR AL, imm8 - AL XOR imm8.
	static const CODE OP_XOR_EAX_IMM32		= 0x35;		// XOR EAX, imm32 | XOR AX, imm16 - EAX XOR imm32.
	static const CODE OP_XOR_RM8_IMM8		= 0x80;		// XOR r/m8, imm8 - r/m8 XOR imm8.
	static const CODE OP_XOR_RM32_IMM32	= 0x81;		// XOR r/m16, imm16 - r/m16 XOR imm16.
	static const CODE OP_XOR_RM32_IMM8		= 0x83;		// XOR r/m32, imm8 - r/m32 XOR imm8 (signextended).
	static const CODE OP_XOR_RM8_R8		= 0x30;		// XOR r/m8, r8 - r/m8 XOR r8.
	static const CODE OP_XOR_RM32_R32		= 0x31;		// XOR r/m16, r16 - r/m16 XOR r16.
	static const CODE OP_XOR_R8_RM8		= 0x32;		// XOR r8, r/m8 - r8 XOR r/m8.
	static const CODE OP_XOR_R32_RM32		= 0x33;		// XOR r32, r/m32 - r32 XOR r/m32.
	static const CODE MODRM_OP_XOR			= 6;		// additional opcode for XOR in ModRM byte

	static const CODE OP_CDQ				= 0x99;

	static const CODE OP_SHIFT_RM8			= 0xD0;		// SHIFT r/m8, 1 - Multiply r/m8 by 2, once.
	static const CODE OP_SHIFT_RM8_IMM8	= 0xC0;		// SHIFT r/m8, imm8 - Multiply r/m8 by 2, imm8 times.
	static const CODE OP_SHIFT_RM32		= 0xD1;		// SHIFT r/m32, 1 - Multiply r/m32 by 2, once.
	static const CODE OP_SHIFT_RM32_IMM8	= 0xC1;		// SHIFT r/m32, imm8 - Multiply r/m32 by 2, imm8 times.
	static const CODE MODRM_OP_SAL			= 4;		// additional opcode for SAL in ModRM byte
	static const CODE MODRM_OP_SAR			= 7;		// additional opcode for SAR in ModRM byte
	static const CODE MODRM_OP_SHL			= 4;		// additional opcode for SHL in ModRM byte
	static const CODE MODRM_OP_SHR			= 5;		// additional opcode for SHR in ModRM byte

	static const CODE OP_LEA_R32_M			= 0x8D;		// LEA r32,m  - Store effective address for m in register r32.

	static const CODE OP_MOV_RM8_R8		= 0x88;		// MOV r/m8,r8 - Move r8 to r/m8.
	static const CODE OP_MOV_RM32_R32		= 0x89;		// MOV r/m16,r16 | r/m32,r32 - Move r16 to r/m16 | r32 to r/m32.
	static const CODE OP_MOV_R8_RM8		= 0x8A;		// MOV r8,r/m8 - Move r/m8 to r8.
	static const CODE OP_MOV_R32_RM32		= 0x8B;		// MOV r16,r/m16 | r32,r/m32 - Move r/m16 to r16 | r/m32 to r32.
	static const CODE OP_MOV_R8_IMM8		= 0xB0;		// MOV r8, imm8 - Move imm8 to r8.
	static const CODE OP_MOV_R32_IMM32		= 0xB8;		// MOV r16, imm16 | r32, imm32 - Move imm16 to r16 | imm32 to r32.
	static const CODE OP_MOV_RM8_IMM8		= 0xC6;		// MOV r/m8, imm8 - Move imm8 to r/m8.
	static const CODE OP_MOV_RM32_IMM32	= 0xC7;		// MOV r/m16, imm16 | r/m32, imm32 - Move imm16 to r/m16 | imm32 to r/m32.

	static const CODE OP_TEST_AL_IMM8		= 0xA8;		// TEST AL, imm8 - AND imm8 with AL; set SF,ZF, PF according to result.
	static const CODE OP_TEST_EAX_IMM32	= 0xA9;		// TEST EAX, imm32 - AND imm32 with EAX; set SF,ZF, PF according to result.
	static const CODE OP_TEST_RM8_IMM8		= 0xF6;		// TEST r/m8, imm8 - AND imm8 with r/m8; set SF,ZF, PF according to result.
	static const CODE OP_TEST_RM32_IMM32	= 0xF7;		// TEST r/m32, imm32 - AND imm32 with r/m32; set SF, ZF, PF according to result.
	static const CODE OP_TEST_RM8_R8		= 0x84;		// TEST r/m8, r8 - AND r8 with r/m8; set SF, ZF, PF according to result.
	static const CODE OP_TEST_RM32_R32		= 0x85;		// TEST r/m32, r32 - AND r32 with r/m32; set SF, ZF, PF according to result.

	static const CODE OP_JMP_REL8			= 0xEB;		// JMP rel8 - Jump short, RIP = RIP + 8-bit displacement
	static const CODE OP_JMP_REL32			= 0xE9;		// JMP rel16 | rel32 - Jump near, relative, RIP = RIP + 16/32-bit
	static const CODE OP_JMP_RM32			= 0xFF;		// JMP r/m16 | r/m32 - Jump near, absolute indirect
	static const CODE MODRM_OP_JMP			= 4;		// additional opcode for JMP in ModRM byte

	static const CODE OP_CALL_R32			= 0xFF;		// CALL r/m16 | r/m32 Call near, absolute indirect, address given in r/m16 | r/m32.
	static const CODE MODRM_OP_CALL		= 2;		// additional opcode for CALL in ModRM byte
};

#endif	// _VFP2CASSEMBLY_H__