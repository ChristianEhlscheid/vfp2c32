#ifndef _VFP2CTYPES_H__
#define _VFP2CTYPES_H__

#if defined(_DEBUGALLOCATIONS)
extern int VfpAllocationCount;
#endif

// some VFP internal error numbers
const int E_ALIASNOTFOUND = 137;
const int E_NUMERICOVERFLOW = 159;
const int E_VARIABLENOTFOUND = 170;
const int E_NOTANARRAY = 176;
const int E_INSUFMEMORY = 182;
const int E_INVALIDSUBSCRIPT = 224;
const int E_DIVIDEBYZERO = 307;
const int E_LOCKFAILED = 503;
const int E_TYPECONFLICT = 532;
const int E_NOENTRYPOINT = 754;
const int E_FIELDNOTFOUND = 806;
const int E_INVALIDPARAMS = 901;
const int E_CURRENCYOVERFLOW = 988;
const int E_CUSTOMERROR = 7777;

class FoxValue;
class FoxObject;

// thin wrappers around Value structure to quickly initialize to a specific type
/* modified Value structure for easy access to typed pointers in ev_long */
#pragma pack(push,1)
typedef struct ValueEx : public Value
{
	void SetUChar() { ev_type = 'I'; ev_width = 3; ev_long = 0; }
	void SetUChar(unsigned char nValue) { ev_type = 'I'; ev_width = 3; ev_long = static_cast<long>(nValue); }
	void SetShort() { ev_type = 'I'; ev_width = 6; }
	void SetShort(short nValue) { ev_type = 'I'; ev_width = 6; ev_long = static_cast<long>(nValue); }
	void SetUShort() { ev_type = 'I'; ev_width = 6; }
	void SetUShort(unsigned short nValue) { ev_type = 'I'; ev_width = 6; ev_long = static_cast<long>(nValue); }
	void SetInt() { ev_type = 'I'; ev_width = 11; ev_long = 0; }
	void SetInt(int nValue) { ev_type = 'I'; ev_width = 11; ev_long = nValue; }
	void SetUInt() { ev_type = 'N'; ev_width = 11; ev_length = 0; ev_real = 0; }
	void SetUInt(unsigned int nValue) { ev_type = 'N'; ev_width = 11; ev_length = 0; ev_real = static_cast<double>(nValue); }
	void SetFloat(unsigned int nPrecision = 7) { ev_type = 'N'; ev_width = 20; ev_length = nPrecision; ev_real = 0; }
	void SetFloat(float nValue, unsigned int nPrecision = 7) { ev_type = 'N'; ev_width = 20; ev_length = nPrecision; ev_real = nValue; }
	void SetDouble(unsigned int nPrecision = 16) { ev_type = 'N'; ev_width = 20; ev_length = nPrecision; ev_real = 0; }
	void SetDouble(double nValue, unsigned int nPrecision = 16) { ev_type = 'N'; ev_width = 20; ev_length = nPrecision; ev_real = nValue; }
	void SetCurrency() { ev_type = 'Y'; ev_currency.QuadPart = 0; }
	void SetCurrency(CCY nValue) { ev_type = 'Y'; ev_currency.QuadPart = nValue.QuadPart; }
	void SetCurrency(__int64 nValue) { ev_type = 'Y'; ev_currency.QuadPart = nValue; }
	void SetNumeric(int nWidth, unsigned int nPrecision) { ev_type = 'N'; ev_width = nWidth; ev_length = nPrecision; }
	void SetString(unsigned int nLen = 0) { ev_type = 'C'; ev_handle = 0; ev_length = nLen; }
	void SetLogical() { ev_type = 'L'; ev_length = 0; }
	void SetLogical(bool bValue) { ev_type = 'L'; ev_length = bValue; }
	void SetLogical(unsigned int bValue) { ev_type = 'L'; ev_length = bValue; }
	void SetDate() { ev_type = 'D'; ev_real = 0; }
	void SetDate(double nDate) { ev_type = 'D'; ev_real = nDate; }
	void SetDateTime() { ev_type = 'T'; ev_real = 0; }
	void SetDateTime(double nDate) { ev_type = 'T'; ev_real = nDate; }
	void SetInt64() { ev_type = 'N'; ev_width = 20; ev_length = 0; }
	void SetInt64(__int64 nValue) { ev_type = 'N'; ev_width = 20; ev_length = 0; ev_real = static_cast<double>(nValue); }
	void SetUInt64() { ev_type = 'N'; ev_width = 20; ev_length = 0; }
	void SetUInt64(unsigned __int64 nValue) { ev_type = 'N'; ev_width = 20; ev_length = 0; ev_real = static_cast<double>(nValue); }
#if !defined(_WIN64)
	void SetPointer() { ev_type = 'I'; ev_width = 11; ev_long = 0; }
	void SetPointer(void* pValue) { ev_type = 'I'; ev_width = 11; ev_long = reinterpret_cast<long>(pValue); }
	template<typename T>
	T Ptr() { return reinterpret_cast<T>(reinterpret_cast<void*>(ev_long)); }
	void* Ptr() { return reinterpret_cast<void*>(ev_long); }
#else
	void SetPointer() { ev_type = 'N'; ev_width = 20; ev_real = 0; }
	void SetPointer(void* pValue) { ev_type = 'N'; ev_width = 20; ev_real = static_cast<double>(reinterpret_cast<unsigned __int64>(pValue)); }
	template<typename T>
	T Ptr() { return reinterpret_cast<T>(ev_long < 0 ? static_cast<UINT_PTR>(ev_real) : static_cast<UINT_PTR>(ev_long)); }
	template<>
	UINT_PTR Ptr<UINT_PTR>() { return static_cast<UINT_PTR>(ev_long > -1 ? ev_long : ev_real); }
	void* Ptr() { return reinterpret_cast<void*>(ev_long > -1 ? static_cast<UINT_PTR>(ev_long) : static_cast<UINT_PTR>(ev_real)); }
#endif
	template<typename T>
	T DynamicPtr() { return reinterpret_cast<T>(ev_type == 'I' ? static_cast<UINT_PTR>(ev_long) : static_cast<UINT_PTR>(ev_real)); }
	template<>
	UINT_PTR DynamicPtr<UINT_PTR>() { return ev_type == 'I' ? static_cast<UINT_PTR>(ev_long) : static_cast<UINT_PTR>(ev_real); }

	void SetNull() { ev_type = '0'; }
	char Vartype() { return ev_type; }
	
	unsigned int Len() { return ev_length; }

	bool AllocHandle(int nBytes)
	{
		assert(ev_type == 'C' && ev_handle == 0); 
		ev_handle = _AllocHand(nBytes); 
#if defined(_DEBUGALLOCATIONS)
		if (ev_handle)
			VfpAllocationCount++;
#endif
		return ev_handle != 0;
	}

	void FreeHandle() { 
		assert(ev_type == 'C');  
		if (ev_handle)
		{
#if defined(_DEBUGALLOCATIONS)
			VfpAllocationCount--;
#endif
			_FreeHand(ev_handle);
			ev_handle = 0;
		}
	}
	
	char* HandleToPtr() 
	{ 
		assert(ev_type == 'C' && ev_handle != 0);
		return reinterpret_cast<char*>(_HandToPtr(ev_handle));
	}

	bool ValidHandle() 
	{ 
		assert(ev_type == 'C'); 
		return ev_handle > 0;
	}

	void LockHandle() 
	{ 
		assert(ev_type == 'C' && ev_handle != 0);  
		_HLock(ev_handle);
	}

	void UnlockHandle() 
	{ 
		assert(ev_type == 'C' && ev_handle != 0);  
		_HUnLock(ev_handle);
	}

	unsigned long GetHandleSize() 
	{ 
		assert(ev_type == 'C' && ev_handle != 0);
		return _GetHandSize(ev_handle);
	}

	bool SetHandleSize(unsigned long nSize) 
	{ 
		assert(ev_type == 'C' && ev_handle != 0); 
		return _SetHandSize(ev_handle, nSize) > 0;
	}

	bool ExpandHandleSize(int nBytes) 
	{ 
		assert(ev_type == 'C' && ev_handle != 0); 
		return _SetHandSize(ev_handle, ev_length + nBytes) > 0;
	}

	bool NullTerminate() 
	{ 
		assert(ev_type == 'C' && ev_handle != 0);
		return _SetHandSize(ev_handle, ev_length + 1) > 0;
	}

	void Release()
	{
		if (ev_type == 'C' && ev_handle > 0)
		{
#if defined(_DEBUGALLOCATIONS)
			VfpAllocationCount--;
#endif
			_FreeHand(ev_handle);
			ev_handle = 0;
		}
		else if (ev_type == 'O' && ev_object > 0)
			_FreeObject(*this);
	}

	void Return() {
		assert(ev_type != 'R');
		_RetVal(*this);
	}

	int Evaluate(char* pExpression)
	{
		int retval = _Evaluate(*this, pExpression);
#if defined(_DEBUGALLOCATIONS)
		if (ev_type == 'C')
			VfpAllocationCount++;
#endif
		return retval;
	}

	void ObjectRelease()
	{
		int nErrorNo;
		if (nErrorNo = _ObjectRelease(*this))
			throw nErrorNo;
	}

	void StoreObjectRef(char* pName, NTI& nVarNti);
	static void ReleaseObjectRef(char* pName, NTI nVarNti);

	// casting operators
	operator Value& () { return static_cast<Value&>(*this); }
	operator Value* () { return static_cast<Value*>(this); }

	// easy initialization to zero for handle based values
	ValueEx& operator=(int nInit) { assert(nInit == 0); ev_type = '0'; ev_handle = 0; ev_object = 0; return *this; }
	ValueEx& operator=(Locator& pLoc)
	{
		int nErrorNo;
		if (nErrorNo = _Load(&pLoc, *this))
			throw nErrorNo;
#if defined(_DEBUGALLOCATIONS)
		if (ev_type == 'C')
			VfpAllocationCount++;
#endif
		return *this;
	};
/*
	char				ev_type;
	char				ev_padding;
	short				ev_width;
	unsigned 			ev_length;
	long				ev_long;
	double				ev_real;
	CCY					ev_currency;
	MHandle				ev_handle;
	unsigned long		ev_object;
*/
} ValueEx;

typedef struct LocatorEx : public Locator
{
	bool IsVariableRef() { return l_type == 'R' && l_where == -1; }
	bool IsMemoRef() { return l_type == 'R' && l_where != -1; }

	NTI NewVar(char* pVarname, bool bPublic)
	{
		l_subs = 0;
		NTI nVarNti;
		nVarNti = _NewVar(pVarname, *this, bPublic ? NV_PUBLIC : NV_PRIVATE);
		if (nVarNti < 0)
		{
			int nErrorNo = (int)-nVarNti;
			throw nErrorNo;
		}
		return nVarNti;
	}

	NTI FindVar(char* pVarname)
	{
		NTI nVarNti;
		nVarNti = _NameTableIndex(pVarname);
		if (nVarNti == -1)
			throw E_VARIABLENOTFOUND;
		FindVar(nVarNti);
		return nVarNti;
	}

	void FindVar(NTI nVarNti)
	{
		if (!_FindVar(nVarNti, -1, *this))
			throw E_VARIABLENOTFOUND;
	}

	static NTI FindVarStatic(char* pVarname)
	{
		NTI nVarNti;
		nVarNti = _NameTableIndex(pVarname);
		if (nVarNti == -1)
			throw E_VARIABLENOTFOUND;
		return nVarNti;
	}

	static void ReleaseVarStatic(NTI nVarNti)
	{
		int nErrorNo;
		if (nErrorNo = _Release(nVarNti))
			throw nErrorNo;
	}

	static void ReleaseVarStatic(char* pVarname)
	{
		NTI nVarNti = FindVarStatic(pVarname);
		ReleaseVarStatic(nVarNti);
	}

	void Release()
	{
		if (l_NTI > 0)
			_Release(l_NTI);
	}

	void Store(ValueEx& sValue)
	{
		int nErrorNo;
		if (nErrorNo = _Store(*this, sValue))
			throw nErrorNo;
	}

	void Load(ValueEx& sValue)
	{
		int nErrorNo;
		if (nErrorNo = _Load(*this, sValue))
			throw nErrorNo;
#if defined(_DEBUGALLOCATIONS)
		if (sValue.ev_type == 'C')
			VfpAllocationCount++;
#endif
	}

	long ALen(int mode)
	{
		int nSubscript;
		nSubscript = _ALen(l_NTI, mode);
		if (nSubscript == -1)
			throw E_NOTANARRAY;
		return nSubscript;
	}

	long AElements()
	{
		long nSubscript;
		nSubscript = _ALen(l_NTI, AL_ELEMENTS);
		if (nSubscript == -1)
			throw E_NOTANARRAY;
		return nSubscript;
	}

	long ARows()
	{
		long nSubscript;
		nSubscript = _ALen(l_NTI, AL_SUBSCRIPT1);
		if (nSubscript == -1)
			throw E_NOTANARRAY;
		return nSubscript;
	}

	long ADims()
	{
		int nSubscript;
		nSubscript = _ALen(l_NTI, AL_SUBSCRIPT2);
		if (nSubscript == -1)
			throw E_NOTANARRAY;
		return nSubscript;
	}

	void ResetArray(int nColumns = 1)
	{
		l_subs = nColumns > 1 ? 2 : 1;
		l_sub1 = 0;
		l_sub2 = 0;
	}

	void Init(LocatorEx& pLoc)
	{
		l_type = pLoc.l_type;
		l_where = pLoc.l_where;
		l_NTI = pLoc.l_NTI;
		l_offset = pLoc.l_offset;
		l_subs = pLoc.l_subs;
		l_sub1 = pLoc.l_sub1;
		l_sub2 = pLoc.l_sub2;
	}

	operator Locator& () { return static_cast<Locator&>(*this); }
	operator Locator* () { return static_cast<Locator*>(this); }
	LocatorEx& operator()(int nRow) { l_sub1 = nRow; return *this; }
	LocatorEx& operator()(int nRow, int nDim) { l_sub1 = nRow; l_sub2 = nDim; return *this; }

	LocatorEx& operator<<(FoxObject& pObject);
	LocatorEx& operator=(FoxValue& pValue);

	LocatorEx& operator=(Value& pValue)
	{
		int nErrorNo;
		if (l_where == -1)
		{
			if (nErrorNo = _Store(*this, &pValue))
				throw nErrorNo;
		}
		else
		{
			if (nErrorNo = _DBReplace(*this, &pValue))
				throw nErrorNo;
		}
		return *this;
	}

	LocatorEx& operator=(ValueEx& pValue)
	{
		int nErrorNo;
		if (l_where == -1)
		{
			if (nErrorNo = _Store(*this, pValue))
				throw nErrorNo;
		}
		else
		{
			if (nErrorNo = _DBReplace(*this, pValue))
				throw nErrorNo;
		}
		return *this;
	}

	LocatorEx& operator=(LocatorEx& pLoc)
	{
		int nErrorNo;
		ValueEx pValue;
		pValue = 0;
		pValue = pLoc;
		if (l_where == -1)
		{
			if (nErrorNo = _Store(*this, pValue))
			{
				pValue.Release();
				throw nErrorNo;
			}
		}
		else
		{
			if (nErrorNo = _DBReplace(*this, pValue))
			{
				pValue.Release();
				throw nErrorNo;
			}
		}
		pValue.Release();
		return *this;
	}

	LocatorEx& operator=(int nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetInt(nValue);
		if (l_where == -1)
		{
			if (nErrorNo = _Store(*this, vTmp))
				throw nErrorNo;
		}
		else
		{
			if (nErrorNo = _DBReplace(*this, vTmp))
				throw nErrorNo;
		}
		return *this;
	}

	LocatorEx& operator=(unsigned int nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		if (l_where == -1)
		{
			if (nErrorNo = _Store(*this, vTmp))
				throw nErrorNo;
		}
		else
		{
			if (nErrorNo = _DBReplace(*this, vTmp))
				throw nErrorNo;
		}
		return *this;
	}

	LocatorEx& operator=(unsigned long nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		if (l_where == -1)
		{
			if (nErrorNo = _Store(*this, vTmp))
				throw nErrorNo;
		}
		else
		{
			if (nErrorNo = _DBReplace(*this, vTmp))
				throw nErrorNo;
		}
		return *this;
	}

	LocatorEx operator=(void* pPointer)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetPointer(pPointer);
		if (l_where == -1)
		{
			if (nErrorNo = _Store(*this, vTmp))
				throw nErrorNo;
		}
		else
		{
			if (nErrorNo = _DBReplace(*this, vTmp))
				throw nErrorNo;
		}
		return *this;
	}

	LocatorEx& operator=(double nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetDouble(nValue);
		if (l_where == -1)
		{
			if (nErrorNo = _Store(*this, vTmp))
				throw nErrorNo;
		}
		else
		{
			if (nErrorNo = _DBReplace(*this, vTmp))
				throw nErrorNo;
		}
		return *this;
	}

	LocatorEx& operator=(__int64 nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetInt64(nValue);
		if (l_where == -1)
		{
			if (nErrorNo = _Store(*this, vTmp))
				throw nErrorNo;
		}
		else
		{
			if (nErrorNo = _DBReplace(*this, vTmp))
				throw nErrorNo;
		}
		return *this;
	}

	LocatorEx& operator=(unsigned __int64 nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt64(nValue);
		if (l_where == -1)
		{
			if (nErrorNo = _Store(*this, vTmp))
				throw nErrorNo;
		}
		else
		{
			if (nErrorNo = _DBReplace(*this, vTmp))
				throw nErrorNo;
		}
		return *this;
	}

	LocatorEx& operator=(bool bValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetLogical(bValue);
		if (l_where == -1)
		{
			if (nErrorNo = _Store(*this, vTmp))
				throw nErrorNo;
		}
		else
		{
			if (nErrorNo = _DBReplace(*this, vTmp))
				throw nErrorNo;
		}
		return *this;
	}
/*
	char		l_type;
	SHORT		l_where;
	USHORT		l_NTI,	
		l_offset,		
		l_subs,			
		l_sub1, l_sub2; 
*/
} LocatorEx;

typedef struct VarLocatorEx : public LocatorEx
{
	VarLocatorEx& operator<<(FoxObject& pObject);
	VarLocatorEx& operator=(FoxValue& pValue);

	VarLocatorEx& operator=(Value& pValue)
	{
		int nErrorNo;
		if (nErrorNo = _Store(*this, &pValue))
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(ValueEx& pValue)
	{
		int nErrorNo;
		if (nErrorNo = _Store(*this, pValue))
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(LocatorEx& pLoc)
	{
		int nErrorNo;
		ValueEx pValue;
		pValue = pLoc;
		if (nErrorNo = _Store(*this, pValue))
		{
			pValue.Release();
			throw nErrorNo;
		}
		pValue.Release();
		return *this;
	}

	VarLocatorEx& operator=(int nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetInt(nValue);
		if (nErrorNo = _Store(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(unsigned int nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		if (nErrorNo = _Store(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(unsigned long nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		if (nErrorNo = _Store(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(void* pPointer)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetPointer(pPointer);
		if (nErrorNo = _Store(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(double nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetDouble(nValue);
		if (nErrorNo = _Store(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(__int64 nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetInt64(nValue);
		if (nErrorNo = _Store(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(unsigned __int64 nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt64(nValue);
		if (nErrorNo = _Store(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(bool bValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetLogical(bValue);
		if (nErrorNo = _Store(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

} VarLocatorEx;

typedef struct FieldLocatorEx : public LocatorEx
{
	FieldLocatorEx& operator<<(FoxObject& pObject);
	FieldLocatorEx& operator=(FoxValue& pValue);

	FieldLocatorEx& operator=(Value& pValue)
	{
		int nErrorNo;
		if (nErrorNo = _DBReplace(*this, &pValue))
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(ValueEx& pValue)
	{
		int nErrorNo;
		if (nErrorNo = _DBReplace(*this, pValue))
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(LocatorEx& pLoc)
	{
		int nErrorNo;
		ValueEx pValue;
		pValue = pLoc;
		if (nErrorNo = _DBReplace(*this, pValue))
		{
			pValue.Release();
			throw nErrorNo;
		}
		pValue.Release();
		return *this;
	}

	FieldLocatorEx& operator=(int nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetInt(nValue);
		if (nErrorNo = _DBReplace(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(unsigned int nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		if (nErrorNo = _DBReplace(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(unsigned long nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		if (nErrorNo = _DBReplace(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(void* pPointer)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetPointer(pPointer);
		if (nErrorNo = _DBReplace(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(double nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetDouble(nValue);
		if (nErrorNo = _DBReplace(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(__int64 nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetInt64(nValue);
		if (nErrorNo = _DBReplace(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(unsigned __int64 nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt64(nValue);
		if (nErrorNo = _DBReplace(*this, vTmp))
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(bool bValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetLogical(bValue);
		if (nErrorNo = _DBReplace(*this, vTmp))
			throw nErrorNo;
		return *this;
	}
} FieldLocatorEx;
#pragma pack(pop)

/* extended FoxParameterEx struct with operator-> for easy access to val */
typedef union _FoxParamterEx
{
	ValueEx		val;
	Locator		loc;

	bool IsVariableRef() { return loc.l_type == 'R' && loc.l_where == -1; }
	bool IsMemoRef() { return loc.l_type == 'R' && loc.l_where != -1; }

	ValueEx* operator->() { return static_cast<ValueEx*>(&val); }
	operator ValueEx& () { return val; }
	operator LocatorEx& () { return static_cast<LocatorEx&>(loc); }
} FoxParameterEx;

/* extended ParamBlkEx struct with operator() for easy parameter access */
typedef struct _ParamBlkEx
{
	short 		pCount;			/* Number of Parameters PASSED.			*/
	FoxParameterEx	p[1];		/* pCount Parameters.					*/
	FoxParameterEx& operator()(int nIndex) { return p[nIndex - 1]; }
	short PCount() { return pCount; }
	bool CheckOptionalParameterLen(int nParmNo) { return pCount >= nParmNo && (*this)(nParmNo)->ev_length; }
} ParamBlkEx;

#endif // _VFP2CTYPES_H__