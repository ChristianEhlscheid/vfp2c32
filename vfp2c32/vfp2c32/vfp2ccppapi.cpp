#include <new>
#include <cmath>

#if MSVC_VER < 1400
#include <cstdio>
#include <cstdarg>
#endif

#include "vfp2c32.h"
#include "vfp2ctime.h"

int CFoxVersion::m_Version = 0;
int CFoxVersion::m_MajorVersion = 0;
int CFoxVersion::m_MinorVersion = 0;

void _fastcall ReturnIDispatch(void* pObject)
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
		vTmpObject.SetNull();
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
	const char* VarName = "__VFP2C32_TEMP_OBJECT";
	const char* VarName2 = "__VFP2C32_TEMP_COMOBJECT";
	const char* pCommand = "m.__VFP2C32_TEMP_COMOBJECT = _VFP.Eval('m.__VFP2C32_TEMP_OBJECT')";
	const char* pCommand2 = "GetIDispatch(m.__VFP2C32_TEMP_COMOBJECT)";
	ValueEx pDisp;
	pDisp.SetNull();
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

long _fastcall RecCountEx(SHORT nWorkarea, bool bCountFromCurrentRecord)
{
	ValueEx vExpr;
	int nErrorNo;
	long nRecCount;
	bool bDeleted = false;
	vExpr.SetNull();
	if (!bCountFromCurrentRecord)
	{
		nErrorNo = __Evaluate(vExpr, "SET('DELETED')");
		if (nErrorNo)
			throw nErrorNo;
		vExpr.FreeHandle();
		bDeleted = vExpr.ev_length == 2;  // ON - string len = 2
	}

	if (bCountFromCurrentRecord || bDeleted)
	{
		AutoWorkAreaRestore pWorkAreaRestore(nWorkarea, 0, true);
		AutoRecNoRestore pRecNoRestore(nWorkarea);
		FoxVariable pCount;
		if (bCountFromCurrentRecord)
			nErrorNo = __Execute("COUNT REST TO m.__VFP2C_COUNT_INTERNAL");
		else
			nErrorNo = __Execute("COUNT TO m.__VFP2C_COUNT_INTERNAL");
		if (nErrorNo)
			throw nErrorNo;
		pCount.Attach("__VFP2C_COUNT_INTERNAL");
		vExpr = pCount();
		nRecCount = vExpr.ev_long;
	}
	else
	{
		nRecCount = RecCount(nWorkarea);
	}
	return nRecCount;
}

int _fastcall Dimension(char* pArrayName, unsigned long nRows, unsigned long nDims)
{
	CStrBuilder<256> pExeBuffer;
	if (nDims > 1)
		pExeBuffer.Format("DIMENSION %S[%U,%U]", pArrayName, nRows, nDims);
	else
		pExeBuffer.Format("DIMENSION %S[%U]", &pArrayName, nRows);
	return _Execute(pExeBuffer);
}

int _fastcall Dimension(char* pArrayName, Locator* lArrayLoc, unsigned long nRows, unsigned long nDims)
{
	int nErrorNo;
	nErrorNo = FindFoxVar(pArrayName, lArrayLoc);
	if (nErrorNo == E_VARIABLENOTFOUND)
	{
		// create array
		nErrorNo = Dimension(pArrayName, nRows, nDims);
		if (nErrorNo)
			return nErrorNo;

		nErrorNo = FindFoxVar(pArrayName, lArrayLoc);
		if (nErrorNo)
			return nErrorNo;
	}
	else if (nErrorNo == 0)
	{
		// redimension array to passed size
		nErrorNo = Dimension(pArrayName, nRows, nDims);
		if (nErrorNo)
			return nErrorNo;

		// initialize all elements to .F.
		Value vFalse;
		vFalse.ev_type = 'L';
		vFalse.ev_width = 0;
		lArrayLoc->l_subs = 0;
		nErrorNo = _Store(lArrayLoc, &vFalse);
		if (nErrorNo)
			return nErrorNo;

		lArrayLoc->l_subs = nDims > 1 ? 2 : 1;
	}
	else
		return nErrorNo;

	lArrayLoc->l_sub1 = 0;
	lArrayLoc->l_sub2 = 0;
	return 0;
}

int _fastcall ASubscripts(Locator* pLoc, int* nRows, int* nDims)
{
	*nRows = _ALen(pLoc->l_NTI, AL_SUBSCRIPT1);
	if (*nRows == -1)
		return E_NOTANARRAY;
	*nDims = _ALen(pLoc->l_NTI, AL_SUBSCRIPT2);
	return 0;
}

// fill a Locator with a reference to a FoxPro variable
int _fastcall FindFoxVar(char* pName, Locator* pLoc)
{
	NTI nVarNti = _NameTableIndex(pName);
	if (nVarNti == -1)
	{
		SaveCustomErrorEx("_NameTableIndex", "Variable '%S' not found.", E_VARIABLENOTFOUND, pName);
		return E_APIERROR;
	}

	if (_FindVar(nVarNti, -1, pLoc))
		return 0;
	else
	{
		SaveCustomErrorEx("_FindVar", "Variable '%S' not found.", E_VARIABLENOTFOUND, pName);
		return E_APIERROR;
	}
}

// fill a Locator with a reference to a FoxPro field
int _fastcall FindFoxField(char* pName, Locator* pLoc, SHORT nWorkarea)
{
	NTI nVarNti = _NameTableIndex(pName);
	if (nVarNti == -1)
	{
		SaveCustomErrorEx("_NameTableIndex", "Field '%S' not found in workarea %I.", E_FIELDNOTFOUND, pName, nWorkarea);
		return E_APIERROR;
	}

	if (_FindVar(nVarNti, nWorkarea, pLoc))
		return 0;
	else
	{
		SaveCustomErrorEx("_FindVar", "Field '%S' not found in workarea %I.", E_FIELDNOTFOUND, pName, nWorkarea);
		return E_APIERROR;
	}
}

// like the above .. but instead of the workarea number the cursor/tablename can be passed
int _fastcall FindFoxField(char* pName, Locator* pLoc, char* pCursor)
{
	NTI nVarNti;
	int nErrorNo;
	Value vWorkarea = { '0' };
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;
	
	nVarNti = _NameTableIndex(pName);
	if (nVarNti == -1)
	{
		SaveCustomErrorEx("_NameTableIndex", "Field '%S' not found.", E_FIELDNOTFOUND, pName);
		return E_APIERROR;
	}

	pExeBuffer.Format("SELECT('%S')", pCursor);
	nErrorNo = _Evaluate(&vWorkarea, pExeBuffer);
	if (nErrorNo)
		return nErrorNo;

	if (_FindVar(nVarNti, vWorkarea.ev_long, pLoc))
		return 0;
	else
	{
		SaveCustomErrorEx("_FindVar", "Field '%S' not found in workarea %I.", E_FIELDNOTFOUND, pName, vWorkarea.ev_long);
		return E_APIERROR;
	}
}

// fill a Locator with a reference to a FoxPro field, instead of passing the name one passes the field offset (1st field, 2nd field ..)
int _fastcall FindFoxField(int nFieldNo, Locator* pLoc, SHORT nWorkarea)
{
	ValueEx vFieldName;
	vFieldName.SetNull();
	char* pFieldName;
	NTI nVarNti;
	int nErrorNo;
	CStrBuilder<64> pExeBuffer;

	pExeBuffer.Format("FIELD(%I,%I)+CHR(0)", nFieldNo, nWorkarea);
	nErrorNo = _Evaluate(vFieldName, pExeBuffer);
	if (nErrorNo)
		return nErrorNo;

	pFieldName = vFieldName.HandleToPtr();
	nVarNti = _NameTableIndex(pFieldName);
	vFieldName.FreeHandle();

	if (nVarNti == -1)
	{
		SaveCustomErrorEx("_NameTableIndex", "Field '%S' not found.", E_FIELDNOTFOUND, pFieldName);
		return E_APIERROR;
	}

	if (_FindVar(nVarNti, nWorkarea, pLoc))
		return 0;
	else
	{
		SaveCustomErrorEx("_FindVar", "Field '%S' not found in workarea %I.", E_FIELDNOTFOUND, pFieldName, nWorkarea);
		return E_APIERROR;
	}
}

int _fastcall FindFoxVarOrField(char* pName, Locator* pLoc)
{
	NTI nVarNti;
	nVarNti = _NameTableIndex(pName);
	if (nVarNti == -1)
	{
		SaveCustomError("_NameTableIndex", "Variable or field '%S' not found.", pName);
		return E_APIERROR;
	}

	if (_FindVar(nVarNti, 0, pLoc))
		return 0;
	else
	{
		SaveCustomError("_FindVar", "Variable or field '%S' not found.", pName);
		return E_APIERROR;
	}
}

int _fastcall FindFoxVarOrFieldEx(char* pFullName, Locator* pLoc)
{
	CStringMatcher pMatcher(pFullName);
	char aTableOrVar[VFP2C_VFP_MAX_CURSOR_NAME];
	char aColumn[VFP2C_VFP_MAX_COLUMN_NAME];
	if (pMatcher.MatchIdentifier(aTableOrVar, VFP2C_VFP_MAX_CURSOR_NAME))
	{
		if (pMatcher.MatchChar('.') && pMatcher.MatchIdentifier(aColumn, VFP2C_VFP_MAX_COLUMN_NAME))
			return FindFoxField(aColumn, pLoc, aTableOrVar);
		else
			return FindFoxVarOrField(aTableOrVar, pLoc);
	}
	else
	{
		SaveCustomErrorEx("FindFoxVarOrFieldEx", "Variable or field '%S' not found, invalid identifier.", E_VARIABLENOTFOUND, pFullName);
		return E_APIERROR;
	}
}

int _fastcall StoreEx(Locator* pLoc, Value* pValue)
{
	if (pLoc->l_where == -1)
		return _Store(pLoc, pValue);
	else
		return _DBReplace(pLoc, pValue);
}

int _fastcall Zap(SHORT nWorkArea)
{
	CStrBuilder<64> pExeBuffer;
	pExeBuffer.Format("ZAP IN %i", nWorkArea);
	return _Execute(pExeBuffer);
}

int _fastcall Zap(CStringView pCursor)
{
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;
	pExeBuffer.Format("ZAP IN %V", &pCursor);
	return _Execute(pExeBuffer);
}

bool _fastcall Used(CStringView pCursor)
{
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;
	Value vValue;
	pExeBuffer.Format("USED('%V')", &pCursor);
	vValue.ev_type = '0';
	int nErrorNo = _Evaluate(&vValue, pExeBuffer);
	if (nErrorNo)
		throw nErrorNo;
	return vValue.ev_length > 0;
}

SHORT _fastcall Select(SHORT nWorkarea)
{
	CStrBuilder<64> pExeBuffer;
	Value vValue;
	pExeBuffer.Format("SELECT(%i)", nWorkarea);
	vValue.ev_type = '0';
	int nErrorNo = _Evaluate(&vValue, pExeBuffer);
	if (nErrorNo)
		throw nErrorNo;
	return (SHORT)vValue.ev_long;
}

SHORT _fastcall Select(CStringView pCursor)
{
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;
	Value vValue;
	pExeBuffer.Format("SELECT('%V')", &pCursor);
	vValue.ev_type = '0';
	int nErrorNo = _Evaluate(&vValue, pExeBuffer);
	if (nErrorNo)
		throw nErrorNo;
	return (SHORT)vValue.ev_long;
}

int _fastcall SelectCmd(SHORT nWorkarea)
{
	CStrBuilder<64> pExeBuffer;
	pExeBuffer.Format("SELECT %i", nWorkarea);
	return _Execute(pExeBuffer);
}

/* FoxString */
FoxString::FoxString() : FoxValue('C')
{
	m_BufferSize = 0;
	m_String = 0;
	m_CodePage = CP_ACP;
}

FoxString::FoxString(FoxString &pString) : FoxValue('C')
{
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

FoxString::FoxString(ParamBlkEx& parm, int nParmNo, FoxStringInitialization eInit) : FoxValue('C')
{
	Attach(parm, nParmNo, eInit);
}

FoxString::FoxString(ParamBlkEx& parm, int nParmNo, unsigned int nExpand, FoxStringInitialization eInit) : FoxValue('C')
{
	Attach(parm, nParmNo, nExpand, eInit);
}

FoxString::FoxString(const CStringView pString) : FoxValue('C')
{
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
	AllocHandle(nBufferSize);
	LockHandle();
	m_String = HandleToPtr();
	m_BufferSize = nBufferSize;
	*m_String = '\0';
}

FoxString::FoxString(BSTR pString, UINT nCodePage) : FoxValue('C')
{
	m_CodePage = nCodePage;
	if (pString)
	{
		unsigned int nLen = SysStringLen(pString);
		m_Value.ev_length = nLen;
		m_BufferSize = nLen + 1;
		AllocHandle(m_BufferSize);
		LockHandle();
		m_String = HandleToPtr();
		int nChars;
		nChars = WideCharToMultiByte(nCodePage, 0, pString, (int)nLen, m_String, (int)nLen, 0, 0);
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
	assert(m_Value.ev_handle == 0);
	// nullterminate
	if (!pVal->NullTerminate())
		throw E_INSUFMEMORY;

	m_Value.ev_length = pVal->ev_length;
	m_Value.ev_width = pVal->ev_width;
	m_Value.ev_handle = pVal->ev_handle;
	m_BufferSize = pVal->ev_length + 1;
	m_Value.ev_padding = Parameter;
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
	m_Value.ev_padding = Parameter;
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
			m_Value.ev_padding = Parameter;
			LockHandle();
			m_String = HandleToPtr();
			return true;
		}
	}
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
			m_Value.ev_padding = Parameter;
			LockHandle();
			m_String = HandleToPtr();
			return true;
		}
	}
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
				m_Value.ev_padding = Parameter;
				LockHandle();
				m_String = HandleToPtr();
				return true;
			}
		}
	}
	m_BufferSize = 0;
	m_String = 0;
	return false;
}

bool FoxString::Attach(ParamBlkEx& parm, int nParmNo, unsigned int nExpand, FoxStringInitialization eInit)
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
				if (nExpand > 0)
				{
					if (!pVal.ExpandHandleSize(nExpand))
						throw E_INSUFMEMORY;
				}
				m_Value.ev_handle = pVal.ev_handle;
				m_Value.ev_length = pVal.ev_length;
				m_Value.ev_width = pVal.ev_width;
				m_BufferSize = pVal.ev_length + 1;
				m_Value.ev_padding = Parameter;
				LockHandle();
				m_String = HandleToPtr();
				return true;
			}
		}
	}
	m_BufferSize = 0;
	m_String = 0;
	return false;
}

void FoxString::Release()
{
	if (m_Value.ev_handle)
	{
		if (m_Value.ev_padding & Locked)
			_HUnLock(m_Value.ev_handle);
		if ((m_Value.ev_padding & Parameter) == 0)
		{
			_FreeHand(m_Value.ev_handle);
#if defined(_DEBUGALLOCATIONS)
			VfpAllocationCount--;
#endif
		}
		m_Value.ev_handle = 0;
		m_Value.ev_padding = Unlocked;
	}
	m_String = 0;
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
	return Len((unsigned int)strnlen(m_String, m_BufferSize));
}

unsigned int FoxString::StringDblLen()
{
	const char *pString = m_String;
	unsigned int nMaxLen = m_BufferSize;

	while (true)
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

	while (true)
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
	{
#pragma warning(disable : 4996)
		_strlwr(m_String);
#pragma warning(default : 4996)
	}
	return *this;
}

FoxString& FoxString::Upper()
{
	if (m_String)
	{
#pragma warning(disable : 4996)
		_strupr(m_String);
#pragma warning(default : 4996)
	}
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

bool FoxString::StartsWith(const CStringView pString) const
{
	return pString.Data != 0 && pString.Len <= Len() && memcmp(m_String, pString.Data, pString.Len) == 0;
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
			m_String[0] = '\0';
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

FoxString& FoxString::Format(const char* format, ...)
{
	va_list lpArgs;
	va_start(lpArgs, format);
	m_Value.ev_length = nprintfex(m_String, format, (int)m_BufferSize, lpArgs);
	va_end(lpArgs);
	return *this;
}

FoxString& FoxString::Replicate(const CStringView pString, unsigned int nCount)
{
	if (pString)
	{
		unsigned int nNewSize = pString.Len * nCount + 1;
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
	if (nMax == 0)
		nMax = m_Value.ev_length;

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

	char *pSearch = m_String + (m_Value.ev_length - 1);
	nMax = max(nMax, m_Value.ev_length);

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
	unsigned int nLen = m_Value.ev_length;
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
	unsigned int nLen = m_Value.ev_length;
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
	if (m_BufferSize < m_Value.ev_length + 1)
		Expand(1);

	m_String[m_Value.ev_length] = '\0';
	return *this;
}

FoxString& FoxString::DoubleNullTerminate()
{
	assert(m_Value.ev_handle);
	if (m_BufferSize < m_Value.ev_length + 2)
		Expand(2);

	m_String[m_Value.ev_length] = '\0';
	m_String[m_Value.ev_length + 1] = '\0';
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
	vFullpath.SetNull();
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pVarName;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pExeBuffer;
	pVarName.Format("_VFP2C32_FULLPATH_%U", this);
	FoxVariable pVar(pVarName, false);
	pVar() = m_Value;
	CStringView pView = pVarName.StringView();
	pExeBuffer.Format("FULLPATH(m.%V)+CHR(0)", &pView);
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

	int nChars = MultiByteToWideChar(m_CodePage, MB_PRECOMPOSED, m_String, (int)dwLen, pBstr, (int)dwLen);
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
	hr = SafeArrayAccessData(pArray, (void**)&pData);
    if (hr != S_OK)
	{
		SaveWin32Error("SafeArrayAccessData", hr);
		SafeArrayDestroy(pArray);
		throw E_APIERROR;
	}
	memcpy(pData,m_String,Len());
	hr = SafeArrayUnaccessData(pArray);
	if (hr != S_OK)
	{
		SaveWin32Error("SafeArrayUnaccessData", hr);
		SafeArrayDestroy(pArray);
		throw E_APIERROR;
	}
	return pArray;
}

void FoxString::DetachParameter()
{
	assert(m_Value.ev_type == 'C' && (m_Value.ev_padding & Parameter) && m_Value.ev_handle);
	ValueEx vValue;
	vValue.SetString(m_Value.ev_length);
	vValue.ev_width = m_Value.ev_width;
	if (!vValue.AllocHandle(m_BufferSize))
		throw E_INSUFMEMORY;
	vValue.LockHandle();
	char* pString = vValue.HandleToPtr();
	memcpy(pString, m_String, m_BufferSize);
	UnlockHandle();
	m_Value.ev_padding = Locked;
	m_Value.ev_handle = vValue.ev_handle;
	m_String = pString;
}

/* Operator overloading */
FoxString& FoxString::operator=(LocatorEx &pLoc)
{
	Release();
	int nErrorNo = _Load(pLoc, &m_Value);
	if (nErrorNo)
		throw nErrorNo;
	if (m_Value.ev_type == 'C')
	{
		m_BufferSize = m_Value.ev_length;
		m_String = HandleToPtr();
#if defined(_DEBUGALLOCATIONS)
		if (m_Value.ev_type == 'C')
			VfpAllocationCount++;
#endif
	}
	else
	{
		Release();
		throw E_TYPECONFLICT;
	}
	return *this;
}

FoxString& FoxString::operator=(FoxValue &pValue)
{
	if (pValue.Vartype() != 'C')
		throw E_TYPECONFLICT;

	unsigned int nSize = pValue.GetHandleSize(); 
	Size(nSize);
	Len(pValue.Len());
	Binary(pValue->ev_width == 1);
	if (Len())
		memcpy(m_String, pValue.HandleToPtr(), Len());
	if (pValue->ev_width != 1)
		m_String[Len()] = '\0';
	return *this;
}

FoxString& FoxString::operator<<(FoxObject &pObject)
{
	Release();
	int nErrorNo = _GetObjectProperty(&m_Value, pObject, pObject.Property());
	if (nErrorNo)
		throw nErrorNo;
	if (m_Value.ev_type == 'C')
	{
		m_BufferSize = m_Value.ev_length;
		LockHandle();
		m_String = HandleToPtr();
	}
	else
	{
		Release();
		throw E_TYPECONFLICT;
	}
	return *this;
}

FoxString& FoxString::operator=(char* pString)
{
	if (pString)
	{
		unsigned int nStrLen = (unsigned int)strlen(pString) + 1;
		if (m_BufferSize < nStrLen + 1)
			Size(nStrLen + 1);

		memcpy(m_String, pString, nStrLen + 1);
		m_Value.ev_length = nStrLen;
	}
	else
	{
		if (m_String)
			*m_String = '\0';
		m_Value.ev_length = 0;
	}
	return *this;
}

FoxString& FoxString::operator=(const char* pString)
{
	if (pString)
	{
		unsigned int nStrLen = (unsigned int)strlen(pString);
		if (m_BufferSize < nStrLen + 1)
			Size(nStrLen + 1);

		memcpy(m_String, pString, nStrLen + 1);
		m_Value.ev_length = nStrLen;
	}
	else
	{
		if (m_String)
			*m_String = '\0';
		m_Value.ev_length = 0;
	}
	return *this;
}

FoxString& FoxString::operator=(const CStringView pString)
{
	if (pString && pString.Data != m_String)
	{
		unsigned int nBufferSize = pString.Len + 1;
		if (m_BufferSize < nBufferSize)
			Size(nBufferSize);

		memcpy(m_String, pString.Data, pString.Len);
		m_String[pString.Len] = '\0';
		m_Value.ev_length = pString.Len;
	}
	else
	{
		if (m_String)
			*m_String = '\0';
		m_Value.ev_length = 0;
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

		nChars = WideCharToMultiByte(m_CodePage, 0, pWString.Data, (int)pWString.Len, m_String, (int)m_BufferSize, 0, 0);
		if (!nChars)
		{
			SaveWin32Error("MultiByteToWideChar", GetLastError());
			throw E_APIERROR;
		}
		m_Value.ev_length = nChars;
		m_String[nChars] = '\0';
	}
	else
	{
		if (m_String)
			*m_String = '\0';
		m_Value.ev_length = 0;
	}
	return *this;
}

FoxString& FoxString::operator+=(const CStringView pString)
{
	unsigned int nLen = pString.Len;
	// NOLINTNEXTLINE(bugprone-narrowing-conversions)
	int nDiff = m_BufferSize - (m_Value.ev_length + nLen + 1);
	if (nDiff < 0)
		Expand(-nDiff);
	memcpy(m_String + m_Value.ev_length, pString.Data, nLen);
	m_Value.ev_length += nLen;
	m_String[m_Value.ev_length] = '\0';
	return *this;
}

FoxString& FoxString::operator+=(const CWideStringView pWString)
{
	unsigned int nLen = pWString.Len * sizeof(wchar_t);
	// NOLINTNEXTLINE(bugprone-narrowing-conversions)
	int nDiff = m_BufferSize - (m_Value.ev_length + nLen + 1);
	if (nDiff < 0)
		Expand(-nDiff);

	int nChars = WideCharToMultiByte(m_CodePage, 0, pWString.Data, (int)pWString.Len, m_String + m_Value.ev_length, (int)m_BufferSize, 0, 0);
	if (!nChars)
	{
		SaveWin32Error("MultiByteToWideChar", GetLastError());
		throw E_APIERROR;
	}

	m_Value.ev_length += nChars;
	m_String[m_Value.ev_length] = '\0';
	return *this;
}


FoxString& FoxString::operator+=(const char pChar)
{
	// NOLINTNEXTLINE(bugprone-narrowing-conversions)
	int nDiff = m_BufferSize - (m_Value.ev_length + 2);
	if (nDiff < 0)
		Expand(-nDiff);
	char *pTmp = m_String + m_Value.ev_length;
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
			return pString.Len == m_Value.ev_length;
		else if (pString.Len == m_Value.ev_length)
			return memcmp(m_String, pString.Data, pString.Len) == 0;
	}
	return false;
}

/* FoxDate */
FoxDate::FoxDate(ValueEx &pVal)
{
	assert(pVal.Vartype() == 'D');
	m_Value.ev_type = 'D';
	m_Value.ev_real = pVal.ev_real;
}

FoxDate::FoxDate(SYSTEMTIME &sTime)
{
	m_Value.ev_type = 'D';
	SystemTimeToDate(sTime);
}

FoxDate::FoxDate(FILETIME &sTime)
{
	m_Value.ev_type = 'D';
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
    ULARGE_INTEGER pTime;
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
		// NOLINTNEXTLINE(bugprone-integer-division)
		m_Value.ev_real = VFPFILETIMEBASE + (double)(pTime.QuadPart / SECONDSPERDAY);
	}
}

void FoxDate::DateToSystemTime(SYSTEMTIME &sTime) const
{
	int lnA, lnB, lnC, lnD, lnE, lnM;
	DWORD lnDays;

	lnDays = static_cast<DWORD>(floor(m_Value.ev_real));

	lnA = (int)(lnDays + 32044);
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
FoxDateTime::FoxDateTime(ValueEx &pVal)
{
	assert(pVal.Vartype() == 'T');
	m_Value.ev_type = 'T';
	m_Value.ev_real = pVal.ev_real;
}

FoxDateTime::FoxDateTime(SYSTEMTIME &sTime)
{
	m_Value.ev_type = 'T';
	SystemTimeToDateTime(sTime);
}

FoxDateTime::FoxDateTime(FILETIME &sTime)
{
	m_Value.ev_type = 'T';
	FileTimeToDateTime(sTime);
}

FoxDateTime::FoxDateTime(double dTime)
{
	m_Value.ev_type = 'T';
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
	{
		TimeZoneInfo::GetTsi().ToUTC(m_Value.ev_real);
	}
	return *this;
}

FoxDateTime& FoxDateTime::ToLocal()
{
	if (m_Value.ev_real != 0.0)
	{
		TimeZoneInfo::GetTsi().ToLocal(m_Value.ev_real);
	}
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
	pTime.HighPart = (LONG)sTime.dwHighDateTime;

	if (pTime.QuadPart > MAXVFPFILETIME) //if bigger than maximum DATETIME - 9999/12/12 23:59:59
		pTime.QuadPart = MAXVFPFILETIME; //set to maximum date ..
	else if (pTime.QuadPart == 0) // empty Filetime?
		m_Value.ev_real = 0.0;
	else
	{
		// gives us seconds since 1601/01/01
		pTime.QuadPart /= NANOINTERVALSPERSECOND;
		// NOLINTNEXTLINE(bugprone-integer-division)
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

	lnA = (int)(lnDays + 32044);
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
			m_Length = (unsigned int)strlen("{ ::}");
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
			m_Length = (unsigned int)strlen("{ ::}");
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
		m_Length = (unsigned int)strlen("{ ::}");
	}
	return *this;
}

FoxDateTimeLiteral::operator CStringView()
{ 
	return CStringView(m_Literal, m_Length); 
}

FoxObject::FoxObject(ValueEx &pVal) : FoxValue('O'), m_Property(0)
{
	assert(pVal.Vartype() == 'O');
	m_Value.ev_padding = Unlocked;
	m_Value.ev_object = pVal.ev_object;
}

FoxObject::FoxObject(ParamBlkEx& parm, int nParmNo) : m_Property(0)
{
	if (parm.pCount >= nParmNo && parm(nParmNo)->ev_type == 'O')
	{
		m_Value.ev_type = 'O';
		m_Value.ev_object = parm(nParmNo)->ev_object;
		m_Value.ev_padding = Parameter;
	}
	else
	{
		m_Value.ev_type = '0';
		m_Value.ev_object = 0;
		m_Value.ev_padding = Unlocked;
	}
}

FoxObject::FoxObject(char *pExpression) : FoxValue('0'), m_Property(0)
{
	Evaluate(pExpression);
}

// TODO - simplify
void FoxObject::Release()
{
	if (m_Value.ev_object)
	{
		UnlockObject();
		if ((m_Value.ev_padding & Parameter) == 0)
			FreeObject();
		else
			m_Value.ev_object = 0;
	}
	m_Value.ev_type = '0';
	m_Value.ev_padding = Unlocked;
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
FoxMemo::FoxMemo() : m_File(0), m_Location(0)
{
	memset((void*)&m_Loc, 0, sizeof(LocatorEx));
}

FoxMemo::FoxMemo(ParamBlkEx& parm, int nParmNo)
{
	if (parm.pCount >= nParmNo && parm(nParmNo).IsMemoRef())
	{
		Attach(parm(nParmNo));
	}
	else
	{
		m_File = 0;
		m_Location = 0;
		m_Size = 0;
		memset((void*)&m_Loc, 0, sizeof(LocatorEx));
	}
}

FoxMemo::FoxMemo(FoxParameterEx &parm)
{
	if (parm.IsMemoRef())
		Attach(parm.loc);
	else
	{
		m_Location = (long)parm->ev_real; // _FindMemo(pValue.ev_long);
		m_File = parm->ev_width; // _MemoChan(m_Loc.l_where);
		m_Size = parm->ev_long; // _MemoSize(m_Loc);
	}
}

FoxMemo::FoxMemo(Value& pValue)
{
	m_Location = (long)pValue.ev_real; // _FindMemo(pValue.ev_long);
	m_File = pValue.ev_width; // _MemoChan(m_Loc.l_where);
	m_Size = pValue.ev_long; // _MemoSize(m_Loc);
}

void FoxMemo::Attach(Locator& pLoc)
{
	Attach((LocatorEx&)pLoc);
}

void FoxMemo::Attach(LocatorEx& pLoc)
{
	m_Loc.l_type = pLoc.l_type;
	m_Loc.l_where = pLoc.l_where;
	m_Loc.l_NTI = pLoc.l_NTI;
	m_Loc.l_offset = pLoc.l_offset;

	m_Location = _FindMemo(m_Loc);
	if (m_Location < 0)
		throw E_FIELDNOTFOUND;

	m_File = _MemoChan(m_Loc.l_where);
	if (m_File == -1)
	{
		SaveCustomError("_MemoChan", "Function failed for workarea %I.", m_Loc.l_where);
		throw E_APIERROR;
	}

	if (m_Location != 0)
	{
		m_Size = _MemoSize(m_Loc);
		if (m_Size < 0)
			throw m_Size;
	}
	else
		m_Size = 0;
}

void FoxMemo::Alloc(int nSize)
{
	m_Location = _AllocMemo(m_Loc,nSize);
	if (m_Location == -1)
	{
		SaveCustomError("_AllocMemo","Function failed.");
		throw E_APIERROR;
	}
}

void FoxMemo::Append(char *pData, int nLen)
{
	if (_FSeek(m_File, m_Location, FS_FROMBOF) != m_Location)
	{
		SaveCustomError("_FSeek", "Function failed.");
		throw E_APIERROR;
	}
	if (_FWrite(m_File,pData,(unsigned int)nLen) != (unsigned int)nLen)
		throw _FError();
	m_Location += nLen;
}

void FoxMemo::Read(char* pDestination, unsigned int nBytes)
{
	if (_FSeek(m_File, m_Location, FS_FROMBOF) != m_Location)
	{
		SaveCustomError("_FSeek", "Function failed.");
		throw E_APIERROR;
	}
	unsigned int nBytesRead = _FRead(m_File, pDestination, nBytes);
	if (nBytesRead != nBytes)
	{
		SaveCustomError("_FRead", "Function failed.");
		throw E_APIERROR;
	}
}

void FoxMemo::Read(FoxParameterEx& pMemo, FoxString& pDestination, bool bNullTerminate)
{
	assert(pMemo.IsMemoRef());

	long nLocation = _FindMemo(&pMemo.loc);
	if (nLocation < 0)
	{
		SaveCustomError("_FindMemo", "Function failed.");
		throw E_APIERROR;
	}

	if (nLocation != 0)
	{
		FCHAN hFile = _MemoChan(pMemo.loc.l_where);
		if (hFile == -1)
		{
			SaveCustomError("_MemoChan", "Function failed.");
			throw E_APIERROR;
		}

		long nBytes = _MemoSize(&pMemo.loc);
		if (nBytes < 0)
			throw nBytes;

		if (_FSeek(hFile, nLocation, FS_FROMBOF) != nLocation)
		{
			SaveCustomError("_FSeek", "Function failed.");
			throw E_APIERROR;
		}
		pDestination.ExtendBuffer(nBytes + (bNullTerminate ? 1 : 0));
		char* pDest = pDestination.Ptr<char*>();
		long nBytesRead = (long)_FRead(hFile, pDest, nBytes);
		if (nBytesRead != nBytes)
		{
			SaveCustomError("_FRead", "Function failed.");
			throw E_APIERROR;
		}

		pDestination.Len(nBytes);
		if (bNullTerminate)
			pDest[nBytes] = '\0';
	}
	else
	{
		pDestination.Len(0);
	}

}

void FoxMemo::Read(ValueEx& pMemo, FoxString& pDestination, bool bNullTerminate)
{
	unsigned int nBytes = pMemo.ev_long;
	long location = (long)pMemo.ev_real;
	long memochan = pMemo.ev_width;

	if (_FSeek(memochan, location, FS_FROMBOF) != location)
	{
		SaveCustomError("_FSeek", "Function failed.");
		throw E_APIERROR;
	}

	pDestination.ExtendBuffer(nBytes + (bNullTerminate ? 1 : 0));
	char* pDest = pDestination.Ptr<char*>();
	unsigned int nBytesRead = _FRead(memochan, pDest, nBytes);
	if (nBytesRead != nBytes)
	{
		SaveCustomError("_FRead", "Function failed.");
		throw E_APIERROR;
	}
	pDestination.Len(nBytes);
	if (bNullTerminate)
		pDest[nBytes] = '\0';
}

unique_ptr<char*> FoxMemo::Read(unsigned int &nLen)
{
	unsigned int nAllocBytes, nReadBytes;
	if (nLen == 0)
	{
		nAllocBytes = nReadBytes = m_Size;
	}
	else
	{
		nAllocBytes = nLen;
		nReadBytes = (long)nLen > m_Size ? m_Size : nLen;
	}

	unique_ptr<char*> pData(new(std::nothrow) char[nAllocBytes]);
	if (!pData)
		throw E_INSUFMEMORY;

	if (_FSeek(m_File, m_Location, FS_FROMBOF) != m_Location)
	{
		SaveCustomError("_FSeek", "Function failed.");
		throw E_APIERROR;
	}
	nLen = _FRead(m_File, pData, nReadBytes);
	return pData;
}

void FoxMemo::Read(char* pDestination, int nOffset, unsigned int nBytes)
{
	long location = m_Location + nOffset;
	if (_FSeek(m_File, location, FS_FROMBOF) != location)
	{
		SaveCustomError("_FSeek", "Function failed.");
		throw E_APIERROR;
	}
	unsigned int nBytesRead = _FRead(m_File, pDestination, nBytes);
	if (nBytesRead != nBytes)
	{
		SaveCustomError("_FRead", "Function failed.");
		throw E_APIERROR;
	}
}

void FoxMemo::Write(char* pData, int nOffset, unsigned int nBytes)
{
	long location = m_Location + nOffset;
	if (_FSeek(m_File, location, FS_FROMBOF) != location)
	{
		SaveCustomError("_FSeek", "Function failed.");
		throw E_APIERROR;
	}
	unsigned int nBytesWritten = _FWrite(m_File, pData, nBytes);
	if (nBytesWritten != nBytes)
	{
		SaveCustomError("_FWrite", "Function failed.");
		throw E_APIERROR;
	}
}

FoxMemo& FoxMemo::operator=(FoxString &pString)
{
	/* if data is smaller or equal to 65000, we can use _DBReplace */
	if (pString.Len() <= 65000)
	{
		int nErrorNo = _DBReplace(m_Loc, pString);
		if (nErrorNo)
			throw nErrorNo;
	}
	else
	{
		Alloc((int)pString.Len());
		if (_FSeek(m_File, m_Location, FS_FROMBOF) != m_Location)
		{
			SaveCustomError("_FSeek", "Function failed.");
			throw E_APIERROR;
		}
		if (_FWrite(m_File,pString.Ptr<LPSTR>(),pString.Len()) != pString.Len())
			throw _FError();
	}
	return *this;
}

FoxMemo& FoxMemo::operator=(CStringView& pString)
{
	Alloc((int)pString.Len);
	if (_FSeek(m_File, m_Location, FS_FROMBOF) != m_Location)
	{
		SaveCustomError("_FSeek", "Function failed.");
		throw E_APIERROR;
	}
	if (_FWrite(m_File, pString.Data, pString.Len) != pString.Len)
		throw _FError();
	return *this;
}

int _fastcall FoxMemo::Alloc(Locator* pLoc, int nSize, long* nLocation)
{
	*nLocation = _AllocMemo(pLoc, nSize);
	if (*nLocation == -1)
	{
		SaveCustomError("_AllocMemo", "Function failed.");
		return E_APIERROR;
	}
	return 0;
}

int _fastcall FoxMemo::Chan(int nWorkarea, FCHAN* nChan)
{
	*nChan = _MemoChan(nWorkarea);
	if (*nChan == -1)
	{
		SaveCustomError("_MemoChan", "Function failed.");
		return E_APIERROR;
	}
	return 0;
}

int _fastcall FoxMemo::Read(Value* pValue, char* pData)
{
	FCHAN hFile = pValue->ev_width;
	int nLocation = (int)pValue->ev_real;
	if (_FSeek(hFile, nLocation, FS_FROMBOF) != nLocation)
	{
		SaveCustomError("_FSeek", "Function failed.");
		return E_APIERROR;
	}
	if (_FRead(hFile, pData, (unsigned int)pValue->ev_long) != (unsigned int)pValue->ev_long)
	{
		SaveCustomErrorEx("_FRead", "Function failed.", _FError());
		return E_APIERROR;
	}
	else
		return 0;
}

int _fastcall FoxMemo::Read(Value* pValue, char* pData, unsigned int nLen, int nOffset)
{
	FCHAN hFile = pValue->ev_width;
	int nLocation = ((int)pValue->ev_real) + nOffset;
	if (_FSeek(hFile, nLocation, FS_FROMBOF) != nLocation)
	{
		SaveCustomError("_FSeek", "Function failed.");
		return E_APIERROR;
	}
	if (_FRead(hFile, pData, nLen) != nLen)
	{
		SaveCustomErrorEx("_FRead", "Function failed.", _FError());
		return E_APIERROR;
	}
	else
		return 0;
}

int _fastcall FoxMemo::Read(Locator* pLoc, char** pData, int* nErrorNo)
{
	FCHAN hFile;
	int nLocation, nLen;

	nLocation = _FindMemo(pLoc);
	if (nLocation < 0)
	{
		*nErrorNo = E_INVALIDPARAMS;
		return -1;
	}

	hFile = _MemoChan(pLoc->l_where);
	if (hFile == -1)
	{
		SaveCustomError("_MemoChan", "Function failed for workarea %I.", pLoc->l_where);
		*nErrorNo = E_APIERROR;
		return -1;
	}

	nLen = _MemoSize(pLoc);
	if (nLen <= 0)
	{
		if (nLen == 0)
			return 0;
		else
		{
			*nErrorNo = nLen;
			return -1;
		}
	}

	*pData = (char*)malloc(nLen);
	if (!*pData)
	{
		*nErrorNo = E_INSUFMEMORY;
		return -1;
	}

	if (_FSeek(hFile, nLocation, FS_FROMBOF) != nLocation)
	{
		SaveCustomError("_FSeek", "Function failed.");
		*nErrorNo = E_APIERROR;
		return -1;
	}

	return _FRead(hFile, *pData, nLen);
}

int _fastcall FoxMemo::Replace(Locator* pLoc, char* pData, int nLen)
{
	FCHAN hFile;
	long nLocation;

	hFile = _MemoChan(pLoc->l_where);
	if (hFile == -1)
		return E_FIELDNOTFOUND;

	nLocation = _AllocMemo(pLoc, nLen);
	if (nLocation == -1)
	{
		SaveCustomError("_AllocMemo", "Function failed.");
		return E_APIERROR;
	}

	if (_FSeek(hFile, nLocation, FS_FROMBOF) != nLocation)
	{
		SaveCustomError("_FSeek", "Function failed.");
		return E_APIERROR;
	}

	if (_FWrite(hFile, pData, (unsigned int)nLen) != (unsigned int)nLen)
	{
		SaveCustomErrorEx("_FWrite", "Function failed.", _FError());
		return E_APIERROR;
	}
	return 0;
}

int _fastcall FoxMemo::Replace(Locator* pLoc, char* pData, int nLen, FCHAN hFile)
{
	long nLocation;
	nLocation = _AllocMemo(pLoc, nLen);
	if (nLocation == -1)
	{
		SaveCustomError("_AllocMemo", "Function failed.");
		return E_APIERROR;
	}

	if (_FSeek(hFile, nLocation, FS_FROMBOF) != nLocation)
	{
		SaveCustomError("_FSeek", "Function failed.");
		return E_APIERROR;
	}

	if (_FWrite(hFile, pData, (unsigned int)nLen) != (unsigned int)nLen)
	{
		SaveCustomErrorEx("_FWrite", "Function failed.", _FError());
		return E_APIERROR;
	}
	return 0;
}

int _fastcall FoxMemo::Append(char* pData, int nLen, FCHAN hFile, long* nLocation)
{
	if (nLocation)
	{
		if (_FSeek(hFile, *nLocation, FS_FROMBOF) != *nLocation)
		{
			SaveCustomError("_FSeek", "Function failed.");
			return E_APIERROR;
		}
	}
	if (_FWrite(hFile, pData, (unsigned int)nLen) != (unsigned int)nLen)
	{
		SaveCustomErrorEx("_FWrite", "Function failed.", _FError());
		return E_APIERROR;
	}
	if (nLocation)
		*nLocation += nLen;
	return 0;
}

/* FoxArray */
FoxArray::FoxArray()
{
	Init(false);
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
	{
		LocatorEx& pRef = pVal;
		Init(pRef);
	}
	else
	{
		SaveCustomErrorEx("FoxArray:Constructor", "Invalid parameter type '%s' to initialize array.", E_VFP_INVALIDPARAMS, pVal->Vartype());
		throw E_APIERROR;
	}
}

FoxArray::FoxArray(FoxParameterEx& pVal, FoxArray::Initialisation eInit)
{
	if (pVal->Vartype() == 'C' && pVal->Len() > 0)
	{
		Init(pVal.val, eInit);
	}
	else
	{
		SaveCustomErrorEx("FoxArray:Constructor", "Invalid parameter type '%s' to initialize array.", E_VFP_INVALIDPARAMS, pVal->Vartype());
		throw E_APIERROR;
	}
}

FoxArray::FoxArray(LocatorEx &pLoc)
{
	Init(pLoc);
}

FoxArray::FoxArray(ParamBlkEx& parm, int nParmNo)
{ 
	if (parm.PCount() >= nParmNo)
	{
		FoxParameterEx& pParameter = parm(nParmNo);
		if (pParameter->Vartype() == 'C')
		{
			Init(pParameter.val);
			return;
		}
		else if (pParameter->Vartype() == 'R')
		{
			LocatorEx& pRef = pParameter;
			Init(pRef);
			return;
		}
	}
	Init(false);
}

FoxArray::FoxArray(ParamBlkEx& parm, int nParmNo, FoxArray::BindingSource eBind)
{
	if (parm.PCount() >= nParmNo)
	{
		FoxParameterEx& pParameter = parm(nParmNo);
		if (eBind == BindByName && pParameter->Vartype() == 'C')
		{
			Init(pParameter.val);
			return;
		}
		else if (eBind == BindByReference && pParameter->Vartype() == 'R')
		{
			LocatorEx& pRef = pParameter;
			Init(pRef);
			return;
		}
	}
	Init(false);
}

void FoxArray::Init(bool bPassed)
{
	m_Loc.l_NTI = 0;
	m_Loc.l_where = bPassed ? 0 : 1;
	m_Rows = 0;
	m_Dims = 0;
	m_AutoGrow = 0;
	m_AutoOverflow = 0;
}

void FoxArray::Init(LocatorEx &pLoc)
{
	m_Loc.l_type = pLoc.l_type;
	m_Loc.l_NTI = pLoc.l_NTI;
	m_Loc.l_where = -1;
	m_Loc.l_offset = 0;
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
		SaveCustomErrorEx("FoxArray:Init", "The parameter has to be the variable name as a string no longer than 128 characters", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}
	m_Name.Append(pVal);
	m_Name.SetFormatBase();
	m_Loc.l_NTI = 0;
	m_Loc.l_where = 0;
	m_AutoGrow = 0;
	m_AutoOverflow = 0;
	if (nRows > 0)
		Dimension(nRows, nDims);
	else
	{
		FindArrayResult res = FindArray();
		if (res == ArrayNotFound)
			Init(true);
		else if (res == NotAnArray)
		{
			CStringView pName = m_Name.StringView();
			SaveCustomErrorEx("FoxArray:Init", "The passed variable '%V' is not an array", E_NOTANARRAY, &pName);
			throw E_APIERROR;
		}
	}
}

void FoxArray::Init(ValueEx &pVal, FoxArray::Initialisation eInit)
{
	if (pVal.Vartype() != 'C' || pVal.ev_handle == 0 || pVal.ev_length == 0 || pVal.ev_length > VFP_MAX_VARIABLE_NAME)
	{
		SaveCustomErrorEx("FoxArray:Init", "Invalid parameter to initialize array: the parameter has to be the variable name as a string no longer than 128 characters", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}
	m_Name.Append(pVal); 
	m_Name.SetFormatBase();
	m_Loc.l_NTI = 0;
	m_Loc.l_where = 0;
	m_AutoGrow = 0;
	m_AutoOverflow = 0;

	FindArrayResult res = FindArray();
	if (res == ArrayNotFound && eInit == RaiseErrorIfNotExists)
	{
		CStringView pName = m_Name.StringView();
		SaveCustomErrorEx("FoxArray:Init", "The passed variable '%V' is not found", E_VARIABLENOTFOUND, &pName);
		throw E_APIERROR;
	}
	else if (res == NotAnArray)
	{
		CStringView pName = m_Name.StringView();
		SaveCustomErrorEx("FoxArray:Init", "The passed variable '%V' is not an array", E_NOTANARRAY, &pName);
		throw E_APIERROR;
	}
}

void FoxArray::Dimension(unsigned int nRows, unsigned int nDims)
{
	assert(m_Loc.l_where <= 0);
	ReDimension(nRows, nDims);
	if (m_Loc.l_NTI == 0)
		FindArray();
}

void FoxArray::Dimension(ValueEx &pVal, unsigned int nRows, unsigned int nDims)
{
	assert(pVal.Vartype() == 'C' && pVal.ev_handle != 0);
	CStringView pName(pVal.HandleToPtr(), pVal.Len());
	Dimension(pName, nRows, nDims);
}

void FoxArray::Dimension(CStringView pName, unsigned int nRows, unsigned int nDims)
{
	if (pName.Len == 0 || pName.Len > VFP_MAX_VARIABLE_NAME)
	{
		SaveCustomErrorEx("FoxArray:Dimension", "Invalid array name '%V' passed to initialize array, name to short or to long.", E_VFP_INVALIDPARAMS, &pName);
		throw E_APIERROR;
	}
	m_Loc.l_where = 0;
	m_Name = pName;
	m_Name.SetFormatBase();
	Dimension(nRows,nDims);
}

FoxArray::FindArrayResult FoxArray::FindArray()
{
	NTI nVarNti = _NameTableIndex(m_Name);
	if (nVarNti == -1)
		return ArrayNotFound;

	bool found = _FindVar(nVarNti, -1, m_Loc) > 0;
	if (found)
	{
		if (m_Loc.l_subs == 0)
			return NotAnArray;

		m_Rows = m_Loc.l_sub1;
		m_Dims = m_Loc.l_sub2;
		m_Loc.l_subs = m_Loc.l_sub2 > 1 ? 2 : 1;
	}
	return found ? ArrayFound : ArrayNotFound;
}

void FoxArray::InitArray()
{
	assert(m_Loc.l_NTI > 0);
	int nErrorNo;
	USHORT tmpSubs;
	ValueEx vFalse;
	vFalse.SetLogical(false);
	tmpSubs = m_Loc.l_subs;
	m_Loc.l_subs = 0;
	nErrorNo = _Store(m_Loc, vFalse);
	if (nErrorNo)
		throw nErrorNo;
	m_Loc.l_subs = tmpSubs;
}

void FoxArray::ReDimension(unsigned int nRows, unsigned int nDims)
{
	assert(m_Name.Len());
	CStrBuilder<256> pExeBuffer;
	CStringView vName = m_Name;
	if (nDims > 1)
		pExeBuffer.Format("DIMENSION %V[%U,%U]", &vName, nRows, nDims);
	else
		pExeBuffer.Format("DIMENSION %V[%U]", &vName, nRows);
	Execute(pExeBuffer);
	m_Loc.l_subs = nDims > 1 ? 2 : 1;
	m_Rows = nRows;
	m_Dims = nDims;
}

unsigned int FoxArray::ALen(unsigned int &nDims)
{
	if (m_Loc.l_where == 0)
	{	
		FindArrayResult res = FindArray();
		if (res == ArrayNotFound)
			throw E_VARIABLENOTFOUND;
		else if (res == NotAnArray)
			throw E_NOTANARRAY;
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

FoxArray& FoxArray::Reset()
{
	m_Loc.l_sub1 = 0;
	m_Loc.l_sub2 = 0;
	return *this;
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
	if (FindArray() == ArrayNotFound)
		throw E_VARIABLENOTFOUND;
}

int FoxArray::CompactOverflow()
{
	if (m_AutoOverflow > 1)
	{
		m_Name.ResetToFormatBase();
		CStrBuilder<512> pExeBuffer;
		CStrBuilder<VFP_MAX_VARIABLE_NAME + 1> pArrayName;
		CStringView pArrayView, pNameView = m_Name;
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
			pArrayView = pArrayName.StringView();
			pExeBuffer.Format("ACOPY(%V,%V,1,%U,%U)", &pArrayView, &pNameView, min(nMaxElementsPerArray, nTotalElementsToCopy), nFirstDestinationElement);
			Execute(pExeBuffer);
			// release the intermediary arrays
			pExeBuffer.Format("RELEASE %V", &pArrayView);
			Execute(pExeBuffer);
			nFirstDestinationElement += nMaxElementsPerArray;
			nTotalElementsToCopy -= nMaxElementsPerArray;
		}
		return (int)nTotalRows;
	}
	else if (m_AutoGrow && m_Loc.l_sub1 && m_Loc.l_sub1 < m_Rows)
		ReDimension(m_Loc.l_sub1, m_Dims);
	return m_Loc.l_sub1;
}

CStringView FoxArray::Name()
{
	return m_Name;
}

Value FoxArray::EvaluateElement(unsigned int nRow, unsigned int nDim)
{
	int nErrorNo;
	Value pValue;
	pValue.ev_type = '0';
	if (nDim == 0)
		m_Name.AppendFormatBase("[%U]", nRow);
	else
		m_Name.AppendFormatBase("[%U,%U]", nRow, nDim);

	nErrorNo = _Evaluate(&pValue, m_Name);
	m_Name.ResetToFormatBase();
	if (nErrorNo)
	{
		if (nErrorNo == 173)
		{
			SaveCustomErrorEx("FoxArray::EvaluateElement", "Passed variable name is not an array.", E_NOTANARRAY);
			throw E_APIERROR;
		}
		else
			throw nErrorNo;
	}
	return pValue;
}

FoxCursor::~FoxCursor()
{
	if (m_AutoFLocked)
		Unlock();

	if (m_pFieldLocs)
		delete[] m_pFieldLocs;
}

void FoxCursor::Create(CStringView pCursorName, CStringView pFields)
{
	if (pCursorName.Len == 0 || pCursorName.Len > VFP_MAX_CURSOR_NAME)
	{
		SaveCustomErrorEx("FoxCursor:Create", "Passed cursorname is invalid, len = 0 or > VFP_MAX_CURSOR_NAME (128).", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}
	
	char* pField;
	int nErrorNo;
	FoxVariable pFieldArrayRef;
	FoxValue vValue;
	FoxArray pFieldArray;
	CStrBuilder<4096> pExeBuffer;

	// create the cursor
	pExeBuffer.Format("CREATE CURSOR %V (%V)", &pCursorName, &pFields);
	nErrorNo = _Execute(pExeBuffer);
	if (nErrorNo)
	{
		SaveCustomErrorEx("FoxCursor::Create", "CREATE CURSOR command failed.", nErrorNo);
		throw E_APIERROR;
	}
	// get the workarea
	vValue.Evaluate("SELECT(0)");
	m_WorkArea = (SHORT)vValue->ev_long;
	
	// get fieldcount
	pExeBuffer.Format("_VFP2C_FIELDS_%U", this);
	pFieldArray.Dimension(pExeBuffer, 1, 18);
	pFieldArrayRef.Attach(pExeBuffer);
	pExeBuffer.Format("AFIELDS(_VFP2C_FIELDS_%U)", this);
	vValue.Evaluate(pExeBuffer);
	m_FieldCnt = vValue->ev_long;
	if (m_pFieldLocs)
		delete[] m_pFieldLocs;
	m_pFieldLocs = new(std::nothrow) FieldLocatorEx[m_FieldCnt];
	if (m_pFieldLocs == 0)
		throw E_INSUFMEMORY;

	// get locators to each field
	NTI nVarNti;
	for (unsigned int nFieldNo = 1; nFieldNo <= m_FieldCnt; nFieldNo++)
	{
		vValue = pFieldArray(nFieldNo, 1);
		vValue.NullTerminate();
		pField = vValue.HandleToPtr();
		nVarNti = _NameTableIndex(pField);
		vValue.FreeHandle();
		vValue->ev_type = '0';
		/*
		vValue = pFieldArray(nFieldNo, 18); // autoinc value
		if (vValue.Vartype() == 'N' && vValue->ev_long > 0)
		{
			m_AutoIncValue = true;
			m_AppendCommand.Format("APPEND BLANK IN %I", m_WorkArea);
		}
		*/

		if (nVarNti == -1)
			throw E_FIELDNOTFOUND;

		if (!_FindVar(nVarNti, m_WorkArea, &m_pFieldLocs[nFieldNo-1]))
			throw E_FIELDNOTFOUND;

	}
}

int FoxCursor::Attach(CStringView pCursorName, CStringView pFields)
{
	FoxValue vValue;
	CStrBuilder<256> pExeBuffer;
	CStrBuilder<VFP_MAX_COLUMN_NAME + 1> pFieldname;

	pExeBuffer.Format("SELECT('%V')", &pCursorName);
	vValue.Evaluate(pExeBuffer);
	// if workarea == 0 the cursor does not exist
	if (vValue->ev_long == 0)
		return E_ALIASNOTFOUND;

	AutoWorkAreaRestore m_WorkAreaRestore((SHORT)vValue->ev_long, 0, true);
	m_FieldCnt = pFields.GetWordCount(',');
	if (m_pFieldLocs)
		delete[] m_pFieldLocs;
	m_pFieldLocs = new(std::nothrow) FieldLocatorEx[m_FieldCnt];
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
			return E_FIELDNOTFOUND;

		if (!_FindVar(nVarNti, m_WorkArea, &m_pFieldLocs[nFieldNo]))
			return E_FIELDNOTFOUND;

		pFields = pFields + (pField.Len + 1);
	}

	m_WorkArea = (SHORT)vValue->ev_long;
	return 0;
}

int FoxCursor::Attach(SHORT nWorkArea, CStringView pFields)
{
	CStrBuilder<64> pExeBuffer;
	CStrBuilder<VFP_MAX_COLUMN_NAME + 1> pFieldName;

	// select the workarea
	AutoWorkAreaRestore m_WorkAreaRestore(nWorkArea, 0, true);
	m_FieldCnt = pFields.GetWordCount(',');
	if (m_pFieldLocs)
		delete[] m_pFieldLocs;
	m_pFieldLocs = new(std::nothrow) FieldLocatorEx[m_FieldCnt];
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
			return E_FIELDNOTFOUND;

		if (!_FindVar(nVarNti, m_WorkArea, &m_pFieldLocs[nFieldNo]))
			return E_FIELDNOTFOUND;

		pFields = pFields + (pField.Len + 1);
	}
	
	m_WorkArea = nWorkArea;
	return 0;
}

int FoxCursor::Attach(CStringView pCursorName, CStringView pFields, int& nFieldStatus, int nRequiredFields)
{
	NTI nVarNti;
	int nErrorNo = 0;
	FoxValue vValue;
	CStrBuilder<256> pExeBuffer;
	CStrBuilder<VFP_MAX_COLUMN_NAME + 1> pFieldname;

	pExeBuffer.Format("SELECT('%V')", &pCursorName);
	vValue.Evaluate(pExeBuffer);
	// if workarea == 0 the cursor does not exist
	if (vValue->ev_long == 0)
		return E_ALIASNOTFOUND;

	AutoWorkAreaRestore m_WorkAreaRestore((SHORT)vValue->ev_long, 0, true);
	m_FieldCnt = pFields.GetWordCount(',');
	if (m_pFieldLocs)
		delete[] m_pFieldLocs;
	m_pFieldLocs = new(std::nothrow) FieldLocatorEx[m_FieldCnt];
	if (m_pFieldLocs == 0)
		throw E_INSUFMEMORY;

	nFieldStatus = 0;
	// get locators to each field
	for (unsigned int nFieldNo = 0; nFieldNo < m_FieldCnt; nFieldNo++)
	{
		CStringView pField = pFields.GetWordNum(1, ',');
		pFieldname = pField.Alltrim();
		nVarNti = _NameTableIndex(pFieldname);
		if (nVarNti == -1 || !_FindVar(nVarNti, m_WorkArea, &m_pFieldLocs[nFieldNo]))
		{
			if (nRequiredFields & (1 << nFieldNo))
				nErrorNo = E_FIELDNOTFOUND;
		}
		else
			nFieldStatus |= (1 << nFieldNo);

		pFields = pFields + (pField.Len + 1);
	}

	if (nErrorNo == 0)
		m_WorkArea = (SHORT)vValue->ev_long;

	return nErrorNo;
}

int FoxCursor::Attach(SHORT nWorkArea, CStringView pFields, int& nFieldStatus, int nRequiredFields)
{
	NTI nVarNti;
	int nErrorNo = 0;
	CStrBuilder<64> pExeBuffer;
	CStrBuilder<VFP_MAX_COLUMN_NAME + 1> pFieldName;

	// select the workarea
	AutoWorkAreaRestore m_WorkAreaRestore(nWorkArea, 0, true);
	m_FieldCnt = pFields.GetWordCount(',');
	if (m_pFieldLocs)
		delete[] m_pFieldLocs;
	m_pFieldLocs = new(std::nothrow) FieldLocatorEx[m_FieldCnt];
	if (m_pFieldLocs == 0)
		throw E_INSUFMEMORY;

	nFieldStatus = 0;
	// get locators to each field
	for (unsigned int nFieldNo = 0; nFieldNo < m_FieldCnt; nFieldNo++)
	{
		CStringView pField = pFields.GetWordNum(1, ',');
		pFieldName = pField.Alltrim();
		nVarNti = _NameTableIndex(pFieldName);
		if (nVarNti == -1 || !_FindVar(nVarNti, m_WorkArea, &m_pFieldLocs[nFieldNo]))
		{
			if (nRequiredFields & (1 << nFieldNo))
				nErrorNo = E_FIELDNOTFOUND;
		}
		else
			nFieldStatus |= (1 << nFieldNo);

		pFields = pFields + (pField.Len + 1);
	}

	if (nErrorNo == 0)
		m_WorkArea = nWorkArea;

	return nErrorNo;
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

	m_pStrings = new(std::nothrow) char*[nRows];
	if (m_pStrings == 0)
		throw E_INSUFMEMORY;
    ZeroMemory(m_pStrings, sizeof(char*)*nRows);

    m_pValues = new(std::nothrow) FoxValue[nRows];
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

			pValue->NullTerminate();
			pValue->LockHandle();
			m_pStrings[xj] = pValue->HandleToPtr();
			pValue++;
		}
	}
	return *this;
}
