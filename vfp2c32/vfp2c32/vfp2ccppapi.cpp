#include <math.h>
#include <new>
#include <stdio.h>

#include "vfp2c32.h"

int CFoxVersion::m_Version = 0;
int CFoxVersion::m_MajorVersion = 0;
int CFoxVersion::m_MinorVersion = 0;

void ReturnIDispatch(void* pObject)
{
	Value vObject = { '0' };
	CStrBuilder<32> pCommand;
	pCommand.Format("SYS(3096,%I)", pObject);
	Evaluate(vObject, pCommand);
	Return(vObject);
}

void _stdcall StoreObjectRef(char *pName, NTI &nVarNti, ValueEx &sObject)
{
	LocatorEx lVar;
	if (nVarNti)
	{
		ValueEx vTmpObject;
		vTmpObject = 0;
		// FindVar(nVarNti, lVar);
		lVar.FindVar(nVarNti),
		// increment reference count by calling evaluate
		// Evaluate(vTmpObject, pName);
		vTmpObject.Evaluate(pName);
		// Store(lVar, sObject);
		lVar.Store(sObject);
		// ObjectRelease(sObject);
		sObject.ObjectRelease();
	}
	else
	{
		// nVarNti = NewVar(pName, lVar, true);
		nVarNti = lVar.NewVar(pName, true);
		// Store(lVar, sObject);
		lVar.Store(sObject);
		// ObjectRelease(sObject);
		sObject.ObjectRelease();
	}
}

/* implementation of C++ wrapper classes over FoxPro datatypes */
IDispatch* FoxValue::GetIDispatch()
{
	IDispatch* pObject;
	char* VarName = "__VFP2C32_TEMP_OBJECT";
	char* VarName2 = "__VFP2C32_TEMP_COMOBJECT";
	char* pCommand = "m.__VFP2C32_TEMP_COMOBJECT = _VFP.Eval('m.__VFP2C32_TEMP_OBJECT')";
	char* pCommand2 = "GetIDispatch(m.__VFP2C32_TEMP_COMOBJECT)";
	ValueEx pDisp;
	pDisp = 0;
	FoxVariable pFoxObject(VarName, false);
	FoxVariable pComObject(VarName2, false);
	pFoxObject() = m_Value;
	Execute(pCommand);
	::Evaluate(pDisp, pCommand2);
	pObject = pDisp.DynamicPtr<IDispatch*>();
	if (pObject)
		pObject->AddRef();
	return pObject;
}

/* FoxString */
FoxString::FoxString() : FoxValue('C')
{
	m_ParameterRef = false;
	m_BufferSize = 0;
	m_String = 0;
	m_CodePage = CP_ACP;
}

FoxString::FoxString(FoxString &pString) : FoxValue('C')
{
	m_ParameterRef = false;
	m_CodePage = pString.m_CodePage;
	m_BufferSize = pString.Size();
	if (m_BufferSize)
	{
		AllocHandle(m_BufferSize);
		Len(pString.Len());
		Binary(pString.Binary());
		m_String = HandleToPtr();
		memcpy(m_String, pString.Ptr<const void*>(), Len());
	}
	else
	{
		m_String = 0;
		Len(0);
	}
}

FoxString::FoxString(FoxParameterEx& pVal) : FoxValue('C')
{
	Attach(pVal);
}

FoxString::FoxString(FoxParameterEx& pVal, unsigned int nExpand) : FoxValue('C')
{
	Attach(pVal, nExpand);
}

FoxString::FoxString(ParamBlkEx& parm, int nParmNo) : FoxValue('C')
{
	Attach(parm, nParmNo);
}

FoxString::FoxString(ParamBlkEx& parm, int nParmNo, unsigned int nExpand) : FoxValue('C')
{
	Attach(parm, nParmNo, nExpand);
}

FoxString::FoxString(ParamBlkEx& parm, int nParmNo, FoxStringInitialization eInit)
{
	Attach(parm, nParmNo, eInit);
}

FoxString::FoxString(const CStringView pString) : FoxValue('C')
{
	m_ParameterRef = false;
	m_CodePage = CP_ACP;
	if (pString)
	{
		m_BufferSize = pString.Len + 1;
		AllocHandle(m_BufferSize);
		LockHandle();
		m_String = HandleToPtr();
		memcpy(m_String, pString.Data, pString.Len);
		m_String[pString.Len] = '\0';
		m_Value.ev_length = pString.Len;
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
	m_CodePage = CP_ACP;
	m_ParameterRef = false;
	AllocHandle(nBufferSize);
	LockHandle();
	m_String = HandleToPtr();
	m_BufferSize = nBufferSize;
	*m_String = '\0';
}

FoxString::FoxString(BSTR pString, UINT nCodePage) : FoxValue('C')
{
	m_CodePage = nCodePage;
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
	m_CodePage = CP_ACP;
	m_ParameterRef = false;
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

FoxString& FoxString::Attach(ValueEx& pValue, unsigned int nExpand)
{
	assert(pValue.Vartype() == 'C');
	if (nExpand > 0)
	{
		if (!pValue.ExpandHandleSize(nExpand))
		{
			pValue.FreeHandle();
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

FoxString& FoxString::Attach(FoxParameterEx& pVal)
{
	// nullterminate
	if (!pVal->NullTerminate())
		throw E_INSUFMEMORY;

	m_Value.ev_length = pVal->ev_length;
	m_Value.ev_width = pVal->ev_width;
	m_Value.ev_handle = pVal->ev_handle;
	m_BufferSize = pVal->ev_length + 1;
	m_ParameterRef = true;
	m_CodePage = CP_ACP;
	LockHandle();
	m_String = HandleToPtr();
	return *this;
}

FoxString& FoxString::Attach(FoxParameterEx& pVal, unsigned int nExpand)
{
	if (nExpand > 0)
	{
		if (!pVal->ExpandHandleSize(nExpand))
			throw E_INSUFMEMORY;
	}
	m_Value.ev_length = pVal->ev_length;
	m_Value.ev_width = pVal->ev_width;
	m_Value.ev_handle = pVal->ev_handle;
	m_BufferSize = pVal->ev_length + nExpand;
	m_ParameterRef = true;
	m_CodePage = CP_ACP;
	LockHandle();
	m_String = HandleToPtr();
	return *this;
}

bool FoxString::Attach(ParamBlkEx& parm, int nParmNo)
{
	m_CodePage = CP_ACP;
	// if parameter count is equal or greater than the parameter we want
	if (parm.pCount >= nParmNo)
	{
		ValueEx& pVal = parm(nParmNo);
		if (pVal.Vartype() == 'C')
		{
			// nullterminate
			if (!pVal.NullTerminate())
				throw E_INSUFMEMORY;

			m_Value.ev_handle = pVal.ev_handle;
			m_Value.ev_length = pVal.ev_length;
			m_Value.ev_width = pVal.ev_width;
			m_BufferSize = pVal.ev_length + 1;
			m_ParameterRef = true;
			LockHandle();
			m_String = HandleToPtr();
			return true;
		}
	}
	m_ParameterRef = false;
	m_BufferSize = 0;
	m_String = 0;
	return false;
}

bool FoxString::Attach(ParamBlkEx& parm, int nParmNo, unsigned int nExpand)
{
	m_CodePage = CP_ACP;
	// if parameter count is equal or greater than the parameter we want
	if (parm.pCount >= nParmNo)
	{
		ValueEx& pVal = parm(nParmNo);
		if (pVal.Vartype() == 'C')
		{
			if (nExpand > 0)
			{
				if (!pVal.ExpandHandleSize(nExpand))
					throw E_INSUFMEMORY;
			}
			m_Value.ev_handle = pVal.ev_handle;
			m_Value.ev_length = pVal.ev_length;
			m_Value.ev_width = pVal.ev_width;
			m_BufferSize = pVal.ev_length + nExpand;
			m_ParameterRef = true;
			LockHandle();
			m_String = HandleToPtr();
			return true;
		}
	}
	m_ParameterRef = false;
	m_BufferSize = 0;
	m_String = 0;
	return false;
}

bool FoxString::Attach(ParamBlkEx& parm, int nParmNo, FoxStringInitialization eInit)
{
	m_CodePage = CP_ACP;
	// if parameter count is equal or greater than the parameter we want
	if (parm.pCount >= nParmNo)
	{
		ValueEx& pVal = parm(nParmNo);
		if (pVal.Vartype() == 'C')
		{
			if (eInit == NoNullIfEmpty || pVal.ev_length)
			{
				if (!pVal.NullTerminate())
					throw E_INSUFMEMORY;

				m_Value.ev_handle = pVal.ev_handle;
				m_Value.ev_length = pVal.ev_length;
				m_Value.ev_width = pVal.ev_width;
				m_BufferSize = pVal.ev_length + 1;
				m_ParameterRef = true;
				LockHandle();
				m_String = HandleToPtr();
				return true;
			}
		}
	}
	m_ParameterRef = false;
	m_BufferSize = 0;
	m_String = 0;
	return false;
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
		pString++;		
		nStringCnt++;
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
		unsigned int nLen = Len();
		if (m_String[nLen - 1]  != '\\')
		{
			Size(nLen + 1);
			m_String[nLen] = '\\';
			m_String[nLen + 1] = '\0';
			Len(nLen);
		}
	}
	return *this;
}

FoxString& FoxString::AddBsWildcard()
{
	if (m_String && Len())
	{
		unsigned int nLen = Len();
		if (nLen > 1 && m_String[nLen - 2] == '\\' && m_String[nLen - 1] == '*')
			return *this;

		if (m_String[nLen - 1] == '\\')
		{
			Size(nLen + 2);
			m_String[nLen] = '*';
			m_String[nLen + 1] = '\0';
			Len(nLen + 1);
		}
		else
		{
			Size(nLen + 3);
			m_String[nLen] = '\\';
			m_String[nLen + 1] = '*';
			m_String[nLen + 2] = '\0';
			Len(nLen + 2);
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
#pragma warning(disable : 4996)
		_strlwr(m_String);
#pragma warning(default : 4996)
	return *this;
}

FoxString& FoxString::Upper()
{
	if (m_String)
#pragma warning(disable : 4996)
		_strupr(m_String);
#pragma warning(default : 4996)
	return *this;
}

FoxString& FoxString::Prepend(const CStringView pString)
{
	if (pString)
	{
		Size(pString.Len + m_BufferSize);
		memmove(m_String + pString.Len, m_String, Len());
		memcpy(m_String, pString.Data, pString.Len);
		m_Value.ev_length += pString.Len;
	}
	return *this;
}

FoxString& FoxString::PrependIfNotPresent(const CStringView pString)
{
	if (pString)
	{
		if (Len() < pString.Len || memcmp(pString.Data, m_String, pString.Len) != 0)
		{
			Size(pString.Len + m_BufferSize);
			memmove(m_String + pString.Len, m_String, Len());
			memcpy(m_String, pString.Data, pString.Len);
			m_Value.ev_length += pString.Len;
		}
	}
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

FoxString& FoxString::Strtran(const CStringView sSearchFor, const CStringView sReplacement)
{
	if (sSearchFor.Len > Len())
		return *this;

	if (sSearchFor.Len >= sReplacement.Len)
	{
		//strstr(
	}
	else if (sSearchFor.Len < sReplacement.Len)
	{
		char *pSearchIn = m_String, *pSearchFor = sSearchFor.Data;
		int nCount = 0;

		while ((pSearchIn = strstr(pSearchIn,pSearchFor)))
		{
			nCount++;
			pSearchIn += sSearchFor.Len;
		}

		if (nCount == 0)
			return *this;

		FoxString sBuffer(Len() + nCount * (sReplacement.Len - sSearchFor.Len));
		char *pBuffer = sBuffer;
	}

	return *this;
}


FoxString& FoxString::Format(const char* format, ...)
{
	va_list lpArgs;
	va_start(lpArgs, format);
	m_Value.ev_length = nprintfex(m_String, format, m_BufferSize, lpArgs);
	va_end(lpArgs);
	return *this;
}

FoxString& FoxString::Replicate(const CStringView pString, unsigned int nCount)
{
	if (pString)
	{
		int nNewSize = pString.Len * nCount + 1;
		Size(nNewSize);
		char* pPtr = m_String;
		for (unsigned int xj = 1; xj <= nCount; xj++)
		{
			memcpy(pPtr, pString.Data, pString.Len);
			pPtr += pString.Len;
		}
		*++pPtr = '\0';
		Len(nNewSize);
	}
	return *this;
}

unsigned int FoxString::At(char cSearchFor, unsigned int nOccurence, unsigned int nMax) const
{
	assert(m_String);
	char *pSearch = m_String;
	nMax = min(nMax, Len());

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

	unsigned int nPos = nMax;
	while(true)
	{
		if (*pSearch == cSearchFor)
		{
			if (--nOccurence == 0)
				return nPos;
		}
		if (nPos == 0)
			break;
		pSearch--;
		nPos--;
	}
	return 0;
}

CStringView FoxString::GetWordNum(unsigned int nWordnum, const char pSeperator) const
{
	unsigned int nCurrentWord = 1;
	unsigned int nLen = Len();
	char* pWordStart = m_String;
	char* pString = m_String;
	while (nLen--)
	{
		if (*pString == pSeperator)
		{
			if (nCurrentWord == nWordnum)
				return CStringView(pWordStart, pString - pWordStart);
			nCurrentWord++;
			pWordStart = pString + 1;
		}
		pString++;
	}
	if (nCurrentWord == nWordnum)
		return CStringView(pWordStart, pString - pWordStart);
	else
		return 0;
}

unsigned int FoxString::GetWordCount(const char pSeperator) const
{
	unsigned int nTokens = 1;
	unsigned int nLen = Len();
	const char *pString = m_String;
	while (nLen--)
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
	ValueEx vFullpath;
	vFullpath = 0;
	CStrBuilder<VFP2C_MAX_CALLBACKFUNCTION> pExeBuffer;
	pExeBuffer.Format("FULLPATH(\"%S\")+CHR(0)", m_String);
	::Evaluate(vFullpath, pExeBuffer);
	return Attach(vFullpath);
}

FoxString& FoxString::FileAttributesToString(DWORD dwFileAttributes)
{
	ExtendBuffer(10);
	char* pString = m_String;
	if ((dwFileAttributes & FILE_ATTRIBUTE_READONLY) > 0)
		*pString++ = 'R';
	if ((dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) > 0)
		*pString++ = 'H';
	if ((dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) > 0)
		*pString++ = 'S';
	if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0)
		*pString++ = 'D';
	if ((dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) > 0)
		*pString++ = 'A';
	if ((dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) > 0)
		*pString++ = 'T';
	if ((dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) > 0)
		*pString++ = 'P';
	if ((dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) > 0)
		*pString++ = 'L';
	if ((dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) > 0)
		*pString++ = 'C';
	if ((dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) > 0)
		*pString++ = 'O';
	if ((dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) > 0)
		*pString++ = 'I';
	if ((dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED) > 0)
		*pString++ = 'E';
	if ((dwFileAttributes & 0x80000000 /* FILE_ATTRIBUTE_FAKEDIRECTORY */) > 0)
		*pString++ = 'K';
	
	Len(pString - m_String);
	return *this;
}

bool FoxString::StringToFileAttributes(DWORD& nAttributesSet, DWORD& nAttributesClear) const
{
	nAttributesSet = 0;
	nAttributesClear = 0;
	bool bClearOrSet = false, bSet = true;
	for (unsigned long xj = 0; xj < Len(); xj++)
	{
		switch (m_String[xj])
		{
		case '+':
			bClearOrSet = true;
			bSet = true;
			break;
		case '-':
			bClearOrSet = true;
			bSet = false;
			break;
		case 'R':
		case 'r':
			if (bSet)
				nAttributesSet |= FILE_ATTRIBUTE_READONLY;
			else
				nAttributesClear |= FILE_ATTRIBUTE_READONLY;
			break;
		case 'H':
		case 'h':
			if (bSet)
				nAttributesSet |= FILE_ATTRIBUTE_HIDDEN;
			else
				nAttributesClear |= FILE_ATTRIBUTE_HIDDEN;
			break;
		case 'S':
		case 's':
			if (bSet)
				nAttributesSet |= FILE_ATTRIBUTE_SYSTEM;
			else
				nAttributesClear |= FILE_ATTRIBUTE_SYSTEM;
			break;
		case 'A':
		case 'a':
			if (bSet)
				nAttributesSet |= FILE_ATTRIBUTE_ARCHIVE;
			else
				nAttributesClear |= FILE_ATTRIBUTE_ARCHIVE;
			break;
		case 'N':
		case 'n':
			if (bSet)
				nAttributesSet |= FILE_ATTRIBUTE_NORMAL;
			else
				nAttributesClear |= FILE_ATTRIBUTE_NORMAL;
			break;
		case 'T':
		case 't':
			if (bSet)
				nAttributesSet |= FILE_ATTRIBUTE_TEMPORARY;
			else
				nAttributesClear |= FILE_ATTRIBUTE_TEMPORARY;
			break;			
		case 'O':
		case 'o':
			if (bSet)
				nAttributesSet |= FILE_ATTRIBUTE_OFFLINE;
			else
				nAttributesClear |= FILE_ATTRIBUTE_OFFLINE;
			break;
		case 'I':
		case 'i':
			if (bSet)
				nAttributesSet |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
			else
				nAttributesClear |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
			break;
		default:
			SaveCustomError("StringToFileAttributes", "Unknown file attribute letter '%s'.", m_String[xj]);
			throw E_APIERROR;
		}
	}
	return bClearOrSet;
}

bool FoxString::ICompare(CStringView pString) const
{
	if (m_String && pString.Data)
	{
		if (Len() != pString.Len)
			return false;

		unsigned char* pStr1 = reinterpret_cast<unsigned char*>(m_String);
		unsigned char* pStr2 = reinterpret_cast<unsigned char*>(pString.Data);
		unsigned char pChar1, pChar2;
		for (unsigned int xj = 0; xj < Len(); xj++)
		{
			pChar1 = *pStr1;
			pChar2 = *pStr2;
			if (pChar1 >= 'A' && pChar1 <= 'Z')
				pChar1 = 'a' + (pChar1 - 'A');
			if (pChar2 >= 'A' && pChar2 <= 'Z')
				pChar2 = 'a' + (pChar2 - 'A');
			if (pChar1 != pChar2)
				return false;

			pStr1++;
			pStr2++;
		}
		return true;
	}
	else if (m_String == 0 && pString.Data == 0)
		return true;
	return false;
}

BSTR FoxString::ToBSTR() const
{
	DWORD dwLen = Len();
	BSTR pBstr = SysAllocStringByteLen(0,dwLen * 2);

	if (pBstr == 0)
		throw E_INSUFMEMORY;

	int nChars = MultiByteToWideChar(m_CodePage, MB_PRECOMPOSED,m_String,dwLen,pBstr,dwLen);
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
    if ((hr = SafeArrayAccessData(pArray,(void**)&pData)) != S_OK)
	{
		SaveWin32Error("SafeArrayAccessData", hr);
		SafeArrayDestroy(pArray);
		throw E_APIERROR;
	}
	memcpy(pData,m_String,Len());
	if ((hr = SafeArrayUnaccessData(pArray)) != S_OK)
	{
		SaveWin32Error("SafeArrayUnaccessData", hr);
		SafeArrayDestroy(pArray);
		throw E_APIERROR;
	}
	return pArray;
}

void FoxString::Detach()
{
	UnlockHandle();
	m_BufferSize = m_Value.ev_length = m_Value.ev_handle = 0;
	m_String = 0;
}

void FoxString::Detach(ValueEx &pValue)
{
	pValue.ev_type = 'C';
	pValue.ev_handle = m_Value.ev_handle;
	pValue.ev_length = m_Value.ev_length;
	pValue.ev_width = m_Value.ev_width;
	UnlockHandle();
	m_BufferSize = m_Value.ev_length = m_Value.ev_handle = 0;
	m_String = 0;
}

void FoxString::DetachParameter()
{
	assert(m_ParameterRef == true && m_Value.ev_type == 'C' && m_Value.ev_handle);
	ValueEx vValue;
	if (!vValue.AllocHandle(m_Value.ev_length))
		throw E_INSUFMEMORY;
	vValue.LockHandle();
	char* pString = vValue.HandleToPtr();
	memcpy(pString, m_String, m_Value.ev_length);
	Release();
	m_Value.ev_handle = vValue.ev_handle;
	m_BufferSize = m_Value.ev_length;
	m_String = pString;
	m_ParameterRef = false;
	m_Locked = true;
}

/* Operator overloading */
FoxString& FoxString::operator=(LocatorEx &pLoc)
{
	Release();
	int nErrorNo;
	if (nErrorNo = _Load(pLoc, &m_Value))
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

	Len(pValue.Len());
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
	if (m_Value.ev_type == 'C')
	{
		m_BufferSize = Len();
		LockHandle();
		m_String = HandleToPtr();
	}
	else if (m_Value.ev_type == 'O')
	{
        FreeObject();
		throw E_TYPECONFLICT;
	}
	else
		throw E_TYPECONFLICT;
	return *this;
}

FoxString& FoxString::operator=(const CStringView pString)
{
	if (pString && pString.Data != m_String)
	{
		unsigned int nBufferSize = pString.Len + 1;
		if (m_BufferSize < nBufferSize)
			Size(nBufferSize);

		memcpy(m_String, pString.Data, nBufferSize);
		Len(pString.Len);
	}
	else
	{
		if (m_String)
			*m_String = '\0';
		Len(0);
	}
	return *this;
}

FoxString& FoxString::operator=(const CWideStringView pWString)
{
	int nChars;
	if (pWString)
	{
		unsigned int nBufferSize = pWString.Len + 1;
		if (m_BufferSize < nBufferSize)
			Size(nBufferSize);

		nChars = WideCharToMultiByte(m_CodePage, 0, pWString.Data, pWString.Len, m_String, m_BufferSize, 0, 0);
		if (!nChars)
		{
			SaveWin32Error("MultiByteToWideChar", GetLastError());
			throw E_APIERROR;
		}
		Len(nChars);
		m_String[nChars] = '\0';
	}
	else
	{
		if (m_String)
			*m_String = '\0';
		Len(0);
	}
	return *this;
}

FoxString& FoxString::operator+=(const CStringView pString)
{
	unsigned int nLen = pString.Len + 1;
	int nDiff = m_BufferSize - (Len() + nLen);
	if (nDiff < 0)
		Expand(m_BufferSize + (-nDiff));
	memcpy(m_String + Len(), pString.Data, nLen);
	Len(nLen - 1);
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

bool FoxString::operator==(const CStringView pString) const
{
	if (pString)
	{
		if (pString.Data == m_String)
			return pString.Len == Len();
		else if (pString.Len == Len())
			return memcmp(m_String, pString.Data, pString.Len) == 0;
	}
	return false;
}

FoxString::operator CStringView()
{ 
	return CStringView(m_String, m_Value.ev_length);
}

/* FoxDate */
FoxDate::FoxDate(ValueEx &pVal) : FoxValue('D')
{
	assert(pVal.Vartype() == 'D');
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

	if (pTime.QuadPart > MAXVFPFILETIME) //if bigger than maximumDATETIME - 9999/12/12 23:59:59
		pTime.QuadPart = MAXVFPFILETIME; //set to maximum date ..
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

void FoxDate::DateToSystemTime(SYSTEMTIME &sTime) const
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

void FoxDate::DateToFileTime(FILETIME &sTime) const
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
FoxDateTime::FoxDateTime(ValueEx &pVal) : FoxValue('T')
{
	assert(pVal.Vartype() == 'T');
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

FoxDateTime& FoxDateTime::operator=(ValueEx& pValue)
{
	assert(pValue.Vartype() == 'D' || pValue.Vartype() == 'T');
	m_Value.ev_real = pValue.ev_real;
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

	if (pTime.QuadPart > MAXVFPFILETIME) //if bigger than maximum DATETIME - 9999/12/12 23:59:59
		pTime.QuadPart = MAXVFPFILETIME; //set to maximum date ..
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

void FoxDateTime::DateTimeToSystemTime(SYSTEMTIME &sTime) const
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

void FoxDateTime::DateTimeToFileTime(FILETIME &sTime) const
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
FoxDateTimeLiteral& FoxDateTimeLiteral::operator=(SYSTEMTIME& sTime)
{
	FILETIME sFileTime, sLocalFTime;
	SYSTEMTIME sSysTime;

	if (m_ToLocal)
	{
		if (!SystemTimeToFileTime(&sTime, &sFileTime))
		{
			SaveWin32Error("SystemTimeToFileTime", GetLastError());
			throw E_APIERROR;
		}
		if (!FileTimeToLocalFileTime(&sFileTime, &sLocalFTime))
		{
			SaveWin32Error("FileTimeToLocalFileTime", GetLastError());
			throw E_APIERROR;
		}
		if (!FileTimeToSystemTime(&sLocalFTime, &sSysTime))
		{
			SaveWin32Error("FileTimeToSystemTime", GetLastError());
			throw E_APIERROR;
		}

		if (sSysTime.wYear > 0 && sSysTime.wYear < 10000)
		{
			m_Length = sprintf(m_Literal, "{^%04hu-%02hu-%02hu %02hu:%02hu:%02hu}",
				sSysTime.wYear, sSysTime.wMonth, sSysTime.wDay, sSysTime.wHour, sSysTime.wMinute, sSysTime.wSecond);
		}
		else
		{
			strcpy(m_Literal, "{ ::}");
			m_Length = strlen("{ ::}");
		}
	}
	else
	{
		if (sTime.wYear > 0 && sTime.wYear < 10000)
		{
			m_Length = sprintf(m_Literal, "{^%04hu-%02hu-%02hu %02hu:%02hu:%02hu}",
				sTime.wYear, sTime.wMonth, sTime.wDay, sTime.wHour, sTime.wMinute, sTime.wSecond);
		}
		else
		{
			strcpy(m_Literal, "{ ::}");
			m_Length = strlen("{ ::}");
		}
	}
	return *this;
}

FoxDateTimeLiteral& FoxDateTimeLiteral::operator=(FILETIME& sTime)
{
	SYSTEMTIME sSysTime;
	FILETIME sFileTime;

	if (m_ToLocal)
	{
		if (!FileTimeToLocalFileTime(&sTime, &sFileTime))
		{
			SaveWin32Error("FileTimeToLocalFileTime", GetLastError());
			throw E_APIERROR;
		}
		if (!FileTimeToSystemTime(&sFileTime, &sSysTime))
		{
			SaveWin32Error("FileTimeToSystemTime", GetLastError());
			throw E_APIERROR;
		}
	}
	else if (!FileTimeToSystemTime(&sTime, &sSysTime))
	{
		SaveWin32Error("FileTimeToSystemTime", GetLastError());
		throw E_APIERROR;
	}

	if (sSysTime.wYear > 0 && sSysTime.wYear < 10000)
	{
		m_Length = sprintf(m_Literal, "{^%04hu-%02hu-%02hu %02hu:%02hu:%02hu}",
			sSysTime.wYear, sSysTime.wMonth, sSysTime.wDay, sSysTime.wHour, sSysTime.wMinute, sSysTime.wSecond);
	}
	else
	{
		strcpy(m_Literal, "{ ::}");
		m_Length = strlen("{ ::}");
	}
	return *this;
}

FoxDateTimeLiteral::operator CStringView()
{ 
	return CStringView(m_Literal, m_Length); 
}

FoxObject::FoxObject(ValueEx &pVal) : FoxValue('O'), m_Property(0), m_ParameterRef(true)
{
	assert(pVal.Vartype() == 'O');
	m_Value.ev_object = pVal.ev_object;
}

FoxObject::FoxObject(ParamBlkEx& parm, int nParmNo) : m_Property(0)
{

	if (parm.pCount >= nParmNo && parm(nParmNo)->ev_type == 'O')
	{
		m_Value.ev_type = 'O';
		m_Value.ev_object = parm(nParmNo)->ev_object;
		m_ParameterRef = true;
	}
	else
	{
		m_Value.ev_type = '0';
		m_Value.ev_object = 0;
		m_ParameterRef = false;
	}
}

FoxObject::FoxObject(char *pExpression) : FoxValue('0'), m_Property(0), m_ParameterRef(false)
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
	CStrBuilder<VFP2C_MAX_CALLBACKFUNCTION> pExeBuffer;
	pExeBuffer.Format("CREATEOBJECT('%S')", pClass);
	Evaluate(pExeBuffer);
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

FoxMemo::FoxMemo(ParamBlkEx& parm, int nParmNo) : m_pContent(0)
{
	if (parm.pCount >= nParmNo && parm(nParmNo).IsMemoRef())
	{
		m_Loc.Init(parm(nParmNo));

		m_Location = _FindMemo(m_Loc);
		if (m_Location < 0)
			throw E_FIELDNOTFOUND;

		m_File = _MemoChan(m_Loc.l_where);
		if (m_File == -1)
		{
			SaveCustomError("_MemoChan", "Function failed for workarea %I.", m_Loc.l_where);
			throw E_APIERROR;
		}

		m_Size = _MemoSize(m_Loc);
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

FoxMemo::FoxMemo(LocatorEx &pLoc) : m_pContent(0)
{
	m_Loc.Init(pLoc);

	m_Location = _FindMemo(m_Loc);
	if (m_Location < 0)
		throw E_FIELDNOTFOUND;

	m_File = _MemoChan(m_Loc.l_where);
	if (m_File == -1)
	{
		SaveCustomError("_MemoChan", "Function failed for workarea %I.", m_Loc.l_where);
		throw E_APIERROR;
	}

	m_Size = _MemoSize(m_Loc);
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
	m_Location = _AllocMemo(m_Loc,nSize);
	if (m_Location == -1)
	{
		SaveCustomError("_AllocMemo","Function failed.");
		throw E_APIERROR;
	}
}

void FoxMemo::Append(char *pData, unsigned int nLen)
{
	if (_FSeek(m_File, m_Location, FS_FROMBOF) != m_Location)
	{
		SaveCustomError("_FSeek", "Function failed.");
		throw E_APIERROR;
	}
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

	if (_FSeek(m_File, m_Location, FS_FROMBOF) != m_Location)
	{
		SaveCustomError("_FSeek", "Function failed.");
		throw E_APIERROR;
	}
	nLen = _FRead(m_File, m_pContent, nBytes);
	return m_pContent;
}

FoxMemo& FoxMemo::operator=(FoxString &pString)
{
	/* if data is smaller or equal to 65000, we can use _DBReplace */
	if (pString.Len() <= 65000)
	{
		int nErrorNo;
		if (nErrorNo = _DBReplace(m_Loc,pString))
			throw nErrorNo;
	}
	else
	{
		Alloc(pString.Len());
		if (_FSeek(m_File, m_Location, FS_FROMBOF) != m_Location)
		{
			SaveCustomError("_FSeek", "Function failed.");
			throw E_APIERROR;
		}
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

FoxArray::FoxArray(ValueEx &pVal, unsigned int nRows, unsigned int nDims)
{
 	Init(pVal, nRows, nDims);
}

FoxArray::FoxArray(FoxParameterEx& pVal, unsigned int nRows, unsigned int nDims)
{
	if (pVal->Vartype() == 'C' && pVal->Len() > 0)
		Init(pVal.val, nRows, nDims);
	else if (pVal->Vartype() == 'R')
		Init((LocatorEx)pVal);
	else
	{
		SaveCustomErrorEx("FoxArray:Constructor", "Invalid parameter type '%s' to initialize array.", 0, pVal->Vartype());
		throw E_INVALIDPARAMS;
	}
}

FoxArray::FoxArray(LocatorEx &pLoc)
{
	Init(pLoc);
}

FoxArray::FoxArray(ParamBlkEx& parm, int nParmNo)
{ 
	if (parm.pCount >= nParmNo)
	{
		FoxParameterEx& pParameter = parm(nParmNo);
		if (pParameter->Vartype() == 'C')
			Init(pParameter.val);
		else if (pParameter->Vartype() == 'R')
			Init((LocatorEx)pParameter);
		else
			Init();
	}
	else
		Init();
}

FoxArray::FoxArray(ParamBlkEx& parm, int nParmNo, char cTypeCheck)
{
	if (parm.pCount >= nParmNo)
	{
		FoxParameterEx& pVal = parm(nParmNo);
		if (pVal->Vartype() == 'C')
		{
			Init(pVal.val);
			return;
		}
		else if (pVal->Vartype() != cTypeCheck)
		{
			SaveCustomErrorEx("FoxArray:Constructor", "Invalid parameter type '%s' to initialize array.", 0, cTypeCheck);
			throw E_INVALIDPARAMS;
		}
	}
	Init();
}

void FoxArray::Init()
{
	m_Loc.l_NTI = 0;
	m_Loc.l_subs = 0;
	m_Loc.l_sub1 = 0;
	m_Loc.l_sub2 = 0;
	m_Rows = 0;
	m_Dims = 0;
	m_AutoGrow = 0;
	m_AutoOverflow = 0;
}

void FoxArray::Init(LocatorEx &pLoc)
{
	m_Loc.Init(pLoc);
	m_Rows = m_Loc.ARows();
	m_Dims = m_Loc.ADims();
	m_Dims = m_Dims > 0 ? m_Dims : 1;
	m_Loc.l_subs = m_Dims > 1 ? 2 : 1;
	m_Loc.l_sub1 = 0;
	m_Loc.l_sub2 = 0;
	m_AutoGrow = 0;
	m_AutoOverflow = 0;
}

void FoxArray::Init(ValueEx &pVal, unsigned int nRows, unsigned int nDims)
{
	if (pVal.Vartype() != 'C' || pVal.ev_handle == 0 || pVal.ev_length == 0 || pVal.ev_length > VFP_MAX_VARIABLE_NAME)	
	{
		SaveCustomError("FoxArray:Init", "Invalid parameter to initialize array: the parameter has to be the variable name as a string no longer than 128 characters");
		throw E_INVALIDPARAMS;
	}
	m_Name.Append(pVal);
	m_Name.SetFormatBase();
	m_Loc.l_NTI = 0;
	m_AutoGrow = 0;
	m_AutoOverflow = 0;
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
		NTI nVarNti = _NewVar(m_Name, m_Loc, NV_PRIVATE);
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

FoxArray& FoxArray::Dimension(ValueEx &pVal, unsigned int nRows, unsigned int nDims)
{
	assert(pVal.Vartype() == 'C' && pVal.ev_handle != 0);
	CStringView pName(pVal.HandleToPtr(), pVal.Len());
	return Dimension(pName, nRows, nDims);
}

FoxArray& FoxArray::Dimension(CStringView pName, unsigned int nRows, unsigned int nDims)
{
	if (pName.Len == 0 || pName.Len > VFP_MAX_VARIABLE_NAME)
	{
		SaveCustomErrorEx("FoxArray:Dimension", "Invalid array name '%V' passed to initialize array, name to short or to long.", 0, &pName);
		throw E_INVALIDPARAMS;
	}
	m_Name = pName;
	m_Name.SetFormatBase();
	return Dimension(nRows,nDims);
}

bool FoxArray::FindArray()
{
	NTI nVarNti = _NameTableIndex(m_Name);
	if (nVarNti == -1)
		return false;
	return _FindVar(nVarNti, -1, m_Loc) > 0;
}

void FoxArray::InitArray()
{
	assert(m_Loc.l_NTI);
	int nErrorNo;
	USHORT tmpSubs;
	ValueEx vFalse;
	vFalse.SetLogical(false);
	tmpSubs = m_Loc.l_subs;
	m_Loc.l_subs = 0;
	if (nErrorNo = _Store(m_Loc, vFalse))
		throw nErrorNo;
	m_Loc.l_subs = tmpSubs;
}

void FoxArray::ReDimension(unsigned int nRows, unsigned int nDims)
{
	assert(m_Name.Len());
	CStrBuilder<256> pExeBuffer;
	if (nDims > 1)
		pExeBuffer.Format("DIMENSION %V[%U,%U]", &(CStringView)m_Name, nRows, nDims);
	else
		pExeBuffer.Format("DIMENSION %V[%U]", &(CStringView)m_Name, nRows);
	Execute(pExeBuffer);
	m_Rows = nRows;
	m_Dims = nDims;
}

unsigned int FoxArray::ALen(unsigned int &nDims)
{
	if (m_Loc.l_NTI == 0)
	{	
		if (!FindArray())
			throw E_VARIABLENOTFOUND;

		m_Rows = m_Loc.ARows();
		m_Dims = m_Loc.ADims();
		m_Loc.l_subs = m_Dims > 1 ? 2 : 1;
	}

	nDims = m_Dims;
	return m_Rows;
}

void FoxArray::Release()
{
	if (m_Loc.l_NTI > 0)
	{
		int nErrorNo = _Release(m_Loc.l_NTI);
		if (nErrorNo)
			throw nErrorNo;
		m_Loc.l_NTI = 0;
	}
}

unsigned int FoxArray::Grow()
{
	assert(m_AutoOverflow > 0 || m_Loc.l_sub1 + 1 <= VFP_MAX_ARRAY_ROWS); // LCK only supports array's up to 65000 rows
	int rowcount = m_Loc.l_sub1 + 1;
	if (rowcount <= VFP_MAX_ARRAY_ROWS)
	{
		m_Loc.l_sub1 = rowcount;
		if (m_Loc.l_sub1 > m_Rows)
			ReDimension(min(m_Loc.l_sub1 + m_AutoGrow, VFP_MAX_ARRAY_ROWS), m_Dims);
	}
	else
	{
		if (m_AutoOverflow > 0)
		{
			OverflowArray();
			m_Loc.l_sub1 = 1;
		}
	}
	return m_Loc.l_sub1;
}

void FoxArray::OverflowArray()
{
	m_AutoOverflow++;
	m_Name.AppendFormatBase("_VFP2C_OF_%U", m_AutoOverflow);
	ReDimension(m_AutoGrow, m_Dims);
	if (!FindArray())
		throw E_VARIABLENOTFOUND;
}

unsigned int FoxArray::CompactOverflow()
{
	if (m_AutoOverflow > 1)
	{
		CStrBuilder<512> pExeBuffer;
		CStrBuilder<VFP_MAX_VARIABLE_NAME + 1> pArrayName;

		m_Name.ResetToFormatBase();
		pArrayName = m_Name;
		pArrayName.SetFormatBase();
		// calculate total array size
		unsigned int nTotalRows = (m_AutoOverflow - 1) * VFP_MAX_ARRAY_ROWS + CurrentRow();
		unsigned int nMaxElementsPerArray = VFP_MAX_ARRAY_ROWS * ADims();
		unsigned int nTotalElementsToCopy = nTotalRows * ADims() - nMaxElementsPerArray;
		unsigned int nFirstDestinationElement = nMaxElementsPerArray + 1;
		// redimension first array to final size
		ReDimension(nTotalRows, ADims());
		for (unsigned int xj = 2; xj <= m_AutoOverflow; xj++)
		{
			pArrayName.AppendFormatBase("_VFP2C_OF_%U", xj);
			// append elements from 2nd, 3rd .. array into the first array
			pExeBuffer.Format("ACOPY(%V,%V,1,%U,%U)", &(CStringView)pArrayName, &(CStringView)m_Name, min(nMaxElementsPerArray, nTotalElementsToCopy), nFirstDestinationElement);
			Execute(pExeBuffer);
			// release the intermediary arrays
			pExeBuffer.Format("RELEASE %V", &(CStringView)pArrayName);
			Execute(pExeBuffer);
			nFirstDestinationElement += nMaxElementsPerArray;
			nTotalElementsToCopy -= nMaxElementsPerArray;
		}
		return nTotalRows;
	}
	else if (m_AutoGrow && m_Loc.l_sub1 && m_Loc.l_sub1 < m_Rows)
		ReDimension(m_Loc.l_sub1, m_Dims);
	return m_Loc.l_sub1;
}

FoxCursor::~FoxCursor()
{
	if (m_AutoFLocked)
		Unlock();

	if (m_pFieldLocs)
		delete[] m_pFieldLocs;
}

bool FoxCursor::Create(CStringView pCursorName, CStringView pFields, bool bAttach)
{
	if (pCursorName.Len == 0 || pCursorName.Len > VFP_MAX_CURSOR_NAME)
	{
		SaveCustomError("FoxCursor:Create", "Passed cursorname is invalid, len = 0 or > VFP_MAX_CURSOR_NAME (128).");
		throw E_INVALIDPARAMS;
	}
		
	bool bCursorCreated;
	FoxValue vValue;
	CStrBuilder<4096> pExeBuffer;
	char* pField;

	if (bAttach)
	{
		pExeBuffer.Format("SELECT('%V')", &pCursorName);
		vValue.Evaluate(pExeBuffer);
		m_WorkArea = vValue->ev_long;
	}

	// if workarea == 0 the cursor does not exist
	if(m_WorkArea == 0)
	{
		bCursorCreated = true;
		// create the cursor
		pExeBuffer.Format("CREATE CURSOR %V (%V)", &pCursorName, &pFields);
		Execute(pExeBuffer);
		// get the workarea
		vValue.Evaluate("SELECT(0)");
		m_WorkArea = vValue->ev_long;
	}
	else
	{
		bCursorCreated = false;
		// select the workarea
		pExeBuffer.Format("SELECT %I", m_WorkArea);
		Execute(pExeBuffer);
	}
	
	// get fieldcount
	vValue.Evaluate("FCOUNT()");
	m_FieldCnt = vValue->ev_long;

	if (m_pFieldLocs)
	{
		delete[] m_pFieldLocs;
		m_pFieldLocs = 0;
	}
	m_pFieldLocs = new FieldLocatorEx[m_FieldCnt];
	if (m_pFieldLocs == 0)
		throw E_INSUFMEMORY;

	// get locators to each field
	NTI nVarNti;
	for (unsigned int nFieldNo = 1; nFieldNo <= m_FieldCnt; nFieldNo++)
	{
		pExeBuffer.Format("FIELD(%I)+CHR(0)", nFieldNo);
		vValue.Evaluate(pExeBuffer);
	
		vValue.LockHandle();
		pField = vValue.HandleToPtr();
        nVarNti = _NameTableIndex(pField);
		vValue.UnlockHandle();
		vValue.FreeHandle();

		if (nVarNti == -1)
			throw E_FIELDNOTFOUND;

		if (!_FindVar(nVarNti, m_WorkArea, *(m_pFieldLocs + (nFieldNo-1))))
			throw E_FIELDNOTFOUND;
	}
	return bCursorCreated;
}

FoxCursor& FoxCursor::Attach(CStringView pCursorName, CStringView pFields)
{
	FoxValue vValue;
	CStrBuilder<256> pExeBuffer;
	CStrBuilder<VFP_MAX_COLUMN_NAME + 1> pFieldname;
	pExeBuffer.Format("SELECT('%V')", &pCursorName);
	vValue.Evaluate(pExeBuffer);
	m_WorkArea = vValue->ev_long;

	// if workarea == 0 the cursor does not exist
	if(m_WorkArea == 0)
		throw E_ALIASNOTFOUND;

	// select the workarea
	pExeBuffer.Format("SELECT %I", m_WorkArea);
	Execute(pExeBuffer);
		
	m_FieldCnt = pFields.GetWordCount(',');
	if (m_pFieldLocs)
	{
		delete[] m_pFieldLocs;
		m_pFieldLocs = 0;
	}
	m_pFieldLocs = new FieldLocatorEx[m_FieldCnt];
	if (m_pFieldLocs == 0)
		throw E_INSUFMEMORY;

	// get locators to each field
	NTI nVarNti;
	for (unsigned int nFieldNo = 0; nFieldNo < m_FieldCnt; nFieldNo++)
	{
		CStringView pField = pFields.GetWordNum(1, ',');
		pFieldname = pField.Alltrim();
        nVarNti = _NameTableIndex(pFieldname);
		if (nVarNti == -1)
			throw E_FIELDNOTFOUND;

		if (!_FindVar(nVarNti, m_WorkArea, *(m_pFieldLocs + nFieldNo)))
			throw E_FIELDNOTFOUND;

		pFields = pFields + pField.Len;
	}
	return *this;
}

FoxCursor& FoxCursor::Attach(int nWorkArea, CStringView pFields)
{
	CStrBuilder<256> pExeBuffer;
	CStrBuilder<VFP_MAX_COLUMN_NAME> pFieldName;

	m_WorkArea = nWorkArea;
	// select the workarea
	pExeBuffer.Format("SELECT %I", nWorkArea);
	Execute(pExeBuffer);

	m_FieldCnt = pFields.GetWordCount(',');
	if (m_pFieldLocs)
	{
		delete[] m_pFieldLocs;
		m_pFieldLocs = 0;
	}	
	m_pFieldLocs = new FieldLocatorEx[m_FieldCnt];
	if (m_pFieldLocs == 0)
		throw E_INSUFMEMORY;

	// get locators to each field
	NTI nVarNti;
	for (unsigned int nFieldNo = 0; nFieldNo < m_FieldCnt; nFieldNo++)
	{
		CStringView pField = pFields.GetWordNum(1, ',');
		pFieldName = pField.Alltrim();
        nVarNti = _NameTableIndex(pFieldName);
		if (nVarNti == -1)
			throw E_FIELDNOTFOUND;

		if (!_FindVar(nVarNti, m_WorkArea, *(m_pFieldLocs + nFieldNo)))
			throw E_FIELDNOTFOUND;

		pFields = pFields + pField.Len;
	}
	return *this;
}

int FoxCursor::GetFieldNumber(char* pField)
{
	assert(m_WorkArea && m_pFieldLocs);
	NTI nVarNti;
	LocatorEx* pLoc;
	nVarNti = _NameTableIndex(pField);
	if (nVarNti == -1)
		return -1;

	for (unsigned int nFieldNo = 0; nFieldNo < m_FieldCnt; nFieldNo++)
	{
		pLoc = m_pFieldLocs + nFieldNo;
		if (pLoc->l_NTI == nVarNti)
			return nFieldNo;
	}
	return -1;
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
