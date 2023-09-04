#define _WINSOCKAPI_ // we're using winsock2 .. so this is neccessary to exclude winsock.h 

#include "vfp2c32.h"
#include "vfp2cassembly.h"
#include "vfp2ctls.h"
#include "vfp2ccallback.h"
#include "vfp2cdatastructure.h"
#include "vfp2casmfuncs.h"
#include "vfp2cutil.h"

const int AsmTypes[10][3] =  // size, alignment & sign of C types
{
	{ sizeof(char), __alignof(char), TRUE },
	{ sizeof(unsigned char), __alignof(unsigned char), FALSE },
	{ sizeof(short),__alignof(short), TRUE },
	{ sizeof(unsigned short), __alignof(unsigned short), FALSE },
	{ sizeof(int), __alignof(int), TRUE },
	{ sizeof(unsigned int), __alignof(unsigned int), FALSE },
	{ sizeof(__int64), __alignof(__int64), TRUE },
	{ sizeof(unsigned __int64), __alignof(unsigned __int64), FALSE },
	{ sizeof(float), __alignof(float), FALSE },
	{ sizeof(double), __alignof(double), FALSE }
};

AsmByte* AsmInstruction::Write(AsmByte* pAddress, AsmByte* pDataSegmentAddress)
{
	AsmByte* pInstructionAddress = pAddress;
	if (Prefix)
		*pAddress++ = Prefix;
	if (Prefix2)
		*pAddress++ = Prefix2;
	if (Prefix3)
		*pAddress++ = Prefix3;
	if (Prefix4)
		*pAddress++ = Prefix4;
	if (Opcode)
		*pAddress++ = Opcode;
	if (Opcode2)
		*pAddress++ = Opcode2;
	if (Opcode3)
		*pAddress++ = Opcode3;
	if (ModRM)
		*pAddress++ = ModRM;
	if (Sib)
		*pAddress++ = Sib;
	if ((ModRM.Mod == MODRM_MOD_RELATIVE) && (ModRM.Rm == MODRM_RM_RIP_RELATIVE))
		pAddress = Displacement.Write(pAddress, pDataSegmentAddress, pInstructionAddress + Size());
	else
		pAddress = Displacement.Write(pAddress, 0, 0);

	pAddress = Immediate.Write(pAddress);
	return pAddress;
}


AsmVariable::_AsmVariable(_AsmVariable& pVar)
{
	Location = pVar.Location;
	Type = pVar.Type;
	Size = pVar.Size;
	Alignment = pVar.Alignment;
	Signed = pVar.Signed;
	Fixed = pVar.Fixed;
	Offset = pVar.Offset;
	Register = pVar.Register;
}

_AsmVariable::_AsmVariable(AsmRegister nReg, int nOffset, bool fixed)
{
	Location = Memory;
	Register = nReg;
	Size = RegisterBitCount(nReg) / 8;
	Alignment = Size;
	Type = Size == 1 ? T_UCHAR : Size == 2 ? T_USHORT : Size == 4 ? T_UINT : Size == 8 ? T_UINT64 : T_STRUCT;
	Signed = false;
	Offset = nOffset;
	Fixed = fixed;
}

void* RuntimeAssembler::AllocateMemory(size_t nSize)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	void* pAlloc = HeapAlloc(tls.GetJitHeap(), HEAP_ZERO_MEMORY, nSize);
	if (pAlloc == 0)
	{
		SaveWin32Error("HeapAlloc", GetLastError());
		throw E_APIERROR;
	}
	return pAlloc;
}

void RuntimeAssembler::FreeMemory(void* pAddress)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	if (!HeapFree(tls.GetJitHeap(), 0, pAddress))
	{
		SaveWin32Error("HeapAlloc", GetLastError());
		throw E_APIERROR;
	}
}

/* pointer to codesection */
RuntimeAssembler::RuntimeAssembler(AsmCallingConvention nCallConv, bool bSpillParameters, bool bUseFramePointer)
{
	Reset(nCallConv, bSpillParameters, bUseFramePointer);
}

void RuntimeAssembler::WriteCode(void *lpCodeAddress)
{
	AsmByte* pStartAddress = (AsmByte*)lpCodeAddress;
	AsmByte* pWriteAddress = (AsmByte*)lpCodeAddress;
	AsmByte* pDataSegmentAddress = (AsmByte*)lpCodeAddress + m_DataSegmentOffset;
	for (int xj = 0; xj < m_PrologSegment.GetCount(); xj++)
	{
		pWriteAddress = m_PrologSegment[xj].Write(pWriteAddress, 0);
	}
	for (int xj = 0; xj < m_CodeSegment.GetCount(); xj++)
	{
		pWriteAddress = m_CodeSegment[xj].Write(pWriteAddress, pDataSegmentAddress);
	}
	if (!m_DataSegment.Empty())
	{
		pWriteAddress = pDataSegmentAddress;
		for (int xj = 0; xj < m_DataSegment.GetCount(); xj++)
		{
			pWriteAddress = m_DataSegment[xj].Write(pWriteAddress);
		}
	}
#if defined(_WIN64)
	memcpy(pStartAddress + m_UnwindInfoOffset, &m_UnwindData[0], m_UnwindData.GetCount() * sizeof(unsigned short));
#endif

	if (!FlushInstructionCache(GetCurrentProcess(), lpCodeAddress, CodeSize()))
	{
		SaveWin32Error("FlushInstructionCache", GetLastError());
		throw E_APIERROR;
	}
}

void RuntimeAssembler::RegisterUnwindInfo(void* lpCodeAddress)
{
#if defined(_WIN64)
	VFP2CTls& tls = VFP2CTls::Tls();
	RUNTIME_FUNCTION rf;
	DWORD64 pStartAddress = (DWORD64)lpCodeAddress;
	rf.BeginAddress = pStartAddress - tls.JitBaseAddress;
	rf.EndAddress = pStartAddress + m_DataSegmentOffset - tls.JitBaseAddress;
	rf.UnwindData = pStartAddress + m_UnwindInfoOffset - tls.JitBaseAddress;
	tls.RuntimeFunctions.Add(rf);
#endif
}

void RuntimeAssembler::UnregisterUnwindInfo(void* lpCodeAddress)
{
#if defined(_WIN64)
	VFP2CTls& tls = VFP2CTls::Tls();
	DWORD VirtualAddress = (DWORD64)lpCodeAddress - tls.JitBaseAddress;
	for (int xj = 0; xj < tls.RuntimeFunctions.GetCount(); xj++)
	{
		if (tls.RuntimeFunctions[xj].BeginAddress == VirtualAddress)
		{
			tls.RuntimeFunctions.Remove(xj);
			break;
		}
	}
#endif
}

int RuntimeAssembler::CodeSize()
{
	assert(m_Finished);
	if (m_CachedCodeSize != 0)
		return m_CachedCodeSize;
	int nCodeSize = 0;
	for (int xj = 0; xj < m_PrologSegment.GetCount(); xj++)
	{
		nCodeSize += m_PrologSegment[xj].Size();
	}
	m_CachedPrologSize = nCodeSize;
	for (int xj = 0; xj < m_CodeSegment.GetCount(); xj++)
	{
		nCodeSize += m_CodeSegment[xj].Size();
	}
	nCodeSize += (nCodeSize % 8); // align datasegment on 8 byte boundary
	m_DataSegmentOffset = nCodeSize;
	nCodeSize += m_DataSegment.GetCount() * 8;
#if defined(_WIN64)
	nCodeSize += (nCodeSize % 8); // align UNWIND_INFO on 8 byte boundary
	m_UnwindInfoOffset = nCodeSize;
	nCodeSize += m_UnwindData.GetCount() * 2;
#endif
	m_CachedCodeSize = nCodeSize;
	return nCodeSize;
}

void RuntimeAssembler::Reset(AsmCallingConvention nCallConv, bool bSpillParameters, bool bUseFramePointer)
{
	m_CodeSegment.Reserve(256);
	m_CodeSegment.SetIndex(-1);
	m_DataSegment.Reserve(32);
	m_DataSegment.SetIndex(-1);
	m_PrologSegment.Reserve(32);
	m_PrologSegment.SetIndex(-1);
	m_Jumps.Reserve(32);
	m_Jumps.SetIndex(-1);
	m_Labels.Reserve(32);
	m_Labels.SetIndex(-1);
	m_Variables.Reserve(32);
	m_Variables.SetIndex(-1);
	m_Parameters.Reserve(32);
	m_Parameters.SetIndex(-1);
	m_CodeStack.Reserve(32);
	m_CodeStack.SetIndex(-1);
	m_Code = &m_CodeSegment;
	m_UsedRegisters = 0;
	m_SavedRegisterSpace = 0;
	m_CachedCodeSize = 0;
	m_CachedPrologSize = 0;
	m_DataSegmentOffset = 0;
	m_ParameterCount = 0;
	m_SubFunctionParameterSize = 32;
	m_InPrologEpilog = false;
	m_SpillParameters = bSpillParameters;
	m_UseFramePointer = bUseFramePointer;
	m_FrameOffset = bUseFramePointer ? 8 : 0;
	m_CallConv = nCallConv;
	m_Finished = false;
}

void RuntimeAssembler::MarkRegister(AsmRegister nReg)
{
	if (m_InPrologEpilog)
		return;
	m_UsedRegisters |= RegisterMask(nReg);
	SpillRegister(nReg);
}

bool RuntimeAssembler::IsRegisterMarked(AsmRegister nReg)
{
	return (m_UsedRegisters & RegisterMask(nReg)) > 0;
}

int RuntimeAssembler::RegisterMask(AsmRegister nReg)
{
	nReg = (AsmRegister)Make_Reg_Code(nReg);
	return 1 << nReg;
}

int RuntimeAssembler::TypeBitCount(int nSize)
{
	return nSize * 8;
}

int RuntimeAssembler::TypeBitCount(AsmType nType)
{
	switch (nType)
	{
		case T_BOOL:
		case T_CHAR:
		case T_UCHAR:
			return 8;
		case T_SHORT:
		case T_USHORT:
			return 16;
		case T_INT:
		case T_UINT:
		case T_FLOAT:
#if !defined(_WIN64)
		case T_PTR:
#endif
			return 32;
		case T_INT64:
		case T_UINT64:
		case T_DOUBLE:
#if defined(_WIN64)
		case T_PTR:
#endif
			return 64;
		default: // case T_STRUCT:
			return 128;
	}
}

void RuntimeAssembler::Prolog()
{
	m_InPrologEpilog = true;
	m_Code = &m_PrologSegment;
	m_SavedRegisterSpace = 0;
	// BreakPoint();
#if !defined(_WIN64)
	// always save the current stack pointer in EBP 
	Push(EBP);
	Mov(EBP, ESP);
	m_SavedRegisterSpace += 4;
	// save all non volatile registers used in the function have to be preserved on the stack
	for (AsmRegister nReg = EAX; nReg <= EDI; ++nReg)
	{
		if (nReg == EBP || nReg == ESP)
			continue;
		if (IsRegisterMarked(nReg) && IsRegisterNonVolatile(nReg))
		{
			m_SavedRegisterSpace += 4;
			Push(nReg);
		}
	}
	Sub(ESP, StackSize());
#else
	if (m_SpillParameters)
	{
		int nCount = m_Parameters.GetCount();
		if (nCount >= 1)
			Mov(AsmVariable(RSP, 8), RCX);
		if (nCount >= 2)
			Mov(AsmVariable(RSP, 16), RDX);
		if (nCount >= 3)
			Mov(AsmVariable(RSP, 24), R8);
		if (nCount >= 4)
			Mov(AsmVariable(RSP, 32), R9);
	}
	else
	{
		if (IsRegisterMarked(RCX))
			Mov(AsmVariable(RSP, 8), RCX);
		if (IsRegisterMarked(RDX))
			Mov(AsmVariable(RSP, 16), RDX);
		if (IsRegisterMarked(R8))
			Mov(AsmVariable(RSP, 24), R8);
		if (IsRegisterMarked(R9))
			Mov(AsmVariable(RSP, 32), R9);
	}

	if (m_UseFramePointer)
	{
		m_SavedRegisterSpace += 8;
		Push(RBP);
		Mov(RBP, RSP);
	}

	// save all non volatile registers used in the function
	AsmRegister FrameRegister = m_UseFramePointer ? RBP : RSP;
	for (AsmRegister nReg = RAX; nReg <= R15; ++nReg)
	{
		if (nReg == RSP || nReg == FrameRegister)
			continue;
		if (IsRegisterMarked(nReg) && IsRegisterNonVolatile(nReg))
		{
			m_SavedRegisterSpace += 8;
			Push(nReg);
		}
	}
	Sub(RSP, StackSize());


#endif
	// switch back to normal codesegment
	m_Code = &m_CodeSegment;
	m_InPrologEpilog = false;
}

void RuntimeAssembler::Epilog()
{
	m_InPrologEpilog = true;
#if !defined(_WIN64)
	for (AsmRegister nReg = EDI; nReg >= EAX; --nReg)
	{
		if (nReg == EBP || nReg == ESP)
			continue;
		if (IsRegisterMarked(nReg) && IsRegisterNonVolatile(nReg))
		{
			Pop(nReg);
		}
	}
	Mov(ESP, EBP);
	Pop(EBP);
	Ret(m_CallConv == cdeclaration ? 0 : ParameterSize());
#else
	Add(RSP, StackSize());
	for (AsmRegister nReg = R15; nReg >= RAX; --nReg)
	{
		if (nReg == RSP || (m_UseFramePointer && nReg == RBP))
			continue;
		if (IsRegisterMarked(nReg) && IsRegisterNonVolatile(nReg))
		{
			// all non volatile registers used in the function have to be preserved on the stack
			Pop(nReg);
		}
	}
	if (m_UseFramePointer)
	{
		Pop(RBP);
	}
	Ret(0);
#endif
	m_InPrologEpilog = false;
}

int RuntimeAssembler::ParameterSize()
{
	int nCount = m_Parameters.GetCount();
#if !defined(_WIN64)
	if (nCount > 0)
	{
		int nSize = m_Parameters[nCount - 1].Offset + 4;
		// subtract 8 bytes for EBP basepointer and function return address
		nSize -= 8;
		return nSize;
	}
	else
		return 0;
#else
	if (nCount > 0)
	{
		return max(m_Parameters[nCount - 1].Offset, 32);
	}
	return 32;
#endif
}

int RuntimeAssembler::StackSize()
{
	int nCount = m_Variables.GetCount();
#if !defined(_WIN64)
	if (nCount > 0)
	{
		int nSize = -m_Variables[nCount - 1].Offset;
		nSize += (nSize % 4); // align on 4 byte boundary
		return nSize;
	}
	return 0;
#else
	int nSize = 0;
	if (nCount > 0)
		nSize = -m_Variables[nCount - 1].Offset + m_Variables[nCount - 1].Size;

	nSize += m_SubFunctionParameterSize;
	int nAlignCorrect = ((nSize + m_SavedRegisterSpace) % 16); // align on 16 byte boundary
	if (nAlignCorrect == 0)
		return nSize + 8;
	if (nAlignCorrect < 8)
		nSize += (8 - nAlignCorrect);
	else if (nAlignCorrect > 8)
		nSize += (16 - nAlignCorrect + 8);
	return nSize;
#endif
}

#if defined(_WIN64)
void RuntimeAssembler::CreateUnwindInfo()
{
	m_UnwindData.SetIndex(-1);
	CArray<unsigned short> pUnwindCodes;
	pUnwindCodes.Reserve(m_PrologSegment.GetCount());
	unsigned short nOpinfo;
	unsigned short nInstructionOffset = 0;
	for (int xj = 0; xj < m_PrologSegment.GetCount(); xj++)
	{
		AsmInstruction& ins = m_PrologSegment[xj];
		if (ins.Opcode == OP_MOV_R32_RM32 && ins.ModRM.Reg == RBP && ins.ModRM.Rm == RSP)
		{
			nInstructionOffset += ins.Size();
		}
		else if (ins.Opcode == OP_MOV_RM32_R32)
		{
			nInstructionOffset += ins.Size();
		}
		if (ins.Opcode.Code >= Make_Op(OP_PUSH_R32, RAX) && ins.Opcode.Code <= Make_Op(OP_PUSH_R32, RDI))
		{
			nInstructionOffset += ins.Size();
			nOpinfo = ins.Opcode.Code - OP_PUSH_R32;
			if (ins.Prefix.Code == (PREFIX_REX | PREFIX_REX_R))
				nOpinfo += 8;
			pUnwindCodes.Add(nInstructionOffset | (UWOP_PUSH_NONVOL << 8) | (nOpinfo << 12));
		}
		else if ((ins.Opcode.Code == OP_SUB_R32_IMM32 || ins.Opcode.Code == OP_SUB_R32_IMM8) && ins.ModRM.Rm == RSP)
		{
			nInstructionOffset += ins.Size();
			unsigned int nStackAdjust = ins.Immediate.Size() == 1 ? ins.Immediate.Value8 : ins.Immediate.Value32;
			if (nStackAdjust <= 128)
			{
				nOpinfo = nStackAdjust / 8 - 1;
				pUnwindCodes.Add(nInstructionOffset | (UWOP_ALLOC_SMALL << 8) | (nOpinfo << 12));
			}
			else if (nStackAdjust <= 512 * 1024 - 8)
			{
				nOpinfo = 0;
				pUnwindCodes.Add(nStackAdjust / 8);
				pUnwindCodes.Add(nInstructionOffset | (UWOP_ALLOC_LARGE << 8) | (nOpinfo << 12));
			}
			else
			{
				nOpinfo = 1;
				pUnwindCodes.Add(nStackAdjust >> 16);
				pUnwindCodes.Add(nStackAdjust);
				pUnwindCodes.Add(nInstructionOffset | (UWOP_ALLOC_LARGE << 8) | (nOpinfo << 12));
			}
		}
	}

	unsigned short nVersion = 1, nFlags = 0, nFrameRegister = 0, nFrameOffset = 0;
	unsigned short sizeOfProlog = (unsigned short)nInstructionOffset;
	unsigned short nCodeCount = (unsigned short)pUnwindCodes.GetCount();
	if (m_UseFramePointer)
	{
		nFrameRegister = Make_Reg_Code(RBP);
		nFrameOffset = 0; // TODO
	}
	m_UnwindData.Add(nVersion | (nFlags << 3) | (sizeOfProlog << 8));
	m_UnwindData.Add(nCodeCount | (nFrameRegister << 8) | (nFrameRegister << 12));
	for (int xj = pUnwindCodes.GetIndex(); xj >= 0; xj--)
	{
		m_UnwindData.Add(pUnwindCodes[xj]);
	}
	if (m_UnwindData.GetCount() % 2 == 1)
		m_UnwindData.Add(0);
}
#endif

void RuntimeAssembler::Finish()
{
	Prolog();
	Epilog();
	Patch();
#if defined(_WIN64)
	CreateUnwindInfo();
#endif
	m_Finished = true;
	CodeSize();
}

size_t RuntimeAssembler::Data(float nValue)
{
	AsmData data(nValue);
	return m_DataSegment.Add(data);
}

size_t RuntimeAssembler::Data(double nValue)
{
	AsmData data(nValue);
	return m_DataSegment.Add(data);
}

bool RuntimeAssembler::IsRegisterNonVolatile(AsmRegister nReg)
{
	switch (nReg)
	{
		case BL:
		case BH:
		case BP:
		case SI:
		case DI:
		case EBX:
		case EBP:
		case ESI:
		case EDI:
		case RBX:
		case RBP:
		case RSI:
		case RDI:
		case R12:
		case R13:
		case R14:
		case R15:
		case XMM6:
		case XMM7:
		case XMM8:
		case XMM9:
		case XMM10:
		case XMM11:
		case XMM12:
		case XMM13:
		case XMM14:
		case XMM15:
			return true;
	}
	return false;
}

void RuntimeAssembler::Label(const char* pLabel)
{
	AsmLabel label;
	label.Name = pLabel;
	label.InstructionIndex = m_Code->GetIndex();
	label.LabelIndex = 0;
	m_Labels.Add(label);
}

RuntimeAssembler& RuntimeAssembler::AddJump(AsmInstruction& ins, const char* pLabel)
{
	AsmLabel label;
	label.Name = pLabel;
	label.InstructionIndex = m_Code->Add(ins);
	label.LabelIndex = 0;
	m_Jumps.Add(label);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Jump(const char *pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_JMP_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

void RuntimeAssembler::Patch()
{
#if defined(_WIN64)
	// patch all stack relative instructions
	int nFrameOffset;
	if (m_UseFramePointer)
	{
		nFrameOffset = m_FrameOffset + StackSize();
	}
	AsmByte nFramePtrCode = Make_Reg_Code(StackFrameRegister());
	int nParameterAdjust = StackSize() + m_SavedRegisterSpace;
	int nLocalVariableAdjust = ParameterSize();

	/*
	for (int nInstruction = 0; nInstruction < m_PrologSegment.GetCount(); nInstruction++)
	{
		AsmInstruction& rIns = m_PrologSegment[nInstruction];
		if ((rIns.ModRM.Mod == MODRM_MOD_DISPLACEMENT8 || rIns.ModRM.Mod == MODRM_MOD_DISPLACEMENT32) && rIns.ModRM.Rm == nFramePtrCode)
		{
			int nDisplacement = rIns.Displacement.GetDisplacement() + nParameterAdjust;
			rIns.ModRM.Mod = rIns.Displacement.Displacement(nDisplacement, true);
		}
	}
	*/

	for (int nInstruction = 0; nInstruction < m_CodeSegment.GetCount(); nInstruction++)
	{
		AsmInstruction& rIns = m_CodeSegment[nInstruction];
		if ((rIns.ModRM.Mod == MODRM_MOD_DISPLACEMENT8 || rIns.ModRM.Mod == MODRM_MOD_DISPLACEMENT32) && rIns.ModRM.Rm == nFramePtrCode && !rIns.Displacement.Fixed)
		{
			if (rIns.Displacement.GetDisplacement() <= 0)
			{
				int nDisplacement = -rIns.Displacement.GetDisplacement() + nLocalVariableAdjust;
				rIns.ModRM.Mod = rIns.Displacement.Displacement(nDisplacement, true);
			}
			else
			{
				int nDisplacement = rIns.Displacement.GetDisplacement() + nParameterAdjust;
				rIns.ModRM.Mod = rIns.Displacement.Displacement(nDisplacement, true);
			}
		}
	}
#endif

	/* we resolve the jumps in a two pass process
	in the first pass we calculate for each jump instruction if a near jump (8bit) is sufficient so we can use a smaller encoding
	in the second pass we calculate the actual bytes relative to the RIP (instruction pointer) and patch each instruction with the calculated values
	*/
	int nInstructionDistance;
	// first pass
	for (int nJump = 0; nJump < m_Jumps.GetCount(); nJump++)
	{
		AsmLabel& pJump = m_Jumps[nJump];
		AsmLabel* pLabel = LabelRef(pJump.Name);
		assert(pLabel);
		// cache location so we don't have to resolve the label twice
		pJump.LabelIndex = pLabel->InstructionIndex;
		nInstructionDistance = 0;
		if (pJump.InstructionIndex < pLabel->InstructionIndex)
		{
			for (int xj = pJump.InstructionIndex + 1; xj <= pLabel->InstructionIndex; xj++)
			{
				nInstructionDistance += m_CodeSegment[xj].Size();
			}
		}
		else
		{
			for (int xj = pJump.InstructionIndex - 1; xj >= pLabel->InstructionIndex; xj--)
			{
				nInstructionDistance += m_CodeSegment[xj].Size();
			}
		}

		AsmInstruction& ins = m_CodeSegment[pJump.InstructionIndex];
		if (In8BitRange(nInstructionDistance))
		{
			// clear OP_2BYTE if necessary and convert long jump opcode into near jump opcode
			ins.Opcode = 0;
			if (ins.Opcode2 == OP_JMP_REL32)
			{
				ins.Opcode2 = OP_JMP_REL8;
			}
			else
			{
				ins.Opcode2 = ins.Opcode2.Code - 0x10;
			}
			ins.Immediate.Immediate8(0);
		}
	}

	// second pass
	for (int nJump = 0; nJump < m_Jumps.GetCount(); nJump++)
	{
		AsmLabel& pJump = m_Jumps[nJump];
		nInstructionDistance = 0;
		if (pJump.InstructionIndex < pJump.LabelIndex)
		{
			for (int xj = pJump.InstructionIndex + 1; xj <= pJump.LabelIndex; xj++)
			{
				nInstructionDistance += m_CodeSegment[xj].Size();
			}
		}
		else
		{
			for (int xj = pJump.InstructionIndex - 1; xj >= pJump.LabelIndex; xj--)
			{
				nInstructionDistance += m_CodeSegment[xj].Size();
			}
		}

		// patch the actual jump distance in bytes
		AsmInstruction& ins = m_CodeSegment[pJump.InstructionIndex];
		if (ins.Immediate.ByteLen == 4) // long jump 
			ins.Immediate.Immediate32(nInstructionDistance);
		else
			ins.Immediate.Immediate8(nInstructionDistance);
	}
}

AsmLabel* RuntimeAssembler::LabelRef(const char *pLabel)
{
	int nLabels;
	for (nLabels = 0; nLabels < m_Labels.GetCount(); nLabels++)
	{
		if (!strcmp(m_Labels[nLabels].Name,pLabel))
			return &m_Labels[nLabels];
	}
	return 0;
}

void* RuntimeAssembler::LabelAddress(const char *pLabel, void* pCodeAddress)
{
	LPAsmLabel lpLabel = LabelRef(pLabel);
	assert(lpLabel);
	int nLabelOffset = m_CachedPrologSize;
	for (int xj = 0; xj <= lpLabel->InstructionIndex; xj++)
	{
		nLabelOffset += m_CodeSegment[xj].Size();
	}
	return ((AsmByte*)pCodeAddress) + nLabelOffset;
}

RuntimeAssembler& RuntimeAssembler::BreakPoint()
{
	AsmInstruction ins;
	ins.Opcode = OP_BREAKPOINT;
	m_Code->Add(ins);
	return *this;
}

/* push a register onto the stack */
RuntimeAssembler& RuntimeAssembler::Push(AsmRegister nReg)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nReg);
#if !defined(_WIN64)
	if (nBitCount == 8)
		ins.Prefix = PREFIX_OP_SIZE;
		
	ins.Opcode = Make_Op(OP_PUSH_R32, nReg);
#else
	if (nBitCount == 16)
		ins.Prefix = PREFIX_OP_SIZE;
		
	ins.Prefix2 = Rex_Prefix_B(nReg);
	ins.Opcode = Make_Op(OP_PUSH_R32, nReg);
#endif
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Push(AsmImmediate nValue)
{
	AsmInstruction ins;
#if !defined(_WIN64)
	if (nValue.ByteLen == 2)
		ins.Prefix = PREFIX_ADR_SIZE;
	if (nValue.ByteLen >= 2)
		ins.Opcode = OP_PUSH_IMM32;
	else
		ins.Opcode = OP_PUSH_IMM8;
	ins.Immediate = nValue;
	m_Code->Add(ins);
#else
	if (nValue.ByteLen == 8)
	{
		if (m_InStackMode == false)
		{
			Mov(RAX, nValue);
			Push(RAX);
		}
		else
		{
			Push(RAX);
			Mov(RAX, nValue);
		}
	}
	else
	{
		AsmInstruction ins;
		if (nValue.ByteLen == 2)
			ins.Prefix = PREFIX_ADR_SIZE;
		if (nValue.ByteLen >= 2)
			ins.Opcode = OP_PUSH_IMM32;
		else
			ins.Opcode = OP_PUSH_IMM8;
		ins.Immediate = nValue;
		m_Code->Add(ins);
	}
#endif
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Push(const char* pValue)
{
#if !defined(_WIN64)
	AsmInstruction ins;
	ins.Opcode = OP_PUSH_IMM32;
	ins.Immediate.Immediate32((INT_PTR)pValue);
	m_Code->Add(ins);
#else
	AsmImmediate im(pValue);
	if (m_InStackMode == false)
	{
		Mov(RAX, im);
		Push(RAX);
	}
	else
	{
		Push(RAX);
		Mov(RAX, im);
	}
#endif
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Push(int nValue)
{
	AsmInstruction ins;
	if (In8BitRange(nValue))
	{
		ins.Opcode = OP_PUSH_IMM8;
		ins.Immediate.Immediate8(nValue);
	}
	else
	{
		ins.Opcode = OP_PUSH_IMM32;
		ins.Immediate.Immediate32(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Push(unsigned int nValue)
{
	AsmInstruction ins;
	if (InU8BitRange(nValue))
	{
		ins.Opcode = OP_PUSH_IMM8;
		ins.Immediate.Immediate8(nValue);
	}
	else
	{
		ins.Opcode = OP_PUSH_IMM32;
		ins.Immediate.Immediate32(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Push(AsmVariable& pVar)
{
	if (pVar.Location == Register)
		return Push(pVar.Register);

	AsmInstruction ins;
	int nBitCount = TypeBitCount(pVar.Size);
	if (nBitCount <= 16)
	{
		ins.Prefix = PREFIX_ADR_SIZE;
		ins.Opcode = OP_PUSH_RM32;
		ModRm_Op_Var(ins, MODRM_OP_PUSH, pVar);
		m_Code->Add(ins);
	}
	else if (nBitCount == 32)
	{
		ins.Opcode = OP_PUSH_RM32;
		ModRm_Op_Var(ins, MODRM_OP_PUSH, pVar);
		m_Code->Add(ins);
	}
	else if (nBitCount == 64 && pVar.Type != T_DOUBLE)
	{
#if !defined(_WIN64)
		// push upper 32 bit
		ins.Opcode = OP_PUSH_RM32;
		ModRm_Op_Var(ins, MODRM_OP_PUSH, AsmVariable::Var<unsigned int>(pVar, 4));
		m_Code->Add(ins);

		// push lower 32 bit
		ins.Opcode = OP_PUSH_RM32;
		ModRm_Op_Var(ins, MODRM_OP_PUSH, AsmVariable::Var<unsigned int>(pVar, 0));
		m_Code->Add(ins);
#else
		ins.Opcode = OP_PUSH_RM32;
		ModRm_Op_Var(ins, MODRM_OP_PUSH, pVar);
		m_Code->Add(ins);
#endif
	}
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Push(float nValue)
{
	AsmInstruction ins;
	ins.Opcode = OP_PUSH_IMM32;
	ins.Immediate = AsmImmediate(nValue);
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Push(double nValue)
{
	AsmInstruction ins;
	size_t nIndex = Data(nValue);
	ins.Opcode = OP_PUSH_IMM32;
	ins.ModRM.Mod = ins.Displacement.DataDisplacement(nIndex);
#if !defined(_WIN64)
	ins.ModRM.Rm = MODRM_RM_SIB;
#else
	ins.ModRM.Rm = MODRM_RM_RIP_RELATIVE;
#endif
	m_Code->Add(ins);
	return *this;
}

/* pop a value from the stack into a register */
RuntimeAssembler& RuntimeAssembler::Pop(AsmRegister nReg)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount >= 32)
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		ins.Opcode = Make_Op(OP_POP_R32, nReg);
	}
	else if (nBitCount <= 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_B(nReg);
		ins.Opcode = Make_Op(OP_POP_R32, nReg);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Sub(AsmRegister nReg, int nBytes)
{
	if (nBytes == 0)
		return *this;
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 64)
	{
		if (In8BitRange(nBytes))
		{
			ins.Prefix = Rex_Prefix_WB(nReg);
			ins.Opcode = OP_SUB_R32_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_SUB;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nBytes);
		}
		else
		{
			ins.Prefix = Rex_Prefix_WB(nReg);
			if (nReg == RAX)
				ins.Opcode = OP_SUB_EAX_IMM32;
			else
			{
				ins.Opcode = OP_SUB_R32_IMM32;
				ins.ModRM.Mod = MODRM_MOD_REGISTER;
				ins.ModRM.Reg = MODRM_OP_SUB;
				ins.ModRM.Rm = nReg;
			}
			ins.Immediate.Immediate32(nBytes);
		}
	}
	else if (nBitCount == 32)
	{
		if (In8BitRange(nBytes))
		{
			ins.Prefix = Rex_Prefix_B(nReg);
			ins.Opcode = OP_SUB_R32_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_SUB;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nBytes);
		}
		else
		{
			if (nReg == EAX)
				ins.Opcode = OP_SUB_EAX_IMM32;
			else
			{
				ins.Prefix = Rex_Prefix_B(nReg);
				ins.Opcode = OP_SUB_R32_IMM32;
				ins.ModRM.Mod = MODRM_MOD_REGISTER;
				ins.ModRM.Reg = MODRM_OP_SUB;
				ins.ModRM.Rm = nReg;
			}
			ins.Immediate.Immediate32(nBytes);
		}
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		if (nReg == AX)
			ins.Opcode = OP_SUB_EAX_IMM32;
		else
		{
			ins.Prefix2 = Rex_Prefix_B(nReg);
			ins.Opcode = OP_SUB_R32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_SUB;
			ins.ModRM.Rm = nReg;
		}
		ins.Immediate.Immediate16(nBytes);
	}
	else // nBitCount == 8
	{
		if (nReg == AL)
			ins.Opcode = OP_SUB_AL_IMM8;
		else
		{
			ins.Prefix = Rex_Prefix_B(nReg);
			ins.Opcode = OP_SUB_RM8_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_SUB;
			ins.ModRM.Rm = nReg;
		}
		ins.Immediate.Immediate8(nBytes);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Sub(AsmRegister nReg, unsigned int nBytes)
{
	assert(In32BitRange(nBytes));
	return Sub(nReg, static_cast<int>(nBytes));
}

RuntimeAssembler& RuntimeAssembler::Sub(AsmRegister nReg, __int64 nBytes)
{
	assert(RegisterBitCount(nReg) == 64);
	if (nBytes == 0)
		return *this;
	AsmInstruction ins;
	MarkRegister(nReg);
	if (In8BitRange(nBytes))
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = OP_SUB_R32_IMM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = MODRM_OP_SUB;
		ins.ModRM.Rm = nReg;
		ins.Immediate.Immediate8(nBytes);
	}
	else if (In32BitRange(nBytes))
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		if (nReg == RAX)
			ins.Opcode = OP_SUB_EAX_IMM32;
		else
		{
			ins.Opcode = OP_SUB_R32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_SUB;
			ins.ModRM.Rm = nReg;
		}
		ins.Immediate.Immediate32(nBytes);
	}
	else
	{
		ins.Prefix = Rex_Prefix_WR(nReg);
		if (nReg == RAX)
			ins.Opcode = OP_SUB_EAX_IMM32;
		else
		{
			ins.Opcode = OP_SUB_R32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_SUB;
			ins.ModRM.Rm = nReg;
		}
		ins.Immediate.Immediate64(nBytes);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Sub(AsmRegister nReg, unsigned __int64 nBytes)
{
	assert(In64BitRange(nBytes));
	return Sub(nReg, static_cast<__int64>(nBytes));
}

RuntimeAssembler& RuntimeAssembler::Sub(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	MarkRegister(nRegDest);
	int nBitCount = RegisterBitCount(nRegDest);
	if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_SUB_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegSource;
		ins.ModRM.Rm = nRegDest;
	}
	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_SUB_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegSource;
		ins.ModRM.Rm = nRegDest;
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_SUB_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegSource;
		ins.ModRM.Rm = nRegDest;
	}
	else
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_SUB_R8_RM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegSource;
		ins.ModRM.Rm = nRegDest;
	}
	m_Code->Add(ins);
	return *this;
}

/* Add ?! :) */
RuntimeAssembler& RuntimeAssembler::Add(AsmRegister nReg, int nBytes)
{
	if (nBytes == 0)
		return *this;
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 64)
	{
		if (In8BitRange(nBytes))
		{
			ins.Prefix = Rex_Prefix_WR(nReg);
			ins.Opcode = OP_ADD_RM32_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nBytes);
		}
		else
		{
			ins.Prefix = Rex_Prefix_WR(nReg);
			if (nReg == EAX)
				ins.Opcode = OP_ADD_EAX_IMM32;
			else
			{
				ins.Opcode = OP_ADD_RM32_IMM32;
				ins.ModRM.Mod = MODRM_MOD_REGISTER;
				ins.ModRM.Rm = nReg;
			}
			ins.Immediate.Immediate32(nBytes);
		}
	}
	else if (nBitCount == 32)
	{
		if (In8BitRange(nBytes))
		{
			ins.Prefix = Rex_Prefix_R(nReg);
			ins.Opcode = OP_ADD_RM32_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nBytes);
		}
		else
		{
			if (nReg == EAX)
				ins.Opcode = OP_ADD_EAX_IMM32;
			else
			{
				ins.Prefix = Rex_Prefix_R(nReg);
				ins.Opcode = OP_ADD_RM32_IMM32;
				ins.ModRM.Mod = MODRM_MOD_REGISTER;
				ins.ModRM.Rm = nReg;
			}
			ins.Immediate.Immediate32(nBytes);
		}
	}
	else if (nBitCount == 16)
	{
		if (nReg == AX)
			ins.Opcode = OP_ADD_EAX_IMM32;
		else
		{
			ins.Prefix = PREFIX_OP_SIZE;
			ins.Prefix2 = Rex_Prefix_R(nReg);
			ins.Opcode = OP_ADD_RM32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_ADD;
			ins.ModRM.Rm = nReg;
		}
		ins.Immediate.Immediate16(nBytes);
	}
	else 
	{
		if (nReg == AL)
			ins.Opcode = OP_ADD_AL_IMM8;
		else
		{
			ins.Prefix = Rex_Prefix_R(nReg);
			ins.Opcode = OP_ADD_RM8_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Rm = nReg;
		}
		ins.Immediate.Immediate8(nBytes);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Add(AsmRegister nReg, unsigned int nBytes)
{
	if (nBytes == 0)
		return *this;
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 64)
	{
		if (InU8BitRange(nBytes))
		{
			ins.Prefix = Rex_Prefix_WR(nReg);
			ins.Opcode = OP_ADD_RM32_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_ADD;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nBytes);
		}
		else
		{
			ins.Prefix = Rex_Prefix_WR(nReg);
			if (nReg == EAX)
				ins.Opcode = OP_ADD_EAX_IMM32;
			else
			{
				ins.Opcode = OP_ADD_RM32_IMM32;
				ins.ModRM.Mod = MODRM_MOD_REGISTER;
				ins.ModRM.Reg = MODRM_OP_ADD;
				ins.ModRM.Rm = nReg;
			}
			ins.Immediate.Immediate32(nBytes);
		}
	}
	else if (nBitCount == 32)
	{
		if (InU8BitRange(nBytes))
		{
			ins.Prefix = Rex_Prefix_R(nReg);
			ins.Opcode = OP_ADD_RM32_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_ADD;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nBytes);
		}
		else
		{
			if (nReg == EAX)
				ins.Opcode = OP_ADD_EAX_IMM32;
			else
			{
				ins.Prefix = Rex_Prefix_R(nReg);
				ins.Opcode = OP_ADD_RM32_IMM32;
				ins.ModRM.Mod = MODRM_MOD_REGISTER;
				ins.ModRM.Reg = MODRM_OP_ADD;
				ins.ModRM.Rm = nReg;
			}
			ins.Immediate.Immediate32(nBytes);
		}
	}
	else if (nBitCount == 16)
	{
		if (nReg == AX)
			ins.Opcode = OP_ADD_EAX_IMM32;
		else
		{
			ins.Prefix = PREFIX_OP_SIZE;
			ins.Prefix2 = Rex_Prefix_R(nReg);
			ins.Opcode = OP_ADD_RM32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_ADD;
			ins.ModRM.Rm = nReg;
		}
		ins.Immediate.Immediate16(nBytes);
	}
	else
	{
		if (nReg == AL)
			ins.Opcode = OP_ADD_AL_IMM8;
		else
		{
			ins.Prefix = Rex_Prefix_R(nReg);
			ins.Opcode = OP_ADD_RM8_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Rm = nReg;
		}
		ins.Immediate.Immediate8(nBytes);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Add(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	MarkRegister(nRegDest);
	int nBitCount = RegisterBitCount(nRegDest);
	if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_ADD_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_ADD_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_ADD_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else
	{
		ins.Prefix = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_ADD_R8_RM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Adcx(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	MarkRegister(nRegDest);
	int nBitCount = RegisterBitCount(nRegDest);
	if (nBitCount == 32)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_2BYTE;
		ins.Opcode2 = OP_ADCX_R64_RM64;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCount == 64)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_2BYTE;
		ins.Opcode2 = OP_ADCX_R64_RM64;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	m_Code->Add(ins);
	return *this;
}

/* Dec ?! :) decrement; someRegister--; */
RuntimeAssembler& RuntimeAssembler::Dec(AsmRegister nReg)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
#if !defined(_WIN64)
	if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		ins.Opcode = Make_Op(OP_DEC_R32, nReg);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_B(nReg);
		ins.Opcode = Make_Op(OP_DEC_R32,nReg);
	}
	else
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		ins.Opcode = OP_DEC_RM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = MODRM_OP_DEC;
		ins.ModRM.Rm = nReg;
	}
#else
	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WB(nReg);
		ins.Opcode = OP_DEC_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = MODRM_OP_DEC;
		ins.ModRM.Rm = nReg;
	}
	else if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		ins.Opcode = OP_DEC_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = MODRM_OP_DEC;
		ins.ModRM.Rm = nReg;
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_B(nReg);
		ins.Opcode = OP_DEC_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = MODRM_OP_DEC;
		ins.ModRM.Rm = nReg;
	}
	else
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		ins.Opcode = OP_DEC_RM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = MODRM_OP_DEC;
		ins.ModRM.Rm = nReg;
	}
#endif
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Inc(AsmRegister nReg)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
#if !defined(_WIN64)
	if (nBitCount == 32)
	{
		ins.Opcode = Make_Op(OP_INC_R32, nReg);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Opcode = Make_Op(OP_INC_R32,nReg);
	}
	else
	{
		ins.Opcode = OP_INC_RM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Rm = nReg;
	}
#else
	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WB(nReg);
		ins.Opcode = OP_INC_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Rm = nReg;
	}
	else if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		ins.Opcode = OP_INC_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Rm = nReg;
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_B(nReg);
		ins.Opcode = OP_INC_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Rm = nReg;
	}
	else
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		ins.Opcode = OP_INC_RM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Rm = nReg;
	}
#endif

	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::And(AsmRegister nReg, int nValue)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WB(nReg);
		if (nReg == RAX)
		{
			ins.Opcode = OP_AND_EAX_IMM32;
			ins.Immediate.Immediate32(nValue);
		}
		else
		{
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_AND;
			ins.ModRM.Rm = nReg;
			if (In8BitRange(nValue))
			{
				ins.Opcode = OP_AND_RM32_IMM8;
				ins.Immediate.Immediate8(nValue);
			}
			else
			{
				ins.Opcode = OP_AND_RM32_IMM32;
				ins.Immediate.Immediate32(nValue);
			}
		}
	}
	else if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		if (nReg == EAX)
		{
			ins.Opcode = OP_AND_EAX_IMM32;
			ins.Immediate.Immediate32(nValue);
		}
		else
		{
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_AND;
			ins.ModRM.Rm = nReg;
			if (In8BitRange(nValue))
			{
				ins.Opcode = OP_AND_RM32_IMM8;
				ins.Immediate.Immediate8(nValue);
			}
			else
			{
				ins.Opcode = OP_AND_RM32_IMM32;
				ins.Immediate.Immediate32(nValue);
			}
		}
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_B(nReg);
		if (nReg == AX)
		{
			ins.Opcode = OP_AND_EAX_IMM32;
			ins.Immediate.Immediate32(nValue);
		}
		else
		{
			ins.Opcode = OP_AND_RM32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_AND;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate32(nValue);
		}
	}
	else
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		if (nReg == AL)
		{
			ins.Opcode = OP_AND_AL_IMM8;
			ins.Immediate.Immediate8(nValue);
		}
		else
		{
			ins.Opcode = OP_AND_RM8_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_AND;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nValue);
		}
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::And(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	MarkRegister(nRegDest);
	int nBitCount = RegisterBitCount(nRegDest);
	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_AND_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_AND_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_AND_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else
	{
		ins.Prefix = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_AND_R8_RM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	m_Code->Add(ins);
	return *this;
}



RuntimeAssembler& RuntimeAssembler::Or(AsmRegister nReg, int nValue)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Immediate.Immediate32(nValue);
		if (nReg == EAX)
		{
			ins.Opcode = OP_OR_EAX_IMM32;
		}
		else
		{
			ins.Opcode = OP_OR_RM32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Rm = nReg;
		}
	}
	if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Immediate.Immediate32(nValue);
		if (nReg == EAX)
		{
			ins.Opcode = OP_OR_EAX_IMM32;
		}
		else
		{
			ins.Opcode = OP_OR_RM32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Rm = nReg;
		}
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_R(nReg);
		ins.Immediate.Immediate16(nValue);
		if (nReg == AX)
		{
			ins.Opcode = OP_OR_EAX_IMM32;
		}
		else
		{
			ins.Opcode = OP_OR_RM32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Rm = nReg;
		}
	}
	else
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Immediate.Immediate8(nValue);
		if (nReg == AL)
		{
			ins.Opcode = OP_OR_AL_IMM8;
		}
		else
		{
			ins.Opcode = OP_OR_RM8_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Rm = nReg;
		}
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Or(AsmRegister nReg, __int64 nValue)
{
	AsmInstruction ins;
	assert(RegisterBitCount(nReg) == 64);
	MarkRegister(nReg);
	ins.Prefix = Rex_Prefix_R(nReg);
	ins.Immediate.Immediate64(nValue);
	if (nReg == EAX)
	{
		ins.Opcode = OP_OR_EAX_IMM32;
	}
	else
	{
		ins.Opcode = OP_OR_RM32_IMM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Rm = nReg;
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Or(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	MarkRegister(nRegDest);
	int nBitCount = RegisterBitCount(nRegDest);
	if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_OR_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_OR_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_OR_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_OR_R8_RM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Xor(AsmRegister nReg, int nValue)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Immediate.Immediate32(nValue);
		if (nReg == EAX)
		{
			ins.Opcode = OP_XOR_EAX_IMM32;
		}
		else
		{
			ins.Opcode = OP_XOR_RM32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_XOR;
			ins.ModRM.Rm = nReg;
		}
	}
	else if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Immediate.Immediate32(nValue);
		if (nReg == EAX)
		{
			ins.Opcode = OP_XOR_EAX_IMM32;
		}
		else
		{
			ins.Opcode = OP_XOR_RM32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_XOR;
			ins.ModRM.Rm = nReg;
		}
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_R(nReg);
		ins.Immediate.Immediate16(nValue);
		if (nReg == AX)
		{
			ins.Opcode = OP_XOR_EAX_IMM32;
		}
		else
		{
			ins.Opcode = OP_XOR_RM32_IMM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_XOR;
			ins.ModRM.Rm = nReg;
		}
		
	}
	else
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Immediate.Immediate8(nValue);
		if (nReg == AL)
		{
			ins.Opcode = OP_XOR_AL_IMM8;
		}
		else
		{
			ins.Opcode = OP_XOR_RM8_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_XOR;
			ins.ModRM.Rm = nReg;
		}
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Xor(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	MarkRegister(nRegDest);
	int nBitCount = RegisterBitCount(nRegDest);
	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_XOR_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_XOR_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_XOR_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else
	{
		ins.Prefix = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_XOR_R8_RM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	m_Code->Add(ins);
	return *this;
}


RuntimeAssembler& RuntimeAssembler::Cdq()
{
	AsmInstruction ins;
	ins.Opcode = OP_CDQ;
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::CvtSd2Si(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nRegDest);
	ins.Prefix = PREFIX_SSE_F2;
	ins.Prefix2 = Rex_Prefix_WR(nRegDest);
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_CVTSD2SI;
	ins.ModRM.Mod = MODRM_MOD_REGISTER;
	ins.ModRM.Reg = nRegDest;
	ins.ModRM.Rm = nRegSource;
	m_Code->Add(ins);
	return *this;

}

RuntimeAssembler& RuntimeAssembler::CvtSd2Si(AsmRegister nRegDest, AsmVariable pVar)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nRegDest);
	ins.Prefix = PREFIX_SSE_F2;
	ins.Prefix2 = Rex_Prefix_WR(nRegDest);
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_CVTSD2SI;
	ModRm_Reg_Var(ins, nRegDest, pVar);
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::CvttSd2Si(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nRegDest);
	ins.Prefix = PREFIX_SSE_F2;
	ins.Prefix2 = Rex_Prefix_WR(nRegDest);
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_CVTTSD2SI;
	ins.ModRM.Mod = MODRM_MOD_REGISTER;
	ins.ModRM.Reg = nRegDest;
	ins.ModRM.Rm = nRegSource;
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::CvttSd2Si(AsmRegister nRegDest, AsmVariable pVar)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nRegDest);
	ins.Prefix = PREFIX_SSE_F2;
	ins.Prefix2 = Rex_Prefix_WR(nRegDest);
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_CVTTSD2SI;
	ModRm_Reg_Var(ins, nRegDest, pVar);
	m_Code->Add(ins);
	return *this;
}


RuntimeAssembler& RuntimeAssembler::CvtSi2Sd(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nRegDest);
	ins.Prefix = PREFIX_SSE_F3;
	ins.Prefix2 = Rex_Prefix_WR(nRegDest);
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_CVTSI2SD;
	ins.ModRM.Mod = MODRM_MOD_REGISTER;
	ins.ModRM.Reg = nRegDest;
	ins.ModRM.Rm = nRegSource;
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::CvtSi2Sd(AsmRegister nRegDest, AsmVariable pVar)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nRegDest);
	ins.Prefix = PREFIX_SSE_F3;
	ins.Prefix2 = Rex_Prefix_WR(nRegDest);
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_CVTSI2SD;
	ModRm_Reg_Var(ins, nRegDest, pVar);
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Shift_Ex(AsmRegister nReg, int nBits, int nOpcode)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WB(nReg);
		if (nBits == 1)
		{
			ins.Opcode = OP_SHIFT_RM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = nOpcode;
			ins.ModRM.Rm = nReg;
		}
		else
		{
			ins.Opcode = OP_SHIFT_RM32_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = nOpcode;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nBits);
		}
	}
	else if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		if (nBits == 1)
		{
			ins.Opcode = OP_SHIFT_RM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = nOpcode;
			ins.ModRM.Rm = nReg;
		}
		else
		{
			ins.Opcode = OP_SHIFT_RM32_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = nOpcode;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nBits);
		}
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_B(nReg);
		if (nBits == 1)
		{
			ins.Opcode = OP_SHIFT_RM32;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = nOpcode;
			ins.ModRM.Rm = nReg;
		}
		else
		{
			ins.Opcode = OP_SHIFT_RM32_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = nOpcode;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nBits);
		}
	}
	else
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		if (nBits == 1)
		{
			ins.Opcode = OP_SHIFT_RM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = nOpcode;
			ins.ModRM.Rm = nReg;
		}
		else
		{
			ins.Opcode = OP_SHIFT_RM8_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = nOpcode;
			ins.ModRM.Rm = nReg;
		}
		ins.Immediate.Immediate8(nBits);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Sal(AsmRegister nReg, int nBits)
{
	return Shift_Ex(nReg,nBits,MODRM_OP_SAL);
}

RuntimeAssembler& RuntimeAssembler::Sar(AsmRegister nReg, int nBits)
{
	return Shift_Ex(nReg,nBits,MODRM_OP_SAR);
}

RuntimeAssembler& RuntimeAssembler::Shl(AsmRegister nReg, int nBits)
{
	return Shift_Ex(nReg,nBits,MODRM_OP_SHL);
}

RuntimeAssembler& RuntimeAssembler::Shr(AsmRegister nReg, int nBits)
{
	return Shift_Ex(nReg,nBits,MODRM_OP_SHR);
}

RuntimeAssembler& RuntimeAssembler::Lea(AsmRegister nRegDest, AsmVariable& pSource)
{
	AsmInstruction ins;
	MarkRegister(nRegDest);
	int nBitCount = RegisterBitCount(nRegDest);
	if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_R(nRegDest);
		ins.Opcode = OP_LEA_R32_M;
		ModRm_Reg_Var(ins, nRegDest, pSource);
	}
	else if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WR(nRegDest);
		ins.Opcode = OP_LEA_R32_M;
		ModRm_Reg_Var(ins, nRegDest, pSource);
	}
	m_Code->Add(ins);
	return *this;
}

/* Move declared Parameter/Local variable from stack into a register */
RuntimeAssembler& RuntimeAssembler::Mov(AsmRegister nReg, AsmVariable& pVar)
{
	if (pVar.Location == Register)
		return Mov(nReg, pVar.Register);

	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	int nTypeBitCount = TypeBitCount(pVar.Type);
	if (nBitCount == 32)
	{
		if (nTypeBitCount <= 16)
		{
			if (pVar.Signed)
				return MovSX(nReg, pVar);
			else
				return MovZX(nReg, pVar);
		}
		else
		{
#if defined(_WIN64)
			if (nReg != RSP)
			{
				ins.Prefix = Rex_Prefix_R(nReg);
			}
#endif
			ins.Opcode = OP_MOV_R32_RM32;
			ModRm_Reg_Var(ins, nReg, pVar);
		}
	}
	else if (nBitCount == 64)
	{
		if (nTypeBitCount <= 32)
		{
			if (pVar.Signed) // signed?
				return MovSX(nReg, pVar);
			else
				return MovZX(nReg, pVar);
		}
		else if (nTypeBitCount >= 64)
		{
			ins.Prefix = Rex_Prefix_WR(nReg);
			ins.Opcode = OP_MOV_R32_RM32;
			ModRm_Reg_Var(ins, nReg, pVar);
		}
	}
	else if (nBitCount == 128)
	{
		if (nTypeBitCount == 64)
			ins.Prefix2 = Rex_Prefix_R(nReg);

		ins.Prefix = PREFIX_OP_SIZE;
		ins.Opcode = OP_2BYTE;
		ins.Opcode2 = OP_MOV_R128_RM64;
		ModRm_Reg_Var(ins, nReg, pVar);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
#if defined(_WIN64)
		ins.Prefix2 = Rex_Prefix_R(nReg);
#endif
		ins.Opcode = OP_MOV_R32_RM32;
		ModRm_Reg_Var(ins, nReg, pVar);
	}
	else
	{
#if defined(_WIN64)
		ins.Prefix = Rex_Prefix_R(nReg);
#endif
		ins.Opcode = OP_MOV_R8_RM8;
		ModRm_Reg_Var(ins, nReg, pVar);
	}
	m_Code->Add(ins);
	return *this;
}

/* Move direct value into a register */
RuntimeAssembler& RuntimeAssembler::Mov(AsmRegister nReg, char nValue)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate32(nValue);
	}
	else if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WR(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate64(nValue);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate16(nValue);
	}
	else
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R8_IMM8, nReg);
		ins.Immediate.Immediate8(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmVariable& pVar, char pValue)
{
	if (pVar.Location == Register)
		return Mov(pVar.Register, pValue);

	AsmInstruction ins;
	int nBitCount = TypeBitCount(pVar.Type);
	if (nBitCount == 32)
	{
#if defined(_WIN64)
		ins.Prefix = PREFIX_OP_SIZE;
#endif
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate32(pValue);
	}
	else if (nBitCount == 64)
	{
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate32(pValue);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate16(pValue);
	}
	else
	{
		ins.Opcode = OP_MOV_RM8_IMM8;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate8(pValue);
	}
	m_Code->Add(ins);
	return *this;
}

/* Move direct value into a register */
RuntimeAssembler& RuntimeAssembler::Mov(AsmRegister nReg, int nValue)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate32(nValue);
	}
	else if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WR(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate64(nValue);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate16(nValue);
	}
	else
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R8_IMM8, nReg);
		ins.Immediate.Immediate8(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmVariable& pVar, int nValue)
{
	if (pVar.Location == Register)
		return Mov(pVar.Register, nValue);

	AsmInstruction ins;
	int nBitCount = TypeBitCount(pVar.Type);
	if (nBitCount == 32)
	{
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate32(nValue);
	}
	else if (nBitCount == 64)
	{
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate32(nValue);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate16(16);
	}
	else
	{
		ins.Opcode = OP_MOV_RM8_IMM8;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate8(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmRegister nReg, unsigned int nValue)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 32)
	{
#if defined(_WIN64)
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_R(nReg);
#endif
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate32(nValue);
	}
	else if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WR(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate64(nValue);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate16(nValue);
	}
	else
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R8_IMM8, nReg);
		ins.Immediate.Immediate8(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmVariable& pVar, unsigned int nValue)
{
	if (pVar.Location == Register)
		return Mov(pVar.Register, nValue);

	AsmInstruction ins;
	int nBitCount = TypeBitCount(pVar.Type);
	if (nBitCount == 32)
	{
#if defined(_WIN64)
		ins.Prefix = PREFIX_OP_SIZE;
#endif
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate32(nValue);
	}
	else if (nBitCount == 64)
	{
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate(nValue);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate16(nValue);
	}
	else
	{
		ins.Opcode = OP_MOV_RM8_IMM8;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate8(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmRegister nReg, unsigned __int64 nValue)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 32)
	{
		ins.Prefix2 = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate32(nValue);
	}
	else if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WR(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate64(nValue);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate.Immediate16(nValue);
	}
	else
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R8_IMM8, nReg);
		ins.Immediate.Immediate8(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmVariable& pVar, unsigned __int64 nValue)
{
	if (pVar.Location == Register)
		return Mov(pVar.Register, nValue);

	AsmInstruction ins;
	int nBitCount = TypeBitCount(pVar.Type);
	assert(In32BitRange(nValue));
	if (nBitCount == 32)
	{
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate32(nValue);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate16(nValue);
	}
	else
	{
		ins.Opcode = OP_MOV_RM8_IMM8;
		ModRm_Var(ins, pVar);
		ins.Immediate.Immediate8(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmRegister nReg, float nValue)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount >= 32)
	{
		if (nBitCount == 64)
			ins.Prefix = Rex_Prefix_WR(nReg);
		else
			ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate = AsmImmediate(nValue);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate = AsmImmediate(nValue);
	}
	else
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R8_IMM8, nReg);
		ins.Immediate = AsmImmediate(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmVariable& pVar, float nValue)
{
	if (pVar.Location == Register)
		return Mov(pVar.Register, nValue);

	AsmInstruction ins;
	int nBitCount = TypeBitCount(pVar.Type);
	if (nBitCount >= 32)
	{
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate = AsmImmediate(nValue);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate = AsmImmediate(nValue);
	}
	else
	{
		ins.Opcode = OP_MOV_RM8_IMM8;
		ModRm_Var(ins, pVar);
		ins.Immediate = AsmImmediate(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmRegister nReg, AsmImmediate nValue)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 64)
	{
		if (nValue.Size() == 8)
			ins.Prefix = Rex_Prefix_WR(nReg);
		else
			ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate = nValue;
	}
	else if (nBitCount == 32)
	{
		if (nValue.Size() == 2)
			ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate = nValue;
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R64_IMM64, nReg);
		ins.Immediate = nValue;
	}
	else if (nBitCount == 8)
	{
		ins.Prefix = Rex_Prefix_R(nReg);
		ins.Opcode = Make_Op(OP_MOV_R8_IMM8, nReg);
		ins.Immediate = nValue;
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmVariable& pVar, AsmImmediate nValue)
{
	if (pVar.Location == Register)
		return Mov(pVar.Register, nValue);

	AsmInstruction ins;
	int nBitCount = TypeBitCount(pVar.Type);
	if (nBitCount == 64)
	{
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate = nValue;
	}
	else if (nBitCount == 32)
	{
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate = nValue;
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Opcode = OP_MOV_RM64_IMM32;
		ModRm_Var(ins, pVar);
		ins.Immediate = nValue;
	}
	else if (nBitCount == 8)
	{
		ins.Opcode = OP_MOV_RM8_IMM8;
		ModRm_Var(ins, pVar);
		ins.Immediate = nValue;
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmRegister nReg, const char* pValue)
{
	return Mov(nReg, AsmImmediate(pValue));
}

RuntimeAssembler& RuntimeAssembler::Mov(AsmVariable& pVar, const char* pValue)
{
	return Mov(pVar, AsmImmediate(pValue));
}

/* move register into parameter/local variable */
RuntimeAssembler& RuntimeAssembler::Mov(AsmVariable& pVar, AsmRegister nRegSource)
{
	AsmInstruction ins;
	int nTypeBitCount = TypeBitCount(pVar.Type);
	int nBitCount = RegisterBitCount(nRegSource);
	assert(nTypeBitCount >= nBitCount);
	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WR(nRegSource);
		ins.Opcode = OP_MOV_RM32_R32;
		ModRm_Var_Reg(ins, pVar, nRegSource);
	}
	else if (nBitCount == 32)
	{
		ins.Prefix2 = Rex_Prefix_R(nRegSource);
		ins.Opcode = OP_MOV_RM32_R32;
		ModRm_Var_Reg(ins, pVar, nRegSource);
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_R(nRegSource);
		ins.Opcode = OP_MOV_RM32_R32;
		ModRm_Var_Reg(ins, pVar, nRegSource);
	}
	else if (nTypeBitCount == 8)
	{
		ins.Prefix = Rex_Prefix_R(nRegSource);
		ins.Opcode = OP_MOV_RM8_R8;
		ModRm_Var_Reg(ins, pVar, nRegSource);
	}
	m_Code->Add(ins);
	return *this;
}

/* Move value from one register into another */
RuntimeAssembler& RuntimeAssembler::Mov(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	if (Make_Reg_Code(nRegDest) == Make_Reg_Code(nRegSource) && Rex_Prefix_R(nRegDest) == Rex_Prefix_R(nRegSource))
		return *this;
	int nBitCountDest = RegisterBitCount(nRegDest);
	int nBitCountSource = RegisterBitCount(nRegSource);
	if (nBitCountDest > nBitCountSource)
		return MovZX(nRegDest, nRegSource);
	MarkRegister(nRegDest);
	if (nBitCountDest == 64)
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_MOV_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCountDest == 32)
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_MOV_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCountDest == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_MOV_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_MOV_R8_RM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	m_Code->Add(ins);
	return *this;
}

/* Move on variable into another, using a temporary register if needed */
RuntimeAssembler& RuntimeAssembler::Mov(AsmVariable& pVarDest, AsmVariable& pVarSource)
{
	if (pVarDest.Location == Register && pVarSource.Location == Register)
		return Mov(pVarDest.Register, pVarSource.Register);
	if (pVarDest.Location == Memory && pVarSource.Location == Register)
		return Mov(pVarDest, pVarSource.Register);
	if (pVarDest.Location == Register && pVarSource.Location == Memory)
		return Mov(pVarDest.Register, pVarSource);
	if (pVarDest.Location == Memory && pVarSource.Location == Memory)
	{
		AsmRegister nReg = GetTemporayRegister(pVarSource.Size);
		if (m_InStackMode == false)
		{
			Mov(nReg, pVarSource);
			Mov(pVarDest, nReg);
		}
		else
		{
			Mov(pVarDest, nReg);
			Mov(nReg, pVarSource);
		}
	}
	return *this;
}

RuntimeAssembler& RuntimeAssembler::MovZX(AsmRegister nReg, AsmVariable pVar)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = TypeBitCount(pVar.Type);
	ins.Prefix = Rex_Prefix_R(nReg);
	ins.Opcode = OP_2BYTE;
	if (nBitCount == 16)
		ins.Opcode2 = OP_MOVZX_R32_RM16;
	else
		ins.Opcode2 = OP_MOVZX_R32_RM8;

	ModRm_Reg_Var(ins, nReg, pVar);
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::MovZX(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	MarkRegister(nRegDest);
	int nBitCount = RegisterBitCount(nRegSource);
	ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
	ins.Opcode = OP_2BYTE;
	if (nBitCount == 16)
		ins.Opcode2 = OP_MOVZX_R32_RM16;
	else
		ins.Opcode2 = OP_MOVZX_R32_RM8;

	ins.ModRM.Mod = MODRM_MOD_REGISTER;
	ins.ModRM.Reg = nRegDest;
	ins.ModRM.Rm = nRegSource;
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::MovSX(AsmRegister nReg, AsmVariable pVar)
{
	AsmInstruction ins;
	MarkRegister(nReg);
	int nBitCount = RegisterBitCount(nReg);
	int nTypeBitCount = TypeBitCount(pVar.Type);

	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_B(nReg);
		if (nTypeBitCount == 32)
		{
			ins.Opcode = OP_MOVSX_R64_RM32;
		}
		else if (nTypeBitCount == 16)
		{
			ins.Opcode = OP_2BYTE;
			ins.Opcode2 = OP_MOVSX_R32_RM16;
		}
		else
		{
			ins.Opcode = OP_2BYTE;
			ins.Opcode2 = OP_MOVSX_R32_RM8;
		}
	}
	else if (nBitCount == 32)
	{
		if (nTypeBitCount == 32)
		{
			ins.Opcode = OP_MOVSX_R64_RM32;
		}
		else if (nTypeBitCount == 16)
		{
			ins.Opcode = OP_2BYTE;
			ins.Opcode2 = OP_MOVSX_R32_RM16;
		}
		else
		{
			ins.Opcode = OP_2BYTE;
			ins.Opcode2 = OP_MOVSX_R32_RM8;
		}
	}
	else if (nBitCount == 16)
	{
		if (nTypeBitCount == 16)
		{
			ins.Opcode = OP_2BYTE;
			ins.Opcode2 = OP_MOVSX_R32_RM16;
		}
		else
		{
			ins.Opcode = OP_2BYTE;
			ins.Opcode2 = OP_MOVSX_R32_RM8;
		}
	}
	ModRm_Reg_Var(ins, nReg, pVar);
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Cmp(AsmRegister nReg, int nValue)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nReg);
	if (nBitCount == 64)
	{
		if (In8BitRange(nValue))
		{
			ins.Prefix = Rex_Prefix_WB(nReg);
			ins.Opcode = OP_CMP_RM32_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_CMP;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nValue);
		}
		else
		{
			if (nReg != EAX)
			{
				ins.Prefix = Rex_Prefix_WB(nReg);
				ins.Opcode = OP_CMP_RM32_IMM32;
				ins.ModRM.Mod = MODRM_MOD_REGISTER;
				ins.ModRM.Reg = MODRM_OP_CMP;
				ins.ModRM.Rm = nReg;
			}
			else
			{
				ins.Opcode = OP_CMP_EAX_IMM32;
			}
			ins.Immediate.Immediate32(nValue);
		}
	}
	else if (nBitCount == 32)
	{
		if (In8BitRange(nValue))
		{
			ins.Prefix = Rex_Prefix_B(nReg);
			ins.Opcode = OP_CMP_RM32_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_CMP;
			ins.ModRM.Rm = nReg;
			ins.Immediate.Immediate8(nValue);
		}
		else
		{
			if (nReg != EAX)
			{
				ins.Prefix = Rex_Prefix_B(nReg);
				ins.Opcode = OP_CMP_RM32_IMM32;
				ins.ModRM.Mod = MODRM_MOD_REGISTER;
				ins.ModRM.Reg = MODRM_OP_CMP;
				ins.ModRM.Rm = nReg;
			}
			else
			{
				ins.Opcode = OP_CMP_EAX_IMM32;
			}
			ins.Immediate.Immediate32(nValue);
		}
	}
	else if (nBitCount == 16)
	{
		assert(In16BitRange(nValue) || InU16BitRange(nValue));
		if (In8BitRange(nValue))
		{
			ins.Prefix = PREFIX_OP_SIZE;
			if (nReg != AX)
			{
				ins.Prefix2 = Rex_Prefix_B(nReg);
				ins.Opcode = OP_CMP_RM32_IMM8;
				ins.ModRM.Mod = MODRM_MOD_REGISTER;
				ins.ModRM.Reg = MODRM_OP_CMP;
				ins.ModRM.Rm = nReg;
				ins.Immediate.Immediate8(nValue);
			}
			else
			{
				ins.Opcode = OP_CMP_EAX_IMM32;
				ins.Immediate.Immediate16(nValue);
			}
		}
		else
		{
			ins.Prefix = PREFIX_OP_SIZE;
			if (nReg != AX)
			{
				ins.Prefix2 = Rex_Prefix_B(nReg);
				ins.Opcode = OP_CMP_RM32_IMM32;
				ins.ModRM.Mod = MODRM_MOD_REGISTER;
				ins.ModRM.Reg = MODRM_OP_CMP;
				ins.ModRM.Rm = nReg;
			}
			else
			{
				ins.Opcode = OP_CMP_EAX_IMM32;
			}
			ins.Immediate.Immediate16(nValue);
		}
	}
	else if (nBitCount == 8)
	{
		assert(In8BitRange(nValue));
		if (nReg != AL)
		{
			ins.Prefix = Rex_Prefix_B(nReg);
			ins.Opcode = OP_CMP_RM8_IMM8;
			ins.ModRM.Mod = MODRM_MOD_REGISTER;
			ins.ModRM.Reg = MODRM_OP_CMP;
			ins.ModRM.Rm = nReg;
		}
		else
			ins.Opcode = OP_CMP_AL_IMM8;

		ins.Immediate.Immediate8(nValue);
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Cmp(AsmRegister nRegDest, AsmRegister nRegSource)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nRegDest);
	if (nBitCount == 64)
	{
		ins.Prefix = Rex_Prefix_WRB(nRegDest, nRegSource);
		ins.Opcode = OP_CMP_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	if (nBitCount == 32)
	{
		ins.Prefix = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_CMP_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCount == 16)
	{
		ins.Prefix = PREFIX_OP_SIZE;
		ins.Prefix2 = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_CMP_R32_RM32;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	else if (nBitCount == 8)
	{
		ins.Prefix = Rex_Prefix_RB(nRegDest, nRegSource);
		ins.Opcode = OP_CMP_R8_RM8;
		ins.ModRM.Mod = MODRM_MOD_REGISTER;
		ins.ModRM.Reg = nRegDest;
		ins.ModRM.Rm = nRegSource;
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Jo(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JO_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jno(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JNO_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jc(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JC_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jb(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JB_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jae(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JAE_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Je(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JE_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jz(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JZ_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jne(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JNE_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jbe(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JBE_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Ja(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JA_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Js(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JS_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jns(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JNS_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jnp(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JNP_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jl(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JL_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jge(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JGE_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jle(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JLE_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jg(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JG_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

RuntimeAssembler& RuntimeAssembler::Jmp(const char* pLabel)
{
	AsmInstruction ins;
	ins.Opcode = OP_2BYTE;
	ins.Opcode2 = OP_JMP_REL32;
	ins.Immediate.Immediate32(0);
	return AddJump(ins, pLabel);
}

/* jump to location in a register */
RuntimeAssembler& RuntimeAssembler::Jmp(AsmRegister nReg)
{
	AsmInstruction ins;
	ins.Opcode = OP_JMP_RM32;
	ins.ModRM.Mod = MODRM_MOD_REGISTER;
	ins.ModRM.Reg = MODRM_OP_JMP;
	ins.ModRM.Rm = nReg;
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Jmp(AsmRegister nReg, AsmImmediate pLocation)
{
	Mov(nReg,pLocation);
	return Jmp(nReg);
}

RuntimeAssembler& RuntimeAssembler::CallConv(AsmCallingConvention nConv)
{
	m_SubCallConv = nConv;
	return *this;
}

void RuntimeAssembler::SpillRegister(AsmRegister nReg)
{
#if defined(_WIN64)
	int nOffset;
	switch (nReg)
	{
		case RDX:
		case EDX:
		case DX:
		case DL:
		case DH:
			nOffset = 0;
			break;
		case RCX:
		case ECX:
		case CX:
		case CL:
		case CH:
			nOffset = 1;
			break;
		case R8:
		case R8D:
		case R8W:
		case R8B:
			nOffset = 2;
			break;
		case R9:
		case R9D:
		case R9W:
		case R9B:
			nOffset = 3;
			break;
		default:
			return;
	}
	if (m_Parameters.GetCount() > nOffset && m_Parameters[nOffset].Location == AsmLocation::Register)
	{
		m_Parameters[nOffset].Location = AsmLocation::Memory;
		m_Parameters[nOffset].Offset = (nOffset + 1) * 8;
		m_Parameters[nOffset].Register = RSP;
	}
#endif
}

void RuntimeAssembler::OnCall(int nParmCount)
{
	// if we make a function call, all volatile registers need to be spilled
	SpillRegister(RCX);
	SpillRegister(RDX);
	SpillRegister(R8);
	SpillRegister(R9);
	m_UsedRegisters |= (RegisterMask(RCX) | RegisterMask(RDX) | RegisterMask(R8) | RegisterMask(R9));
	m_SubFunctionParameterSize = max(m_SubFunctionParameterSize, nParmCount * 8);
}

RuntimeAssembler& RuntimeAssembler::StackMode(bool bEnable, bool bDiscard)
{
	if (bEnable)
	{
		m_ParameterCount = 0;
		m_CodeStack.SetIndex(-1);
		m_Code = &m_CodeStack;
	}
	else
	{
		m_Code = &m_CodeSegment;
		if (bDiscard == false)
		{
			for (int xj = m_CodeStack.GetIndex(); xj >= 0; xj--)
			{
				m_Code->Add(m_CodeStack[xj]);
			}
		}
	}
	m_InStackMode = bEnable;
	return *this;
}

AsmRegister RuntimeAssembler::Call(AsmRegister nReg, AsmFuncPtr pFunction)
{
	AsmImmediate rImm;
#if !defined(_WIN64)
	rImm.Immediate32((INT_PTR)pFunction);
#else
	rImm.Immediate64((INT_PTR)pFunction);
#endif
	Mov(nReg, rImm);
	return Call(nReg);
}

AsmRegister RuntimeAssembler::GetParameterRegister(int nSize, AsmType nType, int& nParmNo)
{
#if defined(_WIN64)
	nParmNo++;
	if (nType < T_FLOAT)
	{
		if (nSize == 8)
		{
			if (nParmNo == 1)
				return RCX;
			else if (nParmNo == 2)
				return RDX;
			else if (nParmNo == 3)
				return R8;
			return R9;
		}
		else if (nSize == 4)
		{
			if (nParmNo == 1)
				return ECX;
			else if (nParmNo == 2)
				return EDX;
			else if (nParmNo == 3)
				return R8D;
			return R9D;
		}
		else if (nSize == 2)
		{
			if (nParmNo == 1)
				return CX;
			else if (nParmNo == 2)
				return DX;
			else if (nParmNo == 3)
				return R8W;
			return R9W;
		}
		else
		{
			if (nParmNo == 1)
				return AL;
			else if (nParmNo == 2)
				return DL;
			else if (nParmNo == 3)
				return R8B;
			return R9B;
		}
	}
	else if (nType <= T_DOUBLE)
	{
		if (nParmNo == 1)
			return XMM0;
		else if (nParmNo == 2)
			return XMM1;
		else if (nParmNo == 3)
			return XMM2;
		return XMM3;
	}
	else // T_STRUCT
	{
		if (nParmNo == 1)
			return RCX;
		else if (nParmNo == 2)
			return RDX;
		else if (nParmNo == 3)
			return R8;
		return R9;
	}
#else
	if (nType == T_FLOAT || nType == T_DOUBLE)
	{
		return EAX;
	}
	nParmNo++;
	if (nType == T_STRUCT)
	{
		if (nParmNo == 1)
			return ECX;
		return EDX;
	}
	if (nSize == 4)
	{
		if (nParmNo == 1)
			return ECX;
		return EDX;
	}
	else if (nSize == 2)
	{
		if (nParmNo == 1)
			return CX;
		return DX;
	}
	else
	{
		if (nParmNo == 1)
			return CL;
		return DL;
	}
#endif
}

AsmRegister RuntimeAssembler::GetParameterRegister(AsmRegister parm, int& nParmNo)
{
	int nBitCount = RegisterBitCount(parm);
	int nSize = nBitCount / 8;
	AsmType nType = (parm >= ST0 && parm <= ST7) ? T_DOUBLE : (nBitCount == 64) ? T_INT64 : T_INT;
	return GetParameterRegister(nSize, nType, nParmNo);
}

RuntimeAssembler& RuntimeAssembler::Ret(int nBytes)
{
	AsmInstruction ins;
	if (nBytes == -1)
		nBytes = ParameterSize();
	if (nBytes > 0)
	{
		ins.Opcode = OP_RET_IMM16;
		ins.Immediate.Immediate16(nBytes);
	}
	else if (nBytes == 0)
		ins.Opcode = OP_RET;
	m_Code->Add(ins);
	return *this;
}

AsmRegister RuntimeAssembler::ReturnRegister()
{
#if !defined(_WIN64)
	return EAX;
#else
	return RAX;
#endif
}

RuntimeAssembler& RuntimeAssembler::Fld(float nValue)
{
	AsmInstruction ins;
	if (nValue == 1.0)
	{
		ins.Opcode = OP_FLD_M32;
		ins.Opcode = OP_FLD_1;
	}
	else if (nValue == 0.0)
	{
		ins.Opcode = OP_FLD_M32;
		ins.Opcode = OP_FLD_0;
	}
	else
	{
		ins.Opcode = OP_FLD_M32;
		size_t nIndex = Data(nValue);
		ins.ModRM.Mod = ins.Displacement.DataDisplacement(nIndex);
#if !defined(_WIN64)
		ins.ModRM.Rm = MODRM_RM_SIB;
#else
		ins.ModRM.Rm = MODRM_RM_RIP_RELATIVE;
#endif
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Fld(double nValue)
{
	AsmInstruction ins;
	if (nValue == 1.0)
	{
		ins.Opcode = OP_FLD_M32;
		ins.Opcode = OP_FLD_1;
	}
	else if (nValue == 0.0)
	{
		ins.Opcode = OP_FLD_M32;
		ins.Opcode = OP_FLD_0;
	}
	else
	{
		size_t nIndex = Data(nValue);
		ins.Opcode = OP_FLD_M64;
		ins.ModRM.Mod = ins.Displacement.DataDisplacement(nIndex);
#if !defined(_WIN64)
		ins.ModRM.Rm = MODRM_RM_SIB;
#else
		ins.ModRM.Rm = MODRM_RM_RIP_RELATIVE;
#endif
	}
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Fld(AsmVariable pVar)
{
	assert(pVar.Location == Memory && (pVar.Type == T_DOUBLE || pVar.Type == T_FLOAT)) ;
	AsmInstruction ins;
	if (pVar.Type == T_DOUBLE)
		ins.Opcode = OP_FLD_M64;
	else if (pVar.Type == T_FLOAT)
		ins.Opcode = OP_FLD_M32;

	ModRm_Var(ins, pVar);
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Fst(AsmRegister nReg)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nReg);
	assert(nReg >= ST0 && nReg <= ST7);
	ins.Opcode = OP_FST_M64;
	ins.Opcode2 = Make_Op(OP_FST_REG, nReg);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Fstp(AsmRegister nReg)
{
	AsmInstruction ins;
	int nBitCount = RegisterBitCount(nReg);
	assert(nReg >= ST0 && nReg <= ST7);
	ins.Opcode = OP_FSTP_M64;
	ins.Opcode2 = Make_Op(OP_FSTP_REG, nReg);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::Fstp(AsmVariable pVar)
{
	AsmInstruction ins;
	if (pVar.Location == Register)
		return Fstp(pVar.Register);

	if (pVar.Type == T_DOUBLE)
		ins.Opcode = OP_FSTP_M64;
	else if (pVar.Type == T_FLOAT)
		ins.Opcode = OP_FSTP_M32;
	else
		return *this;

	ModRm_Var(ins, pVar);
	m_Code->Add(ins);
	return *this;
}

RuntimeAssembler& RuntimeAssembler::VMovsp(AsmRegister nReg)
{
	return *this;
}

RuntimeAssembler& RuntimeAssembler::VMovsp(AsmVariable pVar)
{
	// TODO 
	// return Fstp_Ex(pVar.Type, pVar.Offset);
	return *this;
}