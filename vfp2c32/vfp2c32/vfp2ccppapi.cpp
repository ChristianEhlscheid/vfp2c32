#include <math.h>
#include <new>
#include <stdio.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2ccppapi.h"

int CFoxVersion::m_Version = 0;
int CFoxVersion::m_MajorVersion = 0;
int CFoxVersion::m_MinorVersion = 0;

NTI _stdcall NewVar(char *pVarname, Locator &sVar, bool bPublic) throw(int)
{
	NTI nVarNti;
	sVar.l_subs = 0;
	nVarNti = _NewVar(pVarname, &sVar, bPublic ? NV_PUBLIC : NV_PRIVATE);
	if (nVarNti < 0)
	{
		int nErrorNo = (int)-nVarNti;
		throw nErrorNo;
	}
	return nVarNti;
}

 NTI _stdcall FindVar(char *pVarname) throw(int)
 {
	NTI nVarNti;
	nVarNti = _NameTableIndex(pVarname);
	if (nVarNti == -1)
		throw E_VARIABLENOTFOUND;

	return nVarNti;
 }

void _stdcall FindVar(NTI nVarNti, Locator &sVar) throw(int)
{
	if (!_FindVar(nVarNti, -1, &sVar))
		throw E_VARIABLENOTFOUND;
}

void _stdcall FindVar(char *pVarname, Locator &sVar) throw(int)
{
	NTI nVarNti;
	nVarNti = FindVar(pVarname);
	FindVar(nVarNti, sVar);
}

void _stdcall StoreObjectRef(char *pName, NTI &nVarNti, Value &sObject)
{
	Locator lVar;
	Value vTmpObject = {'0'};
	if (nVarNti)
	{
		FindVar(nVarNti, lVar);
		// increment reference count by calling evaluate
		Evaluate(vTmpObject, pName);
		Store(lVar, sObject);
		ObjectRelease(sObject);
	}
	else
	{
		nVarNti = NewVar(pName, lVar, true);
		Store(lVar, sObject);
		ObjectRelease(sObject);
	}
}

void _stdcall ReleaseObjectRef(char *pName, NTI nVarNti)
{
	Value vObject = {'0'};
	if (nVarNti)
	{
		_Evaluate(&vObject, pName);
		_Release(nVarNti);
	}
}



/* implementation of C++ wrapper classes over FoxPro datatypes */

/* FoxReference */
FoxReference& FoxReference::operator=(FoxValue &pValue)
{
	int nErrorNo;
	if (m_Loc.l_where == -1)
	{
		if (nErrorNo = _Store(&m_Loc, pValue))
			throw nErrorNo;
	}
	else
	{
		if (nErrorNo = _DBReplace(&m_Loc, pValue))
			throw nErrorNo;
	}
	return *this;
}

FoxReference& FoxReference::operator=(Locator &pLoc)
{
	int nErrorNo;
	FoxValue pValue(pLoc);
	if (m_Loc.l_where == -1)
	{
		if (nErrorNo = _Store(&m_Loc, pValue))
			throw nErrorNo;
	}
	else
	{
		if (nErrorNo = _DBReplace(&m_Loc, pValue))
			throw nErrorNo;
	}
	return *this;
}

FoxReference& FoxReference::operator=(int nValue)
{
	int nErrorNo;
	IntValue vTmp(nValue);
	if (m_Loc.l_where == -1)
	{
		if (nErrorNo = _Store(&m_Loc, &vTmp))
			throw nErrorNo;
	}
	else
	{
		if (nErrorNo = _DBReplace(&m_Loc, &vTmp))
			throw nErrorNo;
	}
	return *this;
}

FoxReference& FoxReference::operator=(unsigned long nValue)
{
	int nErrorNo;
	UIntValue vTmp(nValue);
	if (m_Loc.l_where == -1)
	{
		if (nErrorNo = _Store(&m_Loc, &vTmp))
			throw nErrorNo;
	}
	else
	{
		if (nErrorNo = _DBReplace(&m_Loc, &vTmp))
			throw nErrorNo;
	}
	return *this;
}

FoxReference& FoxReference::operator=(double nValue)
{
	int nErrorNo;
	DoubleValue vTmp(nValue);
	if (m_Loc.l_where == -1)
	{
		if (nErrorNo = _Store(&m_Loc, &vTmp))
			throw nErrorNo;
	}
	else
	{
		if (nErrorNo = _DBReplace(&m_Loc, &vTmp))
			throw nErrorNo;
	}
	return *this;
}

FoxReference& FoxReference::operator=(bool bValue)
{
	int nErrorNo;
	LogicalValue vTmp(bValue);
	if (m_Loc.l_where == -1)
	{
		if (nErrorNo = _Store(&m_Loc, &vTmp))
			throw nErrorNo;
	}
	else
	{
		if (nErrorNo = _DBReplace(&m_Loc, &vTmp))
			throw nErrorNo;
	}
	return *this;
}

/* FoxString */
FoxString::FoxString() : FoxValue('C')
{
	m_ParameterRef = false;
	m_BufferSize = 0;
	m_String = 0;
}

FoxString::FoxString(FoxString &pString) : FoxValue('C')
{
	m_ParameterRef = false;
	m_BufferSize = pString.Size();

	if (m_BufferSize)
	{
		AllocHandle(m_BufferSize);
		Len(pString.Len());
		Binary(pString.Binary());
		m_String = HandleToPtr();
		memcpy(m_String, pString, Len());
	}
	else
	{
		m_String = 0;
	}
}

FoxString::FoxString(Value &pVal) : FoxValue('C')
{
	// nullterminate
	if (!NullTerminateValue(pVal))
		throw E_INSUFMEMORY;

	m_Value.ev_length = pVal.ev_length;
	m_Value.ev_width = pVal.ev_width;
	m_Value.ev_handle = pVal.ev_handle;
	m_BufferSize = pVal.ev_length + 1;
	m_ParameterRef = true;
	LockHandle();
	m_String = HandleToPtr();
}

FoxString::FoxString(Value &pVal, unsigned int nExpand) : FoxValue('C')
{
	if (nExpand > 0)
	{
		if (!ExpandValue(pVal, nExpand))
			throw E_INSUFMEMORY;
	}
	m_Value.ev_length = pVal.ev_length;
	m_Value.ev_width = pVal.ev_width;
	m_Value.ev_handle = pVal.ev_handle;
	m_BufferSize = pVal.ev_length + nExpand;
	m_ParameterRef = true;
	LockHandle();
	m_String = HandleToPtr();
}

FoxString::FoxString(ParamBlk *pParms, int nParmNo) : FoxValue('C')
{
	// if parameter count is equal or greater than the parameter we want
	if (pParms->pCount >= nParmNo)
	{
    	Value *pVal = &pParms->p[nParmNo-1].val;
		if (pVal->ev_type == 'C')
		{
			// nullterminate
			if (!NullTerminateValue(pVal))
				throw E_INSUFMEMORY;
	
			m_Value.ev_handle = pVal->ev_handle;
			m_Value.ev_length = pVal->ev_length;
			m_Value.ev_width = pVal->ev_width;
			m_BufferSize = pVal->ev_length + 1;
			m_ParameterRef = true;
			LockHandle();
			m_String = HandleToPtr();
			return;
		}
	}
	// else
	m_ParameterRef = false;
	m_BufferSize = 0;
	m_String = 0;
}

FoxString::FoxString(ParamBlk *pParms, int nParmNo, unsigned int nExpand) : FoxValue('C')
{
	// if parameter count is equal or greater than the parameter we want
	if (pParms->pCount >= nParmNo)
	{
    	Value *pVal = &pParms->p[nParmNo-1].val;
		if (pVal->ev_type == 'C')
		{
			if (nExpand > 0)
			{
				if (!ExpandValue(pVal, nExpand))
					throw E_INSUFMEMORY;
			}
			m_Value.ev_handle = pVal->ev_handle;
			m_Value.ev_length = pVal->ev_length;
			m_Value.ev_width = pVal->ev_width;
			m_BufferSize = pVal->ev_length + nExpand;
			m_ParameterRef = true;
			LockHandle();
			m_String = HandleToPtr();
			return;
		}
	}
	// else
	m_ParameterRef = false;
	m_BufferSize = 0;
	m_String = 0;
}

FoxString::FoxString(ParamBlk *pParms, int nParmNo, FoxStringInitialization eInit)
{
	// if parameter count is equal or greater than the parameter we want
	if (pParms->pCount >= nParmNo)
	{
    	Value *pVal = &pParms->p[nParmNo-1].val;
		if (pVal->ev_type == 'C')
		{
			if (eInit == NoNullIfEmpty || pVal->ev_length)
			{
				if (!NullTerminateValue(pVal))
					throw E_INSUFMEMORY;

				m_Value.ev_handle = pVal->ev_handle;
				m_Value.ev_length = pVal->ev_length;
				m_Value.ev_width = pVal->ev_width;
				m_BufferSize = pVal->ev_length + 1;
				m_ParameterRef = true;
				LockHandle();
				m_String = HandleToPtr();
				return;
			}
		}
	}
	// else
	m_ParameterRef = false;
	m_BufferSize = 0;
	m_String = 0;
}

FoxString::FoxString(const char *pString) : FoxValue('C')
{
	m_ParameterRef = false;
	if (pString)
	{
		unsigned int nStrLen = strlen(pString);
		m_BufferSize = nStrLen + 1;
		AllocHandle(m_BufferSize);
		LockHandle();
		m_String = HandleToPtr();
		memcpy(m_String,pString,nStrLen);
		m_String[nStrLen] = '\0';
		m_Value.ev_length = nStrLen;
		m_Value.ev_width = 0;
	}
	else
	{
		m_BufferSize = 0;
		m_String = 0;
	}
}

FoxString::FoxString(unsigned int nBufferSize) : FoxValue('C')
   {
	assert(nBufferSize > 0);
	m_ParameterRef = false;
	AllocHandle(nBufferSize);
	LockHandle();
	m_String = HandleToPtr();
	m_BufferSize = nBufferSize;
	*m_String = '\0';
}

FoxString::FoxString(BSTR pString, UINT nCodePage) : FoxValue('C')
{
	m_ParameterRef = false;
	if (pString)
	{
		unsigned int nLen = SysStringLen(pString);
		m_Value.ev_length = nLen;
		m_BufferSize = nLen + 1;
		AllocHandle(m_BufferSize);
		LockHandle();
		m_String = HandleToPtr();
		int nChars;
		nChars = WideCharToMultiByte(nCodePage, 0, pString, nLen, m_String, nLen, 0, 0);
		if (!nChars)
		{
			SaveWin32Error("WideCharToMultiByte", GetLastError());
			throw E_APIERROR;
		}
		m_String[nLen] = '\0';
	}
	else
	{
		m_BufferSize = 0;
		m_String = 0;
	}
}

FoxString::FoxString(SAFEARRAY *pArray) : FoxValue('C')
{
	HRESULT hr;
	VARTYPE vt;
	if (pArray)
	{
        if (pArray->fFeatures & FADF_HAVEVARTYPE)
		{
			hr = SafeArrayGetVartype(pArray, &vt);
			if (FAILED(hr))
			{
				SaveWin32Error("SafeArrayGetVartype", hr);
				throw E_APIERROR;
			}
			if (vt != VT_UI1)
				throw E_TYPECONFLICT;
		}

		m_ParameterRef = false;

		unsigned long nLen;
		nLen = pArray->rgsabound[0].cElements;
		m_Value.ev_length = nLen;
		m_Value.ev_width = 1;
		m_BufferSize = nLen;
		AllocHandle(m_BufferSize);
		LockHandle();
		m_String = HandleToPtr();

		void *pData;
		hr = SafeArrayAccessData(pArray, &pData);
		if (FAILED(hr))
		{
			SaveWin32Error("SafeArrayAccessData", hr);
			throw E_APIERROR;
		}

		memcpy(m_String, pData, nLen);

		hr = SafeArrayUnaccessData(pArray);
		if (FAILED(hr))
		{
			SaveWin32Error("SafeArrayUnaccessData", hr);
			throw E_APIERROR;
		}
	}
	else
	{
		m_BufferSize = 0;
		m_String = 0;
	}
}

FoxString::~FoxString()
{
	if (m_Value.ev_handle)
	{
		UnlockHandle();
		if (!m_ParameterRef)
			FreeHandle();
	}
	m_Value.ev_type = '0';
}

void FoxString::Release()
{
	if (m_Value.ev_handle)
	{
		UnlockHandle();
		if (!m_ParameterRef)
			FreeHandle();
		else
			m_Value.ev_handle = 0;
	}
	m_ParameterRef = false;
	m_Value.ev_length = 0;
	m_BufferSize = 0;
}

FoxString& FoxString::Size(unsigned int nSize)
{
	if (m_BufferSize >= nSize)
		return *this;

	if (m_Value.ev_handle)
	{
		UnlockHandle();
		SetHandleSize(nSize);
	}
	else
		AllocHandle(nSize);

	LockHandle();
	m_String = HandleToPtr();
	m_BufferSize = nSize;
	return *this;
}

bool FoxString::Empty() const
{
	if (m_String && Len())
	{
		const char *pString = m_String;
		do
		{
			if (*pString == ' ' || *pString == '\t' || *pString == '\n' || *pString == '\r')
				pString++;
			else
				return false;
		} while (*pString);
	}
	return true;
}

FoxString& FoxString::StrnCpy(const char *pString, unsigned int nMaxLen)
{
	assert(nMaxLen < m_BufferSize);
	return Len(strncpyex(m_String, pString, nMaxLen));
}

FoxString& FoxString::CopyBytes(const unsigned char *pBytes, unsigned int nLen)
{
	assert(nLen <= m_BufferSize);
	memcpy(m_String, pBytes, nLen);
	return Len(nLen);
}

FoxString& FoxString::CopyDblString(const char *pDblString, unsigned int nMaxLen)
{
	if (pDblString)
	{
		unsigned int nLen = strdblnlen(pDblString, nMaxLen);
		ExtendBuffer(nLen);
		return CopyBytes(reinterpret_cast<const unsigned char*>(pDblString), nLen);
	}
	else
		return Len(0);
}

FoxString& FoxString::StringLen()
{
	return Len(strnlen(m_String, m_BufferSize));
}

unsigned int FoxString::StringDblLen()
{
	const char *pString = m_String;
	unsigned int nMaxLen = m_BufferSize;

	while (1)
	{
		while (*pString != '\0' && nMaxLen--) ++pString;
	
		if (nMaxLen == 0)
			return m_BufferSize;

		pString++;
		if (*pString == '\0')
			return pString - m_String - 1;
		else
			nMaxLen--;
	}
}

unsigned int FoxString::StringDblCount()
{
	assert(m_String);
	const char *pString = m_String;
	unsigned int nMaxLen = m_BufferSize, nStringCnt = 0;

	if (*pString == '\0')
		return 0;

	while (1)
	{
		while (*pString != '\0' && nMaxLen--) ++pString;
		nStringCnt++;
		pString++;		
		if (!nMaxLen || *pString == '\0')
			break;
		nMaxLen--;
	}
	return nStringCnt;
}

void FoxString::Return()
{
	UnlockHandle();
	_RetVal(&m_Value);
	m_Value.ev_handle = 0;
	m_String = 0;
}

FoxString& FoxString::AddBs()
{
	if (m_String && Len())
	{
		char *pEnd = m_String + Len() - 1;
		if (*pEnd != '\\')
		{
			Size(Len() + 2);
			*++pEnd = '\\';
			*++pEnd = '\0';
		}
	}
	return *this;
}

FoxString& FoxString::Alltrim(char cParseChar)
{
	assert(cParseChar != '\0');
	 /* nonvalid or empty string */
	if (m_String == 0 || Len() == 0)
		return *this;

	char *pStart, *pEnd;
	pStart = m_String;
	pEnd = m_String + Len();	/* compute end of string */

	while (*pStart == cParseChar) pStart++; /* skip over spaces at beginning of string */

	if (pStart == pEnd) /* entire string consisted of spaces */
	{
		*m_String = '\0';
		Len(0);
		return *this;
	}

	while (*--pEnd == cParseChar); /* find end of valid characters */
	Len(pEnd - pStart + 1); // set new length
	if (pStart != m_String) /* need to move string back */
	{
		char *pString = m_String;
		while (pString != pEnd)
			*pString++ = *pStart++;
		*pString = '\0';
	}
	else
		*++pEnd = '\0';	/* just set nullterminator */

	return *this;
}

/* to be done */
FoxString& FoxString::LTrim(char cParseChar)
{
	assert(cParseChar != '\0');
	if (m_String == 0 || Len() == 0)
		return *this;

	char *pStart = m_String;
	while (*pStart == cParseChar) pStart++; /* skip over spaces at beginning of string */

	if (pStart != m_String) /* there were spaces on the left side */
	{
		unsigned int nBytes = pStart - m_String;
		m_Value.ev_length -= nBytes;
		memmove(m_String, pStart, Len() + 1);
	}
	return *this;
}

FoxString& FoxString::RTrim(char cParseChar)
{
	assert(cParseChar != '\0');
	if (m_String == 0 || Len() == 0)
		return *this;

	char *pEnd = m_String + Len();
	while (*--pEnd == cParseChar); /* skip back over spaces at end of string */

	Len(pEnd - m_String + 1);
	m_String[Len()] = '\0';
	return *this;
}

FoxString& FoxString::Lower()
{
	if (m_String)
		_strlwr(m_String);
	return *this;
}

FoxString& FoxString::Upper()
{
	if (m_String)
		_strupr(m_String);
	return *this;
}

FoxString& FoxString::Prepend(const char *pString)
{
	unsigned int nLen;
	nLen = strlen(pString);
	Size(nLen + m_BufferSize);
	memmove(m_String + nLen, m_String, Len());
	memcpy(m_String, pString, nLen);
	m_Value.ev_length += nLen;
	return *this;
}

FoxString& FoxString::SubStr(unsigned int nStartPos, unsigned int nLen)
{
	assert(m_String);
	unsigned int nNewLen;

	if (nStartPos > 0)
		nStartPos--;

	if (nLen == -1)
	{
		if (nStartPos == 0);
		else if (nStartPos < Len())
		{
			nNewLen = Len() - nStartPos;
			memmove(m_String, m_String + nStartPos, nNewLen);
			m_String[nNewLen] = '\0';
			Len(nNewLen);
		}
		else
		{
			m_String[0] = '\0';
			Len(0);
		}
	}
	else
	{
		if (nStartPos == 0)
		{
			if (nLen < Len())
			{
				m_String[nLen] = '\0';
				Len(nLen);
			}
		}
		else if (nStartPos > Len())
		{
			m_String = '\0';
			Len(0);
		}
		else
		{
			if (nStartPos + nLen > Len())
				nNewLen = Len() - nStartPos;	
			else
				nNewLen = nLen;
			memmove(m_String, m_String + nStartPos, nNewLen);
			m_String[nNewLen] = '\0';
			Len(nNewLen);
		}
	}
	return * this;
}

FoxString& FoxString::Strtran(FoxString &sSearchFor, FoxString &sReplacement)
{
	if (sSearchFor.Len() > Len())
		return *this;

	if (sSearchFor.Len() >= sReplacement.Len())
	{
		//strstr(
	}
	else if (sSearchFor.Len() < sReplacement.Len())
	{
		char *pSearchIn = m_String, *pSearchFor = sSearchFor;
		int nCount = 0;

		while ((pSearchIn = strstr(pSearchIn,pSearchFor)))
		{
			nCount++;
			pSearchIn += sSearchFor.Len();
		}

		if (nCount == 0)
			return *this;

		FoxString sBuffer(Len() + nCount * (sReplacement.Len() - sSearchFor.Len()));
		char *pBuffer = sBuffer;
	}

	return *this;
}


FoxString& FoxString::Replicate(char *pString, unsigned int nCount)
{
	int nStrLen = strlen(pString);
	Size((nStrLen * nCount) + 1);
	char *pPtr = m_String;
	for(unsigned int xj = 1; xj <= nCount; xj++)
	{
		memcpy(pPtr,pString,nStrLen);
		pPtr += nStrLen;
	}
	*++pPtr = '\0';
	return *this;
}

unsigned int FoxString::At(char cSearchFor, unsigned int nOccurence, unsigned int nMax) const
{
	assert(m_String);
	char *pSearch = m_String;
	nMax = max(nMax, Len());

	for (unsigned int nPos = 1; nPos <= nMax; nPos++)
	{
		if (*pSearch == cSearchFor)
		{
			if (--nOccurence == 0)
				return nPos;
		}
		pSearch++;
	}
	return 0;
}

unsigned int FoxString::Rat(char cSearchFor, unsigned int nOccurence, unsigned int nMax) const
{
	assert(m_String);
	if (Len() == 0)
		return 0;

	char *pSearch = m_String + Len() - 1;
	nMax = max(nMax, Len());

	for (unsigned int nPos = nMax; nPos >= 0; nPos--)
	{
		if (*pSearch == cSearchFor)
		{
			if (--nOccurence == 0)
				return nPos;
		}
		pSearch--;
	}
	return 0;
}

unsigned int FoxString::GetWordCount(const char pSeperator) const
{
	assert(m_String);
	unsigned int nTokens = 1;
	const char *pString = m_String;
	while (*pString)
	{
		if (*pString++ == pSeperator)
			nTokens++;
	}
	if (pString != m_String)
        return nTokens;
	else
		return 0;
}

unsigned int FoxString::Expand(int nSize)
{
	m_BufferSize += nSize;
	if (m_Value.ev_handle)
	{
		UnlockHandle();
		SetHandleSize(m_BufferSize);
	}
	else
		AllocHandle(m_BufferSize);

	LockHandle();
	m_String = HandleToPtr();
	return m_BufferSize;
}

FoxString& FoxString::NullTerminate()
{
	assert(m_Value.ev_handle);
	if (m_BufferSize > Len())
		m_String[Len()] = '\0';
	else
	{
		UnlockHandle();
		FoxValue::NullTerminate();
		LockHandle();
		m_String = HandleToPtr();
		m_BufferSize++;
	}
	return *this;
}

FoxString& FoxString::ExtendBuffer(unsigned int nNewMinBufferSize)
{
	if (m_BufferSize >= nNewMinBufferSize)
		return *this;

	m_BufferSize = nNewMinBufferSize;
	if (m_Value.ev_handle)
	{
		UnlockHandle();
		SetHandleSize(m_BufferSize);
	}
	else
		AllocHandle(m_BufferSize);

	LockHandle();
	m_String = HandleToPtr();
	return *this;
}

FoxString& FoxString::Fullpath()
{
	assert(m_String);

	Value vFullpath = {'0'};
	char aBuffer[VFP2C_MAX_CALLBACKFUNCTION];

	_snprintf(aBuffer,VFP2C_MAX_CALLBACKFUNCTION,"FULLPATH(\"%s\")+CHR(0)",m_String);
	::Evaluate(vFullpath,aBuffer);

	return Attach(vFullpath);
}

bool FoxString::ICompare(char *pString) const
{
	assert(m_String && pString);
	return stricmp(m_String,pString) == 0;
}

BSTR FoxString::ToBSTR() const
{
	DWORD dwLen = Len();
	BSTR pBstr = SysAllocStringByteLen(0,dwLen * 2);

	if (pBstr == 0)
		throw E_INSUFMEMORY;

	int nChars = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,m_String,dwLen,pBstr,dwLen);
	if (!nChars)
	{
		SaveWin32Error("MultiByteToWideChar", GetLastError());
		SysFreeString(pBstr);
		throw E_APIERROR;
	}
	return pBstr;
}

SAFEARRAY* FoxString::ToU1SafeArray() const
{
	HRESULT hr;
	SAFEARRAY *pArray;
	unsigned char *pData;
	pArray = SafeArrayCreateVector(VT_UI1,0,Len());
	if (pArray == 0)
		throw E_INSUFMEMORY;
    if (hr = SafeArrayAccessData(pArray,(void**)&pData) != S_OK)
	{
		SaveWin32Error("SafeArrayAccessData", hr);
		SafeArrayDestroy(pArray);
		throw E_APIERROR;
	}
	memcpy(pData,m_String,Len());
	if (hr = SafeArrayUnaccessData(pArray) != S_OK)
	{
		SaveWin32Error("SafeArrayUnaccessData", hr);
		SafeArrayDestroy(pArray);
		throw E_APIERROR;
	}
	return pArray;
}

FoxString& FoxString::Attach(Value &pValue, unsigned int nExpand)
{
	assert(pValue.ev_type == 'C');

	if (nExpand > 0)
	{
		if (!ExpandValue(pValue, nExpand))
		{
			::FreeHandle(pValue);
			throw E_INSUFMEMORY;
		}
	}

	Release();

	m_Value.ev_handle = pValue.ev_handle;
	m_Value.ev_width = pValue.ev_width;
	LockHandle();
	m_String = HandleToPtr();
	m_Value.ev_length = pValue.ev_length;
	m_BufferSize = pValue.ev_length + nExpand;
	return *this;
}

void FoxString::Detach()
{
	UnlockHandle();
	m_BufferSize = m_Value.ev_length = m_Value.ev_handle = 0;
	m_String = 0;
}

void FoxString::Detach(Value &pValue)
{
	pValue.ev_type = 'C';
	pValue.ev_handle = m_Value.ev_handle;
	pValue.ev_length = m_Value.ev_length;
	pValue.ev_width = m_Value.ev_width;
	UnlockHandle();
	m_BufferSize = m_Value.ev_length = m_Value.ev_handle = 0;
	m_String = 0;
}

/* Operator overloading */
FoxString& FoxString::operator=(Locator &pLoc)
{
	Release();
	int nErrorNo;
	if (nErrorNo = _Load(&pLoc, &m_Value))
		throw nErrorNo;
	return *this;	
}

FoxString& FoxString::operator=(FoxValue &pValue)
{
	if (pValue.Vartype() != 'C')
		throw E_TYPECONFLICT;

	unsigned int nSize = pValue.GetHandleSize(); 
	if (Size() < nSize)
		Size(nSize);

	Len(::Len(pValue));
	Binary(pValue->ev_width == 1);
	LockHandle();
    m_String = HandleToPtr();
	memcpy(m_String, pValue.HandleToPtr(), Len());
	return *this;
}

FoxString& FoxString::operator<<(FoxObject &pObject)
{
	Release();
	int nErrorNo;
	if (nErrorNo = _GetObjectProperty(&m_Value, pObject, pObject.Property()))
		throw nErrorNo;
	if (Vartype() == 'C')
	{
		m_BufferSize = Len();
		LockHandle();
		m_String = HandleToPtr();
	}
	else if (Vartype() == 'O')
	{
        FreeObject();
		throw E_TYPECONFLICT;
	}
	else
		throw E_TYPECONFLICT;
	return *this;
}

FoxString& FoxString::operator=(FoxString &pString)
{
	if (&pString == this)
		return *this;

	if (m_BufferSize < pString.Size())
		Size(pString.Size());

	memcpy(m_String, pString, pString.Size());
	return *this;
}

FoxString& FoxString::operator=(char *pString)
{
	if (pString)
	{
		unsigned int nLen = strlen(pString) + 1;
		if (m_BufferSize < nLen)
			Expand(nLen - m_BufferSize);
		memcpy(m_String, pString, nLen);
		Len(nLen - 1);
	}
	else
	{
		Len(0);
		if (m_String)
			*m_String = '\0';
	}
	return *this;
}

FoxString& FoxString::operator=(wchar_t *pWString)
{
	unsigned int nLen;
	int nChars;

	if (pWString)
	{
		nLen = wcslen(pWString) + 1;
		if (m_BufferSize < nLen)
			Size(nLen);

		if (nLen == 1)
		{
			m_Value.ev_length = 0;
			m_String[0] = '\0';
		}
		else
		{
			nChars = WideCharToMultiByte(CP_ACP, 0, pWString, nLen-1, m_String, m_BufferSize, 0, 0);
			if (!nChars)
			{
				SaveWin32Error("MultiByteToWideChar", GetLastError());
				throw E_APIERROR;
			}
			m_Value.ev_length = nChars;
			m_String[nChars] = '\0';
		}
	}
	else
	{
		m_Value.ev_length = 0;
		if (m_String)
			*m_String = '\0';
	}
	return *this;
}

FoxString& FoxString::operator+=(const char *pString)
{
	unsigned int nLen = strlen(pString) + 1;
	int nDiff = m_BufferSize - (Len() + nLen);
	if (nDiff < 0)
		Expand(m_BufferSize + (-nDiff));
	memcpy(m_String + Len(), pString, nLen);
	m_Value.ev_length += (nLen - 1);
	return *this;
}

FoxString& FoxString::operator+=(FoxString &pString)
{
	unsigned int nLen = pString.Len() + 1;
	int nDiff = m_BufferSize - (Len() + nLen);
	if (nDiff < 0)
		Expand(m_BufferSize + (-nDiff));
	memcpy(m_String + Len(), pString, nLen);
	m_Value.ev_length += (nLen - 1);
	return *this;
}

FoxString& FoxString::operator+=(const char pChar)
{
	if ((Len() + 1) > m_BufferSize)
		Expand(1);
	char *pTmp = m_String + Len();
	*pTmp++ = pChar;
	*pTmp = '\0';
	m_Value.ev_length++;
	return *this;
} 

bool FoxString::operator==(const char *pString) const
{
	assert(m_String && pString);
	return strcmp(m_String, pString) == 0;
}

bool FoxString::operator==(FoxString &pString) const
{
	if (&pString == this)
		return true;
	else if (Len() != pString.Len())
		return false;
	else
		return memcmp(m_String, pString, Len()) == 0;
}

/* FoxWString */
FoxWString::FoxWString(Value &pVal)
{
	if (pVal.ev_length > 0)
	{
		DWORD dwLength = pVal.ev_length + 1;
		DWORD nWChars;

		m_String = new wchar_t[dwLength];
		if (m_String == 0)
			throw E_INSUFMEMORY;
		
		char *pString = ::HandleToPtr(pVal);
		nWChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,pString,pVal.ev_length,m_String,dwLength);
		if (!nWChars)
		{
			SaveWin32Error("MultiByteToWideChar", GetLastError());
			::UnlockHandle(pVal);
			throw E_APIERROR;
		}
		m_String[pVal.ev_length] = L'\0'; // nullterminate
	}
	else
		m_String = 0;
}

FoxWString::FoxWString(ParamBlk *pParms, int nParmNo)
{
	// if parameter count is equal or greater than the parameter we want
	if (pParms->pCount >= nParmNo)
	{
    	Value *pVal = &pParms->p[nParmNo-1].val;
		if (pVal->ev_type == 'C' && pVal->ev_length > 0)
		{
			DWORD dwLength = pVal->ev_length + 1;
			DWORD nWChars;

			m_String = new wchar_t[dwLength];
			if (m_String == 0)
				throw E_INSUFMEMORY;

			char *pString = ::HandleToPtr(pVal);
			nWChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pString, pVal->ev_length, m_String, dwLength);
			if (!nWChars)
			{
				SaveWin32Error("MultiByteToWideChar", GetLastError());
				throw E_APIERROR;
			}
			m_String[pVal->ev_length] = L'\0'; // nullterminate
			return;
		}
	}
	// else
	m_String = 0;
}

FoxWString::FoxWString(ParamBlk *pParms, int nParmNo, char cTypeCheck)
{
	// if parameter count is equal or greater than the parameter we want
	if (pParms->pCount >= nParmNo)
	{
    	Value *pVal = &pParms->p[nParmNo-1].val;
		if (pVal->ev_type == 'C')
		{
			if (pVal->ev_length > 0)
			{
				DWORD dwLength = pVal->ev_length + 1;
				DWORD nWChars;

				m_String = new wchar_t[dwLength];
				if (m_String == 0)
					throw E_INSUFMEMORY;

				char *pString = ::HandleToPtr(pVal);
				nWChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pString, pVal->ev_length, m_String, dwLength);
				if (!nWChars)
				{
					SaveWin32Error("MultiByteToWideChar", GetLastError());
					throw E_APIERROR;
				}
				m_String[pVal->ev_length] = L'\0'; // nullterminate
				return;
			}
		}
		else if (pVal->ev_type != cTypeCheck)
			throw E_INVALIDPARAMS;
	}
	// else
	m_String = 0;
}

FoxWString::FoxWString(FoxString& pString)
{
	DWORD dwLength = pString.Len();
	if (dwLength > 0)
	{
		DWORD nWChars;
		m_String = new wchar_t[dwLength+1];
		if (m_String == 0)
			throw E_INSUFMEMORY;
		
		nWChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pString, dwLength, m_String, dwLength);
		if (!nWChars)
		{
			SaveWin32Error("MultiByteToWideChar", GetLastError());
			throw E_APIERROR;
		}
		m_String[dwLength] = L'\0'; // nullterminate
	}
	else
		m_String = 0;
}

FoxWString::~FoxWString()
{
	if (m_String)
		delete[] m_String;
}

wchar_t* FoxWString::Duplicate()
{
	DWORD dwLen = wcslen(m_String) + 1;

	wchar_t* pNewString = new wchar_t[dwLen];
	if (pNewString == 0)
		throw E_INSUFMEMORY;

	memcpy(pNewString,m_String,dwLen*2);
	return pNewString;
}

wchar_t* FoxWString::Detach()
{
	wchar_t* pWStr = m_String;
	m_String = 0;
	return pWStr;
}

FoxWString& FoxWString::operator=(char *pString)
{
	DWORD dwLength = strlen(pString);

	if (m_String != 0)
		delete[] m_String;

	// nullterminate
	m_String = new wchar_t[dwLength+1];
	if (m_String == 0)
		throw E_INSUFMEMORY;

	int nChars = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,pString,dwLength,m_String,dwLength);
	if (!nChars)
	{
		SaveWin32Error("MultiByteToWideChar", GetLastError());
		throw E_APIERROR;
	}
	m_String[dwLength] = L'\0'; // nullterminate
	return *this;
}

FoxWString& FoxWString::operator=(FoxString& pString)
{
	DWORD dwLength = pString.Len();

	if (m_String != 0)
		delete[] m_String;

	m_String = new wchar_t[dwLength+1];
	if (m_String == 0)
		throw E_INSUFMEMORY;

	int nChars = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,pString,dwLength,m_String,dwLength);
	if (!nChars)
	{
		SaveWin32Error("MultiByteToWideChar", GetLastError());
		throw E_APIERROR;
	}
	m_String[dwLength] = L'\0'; // nullterminate
	return *this;
}

/* FoxDate */
FoxDate::FoxDate(Value &pVal) : FoxValue('D')
{
	assert(::Vartype(pVal) == 'D');
	m_Value.ev_real = pVal.ev_real;
}

FoxDate::FoxDate(SYSTEMTIME &sTime) : FoxValue('D')
{
	SystemTimeToDate(sTime);
}

FoxDate::FoxDate(FILETIME &sTime) : FoxValue('D')
{
	FileTimeToDate(sTime);
}

FoxDate& FoxDate::operator=(const SYSTEMTIME &sTime)
{
	SystemTimeToDate(sTime);
	return *this;
}

FoxDate& FoxDate::operator=(const FILETIME &sTime)
{
	FileTimeToDate(sTime);
	return *this;
}

FoxDate::operator SYSTEMTIME()
{
	SYSTEMTIME sTime;
	DateToSystemTime(sTime);
	return sTime;
}

FoxDate::operator FILETIME()
{
	FILETIME sTime;
	DateToFileTime(sTime);
	return sTime;
}

void FoxDate::SystemTimeToDate(const SYSTEMTIME &sTime)
{
	int lnA, lnY, lnM, lnJDay;
	lnA = (14 - sTime.wMonth) / 12;
	lnY = sTime.wYear + 4800 - lnA;
	lnM = sTime.wMonth + 12 * lnA - 3;
	lnJDay = sTime.wDay + (153 * lnM + 2) / 5 + lnY * 365 + lnY / 4 - lnY / 100 + lnY / 400 - 32045;
	m_Value.ev_real = ((double)lnJDay);
}

void FoxDate::FileTimeToDate(const FILETIME &sTime)
{
	// FILETIME base: Januar 1 1601 | C = 0 | FoxPro = 2305814.0
	// 86400 secs a day, 10000000 "100 nanosecond intervals" in one second
    LARGE_INTEGER pTime;
	pTime.LowPart = sTime.dwLowDateTime;
	pTime.HighPart = sTime.dwHighDateTime;

	if (pTime.QuadPart > MAXVFPFILETIME) //if bigger than max DATETIME - 9999/12/12 23:59:59
		pTime.QuadPart = MAXVFPFILETIME; //set to max date ..
	else if (pTime.QuadPart == 0) // empty Filetime?
		m_Value.ev_real = 0.0;
	else
	{
		// gives us seconds since 1601/01/01
		pTime.QuadPart /= NANOINTERVALSPERSECOND;
		// 1601/01/01 + number of seconds / 86400 (= number of days)
		m_Value.ev_real = VFPFILETIMEBASE + (double)(pTime.QuadPart / SECONDSPERDAY);
	}
}

void FoxDate::DateToSystemTime(SYSTEMTIME &sTime)
{
	int lnA, lnB, lnC, lnD, lnE, lnM;
	DWORD lnDays;

	lnDays = static_cast<DWORD>(floor(m_Value.ev_real));

	lnA = lnDays + 32044;
	lnB = (4 * lnA + 3) / 146097;
	lnC = lnA - (lnB * 146097) / 4;

	lnD = (4 * lnC + 3) / 1461;
	lnE = lnC - (1461 * lnD) / 4;
	lnM = (5 * lnE + 2) / 153;
	
	sTime.wDay = (WORD) lnE - (153 * lnM + 2) / 5 + 1;
	sTime.wMonth = (WORD) lnM + 3 - 12 * (lnM / 10);
	sTime.wYear = (WORD) lnB * 100 + lnD - 4800 + lnM / 10;

	sTime.wHour = 0;
	sTime.wMinute = 0;
	sTime.wSecond = 0;
	sTime.wMilliseconds = 0;
}

void FoxDate::DateToFileTime(FILETIME &sTime)
{
	LARGE_INTEGER nFileTime;
	double dDateTime;

	if (m_Value.ev_real >= VFPFILETIMEBASE)
		dDateTime = floor(m_Value.ev_real); // get absolute value
	else if (m_Value.ev_real == 0.0) // if empty date .. set filetime to zero
	{
		sTime.dwLowDateTime = 0;
		sTime.dwHighDateTime = 0;
		return;
	}
	else
		dDateTime = VFPFILETIMEBASE; // if before 1601/01/01 00:00:00 set to 1601/01/01 ..

	nFileTime.QuadPart = ((LONGLONG)(dDateTime - VFPFILETIMEBASE)) * NANOINTERVALSPERDAY;
	sTime.dwLowDateTime = nFileTime.LowPart;
	sTime.dwHighDateTime = nFileTime.HighPart;	
}


/* FoxDateTime */
FoxDateTime::FoxDateTime(Value &pVal) : FoxValue('T')
{
	assert(::Vartype(pVal) == 'T');
	m_Value.ev_real = pVal.ev_real;
}

FoxDateTime::FoxDateTime(SYSTEMTIME &sTime) : FoxValue('T')
{
	SystemTimeToDateTime(sTime);
}

FoxDateTime::FoxDateTime(FILETIME &sTime) : FoxValue('T')
{
	FileTimeToDateTime(sTime);
}

FoxDateTime::FoxDateTime(double dTime) : FoxValue('T')
{
	m_Value.ev_real = dTime;
}

FoxDateTime& FoxDateTime::operator=(const SYSTEMTIME &sTime)
{
	SystemTimeToDateTime(sTime);
	return *this;
}

FoxDateTime& FoxDateTime::operator=(const FILETIME &sTime)
{
	FileTimeToDateTime(sTime);
	return *this;
}

FoxDateTime::operator SYSTEMTIME()
{
	SYSTEMTIME sTime;
	DateTimeToSystemTime(sTime);
	return sTime;
}

FoxDateTime::operator FILETIME()
{
	FILETIME sTime;
	DateTimeToFileTime(sTime);
	return sTime;
}

FoxDateTime& FoxDateTime::ToUTC()
{ 
	if (m_Value.ev_real != 0.0)
		m_Value.ev_real += TimeZoneInfo::GetTsi().Bias;
	return *this;
}

FoxDateTime& FoxDateTime::ToLocal()
{
	if (m_Value.ev_real != 0.0)
		m_Value.ev_real -= TimeZoneInfo::GetTsi().Bias;
	return *this;
}

void FoxDateTime::SystemTimeToDateTime(const SYSTEMTIME &sTime)
{
	int lnA, lnY, lnM, lnJDay, lnSeconds;
	lnA = (14 - sTime.wMonth) / 12;
	lnY = sTime.wYear + 4800 - lnA;
	lnM = sTime.wMonth + 12 * lnA - 3;
	lnJDay = sTime.wDay + (153 * lnM + 2) / 5 + lnY * 365 + lnY / 4 - lnY / 100 + lnY / 400 - 32045;
	lnSeconds = sTime.wHour * 3600 + sTime.wMinute * 60 + sTime.wSecond;
	m_Value.ev_real = ((double)lnJDay) + ((double)lnSeconds / SECONDSPERDAY);
}

void FoxDateTime::FileTimeToDateTime(const FILETIME &sTime)
{
	// FILETIME base: Januar 1 1601 | C = 0 | FoxPro = 2305814.0
	// 86400 secs a day, 10000000 "100 nanosecond intervals" in one second
    LARGE_INTEGER pTime;
	pTime.LowPart = sTime.dwLowDateTime;
	pTime.HighPart = sTime.dwHighDateTime;

	if (pTime.QuadPart > MAXVFPFILETIME) //if bigger than max DATETIME - 9999/12/12 23:59:59
		pTime.QuadPart = MAXVFPFILETIME; //set to max date ..
	else if (pTime.QuadPart == 0) // empty Filetime?
		m_Value.ev_real = 0.0;
	else
	{
		// gives us seconds since 1601/01/01
		pTime.QuadPart /= NANOINTERVALSPERSECOND;
		m_Value.ev_real = VFPFILETIMEBASE + (double)(pTime.QuadPart / SECONDSPERDAY);
		m_Value.ev_real += ((double)(pTime.QuadPart % SECONDSPERDAY)) / SECONDSPERDAY; 
	}
}

void FoxDateTime::DateTimeToSystemTime(SYSTEMTIME &sTime)
{
	int lnA, lnB, lnC, lnD, lnE, lnM;
	DWORD lnDays, lnSecs;
	double dDays, dSecs;

	dSecs = modf(m_Value.ev_real,&dDays);
	lnDays = static_cast<DWORD>(dDays);

	lnA = lnDays + 32044;
	lnB = (4 * lnA + 3) / 146097;
	lnC = lnA - (lnB * 146097) / 4;

	lnD = (4 * lnC + 3) / 1461;
	lnE = lnC - (1461 * lnD) / 4;
	lnM = (5 * lnE + 2) / 153;
	
	sTime.wDay = (WORD) lnE - (153 * lnM + 2) / 5 + 1;
	sTime.wMonth = (WORD) lnM + 3 - 12 * (lnM / 10);
	sTime.wYear = (WORD) lnB * 100 + lnD - 4800 + lnM / 10;

	lnSecs = (int)floor(dSecs * 86400.0 + 0.1);
	sTime.wHour = static_cast<WORD>(lnSecs / 3600);
	lnSecs %= 3600;
	sTime.wMinute = (WORD)(lnSecs / 60);
	lnSecs %= 60;
	sTime.wSecond = (WORD)lnSecs;

	sTime.wDayOfWeek = (WORD)((lnDays + 1) % 7);
	sTime.wMilliseconds = 0; // FoxPro's datetime doesn't have milliseconds .. so just set to zero
}

void FoxDateTime::DateTimeToFileTime(FILETIME &sTime)
{
	LARGE_INTEGER nFileTime;
	double dDays, dSecs, dDateTime;

	if (m_Value.ev_real >= VFPFILETIMEBASE)
		dDateTime = m_Value.ev_real;
	else if (m_Value.ev_real == 0.0) // if empty date .. set filetime to zero
	{
		sTime.dwLowDateTime = 0;
		sTime.dwHighDateTime = 0;
		return;
	}
	else
		dDateTime = VFPFILETIMEBASE; // if before 1601/01/01 00:00:00 set to 1601/01/01 ..

	dSecs = modf(dDateTime,&dDays); // get absolute value and fractional part
	// cause double arithmetic isn't 100% accurate we have to round down to the nearest integer value
	dSecs = floor(dSecs * SECONDSPERDAY + 0.1);
	// + 0.1 cause we may get for example 34.9999899 after 0.xxxx * SECONDSPERDAY,
	// which really stands for 35 seconds after midnigth
	nFileTime.QuadPart = ((LONGLONG)(dDays - VFPFILETIMEBASE)) * NANOINTERVALSPERDAY +
						((LONGLONG)dSecs) * NANOINTERVALSPERSECOND;
	sTime.dwLowDateTime = nFileTime.LowPart;
	sTime.dwHighDateTime = nFileTime.HighPart;	
}

/* FoxDateTimeLiteral */
void FoxDateTimeLiteral::Convert(SYSTEMTIME &sTime, bool bToLocal)
{
	FILETIME sFileTime, sLocalFTime;
	SYSTEMTIME sSysTime;

	if (bToLocal)
	{
		if (!SystemTimeToFileTime(&sTime,&sFileTime))
		{
			SaveWin32Error("SystemTimeToFileTime", GetLastError());
			throw E_APIERROR;
		}
		if (!FileTimeToLocalFileTime(&sFileTime,&sLocalFTime))
		{
			SaveWin32Error("FileTimeToLocalFileTime", GetLastError());
			throw E_APIERROR;
		}
		if (!FileTimeToSystemTime(&sLocalFTime,&sSysTime))
		{
			SaveWin32Error("FileTimeToSystemTime", GetLastError());
			throw E_APIERROR;
		}

		if (sSysTime.wYear > 0 && sSysTime.wYear < 10000)
		{
			sprintf(m_Literal,"{^%04hu-%02hu-%02hu %02hu:%02hu:%02hu}",
			sSysTime.wYear,sSysTime.wMonth,sSysTime.wDay,sSysTime.wHour,sSysTime.wMinute,sSysTime.wSecond);
		}
		else
			strcpy(m_Literal,"{ ::}");

	}
	else
	{
		if (sTime.wYear > 0 && sTime.wYear < 10000)
		{
			sprintf(m_Literal,"{^%04hu-%02hu-%02hu %02hu:%02hu:%02hu}",
			sTime.wYear,sTime.wMonth,sTime.wDay,sTime.wHour,sTime.wMinute,sTime.wSecond);
		}
		else
			strcpy(m_Literal,"{ ::}");
	}
}

void FoxDateTimeLiteral::Convert(FILETIME &sTime, bool bToLocal)
{
	SYSTEMTIME sSysTime;
	FILETIME sFileTime;

	if (bToLocal)
	{
		if (!FileTimeToLocalFileTime(&sTime,&sFileTime))
		{
			SaveWin32Error("FileTimeToLocalFileTime", GetLastError());
			throw E_APIERROR;
		}
		if (!FileTimeToSystemTime(&sFileTime,&sSysTime))
		{
			SaveWin32Error("FileTimeToSystemTime", GetLastError());
			throw E_APIERROR;
		}
	}
	else if (!FileTimeToSystemTime(&sTime,&sSysTime))
	{
		SaveWin32Error("FileTimeToSystemTime", GetLastError());
		throw E_APIERROR;
	}

	if (sSysTime.wYear > 0 && sSysTime.wYear < 10000)
	{
		sprintf(m_Literal,"{^%04hu-%02hu-%02hu %02hu:%02hu:%02hu}",
		sSysTime.wYear,sSysTime.wMonth,sSysTime.wDay,sSysTime.wHour,sSysTime.wMinute,sSysTime.wSecond);
	}
	else
		strcpy(m_Literal,"{ ::}");
}

FoxObject::FoxObject(Value &pVal) : FoxValue('O'), m_Property(0), m_ParameterRef(true)
{
	assert(::Vartype(pVal) == 'O');
	m_Value.ev_object = pVal.ev_object;
}

FoxObject::FoxObject(ParamBlk *parm, int nParmNo) : m_Property(0)
{

	if (parm->pCount >= nParmNo && parm->p[nParmNo-1].val.ev_type == 'O')
	{
		m_Value.ev_type = 'O';
		m_Value.ev_object = parm->p[nParmNo-1].val.ev_object;
		m_ParameterRef = true;
	}
	else
	{
		m_Value.ev_type = '0';
		m_Value.ev_object = 0;
		m_ParameterRef = false;
	}
}

FoxObject::FoxObject(char *pExpression) : m_Property(0), m_ParameterRef(false)
{
	Evaluate(pExpression);
}

FoxObject::~FoxObject()
{
	if (m_Value.ev_object)
	{
		UnlockObject();
		if (!m_ParameterRef)
			FreeObject();
	}
	m_Value.ev_type = '0';
}

void FoxObject::Release()
{
	if (m_Value.ev_object)
	{
		UnlockObject();
		if (!m_ParameterRef)
			FreeObject();
		else
			m_Value.ev_object = 0;
	}
	m_ParameterRef = false;
	m_Value.ev_type = '0';
}

FoxObject& FoxObject::NewObject(char *pClass)
{
	Release();
	char aCommand[VFP2C_MAX_CALLBACKFUNCTION];
	sprintfex(aCommand,"CREATEOBJECT('%S')",pClass);
	Evaluate(aCommand);
	return *this;
}

FoxObject& FoxObject::EmptyObject()
{
	Release();
	if (CFoxVersion::MajorVersion() >= 8)
		Evaluate("CREATEOBJECT('Empty')");
	else
		Evaluate("CREATEOBJECT('Relation')");
	return *this;
}

/* FoxMemo */
FoxMemo::FoxMemo() : m_File(0), m_Location(0), m_pContent(0)
{
	memset(&m_Loc,0,sizeof(Locator));
}

FoxMemo::FoxMemo(ParamBlk *parm, int nParmNo) : m_pContent(0)
{
	if (parm->pCount >= nParmNo && IsMemoRef(parm->p[nParmNo-1].loc))
	{
		m_Loc = parm->p[nParmNo-1].loc;

		m_Location = _FindMemo(&m_Loc);
		if (m_Location < 0)
			throw E_FIELDNOTFOUND;

		m_File = _MemoChan(m_Loc.l_where);
		if (m_File == -1)
		{
			SaveCustomError("_MemoChan", "Function failed for workarea %I.", m_Loc.l_where);
			throw E_APIERROR;
		}

		m_Size = _MemoSize(&m_Loc);
		if (m_Size < 0)
			throw m_Size;
	}
	else
	{
		m_File = 0;
		m_Location = 0;
		m_Size = 0;
		memset(&m_Loc, 0 ,sizeof(Locator));
	}
}

FoxMemo::FoxMemo(Locator &pLoc) : m_pContent(0)
{
	m_Loc = pLoc;

	m_Location = _FindMemo(&m_Loc);
	if (m_Location < 0)
		throw E_FIELDNOTFOUND;

	m_File = _MemoChan(m_Loc.l_where);
	if (m_File == -1)
	{
		SaveCustomError("_MemoChan", "Function failed for workarea %I.", m_Loc.l_where);
		throw E_APIERROR;
	}

	m_Size = _MemoSize(&m_Loc);
	if (m_Size < 0)
		throw m_Size;
}

FoxMemo::~FoxMemo()
{
	if (m_pContent)
		delete[] m_pContent;
}

void FoxMemo::Alloc(unsigned int nSize)
{
	m_Location = _AllocMemo(&m_Loc,nSize);
	if (m_Location == -1)
	{
		SaveCustomError("_AllocMemo","Function failed.");
		throw E_APIERROR;
	}
}

void FoxMemo::Append(char *pData, unsigned int nLen)
{
	_FSeek(m_File,m_Location,FS_FROMBOF);
	if (_FWrite(m_File,pData,nLen) != nLen)
		throw _FError();
	m_Location += nLen;
}

char* FoxMemo::Read(unsigned int &nLen)
{
	unsigned int nBytes = nLen == 0 ? m_Size : nLen;

	if (m_pContent)
		delete[] m_pContent;

	m_pContent = new char[nBytes];
	if (m_pContent == 0)
		throw E_INSUFMEMORY;

	_FSeek(m_File, m_Location, FS_FROMBOF);
	nLen = _FRead(m_File, m_pContent, nBytes);
	return m_pContent;
}

FoxMemo& FoxMemo::operator=(FoxString &pString)
{
	/* if data is smaller or equal to 65000, we can use _DBReplace */
	if (pString.Len() <= 65000)
	{
		int nErrorNo;
		if (nErrorNo = _DBReplace(&m_Loc,pString))
			throw nErrorNo;
	}
	else
	{
		Alloc(pString.Len());
		_FSeek(m_File,m_Location,FS_FROMBOF);
		if (_FWrite(m_File,pString,pString.Len()) != pString.Len())
			throw _FError();
	}
	return *this;
}


/* FoxArray */
FoxArray::FoxArray()
{
	Init();
}

FoxArray::FoxArray(Value &pVal, unsigned int nRows, unsigned int nDims)
{
 	Init(pVal, nRows, nDims);
}

FoxArray::FoxArray(Locator &pLoc)
{
	Init(pLoc);
}

FoxArray::FoxArray(ParamBlk *parm, int nParmNo)
{
	if (parm->pCount >= nParmNo)
	{
		Parameter *pParameter = &parm->p[nParmNo-1];
		if (Vartype(pParameter) == 'C' && pParameter->val.ev_length > 0)
			Init(pParameter->val);
		else if (Vartype(pParameter) == 'R')
			Init(pParameter->loc);
		else
			Init();
	}
	else
		Init();
}

FoxArray::FoxArray(ParamBlk *parm, int nParmNo, char cTypeCheck)
{
	if (parm->pCount >= nParmNo)
	{
		Value *pVal = &parm->p[nParmNo-1].val;
		if (Vartype(pVal) == 'C' && pVal->ev_length > 0)
		{
			Init(*pVal);
			return;
		}
		else if (pVal->ev_type != cTypeCheck)
			throw E_INVALIDPARAMS;
	}
	Init();
}

void FoxArray::Init()
{
	m_Name[0] = '\0';
	m_Loc.l_NTI = 0;
	m_Loc.l_subs = 0;
	m_Loc.l_sub1 = 0;
	m_Loc.l_sub2 = 0;
	m_Rows = 0;
	m_Dims = 0;
	m_AutoGrow = 0;
}

void FoxArray::Init(Locator &pLoc)
{
	m_Name[0] = '\0';
	m_Loc = pLoc;
	m_Rows = ::ARows(m_Loc);
	m_Dims = ::ADims(m_Loc);
	m_Dims = m_Dims > 0 ? m_Dims : 1;
	m_Loc.l_subs = m_Dims > 1 ? 2 : 1;
	m_Loc.l_sub1 = 0;
	m_Loc.l_sub2 = 0;
	m_AutoGrow = 0;
}

void FoxArray::Init(Value &pVal, unsigned int nRows, unsigned int nDims)
{
	if (Vartype(pVal) != 'C' || pVal.ev_handle == 0 || pVal.ev_length > VFP_MAX_VARIABLE_NAME)
		throw E_INVALIDPARAMS;
 	memcpy(m_Name, HandleToPtr(pVal), pVal.ev_length);
 	m_Name[pVal.ev_length] = '\0';
	m_Loc.l_NTI = 0;
	m_AutoGrow = 0;
	if (nRows)
		Dimension(nRows, nDims);
	else
	{
		m_Loc.l_subs = 0;
		m_Loc.l_sub1 = 0;
		m_Loc.l_sub2 = 0;
		m_Rows = 0;
		m_Dims = 0;
	}
}

FoxArray& FoxArray::Dimension(unsigned int nRows, unsigned int nDims)
{
	if (m_Loc.l_NTI)
	{
		ReDimension(nRows,nDims);
		return *this;
	}

	if (!FindArray())
	{
		m_Loc.l_subs = nDims > 1 ? 2 : 1;
		m_Loc.l_sub1 = nRows;
		m_Loc.l_sub2 = nDims;
		NTI nVarNti = _NewVar(m_Name, &m_Loc, NV_PRIVATE);
		if (nVarNti < 0)
			throw (int)-nVarNti;
		m_Rows = nRows;
		m_Dims = nDims;
	}
	else
	{
		ReDimension(nRows, nDims);
		if (!FindArray())
			throw E_NOTANARRAY;
		m_Loc.l_subs = nDims > 1 ? 2 : 1;
		InitArray();
	}
	m_Loc.l_sub1 = 0;
	m_Loc.l_sub2 = 0;
	return *this;
}

FoxArray& FoxArray::Dimension(Value &pVal, unsigned int nRows, unsigned int nDims)
{
	if (Vartype(pVal) != 'C' || pVal.ev_handle == 0 || pVal.ev_length > VFP_MAX_VARIABLE_NAME)
		throw E_INVALIDPARAMS;
	memcpy(m_Name, HandleToPtr(pVal), pVal.ev_length);
	m_Name[pVal.ev_length] = '\0';
	return Dimension(nRows,nDims);
}

FoxArray& FoxArray::Dimension(char *pName, unsigned int nRows, unsigned int nDims)
{
	assert(pName);
	unsigned int nLen = strlen(pName);
	if (nLen > VFP_MAX_VARIABLE_NAME)
		throw E_INVALIDPARAMS;
	memcpy(m_Name, pName, nLen);
	m_Name[nLen] = '\0';
	return Dimension(nRows,nDims);
}

bool FoxArray::FindArray()
{
	NTI nVarNti = _NameTableIndex(m_Name);
	if (nVarNti == -1)
		return false;
	return _FindVar(nVarNti, -1, &m_Loc) > 0;
}

void FoxArray::InitArray()
{
	assert(m_Loc.l_NTI);
	int nErrorNo;
	USHORT tmpSubs;
	LogicalValue vFalse(false);
	tmpSubs = m_Loc.l_subs;
	m_Loc.l_subs = 0;
	if (nErrorNo = _Store(&m_Loc,&vFalse))
		throw nErrorNo;
	m_Loc.l_subs = tmpSubs;
}

void FoxArray::ReDimension(unsigned int nRows, unsigned int nDims)
{
	assert(strlen(m_Name));
	char aExeBuffer[256];
	if (nDims > 1)
		sprintfex(aExeBuffer,"DIMENSION %S[%U,%U]", m_Name, nRows, nDims);
	else
		sprintfex(aExeBuffer,"DIMENSION %S[%U]", m_Name, nRows);
	Execute(aExeBuffer);
	m_Rows = nRows;
	m_Dims = nDims;
}

unsigned int FoxArray::ALen(unsigned int &nDims)
{
	if (m_Loc.l_NTI == 0)
	{	
		if (!FindArray())
			throw E_VARIABLENOTFOUND;

		m_Rows = ::ARows(m_Loc);
		m_Dims = ::ADims(m_Loc);
		m_Loc.l_subs = m_Dims > 1 ? 2 : 1;
	}

	nDims = m_Dims;
	return m_Rows;
}

FoxCursor::~FoxCursor()
{
	if (m_pFieldLocs)
		delete[] m_pFieldLocs;
}

FoxCursor& FoxCursor::Create(char *pCursorName, char *pFields)
{
	FoxValue vValue;
	char aExeBuffer[8192];

	sprintfex(aExeBuffer, "SELECT('%S')", pCursorName);
	vValue.Evaluate(aExeBuffer);

	// if workarea == 0 the cursor does not exist
	if(vValue->ev_long == 0)
	{
		// create the cursor
		sprintfex(aExeBuffer,"CREATE CURSOR %S (%S)",pCursorName,pFields);
		Execute(aExeBuffer);

		// get the workarea
		vValue.Evaluate("SELECT(0)");
	}
	
	m_WorkArea = vValue->ev_long;

	// get fieldcount
	sprintfex(aExeBuffer, "FCOUNT(%I)", m_WorkArea);
	vValue.Evaluate(aExeBuffer);
	m_FieldCnt = vValue->ev_long;

	m_pFieldLocs = new Locator[m_FieldCnt];
	if (m_pFieldLocs == 0)
		throw E_INSUFMEMORY;

	// get locators to each field
	NTI nVarNti;
	for (unsigned int nFieldNo = 1; nFieldNo <= m_FieldCnt; nFieldNo++)
	{
		sprintfex(aExeBuffer, "FIELD(%I,%I)+CHR(0)", nFieldNo, m_WorkArea);
		vValue.Evaluate(aExeBuffer);
	
        nVarNti = _NameTableIndex(vValue.HandleToPtr());
		vValue.Release();

		if (nVarNti == -1)
			throw E_FIELDNOTFOUND;

		if (!_FindVar(nVarNti, m_WorkArea, m_pFieldLocs + (nFieldNo-1)))
			throw E_FIELDNOTFOUND;
	}
	return *this;
}

FoxCursor& FoxCursor::Attach(char *pCursorName, char *pFields)
{
	FoxValue vValue;
	char aExeBuffer[256];

	sprintfex(aExeBuffer, "SELECT('%S')", pCursorName);
	vValue.Evaluate(aExeBuffer);

	// if workarea == 0 the cursor does not exist
	if(vValue->ev_long == 0)
		throw E_ALIASNOTFOUND;

	m_WorkArea = vValue->ev_long;
	m_FieldCnt = GetWordCount(pFields, ',');
	m_pFieldLocs = new Locator[m_FieldCnt];
	if (m_pFieldLocs == 0)
		throw E_INSUFMEMORY;

	// get locators to each field
	NTI nVarNti;
	char aFieldName[VFP_MAX_COLUMN_NAME];
	for (unsigned int nFieldNo = 1; nFieldNo <= m_FieldCnt; nFieldNo++)
	{
		GetWordNumN(aFieldName, pFields, ',', nFieldNo, VFP_MAX_COLUMN_NAME);
		Alltrim(aFieldName);
        nVarNti = _NameTableIndex(aFieldName);
		if (nVarNti == -1)
			throw E_FIELDNOTFOUND;

		if (!_FindVar(nVarNti, m_WorkArea, m_pFieldLocs + (nFieldNo-1)))
			throw E_FIELDNOTFOUND;
	}
	return *this;
}

FoxCursor& FoxCursor::Attach(int nWorkArea, char *pFields)
{
	m_WorkArea = nWorkArea;
	m_FieldCnt = GetWordCount(pFields, ',');
	m_pFieldLocs = new Locator[m_FieldCnt];
	if (m_pFieldLocs == 0)
		throw E_INSUFMEMORY;

	// get locators to each field
	NTI nVarNti;
	char aFieldName[VFP_MAX_COLUMN_NAME];
	for (unsigned int nFieldNo = 1; nFieldNo <= m_FieldCnt; nFieldNo++)
	{
		GetWordNumN(aFieldName, pFields, ',', nFieldNo, VFP_MAX_COLUMN_NAME);
		Alltrim(aFieldName);
        nVarNti = _NameTableIndex(aFieldName);
		if (nVarNti == -1)
			throw E_FIELDNOTFOUND;

		if (!_FindVar(nVarNti, m_WorkArea, m_pFieldLocs + (nFieldNo-1)))
			throw E_FIELDNOTFOUND;
	}
	return *this;
}

/* FoxCStringArray */
FoxCStringArray::~FoxCStringArray()
{
	if (m_pStrings)
		delete[] m_pStrings;
	if (m_pValues)
		delete[] m_pValues;
}

void FoxCStringArray::Dimension(unsigned int nRows)
{
	assert(!m_pStrings);

	m_pStrings = new char*[nRows];
	if (m_pStrings == 0)
		throw E_INSUFMEMORY;
    ZeroMemory(m_pStrings, sizeof(char*)*nRows);

    m_pValues = new FoxValue[nRows];
	if (m_pValues == 0)
		throw E_INSUFMEMORY;
}

FoxCStringArray& FoxCStringArray::operator=(FoxArray &pArray)
{
	FoxValue *pValue;
	m_Rows = pArray.ARows();
	if (m_Rows)
	{
		Dimension(m_Rows);
		pValue = m_pValues;
		for (unsigned int xj = 0; xj < m_Rows; xj++)
		{
			*pValue = pArray(xj+1);
			if (pValue->Vartype() != 'C')
				throw E_INVALIDPARAMS;

			m_pStrings[xj] = pValue->NullTerminate().LockHandle().HandleToPtr();
			pValue++;
		}
	}
	return *this;
}

TimeZoneInfo& TimeZoneInfo::GetTsi()
{
	static TimeZoneInfo tsi;
	return tsi;
}

/* TimeZoneInfo class */
TimeZoneInfo::TimeZoneInfo() : m_Hwnd(0), m_Atom(0), Bias(0)
{
	WNDCLASSEX wndClass = {0};
	char *lpClass = "__VFP2C_TZWC";

	m_Atom = (ATOM)GetClassInfoEx(ghModule,lpClass,&wndClass);
	if (!m_Atom)
	{
		wndClass.cbSize = sizeof(WNDCLASSEX);
		wndClass.hInstance = ghModule;
		wndClass.lpfnWndProc = TimeChangeWindowProc;
		wndClass.lpszClassName = lpClass;
		m_Atom = RegisterClassEx(&wndClass);
	}

	if (m_Atom)
	{
		m_Hwnd = CreateWindowEx(0,(LPCSTR)m_Atom,0,WS_POPUP,0,0,0,0,0,0,ghModule,0);
		if (!m_Hwnd)
			AddWin32Error("CreateWindowEx", GetLastError());
	}
	else
		AddWin32Error("RegisterClassEx", GetLastError());

	Refresh();
}

TimeZoneInfo::~TimeZoneInfo()
{
	if (m_Hwnd)
		DestroyWindow(m_Hwnd);
	if (m_Atom)
		UnregisterClass((LPCSTR)m_Atom,ghModule);
}

void TimeZoneInfo::Refresh()
{
	m_CurrentZone = GetTimeZoneInformation(&m_ZoneInfo);
	if (m_CurrentZone == TIME_ZONE_ID_STANDARD || m_CurrentZone == TIME_ZONE_ID_UNKNOWN)
		Bias = ((double)m_ZoneInfo.Bias * 60) / SECONDSPERDAY;
	else if (m_CurrentZone == TIME_ZONE_ID_DAYLIGHT)
		Bias = ((double)m_ZoneInfo.Bias * 60 + m_ZoneInfo.DaylightBias * 60) / SECONDSPERDAY;
	else
		Bias = 0;
}

LRESULT _stdcall TimeZoneInfo::TimeChangeWindowProc(HWND nHwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_TIMECHANGE)
	{
		TimeZoneInfo& tsi = TimeZoneInfo::GetTsi();
		tsi.Refresh();
	}
	return DefWindowProc(nHwnd,uMsg,wParam,lParam);
}
