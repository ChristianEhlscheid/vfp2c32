#define _WINSOCKAPI_ // we're using winsock2 .. so this is neccessary to exclude winsock.h 

#include "windows.h"
#include "vfp2cassembly.h"

/* this code implements a runtime assembly code emitter */

const int RuntimeAssembler::Asm_Types[10][3] =  // size, alignment & sign of C types
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

/* pointer to codesection */
RuntimeAssembler::RuntimeAssembler()
{
	m_CS = m_CodeBuffer;
	m_CSEx = 0;
	m_ParmCount = -1;
	m_VarCount = -1;
	m_LabelCount = -1;
	m_JumpCount = -1;
}

void RuntimeAssembler::WriteCode(void *lpAddress)
{
	m_CSEx = (LPCODE)lpAddress;
	CopyMemory(lpAddress, m_CodeBuffer, CodeSize());
}

int RuntimeAssembler::CodeSize()
{
	return m_CS - m_CodeBuffer;
}

void RuntimeAssembler::Prolog()
{
	Push(EBP);
	Mov(EBP,ESP);
	if (m_VarCount >= 0)
		Sub(ESP, -m_Vars[m_VarCount].nOffset);
}

void RuntimeAssembler::Epilog(bool bCDecl)
{
 	Mov(ESP,EBP);
	Pop(EBP);
	Ret(bCDecl ? 0 : ParameterSize());
}

void RuntimeAssembler::Parameter(char *pParameter, TYPE nType)
{
	LPCTYPEINFO lpType = &m_Parms[m_ParmCount+1];
	strcpy(lpType->aName, pParameter);
	ParameterEx(lpType, nType);
}

void RuntimeAssembler::Parameter(TYPE nType)
{
	LPCTYPEINFO lpType = &m_Parms[m_ParmCount+1];
	strcpy(lpType->aName,"");
	ParameterEx(lpType,nType);
}

void RuntimeAssembler::ParameterEx(LPCTYPEINFO lpType, TYPE nType)
{
	lpType->nType = nType;
	lpType->nSize = Asm_Types[nType][CTYPE_SIZE];
	lpType->nAlign = Asm_Types[nType][CTYPE_ALIGN];
	lpType->bSign = Asm_Types[nType][CTYPE_SIGN];
	
	if (m_ParmCount >= 0)
		lpType->nOffset = m_Parms[m_ParmCount].nOffset + max(m_Parms[m_ParmCount].nSize, sizeof(int));
	else
		lpType->nOffset = 8;

	m_ParmCount++;
}

LPCTYPEINFO RuntimeAssembler::ParameterRef(char *pParameter)
{
	LPCTYPEINFO lpType = m_Parms;
	int nParms = m_ParmCount;
	while (nParms-- >= 0)
	{
		if (!strcmp(pParameter, lpType->aName))
			return lpType;
		lpType++;
	}
	return 0;
}

LPCTYPEINFO RuntimeAssembler::ParameterRef(PARAMNO nParmNo)
{
	if (m_ParmCount >= (nParmNo-1))
		return &m_Parms[nParmNo-1];
	else
		return 0;

}

int RuntimeAssembler::ParameterSize()
{
	if (m_ParmCount >= 0)
		return m_Parms[m_ParmCount].nOffset + max(m_Parms[m_ParmCount].nSize,sizeof(int)) - 8;
	else
		return 0;
}

void RuntimeAssembler::LocalVar(char *pVariable, TYPE nType)
{
	LPCTYPEINFO lpType = &m_Vars[m_VarCount+1];

	strcpy(lpType->aName,pVariable);
	lpType->nType = nType;
	lpType->nSize = Asm_Types[nType][CTYPE_SIZE];
	lpType->nAlign = Asm_Types[nType][CTYPE_ALIGN];
	lpType->bSign = Asm_Types[nType][CTYPE_SIGN];
	
	if (m_VarCount >= 0)
	{
		lpType->nOffset = m_Vars[m_VarCount].nOffset - lpType->nSize;
		lpType->nOffset -= lpType->nOffset % lpType->nAlign;
	}
	else
		lpType->nOffset = -lpType->nSize;

	m_VarCount++;
}

void RuntimeAssembler::LocalVar(char *pVariable, int nSize, int nAlignment)
{
	LPCTYPEINFO lpType = &m_Vars[m_VarCount+1];

	strcpy(lpType->aName,pVariable);
	lpType->nSize = nSize;
	lpType->nAlign = nAlignment;
	
	if (m_VarCount >= 0)
	{
		lpType->nOffset = m_Vars[m_VarCount].nOffset - lpType->nSize;
		lpType->nOffset -= lpType->nOffset % lpType->nAlign;
	}
	else
		lpType->nOffset = -lpType->nSize;

	m_VarCount++;
}

LPCTYPEINFO RuntimeAssembler::LocalVarRef(char *pVariable)
{
	LPCTYPEINFO lpType = m_Vars;
	int nVars = m_VarCount;

	while (nVars-- >= 0)
	{
		if (!strcmp(pVariable,lpType->aName))
			return lpType;
		lpType++;
	}
	return 0;
}

LPCTYPEINFO RuntimeAssembler::ParmOrVarRef(char *pVariable)
{
	LPCTYPEINFO lpType;
	lpType = ParameterRef(pVariable);
	if (lpType)
		return lpType;
	else
		return LocalVarRef(pVariable);
}

void RuntimeAssembler::Label(char *pLabel)
{
	LPLABEL lpLabel = &m_Labels[++m_LabelCount];
	strcpy(lpLabel->aName,pLabel);
	lpLabel->pLocation = m_CS;
}

void RuntimeAssembler::Jump(char *pLabel)
{
	LPLABEL lpJump = &m_Jumps[++m_JumpCount];
	strcpy(lpJump->aName,pLabel);
	lpJump->pLocation = m_CS;
	m_CS += sizeof(int);
}

void RuntimeAssembler::Patch()
{
	int nJumps, nDist;
	LPLABEL lpJump, lpLabel;

	for (nJumps = 0; nJumps <= m_JumpCount; nJumps++)
	{
		lpJump = &m_Jumps[nJumps];
		lpLabel = LabelRef(lpJump->aName);
		if (lpLabel)
		{
			nDist = lpLabel->pLocation - lpJump->pLocation - sizeof(int);
			*(int*)lpJump->pLocation = nDist;
		}
	}
}

LPLABEL RuntimeAssembler::LabelRef(char *pLabel)
{
	int nLabels;
	for (nLabels = 0; nLabels <= m_LabelCount; nLabels++)
	{
		if (!strcmp(m_Labels[nLabels].aName,pLabel))
			return &m_Labels[nLabels];
	}
	return 0;
}

void* RuntimeAssembler::LabelAddress(char *pLabel)
{
	LPLABEL lpLabel = LabelRef(pLabel);
	if (lpLabel)
	{
		// pointer of realcode + offset of label in codebuffer
		return m_CSEx + (lpLabel->pLocation - m_CodeBuffer);
	}
	else
		return 0;
}

void RuntimeAssembler::BreakPoint()
{
	*m_CS++ = 0xCC;
}

/* push a register onto the stack */
void RuntimeAssembler::Push(REGISTER nReg)
{
	if (nReg > EDI)
		*m_CS++ = OP_ADRSIZE_16;
	*m_CS++ = MAKE_OP(OP_PUSH_R32,nReg);
}

void RuntimeAssembler::Push(AVALUE nValue)
{
	int nValue2 = (int)nValue;
	if (InByteRange(nValue2))
	{
		*m_CS++ = OP_PUSH_IMM8;
		*m_CS++ = (CODE)nValue2;
	}
	else
	{
		*m_CS++ = OP_PUSH_IMM32;
		*(int*)m_CS = nValue2;
		m_CS += sizeof(int);
	}
}

void RuntimeAssembler::Push(int nValue)
{
	*m_CS++ = OP_PUSH_IMM32;
	*(int*)m_CS = nValue;
	m_CS += sizeof(int);
}

void RuntimeAssembler::Push_Ex(TYPE nType, int nOffset)
{
	if (nType <= T_UINT || nType == T_FLOAT)
	{
		if (InByteRange(nOffset))
		{
			*m_CS++ = OP_PUSH_RM32;
			*m_CS++ = MODRM_DISP8_OP_STACK(MODRM_OP_PUSH);
			*m_CS++ = (CODE)nOffset;
		}
		else
		{
			*m_CS++ = OP_PUSH_RM32;
			*m_CS++ = MODRM_DISP32_OP_STACK(MODRM_OP_PUSH);
			*(int*)m_CS = nOffset;
			m_CS += sizeof(int);
		}
	}
	else if (nType <= T_INT64 || nType == T_DOUBLE)
	{
		if (InByteRange(nOffset) && InByteRange((nOffset+4)))
		{
			// push upper 32 bit
			*m_CS++ = OP_PUSH_RM32;
			*m_CS++ = MODRM_DISP8_OP_STACK(MODRM_OP_PUSH);
			*m_CS++ = (CODE)nOffset+4;

			// push lower 32 bit
			*m_CS++ = OP_PUSH_RM32;
			*m_CS++ = MODRM_DISP8_OP_STACK(MODRM_OP_PUSH);
			*m_CS++ = (CODE)nOffset;
		}
		else
		{
			// push upper 32 bit
			*m_CS++ = OP_PUSH_RM32;
			*m_CS++ = MODRM_DISP32_OP_STACK(MODRM_OP_PUSH);
			*(int*)m_CS = nOffset+4;
			m_CS += sizeof(int);
			
			// push lower 32 bit
			*m_CS++ = OP_PUSH_RM32;
			*m_CS++ = MODRM_DISP32_OP_STACK(MODRM_OP_PUSH);
			*(int*)m_CS = nOffset;
			m_CS += sizeof(int);
		}
	}
}

void RuntimeAssembler::Push(PARAMNO nParmNo)
{
	LPCTYPEINFO lpType = ParameterRef(nParmNo);
	if (lpType)
		Push_Ex(lpType->nType,lpType->nOffset);
}

void RuntimeAssembler::Push(char *pParmOrVar)
{
	LPCTYPEINFO lpType = ParmOrVarRef(pParmOrVar);
	if (lpType)
		Push_Ex(lpType->nType,lpType->nOffset);
}

void RuntimeAssembler::Push(PARAMNO nParmNo, TYPE nType, int nOffset)
{
	LPCTYPEINFO lpType = ParameterRef(nParmNo);
	if (lpType)
		Push_Ex(nType,lpType->nOffset + nOffset);
}

void RuntimeAssembler::Push(char *pParmOrVar, TYPE nType, int nOffset)
{
	LPCTYPEINFO lpType = ParmOrVarRef(pParmOrVar);
	if (lpType)
		Push_Ex(nType,lpType->nOffset + nOffset);
}

/* pop a value from the stack into a register */
void RuntimeAssembler::Pop(REGISTER nReg)
{
	if (nReg <= EDI)
		*m_CS++ = MAKE_OP(OP_POP_R32,nReg);
	else if (nReg <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = MAKE_OP(OP_POP_R32,nReg);
	}
}

/* Sub ?! :) */
void RuntimeAssembler::Sub(REGISTER nReg, int nBytes)
{
	if (nReg <= EDI)
	{
		if (InByteRange(nBytes))
		{
			*m_CS++ = OP_SUB_R32_IMM8;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_SUB,nReg);
			*(char*)m_CS++ = nBytes;
		}
		else
		{
			if (nReg == EAX)
				*m_CS++ = OP_SUB_EAX_IMM32;
			else
			{
				*m_CS++ = OP_SUB_R32_IMM32;
				*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_SUB,nReg);
			}
			*(int*)m_CS = nBytes;
			m_CS += sizeof(int);
		}
	}
	else if (nReg <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		if (nReg == AX)
			*m_CS++ = OP_SUB_AX_IMM16;
		else
		{
			*m_CS++ = OP_SUB_R32_IMM32;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_SUB,nReg);
		}
		*(short*)m_CS = nBytes;
		m_CS += sizeof(short);
	}
	else
	{
		if (nReg == AL)
			*m_CS++ = OP_SUB_AL_IMM8;
		else
		{
			*m_CS++ = OP_SUB_RM8_IMM8;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_SUB,nReg);
		}
		*(char*)m_CS++ = nBytes;
	}
}

void RuntimeAssembler::Sub(REGISTER nReg, unsigned int nBytes)
{
	if(nReg <= EDI)
	{
		if (nReg == EAX)
			*m_CS++ = OP_SUB_EAX_IMM32;
		else
		{
			*m_CS++ = OP_SUB_R32_IMM32;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_SUB,nReg);
		}
		*(unsigned int*)m_CS = nBytes;
		m_CS += sizeof(unsigned int);
	}
	else if (nReg <= DI)
	{
		if (nReg == AX)
			*m_CS++ = OP_SUB_AX_IMM16;
		else
		{
			*m_CS++ = OP_ADRSIZE_16;
			*m_CS++ = OP_SUB_R32_IMM32;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_SUB,nReg);
		}
		*(unsigned int*)m_CS = nBytes;
		m_CS += sizeof(unsigned int);
	}
	else
	{
		if (nReg = AL)
			*m_CS++ = OP_SUB_AL_IMM8;
		else
		{
			*m_CS++ = OP_SUB_RM8_IMM8;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_SUB,nReg);
		}
		*(unsigned __int8*)m_CS++ = nBytes;
	}
}

void RuntimeAssembler::Sub(REGISTER nReg, REGISTER nReg2)
{
	if (nReg <= EDI)
	{
		*m_CS++ = OP_SUB_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
	else if (nReg <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = OP_SUB_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
	else
	{
		*m_CS++ = OP_SUB_R8_RM8;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
}

/* Add ?! :) */
void RuntimeAssembler::Add(REGISTER nReg, int nBytes)
{
	if (nReg <= EDI)
	{
		if (InByteRange(nBytes))
		{
			*m_CS++ = OP_ADD_R32_IMM8;
			*m_CS++ = MODRM_REG_REG(nReg);
			*(char*)m_CS++ = nBytes;
		}
		else
		{
			if (nReg == EAX)
				*m_CS++ = OP_ADD_EAX_IMM32;
			else
			{
				*m_CS++ = OP_ADD_RM32_IMM32;
				*m_CS++ = MODRM_REG_REG(nReg);
			}
			*(int*)m_CS = nBytes;
			m_CS += sizeof(int);
		}
	}
	else if (nReg <= DI)
	{
		if (nReg == AX)
			*m_CS++ = OP_ADD_AX_IMM16;
		else
		{
			*m_CS++ = OP_ADRSIZE_16;
			*m_CS++ = OP_ADD_RM32_IMM32;
			*m_CS++ = MODRM_REG_REG(nReg);
		}
		*(short*)m_CS = (short)nBytes;
		m_CS += sizeof(short);
	}
	else 
	{
		if (nReg == AL)
			*m_CS++ = OP_ADD_AL_IMM8;
		else
		{
			*m_CS++ = OP_ADD_RM8_IMM8;
			*m_CS++ = MODRM_REG_REG(nReg);
		}
		*(char*)m_CS++ = nBytes;
	}
}

void RuntimeAssembler::Add(REGISTER nReg, unsigned int nBytes)
{
	if (nReg <= EDI)
	{
		if (nReg == EAX)
			*m_CS++ = OP_ADD_EAX_IMM32;
		else
		{
			*m_CS++ = OP_ADD_RM32_IMM32;
			*m_CS++ = MODRM_REG_REG(nReg);
		}
		*(unsigned int*)m_CS = nBytes;
		m_CS += sizeof(unsigned int);
	}
	else if (nReg <= DI)
	{
		if (nReg == AX)
			*m_CS++ = OP_ADD_AX_IMM16;
		else
		{
			*m_CS++ = OP_ADRSIZE_16;
			*m_CS++ = OP_ADD_RM32_IMM32;
			*m_CS++ = MODRM_REG_REG(nReg);
		}
		*(unsigned short*)m_CS = (unsigned short)nBytes;
		m_CS += sizeof(unsigned short);
	}
	else 
	{
		if (nReg == AL)
			*m_CS++ = OP_ADD_AL_IMM8;
		else
		{
			*m_CS++ = OP_ADD_RM8_IMM8;
			*m_CS++ = MODRM_REG_REG(nReg);
		}
		*(char*)m_CS++ = nBytes;
	}
}

void RuntimeAssembler::Add(REGISTER nReg, REGISTER nReg2)
{
	if (nReg <= EDI)
	{
		*m_CS++ = OP_ADD_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
	else if (nReg <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = OP_ADD_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
	else
	{
		*m_CS++ = OP_ADD_R8_RM8;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
}

/* Dec ?! :) decrement; someRegister--; */
void RuntimeAssembler::Dec(REGISTER nReg)
{
	if (nReg <= EDI)
		*m_CS++ = MAKE_OP(OP_DEC_R32,nReg);
	else if (nReg <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = MAKE_OP(OP_DEC_R32,nReg);
	}
	else 
	{
		*m_CS++ = OP_DEC_RM8;
		*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_DEC,nReg);
	}
}

/* Inc ?! :) increment; someRegister++; */
void RuntimeAssembler::Inc(REGISTER nReg)
{
	if (nReg <= EDI)
		*m_CS++ = MAKE_OP(OP_INC_R32,nReg);
	else if (nReg <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = MAKE_OP(OP_INC_R32,nReg);
	}
	else
	{
		*m_CS++ = OP_INC_RM8;
		*m_CS++ = MODRM_REG_REG(nReg);
	}
}

void RuntimeAssembler::And(REGISTER nReg, int nValue)
{
	if (nReg <= ESI)
	{
		if (nReg == EAX)
		{
			*m_CS++ = OP_AND_EAX_IMM32;
			*(int*)m_CS = nValue;
			m_CS += sizeof(int);
		}
		else
		{
            if (InByteRange(nValue))
			{
				*m_CS++ = OP_AND_RM32_IMM8;
				*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_AND,nReg);
				*(char*)m_CS++ = nValue;
			}
			else
			{
				*m_CS++ = OP_AND_RM32_IMM32;
				*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_AND,nReg);
				*(int*)m_CS = nValue;
				m_CS += sizeof(int);
			}
		}
	}
	else if (nReg <= DI)
	{
		if (nReg == AX)
		{
			*m_CS++ = OP_ADRSIZE_16;
			*m_CS++ = OP_AND_EAX_IMM32;
			*(short*)m_CS = (short)nValue;
			m_CS += sizeof(short);
		}
		else
		{
			*m_CS++ = OP_ADRSIZE_16;
			*m_CS++ = OP_AND_RM32_IMM32;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_AND,nReg);
			*(short*)m_CS = (short)nValue;
			m_CS += sizeof(short);
		}
	}
	else
	{
		if (nReg == AL)
		{
			*m_CS++ = OP_AND_AL_IMM8;
			*(char*)m_CS++ = (char)nValue;
		}
		else
		{
			*m_CS++ = OP_AND_RM8_IMM8;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_AND,nReg);
			*(char*)m_CS++ = (char)nValue;
		}
	}
}

void RuntimeAssembler::And(REGISTER nReg, REGISTER nReg2)
{
	if (nReg <= EDI)
	{
		*m_CS++ = OP_AND_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
	else if (nReg <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = OP_AND_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
	else
	{
		*m_CS++ = OP_AND_R8_RM8;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
}

void RuntimeAssembler::Or(REGISTER nReg, int nValue)
{
	if (nReg <= EDI)
	{
		if (nReg == EAX)
		{
			*m_CS++ = OP_OR_EAX_IMM32;
			*(int*)m_CS = nValue;
			m_CS += sizeof(int);
		}
		else
		{
			*m_CS++ = OP_OR_RM32_IMM32;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_OR,nReg);
			*(int*)m_CS = nValue;
			m_CS += sizeof(int);
		}
	}
	else if (nReg <= DI)
	{
		if (nReg == AX)
		{
			*m_CS++ = OP_ADRSIZE_16;
			*m_CS++ = OP_OR_EAX_IMM32;
			*(short*)m_CS = nValue;
			m_CS += sizeof(short);
		}
		else
		{
			*m_CS++ = OP_ADRSIZE_16;
			*m_CS++ = OP_OR_RM32_IMM32;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_OR,nReg);
			*(short*)m_CS = nValue;
			m_CS += sizeof(short);
		}
	}
	else
	{
		if (nReg == AL)
		{
			*m_CS++ = OP_OR_AL_IMM8;
			*(char*)m_CS++ = (char)nValue;
		}
		else
		{
			*m_CS++ = OP_OR_RM8_IMM8;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_OR,nReg);
		}
	}
}

void RuntimeAssembler::Or(REGISTER nReg, REGISTER nReg2)
{
	if (nReg <= EDI)
	{
		*m_CS++ = OP_OR_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
	else if (nReg <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = OP_OR_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
	else
	{
		*m_CS++ = OP_OR_R8_RM8;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
}

void RuntimeAssembler::Xor(REGISTER nReg, int nValue)
{
	if (nReg <= EDI)
	{
		if (nReg == EAX)
		{
			*m_CS++ = OP_XOR_EAX_IMM32;
			*(int*)m_CS = nValue;
			m_CS += sizeof(int);
		}
		else
		{
			*m_CS++ = OP_XOR_RM32_IMM32;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_XOR,nReg);
			*(int*)m_CS = nValue;
			m_CS += sizeof(int);
		}
	}
	else if (nReg <= DI)
	{
		if (nReg == AX)
		{
			*m_CS++ = OP_ADRSIZE_16;
			*m_CS++ = OP_XOR_EAX_IMM32;
			*(short*)m_CS = nValue;
			m_CS += sizeof(short);
		}
		else
		{
			*m_CS++ = OP_ADRSIZE_16;
			*m_CS++ = OP_XOR_RM32_IMM32;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_XOR,nReg);
			*(short*)m_CS = nValue;
			m_CS += sizeof(short);
		}
	}
	else
	{
		if (nReg == AL)
		{
			*m_CS++ = OP_XOR_AL_IMM8;
			*(char*)m_CS++ = (char)nValue;
		}
		else
		{
			*m_CS++ = OP_XOR_RM8_IMM8;
			*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_XOR,nReg);
		}
	}
}

void RuntimeAssembler::Xor(REGISTER nReg, REGISTER nReg2)
{
	if (nReg <= EDI)
	{
		*m_CS++ = OP_XOR_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
	else if (nReg <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = OP_XOR_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
	else
	{
		*m_CS++ = OP_XOR_R8_RM8;
		*m_CS++ = MODRM_REG_REG_REG(nReg,nReg2);
	}
}


void RuntimeAssembler::Cdq()
{
	*m_CS++ = OP_CDQ;
}

void RuntimeAssembler::Shift_Ex(REGISTER nReg, int nBits, int nOpcode)
{
	if (nReg <= DI)
	{
		if (nReg > EDI)
			*m_CS++ = OP_ADRSIZE_16;

		if (nBits == 1)
		{
			*m_CS++ = OP_SHIFT_RM32;
			*m_CS++ = MODRM_REG_OP_REG(nOpcode,nReg);
		}
		else
		{
			*m_CS++ = OP_SHIFT_RM32_IMM8;
			*m_CS++ = MODRM_REG_OP_REG(nOpcode,nReg);
			*(char*)m_CS++ = (char)nBits;
		}
	}
	else
	{
		if (nBits == 1)
		{
			*m_CS++ = OP_SHIFT_RM8;
			*m_CS++ = MODRM_REG_OP_REG(nOpcode,nReg);
		}
		else
		{
			*m_CS++ = OP_SHIFT_RM8_IMM8;
			*m_CS++ = MODRM_REG_OP_REG(nOpcode,nReg);
			*(char*)m_CS++ = (char)nBits;
		}
	}
}

void RuntimeAssembler::Sal(REGISTER nReg, int nBits)
{
	Shift_Ex(nReg,nBits,MODRM_OP_SAL);
}

void RuntimeAssembler::Sar(REGISTER nReg, int nBits)
{
	Shift_Ex(nReg,nBits,MODRM_OP_SAR);
}

void RuntimeAssembler::Shl(REGISTER nReg, int nBits)
{
	Shift_Ex(nReg,nBits,MODRM_OP_SHL);
}

void RuntimeAssembler::Shr(REGISTER nReg, int nBits)
{
	Shift_Ex(nReg,nBits,MODRM_OP_SHR);
}

void RuntimeAssembler::Lea_Ex(REGISTER nReg, int nOffset)
{
	*m_CS++ = OP_LEA_R32_M;
	if (InByteRange(nOffset))
	{
		*m_CS++ = MODRM_DISP8_REG_STACK(nReg);
		*(char*)m_CS++ = (char)nOffset;
	}
	else
	{
		*m_CS++ = MODRM_DISP32_REG_STACK(nReg);
		*(int*)m_CS = nOffset;
		m_CS += sizeof(int);
	}
}

void RuntimeAssembler::Lea(REGISTER nReg, char *pParmOrVar, int nOffset)
{
	LPCTYPEINFO lpType = ParmOrVarRef(pParmOrVar);
	if (lpType)
		Lea_Ex(nReg,lpType->nOffset + nOffset);
}

void RuntimeAssembler::Lea(REGISTER nReg, PARAMNO nParmNo, int nOffset)
{
	LPCTYPEINFO lpType = ParameterRef(nParmNo);
	if (lpType)
		Lea_Ex(nReg,lpType->nOffset + nOffset);
}

/* Move value from stack into a register */
void RuntimeAssembler::Mov_Ex(REGISTER nReg, TYPE nType, int nOffset)
{
	if (nReg <= EDI)
	{
		if (nType < T_INT)
		{
			if (Asm_Types[nType][CTYPE_SIGN]) // signed?
				MovSX_Ex(nReg,nType,nOffset);
			else
				MovZX_Ex(nReg,nType,nOffset);
		}
		else
		{
			*m_CS++ = OP_MOV_R32_RM32;
			if (InByteRange(nOffset))
			{
				*m_CS++ = MODRM_DISP8_REG_STACK(nReg);
				*(char*)m_CS++ = nOffset;
			}
			else
			{
				*m_CS++ = MODRM_DISP32_REG_STACK(nReg);
				*(int*)m_CS = nOffset;
				m_CS += sizeof(int);
			}
		}
	}
	else if (nReg <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = OP_MOV_R32_RM32;

		if (InByteRange(nOffset))
		{
			*m_CS++ = MODRM_DISP8_REG_STACK(nReg);
			*(char*)m_CS++ = nOffset;
		}
		else
		{
			*m_CS++ = MODRM_DISP32_REG_STACK(nReg);
			*(int*)m_CS = nOffset;
			m_CS += sizeof(int);
		}
	}
	else
	{
		*m_CS++ = OP_MOV_R8_RM8;
		if (InByteRange(nOffset))
		{
			*m_CS++ = MODRM_DISP8_REG_STACK(nReg);
			*m_CS++ = (CODE)nOffset;
		}
		else
		{
			*m_CS++ = MODRM_DISP32_REG_STACK(nReg);
			*(int*)m_CS = nOffset;
			m_CS += sizeof(int);
		}
	}
}

void RuntimeAssembler::Mov(REGISTER nReg, PARAMNO nParmNo)
{
	LPCTYPEINFO lpType = ParameterRef(nParmNo);
	if (lpType)
		Mov_Ex(nReg,lpType->nType,lpType->nOffset);
}

/* Move declared Parameter/Local variable from stack into a register */
void RuntimeAssembler::Mov(REGISTER nReg, char *pParmOrVar)
{
    LPCTYPEINFO lpType = ParmOrVarRef(pParmOrVar);
	if (lpType)
		Mov_Ex(nReg,lpType->nType,lpType->nOffset);
}

void RuntimeAssembler::Mov(REGISTER nReg, PARAMNO nParmNo, TYPE nType, int nOffset)
{
	LPCTYPEINFO lpType = ParameterRef(nParmNo);
	if (lpType)
		Mov_Ex(nReg,nType,lpType->nOffset + nOffset);
}

void RuntimeAssembler::Mov(REGISTER nReg, char *pParmOrVar, TYPE nType, int nOffset)
{
	LPCTYPEINFO lpType = ParmOrVarRef(pParmOrVar);
	if (lpType)
		Mov_Ex(nReg,nType,lpType->nOffset + nOffset);
}

/* Move direct value into a register */
void RuntimeAssembler::Mov(REGISTER nReg, AVALUE nValue)
{
	if (nReg <= EDI)
	{
		*m_CS++ = MAKE_OP(OP_MOV_R32_IMM32,nReg);
		*(AVALUE*)m_CS = nValue;
		m_CS += sizeof(AVALUE);
	}
	else if (nReg <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = MAKE_OP(OP_MOV_R32_IMM32,nReg);
		*(unsigned short*)m_CS = (unsigned short)nValue;
		m_CS += sizeof(short);
	}
	else
	{
		*m_CS++ = MAKE_OP(OP_MOV_R8_IMM8,nReg);
		*(unsigned char*)m_CS++ = (unsigned char)nValue;
	}
}

/* Move value into parameter/local variable */
void RuntimeAssembler::Mov(char *pParmOrVar, unsigned int nValue)
{
	LPCTYPEINFO lpType = ParmOrVarRef(pParmOrVar);
	if (!lpType)
		return;

	if (lpType->nType <= T_UCHAR)
	{
		*m_CS++ = OP_MOV_RM8_IMM8;
		if (InByteRange(lpType->nOffset))
		{
            *m_CS++ = MODRM_DISP8_STACK();
			*(char*)m_CS++ = lpType->nOffset;
		}
		else
		{
			*m_CS++ = MODRM_DISP32_STACK();
			*(int*)m_CS = lpType->nOffset;
			m_CS += sizeof(int);
		}
		*(char*)m_CS++ = (char)nValue;
	}
	else if (lpType->nType <= T_USHORT)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = OP_MOV_RM32_IMM32;
		if (InByteRange(lpType->nOffset))
		{
            *m_CS++ = MODRM_DISP8_STACK();
			*(char*)m_CS++ = lpType->nOffset;
		}
		else
		{
			*m_CS++ = MODRM_DISP32_STACK();
			*(int*)m_CS = lpType->nOffset;
			m_CS += sizeof(int);
		}
		*(short*)m_CS = (short)nValue;
		m_CS += sizeof(short);
	}
	else if (lpType->nType <= T_UINT || lpType->nType == T_FLOAT)
	{
		*m_CS++ = OP_MOV_RM32_IMM32;
		if (InByteRange(lpType->nOffset))
		{
            *m_CS++ = MODRM_DISP8_STACK();
			*(char*)m_CS++ = lpType->nOffset;
		}
		else
		{
			*m_CS++ = MODRM_DISP32_STACK();
			*(int*)m_CS = lpType->nOffset;
			m_CS += sizeof(int);
		}
		*(int*)m_CS = nValue;
		m_CS += sizeof(int);
	}
}

/* move register into parameter/local variable */
void RuntimeAssembler::Mov(char *pParmOrVar, REGISTER nReg)
{
	LPCTYPEINFO lpType = ParmOrVarRef(pParmOrVar);
	if (!lpType)
		return;

	if (lpType->nType <= T_UCHAR)
	{
		*m_CS++ = MAKE_OP(OP_MOV_RM8_R8,nReg);
		if (InByteRange(lpType->nOffset))
		{
            *m_CS++ = MODRM_DISP8_STACK();
			*(char*)m_CS++ = lpType->nOffset;
		}
		else
		{
			*m_CS++ = MODRM_DISP32_STACK();
			*(int*)m_CS = lpType->nOffset;
			m_CS += sizeof(int);
		}
	}
	else if (lpType->nType <= T_USHORT)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = MAKE_OP(OP_MOV_RM32_R32,nReg);
		if (InByteRange(lpType->nOffset))
		{
            *m_CS++ = MODRM_DISP8_STACK();
			*(char*)m_CS++ = lpType->nOffset;
		}
		else
		{
			*m_CS++ = MODRM_DISP32_STACK();
			*(int*)m_CS = lpType->nOffset;
			m_CS += sizeof(int);
		}
	}
	else if (lpType->nType <= T_UINT || lpType->nType == T_FLOAT)
	{
		*m_CS++ = MAKE_OP(OP_MOV_RM32_R32,nReg);
		if (InByteRange(lpType->nOffset))
		{
            *m_CS++ = MODRM_DISP8_STACK();
			*(char*)m_CS++ = lpType->nOffset;
		}
		else
		{
			*m_CS++ = MODRM_DISP32_STACK();
			*(int*)m_CS = lpType->nOffset;
			m_CS += sizeof(int);
		}
	}
}

void RuntimeAssembler::Mov(RELREGISTER nRelReg, int nSize, int nValue)
{
	if (nSize == 1)
	{
		*m_CS++ = OP_MOV_RM8_IMM8;
		*m_CS++ = MODRM_REL_REG(nRelReg);
		*(char*)m_CS++ = (char)nValue;
	}
	else if (nSize == 2)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = OP_MOV_RM32_IMM32;
		*m_CS++ = MODRM_REL_REG(nRelReg);
		*(short*)m_CS = (short)nValue;
		m_CS += sizeof(short);
	}
	else if (nSize == 4)
	{
		*m_CS++ = OP_MOV_RM32_IMM32;
		*m_CS++ = MODRM_REL_REG(nRelReg);
		*(int*)m_CS = nValue;
		m_CS += sizeof(int);
	}
}

/* Move value from one register into another */
void RuntimeAssembler::Mov(REGISTER nRegDest, REGISTER nRegSource)
{
 	if (nRegDest <= EDI)
	{
   		*m_CS++ = OP_MOV_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nRegDest,nRegSource);
	}
	else if (nRegDest <= DI)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = OP_MOV_R32_RM32;
		*m_CS++ = MODRM_REG_REG_REG(nRegDest,nRegSource);
	}
	else
	{
		*m_CS++ = OP_MOV_R8_RM8;
		*m_CS++ = MODRM_REG_REG_REG(nRegDest,nRegSource);
	}
}

void RuntimeAssembler::MovZX_Ex(REGISTER nReg, TYPE nType, int nOffset)
{
	if (nReg > R_32BIT)
		*m_CS++ = OP_ADRSIZE_16;

	*m_CS++ = 0x0F;

	if (nType <= T_UCHAR)
		*m_CS++ = 0xB6;
	else
		*m_CS++ = 0xB7;

	if (InByteRange(nOffset))
	{
        //*m_CS++ = Asm_Mov_RS[nReg];
		*m_CS++ = (CODE)nOffset;
	}
	else
	{
		//*m_CS++ = Asm_Mov_RS[nReg] + 0x40;
		*(int*)m_CS = nOffset;
		m_CS += sizeof(int);
	}
}

void RuntimeAssembler::MovZX(REGISTER nReg, PARAMNO nParmNo)
{
	LPCTYPEINFO lpType = ParameterRef(nParmNo);
	if (lpType)
		MovZX_Ex(nReg,lpType->nType,lpType->nOffset);
}

void RuntimeAssembler::MovZX(REGISTER nReg, char *pParmOrVar)
{
	LPCTYPEINFO lpType = ParmOrVarRef(pParmOrVar);
	if (lpType)
		MovZX_Ex(nReg,lpType->nType,lpType->nOffset);
}

void RuntimeAssembler::MovZX(REGISTER nRegDest, REGISTER nRegSource)
{
	if (nRegDest > R_32BIT)
		*m_CS++ = OP_ADRSIZE_16;

	*m_CS++ = 0x0F;

	if (nRegSource > R_16BIT)
		*m_CS++ = 0xB6;
	else
		*m_CS++ = 0xB7;

	//*m_CS++ = Asm_RegReg[nRegDest][nRegSource];
}

void RuntimeAssembler::MovSX_Ex(REGISTER nReg, TYPE nType, int nOffset)
{
	
}

void RuntimeAssembler::Cmp(REGISTER nReg, int nValue)
{
	if (nReg <= R_32BIT)
	{
		if (InByteRange(nValue))
		{
			*m_CS++ = 0x83;
			//*m_CS++ = Asm_Cmp[nReg];
			*m_CS++ = (CODE)nValue;
		}
		else
		{
			if (nReg != EAX)
			{
				*m_CS++ = 0x81;
				//*m_CS++ = Asm_Cmp[nReg];
			}
			else
				*m_CS++ = 0x3D;
			
			*(int*)m_CS = nValue;
			m_CS += sizeof(int);
		}
	}
	else if (nReg <= R_16BIT)
	{
		if (InByteRange(nValue))
		{
			*m_CS++ = OP_ADRSIZE_16;
			if (nReg != AX)
			{
				*m_CS++ = 0x83;
				//*m_CS++ = Asm_Cmp[nReg];
				*m_CS++ = (CODE)nValue;
			}
			else
			{
				*m_CS++ = 0x3D;
				*(short*)m_CS++ = (short)nValue;
				m_CS += sizeof(short);
			}
		}
		else
		{
			*m_CS++ = OP_ADRSIZE_16;
			if (nReg != AX)
			{
				*m_CS++ = 0x81;
				//*m_CS++ = Asm_Cmp[nReg];
				*(short*)m_CS = (short)nValue;
			}
			else
			{
				*m_CS++ = 0x3D;
				*(short*)m_CS = (short)nValue;
			}
			m_CS += sizeof(short);
		}
	}
	else
	{
		if (nReg != AL)
		{
			*m_CS++ = 0x80;
			//*m_CS++ = Asm_Cmp[nReg];
		}
		else
			*m_CS++ = 0x3C;

		*m_CS++ = (CODE)nValue;
	}
}

void RuntimeAssembler::Cmp(REGISTER nReg, REGISTER nReg2)
{
	if (nReg <= R_32BIT)
	{
		*m_CS++ = 0x3B;
		//*m_CS++ = Asm_RegReg[nReg][nReg2];
	}
	else if (nReg <= R_16BIT)
	{
		*m_CS++ = OP_ADRSIZE_16;
		*m_CS++ = 0x3B;
		//*m_CS++ = Asm_RegReg[nReg][nReg2];
	}
	else
	{
		*m_CS++ = 0x3A;
		//*m_CS++ = Asm_RegReg[nReg][nReg2];
	}
}

void RuntimeAssembler::Je(char *pLabel)
{
	*m_CS++ = 0x0F;
	*m_CS++ = 0x84;
	Jump(pLabel);
}

void RuntimeAssembler::Jmp(char *pLabel)
{
	*m_CS++ = OP_JMP_REL32;
	Jump(pLabel);
}

/* jump to location in a register */
void RuntimeAssembler::Jmp(REGISTER nReg)
{
	*m_CS++ = OP_JMP_RM32;
	*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_JMP,nReg);
}

void RuntimeAssembler::Jmp(REGISTER nReg, AVALUE pLocation)
{
	Mov(nReg,pLocation);
	Jmp(nReg);
}

/* call a function pointer in a register */
void RuntimeAssembler::Call(REGISTER nReg)
{
	*m_CS++ = OP_CALL_R32;
	*m_CS++ = MODRM_REG_OP_REG(MODRM_OP_CALL,nReg);
}

void RuntimeAssembler::Call(REGISTER nReg, FUNCPTR pFunction)
{
	Mov(nReg,(AVALUE)pFunction);
	Call(nReg);
}

void RuntimeAssembler::Call(FUNCPTR pFunction)
{
	Mov(EAX,(AVALUE)pFunction);
	Call(EAX);
}

void RuntimeAssembler::Ret()
{
	Ret(ParameterSize());
}

void RuntimeAssembler::Ret(int nBytes)
{
	if (nBytes)
	{
		*m_CS++ = 0xC2;
		*(short*)m_CS = (short)nBytes;
		m_CS += sizeof(short);
	}
	else
		*m_CS++ = 0xC3;
}

void RuntimeAssembler::Fld_Ex(TYPE nType, int nOffset)
{
	if (nType == T_DOUBLE)
		*m_CS++ = 0xDD;
	else if (nType == T_FLOAT)
		*m_CS++ = 0xD9;
	else
		return;

	if (InByteRange(nOffset))
	{
		*m_CS++ = 0x45;
		*m_CS++ = (CODE)nOffset;
	}
	else
	{
		*m_CS++ = 0x45;
		*(int*)m_CS = nOffset;
		m_CS += sizeof(int);
	}
}

void RuntimeAssembler::Fld(PARAMNO nParmNo)
{
	LPCTYPEINFO lpType = ParameterRef(nParmNo);
	if (lpType)
		Fld_Ex(lpType->nType,lpType->nOffset);
}

void RuntimeAssembler::Fld(char *pParmOrVar)
{
	LPCTYPEINFO lpType = ParameterRef(pParmOrVar);
	if (lpType)
		Fld_Ex(lpType->nType,lpType->nOffset);
}

void RuntimeAssembler::Fld(PARAMNO nParmNo, TYPE nType, int nOffset)
{
	LPCTYPEINFO lpType = ParameterRef(nParmNo);
	if (lpType)
		Fld_Ex(nType,lpType->nOffset + nOffset);
}

void RuntimeAssembler::Fld(char *pParmOrVar, TYPE nType, int nOffset)
{
	LPCTYPEINFO lpType = ParameterRef(pParmOrVar);
	if (lpType)
		Fld_Ex(nType,lpType->nOffset + nOffset);
}