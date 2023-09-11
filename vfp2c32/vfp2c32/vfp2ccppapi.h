#ifndef _VFP2CCPPAPI_H__
#define _VFP2CCPPAPI_H__

const int VFP_MAX_ARRAY_ROWS	= 65000;
const int VFP_MAX_PROPERTY_NAME	= 253;
const int VFP_MAX_VARIABLE_NAME	= 128;
const int VFP_MAX_CURSOR_NAME	= 128;
const int VFP_MAX_COLUMN_NAME	= 128;
const int VFP_MAX_CHARCOLUMN	= 254;
const int VFP_MAX_COLUMNS		= 255;

// max fieldname = max cursorname + "." + max columnname
const int VFP_MAX_FIELDNAME		= VFP_MAX_CURSOR_NAME + 1 + VFP_MAX_COLUMN_NAME;
const int VFP_MAX_DATE_LITERAL	= 32;

/* thin wrappers around LCK functions which throw an exception on error */
inline void Evaluate(Value &pVal, char *pExpression)
{
	int nErrorNo;
	if (nErrorNo = _Evaluate(&pVal, pExpression))
		throw nErrorNo;
#if defined(_DEBUGALLOCATIONS)
	if (pVal.ev_type == 'C')
		VfpAllocationCount++;
#endif
}

inline void Execute(char *pExpression)
{
	int nErrorNo;
	if (nErrorNo = _Execute(pExpression))
		throw nErrorNo;
}

/* overloaded Return function to return values to FoxPro without thinking about type issues */
inline void Return(char *pString) { _RetChar(pString); }
inline void _stdcall Return(__int8 nValue) { _RetInt(nValue, 4); }
inline void _stdcall Return(unsigned __int8 nValue) { _RetInt(nValue, 3); }
inline void _stdcall Return(short nValue) { _RetInt(nValue, 6); }
inline void _stdcall Return(unsigned short nValue) { _RetInt(nValue, 5); }
inline void _stdcall Return(int nValue) { _RetInt(nValue, 11); }
inline void _stdcall Return(long nValue) { _RetInt(nValue, 11); }
inline void _stdcall Return(bool bValue) { _RetLogical(bValue); }
inline void _stdcall Return(CCY nValue) { _RetCurrency(nValue, 21); }
inline void _stdcall Return(CCY nValue, int nWidth) { _RetCurrency(nValue, nWidth); }
inline void _stdcall Return(Value& pVal) { _RetVal(&pVal); }
inline void _stdcall ReturnARows(Locator& pLoc) { _RetInt(pLoc.l_sub1, 5); }
inline void _stdcall ReturnNull() { ValueEx vRetval; vRetval.SetNull(); _RetVal(vRetval); }

#if defined(_WIN64)
inline void Return(void* pPointer) { ValueEx vRetVal; vRetVal.SetPointer(pPointer); _RetVal(vRetVal); }
inline void _stdcall Return(unsigned int nValue) { ValueEx vRetVal; vRetVal.SetUInt(nValue); _RetVal(vRetVal); }
inline void _stdcall Return(unsigned long nValue) { ValueEx vRetVal; vRetVal.SetUInt(nValue); _RetVal(vRetVal); }
inline void _stdcall Return(__int64 nValue) { ValueEx vRetVal; vRetVal.SetInt64(nValue); _RetVal(vRetVal); }
inline void _stdcall Return(unsigned __int64 nValue) { ValueEx vRetVal; vRetVal.SetUInt64(nValue); _RetVal(vRetVal); }
inline void _stdcall Return(double nValue) { ValueEx vRetVal; vRetVal.SetDouble(nValue); _RetVal(vRetVal); }
inline void _stdcall Return(double nValue, int nDecimals) { ValueEx vRetVal; vRetVal.SetDouble(nValue, nDecimals); _RetVal(vRetVal); }
inline void _stdcall Return(float nValue) { ValueEx vRetVal; vRetVal.SetFloat(nValue); _RetVal(vRetVal); }
inline void _stdcall Return(float nValue, int nDecimals) { ValueEx vRetVal; vRetVal.SetFloat(nValue, nDecimals); _RetVal(vRetVal); }
#else
inline void _stdcall Return(void *pPointer){ _RetInt(reinterpret_cast<int>(pPointer), 11); }
inline void _stdcall Return(unsigned int nValue) { _RetFloat(nValue, 10, 0); }
inline void _stdcall Return(unsigned long nValue) { _RetFloat(nValue, 10, 0); }
inline void _stdcall Return(__int64 nValue) { _RetFloat(static_cast<double>(nValue), 20, 0); }
inline void _stdcall Return(unsigned __int64 nValue) { _RetFloat(static_cast<double>(nValue), 20, 0); }
inline void _stdcall Return(double nValue) { _RetFloat(nValue, 20, 16); }
inline void _stdcall Return(double nValue, int nDecimals) { _RetFloat(nValue, 20, nDecimals); }
inline void _stdcall Return(float nValue) { _RetFloat(nValue, 20, 7); }
inline void _stdcall Return(float nValue, int nDecimals) { _RetFloat(nValue, 20, nDecimals); }
#endif

void _stdcall ReturnIDispatch(void* pObject);

/* unsigned & signed __int64 helper functions */
inline __int64 _stdcall Value2Int64(ValueEx &pVal)
{
	if (pVal.Vartype() == 'Y')
		return pVal.ev_currency.QuadPart;
	else if (pVal.Vartype() == 'C')
	{
		if (pVal.ev_width == 1 && pVal.Len() == 8)
			return *reinterpret_cast<__int64*>(pVal.HandleToPtr());
		else
			return StringToInt64(pVal.HandleToPtr(), pVal.Len());
	}
	else if (pVal.Vartype() == 'N')
		return static_cast<__int64>(pVal.ev_real);
	else if (pVal.Vartype() == 'I')
		return static_cast<__int64>(pVal.ev_long);
	else
		throw E_INVALIDPARAMS;
}

inline unsigned __int64 _stdcall Value2UInt64(ValueEx &pVal)
{
	if (pVal.Vartype() == 'Y')
		return static_cast<unsigned __int64>(pVal.ev_currency.QuadPart);
	else if (pVal.Vartype() == 'C')
	{
		if (pVal.ev_width == 1 && pVal.Len() == 8)
			return *reinterpret_cast<unsigned __int64*>(pVal.HandleToPtr());
		else
			return StringToUInt64(pVal.HandleToPtr(), pVal.Len());
	}
	else if (pVal.Vartype() == 'N')
		return static_cast<unsigned __int64>(pVal.ev_real);
	else if (pVal.Vartype() == 'I')
		return static_cast<unsigned __int64>(pVal.ev_long);
	else
		throw E_INVALIDPARAMS;
}

inline void _stdcall ReturnInt64AsCurrency(__int64 nValue)
{
	CCY nRetval;
	nRetval.QuadPart = nValue;
	Return(nRetval);
}

inline void _stdcall ReturnInt64AsCurrency(unsigned __int64 nValue)
{
	CCY nRetval;
	nRetval.QuadPart = static_cast<__int64>(nValue);
	Return(nRetval);
}

inline void _stdcall ReturnInt64AsBinary(__int64 nValue)
{
	ValueEx vRetval;
	vRetval.SetString(sizeof(__int64));
	if (!vRetval.AllocHandle(sizeof(__int64)))
		throw E_INSUFMEMORY;
	*reinterpret_cast<__int64*>(vRetval.HandleToPtr()) = nValue;
	Return(vRetval);
}

inline void _stdcall ReturnInt64AsBinary(unsigned __int64 nValue)
{
	ValueEx vRetval;
	vRetval.SetString(sizeof(unsigned __int64));
	if (!vRetval.AllocHandle(sizeof(unsigned __int64)))
		throw E_INSUFMEMORY;
	*reinterpret_cast<unsigned __int64*>(vRetval.HandleToPtr()) = nValue;
	Return(vRetval);
}

inline void _stdcall ReturnInt64AsString(__int64 nValue)
{
	char aRetval[VFP2C_MAX_BIGINT_LITERAL+1];
	Int64ToStrBuffer(aRetval, nValue);
	Return(aRetval);
}

inline void _stdcall ReturnInt64AsString(unsigned __int64 nValue)
{
	char aRetval[VFP2C_MAX_BIGINT_LITERAL+1];
	UInt64ToStrBuffer(aRetval, nValue);
	Return(aRetval);
}

inline void _stdcall ReturnInt64AsDouble(__int64 nValue)
{
	_RetFloat(static_cast<double>(nValue), 20, 0);
}

inline void _stdcall ReturnInt64AsDouble(unsigned __int64 nValue)
{
	_RetFloat(static_cast<double>(nValue), 20, 0);
}

/* table function wrappers */
inline int AppendBlank(int nWorkarea = -1) { return _DBAppend(nWorkarea, 0); }
inline int AppendBlank(Locator &pLoc) { return _DBAppend(pLoc.l_where, 0); }
inline int AppendCarry(int nWorkarea = -1) { return _DBAppend(nWorkarea, 1); }
inline int AppendCarry(Locator &pLoc) { return _DBAppend(pLoc.l_where, 1); }
inline int Append(int nWorkarea = -1) { return _DBAppend(nWorkarea, -1); }
inline int Append(Locator &pLoc) { return _DBAppend(pLoc.l_where, -1); }
inline long GoTop(int nWorkarea = -1) { return _DBRewind(nWorkarea); }
inline long GoTop(Locator &pLoc) { return _DBRewind(pLoc.l_where); }
inline long GoBottom(int nWorkarea = -1) { return _DBUnwind(nWorkarea); }
inline long GoBottom(Locator &pLoc) { return _DBUnwind(pLoc.l_where); }
inline long Skip(int nRecords, int nWorkarea = -1) { return _DBSkip(nWorkarea, nRecords); }
inline long Skip(int nRecords, Locator &pLoc) { return _DBSkip(pLoc.l_where, nRecords); }
inline long RecNo(int nWorkarea = -1) { return _DBRecNo(nWorkarea); }
inline long RecNo(Locator &pLoc) { return _DBRecNo(pLoc.l_where); }
inline long RecCount(int nWorkarea = -1) { return _DBRecCount(nWorkarea); }
inline long RecCount(Locator &pLoc) { return _DBRecCount(pLoc.l_where); }
inline int Go(long nRecord, int nWorkarea = -1) { return _DBRead(nWorkarea, nRecord); }
inline int Go(long nRecord, Locator &pLoc) { return _DBRead(pLoc.l_where, nRecord); }
inline int RLock(int nWorkarea = -1) { return _DBLock(nWorkarea,DBL_RECORD); }
inline int RLock(Locator &pLoc) { return _DBLock(pLoc.l_where,DBL_RECORD); }
inline int FLock(int nWorkarea = -1) { return _DBLock(nWorkarea,DBL_FILE); }
inline int FLock(Locator &pLoc) { return _DBLock(pLoc.l_where,DBL_FILE); }
inline void Unlock(int nWorkarea = -1) { return _DBUnLock(nWorkarea); }
inline void Unlock(Locator &pLoc) { return _DBUnLock(pLoc.l_where); }
inline bool Bof(int nWorkarea = -1) { return (_DBStatus(nWorkarea) & DB_BOF) > 0; }
inline bool Bof(Locator &pLoc) { return (_DBStatus(pLoc.l_where) & DB_BOF) > 0; }
inline bool Eof(int nWorkarea = -1) { return (_DBStatus(nWorkarea) & DB_EOF) > 0; }
inline bool Eof(Locator &pLoc) { return (_DBStatus(pLoc.l_where) & DB_EOF) > 0; }
inline bool RLocked(int nWorkarea = -1) { return (_DBStatus(nWorkarea) & DB_RLOCKED) > 0; }
inline bool RLocked(Locator &pLoc) { return (_DBStatus(pLoc.l_where) & DB_RLOCKED) > 0; }
inline bool FLocked(int nWorkarea = -1) { return (_DBStatus(nWorkarea) & DB_FLOCKED) > 0; }
inline bool FLocked(Locator &pLoc) { return (_DBStatus(pLoc.l_where) & DB_FLOCKED) > 0; }
inline bool Exclusiv(int nWorkarea = -1) { return (_DBStatus(nWorkarea) & DB_EXCLUSIVE) > 0; }
inline bool Exclusiv(Locator &pLoc) { return (_DBStatus(pLoc.l_where) & DB_EXCLUSIVE) > 0; }
inline bool Readonly(int nWorkarea = -1) { return (_DBStatus(nWorkarea) & DB_READONLY) > 0; }
inline bool Readonly(Locator &pLoc) { return (_DBStatus(pLoc.l_where) & DB_READONLY) > 0; }
inline int DBStatus(int nWorkarea = -1) { return _DBStatus(nWorkarea); }
inline int DBStatus(Locator &pLoc) { return _DBStatus(pLoc.l_where); }

inline int AppendRecords(unsigned int nRecords, int nWorkArea = -1)
{
	int nErrorNo = 0;
	if (!FLock(nWorkArea))
		return E_LOCKFAILED;
	while (nRecords--)
	{
		if (nErrorNo = AppendBlank(nWorkArea)) // append blank records
			break;
	}
	Unlock(nWorkArea);
	return nErrorNo;
}

inline int AppendRecords(unsigned int nRecords, Locator &pLoc)
{
	return AppendRecords(nRecords, pLoc.l_where);
}

/* access to common window handles */
inline HWND WMainHwnd() { return _WhToHwnd(_WMainWindow()); }
inline HWND WTopHwnd() { return _WhToHwnd(_WOnTop()); }
inline HWND WHwndByTitle(char *lcWindow) { return _WhToHwnd(_WFindTitle(lcWindow)); };

// misc helper functions
// transform 2 integers (a 64 Bit Integer) to a double 
inline double Ints2Double(int nLowInt, int nHighInt) { return ((double)nHighInt) * 4294967296.0 + nLowInt; }
inline __int64 Ints2Int64(int nLowInt, int nHighInt) { return ((__int64)nHighInt) * 4294967296 + nLowInt; }

// forward declaration 
class FoxValue;
class FoxObject;

class CFoxVersion
{
public:
	static int Version();
	static int MajorVersion();
	static int MinorVersion();

private:
	static void Init();
	static int m_Version;
	static int m_MajorVersion;
	static int m_MinorVersion;
};

class AutoYieldLock
{
public:
	AutoYieldLock()
	{ 
		m_Value.ev_type = '0';
		if (_Evaluate(&m_Value, "_VFP.AutoYield") == 0 && m_Value.ev_length > 0)
		{
			_Execute("_VFP.AutoYield = .F.");
		}
	}

	~AutoYieldLock()
	{
		if (m_Value.ev_type == 'L' && m_Value.ev_length > 0)
		{
			_Execute("_VFP.AutoYield = .T.");
		}
	}
private:
	Value m_Value;
};

class AutoOnOffSetting
{
public:
	AutoOnOffSetting(const char* pSetting, bool bOn) : m_Setting(pSetting), m_Reset(false)
	{
		m_Value.ev_type = '0';
		m_Command.Format("SET('%S')", m_Setting);
		if (_Evaluate(&m_Value, m_Command) == 0)
		{
			if ((bOn && m_Value.ev_length == 3) || (!bOn && m_Value.ev_length == 2))
			{
				m_Reset = true;
				m_ResetTo = bOn ? "OFF" : "ON";
				m_Command.Format("SET %S ", m_Setting);
				m_Command.SetFormatBase();
				m_Command.AppendFromBase(bOn ? "ON" : "OFF");
				_Execute(m_Command);
			}
			_FreeHand(m_Value.ev_handle);
		}
	}
	
	~AutoOnOffSetting()
	{
		if (m_Reset)
		{
			m_Command.AppendFromBase(m_ResetTo);
			_Execute(m_Command);
		}
	}

private:
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> m_Command;
	Value m_Value;
	bool m_Reset;
	char* m_ResetTo;
	const char* m_Setting;
};

/* base class for variable types*/
class FoxValue
{
public:
	FoxValue();
	explicit FoxValue(char cType);
	explicit FoxValue(char cType, int nWidth);
	explicit FoxValue(char cType, int nWidth, unsigned long nPrec);
	explicit FoxValue(LocatorEx &pLoc);
	~FoxValue();
	
	FoxValue& Evaluate(char *pExpression);
	char Vartype() const;
	void Release();
	void Return();
	void ReturnCopy();
	unsigned int Len() const;

	// handle manipulation
	FoxValue& AllocHandle(int nBytes);
	FoxValue& FreeHandle();
	char* HandleToPtr() const;
	FoxValue& LockHandle();
	FoxValue& UnlockHandle();
	unsigned int GetHandleSize() const;
	MHandle GetHandle() const;
	FoxValue& SetHandleSize(unsigned long nSize);
	FoxValue& ExpandHandle(int nBytes);
	FoxValue& NullTerminate();

	// object manipulation
	FoxValue& LockObject();
	FoxValue& UnlockObject();
	FoxValue& FreeObject();
	IDispatch* GetIDispatch();

	// operators
	FoxValue& operator=(LocatorEx &pLoc);
	FoxValue& operator<<(FoxObject &pObject);
	operator Value&() { return m_Value; }
	operator Value*() { return &m_Value; }
	Value* operator->() { return &m_Value; }

protected:
	Value m_Value;
	bool m_Locked;
};

class FoxLogical : public FoxValue
{
public:
	FoxLogical() : FoxValue('L') {}
	~FoxLogical() {}

	FoxLogical& operator=(bool bValue) { m_Value.ev_length = bValue ? 1 : 0; return *this; }
	FoxLogical& operator=(BOOL bValue) { m_Value.ev_length = bValue; return *this; }
	operator bool() { return m_Value.ev_length > 0; }
	operator BOOL() { return m_Value.ev_length; }
};

class FoxShort : public FoxValue
{
public:
	FoxShort() : FoxValue('I', 6, 0) {}
	~FoxShort() {}

	FoxShort& operator=(short nValue) { m_Value.ev_long = nValue; return *this; }
	operator short() { return static_cast<short>(m_Value.ev_long); }
};

class FoxUShort : public FoxValue
{
public:
	FoxUShort() : FoxValue('I', 5, 0) {}
	~FoxUShort() {}

	FoxUShort& operator=(unsigned short nValue) { m_Value.ev_long = nValue; return *this; }
	operator unsigned short() { return static_cast<unsigned short>(m_Value.ev_long); }
};

class FoxInt : public FoxValue
{
public:
	FoxInt() : FoxValue('I', 11, 0) {}
	~FoxInt() {}

	FoxInt& operator=(int nValue) { m_Value.ev_long = nValue; return *this; }
	operator int() { return m_Value.ev_long; }
};

class FoxUInt : public FoxValue
{
public:
	FoxUInt() : FoxValue('N', 10, 0) {}
	~FoxUInt() {}

	FoxUInt& operator=(unsigned int nValue) { m_Value.ev_real = static_cast<double>(nValue); return *this; }
	operator unsigned int() { return static_cast<unsigned int>(m_Value.ev_real); }
};

class FoxFloat : public FoxValue
{
public:
	FoxFloat() : FoxValue('N', 20, 6) {}
	~FoxFloat() {}

	FoxFloat& operator=(float nValue) { m_Value.ev_real = static_cast<double>(nValue); return *this; }
	operator float() { return static_cast<float>(m_Value.ev_real); }
};

class FoxDouble : public FoxValue
{
public: 
	FoxDouble() : FoxValue('N', 20, 16) {}
	explicit FoxDouble(int nPrec) : FoxValue('N', 20, nPrec) { }
	~FoxDouble() {}

	FoxDouble& operator=(double nValue) { m_Value.ev_real = nValue; return *this; }
	operator double() { return m_Value.ev_real; }
};

class FoxInt64 : public FoxValue
{
public:
	FoxInt64() : FoxValue('N', 20, 0) {}
	~FoxInt64() {}

	FoxInt64& operator=(double nValue) { m_Value.ev_real = nValue; return *this; }
	FoxInt64& operator=(__int64 nValue) { m_Value.ev_real = static_cast<double>(nValue); return *this; }
	FoxInt64& operator=(unsigned __int64 nValue) { m_Value.ev_real = static_cast<double>(nValue); return *this; }

	operator __int64() { return static_cast<__int64>(m_Value.ev_real); }
};

class FoxCurrency : public FoxValue
{
public:
	FoxCurrency() : FoxValue('Y', 21, 0) {}
	~FoxCurrency() {}

	FoxCurrency& operator=(__int64 nValue) { m_Value.ev_currency.QuadPart = nValue; return *this; }
	FoxCurrency& operator=(unsigned __int64 nValue) { m_Value.ev_currency.QuadPart = static_cast<__int64>(nValue); return *this; }
	FoxCurrency& operator=(double nValue) { m_Value.ev_currency.QuadPart = static_cast<__int64>(nValue); return *this; }

	operator __int64() { return m_Value.ev_currency.QuadPart; }
};

typedef enum _FoxStringInitialization
{
	NullIfEmpty = 0,
	NoNullIfEmpty
} FoxStringInitialization;

/* FoxString - wraps a FoxPro character/binary string */
class FoxString : public FoxValue
{
public:
	/* Constructors/Destructor */
	FoxString();
	explicit FoxString(FoxString &pString);
	explicit FoxString(FoxParameterEx& pVal);
	explicit FoxString(FoxParameterEx& pVal, unsigned int nExpand);
	explicit FoxString(ParamBlkEx& pParms, int nParmNo);
	explicit FoxString(ParamBlkEx& pParms, int nParmNo, unsigned int nExpand);
	explicit FoxString(ParamBlkEx& pParms, int nParmNo, FoxStringInitialization eInit);
	explicit FoxString(const CStringView pString);
	explicit FoxString(unsigned int nSize);
	explicit FoxString(BSTR pString, UINT nCodePage = CP_ACP);
	explicit FoxString(SAFEARRAY *pArray);
	~FoxString();

	FoxString& Attach(ValueEx& pValue, unsigned int nExpand = 0);
	FoxString& Attach(FoxParameterEx& pVal);
	FoxString& Attach(FoxParameterEx& pVal, unsigned int nExpand);
	bool Attach(ParamBlkEx& pParms, int nParmNo);
	bool Attach(ParamBlkEx& pParms, int nParmNo, unsigned int nExpand);
	bool Attach(ParamBlkEx& pParms, int nParmNo, FoxStringInitialization eInit);

	unsigned int Size() const { return m_BufferSize; }
	FoxString& Size(unsigned int nSize);
	unsigned int Len() const { return m_Value.ev_length; }
	FoxString& Len(unsigned int nLen) { m_Value.ev_length = nLen; return *this; }
	bool Binary() const { return m_Value.ev_width == 1; }
	FoxString& Binary(bool bBinary) { m_Value.ev_width = bBinary ? 1 : 0; return *this; }
	unsigned int CodePage() const { return m_CodePage; }
	FoxString& CodePage(unsigned int nCodePage) { m_CodePage = nCodePage; return *this; }
	bool Empty() const;
	bool ICompare(CStringView pString) const;
	FoxString& Evaluate(char *pExpression);
	FoxString& AddBs();
	FoxString& AddBsWildcard();
	FoxString& Fullpath();
	FoxString& FileAttributesToString(DWORD dwAttributes);
	bool StringToFileAttributes(DWORD& nAttributesSet, DWORD& nAttributesClear) const;
	FoxString& Alltrim(char cParseChar = ' ');
	FoxString& LTrim(char cParseChar = ' ');
	FoxString& RTrim(char cParseChar = ' ');
	FoxString& Lower();
	FoxString& Upper();
	FoxString& Prepend(const CStringView pString);
	FoxString& PrependIfNotPresent(const CStringView pString);
	FoxString& SubStr(unsigned int nStartPos, unsigned int nLen = -1);
	FoxString& Strtran(const CStringView sSearchFor, const CStringView sReplacement);
	FoxString& Replicate(const CStringView pString, unsigned int nCount);
	unsigned int At(char cSearchFor, unsigned int nOccurence = 1, unsigned int nMax = 0) const;
	unsigned int Rat(char cSearchFor, unsigned int nOccurence = 1, unsigned int nMax = 0) const;
	CStringView GetWordNum(unsigned int nWordnum, const char pSeperator) const;
	unsigned int GetWordCount(const char pSeperator) const;
	FoxString& Format(const char* format, ...);
	FoxString& StringLen();
	FoxString& StrnCpy(const char *pString, unsigned int nMaxLen);
	FoxString& CopyBytes(const unsigned char *pBytes, unsigned int nLen);
	FoxString& CopyDblString(const char *pDblString, unsigned int nMaxLen = 4096);
	unsigned int StringDblLen();
	unsigned int StringDblCount();
	FoxString& NullTerminate();
	unsigned int Expand(int nSize = 1);
	FoxString& ExtendBuffer(unsigned int nNewMinBufferSize);
	BSTR ToBSTR() const ;
	SAFEARRAY* ToU1SafeArray() const;

	void DetachParameter();
	void Return();
	void Release();

	template<typename T>
	T Ptr() { return reinterpret_cast<T>(m_String); }

	void* Ptr() { return reinterpret_cast<void*>(m_String); }

	/* operator overloads */
	FoxString& operator<<(FoxObject& pObject);
	FoxString& operator=(LocatorEx &pLoc);
	FoxString& operator=(FoxValue &pValue);
	FoxString& operator=(const CStringView pString);
	FoxString& operator=(const CWideStringView pWString);
	FoxString& operator+=(const CStringView pString);
	FoxString& operator+=(const char pChar);

	bool operator==(const CStringView pString) const;
	char& operator[](int nIndex) { return m_String[nIndex]; }
	char& operator[](unsigned long nIndex) { return m_String[nIndex]; }

	/* cast operators */
	operator char*() const { return m_String; }
	operator const char*() const { return m_String; }
	operator unsigned char*() const { return reinterpret_cast<unsigned char*>(m_String); }
	operator const unsigned char*() const { return reinterpret_cast<const unsigned char*>(m_String); }
	operator const Value&() { return m_Value; }
	operator CStringView();

private:
	char *m_String;
	unsigned int m_BufferSize;
	bool m_ParameterRef;
	unsigned int m_CodePage;
};

/* FoxWString - wraps a unicode string */
template<int nBufferSize>
class FoxWString
{
public:
	/* Constructors/Destructor */
	FoxWString() : m_String(0), m_Length(0), m_Size(0) { m_Buffer[0] = L'\0'; }

	explicit FoxWString(FoxWString& pString)
	{
		m_String = 0;
		m_Size = 0;
		if (pString.m_String || pString.m_Length)
		{
			wchar_t* pBuffer = pString.m_String ? pString.m_String : pString.m_Buffer;
			unsigned int nLen = pString.m_Length + 1;
			if (nLen >= nBufferSize)
			{
				m_String = new wchar_t[nLen];
				if (m_String == 0)
					throw E_INSUFMEMORY;
				m_Size = nLen;
				memcpy(m_String, pBuffer, nLen * 2);
			}
			else
			{
				memcpy(m_Buffer, pBuffer, nLen * 2);
			}
			m_Length = pString.m_Length;
			return;
		}
		m_Length = 0;
	}

	explicit FoxWString(ValueEx& pVal)
	{
		m_String = 0;
		m_Size = 0;
		if (pVal.ev_length > 0)
		{
			CStringView pString(pVal.HandleToPtr(), pVal.Len());
			Assign(pString);
		}
		else
		{
			m_Length = 0;
		}
	}

	explicit FoxWString(ParamBlkEx& parms, int nParmNo)
	{
		m_String = 0;
		m_Size = 0;
		// if parameter count is equal or greater than the parameter we want
		if (parms.pCount >= nParmNo)
		{
			ValueEx& pVal = parms(nParmNo);
			if (pVal.Vartype() == 'C' && pVal.Len() > 0)
			{
				CStringView pString(pVal.HandleToPtr(), pVal.Len());
				Assign(pString);
				return;
			}
		}
		// else
		m_Buffer[0] = L'\0';
		m_Length = 0;
	}

	explicit FoxWString(ParamBlkEx& parms, int nParmNo, char cTypeCheck)
	{
		// if parameter count is equal or greater than the parameter we want
		if (parms.pCount >= nParmNo)
		{
			ValueEx& pVal = parms(nParmNo);
			if (pVal.ev_type == 'C')
			{
				if (pVal.ev_length > 0)
				{
					CStringView pString(pVal.HandleToPtr(), pVal.Len());
					Assign(pString);
					return;
				}
			}
			else if (pVal.ev_type != cTypeCheck)
			{
				SaveCustomError("FoxWString:constructor", "Parameter type '%s' invalid.", pVal.ev_type);
				throw E_INVALIDPARAMS;
			}
		}
		// else
		m_String = 0;
		m_Length = 0;
		m_Size = 0;
	}

	explicit FoxWString(const CStringView pString)
	{
		Assign(pString);
	}

	~FoxWString()
	{
		if (m_String)
			delete m_String;
	}

	void Size(unsigned int nSize)
	{
		if (nSize > nBufferSize || m_String != 0)
		{
			if (m_String == 0)
			{
				m_String = new wchar_t[nSize];
				if (m_String == 0)
					throw E_INSUFMEMORY;
				m_Size = nSize;
			}
			else if (m_Size < nSize)
			{
				wchar_t* pOldString = m_String;
				m_String = new wchar_t[nSize];
				if (m_String == 0)
				{
					delete pOldString;
					throw E_INSUFMEMORY;
				}
				memcpy(m_String, pOldString, m_Size * sizeof(wchar_t*));
				m_Size = nSize;
			}
		}
	}

	unsigned int Size()
	{
		return m_Size;
	}

	unsigned int Len()
	{
		return m_Length;
	}

	void Len(unsigned int nLen)
	{
		m_Length = nLen;
	}

	/* operator overloads */
	FoxWString& operator=(const CStringView pString)
	{
		Assign(pString);
		return *this;
	}

	operator wchar_t*()
	{
		if (m_String)
			return m_String;
		else if (m_Length)
			return m_Buffer;
		else
			return 0;
	}

	operator const wchar_t* () const
	{
		if (m_String)
			return m_String;
		else if (m_Length)
			return reinterpret_cast<const wchar_t*>(m_Buffer);
		else
			return 0;
	}

	operator CWideStringView()
	{
		if (m_String)
			return CWideStringView(m_String, m_Length);
		else if (m_Length)
			return CWideStringView(m_Buffer, m_Length);
		else
			return 0;
	}
	
	bool operator!() { return m_String == 0 && m_Length == 0; }
	operator bool() { return m_String != 0 || m_Length > 0; }

private:
	
	void Assign(const CStringView pString)
	{
		if (pString)
		{
			if (pString.Len >= nBufferSize || nBufferSize == 0)
			{
				if (m_String)
				{
					if (m_Size < pString.Len + 1)
					{
						delete m_String;
						m_Size = pString.Len + 1;
						m_String = new wchar_t[m_Size];
						if (m_String == 0)
							throw E_INSUFMEMORY;
					}
				}
				else
				{
					m_Size = pString.Len + 1;
					m_String = new wchar_t[m_Size];
					if (m_String == 0)
						throw E_INSUFMEMORY;
				}

				Convert(pString.Data, pString.Len, m_String, m_Size);
			}
			else if (m_String)
			{
				Convert(pString.Data, pString.Len, m_String, m_Size);
			}
			else
			{
				Convert(pString.Data, pString.Len, reinterpret_cast<wchar_t*>(m_Buffer), nBufferSize * sizeof(wchar_t*));
			}
		}
		else
		{
			m_Length = 0;
		}
	}

	void Convert(char* pString, int nLen, wchar_t* pBuffer, int nBufferLen)
	{
		int nChars;
		if (nLen)
		{
			nChars = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pString, nLen, pBuffer, nBufferLen);
			if (!nChars)
			{
				SaveWin32Error("MultiByteToWideChar", GetLastError());
				throw E_APIERROR;
			}
			pBuffer[nChars] = L'\0'; // nullterminate
		}
		else
		{
			nChars = 0;
			pBuffer[nChars] = L'\0'; // nullterminate
		}
		m_Length = nChars;
	}

	wchar_t *m_String;
	unsigned int m_Length;
	unsigned int m_Size;
#pragma warning(disable: 4200)
	wchar_t m_Buffer[nBufferSize];
#pragma warning(default: 4200)
};

/* FoxDate - wraps a FoxPro date */
class FoxDate : public FoxValue
{
public:
	FoxDate() : FoxValue ('D') { m_Value.ev_real = 0; }
	explicit FoxDate(ValueEx &pVal);
	explicit FoxDate(SYSTEMTIME &sTime);
	explicit FoxDate(FILETIME &sTime);
	~FoxDate() {};

	FoxDate& operator=(double nDate) { m_Value.ev_real = nDate; return *this; }
	FoxDate& operator=(const SYSTEMTIME &sTime);
	FoxDate& operator=(const FILETIME &sTime);
	operator SYSTEMTIME();
	operator FILETIME();

private:
	void SystemTimeToDate(const SYSTEMTIME &sTime);
	void FileTimeToDate(const FILETIME &sTime);
	void DateToSystemTime(SYSTEMTIME &sTime) const;
	void DateToFileTime(FILETIME &sTime) const;
};

/* FoxDateTime - wraps a FoxPro datetime */
class FoxDateTime : public FoxValue
{
public:
	FoxDateTime() : FoxValue('T') { m_Value.ev_real = 0; }
	explicit FoxDateTime(ValueEx &pVal);
	explicit FoxDateTime(SYSTEMTIME &sTime);
	explicit FoxDateTime(FILETIME &sTime);
	explicit FoxDateTime(double dTime);
	~FoxDateTime() {}

	FoxDateTime& operator=(double nDateTime) { m_Value.ev_real = nDateTime; return *this; }
	FoxDateTime& operator=(const SYSTEMTIME &sTime);
	FoxDateTime& operator=(const FILETIME &sTime);
	FoxDateTime& operator=(ValueEx& pValue);
	operator SYSTEMTIME();
	operator FILETIME();
	FoxDateTime& ToUTC();
	FoxDateTime& ToLocal();

private:
	void SystemTimeToDateTime(const SYSTEMTIME &sTime);
	void FileTimeToDateTime(const FILETIME &sTime);
	void DateTimeToSystemTime(SYSTEMTIME &sTime) const;
	void DateTimeToFileTime(FILETIME &sTime) const;
};

typedef class FoxDateTimeLiteral
{
public:
	FoxDateTimeLiteral() { m_Literal[0] = '\0'; m_Length = 0; m_ToLocal = false; }
	~FoxDateTimeLiteral() {}

	void ToLocal(bool bToLocal) { m_ToLocal = bToLocal; }
	operator CStringView ();

	FoxDateTimeLiteral& operator=(SYSTEMTIME& sTime);
	FoxDateTimeLiteral& operator=(FILETIME& sTime);

private:
	bool m_ToLocal;
	unsigned int m_Length;
	char m_Literal[VFP_MAX_DATE_LITERAL];
}FoxDateTimeLiteral;

/* FoxObject */
class FoxObject : public FoxValue
{
public:
	FoxObject() : m_Property(0), m_ParameterRef(false), FoxValue('O') { }
	explicit FoxObject(ValueEx &pVal);
	explicit FoxObject(ParamBlkEx& parm, int nParmNo);
	explicit FoxObject(char* pExpression);
	~FoxObject();

	FoxObject& NewObject(char *pClass);
	FoxObject& EmptyObject();
	FoxObject& Evaluate(char *pExpression);
	void Release(); 

	FoxObject& operator<<(FoxValue &pValue);
	FoxObject& operator<<(LocatorEx &pLoc);
	FoxObject& operator<<(short nValue);
	FoxObject& operator<<(unsigned short nValue);
	FoxObject& operator<<(int nValue);
	FoxObject& operator<<(unsigned long nValue);
	FoxObject& operator<<(bool bValue);
	FoxObject& operator<<(double nValue);
	FoxObject& operator<<(__int64 nValue);

	FoxObject& operator()(char* pProperty);
	char * Property();
	bool operator!() const;
	operator bool() const;

private:
	bool m_ParameterRef;
	char *m_Property;
};

/* FoxVariable */
class FoxVariable
{
public:
	FoxVariable() { m_Loc.l_subs = 0;  m_Loc.l_NTI = 0; }
	explicit FoxVariable(char* pName) { m_Loc.l_subs = 0;  m_Loc.l_NTI = 0; Attach(pName); }
	explicit FoxVariable(char* pName, bool bPublic) { m_Loc.l_subs; m_Loc.l_NTI = 0; New(pName, bPublic); }
	~FoxVariable() { Release(); }

	FoxVariable& New(char *pName, bool bPublic);
	FoxVariable& Release();
	FoxVariable& Attach(char *pName);
	FoxVariable& Detach();

	VarLocatorEx& operator()() { return m_Loc; };
	operator VarLocatorEx&();

private:
	VarLocatorEx m_Loc;
};

/* FoxMemo */
class FoxMemo
{
public:
	FoxMemo();
	explicit FoxMemo(ParamBlkEx& parm, int nParmNo);
	explicit FoxMemo(LocatorEx &pLoc);
	~FoxMemo();

	void Alloc(unsigned int nSize);
	void Append(char *pData, unsigned int nLen);
	char* Read(unsigned int &nLen);
	FoxMemo& operator=(FoxString &pString);

	long Size() const { return m_Size; }

private:
	LocatorEx m_Loc;
	FCHAN m_File;
	long m_Location;
	long m_Size;
	char *m_pContent;
};

/* FoxArray */
class FoxArray
{
public:
	FoxArray();
	explicit FoxArray(ValueEx &pVal, unsigned int nRows = 0, unsigned int nDims = 0);
	explicit FoxArray(FoxParameterEx& pVal, unsigned int nRows = 0, unsigned int nDims = 0);
	explicit FoxArray(LocatorEx &pLoc);
	explicit FoxArray(ParamBlkEx& parm, int nParmNo);
	explicit FoxArray(ParamBlkEx& parm, int nParmNo, char cTypeCheck);

	FoxArray& Dimension(unsigned int nRows, unsigned int nDims = 0);
	FoxArray& Dimension(ValueEx &pVal, unsigned int nRows, unsigned int nDims = 0);
	FoxArray& Dimension(CStringView pName, unsigned int nRows, unsigned int nDims = 0);
	FoxArray& AutoGrow(unsigned int nRows);
	unsigned int AutoGrow();
	FoxArray& AutoOverflow(bool bOverflow);
	bool AutoOverflow() const;
	unsigned int CompactOverflow();
	FoxArray& ValidateDimension(unsigned int nDim);
	FoxArray& Reset();
	unsigned int Grow();
	unsigned int ALen(unsigned int &nDims);
	unsigned int ARows() const { return m_Rows; }
	unsigned int ADims() const { return m_Dims; }
	unsigned short& CurrentRow();
	unsigned short& CurrentDim();
	void ReturnRows();
	void Release();

	VarLocatorEx& operator()();
	VarLocatorEx& operator()(unsigned int nRow);
	VarLocatorEx& operator()(unsigned int nRow, unsigned int nDim);

	bool operator!();
	operator bool();

private:
	void Init();
	void Init(LocatorEx &pLoc);
	void Init(ValueEx &pVal, unsigned int nRows = 0, unsigned int nDims = 0);
	void InitArray();
	void ReDimension(unsigned int nRows, unsigned int nDims = 0);
	void OverflowArray();
	bool FindArray();

	VarLocatorEx m_Loc;
	unsigned int m_Rows;
	unsigned int m_Dims;
	unsigned int m_AutoGrow;
	unsigned int m_AutoOverflow;
	CStrBuilder<VFP_MAX_VARIABLE_NAME + 1> m_Name;
};

/* FoxCursor */
class FoxCursor
{
public:
	FoxCursor() : m_FieldCnt(0), m_WorkArea(0), m_AutoFLocked(false), m_pFieldLocs(0) {}
	~FoxCursor();

	bool Create(CStringView pCursorName, CStringView pFields, bool bAttach = true);
	FoxCursor& Attach(CStringView pCursorName, CStringView pFields);
	FoxCursor& Attach(int nWorkArea, CStringView pFields);
	FoxCursor& AppendBlank();
	FoxCursor& AppendCarry();
	FoxCursor& Append();
	int GetFieldNumber(char* pField);
	int GoTop();
	int GoBottom();
	int Skip(int nRecords = 1);
	bool Deleted();
	int RecNo();
	int RecCount();
	unsigned int FCount() { return m_FieldCnt; }
	FoxCursor& Go(long nRecord);
	bool RLock();
	bool AutoFLock();
	bool FLock();
	FoxCursor& Unlock();
	bool Bof();
	bool Eof();
	bool RLocked();
	bool FLocked();
	bool Exclusiv();
	bool Readonly();
	int DBStatus();

	FieldLocatorEx& operator()(unsigned int nFieldNo);

private:
	unsigned int m_FieldCnt;
	int m_WorkArea;
	bool m_AutoFLocked;
	FieldLocatorEx *m_pFieldLocs;
};

class FoxCStringArray
{
public:
	FoxCStringArray() : m_Rows(0), m_pStrings(0), m_pValues(0) { }
	~FoxCStringArray();

	unsigned int ARows() { return m_Rows; }
	FoxCStringArray& FoxCStringArray::operator=(FoxArray &pArray);
	operator char**() const { return m_pStrings; }
	operator LPCSTR*() const  { return (LPCSTR*)m_pStrings; }


private:
	void Dimension(unsigned int nRows);
	unsigned int m_Rows;
	char **m_pStrings;
	FoxValue *m_pValues;
};

template<int nArgCount>
class CFoxComCallback
{
public:
	CFoxComCallback() : m_Object(0), m_DispId(0)
	{
		InitParameters();
	}

	CFoxComCallback(Value &pObject, FoxString &pMethod) : m_Object(0), m_DispId(0)
	{
		InitParameters();
		Initialize(pObject, pMethod);
	}

	~CFoxComCallback()
	{
	}

	void Release()
	{
		m_Object.Release();
	}

	void Initialize(IDispatch *pObject, FoxString &pMethod)
	{
		m_Object = pObject;
		if (m_Object == 0)
			throw E_INVALIDPARAMS;
		InitDispId(pMethod);
	}

	void Initialize(FoxObject &pObject, FoxString &pMethod)
	{
		m_Object = pObject.GetIDispatch();
		if (m_Object == 0)
			throw E_INVALIDPARAMS;
		InitDispId(pMethod);
	}

	inline void SetParameterCount(int nArgs)
	{
		assert(nArgs >= 0 && nArgs <= nArgCount);
		m_Disp.cArgs = nArgs;
		m_Disp.rgvarg = &m_Args[nArgCount - nArgs];
	}

	inline void SetParameter(int nArg, wchar_t* pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		vararg->vt = VT_BSTR;
		vararg->bstrVal = SysAllocString(pValue);
	}

	inline void SetParameter(int nArg, CStringView pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		vararg->vt = VT_ARRAY | VT_UI1; 
		vararg->parray = &m_SafeArray[m_Disp.cArgs - nArg];
		vararg->parray->pvData = pValue.Data;
		vararg->parray->rgsabound[0].cElements = pValue.Len;
	}

	inline void SetParameter(int nArg, const char* pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		vararg->vt = VT_ARRAY | VT_UI1;
		vararg->parray = &m_SafeArray[m_Disp.cArgs - nArg];
		vararg->parray->pvData = reinterpret_cast<PVOID>(const_cast<char*>(pValue));
		vararg->parray->rgsabound[0].cElements = strlen(pValue);
	}

	inline void SetParameter(int nArg, char* pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		vararg->vt = VT_ARRAY | VT_UI1;
		vararg->parray = &m_SafeArray[m_Disp.cArgs - nArg];
		vararg->parray->pvData = pValue;
		vararg->parray->rgsabound[0].cElements = strlen(pValue);
	}

	inline void SetParameter(int nArg, bool pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		vararg->vt = VT_I2;
		vararg->boolVal = pValue ? 0xFFFF : 0;
	}

	inline void SetParameter(int nArg, void* pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
#if !defined(_WIN64)
		vararg->vt = VT_UI4;
		vararg->ulVal = reinterpret_cast<unsigned int>(pValue);
#else
		vararg->vt = VT_R8;
		vararg->dblVal = static_cast<double>(reinterpret_cast<UINT_PTR>(pValue));
#endif
	}

	inline void SetParameter(int nArg, short pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		vararg->vt = VT_I2;
		vararg->iVal = pValue;
	}

	inline void SetParameter(int nArg, int pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		vararg->vt = VT_I4;
		vararg->lVal = pValue;
	}
	
	inline void SetParameter(int nArg, long pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		vararg->vt = VT_I4;
		vararg->lVal = pValue;
	}

	inline void SetParameter(int nArg, unsigned int pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		vararg->vt = VT_UI4;
		vararg->ulVal = pValue;
	}

	inline void SetParameter(int nArg, unsigned long pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		vararg->vt = VT_UI4;
		vararg->ulVal = pValue;
	}

	inline void SetParameter(int nArg, double pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		vararg->vt = VT_R8;
		vararg->dblVal = pValue;
	}

	inline void FreeParameter(int nArg, wchar_t* pValue)
	{
		VARIANT *vararg = &m_Disp.rgvarg[m_Disp.cArgs - nArg];
		if (vararg->bstrVal)
		{
			SysFreeString(vararg->bstrVal);
			vararg->bstrVal = 0;
		}
	}

	inline void FreeParameter(int nArg, CStringView pValue) { }
	inline void FreeParameter(int nArg, char* pValue) { }
	inline void FreeParameter(int nArg, const char* pValue) { }
	inline void FreeParameter(int nArg, bool pValue) { }
	inline void FreeParameter(int nArg, void* pValue) { }
	inline void FreeParameter(int nArg, short pValue) { }
	inline void FreeParameter(int nArg, int pValue) { }
	inline void FreeParameter(int nArg, long pValue) { }
	inline void FreeParameter(int nArg, unsigned int pValue) { }
	inline void FreeParameter(int nArg, unsigned long pValue) { }
	inline void FreeParameter(int nArg, double pValue) { }

	inline HRESULT Call()
	{ 
		UINT errorArg;
		EXCEPINFO excinfo;
		HRESULT hr = m_Object->Invoke(m_DispId, IID_NULL, 0, DISPATCH_METHOD, &m_Disp, 0, &excinfo, &errorArg);
		return hr;
	}

	template<typename T1>
	inline HRESULT Call(T1 pParm1)
	{
		assert(nArgCount >= 1);
		SetParameterCount(1);
		SetParameter(1, pParm1);
		HRESULT hr = Call();
		FreeParameter(1, pParm1);
		return hr;
	}

	template<typename T1, typename T2>
	inline HRESULT Call(T1 pParm1, T2 pParm2)
	{
		assert(nArgCount >= 2);
		SetParameterCount(2);
		SetParameter(1, pParm1);
		SetParameter(2, pParm2);
		HRESULT hr = Call();
		FreeParameter(1, pParm1);
		FreeParameter(2, pParm2);
		return hr;
	}

	template<typename T1, typename T2, typename T3>
	inline HRESULT Call(T1 pParm1, T2 pParm2, T3 pParm3)
	{
		assert(nArgCount >= 3);
		SetParameterCount(3);
		SetParameter(1, pParm1);
		SetParameter(2, pParm2);
		SetParameter(3, pParm3);
		HRESULT hr = Call();
		FreeParameter(1, pParm1);
		FreeParameter(2, pParm2);
		FreeParameter(3, pParm3);
		return hr;
	}

	template<typename T1, typename T2, typename T3, typename T4>
	inline HRESULT Call(T1 pParm1, T2 pParm2, T3 pParm3, T4 pParm4)
	{
		assert(nArgCount >= 4);
		SetParameterCount(3);
		SetParameter(1, pParm1);
		SetParameter(2, pParm2);
		SetParameter(3, pParm3);
		SetParameter(4, pParm4);
		HRESULT hr = Call();
		FreeParameter(1, pParm1);
		FreeParameter(2, pParm2);
		FreeParameter(3, pParm3);
		FreeParameter(4, pParm4);
		return hr;
	}

	operator bool() const {
		return m_Object != 0;
	}

private:

	inline void InitDispId(FoxString &pMethod)
	{
		CComBSTR methodName;
		methodName.Attach(pMethod.ToBSTR());
		HRESULT hr = m_Object->GetIDsOfNames(IID_NULL, &methodName, 1, VFP2CTls::Tls().ConvCP, &m_DispId);
		if (FAILED(hr))
		{
			SaveWin32Error("IDispatch::GetIDsOfNames", hr);
			throw E_APIERROR;
		}
	}

	inline void InitParameters()
	{
		m_Disp.rgvarg = m_Args;
		m_Disp.rgdispidNamedArgs = 0;
		m_Disp.cArgs = nArgCount;
		m_Disp.cNamedArgs = 0;
		for(int xj = 0; xj < nArgCount; xj++)
		{
			VariantInit(&m_Args[xj]);
			m_SafeArray[xj].cDims = 1;
			m_SafeArray[xj].fFeatures = FADF_STATIC;
			m_SafeArray[xj].cbElements = 1;
			m_SafeArray[xj].cLocks = 0;
			m_SafeArray[xj].pvData = 0;
			m_SafeArray[xj].rgsabound[0].cElements = 0;
			m_SafeArray[xj].rgsabound[0].lLbound = 0;
		}
	}

	CComPtr<IDispatch> m_Object;
	DISPID m_DispId;
	DISPPARAMS m_Disp;
	VARIANTARG m_Args[nArgCount];
	SAFEARRAY m_SafeArray[nArgCount];
};

class CFoxCallback
{
public:
	void SetCallback(CStringView pCallback)
	{
		m_Callback = pCallback;
		m_Callback.SetFormatBase();
	}

	int Evaluate(Value* pVal)
	{
		m_Callback.ResetToFormatBase().Append("()");
		return _Evaluate(pVal, m_Callback);
	}

	int Execute()
	{
		m_Callback.ResetToFormatBase().Append("()");
		return _Execute(m_Callback);
	}

	template<typename T1>
	int Evaluate(Value* pVal, T1 parm1)
	{
		BuildCallbackNaturalOrder(parm1);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2)
	{
		BuildCallbackNaturalOrder(parm1, parm2);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2, typename T3>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2, T3 parm3)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5, parm6);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5, parm6, parm7);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1>
	int Execute(T1 parm1)
	{
		BuildCallbackNaturalOrder(parm1);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2>
	int Execute(T1 parm1, T2 parm2)
	{
		BuildCallbackNaturalOrder(parm1, parm2);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2, typename T3>
	int Execute(T1 parm1, T2 parm2, T3 parm3)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4>
	int Execute(T1 parm1, T2 parm2, T3 parm3, T4 parm4)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5>
	int Execute(T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
	int Execute(T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5, parm6);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
	int Execute(T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5, parm6, parm7);
		return _Execute(m_Callback);
	}

	bool AsyncExecute(HWND hCallbackWindow, const UINT nCallbackMsg)
	{
		m_Callback.ResetToFormatBase().Append("()");
		char* pCommand = m_Callback.Strdup();
		if (pCommand)
			return PostMessage(hCallbackWindow, nCallbackMsg, reinterpret_cast<WPARAM>(pCommand), 0) > 0;
		return false;
	}

	template<typename T1>
	bool AsyncExecute(HWND hCallbackWindow, const UINT nCallbackMsg, T1 parm1)
	{
		BuildCallbackNaturalOrder(parm1);
		char* pCommand = m_Callback.Strdup();
		if (pCommand)
			return PostMessage(hCallbackWindow, nCallbackMsg, reinterpret_cast<WPARAM>(pCommand), 0) > 0;
		return false;
	}

	template<typename T1, typename T2>
	bool AsyncExecute(HWND hCallbackWindow, const UINT nCallbackMsg, T1 parm1, T2 parm2)
	{
		BuildCallbackNaturalOrder(parm1, parm2);
		char* pCommand = m_Callback.Strdup();
		if (pCommand)
			return PostMessage(hCallbackWindow, nCallbackMsg, reinterpret_cast<WPARAM>(pCommand), 0) > 0;
		return false;
	}

	template<typename T1, typename T2, typename T3>
	bool AsyncExecute(HWND hCallbackWindow, const UINT nCallbackMsg, T1 parm1, T2 parm2, T3 parm3)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3);
		char* pCommand = m_Callback.Strdup();
		if (pCommand)
			return PostMessage(hCallbackWindow, nCallbackMsg, reinterpret_cast<WPARAM>(pCommand), 0) > 0;
		return false;
	}

	template<typename T1, typename T2, typename T3, typename T4>
	bool AsyncExecute(HWND hCallbackWindow, const UINT nCallbackMsg, T1 parm1, T2 parm2, T3 parm3, T4 parm4)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4);
		char* pCommand = m_Callback.Strdup();
		if (pCommand)
			return PostMessage(hCallbackWindow, nCallbackMsg, reinterpret_cast<WPARAM>(pCommand), 0) > 0;
		return false;
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5>
	bool AsyncExecute(HWND hCallbackWindow, const UINT nCallbackMsg, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5);
		char* pCommand = m_Callback.Strdup();
		if (pCommand)
			return PostMessage(hCallbackWindow, nCallbackMsg, reinterpret_cast<WPARAM>(pCommand), 0) > 0;
		return false;
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
	bool AsyncExecute(HWND hCallbackWindow, const UINT nCallbackMsg, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5, parm6);
		char* pCommand = m_Callback.Strdup();
		if (pCommand)
			return PostMessage(hCallbackWindow, nCallbackMsg, reinterpret_cast<WPARAM>(pCommand), 0) > 0;
		return false;
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
	bool AsyncExecute(HWND hCallbackWindow, const UINT nCallbackMsg, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7)
	{
		BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5, parm6, parm7);
		char* pCommand = m_Callback.Strdup();
		if (pCommand)
			return PostMessage(hCallbackWindow, nCallbackMsg, reinterpret_cast<WPARAM>(pCommand), 0) > 0;
		return false;
	}

protected:
	template<typename T1>
	inline void BuildCallbackNaturalOrder(T1 parm1)
	{
		m_Callback.ResetToFormatBase().Append('(').Marshal(parm1).Append(')');
	}

	template<typename T1, typename T2>
	inline void BuildCallbackNaturalOrder(T1 parm1, T2 parm2)
	{
		m_Callback.ResetToFormatBase().Append('(').Marshal(parm1).Append(',').Marshal(parm2).Append(')');
	}

	template<typename T1, typename T2, typename T3>
	inline void BuildCallbackNaturalOrder(T1 parm1, T2 parm2, T3 parm3)
	{
		m_Callback.ResetToFormatBase().Append('(').Marshal(parm1).Append(',').Marshal(parm2).Append(',').Marshal(parm3).Append(')');
	}

	template<typename T1, typename T2, typename T3, typename T4>
	inline void BuildCallbackNaturalOrder(T1 parm1, T2 parm2, T3 parm3, T4 parm4)
	{
		m_Callback.ResetToFormatBase().Append('(').Marshal(parm1).Append(',').Marshal(parm2).Append(',')
			.Marshal(parm3).Append(',').Marshal(parm4).Append(')');
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5>
	inline void BuildCallbackNaturalOrder(T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5)
	{
		m_Callback.ResetToFormatBase().Append('(').Marshal(parm1).Append(',').Marshal(parm2).Append(',')
			.Marshal(parm3).Append(',').Marshal(parm4).Append(',').Marshal(parm5).Append(')');
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
	inline void BuildCallbackNaturalOrder(T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6)
	{
		m_Callback.ResetToFormatBase().Append('(').Marshal(parm1).Append(',').Marshal(parm2).Append(',')
			.Marshal(parm3).Append(',').Marshal(parm4).Append(',').Marshal(parm5).Append(',').Marshal(parm6).Append(')');
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
	inline void BuildCallbackNaturalOrder(T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7)
	{
		m_Callback.ResetToFormatBase().Append('(').Marshal(parm1).Append(',').Marshal(parm2).Append(',')
			.Marshal(parm3).Append(',').Marshal(parm4).Append(',').Marshal(parm5).Append(',').Marshal(parm6).Append(',').Marshal(parm7).Append(')');
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
	inline void BuildCallbackNaturalOrder(T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7, T8 parm8)
	{
		m_Callback.ResetToFormatBase().Append('(').Marshal(parm1).Append(',').Marshal(parm2).Append(',')
			.Marshal(parm3).Append(',').Marshal(parm4).Append(',').Marshal(parm5).Append(',').Marshal(parm6).Append(',').Marshal(parm7).Append(',').Marshal(parm8).Append(')');
	}

	CStrBuilder<2048>	m_Callback;
};

class CDynamicFoxCallback : public CFoxCallback
{
public:
	CDynamicFoxCallback()
	{
		m_NaturalParameterOrder = true;
		for (int nParm = 0; nParm < m_ParameterPosition.GetCount(); nParm++)
			m_ParameterPosition[nParm] = nParm;
		m_ParameterCount = m_ParameterPosition.GetCount();
	}

	// nParmNo - zero based, nParmPosition - one based
	void SetParameterPosition(int nParmNo, int nParmPosition)
	{
		m_ParameterPosition[nParmNo] = nParmPosition;
	}

	void OptimizeParameterPosition()
	{
		m_NaturalParameterOrder = true;
		for (int nParm = 0; nParm < m_ParameterPosition.GetCount(); nParm++)
		{
			if (m_ParameterPosition[nParm] != nParm)
			{
				m_NaturalParameterOrder = false;
				break;
			}
		}
		if (m_NaturalParameterOrder == false)
		{
			int nSortedOrder[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
			int nPos = 0;
			for (int nParm = 0; nParm < m_ParameterPosition.GetCount(); nParm++)
			{
				int nParmPos = m_ParameterPosition.Find(nParm);
				if (nParmPos > -1)
				{
					nSortedOrder[nPos] = nParmPos;
					nPos++;
				}
			}
			m_ParameterPosition.Copy(nSortedOrder, m_ParameterPosition.GetCount());
			int nParmCount = m_ParameterPosition.Find(-1);
			m_ParameterCount = nParmCount == -1 ? m_ParameterPosition.GetCount() : nParmCount;
		}
	}

	void SetCallback(CStringView pCallback)
	{
		m_Callback = pCallback;
		m_Callback.SetFormatBase();
	}

	int Evaluate(Value* pVal)
	{
		m_Callback.ResetToFormatBase().Append("()");
		return _Evaluate(pVal, m_Callback);
	}

	int Execute()
	{
		m_Callback.ResetToFormatBase().Append("()");
		return _Execute(m_Callback);
	}

	template<typename T1>
	int Evaluate(Value* pVal, T1 parm1)
	{
		BuildCallback(parm1);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2)
	{
		BuildCallback(parm1, parm2);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2, typename T3>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2, T3 parm3)
	{
		BuildCallback(parm1, parm2, parm3);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4)
	{
		BuildCallback(parm1, parm2, parm3, parm4);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5)
	{
		BuildCallback(parm1, parm2, parm3, parm4, parm5);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6)
	{
		BuildCallback(parm1, parm2, parm3, parm4, parm5, parm6);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7)
	{
		BuildCallback(parm1, parm2, parm3, parm4, parm5, parm6, parm7);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
	int Evaluate(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7, T8 parm8)
	{
		BuildCallback(parm1, parm2, parm3, parm4, parm5, parm6, parm7, parm8);
		return _Evaluate(pVal, m_Callback);
	}

	template<typename T1>
	int Execute(Value* pVal, T1 parm1)
	{
		BuildCallback(parm1);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2>
	int Execute(Value* pVal, T1 parm1, T2 parm2)
	{
		BuildCallback(parm1, parm2);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2, typename T3>
	int Execute(Value* pVal, T1 parm1, T2 parm2, T3 parm3)
	{
		BuildCallback(parm1, parm2, parm3);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4>
	int Execute(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4)
	{
		BuildCallback(parm1, parm2, parm3, parm4);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5>
	int Execute(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5)
	{
		BuildCallback(parm1, parm2, parm3, parm4, parm5);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
	int Execute(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6)
	{
		BuildCallback(parm1, parm2, parm3, parm4, parm5, parm6);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
	int Execute(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7)
	{
		BuildCallback(parm1, parm2, parm3, parm4, parm5, parm6, parm7);
		return _Execute(m_Callback);
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
	int Execute(Value* pVal, T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7, T8 parm8)
	{
		BuildCallback(parm1, parm2, parm3, parm4, parm5, parm6, parm7, parm8);
		return _Execute(m_Callback);
	}

protected:

	template<typename T1>
	inline void BuildCallback(T1 parm1)
	{
		m_Callback.ResetToFormatBase().Append('(').Marshal(parm1).Append(')');
	}

	template<typename T1, typename T2>
	inline void BuildCallback(T1 parm1, T2 parm2)
	{
		if (m_NaturalParameterOrder)
			return BuildCallbackNaturalOrder(parm1, parm2);

		bool bSeperator = false;
		m_Callback.ResetToFormatBase().Append('(');
		for (int xj = 0; xj < m_ParameterCount; xj++)
		{
			switch (m_ParameterPosition[xj])
			{
			case 0:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm1);
				bSeperator = true;
				break;
			case 1:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm2);
				bSeperator = true;
				break;
			}
		}
		m_Callback.Append(')');
	}

	template<typename T1, typename T2, typename T3>
	inline void BuildCallback(T1 parm1, T2 parm2, T3 parm3)
	{
		if (m_NaturalParameterOrder)
			return BuildCallbackNaturalOrder(parm1, parm2, parm3);

		bool bSeperator = false;
		m_Callback.ResetToFormatBase().Append('(');
		for (int xj = 0; xj < m_ParameterCount; xj++)
		{
			switch (m_ParameterPosition[xj])
			{
			case 0:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm1);
				bSeperator = true;
				break;
			case 1:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm2);
				bSeperator = true;
				break;
			case 2:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm3);
				bSeperator = true;
				break;
			}
		}
		m_Callback.Append(')');
	}

	template<typename T1, typename T2, typename T3, typename T4>
	inline void BuildCallback(T1 parm1, T2 parm2, T3 parm3, T4 parm4)
	{
		if (m_NaturalParameterOrder)
			return BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4);

		bool bSeperator = false;
		m_Callback.ResetToFormatBase().Append('(');
		for (int xj = 0; xj < m_ParameterCount; xj++)
		{
			switch (m_ParameterPosition[xj])
			{
			case 0:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm1);
				bSeperator = true;
				break;
			case 1:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm2);
				bSeperator = true;
				break;
			case 2:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm3);
				bSeperator = true;
				break;
			case 3:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm4);
				bSeperator = true;
				break;
			}
		}
		m_Callback.Append(')');
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5>
	inline void BuildCallback(T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5)
	{
		if (m_NaturalParameterOrder)
			return BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5);

		bool bSeperator = false;
		m_Callback.ResetToFormatBase().Append('(');
		for (int xj = 0; xj < m_ParameterCount; xj++)
		{
			switch (m_ParameterPosition[xj])
			{
			case 0:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm1);
				bSeperator = true;
				break;
			case 1:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm2);
				bSeperator = true;
				break;
			case 2:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm3);
				bSeperator = true;
				break;
			case 3:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm4);
				bSeperator = true;
				break;
			case 4:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm5);
				bSeperator = true;
				break;
			}
		}
		m_Callback.Append(')');
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
	inline void BuildCallback(T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6)
	{
		if (m_NaturalParameterOrder)
			return BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5, parm6);

		bool bSeperator = false;
		m_Callback.ResetToFormatBase().Append('(');
		for (int xj = 0; xj < m_ParameterCount; xj++)
		{
			switch (m_ParameterPosition[xj])
			{
			case 0:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm1);
				bSeperator = true;
				break;
			case 1:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm2);
				bSeperator = true;
				break;
			case 2:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm3);
				bSeperator = true;
				break;
			case 3:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm4);
				bSeperator = true;
				break;
			case 4:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm5);
				bSeperator = true;
				break;
			case 5:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm6);
				bSeperator = true;
				break;
			}
		}
		m_Callback.Append(')');
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
	inline void BuildCallback(T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7)
	{
		if (m_NaturalParameterOrder)
			return BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5, parm6, parm7);

		bool bSeperator = false;
		m_Callback.ResetToFormatBase().Append('(');
		for (int xj = 0; xj < m_ParameterCount; xj++)
		{
			switch (m_ParameterPosition[xj])
			{
			case 0:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm1);
				bSeperator = true;
				break;
			case 1:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm2);
				bSeperator = true;
				break;
			case 2:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm3);
				bSeperator = true;
				break;
			case 3:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm4);
				bSeperator = true;
				break;
			case 4:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm5);
				bSeperator = true;
				break;
			case 5:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm6);
				bSeperator = true;
				break;
			case 6:
				if (bSeperator)
					m_Callback.Append(',');
				m_Callback.Marshal(parm7);
				bSeperator = true;
				break;
			}
		}
		m_Callback.Append(')');
	}

	template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
	inline void BuildCallback(T1 parm1, T2 parm2, T3 parm3, T4 parm4, T5 parm5, T6 parm6, T7 parm7, T8 parm8)
	{
		if (m_NaturalParameterOrder)
			return BuildCallbackNaturalOrder(parm1, parm2, parm3, parm4, parm5, parm6, parm7, parm8);

		bool bSeperator = false;
		m_Callback.ResetToFormatBase().Append('(');
		for (int xj = 0; xj < m_ParameterCount; xj++)
		{
			switch (m_ParameterPosition[xj])
			{
				case 0:
					if (bSeperator)
						m_Callback.Append(',');
					m_Callback.Marshal(parm1);
					bSeperator = true;
					break;
				case 1:
					if (bSeperator)
						m_Callback.Append(',');
					m_Callback.Marshal(parm2);
					bSeperator = true;
					break;
				case 2:
					if (bSeperator)
						m_Callback.Append(',');
					m_Callback.Marshal(parm3);
					bSeperator = true;
					break;
				case 3:
					if (bSeperator)
						m_Callback.Append(',');
					m_Callback.Marshal(parm4);
					bSeperator = true;
					break;
				case 4:
					if (bSeperator)
						m_Callback.Append(',');
					m_Callback.Marshal(parm5);
					bSeperator = true;
					break;
				case 5:
					if (bSeperator)
						m_Callback.Append(',');
					m_Callback.Marshal(parm6);
					bSeperator = true;
					break;
				case 6:
					if (bSeperator)
						m_Callback.Append(',');
					m_Callback.Marshal(parm7);
					bSeperator = true;
					break;
				case 7:
					if (bSeperator)
						m_Callback.Append(',');
					m_Callback.Marshal(parm8);
					bSeperator = true;
					break;
			}
		}
		m_Callback.Append(')');
	}

	bool				m_NaturalParameterOrder;
	int					m_ParameterCount;
	CFixedArray<int,8>	m_ParameterPosition;
};

// helper class which holds the current timezone information (singleton - use GetTsi to get instance)
class TimeZoneInfo
{
public:
	double Bias;
	static TimeZoneInfo& GetTsi();

private:
	TimeZoneInfo();
	~TimeZoneInfo();
	static LRESULT _stdcall TimeChangeWindowProc(HWND nHwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Refresh();

	DWORD m_CurrentZone;
	TIME_ZONE_INFORMATION m_ZoneInfo;
	HWND m_Hwnd;
	ATOM m_Atom;
};

/* inline function implementations for the above classes */
/* CFoxVersion */
inline void CFoxVersion::Init()
{
	FoxValue vFoxVer;
	vFoxVer.Evaluate("INT(VERSION(5))");
	m_Version = vFoxVer->ev_long;
	m_MajorVersion = m_Version / 100;
	m_MinorVersion = m_Version % 100;
}

inline int CFoxVersion::Version()
{
	if (m_Version == 0)
		Init();
	return m_Version;
}

inline int CFoxVersion::MajorVersion()
{
 	if (m_Version == 0)
		Init();
	return m_MajorVersion;
}

inline int CFoxVersion::MinorVersion()
{
	if (m_Version == 0)
		Init();
	return m_MinorVersion;
}

/* FoxValue */
inline FoxValue::FoxValue() : m_Locked(false)
{
	m_Value.ev_type = '0';
	m_Value.ev_handle = 0;
	m_Value.ev_object = 0;
}

inline FoxValue::FoxValue(char cType) : m_Locked(false)
{
	m_Value.ev_type = cType;
	m_Value.ev_width = 0;
	m_Value.ev_length = 0;
	m_Value.ev_object = 0;
	m_Value.ev_handle = 0;
}

inline FoxValue::FoxValue(char cType, int nWidth) : m_Locked(false)
{
	m_Value.ev_type = cType;
	m_Value.ev_width = nWidth;
	m_Value.ev_real = 0.0;
}

inline FoxValue::FoxValue(char cType, int nWidth, unsigned long nPrec) : m_Locked(false)
{
	m_Value.ev_type = cType;
	m_Value.ev_width = nWidth;
	m_Value.ev_length = nPrec;
	m_Value.ev_real = 0.0;
}

inline FoxValue::FoxValue(LocatorEx& pLoc) : m_Locked(false)
{
	int nErrorNo;
	m_Value.ev_type = '0';
	if (nErrorNo = _Load(pLoc, &m_Value))
		throw nErrorNo;
#if defined(_DEBUGALLOCATIONS)
	if (m_Value.ev_type == 'C')
		VfpAllocationCount++;
#endif
}

inline FoxValue::~FoxValue()
{
	if (m_Value.ev_type == 'C')
	{
		UnlockHandle();
		FreeHandle();
	}
	else if (m_Value.ev_type == 'O')
	{
		UnlockObject();
		FreeObject();
	}
}

inline FoxValue& FoxValue::Evaluate(char* pExpression)
{
	Release();
	int nErrorNo;
	if (nErrorNo = _Evaluate(&m_Value, pExpression))
		throw nErrorNo;
#if defined(_DEBUGALLOCATIONS)
	if (m_Value.ev_type == 'C')
		VfpAllocationCount++;
#endif
	return *this;
}

inline char FoxValue::Vartype() const
{
	return m_Value.ev_type;
}

inline void FoxValue::Release()
{
	if (m_Value.ev_type == 'C')
	{
		UnlockHandle();
		FreeHandle();
	}
	else if (m_Value.ev_type == 'O')
	{
		UnlockObject();
		FreeObject();
	}
	m_Value.ev_type = '0';
}

inline void FoxValue::Return()
{
	assert(m_Locked == false);
	_RetVal(&m_Value);
	m_Value.ev_type = '0';
}

inline void FoxValue::ReturnCopy()
{
	if (Vartype() == 'C')
	{
		FoxValue pDup('C');
		if (Len() > 0)
		{
			pDup.AllocHandle(Len());
			memcpy(pDup.HandleToPtr(), HandleToPtr(), Len());
		}
		pDup->ev_length = Len();
		pDup.Return();
	}
	else
		_RetVal(&m_Value);
}

inline unsigned int FoxValue::Len() const
{
	return m_Value.ev_length;
}

inline FoxValue& FoxValue::AllocHandle(int nBytes)
{
	assert(m_Value.ev_handle == 0);
	m_Value.ev_handle = _AllocHand(nBytes);
	if (m_Value.ev_handle == 0)
		throw E_INSUFMEMORY;
#if defined(_DEBUGALLOCATIONS)
	VfpAllocationCount++;
#endif
	return *this;
}

inline FoxValue& FoxValue::FreeHandle()
{
	if (m_Value.ev_handle)
	{
		_FreeHand(m_Value.ev_handle);
		m_Value.ev_handle = 0;
#if defined(_DEBUGALLOCATIONS)
		VfpAllocationCount--;
#endif
	}
	return *this;
}

inline char* FoxValue::HandleToPtr() const
{
	assert(m_Value.ev_type == 'C' && m_Value.ev_handle);
	return reinterpret_cast<char*>(_HandToPtr(m_Value.ev_handle));
}

inline FoxValue& FoxValue::LockHandle()
{
	if (m_Locked == false)
	{
		assert(m_Value.ev_type == 'C' && m_Value.ev_handle);
		_HLock(m_Value.ev_handle);
		m_Locked = true;
	}
	return *this;
}

inline FoxValue& FoxValue::UnlockHandle()
{
	if (m_Locked)
	{
		assert(m_Value.ev_type == 'C' && m_Value.ev_handle);
		_HUnLock(m_Value.ev_handle);
		m_Locked = false;
	}
	return *this;
}

inline unsigned int FoxValue::GetHandleSize() const
{
	assert(m_Value.ev_type == 'C' && m_Value.ev_handle);
	return _GetHandSize(m_Value.ev_handle);
}

inline MHandle FoxValue::GetHandle() const
{
	assert(m_Value.ev_type == 'C' && m_Value.ev_handle);
	return m_Value.ev_handle;
}

inline FoxValue& FoxValue::SetHandleSize(unsigned long nSize)
{
	assert(m_Value.ev_type == 'C' && m_Value.ev_handle && m_Locked == false);
	if (_SetHandSize(m_Value.ev_handle, nSize) == 0)
		throw E_INSUFMEMORY;
	return *this;
}

inline FoxValue& FoxValue::ExpandHandle(int nBytes)
{
	assert(m_Value.ev_type == 'C' && m_Value.ev_handle && m_Locked == false);
	if (_SetHandSize(m_Value.ev_handle, m_Value.ev_length + nBytes) == 0)
		throw E_INSUFMEMORY;
	return *this;
}

inline FoxValue& FoxValue::NullTerminate()
{
	assert(m_Value.ev_type == 'C' && m_Value.ev_handle && m_Locked == false);
	if (_SetHandSize(m_Value.ev_handle, m_Value.ev_length + 1) == 0)
		throw E_INSUFMEMORY;
	return *this;
}

inline FoxValue& FoxValue::LockObject()
{
	assert(m_Value.ev_type == 'O' && m_Value.ev_object);
	if (m_Locked == false)
	{
		int nErrorNo = _ObjectReference(&m_Value);
		if (nErrorNo)
			throw nErrorNo;
		m_Locked = true;
	}
	return *this;
}

inline FoxValue& FoxValue::UnlockObject()
{
	if (m_Locked)
	{
		assert(m_Value.ev_type == 'O' && m_Value.ev_object);
		int nErrorNo = _ObjectRelease(&m_Value);
		if (nErrorNo)
			throw nErrorNo;
		m_Locked = false;
	}
	return *this;
}

inline FoxValue& FoxValue::FreeObject()
{
	if (m_Value.ev_object)
	{
		assert(m_Value.ev_type == 'O');
		_FreeObject(&m_Value);
		m_Value.ev_object = 0;
	}
	return *this;
}

inline FoxValue& FoxValue::operator=(LocatorEx& pLoc)
{
	Release();
	int nErrorNo;
	if (nErrorNo = _Load(pLoc, &m_Value))
		throw nErrorNo;
#if defined(_DEBUGALLOCATIONS)
	if (m_Value.ev_type == 'C')
		VfpAllocationCount++;
#endif
	return *this;
}

inline FoxValue& FoxValue::operator<<(FoxObject& pObject)
{
	Release();
	int nErrorNo;
	if (nErrorNo = _GetObjectProperty(&m_Value, pObject, pObject.Property()))
		throw nErrorNo;
#if defined(_DEBUGALLOCATIONS)
	if (m_Value.ev_type == 'C')
		VfpAllocationCount++;
#endif
	return *this;
}

inline FoxString& FoxString::Evaluate(char* pExpression)
{
	Release();
	int nErrorNo;
	if (nErrorNo = _Evaluate(&m_Value, pExpression))
		throw nErrorNo;
#if defined(_DEBUGALLOCATIONS)
	if (m_Value.ev_type == 'C')
		VfpAllocationCount++;
#endif
	return *this;
}

inline FoxObject& FoxObject::Evaluate(char* pExpression)
{
	Release();
	int nErrorNo;
	if (nErrorNo = _Evaluate(&m_Value, pExpression))
		throw nErrorNo;
#if defined(_DEBUGALLOCATIONS)
	if (m_Value.ev_type == 'C')
		VfpAllocationCount++;
#endif
	return *this;
}

inline FoxObject& FoxObject::operator<<(FoxValue& pValue)
{
	assert(m_Property && m_Value.ev_object);
	int nErrorNo;
	if (nErrorNo = _SetObjectProperty(&m_Value, m_Property, pValue, TRUE))
		throw nErrorNo;
	return *this;
}

inline FoxObject& FoxObject::operator<<(LocatorEx& pLoc)
{
	assert(m_Property && m_Value.ev_object);
	int nErrorNo;
	FoxValue pValue(pLoc);
	if (nErrorNo = _SetObjectProperty(&m_Value, m_Property, pValue, TRUE))
		throw nErrorNo;
	return *this;
}

inline FoxObject& FoxObject::operator<<(short nValue)
{
	assert(m_Property && m_Value.ev_object);
	int nErrorNo;
	ValueEx vTmp;
	vTmp.SetShort(nValue);
	if (nErrorNo = _SetObjectProperty(&m_Value, m_Property, vTmp, TRUE))
		throw nErrorNo;
	return *this;
}

inline FoxObject& FoxObject::operator<<(unsigned short nValue)
{
	assert(m_Property && m_Value.ev_object);
	int nErrorNo;
	ValueEx vTmp;
	vTmp.SetUShort(nValue);
	if (nErrorNo = _SetObjectProperty(&m_Value, m_Property, vTmp, TRUE))
		throw nErrorNo;
	return *this;
}

inline FoxObject& FoxObject::operator<<(int nValue)
{
	assert(m_Property && m_Value.ev_object);
	int nErrorNo;
	ValueEx vTmp;
	vTmp.SetInt(nValue);
	if (nErrorNo = _SetObjectProperty(&m_Value, m_Property, vTmp, TRUE))
		throw nErrorNo;
	return *this;
}

inline FoxObject& FoxObject::operator<<(unsigned long nValue)
{
	assert(m_Property && m_Value.ev_object);
	int nErrorNo;
	ValueEx vTmp;
	vTmp.SetUInt(nValue);
	if (nErrorNo = _SetObjectProperty(&m_Value, m_Property, vTmp, TRUE))
		throw nErrorNo;
	return *this;
}

inline FoxObject& FoxObject::operator<<(bool bValue)
{
	assert(m_Property && m_Value.ev_object);
	int nErrorNo;
	ValueEx vTmp;
	vTmp.SetLogical(bValue);
	if (nErrorNo = _SetObjectProperty(&m_Value, m_Property, vTmp, TRUE))
		throw nErrorNo;
	return *this;
}

inline FoxObject& FoxObject::operator<<(double nValue)
{
	assert(m_Property && m_Value.ev_object);
	int nErrorNo;
	ValueEx vTmp;
	vTmp.SetDouble(nValue);
	if (nErrorNo = _SetObjectProperty(&m_Value, m_Property, vTmp, TRUE))
		throw nErrorNo;
	return *this;
}

inline FoxObject& FoxObject::operator<<(__int64 nValue)
{
	assert(m_Property && m_Value.ev_object);
	int nErrorNo;
	ValueEx vTmp;
	vTmp.SetInt64(nValue);
	if (nErrorNo = _SetObjectProperty(&m_Value, m_Property, vTmp, TRUE))
		throw nErrorNo;
	return *this;
}

inline FoxObject& FoxObject::operator()(char* pProperty)
{
	m_Property = pProperty;
	return *this;
}

inline char* FoxObject::Property()
{
	return m_Property;
}

inline bool FoxObject::operator!() const
{
	return !(m_Value.ev_type == 'O' && m_Value.ev_object);
}

inline FoxObject::operator bool() const
{
	return m_Value.ev_type == 'O' && m_Value.ev_object;
}

inline FoxVariable& FoxVariable::New(char* pName, bool bPublic)
{
	Release();
	m_Loc.l_subs = 0;
	NTI nNti = _NewVar(pName, m_Loc, bPublic ? NV_PUBLIC : NV_PRIVATE);
	if (nNti < 0)
		throw (int)-nNti;
	return *this;
}

inline FoxVariable& FoxVariable::Attach(char* pName)
{
	Release();
	NTI nNti = _NameTableIndex(pName);
	if (nNti < 0)
		throw E_VARIABLENOTFOUND;
	if (!_FindVar(nNti, -1, m_Loc))
		throw E_VARIABLENOTFOUND;
	return *this;
}

inline FoxVariable& FoxVariable::Detach()
{
	m_Loc.l_NTI = 0;
	return *this;
}

inline FoxVariable& FoxVariable::Release()
{
	if (m_Loc.l_NTI > 0)
	{
		_Release(m_Loc.l_NTI);
		m_Loc.l_NTI = 0;
	}
	return *this;
}

inline FoxVariable::operator VarLocatorEx& ()
{
	return m_Loc;
}

inline FoxArray& FoxArray::AutoGrow(unsigned int nRows)
{
	m_AutoGrow = nRows;
	return *this;
}

inline unsigned int FoxArray::AutoGrow()
{
	return m_AutoGrow;
	return *this;
}

inline FoxArray& FoxArray::AutoOverflow(bool bOverflow)
{
	if (bOverflow)
		m_AutoOverflow = m_AutoOverflow > 1 ? m_AutoOverflow : 1;
	else
		m_AutoOverflow = 0;
	return *this;
}

inline bool FoxArray::AutoOverflow() const
{
	return m_AutoOverflow > 0;
}

inline FoxArray& FoxArray::ValidateDimension(unsigned int nDim)
{
	if (nDim > m_Dims)
		throw E_INVALIDSUBSCRIPT;
	return *this;
}

inline void FoxArray::ReturnRows()
{
	if (m_AutoOverflow > 1)
		_RetInt(CompactOverflow(), 11);
	else if (m_AutoGrow && m_Loc.l_sub1 && m_Loc.l_sub1 < m_Rows)
		ReDimension(m_Loc.l_sub1, m_Dims);
	_RetInt(m_Loc.l_sub1, 5);
}

inline VarLocatorEx& FoxArray::operator()()
{
	return m_Loc;
}

inline VarLocatorEx& FoxArray::operator()(unsigned int nRow)
{
	m_Loc.l_sub1 = nRow;
	return m_Loc;
}

inline VarLocatorEx& FoxArray::operator()(unsigned int nRow, unsigned int nDim)
{
	m_Loc.l_sub1 = nRow;
	m_Loc.l_sub2 = nDim;
	return m_Loc;
}

inline unsigned short& FoxArray::CurrentRow()
{
	return m_Loc.l_sub1;
}

inline unsigned short& FoxArray::CurrentDim()
{
	return m_Loc.l_sub2;
}

inline bool FoxArray::operator!()
{
	return m_Name.Len() > 0;
}

inline FoxArray::operator bool()
{
	return m_Name.Len() > 0;
}

inline FoxCursor& FoxCursor::AppendBlank()
{
	int nErrorNo;
	if (nErrorNo = _DBAppend(m_WorkArea, 0) != 0)
		throw - nErrorNo;
	return *this;
}

inline FoxCursor& FoxCursor::AppendCarry()
{
	int nErrorNo;
	if (nErrorNo = _DBAppend(m_WorkArea, 1) != 0)
		throw - nErrorNo;
	return *this;
}

inline FoxCursor& FoxCursor::Append()
{
	int nErrorNo;
	if (nErrorNo = _DBAppend(m_WorkArea, -1) != 0)
		throw - nErrorNo;
	return *this;
}

inline int FoxCursor::GoTop()
{
	int recno;
	recno = _DBRewind(m_WorkArea);
	if (recno < 0)
		throw - recno;
	return recno;
}

inline int FoxCursor::GoBottom()
{
	int recno;
	recno = _DBUnwind(m_WorkArea);
	if (recno < 0)
		throw - recno;
	return recno;
}

inline int FoxCursor::Skip(int nRecords)
{
	int recno;
	recno = _DBSkip(m_WorkArea, nRecords);
	if (recno < 0)
		throw - recno;
	return recno;
}

inline bool FoxCursor::Deleted()
{
	ValueEx value;
	LocatorEx loc;
	value = 0;
	loc.l_where = m_WorkArea;
	loc.l_offset = -1;
	int nErrorNo = _Load(loc, value);
	if (nErrorNo)
		throw -nErrorNo;
	return value.ev_length > 0;
}

inline int FoxCursor::RecNo()
{
	int recno;
	recno = _DBRecNo(m_WorkArea);
	if (recno < 0)
		throw - recno;
	return recno;
}

inline int FoxCursor::RecCount()
{
	int reccount;
	reccount = _DBRecCount(m_WorkArea);
	if (reccount < 0)
		throw - reccount;
	return reccount;
}

inline FoxCursor& FoxCursor::Go(long nRecord)
{
	int nErrorNo;
	if (nErrorNo = _DBRead(m_WorkArea, nRecord) < 0)
		throw - nErrorNo;
	return *this;
}

inline bool FoxCursor::RLock()
{
	return _DBLock(m_WorkArea, DBL_RECORD) > 0;
}

inline bool FoxCursor::FLock()
{
	return _DBLock(m_WorkArea, DBL_FILE) > 0;
}

inline FoxCursor& FoxCursor::Unlock()
{
	m_AutoFLocked = false;
	_DBUnLock(m_WorkArea);
	return *this;
}

inline bool FoxCursor::AutoFLock()
{
	if (m_AutoFLocked == false)
		m_AutoFLocked = FLock();
	return m_AutoFLocked;
}

inline bool FoxCursor::Bof()
{
	int status;
	status = _DBStatus(m_WorkArea);
	if (status < 0)
		throw - status;
	return (status & DB_BOF) > 0;
}

inline bool FoxCursor::Eof()
{
	int status;
	status = _DBStatus(m_WorkArea);
	if (status < 0)
		throw - status;
	return (status & DB_EOF) > 0;
}

inline bool FoxCursor::RLocked()
{
	int status;
	status = _DBStatus(m_WorkArea);
	if (status < 0)
		throw - status;
	return (status & DB_RLOCKED) > 0;
}

inline bool FoxCursor::FLocked()
{
	int status;
	status = _DBStatus(m_WorkArea);
	if (status < 0)
		throw - status;
	return (status & DB_FLOCKED) > 0;
}

inline bool FoxCursor::Exclusiv()
{
	int status;
	status = _DBStatus(m_WorkArea);
	if (status < 0)
		throw - status;
	return (status & DB_EXCLUSIVE) > 0;
}

inline bool FoxCursor::Readonly()
{
	int status;
	status = _DBStatus(m_WorkArea);
	if (status < 0)
		throw - status;
	return (status & DB_READONLY) > 0;
}

inline int FoxCursor::DBStatus()
{
	int status;
	status = _DBStatus(m_WorkArea);
	if (status < 0)
		throw - status;
	return status;
}

inline FieldLocatorEx& FoxCursor::operator()(unsigned int nFieldNo)
{
	assert(nFieldNo < m_FieldCnt);
	return *(m_pFieldLocs + nFieldNo);
}


#endif // _VFP2CCPPAPI_H__