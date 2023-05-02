#ifndef _VFP2CASSEMBLY_H__
#define _VFP2CASSEMBLY_H__

/* 
this code implements a runtime assembly code emitter:
- it supports the basic x86 and x64 instruction set (no MMX, no SSE/SSE2/SSE4)
- on x86 it can handle all calling conventions on the windows platform, cdecl/stdcall/thiscall/fastcall
- on x64 there is only one calling convention which is x64 fastcall 
- it can automatically produce prolog and epilog code, based on the calling convention and on the instructions generated, by keeping track of the used registers and parameter count
- additionally it automatically produces the neccessary unwind information on x64 
*/
// msvcprtd
// #pragma comment(lib, "old_initcode.obj")

#pragma warning(disable : 4482)

#if !defined(UWOP_PUSH_NONVOL)
#define UWOP_PUSH_NONVOL 0
#define UWOP_ALLOC_LARGE 1
#define UWOP_ALLOC_SMALL 2
#define UWOP_SET_FPREG 3
#define UWOP_SAVE_NONVOL 4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM128 8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME 10
#endif

inline bool In8BitRange(__int64 nValue) { return (nValue >= 0 && nValue <= MAX_CHAR) || (nValue < 0 && nValue >= MIN_CHAR); }
inline bool InU8BitRange(unsigned __int64 nValue) { return nValue <= MAX_UCHAR; }
inline bool In16BitRange(__int64 nValue) { return (nValue >= 0 && nValue <= MAX_SHORT) || (nValue < 0 && nValue >= MIN_SHORT); }
inline bool InU16BitRange(unsigned __int64 nValue) { return nValue <= MAX_USHORT; }
inline bool In32BitRange(__int64 nValue) { return (nValue >= 0 && nValue < MAX_INT) || (nValue < 0 && nValue >= MIN_INT); }
inline bool InU32BitRange(unsigned __int64 nValue) { return nValue <= MAX_UINT; }
inline bool In64BitRange(__int64 nValue) { return (nValue >= 0 && nValue < MAX_INT64) || (nValue < 0  && nValue >= MIN_INT64); }

#if defined(_WIN64)
#define Rex_Prefix_WR(nReg)				Rex_Prefix_WR_Impl((AsmRegister)nReg)
#define Rex_Prefix_WB(nReg)				Rex_Prefix_WB_Impl((AsmRegister)nReg)
#define Rex_Prefix_B(nReg)				Rex_Prefix_B_Impl((AsmRegister)nReg)
#define Rex_Prefix_R(nReg)				Rex_Prefix_R_Impl((AsmRegister)nReg)
#define Rex_Prefix_WRB(nReg1, nReg2)	Rex_Prefix_WRB_Impl((AsmRegister)nReg1,(AsmRegister)nReg2)
#define Rex_Prefix_RB(nReg1, nReg2)		Rex_Prefix_RB_Impl((AsmRegister)nReg1,(AsmRegister)nReg2)
#else
#define Rex_Prefix_WR(nReg)				0
#define Rex_Prefix_WB(nReg)				0
#define Rex_Prefix_B(nReg)				0
#define Rex_Prefix_R(nReg)				0
#define Rex_Prefix_WRB(nReg1,nReg2)		0
#define Rex_Prefix_RB(nReg1, nReg2)		0
#endif
typedef void (_stdcall *AsmFuncPtr)();
typedef unsigned char AsmByte;

const int ASM_IDENTIFIER_LEN = 64 - 16; // -8 byte for AsmLabel and -8 byte for CStrBuilder fields, so it fit's on a cacheline

// prefixes
const AsmByte PREFIX_REP			= 0xF3; // REP, REPE, REPZ
const AsmByte PREFIX_REPN			= 0xF2; // REPNE, REPNZ
const AsmByte PREFIX_LOCK			= 0xF0; // LOCK
const AsmByte PREFIX_OP_SIZE		= 0x66;	// operand prefix - 16 bit in 32bit mode or 32bit in 64bit mode
const AsmByte PREFIX_ADR_SIZE		= 0x67;	// immediate/register prefix 32 bit in 64bit mode

const AsmByte PREFIX_REX			= 0x40;	// REX
const AsmByte PREFIX_REX_B			= 0x01;	// REX.B - Base field extension
const AsmByte PREFIX_REX_X			= 0x02;	// REX.X - Index field extension
const AsmByte PREFIX_REX_R			= 0x04;	// REX.R - Register field extension
const AsmByte PREFIX_REX_W			= 0x08;	// REX.W - Operand width
const AsmByte PREFIX_REX_REG_MASK	= 0x08; // (AsmRegister & PREFIX_REX_REG_MASK) > 0 if an extended register is used

const AsmByte PREFIX_SSE_F2			= 0xF2; // Prefix for SIMD instructions
const AsmByte PREFIX_SSE_F3			= 0xF3; // Prefix for SIMD instructions

const AsmByte OP_2BYTE = 0x0F; // 2 byte OPCODE prefix

// Operand Masks
const AsmByte OP_MASK_OPERAND8			= 0x0;
const AsmByte OP_MASK_OPERAND32			= 0x1;
const AsmByte OP_MASK_DIRECTION_REG2RM	= 0x0;
const AsmByte OP_MASK_DIRECTION_RM2REG	= 0x2;
const AsmByte OP_MASK_IMMEDIATE			= 0x80;
const AsmByte OP_MASK_IMMEDIATE8		= 0x2;
const AsmByte OP_MASK_IMMEDIATE_OPSIZE	= 0x0;
const AsmByte OP_MASK_EAX				= 0x4;

// ModR/M byte encoding
// 7 6  5   4    3   2  1  0
// Mod  Reg/Opcode   R/M
const AsmByte MODRM_MOD_RELATIVE		= 0x0;	// 00000000
const AsmByte MODRM_MOD_DISPLACEMENT8	= 0x1;	// 00000001
const AsmByte MODRM_MOD_DISPLACEMENT32	= 0x2;	// 00000010
const AsmByte MODRM_MOD_REGISTER		= 0x3;	// 00000011
const AsmByte MODRM_RM_RIP_RELATIVE		= 0x5;	// 00000101
const AsmByte MODRM_RM_SIB				= 0x4;	// 00000100
const int MODRM_REG_OFFSET				= 3;
const int MODRM_MOD_OFFSET				= 6;

// SIB byte encoding
// 7   6  5    3  2   0
// Scale  Index   Base
const AsmByte SIB_SCALE0	= 0x0;	// 00000000
const AsmByte SIB_SCALE2	= 0x1;	// 00000001
const AsmByte SIB_SCALE4	= 0x2;	// 00000010
const AsmByte SIB_SCALE8	= 0x3;	// 00000011
const int SIB_INDEX_DISP	= 0x4;
const int SIB_INDEX_OFFSET	= 3;
const int SIB_SCALE_OFFSET	= 6;

const AsmByte OP_BREAKPOINT = 0xCC;

const AsmByte OP_PUSH_R32 = 0x50;		// PUSH r16 | r32
const AsmByte OP_PUSH_IMM8 = 0x6A;		// PUSH imm8 - Push sign-extended imm8. Stack pointer is incremented by the size of stack pointer.
const AsmByte OP_PUSH_IMM32 = 0x68;		// PUSH imm16 | imm32 - Push sign-extended imm16. Stack pointer is incremented by the size of stack pointer.
const AsmByte OP_PUSH_RM32 = 0xFF;		// PUSH r/m16 | r/m32 - Push r/m16 | r/m32.
const AsmByte MODRM_OP_PUSH = 6;		// additional opcode for PUSH in ModRM byte

const AsmByte OP_POP_R32 = 0x58;		// pop to register

const AsmByte OP_DEC_RM8 = 0xFE;		// DEC r/m8 - Decrement r/m8 by 1.
const AsmByte OP_DEC_RM32 = 0xFF;		// DEC r/m16 | r/m32 - Decrement r/m16 by 1.
const AsmByte OP_DEC_R32 = 0x48;		// DEC r16 | r32 - Decrement r16 by 1.
const AsmByte MODRM_OP_DEC = 1;		// additional opcode for DEC in ModRM byte

const AsmByte OP_INC_RM8 = 0xFE;		// INC r/m8 - Increment r/m byte by 1.
const AsmByte OP_INC_RM32 = 0xFF;		// INC r/m16 | r/m32 - Increment word/doubleword by 1.
const AsmByte OP_INC_R32 = 0x40;		// INC r16 | r32 - Increment word/doubleword register by 1.

const AsmByte OP_ADD_BASE = 0x00;
const AsmByte OP_ADD_RM8_R8 = (OP_ADD_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_REG2RM);		// ADD r/m8, r8
const AsmByte OP_ADD_RM32_R32 = (OP_ADD_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_REG2RM); // ADD r/m32, r32
const AsmByte OP_ADD_R8_RM8 = (OP_ADD_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_RM2REG); // ADD r8, r/m8
const AsmByte OP_ADD_R32_RM32 = (OP_ADD_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_RM2REG);		// ADD r32, r/m32 | ADD r16, r/m16
const AsmByte OP_ADD_AL_IMM8 = (OP_ADD_BASE | OP_MASK_OPERAND8 | OP_MASK_EAX);	// ADD AL, imm8
const AsmByte OP_ADD_EAX_IMM32 = (OP_ADD_BASE | OP_MASK_OPERAND32 | OP_MASK_EAX);		// ADD EAX, imm32
const AsmByte OP_ADD_RM8_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND8 | OP_MASK_IMMEDIATE_OPSIZE);	// ADD r/m8, imm8 
const AsmByte OP_ADD_RM32_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE8);	// ADD r/m16, imm8 | ADD r/m32, imm8 - Add sign-extended imm8 to r/m16 | r/m32.
const AsmByte OP_ADD_RM32_IMM32 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE_OPSIZE);	// ADD r/m32, imm32 | ADD r/m16, imm16
const AsmByte MODRM_OP_ADD = 0;		// additional opcode for ADD in ModRM byte

const AsmByte OP_ADCX_R64_RM64 = 0x38;
const AsmByte OP_ADCX_OPCODE2 = 0xF6;
const AsmByte OP_ADOX_R64_RM64 = 0x38;
const AsmByte OP_ADOX_OPCODE2 = 0xF6;

const AsmByte OP_SUB_BASE = 0x28;
const AsmByte OP_SUB_RM8_R8 = (OP_SUB_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_REG2RM);	// SUB r/m8, r8 - Subtract r8 from r/m8.
const AsmByte OP_SUB_RM32_R32 = (OP_SUB_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_REG2RM);		// SUB r/m16, r16 | r/m32, r32 - Subtract r16 from r/m16 | r32 from r/m32.
const AsmByte OP_SUB_R8_RM8 = (OP_SUB_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_RM2REG);		// SUB r8, r/m8 - Subtract r/m8 from r8.
const AsmByte OP_SUB_R32_RM32 = (OP_SUB_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_RM2REG);		// SUB r16, r/m16 | r/m32 from r32 - Subtract r/m16 from r16 | r/m32 from r32.
const AsmByte OP_SUB_AL_IMM8 = (OP_SUB_BASE | OP_MASK_OPERAND8 | OP_MASK_EAX);	// SUB AL, imm8 - Subtract imm8 from AL.
const AsmByte OP_SUB_EAX_IMM32 = (OP_SUB_BASE | OP_MASK_OPERAND32 | OP_MASK_EAX);	// SUB EAX, imm32 - Subtract imm32 from EAX.
const AsmByte OP_SUB_RM8_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND8 | OP_MASK_IMMEDIATE_OPSIZE);		// SUB r/m8, imm8 - Subtract imm8 from r/m8.
const AsmByte OP_SUB_R32_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE8);		// SUB r/m16, imm8 | r/m32, imm8 - Subtract sign-extended imm8 from r/m16 | r/m32.
const AsmByte OP_SUB_R32_IMM32 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE_OPSIZE);		// SUB r/m16, imm16 | r/m32, imm32 - Subtract imm16 from r/m16 | imm32 from r/m32.
const AsmByte MODRM_OP_SUB = 5;		// additional opcode for SUB in ModRM byte

const AsmByte OP_AND_BASE = 0x20;
const AsmByte OP_AND_RM8_R8 = (OP_AND_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_REG2RM);		// AND r/m8, r8 - r/m8 AND r8.
const AsmByte OP_AND_RM32_R32 = (OP_AND_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_REG2RM);		// AND r/m16, r16 | AND r/m32, r32 - r/m16 AND r16.
const AsmByte OP_AND_R8_RM8 = (OP_AND_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_RM2REG);		// AND r8, r/m8 - r8 AND r/m8.
const AsmByte OP_AND_R32_RM32 = (OP_AND_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_RM2REG);		// AND r16, r/m16 | AND r32, r/m32 - r16 AND r/m16.
const AsmByte OP_AND_AL_IMM8 = (OP_AND_BASE | OP_MASK_OPERAND8 | OP_MASK_EAX);		// AND AL, imm8 - AL AND imm8.
const AsmByte OP_AND_EAX_IMM32 = (OP_AND_BASE | OP_MASK_OPERAND32 | OP_MASK_EAX);		// AND EAX, imm32 | AND AX, imm16 - EAX AND imm32.
const AsmByte OP_AND_RM8_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND8 | OP_MASK_IMMEDIATE_OPSIZE);		// AND r/m8, imm8 - r/m8 AND imm8.
const AsmByte OP_AND_RM32_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE8);		// AND r/m16, imm8 | AND r/m32, imm8 - r/m16 AND imm8 (signextended).
const AsmByte OP_AND_RM32_IMM32 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE_OPSIZE);		// AND r/m16, imm16 | AND r/m32, imm32 - r/m16 AND imm16.
const AsmByte MODRM_OP_AND = 4;		// additional opcode for AND in ModRM byte

const AsmByte OP_OR_BASE = 0x08;
const AsmByte OP_OR_RM8_R8 = (OP_OR_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_REG2RM);		// OR r/m8, r8 - r/m8 OR r8.
const AsmByte OP_OR_RM32_R32 = (OP_OR_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_REG2RM);		// OR r/m16, r16 | OR r/m32, r32 - r/m16 OR r16.
const AsmByte OP_OR_R8_RM8 = (OP_OR_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_RM2REG);		// OR r8, r/m8 - r8 OR r/m8.
const AsmByte OP_OR_R32_RM32 = (OP_OR_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_RM2REG);		// OR r16, r/m16 | OR r32, r/m32 - r16 OR r/m16.
const AsmByte OP_OR_AL_IMM8 = (OP_OR_BASE | OP_MASK_OPERAND8 | OP_MASK_EAX);		// OR AL, imm8 - AL OR imm8.
const AsmByte OP_OR_EAX_IMM32 = (OP_OR_BASE | OP_MASK_OPERAND32 | OP_MASK_EAX);		// OR AX, imm16 | OR EAX, imm32 - EAX OR imm32.
const AsmByte OP_OR_RM8_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND8 | OP_MASK_IMMEDIATE_OPSIZE);		// OR r/m8, imm8 - r/m8 OR imm8.
const AsmByte OP_OR_RM32_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE8);		// OR r/m16, imm8 | OR r/m32, imm8 - r/m16 OR imm8 (signextended).
const AsmByte OP_OR_RM32_IMM32 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE_OPSIZE);		// OR r/m16, imm16 | OR r/m32, imm32 - r/m32 OR imm32.
const AsmByte MODRM_OP_OR = 1;		// additional opcode for OR in ModRM byte

const AsmByte OP_XOR_BASE = 0x30;
const AsmByte OP_XOR_RM8_R8 = (OP_XOR_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_REG2RM);		// XOR r/m8, r8 - r/m8 XOR r8.
const AsmByte OP_XOR_RM32_R32 = (OP_XOR_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_REG2RM);		// XOR r/m16, r16 - r/m16 XOR r16.
const AsmByte OP_XOR_R8_RM8 = (OP_XOR_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_RM2REG);		// XOR r8, r/m8 - r8 XOR r/m8.
const AsmByte OP_XOR_R32_RM32 = (OP_XOR_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_RM2REG);		// XOR r32, r/m32 - r32 XOR r/m32.
const AsmByte OP_XOR_AL_IMM8 = (OP_XOR_BASE | OP_MASK_OPERAND8 | OP_MASK_EAX);		// XOR AL, imm8 - AL XOR imm8.
const AsmByte OP_XOR_EAX_IMM32 = (OP_XOR_BASE | OP_MASK_OPERAND32 | OP_MASK_EAX);		// XOR EAX, imm32 | XOR AX, imm16 - EAX XOR imm32.
const AsmByte OP_XOR_RM8_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND8 | OP_MASK_IMMEDIATE_OPSIZE);		// XOR r/m8, imm8 - r/m8 XOR imm8.
const AsmByte OP_XOR_RM32_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE8);		// XOR r/m32, imm8 - r/m32 XOR imm8 (signextended).
const AsmByte OP_XOR_RM32_IMM32 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE_OPSIZE);		// XOR r/m16, imm16 - r/m16 XOR imm16.
const AsmByte MODRM_OP_XOR = 6;		// additional opcode for XOR in ModRM byte

const AsmByte OP_CMP_BASE = 0x38;
const AsmByte OP_CMP_RM8_R8 = (OP_CMP_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_REG2RM);		// CMP r/m8, r8 - r/m8 XOR r8.
const AsmByte OP_CMP_RM32_R32 = (OP_CMP_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_REG2RM);		// CMP r/m16, r16 - r/m16 XOR r16.
const AsmByte OP_CMP_R8_RM8 = (OP_CMP_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_RM2REG);		// CMP r8, r/m8 - r8 XOR r/m8.
const AsmByte OP_CMP_R32_RM32 = (OP_CMP_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_RM2REG);		// CMP r32, r/m32 - r32 XOR r/m32.
const AsmByte OP_CMP_AL_IMM8 = (OP_CMP_BASE | OP_MASK_OPERAND8 | OP_MASK_EAX);		// CMP AL, imm8 - AL XOR imm8.
const AsmByte OP_CMP_EAX_IMM32 = (OP_CMP_BASE | OP_MASK_OPERAND32 | OP_MASK_EAX);		// CMP EAX, imm32 | XOR AX, imm16 - EAX XOR imm32.
const AsmByte OP_CMP_RM8_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND8 | OP_MASK_IMMEDIATE_OPSIZE);		// CMP r/m8, imm8 - r/m8 XOR imm8.
const AsmByte OP_CMP_RM32_IMM8 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE8);		// CMP r/m16, imm16 - r/m16 XOR imm16.
const AsmByte OP_CMP_RM32_IMM32 = (OP_MASK_IMMEDIATE | OP_MASK_OPERAND32 | OP_MASK_IMMEDIATE_OPSIZE);		// CMP r/m32, imm8 - r/m32 XOR imm8 (signextended).
const AsmByte MODRM_OP_CMP = 7;		// additional opcode for CMP in ModRM byte

const AsmByte OP_CDQ = 0x99;
const AsmByte OP_CVTSI2SD = 0x2A;
const AsmByte OP_CVTSD2SI = 0x2D;
const AsmByte OP_CVTTSD2SI = 0x2C;
const AsmByte OP_CVTSD2SS = 0x5A;
const AsmByte OP_CVTSS2SD = 0x5A;

const AsmByte OP_SHIFT_RM8 = 0xD0;		// SHIFT r/m8, 1 - Multiply r/m8 by 2, once.
const AsmByte OP_SHIFT_RM32 = 0xD1;		// SHIFT r/m32, 1 - Multiply r/m32 by 2, once.
const AsmByte OP_SHIFT_RM8_IMM8 = 0xC0;		// SHIFT r/m8, imm8 - Multiply r/m8 by 2, imm8 times.
const AsmByte OP_SHIFT_RM32_IMM8 = 0xC1;		// SHIFT r/m32, imm8 - Multiply r/m32 by 2, imm8 times.
const AsmByte MODRM_OP_SAL = 4;		// additional opcode for SAL in ModRM byte
const AsmByte MODRM_OP_SAR = 7;		// additional opcode for SAR in ModRM byte
const AsmByte MODRM_OP_SHL = 4;		// additional opcode for SHL in ModRM byte
const AsmByte MODRM_OP_SHR = 5;		// additional opcode for SHR in ModRM byte

const AsmByte OP_LEA_R32_M = 0x8D;		// LEA r32,m  - Store effective address for m in register r32.

const AsmByte OP_MOV_BASE = 0x88;
const AsmByte OP_MOV_RM8_R8 = (OP_MOV_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_REG2RM);		// MOV r/m8,r8 - Move r8 to r/m8.
const AsmByte OP_MOV_RM32_R32 = (OP_MOV_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_REG2RM);		// MOV r/m16,r16 | r/m32,r32 - Move r16 to r/m16 | r32 to r/m32.
const AsmByte OP_MOV_R8_RM8 = (OP_MOV_BASE | OP_MASK_OPERAND8 | OP_MASK_DIRECTION_RM2REG);		// MOV r8,r/m8 - Move r/m8 to r8.
const AsmByte OP_MOV_R32_RM32 = (OP_MOV_BASE | OP_MASK_OPERAND32 | OP_MASK_DIRECTION_RM2REG);		// MOV r16,r/m16 | r32,r/m32 - Move r/m16 to r16 | r/m32 to r32.
const AsmByte OP_MOV_R8_IMM8 = 0xB0;		// MOV r8, imm8 - Move imm8 to r8.
const AsmByte OP_MOV_R64_IMM64 = 0xB8;		// MOV r16, imm16 | r32, imm32 - Move imm16 to r16 | imm32 to r32.
const AsmByte OP_MOV_RM8_IMM8 = 0xC6;		// MOV r/m8, imm8 - Move imm8 to r/m8.
const AsmByte OP_MOV_RM64_IMM32 = 0xC7;		// MOV r/m16, imm16 | r/m32, imm32 - Move imm16 to r/m16 | imm32 to r/m32.

const AsmByte OP_MOV_R128_RM64 = 0x6E;	// MOV r/m64 to XMM0-7 
const AsmByte OP_MOV_RM64_R128 = 0x7E;  // MOV XMM0-7 to r/m64

const AsmByte OP_MOVZX_R32_RM8 = 0xB6;
const AsmByte OP_MOVZX_R32_RM16 = 0xB7;

const AsmByte OP_MOVSX_R32_RM8 = 0xBE;
const AsmByte OP_MOVSX_R32_RM16 = 0xBF;
const AsmByte OP_MOVSX_R64_RM32 = 0x63;

const AsmByte OP_MOVD_R64_RM32 = 0x6E;
const AsmByte OP_MOVD_RM64_R64 = 0x7E;

const AsmByte OP_MOVSD = 0x10;

/*	F2 0F 10 / r MOVSD xmm1, xmm2	A	V / V	SSE2	Move scalar double - precision floating - point value from xmm2 to xmm1 register.
	F2 0F 10 / r MOVSD xmm1, m64	A	V / V	SSE2	Load scalar double - precision floating - point value from m64 to xmm1 register.
	F2 0F 11 / r MOVSD xmm1 / m64, xmm2	C	V / V	SSE2	Move scalar double - precision floating - point value from xmm2 register to xmm1 / m64.
*/
const AsmByte OP_TEST_AL_IMM8 = 0xA8;		// TEST AL, imm8 - AND imm8 with AL; set SF,ZF, PF according to result.
const AsmByte OP_TEST_EAX_IMM32 = 0xA9;		// TEST EAX, imm32 - AND imm32 with EAX; set SF,ZF, PF according to result.
const AsmByte OP_TEST_RM8_IMM8 = 0xF6;		// TEST r/m8, imm8 - AND imm8 with r/m8; set SF,ZF, PF according to result.
const AsmByte OP_TEST_RM32_IMM32 = 0xF7;		// TEST r/m32, imm32 - AND imm32 with r/m32; set SF, ZF, PF according to result.
const AsmByte OP_TEST_RM8_R8 = 0x84;		// TEST r/m8, r8 - AND r8 with r/m8; set SF, ZF, PF according to result.
const AsmByte OP_TEST_RM32_R32 = 0x85;		// TEST r/m32, r32 - AND r32 with r/m32; set SF, ZF, PF according to result.

const AsmByte OP_JO_REL8 = 0x70; // jump if overflow
const AsmByte OP_JNO_REL8 = 0x71; // jump if not overflow
const AsmByte OP_JC_REL8 = 0x72; // jump if carry
const AsmByte OP_JB_REL8 = 0x72; // jump if below (unsigned int)
const AsmByte OP_JAE_REL8 = 0x73; // jump if above or equal (unsigned int)
const AsmByte OP_JE_REL8 = 0x74; // jump if equal
const AsmByte OP_JZ_REL8 = 0x74; // jump if zero
const AsmByte OP_JNE_REL8 = 0x75; // jump if not equal
const AsmByte OP_JBE_REL8 = 0x76; // jump if below or equal (unsigned int)
const AsmByte OP_JA_REL8 = 0x77; // jump if above (unsigned int)
const AsmByte OP_JS_REL8 = 0x78; // jump if sign
const AsmByte OP_JNS_REL8 = 0x79; // jump if not sign
const AsmByte OP_JNP_REL8 = 0x7B; // jump if not parity
const AsmByte OP_JL_REL8 = 0x7C; // jump if less (signed int)
const AsmByte OP_JGE_REL8 = 0x7D; // jump if greater or equal (signed int)
const AsmByte OP_JLE_REL8 = 0x7E; // jump if less or equal (signed int)
const AsmByte OP_JG_REL8 = 0x7F; // jump if greater (signed int)

/* 32 bit jumps - use in combination with PREFIX_2BYTE_OPCODE */
const AsmByte OP_JO_REL32 = 0x80; // jump if overflow
const AsmByte OP_JNO_REL32 = 0x81; // jump if not overflow
const AsmByte OP_JC_REL32 = 0x82; // jump if carry
const AsmByte OP_JB_REL32 = 0x82; // jump if below (unsigned int)
const AsmByte OP_JAE_REL32 = 0x83; // jump if above or equal (unsigned int)
const AsmByte OP_JE_REL32 = 0x84; // jump if equal
const AsmByte OP_JZ_REL32 = 0x84; // jump if zero
const AsmByte OP_JNE_REL32 = 0x85; // jump if not equal
const AsmByte OP_JBE_REL32 = 0x86; // jump if below or equal (unsigned int)
const AsmByte OP_JA_REL32 = 0x87; // jump if above (unsigned int)
const AsmByte OP_JS_REL32 = 0x79; // jump if sign
const AsmByte OP_JNS_REL32 = 0x89; // jump if not sign
const AsmByte OP_JNP_REL32 = 0x8B; // jump if not parity
const AsmByte OP_JL_REL32 = 0x8C; // jump if less (signed int)
const AsmByte OP_JGE_REL32 = 0x8D; // jump if greater or equal (signed int)
const AsmByte OP_JLE_REL32 = 0x8E; // jump if less or equal (signed int)
const AsmByte OP_JG_REL32 = 0x8F; // jump if greater (signed int)

const AsmByte OP_JMP_REL8 = 0xEB;		// JMP rel8 - Jump short, RIP = RIP + 8-bit displacement
const AsmByte OP_JMP_REL32 = 0xE9;		// JMP rel16 | rel32 - Jump near, relative, RIP = RIP + 16/32-bit
const AsmByte OP_JMP_RM32 = 0xFF;		// JMP r/m16 | r/m32 - Jump near, absolute indirect
const AsmByte MODRM_OP_JMP = 4;		// additional opcode for JMP in ModRM byte

const AsmByte OP_CALL_R32 = 0xFF;		// CALL r/m16 | r/m32 Call near, absolute indirect, address given in r/m16 | r/m32.
const AsmByte MODRM_OP_CALL = 2;		// additional opcode for CALL in ModRM byte

const AsmByte OP_RET_IMM16 = 0xC2;
const AsmByte OP_RET = 0xC3;

const AsmByte OP_FLD_M32 = 0xD9;
const AsmByte OP_FLD_M64 = 0xDD;
const AsmByte OP_FLD_1 = 0xE8;
const AsmByte OP_FLD_0 = 0xEE;
const AsmByte MODRM_OP_FLD = 0;

const AsmByte OP_FST_M32 = 0xD9;
const AsmByte OP_FST_M64 = 0xDD;
const AsmByte OP_FST_REG = 0xD0;
const AsmByte MODRM_OP_FST = 2;

const AsmByte OP_FSTP_M32 = 0xD9;
const AsmByte OP_FSTP_M64 = 0xDD;
const AsmByte OP_FSTP_REG = 0xD8;
const AsmByte MODRM_OP_FSTP = 4;

#if defined(_WIN64)
#define SIZEOF_INT 4
#else
#define SIZEOF_INT 8
#endif

typedef enum _AsmCallingConvention
{
	stdcall = 1,
	cdeclaration,
	fastcall,
	thiscall
} AsmCallingConvention;

typedef enum _AsmRegister {
// 8 bit registers
AL = 0,
CL,
DL,
BL,
AH,
CH,
DH,
BH,
SPL = 4,
BPL,
SIL,
DIL,
R8B,
R9B,
R10B,
R11B,
R12B,
R13B,
R14B,
R15B,
// 16 bit Registers
AX = 16,
CX,
DX,
BX,
SP,
BP,
SI,
DI,
R8W,
R9W,
R10W,
R11W,
R12W,
R13W,
R14W,
R15W,
// 32 bit Registers
EAX = 32,
ECX,
EDX,
EBX,
ESP,
EBP,
ESI,
EDI,
R8D,
R9D,
R10D,
R11D,
R12D,
R13D,
R14D,
R15D,
// 64 bit Registers
RAX = 48,
RCX,
RDX,
RBX,
RSP,
RBP,
RSI,
RDI,
R8,
R9,
R10,
R11,
R12,
R13,
R14,
R15,
// 64  bit floating point registers
ST0 = 64,
ST1,
ST2,
ST3,
ST4,
ST5,
ST6,
ST7,
// 64 bit MMX registers
MM0 = 80,
MM1,
MM2,
MM3,
MM4,
MM5,
MM6,
MM7,
// 128 bit SSE Registers
XMM0 = 96,
XMM1,
XMM2,
XMM3,
XMM4,
XMM5,
XMM6,
XMM7,
XMM8,
XMM9,
XMM10,
XMM11,
XMM12,
XMM13,
XMM14,
XMM15,
} AsmRegister;

inline AsmRegister& operator++(AsmRegister& reg)
{
	return reg = static_cast<AsmRegister>(static_cast<int>(reg) + 1);
}

inline AsmRegister& operator--(AsmRegister& reg)
{
	return reg = static_cast<AsmRegister>(static_cast<int>(reg) - 1);
}

inline int RegisterBitCount(AsmRegister nReg)
{
	if (nReg <= R15B)
		return 8;
	if (nReg <= R15W)
		return 16;
	if (nReg <= R15D)
		return 32;
	if (nReg <= ST7)
		return 64;
	if (nReg <= XMM15)
		return 128;
	return 256;
}

typedef enum _AsmType {
	T_BOOL = 0,
	T_CHAR,
	T_UCHAR,
	T_SHORT,
	T_USHORT,
	T_INT,
	T_UINT,
	T_LONG,
	T_ULONG,
	T_INT64,
	T_UINT64,
	T_PTR,
	T_FLOAT,
	T_DOUBLE,
	T_STRUCT
} AsmType;

inline void AsmGetTypeInfo(AsmType nType, int& nSize, int& nAlignment, bool& bSigned)
{
	switch (nType)
	{
	case T_BOOL:
	case T_CHAR:
	case T_UCHAR:
		nSize = 1;
		nAlignment = 1;
		bSigned = nType == T_CHAR;
		break;
	case T_SHORT:
	case T_USHORT:
		nSize = 2;
		nAlignment = 2;
		bSigned = nType == T_SHORT;
		break;
	case T_INT:
	case T_UINT:
		nSize = 4;
		nAlignment = 4;
		bSigned = nType == T_INT;
		break;
	case T_LONG:
	case T_ULONG:
		nSize = 4;
		nAlignment = 4;
		bSigned = nType == T_LONG;
		break;
	case T_FLOAT:
		nSize = 4;
		nAlignment = 4;
		bSigned = false;
		break;
	case T_INT64:
	case T_UINT64:
		nSize = 8;
		nAlignment = 8;
		bSigned = nType == T_INT64;
		break;
	case T_DOUBLE:
		nSize = 8;
		nAlignment = 8;
		bSigned = false;
		break;
	case T_PTR:
	case T_STRUCT:
		nSize = sizeof(void*);
		nAlignment = sizeof(void*);
		bSigned = false;
	}
}

inline AsmByte Make_Reg_Code(AsmRegister nReg)
{
	AsmByte nRegister = static_cast<AsmByte>(nReg);
	return nRegister & 0x07;
}

typedef struct _AsmCode
{
	AsmByte Code;
	_AsmCode& operator=(AsmRegister nReg) { Code = Make_Reg_Code(nReg); return *this; }
	_AsmCode& operator=(AsmByte nCode) { Code = nCode; return *this; }
	operator bool() const { return Code != 0; }
	operator AsmByte() const { return Code; }
} AsmCode, *LPAsmCode;

inline bool operator==(const AsmCode& lhs, const AsmByte& rhs)
{
	return lhs.Code == rhs;
};

inline bool operator==(const AsmCode& lhs, const AsmRegister& rhs)
{
	return lhs.Code == Make_Reg_Code(rhs);
};

typedef struct _AsmDisplacement
{
	union {
		char Disp8;
		int Disp32;
	};
	char ByteLen;
	bool Fixed;

	AsmByte Displacement(int Value, bool bForceNull = false, bool bFixed = false)
	{
		Fixed = bFixed;
		if (Value == 0 && bForceNull == false)
			return 0;
		if (In8BitRange(Value))
		{
			Disp8 = static_cast<char>(Value);
			ByteLen = 1;
			return MODRM_MOD_DISPLACEMENT8;
		}
		else if (In32BitRange(Value))
		{
			Disp32 = Value;
			ByteLen = 4;
			return MODRM_MOD_DISPLACEMENT32;
		}
		assert(false);
		return 0;
	}

	AsmByte DataDisplacement(size_t nIndex)
	{
		Disp32 = nIndex;
		ByteLen = -1; // special size for Data displacement
		return MODRM_MOD_RELATIVE;
	}

	AsmByte* Write(AsmByte* pAddress, AsmByte* pDataSegmentAddress, AsmByte* pNextInstructionAddress) const
	{
		if (ByteLen == 1)
		{
			*(char*)pAddress = Disp8;
			return pAddress + ByteLen;
		}
		else if (ByteLen == 4)
		{
			*(int*)pAddress = Disp32;
			return pAddress + ByteLen;
		}
		else if (ByteLen == -1)
		{
#if !defined(_WIN64)
			*(AsmByte**)pAddress = (pDataSegmentAddress + Disp32 * 8);
#else
			*(int*)pAddress = ((pDataSegmentAddress + Disp32 * 8) - pNextInstructionAddress);
#endif
			return pAddress + 4;
		}
		return pAddress;
	}

	int Size() const
	{
		return ByteLen == -1 ? 4 : ByteLen;
	}

	int GetDisplacement()
	{
		return ByteLen == 1 ? Disp8 : ByteLen == 4 ? Disp32 : 0;
	}

} AsmDisplacement;

typedef enum _AsmLocation
{
	Register,
	Memory
} AsmLocation;

typedef struct _AsmPtr
{
	AsmLocation Location;
	AsmByte* Address;
	AsmRegister Register;
	AsmDisplacement Displacement;
	int Size;

	static _AsmPtr DwordPtr(AsmRegister reg, int nDisplacement)
	{
		_AsmPtr mem;
		mem.Location = Memory;
		mem.Register = reg;
		mem.Size = RegisterBitCount(reg) / 8;
		mem.Address = 0;
		mem.Displacement.Displacement(nDisplacement);
		return mem;
	}

	template<typename T>
	static _AsmPtr DwordPtr(T pAddress)
	{
		_AsmPtr mem;
		mem.Location = AsmLocation::Memory;
		mem.Size = RegisterBitCount(reg) / 8;
		mem.Displacement.Displacement(nDisplacement);
		mem.Address = reinterpret_cast<AsmByte*>(pAddress);
		return mem;
	}

} AsmPtr;

typedef struct _AsmVariable {
	AsmLocation Location;
	AsmType Type;
	int Size;
	int Alignment;
	bool Signed;
	bool Fixed;
	int Offset;
	AsmRegister Register;

	_AsmVariable() {};
	_AsmVariable(_AsmVariable& pVar);
	_AsmVariable(AsmRegister nReg, int nOffset, bool fixed = false);

	template<typename T>
	static _AsmVariable Var(AsmRegister nReg, int nOffset)
	{
		AsmVariable ret;
		ret.Location = Memory;
		ret.Type = AsmGetType<T>(ret.Size, ret.Alignment, ret.Signed);
		ret.Offset = nOffset;
		ret.Register = nReg;
		ret.Fixed = false;
		return ret;
	}

	template<typename T>
	static _AsmVariable Var(_AsmVariable& pVar, int nOffset)
	{
		AsmVariable ret;
		ret.Location = Memory;
		ret.Type = AsmGetType<T>(ret.Size, ret.Alignment, ret.Signed);
		ret.Offset = pVar.Offset + nOffset;
		ret.Register = pVar.Register;
		ret.Fixed = pVar.Fixed;
		return ret;
	}
} AsmVariable, *LPAsmVariable;

typedef struct _AsmLabel {
	int InstructionIndex;
	int LabelIndex;
	CStrBuilder<ASM_IDENTIFIER_LEN> Name;
	_AsmLabel() : InstructionIndex(0), LabelIndex(0) { }
} AsmLabel, *LPAsmLabel;

typedef struct _AsmModRm
{
	union
	{
		int Code;
		struct
		{
			AsmByte Mod;
			AsmCode Reg;
			AsmCode Rm;
			AsmByte _Used;
		};
	};
	
	AsmByte ToByte() const
	{
		AsmByte nModRm;
		nModRm = (Mod << MODRM_MOD_OFFSET) | (Reg.Code << MODRM_REG_OFFSET) | Rm.Code;
		return nModRm;
	}

	operator bool() const { return Code != 0; }
	operator AsmByte() const { return ToByte(); }
} AsmModRm;

typedef struct _AsmSib
{
	union
	{
		int Code;
		struct
		{
			AsmByte Scale;
			AsmCode Index;
			AsmCode Base;
			AsmByte _Padding;
		};
	};

	AsmCode ToByte() const
	{
		AsmCode nModRm;
		nModRm = (Scale << SIB_SCALE_OFFSET) | (Index.Code << SIB_INDEX_OFFSET) | Base.Code;
		return nModRm;
	}
	operator bool() const { return Code != 0; }
	operator AsmByte() const { return ToByte(); }
} AsmSib;

typedef struct _AsmImmediate
{
	union {
		char Value8;
		short Value16;
		int Value32;
		__int64 Value64;
		float FltValue;
		double DblValue;
	};
	unsigned char ByteLen;

	_AsmImmediate() {};
	_AsmImmediate(const char* pValue) { Immediate((INT_PTR)pValue); }
	_AsmImmediate(void* pValue) { Immediate((INT_PTR)pValue); }
	_AsmImmediate(float pValue) { FltValue = pValue; ByteLen = 4; }
	_AsmImmediate(double pValue) { DblValue = pValue; ByteLen = 8; }
	_AsmImmediate(int pValue) { Immediate32(pValue); }
	_AsmImmediate(unsigned int pValue) { Immediate32(pValue); }
	_AsmImmediate(__int64 pValue) { Immediate64((INT_PTR)pValue); }
	_AsmImmediate(unsigned __int64 pValue) { Immediate64((INT_PTR)pValue); }

	void Immediate8(__int64 Value)
	{
		Value8 = static_cast<char>(Value);
		ByteLen = 1;
	}

	void Immediate16(__int64 Value)
	{
		Value16 = static_cast<short>(Value);
		ByteLen = 2;
	}

	void Immediate32(__int64 Value)
	{
		Value32 = static_cast<int>(Value);
		ByteLen = 4;
	}

	void Immediate64(__int64 Value)
	{
		Value64 = Value;
		ByteLen = 8;
	}

	void Immediate(__int64 Value)
	{
		if (In8BitRange(Value))
		{
			Value8 = static_cast<char>(Value);
			ByteLen = 1;
		}
		else if (In16BitRange(Value))
		{
			Value16 = static_cast<short>(Value);
			ByteLen = 2;
		}
		else if (In32BitRange(Value))
		{
			Value32 = static_cast<int>(Value);
			ByteLen = 4;
		}
		else
		{
			Value64 = Value;
			ByteLen = 8;
		}
	}

	AsmByte* Write(AsmByte* pAddress) const
	{
		switch (ByteLen)
		{
		case 1:
			*(char*)pAddress = Value8;
			break;
		case 2:
			*(short*)pAddress = Value16;
			break;
		case 4:
			*(int*)pAddress = Value32;
			break;
		case 8:
			*(__int64*)pAddress = Value64;
			break;
		}
		return pAddress + ByteLen;
	}

	int Size() const
	{
		return ByteLen;
	}
} AsmImmediate;



typedef struct _AsmInstruction
{
	AsmCode Prefix;
	AsmCode Prefix2;
	AsmCode Prefix3;
	AsmCode Prefix4;
	AsmCode Opcode;
	AsmCode Opcode2;
	AsmCode Opcode3;
	AsmVariable Destination;
	AsmVariable Source;
	AsmModRm ModRM;
	AsmSib Sib;
	AsmDisplacement Displacement;
	AsmImmediate Immediate;

	_AsmInstruction()
	{
		memset(this, 0, sizeof(_AsmInstruction));
	}

	int Size() const
	{
		int nCodeSize = 0;
		if (Prefix)
			nCodeSize += 1;
		if (Prefix2)
			nCodeSize += 1;
		if (Prefix3)
			nCodeSize += 1;
		if (Prefix4)
			nCodeSize += 1;
		if (Opcode)
			nCodeSize += 1;
		if (Opcode2)
			nCodeSize += 1;
		if (Opcode3)
			nCodeSize += 1;
		if (ModRM)
			nCodeSize += 1;
		if (Sib)
			nCodeSize += 1;
		nCodeSize += Displacement.Size();
		nCodeSize += Immediate.Size();
		return nCodeSize;
	}

	AsmByte* Write(AsmByte* pAddress, AsmByte* pDataSegmentAddress);
} AsmInstruction, *LPAsmInstruction;

typedef struct _AsmData
{
	union {
		float floatValue;
		double doubleValue;
	};
	unsigned char ByteLen;

	_AsmData();
	_AsmData(float nValue) { floatValue = nValue; ByteLen = 4; }
	_AsmData(double nValue) { doubleValue = nValue; ByteLen = 8; }

	AsmByte* Write(AsmByte* pAddress)
	{
		switch (ByteLen)
		{
		case 4:
			*(float*)pAddress = floatValue;
			break;
		case 8:
			*(double*)pAddress = doubleValue;
			break;
		}
		return pAddress + 8; // align all data on 8 byte boundary
	}
} AsmData;

template<typename T>
inline AsmType AsmGetType(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(T);
	nAlignment = __alignof(T);
	bSigned = false;
	return T_STRUCT;
}

template<>
inline AsmType AsmGetType<bool>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(bool);
	nAlignment = __alignof(bool);
	bSigned = false;
	return T_BOOL;
}

template<>
inline AsmType AsmGetType<char>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(char);
	nAlignment = __alignof(char);
	bSigned = true;
	return T_CHAR;
}

template<>
inline AsmType AsmGetType<unsigned char>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(unsigned char);
	nAlignment = __alignof(unsigned char);
	bSigned = false;
	return T_UCHAR;
}

template<>
inline AsmType AsmGetType<short>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(short);
	nAlignment = __alignof(short);
	bSigned = true;
	return T_SHORT;
}

template<>
inline AsmType AsmGetType<unsigned short>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(unsigned short);
	nAlignment = __alignof(unsigned short);
	bSigned = false;
	return T_USHORT;
}

template<>
inline AsmType AsmGetType<int>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(int);
	nAlignment = __alignof(int);
	bSigned = true;
	return T_INT;
}

template<>
inline AsmType AsmGetType<unsigned int>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(unsigned int);
	nAlignment = __alignof(unsigned int);
	bSigned = false;
	return T_UINT;
}

template<>
inline AsmType AsmGetType<long>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(long);
	nAlignment = __alignof(long);
	bSigned = true;
	return T_LONG;
}

template<>
inline AsmType AsmGetType<unsigned long>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(unsigned long);
	nAlignment = __alignof(unsigned long);
	bSigned = false;
	return T_ULONG;
}

template<>
inline AsmType AsmGetType<__int64>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(__int64);
	nAlignment = __alignof(__int64);
	bSigned = true;
	return T_INT64;
}

template<>
inline AsmType AsmGetType<unsigned __int64>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(unsigned __int64);
	nAlignment = __alignof(unsigned __int64);
	bSigned = false;
	return T_UINT64;
}

template<>
inline AsmType AsmGetType<void*>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(void*);
	nAlignment = __alignof(void*);
	bSigned = false;
	return T_PTR;
}

template<>
inline AsmType AsmGetType<float>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(float);
	nAlignment = __alignof(float);
	bSigned = false;
	return T_FLOAT;
}

template<>
inline AsmType AsmGetType<double>(int& nSize, int& nAlignment, bool& bSigned)
{
	nSize = sizeof(double);
	nAlignment = __alignof(double);
	bSigned = false;
	return T_DOUBLE;
}

class RuntimeAssembler
{
public:
	RuntimeAssembler(AsmCallingConvention nCallConv = stdcall, bool bSpillParameters = false, bool bUseFramePointer = false);

	void Reset(AsmCallingConvention nCallConv = stdcall, bool bSpillParameters = false, bool bUseFramePointer = false);
	void WriteCode(void* lpAddress);
	int CodeSize();
	void Finish();
	AsmRegister ReturnRegister();
	RuntimeAssembler& StackMode(bool bEnable, bool bDiscard = false);
	RuntimeAssembler& BreakPoint();

	void Label(const char* pLabel);
	void* LabelAddress(const char* pLabel, void* pCodeAddress);
	RuntimeAssembler& Jump(const char* pLabel);
	RuntimeAssembler& Push(AsmRegister nReg);
	RuntimeAssembler& Push(AsmVariable& pVar);
	RuntimeAssembler& Push(AsmImmediate nValue);
	RuntimeAssembler& Push(const char* pValue);
	RuntimeAssembler& Push(int nValue);
	RuntimeAssembler& Push(unsigned int nValue);
	RuntimeAssembler& Push(float nValue);
	RuntimeAssembler& Push(double nValue);

	RuntimeAssembler& Pop(AsmRegister nReg);

	RuntimeAssembler& Sub(AsmRegister nReg, int nBytes);
	RuntimeAssembler& Sub(AsmRegister nReg, unsigned int nBytes);
	RuntimeAssembler& Sub(AsmRegister nReg, __int64 nBytes);
	RuntimeAssembler& Sub(AsmRegister nReg, unsigned __int64 nBytes);
	RuntimeAssembler& Sub(AsmRegister nReg, AsmRegister nReg2);
	
	RuntimeAssembler& Add(AsmRegister nReg, int nBytes);
	RuntimeAssembler& Add(AsmRegister nReg, unsigned int nBytes);
	// RuntimeAssembler& Add(AsmRegister nReg, __int64 nBytes);
	// RuntimeAssembler& Add(AsmRegister nReg, unsigned __int64 nBytes);
	RuntimeAssembler& Add(AsmRegister nReg, AsmRegister nReg2);

	RuntimeAssembler& Adcx(AsmRegister nRegDest, AsmRegister nRegSource);
	RuntimeAssembler& Adcx(AsmRegister nRegDest, AsmVariable pSource);
	RuntimeAssembler& Adox(AsmRegister nRegDest, AsmRegister nRegSource);
	RuntimeAssembler& Adox(AsmRegister nRegDest, AsmVariable pSource);

	RuntimeAssembler& Dec(AsmRegister nReg);
	RuntimeAssembler& Inc(AsmRegister nReg);

	RuntimeAssembler& And(AsmRegister nReg, int nValue);
	RuntimeAssembler& And(AsmRegister nReg, unsigned int nValue);
	RuntimeAssembler& And(AsmRegister nReg, __int64 nValue);
	RuntimeAssembler& And(AsmRegister nReg, unsigned __int64 nValue);
	RuntimeAssembler& And(AsmRegister nReg, AsmRegister nReg2);

	RuntimeAssembler& Or(AsmRegister nReg, int nValue);
	RuntimeAssembler& Or(AsmRegister nReg, unsigned int nValue);
	RuntimeAssembler& Or(AsmRegister nReg, __int64 nValue);
	RuntimeAssembler& Or(AsmRegister nReg, unsigned __int64 nValue);
	RuntimeAssembler& Or(AsmRegister nReg, AsmRegister nReg2);

	RuntimeAssembler& Xor(AsmRegister nReg, int nValue);
	RuntimeAssembler& Xor(AsmRegister nReg, unsigned int nValue);
	RuntimeAssembler& Xor(AsmRegister nReg, __int64 nValue);
	RuntimeAssembler& Xor(AsmRegister nReg, unsigned __int64 nValue);
	RuntimeAssembler& Xor(AsmRegister nReg, AsmRegister nReg2);


	RuntimeAssembler& Cdq();
	RuntimeAssembler& CvtSd2Si(AsmRegister nRegDest, AsmRegister nRegSource); // convert double floatingpoint number into integer 
	RuntimeAssembler& CvtSd2Si(AsmRegister nRegDest, AsmVariable pVar);  
	RuntimeAssembler& CvttSd2Si(AsmRegister nRegDest, AsmRegister nRegSource); // convert double floatingpoint number into integer with truncation
	RuntimeAssembler& CvttSd2Si(AsmRegister nRegDest, AsmVariable pVar);
	RuntimeAssembler& CvtSi2Sd(AsmRegister nRegDest, AsmRegister nRegSource); // convert integer into double floatingpoint number
	RuntimeAssembler& CvtSi2Sd(AsmRegister nRegDest, AsmVariable pVar);

	RuntimeAssembler& Sar(AsmRegister nReg, int nBits);
	RuntimeAssembler& Sal(AsmRegister nReg, int nBits);
	RuntimeAssembler& Shl(AsmRegister nReg, int nBits);
	RuntimeAssembler& Shr(AsmRegister nReg, int nBits);

	// Load effective address
	RuntimeAssembler& Lea(AsmRegister nReg, AsmVariable& pVar);
	
	RuntimeAssembler& Mov(AsmRegister nReg, AsmVariable& pVar);
	RuntimeAssembler& Mov(AsmRegister nReg, AsmImmediate nValue);
	RuntimeAssembler& Mov(AsmVariable& pVar, AsmImmediate nValue);
	RuntimeAssembler& Mov(AsmRegister nReg, char pValue);
	RuntimeAssembler& Mov(AsmVariable& pVar, char pValue);
	RuntimeAssembler& Mov(AsmRegister nReg, const char* pValue);
	RuntimeAssembler& Mov(AsmVariable& pVar, const char* pValue);
	RuntimeAssembler& Mov(AsmRegister nReg, int nValue);
	RuntimeAssembler& Mov(AsmVariable& pVar, int nValue);
	RuntimeAssembler& Mov(AsmRegister nReg, unsigned int nValue);
	RuntimeAssembler& Mov(AsmVariable& pVar, unsigned int nValue);
	RuntimeAssembler& Mov(AsmRegister nReg, unsigned __int64 nValue);
	RuntimeAssembler& Mov(AsmVariable& pVar, unsigned __int64 nValue);
	RuntimeAssembler& Mov(AsmRegister nReg, float nValue);
	RuntimeAssembler& Mov(AsmVariable& pVar, float nValue);
	RuntimeAssembler& Mov(AsmRegister nReg, double nValue);
	RuntimeAssembler& Mov(AsmVariable& pVar, double nValue);
	RuntimeAssembler& Mov(AsmRegister nRegDest, AsmRegister nRegSource);
	RuntimeAssembler& Mov(AsmVariable& pVar, AsmRegister nReg);
	RuntimeAssembler& Mov(AsmVariable& pVar, AsmVariable& nReg);

	// Mov with zero extension
	RuntimeAssembler& MovZX(AsmRegister nReg, AsmVariable pVar);
	RuntimeAssembler& MovZX(AsmRegister nRegSource, AsmRegister nRegDest);

	// Mov with sign extension
	RuntimeAssembler& MovSX(AsmRegister nReg, AsmVariable pVar);
	RuntimeAssembler& MovSX(AsmRegister nRegDest, AsmRegister nRegSource);

	// Fld -  Pushes a Floatingpoint number from the source onto the top of the FPU Stack.
	RuntimeAssembler& Fld(AsmVariable pVar);
	RuntimeAssembler& Fld(float nValue);
	RuntimeAssembler& Fld(double nValue);

	// Fstp - Store a Floatingpoint number from the top of the FPU stack into the destination.
	RuntimeAssembler& Fst(AsmRegister nReg);
	RuntimeAssembler& Fst(AsmVariable pVar);

	// Fstp -  Store and Pop a Floatingpoint number from the top of the FPU stack into a the destination.
	RuntimeAssembler& Fstp(AsmRegister nReg);
	RuntimeAssembler& Fstp(AsmVariable pVar);

	// vmovsp - 
	RuntimeAssembler& VMovsp(AsmRegister nReg);
	RuntimeAssembler& VMovsp(AsmVariable pVar);

	RuntimeAssembler& Cmp(AsmRegister nReg, int nValue);
	RuntimeAssembler& Cmp(AsmRegister nReg, AsmRegister nReg2);

	// conditional jumps
	RuntimeAssembler& Jo(const char* pLabel);
	RuntimeAssembler& Jno(const char* pLabel);
	RuntimeAssembler& Jc(const char* pLabel);
	RuntimeAssembler& Jb(const char* pLabel);
	RuntimeAssembler& Jae(const char* pLabel);
	RuntimeAssembler& Je(const char* pLabel);
	RuntimeAssembler& Jz(const char* pLabel);
	RuntimeAssembler& Jne(const char* pLabel);
	RuntimeAssembler& Jbe(const char* pLabel);
	RuntimeAssembler& Ja(const char* pLabel);
	RuntimeAssembler& Js(const char* pLabel);
	RuntimeAssembler& Jns(const char* pLabel);
	RuntimeAssembler& Jnp(const char* pLabel);
	RuntimeAssembler& Jl(const char* pLabel);
	RuntimeAssembler& Jge(const char* pLabel);
	RuntimeAssembler& Jle(const char* pLabel);
	RuntimeAssembler& Jg(const char* pLabel);

	// direct jumps
	RuntimeAssembler& Jmp(const char* pLabel);
	RuntimeAssembler& Jmp(AsmRegister nReg, AsmImmediate pLocation);
	RuntimeAssembler& Jmp(AsmRegister nReg);

	RuntimeAssembler& CallConv(AsmCallingConvention nConv);
	// function invocation
	AsmRegister Call(AsmRegister nReg, AsmFuncPtr pFunction);
	RuntimeAssembler& Ret(int nBytes = -1);

	template<typename T>
	AsmVariable& Parameter()
	{
		AsmVariable pVar;
		int nCount = m_Parameters.GetCount();
		pVar.Type = AsmGetType<T>(pVar.Size, pVar.Alignment, pVar.Signed);
		pVar.Fixed = false;
#if !defined(_WIN64)
		pVar.Location = Memory;
		if (nCount > 0)
		{
			pVar.Offset = m_Parameters[nCount - 1].Offset + 4;
		}
		else
			pVar.Offset = 8;

		pVar.Register = StackFrameRegister();
#else
		if (nCount < 4)
		{
			if (m_SpillParameters)
			{
				pVar.Location = AsmLocation::Memory;
				pVar.Offset = m_FrameOffset + (nCount + 1) * 8;
				pVar.Register = StackFrameRegister();
			}
			else
			{
				pVar.Location = AsmLocation::Register;
				pVar.Offset = 0;
				if (nCount == 0)
					pVar.Register = (pVar.Type == AsmType::T_FLOAT || pVar.Type == T_DOUBLE) ? XMM0 : RCX;
				else if (nCount == 1)
					pVar.Register = (pVar.Type == AsmType::T_FLOAT || pVar.Type == T_DOUBLE) ? XMM1 : RDX;
				else if (nCount == 2)
					pVar.Register = (pVar.Type == AsmType::T_FLOAT || pVar.Type == T_DOUBLE) ? XMM2 : R8;
				else
					pVar.Register = (pVar.Type == AsmType::T_FLOAT || pVar.Type == T_DOUBLE) ? XMM3 : R9;
				if (IsRegisterMarked(pVar.Register))
				{
					pVar.Location = AsmLocation::Memory;
					pVar.Offset = m_FrameOffset + (nCount + 1) * 8;
					pVar.Register = StackFrameRegister();
				}
			}
		}
		else
		{
			pVar.Location = AsmLocation::Memory;
			pVar.Offset = m_FrameOffset + 32 + ((nCount - 3) * 8); // m_FrameOffset = 16 when we use the stackpointer RBP or 8 when RSP is used, 4 * 8 bytes shadow space for RCX, RDX, R8 and R9 is always allocated + 8 bytes for each parameter after the 4th 
			pVar.Register = StackFrameRegister();
		}
#endif
		int nIndex = m_Parameters.Add(pVar);
		return m_Parameters[nIndex];
	}

	template<typename T>
	AsmVariable& LocalVar()
	{
		AsmVariable pVar;
		int nCount = m_Variables.GetCount();
		pVar.Location = Memory;
		pVar.Register = StackFrameRegister();
		pVar.Fixed = false;
		pVar.Type = AsmGetType<T>(pVar.Size, pVar.Alignment, pVar.Signed);
#if !defined(_WIN64)
		if (nCount > 0)
		{
			pVar.Offset = m_Variables[nCount - 1].Offset - pVar.Size;
			pVar.Offset -= (-pVar.Offset % pVar.Alignment);
		}
		else
			pVar.Offset = -pVar.Size;
#else
		if (nCount > 0)
		{
			pVar.Offset = m_Variables[nCount - 1].Offset - pVar.Size;
			pVar.Offset -= (-pVar.Offset % pVar.Alignment);
		}
		else
			pVar.Offset = 0;
#endif
		int nIndex = m_Variables.Add(pVar);
		return m_Variables[nIndex];
	}

	template<typename T>
	AsmVariable Var(AsmVariable& pBase, int nOffset)
	{
		AsmVariable pVar;
		pVar.Location = pBase.Location;
		pVar.Offset = pBase.Offset;
		pVar.Register = pBase.Register;
		pVar.Type = AsmGetType<T>(pVar.Size, pVar.Alignment, pVar.Signed);
#if defined(_WIN64)
		pVar.Offset -= nOffset;
#else
		pVar.Offset += nOffset;
#endif
		return pVar;
	}

	AsmRegister GetTemporayRegister(int nSize)
	{
		if (!IsRegisterMarked(RCX))
		{
			switch (nSize)
			{
			case 1:
				return CL;
			case 2:
				return CX;
			case 4:
				return ECX;
			default:
#if !defined(_WIN64)
				return RCX;
#else
				return ECX;
#endif
			}
		}
		else if (!IsRegisterMarked(RDX))
		{
			switch (nSize)
			{
			case 1:
				return DL;
			case 2:
				return DX;
			case 4:
				return EDX;
			default:
#if !defined(_WIN64)
				return RDX;
#else
				return EDX;
#endif
			}
		}
		else if (!IsRegisterMarked(RBX))
		{
			switch (nSize)
			{
			case 1:
				return BL;
			case 2:
				return BX;
			case 4:
				return EBX;
			default:
#if !defined(_WIN64)
				return RBX;
#else
				return EBX;
#endif
			}
		}
		else
		{
			switch (nSize)
			{
			case 1:
				return AL;
			case 2:
				return AX;
			case 4:
				return EAX;
			default:
#if !defined(_WIN64)
				return RAX;
#else
				return EAX;
#endif
			}
		}
	}

	template<typename T>
	AsmRegister Call(T pFunction)
	{
		AsmRegister reg = Call((AsmFuncPtr)pFunction);
		return reg;
	}

	template<>
	AsmRegister Call<AsmFuncPtr>(AsmFuncPtr pFunction)
	{
#if !defined(_WIN64)
		return Call(EAX, pFunction);
#else
		return Call(RAX, pFunction);
#endif	
	}

	template<>
	AsmRegister Call<AsmRegister>(AsmRegister nReg)
	{
		AsmInstruction ins;
		ins.Opcode = OP_CALL_R32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = MODRM_OP_CALL;
		ins.ModRM.Rm = nReg;
		m_Code->Add(ins);
		OnCall(0);
		return nReg;
	}

	template<typename F>
	AsmRegister ManualCall(F pFunc)
	{
		AsmRegister nReg = Call(pFunc);
#if !defined(_WIN64)
		if (m_SubCallConv == cdeclaration)
		{
			if (m_CodeStack.GetCount() > 0)
				Add(ESP, m_ParameterCount * sizeof(void*));
		}
#endif
		return nReg;
	}

	template<typename F, typename T1>
	AsmRegister Call(F pFunc, T1 parm1)
	{
		int nParmNo = 0;
#if !defined(_WIN64)
		PassParameter(parm1, nParmNo);
		Call(pFunc);
		if (m_SubCallConv == cdeclaration)
			Add(ESP, 1 * sizeof(void*));
		OnCall(1);
		return EAX;
#else
		PassParameter(parm1, nParmNo);
		Call(pFunc);
		OnCall(1);
		return RAX;
#endif
	}

	template<typename F, typename T1, typename T2>
	AsmRegister Call(F pFunc, T1 parm1, T2 parm2)
	{
		int nParmNo = 0;
#if !defined(_WIN64)
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		StackMode(false);
		Call((AsmFuncPtr)pFunc);
		if (m_SubCallConv == cdeclaration)
			Add(ESP, 2 * sizeof(void*));
		OnCall(2);
		return EAX;
#else
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		StackMode(false);
		Call(pFunc);
		OnCall(2);
		return RAX;
#endif
	}

	template<typename F, typename T1, typename T2, typename T3>
	AsmRegister Call(F pFunc, T1 parm1, T2 parm2, T3 parm3)
	{
		int nParmNo = 0;	
#if !defined(_WIN64)
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		PassParameter(parm3, nParmNo);
		StackMode(false);
		Call((AsmFuncPtr)pFunc);
		if (m_SubCallConv == cdeclaration)
			Add(ESP, 3 * sizeof(void*));
		OnCall(3);
		return EAX;
#else
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		PassParameter(parm3, nParmNo);
		StackMode(false);
		Call(pFunc);
		OnCall(3);
		return RAX;
#endif
	}

	template<typename F, typename T1, typename T2, typename T3, typename T4>
	AsmRegister Call(F pFunc, T1 parm1, T2 parm2, T3 parm3, T4 parm4)
	{
		int nParmNo = 0;
#if !defined(_WIN64)
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		PassParameter(parm3, nParmNo);
		PassParameter(parm4, nParmNo);
		StackMode(false);
		Call(pFunc);
		if (m_SubCallConv == cdeclaration)
			Add(ESP, 4 * sizeof(void*));
		OnCall(4);
		return EAX;
#else
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		PassParameter(parm3, nParmNo);
		PassParameter(parm4, nParmNo);
		StackMode(false);
		Call(pFunc);
		OnCall(4);
		return RAX;
#endif
	}

	template<typename F, typename T1, typename T2, typename T3, typename T4, typename T5>
	AsmRegister Call(F pFunc, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5)
	{
		int nParmNo = 0;
		
#if !defined(_WIN64)
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		PassParameter(parm3, nParmNo);
		PassParameter(parm4, nParmNo);
		PassParameter(parm5, nParmNo);
		StackMode(false);
		Call(pFunc);
		if (m_SubCallConv == cdeclaration)
			Add(ESP, 5 * sizeof(void*));
		OnCall(5);
		return EAX;
#else
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		PassParameter(parm3, nParmNo);
		PassParameter(parm4, nParmNo);
		PassParameter(parm5, nParmNo);
		StackMode(false);
		Call(pFunc);
		OnCall(5);
		return RAX;
#endif
	}

	template<typename F, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
	AsmRegister Call(F pFunc, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6)
	{
		int nParmNo = 0;
#if !defined(_WIN64)
		int nParmNo = 0;
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		PassParameter(parm3, nParmNo);
		PassParameter(parm4, nParmNo);
		PassParameter(parm5, nParmNo);
		PassParameter(parm6, nParmNo);
		StackMode(false);
		Call(pFunc);
		if (m_SubCallConv == cdeclaration)
			Add(ESP, 6 * sizeof(void*));
		OnCall(6);
		return EAX;
#else
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		PassParameter(parm3, nParmNo);
		PassParameter(parm4, nParmNo);
		PassParameter(parm5, nParmNo);
		PassParameter(parm6, nParmNo);
		StackMode(false);
		Call(pFunc);
		OnCall(6);
		return RAX;
#endif
	}

	template<typename F, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
	AsmRegister Call(F pFunc, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7)
	{
		int nParmNo = 0;
#if !defined(_WIN64)
		int nParmNo = 0;
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		PassParameter(parm3, nParmNo);
		PassParameter(parm4, nParmNo);
		PassParameter(parm5, nParmNo);
		PassParameter(parm6, nParmNo);
		PassParameter(parm7, nParmNo);
		StackMode(false);
		Call(pFunc);
		if (m_SubCallConv == cdeclaration)
			Add(ESP, 7 * sizeof(void*));
		OnCall(7);
		return EAX;
#else
		StackMode(true);
		PassParameter(parm1, nParmNo);
		PassParameter(parm2, nParmNo);
		PassParameter(parm3, nParmNo);
		PassParameter(parm4, nParmNo);
		PassParameter(parm5, nParmNo);
		PassParameter(parm6, nParmNo);
		PassParameter(parm7, nParmNo);
		StackMode(false);
		Call(pFunc);
		OnCall(7);
		return RAX;
#endif
	}

	template<typename T>
	void PassParameter(T parm, int& nParmNo)
	{
		m_ParameterCount++;
#if !defined(_WIN64)
		if (m_SubCallConv == fastcall && nParmNo <= 1)
		{
			int nSize = sizeof(T);
			int nLastParmNo = nParmNo;
			AsmRegister nReg = GetParameterRegister(parm, nParmNo);
			if (nLastParmNo != nParmNo)
			{
				Mov(nReg, parm);
			}
			else
			{
				Push(parm);
			}
		}
		else
		{
			Push(parm);
		}
#else
		if (nParmNo <= 3)
		{
			int nSize = sizeof(T);
			AsmRegister nReg = GetParameterRegister(parm, nParmNo);
			assert(nSize <= 8);
			Mov(nReg, parm);
		}
		else
		{
			if (m_UseFramePointer)
			{
				Mov(AsmVariable(StackFrameRegister(), -8 - (8 * (nParmNo - 4)), true), parm);
			}
			else
			{
				Mov(AsmVariable(StackFrameRegister(), 32 + (8 * (nParmNo - 4)), true), parm);
			}
			nParmNo++;
			m_SubFunctionParameterSize = nParmNo * 8;
		}
#endif
	}

	template<>
	void PassParameter<float>(float parm, int& nParmNo)
	{
		m_ParameterCount++;
#if !defined(_WIN64)
		Push(parm);
#else
		if (nParmNo <= 3)
		{
			AsmRegister nReg = GetParameterRegister(parm, nParmNo);
			Mov(nReg, parm);
		}
		else
		{
			if (m_UseFramePointer)
			{
				Mov(AsmVariable(StackFrameRegister(), -8 - (8 * (nParmNo - 4)), true), parm);
			}
			else
			{
				Mov(AsmVariable(StackFrameRegister(), 32 + (8 * (nParmNo - 4)), true), parm);
			}
			nParmNo++;
			m_SubFunctionParameterSize = nParmNo * 8;
		}
#endif
	}

	template<>
	void PassParameter<double>(double parm, int& nParmNo)
	{
		m_ParameterCount++;
#if !defined(_WIN64)
		Push(parm);
#else
		if (nParmNo <= 3)
		{
			AsmRegister nReg = GetParameterRegister(parm, nParmNo);
			Mov(nReg, parm);
		}
		else
		{
			if (m_UseFramePointer)
			{
				Mov(AsmVariable(StackFrameRegister(), -8 - (8 * (nParmNo - 4)), true), parm);
			}
			else
			{
				Mov(AsmVariable(StackFrameRegister(), 32 + (8 * (nParmNo - 4)), true), parm);
			}
			nParmNo++;
			m_SubFunctionParameterSize = nParmNo * 8;
		}
#endif
	}

	template<>
	void PassParameter<AsmVariable>(AsmVariable parm, int& nParmNo)
	{
		m_ParameterCount++;
#if !defined(_WIN64)
		if (m_SubCallConv == fastcall && nParmNo <= 1)
		{
			int nLastParmNo = nParmNo;
			AsmRegister nReg = GetParameterRegister(parm, nParmNo);
			if (nLastParmNo != nParmNo)
			{
				if (parm.Size == 1 || parm.Size == 2 || parm.Size == 4)
				{
					Mov(nReg, parm);
				}
				else
				{
					Lea(nReg, parm);
				}
			}
			else
			{
				Push(parm);
			}
		}
		else
		{
			Push(parm);
		}
#else
		if (nParmNo <= 3)
		{
			AsmRegister nReg = GetParameterRegister(parm, nParmNo);
			if (parm.Size == 1 || parm.Size == 2 || parm.Size == 4 || parm.Size == 8)
			{
				Mov(nReg, parm);
			}
			else
			{
				Lea(nReg, parm);
			}
		}
		else
		{
			if (m_UseFramePointer)
			{
				Mov(AsmVariable(StackFrameRegister(), -8 - (8 * (nParmNo - 4)), true), parm);
			}
			else
			{
				Mov(AsmVariable(StackFrameRegister(), 32 + (8 * (nParmNo - 4)), true), parm);
			}
			nParmNo++;
			m_SubFunctionParameterSize = nParmNo * 8;
		}
#endif
	}

	template<>
	void PassParameter<AsmVariable*>(AsmVariable* parm, int& nParmNo)
	{
		m_ParameterCount++;
#if !defined(_WIN64)
		if (m_SubCallConv == fastcall && nParmNo <= 1)
		{
			int nLastParmNo = nParmNo;
			AsmRegister nReg = GetParameterRegister(parm, nParmNo);
			if (nLastParmNo != nParmNo)
			{
				Lea(nReg, *parm);
				return;
			}
		}
		if (m_InStackMode == false)
		{
			Lea(EAX, *parm);
			Push(EAX);
		}
		else
		{
			Push(EAX);
			Lea(EAX, *parm);
		}
#else
		if (nParmNo <= 3)
		{
			AsmRegister nReg = GetParameterRegister(parm, nParmNo);
			Lea(nReg, *parm);
		}
		else
		{
			AsmVariable pVar;
			if (m_UseFramePointer)
				pVar = AsmVariable(StackFrameRegister(), -8 - (8 * (nParmNo - 4)), true);
			else
				pVar = AsmVariable(StackFrameRegister(), 32 + (8 * (nParmNo - 4)), true);

			if (m_InStackMode == false)
			{
				Lea(RAX, *parm);
				Mov(pVar, RAX);
			}
			else
			{
				Mov(pVar, RAX);
				Lea(RAX, *parm);
			}
			nParmNo++;
			m_SubFunctionParameterSize = nParmNo * 8;
		}
#endif
	}

	static void* AllocateMemory(size_t nSize);
	static void FreeMemory(void* pAddress);
	void RegisterUnwindInfo(void* lpCodeAddress);
	static void UnregisterUnwindInfo(void* lpCodeAddress);

private:

	template<typename T>
	AsmRegister GetParameterRegister(T parm, int& nParmNo)
	{
		int nSize = sizeof(T);
		AsmType nType = nSize == 4 ? T_INT : nSize == 8 ? T_INT64 : T_STRUCT;
		return GetParameterRegister(nSize, nType, nParmNo);
	}

	template<>
	AsmRegister GetParameterRegister(AsmVariable parm, int& nParmNo)
	{
		return GetParameterRegister(parm.Size, parm.Type, nParmNo);
	}

	AsmRegister GetParameterRegister(int nSize, AsmType nType, int& nParmNo);
	AsmRegister GetParameterRegister(AsmRegister parm, int& nParmNo);

	AsmLabel* LabelRef(const char *pLabel);
	RuntimeAssembler& Shift_Ex(AsmRegister nReg, int nBits, int nOpcode);
	RuntimeAssembler& AddJump(AsmInstruction& ins, const char* pLabel);

	void Prolog();
	void Epilog();
	void OnCall(int nParmCount);
	void SpillRegister(AsmRegister nReg);
	size_t Data(float nValue);
	size_t Data(double nValue);
	int ParameterSize();
	int StackSize();
	void Patch();
	void MarkRegister(AsmRegister nReg);
	int RegisterMask(AsmRegister nReg);
	bool IsRegisterMarked(AsmRegister nReg);
	bool IsRegisterNonVolatile(AsmRegister nReg);
	int TypeBitCount(AsmType nType);
	int TypeBitCount(int nSize);

	UINT m_UsedRegisters;
	int m_SavedRegisterSpace;
	int m_SubFunctionParameterSize;
	int m_ParameterCount;
	int m_FrameOffset;
	int m_CachedCodeSize;
	int m_CachedPrologSize;
	int m_DataSegmentOffset;
	bool m_InStackMode;
	bool m_InPrologEpilog;
	bool m_UseFramePointer;
	bool m_SpillParameters;
	bool m_Finished;
	AsmCallingConvention m_CallConv;
	AsmCallingConvention m_SubCallConv;
#if defined(_WIN64)
	int m_UnwindInfoOffset;
	CArray<unsigned short> m_UnwindData;
	void CreateUnwindInfo();
#endif

	CArray<AsmVariable> m_Parameters;
	CArray<AsmVariable> m_Variables;
	CArray<AsmLabel> m_Labels;
	CArray<AsmLabel> m_Jumps;

	CArray<AsmData> m_DataSegment;
	CArray<AsmInstruction>* m_Code;
	CArray<AsmInstruction> m_PrologSegment;
	CArray<AsmInstruction> m_CodeSegment;
	CArray<AsmInstruction> m_CodeStack;

	AsmByte Make_Op(AsmByte nOpcode, AsmRegister nReg)
	{
		AsmByte nCode = nOpcode + Make_Reg_Code(nReg);
		return nCode;
	}

	AsmRegister StackFrameRegister()
	{
#if !defined(_WIN64)
		return EBP;
#else
		if (m_UseFramePointer)
			return RBP;
		return RSP;
#endif
	}

	void ModRm_Op_Reg(AsmInstruction& rIns, AsmByte nOpcode, AsmRegister nRegDest)
	{
		rIns.ModRM._Used = 1;
		rIns.ModRM.Reg = nOpcode;
		rIns.ModRM.Rm = nRegDest;
		rIns.ModRM.Mod = MODRM_MOD_REGISTER;
	}

	void ModRm_Var(AsmInstruction& rIns, AsmVariable& pSource)
	{
		rIns.ModRM._Used = 1;
		if (pSource.Location == Memory)
		{
			if (pSource.Register == ESP || pSource.Register == RSP || pSource.Register == R12)
			{
				rIns.ModRM.Rm = MODRM_RM_SIB;
				rIns.Sib.Index = SIB_INDEX_DISP;
				rIns.Sib.Base = pSource.Register;
			}
			else
			{
				rIns.ModRM.Rm = pSource.Register;
			}
			rIns.ModRM.Mod = rIns.Displacement.Displacement(pSource.Offset, true, pSource.Fixed);
		}
		else
		{
			if (pSource.Register == ESP || pSource.Register == RSP || pSource.Register == R12)
			{
				rIns.ModRM.Rm = MODRM_RM_SIB;
				rIns.Sib.Index = pSource.Register;
				rIns.Sib.Base = SIB_INDEX_DISP;
				rIns.Displacement.Displacement(0, true);
			}
			else
			{
				rIns.ModRM.Rm = pSource.Register;
				rIns.ModRM.Mod = MODRM_MOD_REGISTER;
			}
		}
	}

	void ModRm_Var_Reg(AsmInstruction& rIns, AsmVariable& pDest, AsmRegister nRegSource)
	{
		rIns.ModRM._Used = 1;
		rIns.ModRM.Reg = nRegSource;
		if (pDest.Location == Memory)
		{
			if (pDest.Register == ESP || pDest.Register == RSP || pDest.Register == R12)
			{
				rIns.ModRM.Rm = MODRM_RM_SIB;
				rIns.Sib.Index = SIB_INDEX_DISP;
				rIns.Sib.Base = pDest.Register;
			}
			else
			{
				rIns.ModRM.Rm = pDest.Register;
			}
			rIns.ModRM.Mod = rIns.Displacement.Displacement(pDest.Offset, true, pDest.Fixed);
		}
		else
		{
			if (pDest.Register == ESP || pDest.Register == RSP || pDest.Register == R12)
			{
				rIns.ModRM.Rm = MODRM_RM_SIB;
				rIns.Sib.Index = pDest.Register;
				rIns.Sib.Base = SIB_INDEX_DISP;
				rIns.Displacement.Displacement(0, true);
			}
			else
			{
				rIns.ModRM.Rm = pDest.Register;
				rIns.ModRM.Mod = MODRM_MOD_REGISTER;
			}
		}
	}

	void ModRm_Reg_Var(AsmInstruction& rIns, AsmRegister nRegDest, AsmVariable& pSource)
	{
		// 41 8B 84 24 80 00 00 00
		rIns.ModRM._Used = 1;
		rIns.ModRM.Reg = nRegDest;
		if (pSource.Location == Memory)
		{
			if (pSource.Register == ESP || pSource.Register == RSP || pSource.Register == R12)
			{
				rIns.ModRM.Rm = MODRM_RM_SIB;
				rIns.Sib.Index = pSource.Register;
				rIns.Sib.Base = SIB_INDEX_DISP;
			}
			else
			{
				rIns.ModRM.Rm = pSource.Register;
			}
			rIns.ModRM.Mod = rIns.Displacement.Displacement(pSource.Offset, true, pSource.Fixed);
		}
		else
		{
			if (pSource.Register == ESP || pSource.Register == RSP || pSource.Register == R12)
			{
				rIns.ModRM.Rm = MODRM_RM_SIB;
				rIns.Sib.Index = pSource.Register;
				rIns.Sib.Base = SIB_INDEX_DISP;
				rIns.Displacement.Displacement(0, true);
			}
			else
			{
				rIns.ModRM.Rm = pSource.Register;
				rIns.ModRM.Mod = MODRM_MOD_REGISTER;
			}
		}
	}

	void ModRm_Reg_Reg(AsmInstruction& rIns, AsmRegister nRegDest, AsmRegister nRegSource)
	{
		rIns.ModRM._Used = 1;
		rIns.ModRM.Reg = nRegDest;
		if (nRegSource == RSP || nRegSource == ESP || nRegSource == R12)
		{
			rIns.ModRM.Rm = MODRM_RM_SIB;
			rIns.Sib.Index = nRegSource;
			rIns.Sib.Base = SIB_INDEX_DISP;
			rIns.ModRM.Mod = rIns.Displacement.Displacement(0, true);
			rIns.Displacement.Displacement(0, true);
		}
		else
		{
			rIns.ModRM.Rm = nRegSource;
			rIns.ModRM.Mod = MODRM_MOD_REGISTER;
		}
	}

	void ModRm_Op_Var(AsmInstruction& rIns, AsmByte nOpcode, AsmVariable& pSource)
	{
		rIns.ModRM._Used = 1;
		rIns.ModRM.Reg = nOpcode;
		AsmRegister nFrameReg = StackFrameRegister();
		if (pSource.Location == Memory)
		{
			rIns.ModRM.Rm = pSource.Register;
			rIns.ModRM.Mod = rIns.Displacement.Displacement(pSource.Offset, true, pSource.Fixed);
			if (pSource.Register == RSP || pSource.Register == ESP || pSource.Register == R12)
			{
				rIns.ModRM.Rm = MODRM_RM_SIB;
				rIns.Sib.Index = pSource.Register;
				rIns.Sib.Base = SIB_INDEX_DISP;
			}
		}
		else
		{
			if (pSource.Register == RSP || pSource.Register == ESP || pSource.Register == R12)
			{
				rIns.ModRM.Rm = MODRM_RM_SIB;
				rIns.Sib.Index = pSource.Register;
				rIns.Sib.Base = SIB_INDEX_DISP;
				rIns.Displacement.Displacement(0, true);
			}
			else
			{
				rIns.ModRM.Rm = pSource.Register;
				rIns.ModRM.Mod = MODRM_MOD_REGISTER;
			}
		}
	}

	void ModRm_Stack_Disp(AsmInstruction& rIns, AsmByte nOpcode, int nDisplacement = 0)
	{
		rIns.ModRM._Used = 1;
		rIns.ModRM.Rm = StackFrameRegister();
		rIns.ModRM.Mod = rIns.Displacement.Displacement(nDisplacement);
	}

	void ModRm_Op_Stack_Disp(AsmInstruction& rIns, AsmByte nOpcode, int nDisplacement = 0)
	{
		rIns.ModRM._Used = 1;
		rIns.ModRM.Reg = nOpcode;
		rIns.ModRM.Rm = StackFrameRegister();
		rIns.ModRM.Mod = rIns.Displacement.Displacement(nDisplacement);
	}

	void ModRm_Reg_Stack_Disp(AsmInstruction& rIns, AsmRegister nReg, int nDisplacement = 0)
	{
		rIns.ModRM._Used = 1;
		rIns.ModRM.Reg = nReg;
		rIns.ModRM.Rm = StackFrameRegister();
		rIns.ModRM.Mod = rIns.Displacement.Displacement(nDisplacement, true);
#if defined(_WIN64)
		if (nDisplacement == 0)
		{
			rIns.Sib.Scale = 0;
			rIns.Sib.Base = StackFrameRegister();
			rIns.Sib.Index = StackFrameRegister();
		}
#endif
	}

	// use over macro Rex_Prefix_WR
	AsmByte Rex_Prefix_WR_Impl(AsmRegister nReg) {
		AsmByte nPrefix = 0;
		if (nReg >= RAX)
		{
			// REX.W bit for 64bit operation
			nPrefix |= (PREFIX_REX | PREFIX_REX_W);
		}
		if ((nReg & PREFIX_REX_REG_MASK) > 0)
		{
			// set REX.R for extended registers
			nPrefix |= (PREFIX_REX | PREFIX_REX_R);
		}
		return nPrefix;
	}

	// use over macro Rex_Prefix_WB
	AsmByte Rex_Prefix_WB_Impl(AsmRegister nReg) {
		AsmByte nPrefix = 0;
		if (nReg >= RAX)
		{
			// REX.W bit for 64bit operation
			nPrefix |= (PREFIX_REX | PREFIX_REX_W);
		}
		if ((nReg & PREFIX_REX_REG_MASK) > 0)
		{
			// set REX.R for extended registers
			nPrefix |= (PREFIX_REX | PREFIX_REX_B);
		}
		return nPrefix;
	}

	// use over macro Rex_Prefix_WRB
	AsmByte Rex_Prefix_WRB_Impl(AsmRegister nRegSource, AsmRegister nRegDest) {
		AsmByte nPrefix = 0;
		if (nRegSource >= RAX)
		{
			// REX.W bit for 64bit operation
			nPrefix |= (PREFIX_REX | PREFIX_REX_W);
		}
		if ((nRegSource & PREFIX_REX_REG_MASK) > 0)
		{
			// set REX.R bit for extended registers
			nPrefix |= (PREFIX_REX | PREFIX_REX_R);
		}

		if ((nRegDest & PREFIX_REX_REG_MASK) > 0)
		{
			// set REX.B bit for extended registers
			nPrefix |= (PREFIX_REX | PREFIX_REX_B);
		}
		return nPrefix;
	}

	// use over macro Rex_Prefix_RB
	AsmByte Rex_Prefix_RB_Impl(AsmRegister nRegSource, AsmRegister nRegDest)
	{
		AsmByte nPrefix = 0;
		// set REX.R for extended registers
		if ((nRegSource & PREFIX_REX_REG_MASK) > 0)
		{
			nPrefix = (PREFIX_REX | PREFIX_REX_R);
		}
		if ((nRegDest & PREFIX_REX_REG_MASK) > 0)
		{
			// set REX.B bit for extended registers
			nPrefix |= (PREFIX_REX | PREFIX_REX_B);
		}
		return nPrefix;
	}

	// use over macro Rex_Prefix_B
	AsmByte Rex_Prefix_B_Impl(AsmRegister nReg)
	{
		// set REX.B for extended registers
		if ((nReg & PREFIX_REX_REG_MASK) > 0)
			return (PREFIX_REX | PREFIX_REX_B);
		return 0;
	}

	// use over macro Rex_Prefix_R
	AsmByte Rex_Prefix_R_Impl(AsmRegister nReg)
	{	
		// set REX.R for extended registers
		if ((nReg & PREFIX_REX_REG_MASK) > 0)
			return (PREFIX_REX | PREFIX_REX_R);
		return 0;
	}

	/* Opcodes
	r8/r16/r32/r64					= register of size X
	r8/m8,r16/m16,r32/m32,r64/m64	= register or memory operand of size X
	imm8,imm16,imm32,imm64			= immediate operand of size X
	*/
};

#if defined(_WIN64)
typedef double (*ASMTEST)(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);

__int64 x64AsmTest1(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4, int parm5, unsigned int parm6);
double x64AsmTest2(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
int x64AsmTest3(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
short x64AsmTest4(double parm1, short parm2, __int64 parm3, unsigned __int64 parm4);
char x64AsmTest5(double parm1, char parm2, __int64 parm3, unsigned __int64 parm4);

#else

typedef double (_stdcall *STDCALLASMTEST)(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
typedef __int64 (_cdecl *CDECLASMTEST)(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
typedef __int64 (_fastcall *FASTCALLASMTEST)(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);

__int64 _fastcall fastcallAsmTest1(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
double _fastcall fastcallAsmTest2(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
int _fastcall fastcallAsmTest3(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
short _fastcall fastcallAsmTest4(double parm1, short parm2, __int64 parm3, unsigned __int64 parm4);
char _fastcall fastcallAsmTest5(double parm1, char parm2, __int64 parm3, unsigned __int64 parm4);

__int64 _stdcall stdcallAsmTest1(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
double _stdcall stdcallAsmTest2(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
int _stdcall stdcallAsmTest3(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
short _stdcall stdcallAsmTest4(double parm1, short parm2, __int64 parm3, unsigned __int64 parm4);
char _stdcall stdcallAsmTest5(double parm1, char parm2, __int64 parm3, unsigned __int64 parm4);

__int64 _cdecl cdeclAsmTest1(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
double _cdecl cdeclAsmTest2(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
int _cdecl cdeclAsmTest3(double parm1, int parm2, __int64 parm3, unsigned __int64 parm4);
short _cdecl cdeclAsmTest4(double parm1, short parm2, __int64 parm3, unsigned __int64 parm4);
char _cdecl cdeclAsmTest5(double parm1, char parm2, __int64 parm3, unsigned __int64 parm4);

#endif

void _fastcall AsmTest(ParamBlkEx& parm);

#endif	// _VFP2CASSEMBLY_H__