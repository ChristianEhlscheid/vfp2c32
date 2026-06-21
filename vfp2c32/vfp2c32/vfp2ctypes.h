#ifndef _VFP2CTYPES_H__
#define _VFP2CTYPES_H__

#include <cassert>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif

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
const int E_PROPERTYNOTFOUND = -734;
const int E_VFP_INVALIDPARAMS = 11;
const int E_CUSTOMERROR = 7777;

class FoxValue;
class FoxObject;

// thin wrappers around Value structure to quickly initialize to a specific type
/* modified Value structure for easy access to typed pointers in ev_long */
#pragma pack(push,1)
typedef struct ValueEx : public Value
{
	inline void SetUChar() { ev_type = 'I'; ev_width = 3; ev_long = 0; }
	inline void SetUChar(unsigned char nValue) { ev_type = 'I'; ev_width = 3; ev_long = static_cast<long>(nValue); }
	inline void SetShort() { ev_type = 'I'; ev_width = 6; }
	inline void SetShort(short nValue) { ev_type = 'I'; ev_width = 6; ev_long = static_cast<long>(nValue); }
	inline void SetUShort() { ev_type = 'I'; ev_width = 6; }
	inline void SetUShort(unsigned short nValue) { ev_type = 'I'; ev_width = 6; ev_long = static_cast<long>(nValue); }
	inline void SetInt() { ev_type = 'I'; ev_width = 11; ev_long = 0; }
	inline void SetInt(int nValue) { ev_type = 'I'; ev_width = 11; ev_long = nValue; }
	inline void SetUInt() { ev_type = 'N'; ev_width = 11; ev_length = 0; ev_real = 0; }
	inline void SetUInt(unsigned int nValue) { ev_type = 'N'; ev_width = 11; ev_length = 0; ev_real = static_cast<double>(nValue); }
	inline void SetFloat(unsigned int nPrecision = 7) { ev_type = 'N'; ev_width = 20; ev_length = nPrecision; ev_real = 0; }
	inline void SetFloat(float nValue, unsigned int nPrecision = 7) { ev_type = 'N'; ev_width = 20; ev_length = nPrecision; ev_real = nValue; }
	inline void SetDouble(unsigned int nPrecision = 16) { ev_type = 'N'; ev_width = 20; ev_length = nPrecision; ev_real = 0; }
	inline void SetDouble(double nValue, unsigned int nPrecision = 16) { ev_type = 'N'; ev_width = 20; ev_length = nPrecision; ev_real = nValue; }
	inline void SetCurrency() { ev_type = 'Y'; ev_currency.QuadPart = 0; }
	inline void SetCurrency(CCY nValue) { ev_type = 'Y'; ev_currency.QuadPart = nValue.QuadPart; }
	inline void SetCurrency(__int64 nValue) { ev_type = 'Y'; ev_currency.QuadPart = nValue; }
	inline void SetNumeric(int nWidth, unsigned int nPrecision) { ev_type = 'N'; ev_width = (short)nWidth; ev_length = nPrecision; }
	inline void SetString(unsigned int nLen = 0) { ev_type = 'C'; ev_width = 0; ev_handle = 0; ev_length = nLen; }
	inline void SetBinaryString(unsigned int nLen = 0) { ev_type = 'C'; ev_width = 1; ev_handle = 0; ev_length = nLen; }
	inline void SetLogical() { ev_type = 'L'; ev_length = 0; }
	inline void SetLogical(bool bValue) { ev_type = 'L'; ev_length = bValue; }
	inline void SetLogical(unsigned int bValue) { ev_type = 'L'; ev_length = bValue; }
	inline void SetDate() { ev_type = 'D'; ev_real = 0; }
	inline void SetDate(double nDate) { ev_type = 'D'; ev_real = nDate; }
	inline void SetDateTime() { ev_type = 'T'; ev_real = 0; }
	inline void SetDateTime(double nDate) { ev_type = 'T'; ev_real = nDate; }
	inline void SetInt64() { ev_type = 'N'; ev_width = 20; ev_length = 0; }
	inline void SetInt64(__int64 nValue) { ev_type = 'N'; ev_width = 20; ev_length = 0; ev_real = static_cast<double>(nValue); }
	inline void SetUInt64() { ev_type = 'N'; ev_width = 20; ev_length = 0; }
	inline void SetUInt64(unsigned __int64 nValue) { ev_type = 'N'; ev_width = 20; ev_length = 0; ev_real = static_cast<double>(nValue); }
#if !defined(_WIN64)
	inline void SetPointer() { ev_type = 'I'; ev_width = 11; ev_long = 0; }
	inline void SetPointer(void* pValue) { ev_type = 'I'; ev_width = 11; ev_long = reinterpret_cast<long>(pValue); }
	template<typename T>
	inline T Ptr() { return reinterpret_cast<T>(ev_long); }
	inline void* Ptr() { return reinterpret_cast<void*>(ev_long); }
#else
	inline void SetPointer() { ev_type = 'N'; ev_width = 20; ev_real = 0; }
	inline void SetPointer(void* pValue) { ev_type = 'N'; ev_width = 20; ev_real = static_cast<double>(reinterpret_cast<unsigned __int64>(pValue)); }
	template<typename T>
	inline T Ptr() { return reinterpret_cast<T>(ev_long < 0 ? static_cast<UINT_PTR>(ev_real) : static_cast<UINT_PTR>(ev_long)); }
	template<>
	inline UINT_PTR Ptr<UINT_PTR>() { return static_cast<UINT_PTR>(ev_long > -1 ? ev_long : ev_real); }
	inline void* Ptr() { return reinterpret_cast<void*>(ev_long > -1 ? static_cast<UINT_PTR>(ev_long) : static_cast<UINT_PTR>(ev_real)); }
#endif
	template<typename T>
	inline T DynamicPtr() const { return reinterpret_cast<T>(ev_type == 'I' ? static_cast<UINT_PTR>(ev_long) : static_cast<UINT_PTR>(ev_real)); }
	template<>
	inline UINT_PTR DynamicPtr<UINT_PTR>() const { return ev_type == 'I' ? static_cast<UINT_PTR>(ev_long) : static_cast<UINT_PTR>(ev_real); }

	inline void SetNull() { ev_type = '0'; }
	inline char Vartype() const { return ev_type; }
	inline bool Binary() const { return ev_width > 0; }
	inline bool Numeric() const { return ev_type == 'I' || ev_type == 'N'; }
	template<typename T>
	inline T NumericCast() const { return ev_type == 'I' ? static_cast<T>(ev_long) : static_cast<T>(ev_real); }

	inline unsigned int Len() const { return ev_length; }

	bool AllocHandle(unsigned int nBytes)
	{
		assert(ev_type == 'C' && ev_handle == 0); 
		ev_handle = _AllocHand(nBytes); 
#if defined(_DEBUGALLOCATIONS)
		if (ev_handle)
			VfpAllocationCount++;
#endif
		return ev_handle != 0;
	}

	void FreeHandle()
	{ 
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

	void FreeObject()
	{
		assert(ev_type == 'O');
		if (ev_object > 0)
		{
			_FreeObject(*this);
			ev_object = 0;
		}
	}
	
	inline char* HandleToPtr()
	{
		assert(ev_type == 'C' && ev_handle != 0);
		return reinterpret_cast<char*>(_HandToPtr(ev_handle));
	}

	template<typename T>
	inline T HandleToPtr()
	{ 
		assert(ev_type == 'C' && ev_handle != 0);
		return reinterpret_cast<T>(_HandToPtr(ev_handle));
	}

	inline bool ValidHandle() const
	{ 
		assert(ev_type == 'C'); 
		return ev_handle > 0;
	}

	inline void LockHandle()
	{ 
		assert(ev_type == 'C' && ev_handle != 0);  
		_HLock(ev_handle);
	}

	inline void UnlockHandle()
	{ 
		assert(ev_type == 'C' && ev_handle != 0);  
		_HUnLock(ev_handle);
	}

	inline unsigned long GetHandleSize() const
	{ 
		assert(ev_type == 'C' && ev_handle != 0);
		return _GetHandSize(ev_handle);
	}

	inline bool SetHandleSize(unsigned long nSize)
	{ 
		assert(ev_type == 'C' && ev_handle != 0); 
		return _SetHandSize(ev_handle, nSize) > 0;
	}

	inline bool ExpandHandleSize(unsigned int nBytes)
	{ 
		assert(ev_type == 'C' && ev_handle != 0); 
		return _SetHandSize(ev_handle, ev_length + nBytes) > 0;
	}

	inline bool NullTerminate()
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
		{
			_FreeObject(*this);
			ev_object = 0;
		}
	}

	void UnlockAndFree()
	{
		if (ev_type == 'C' && ev_handle > 0)
		{
#if defined(_DEBUGALLOCATIONS)
			VfpAllocationCount--;
#endif
			_HUnLock(ev_handle);
			_FreeHand(ev_handle);
			ev_handle = 0;
		}
	}

	inline void Return() {
		assert(ev_type != 'R');
		_RetVal(*this);
	}

	inline int Evaluate(const char* pExpression)
	{
		int retval = _Evaluate(*this, (char*)pExpression);
#if defined(_DEBUGALLOCATIONS)
		if (ev_type == 'C')
			VfpAllocationCount++;
#endif
		return retval;
	}

	inline int Evaluate(char* pExpression)
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
		int nErrorNo = _ObjectRelease(*this);
		if (nErrorNo)
			throw nErrorNo;
	}

	void StoreObjectRef(char* pName, NTI& nVarNti);
	static void ReleaseObjectRef(char* pName, NTI nVarNti);

	// casting operators
	// inline operator Value& () { return static_cast<Value&>(*this); }
	inline operator Value* () { return static_cast<Value*>(this); }

	ValueEx& operator=(Locator& pLoc)
	{
		int nErrorNo = _Load(&pLoc, *this);
		if (nErrorNo)
			throw nErrorNo;
#if defined(_DEBUGALLOCATIONS)
		if (ev_type == 'C')
			VfpAllocationCount++;
#endif
		return *this;
	};

	ValueEx& operator<<(FoxObject& pObject);
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
	bool IsVariable() const { return l_type == 'R' && l_where == -1; }
	bool IsField() const { return l_type == 'R' && l_where != -1; }

	NTI NewVar(char* pVarname, bool bPublic)
	{
		l_subs = 0;
		NTI nVarNti = _NewVar(pVarname, *this, bPublic ? NV_PUBLIC : NV_PRIVATE);
		if (nVarNti < 0)
		{
			int nErrorNo = (int)-nVarNti;
			throw nErrorNo;
		}
		return nVarNti;
	}

	NTI FindVar(char* pVarname)
	{
		NTI nVarNti = _NameTableIndex(pVarname);
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
		NTI nVarNti = _NameTableIndex(pVarname);
		if (nVarNti == -1)
			throw E_VARIABLENOTFOUND;
		return nVarNti;
	}

	static void ReleaseVarStatic(NTI nVarNti)
	{
		int nErrorNo = _Release(nVarNti);
		if (nErrorNo)
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
		int nErrorNo = _Store(*this, sValue);
		if (nErrorNo)
			throw nErrorNo;
	}

	void Load(ValueEx& sValue)
	{
		int nErrorNo = _Load(*this, sValue);
		if (nErrorNo)
			throw nErrorNo;
#if defined(_DEBUGALLOCATIONS)
		if (sValue.ev_type == 'C')
			VfpAllocationCount++;
#endif
	}

	long ALen(int mode)
	{
		int nSubscript = _ALen(l_NTI, mode);
		if (nSubscript == -1)
			throw E_NOTANARRAY;
		return nSubscript;
	}

	long AElements()
	{
		long nSubscript = _ALen(l_NTI, AL_ELEMENTS);
		if (nSubscript == -1)
			throw E_NOTANARRAY;
		return nSubscript;
	}

	long ARows()
	{
		long nSubscript = _ALen(l_NTI, AL_SUBSCRIPT1);
		if (nSubscript == -1)
			throw E_NOTANARRAY;
		return nSubscript;
	}

	long ADims()
	{
		int nSubscript = _ALen(l_NTI, AL_SUBSCRIPT2);
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

	// inline operator Locator& () { return static_cast<Locator&>(*this); }
	inline operator Locator* () { return static_cast<Locator*>(this); }
	inline LocatorEx& operator()(int nRow) { l_sub1 = (short)nRow; return *this; }
	inline LocatorEx& operator()(int nRow, int nDim) { l_sub1 = (short)nRow; l_sub2 = (short)nDim; return *this; }

	LocatorEx& operator<<(FoxObject& pObject);
	LocatorEx& operator=(FoxValue& pValue);

	LocatorEx& operator=(Value& pValue)
	{
		int nErrorNo;
		if (l_where == -1)
			nErrorNo = _Store(*this, &pValue);
		else
			nErrorNo = _DBReplace(*this, &pValue);

		if (nErrorNo)
			throw nErrorNo;

		return *this;
	}

	LocatorEx& operator=(ValueEx& pValue)
	{
		int nErrorNo;
		if (l_where == -1)
			nErrorNo = _Store(*this, pValue);
		else
			nErrorNo = _DBReplace(*this, pValue);

		if (nErrorNo)
			throw nErrorNo;

		return *this;
	}

	LocatorEx& operator=(LocatorEx& pLoc)
	{
		int nErrorNo;
		ValueEx pValue;
		pValue.SetNull();
		pValue = pLoc;

		if (l_where == -1)
			nErrorNo = _Store(*this, pValue);
		else
			nErrorNo = _DBReplace(*this, pValue);

		pValue.Release();
		if (nErrorNo)
			throw nErrorNo;

		return *this;
	}

	LocatorEx& operator=(int nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetInt(nValue);
		if (l_where == -1)
			nErrorNo = _Store(*this, vTmp);
		else
			nErrorNo = _DBReplace(*this, vTmp);

		if (nErrorNo)
			throw nErrorNo;

		return *this;
	}

	LocatorEx& operator=(long nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetInt(nValue);
		if (l_where == -1)
			nErrorNo = _Store(*this, vTmp);
		else
			nErrorNo = _DBReplace(*this, vTmp);

		if (nErrorNo)
			throw nErrorNo;

		return *this;
	}

	LocatorEx& operator=(unsigned int nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		if (l_where == -1)
			nErrorNo = _Store(*this, vTmp);
		else
			nErrorNo = _DBReplace(*this, vTmp);

		if (nErrorNo)
			throw nErrorNo;

		return *this;
	}

	LocatorEx& operator=(unsigned long nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		if (l_where == -1)
			nErrorNo = _Store(*this, vTmp);
		else
			nErrorNo = _DBReplace(*this, vTmp);

		if (nErrorNo)
			throw nErrorNo;

		return *this;
	}

	LocatorEx operator=(void* pPointer)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetPointer(pPointer);
		if (l_where == -1)
			nErrorNo = _Store(*this, vTmp);
		else
			nErrorNo = _DBReplace(*this, vTmp);

		if (nErrorNo)
			throw nErrorNo;

		return *this;
	}

	LocatorEx& operator=(double nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetDouble(nValue);
		if (l_where == -1)
			nErrorNo = _Store(*this, vTmp);
		else
			nErrorNo = _DBReplace(*this, vTmp);

		if (nErrorNo)
			throw nErrorNo;

		return *this;
	}

	LocatorEx& operator=(__int64 nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetInt64(nValue);
		if (l_where == -1)
			nErrorNo = _Store(*this, vTmp);
		else
			nErrorNo = _DBReplace(*this, vTmp);

		if (nErrorNo)
			throw nErrorNo;

		return *this;
	}

	LocatorEx& operator=(unsigned __int64 nValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetUInt64(nValue);
		if (l_where == -1)
			nErrorNo = _Store(*this, vTmp);
		else
			nErrorNo = _DBReplace(*this, vTmp);

		if (nErrorNo)
			throw nErrorNo;

		return *this;
	}

	LocatorEx& operator=(bool bValue)
	{
		int nErrorNo;
		ValueEx vTmp;
		vTmp.SetLogical(bValue);
		if (l_where == -1)
			nErrorNo = _Store(*this, vTmp);
		else
			nErrorNo = _DBReplace(*this, vTmp);

		if (nErrorNo)
			throw nErrorNo;

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
		int nErrorNo = _Store(*this, &pValue);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(ValueEx& pValue)
	{
		int nErrorNo = _Store(*this, pValue);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(LocatorEx& pLoc)
	{
		ValueEx pValue;
		pValue = pLoc;
		int nErrorNo = _Store(*this, pValue);
		pValue.Release();
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(int nValue)
	{
		ValueEx vTmp;
		vTmp.SetInt(nValue);
		int nErrorNo = _Store(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(unsigned int nValue)
	{		
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		int nErrorNo = _Store(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(long nValue)
	{
		ValueEx vTmp;
		vTmp.SetInt(nValue);
		int nErrorNo = _Store(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(unsigned long nValue)
	{
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		int nErrorNo = _Store(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(void* pPointer)
	{
		ValueEx vTmp;
		vTmp.SetPointer(pPointer);
		int nErrorNo = _Store(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(double nValue)
	{
		ValueEx vTmp;
		vTmp.SetDouble(nValue);
		int nErrorNo = _Store(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(__int64 nValue)
	{
		ValueEx vTmp;
		vTmp.SetInt64(nValue);
		int nErrorNo = _Store(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(unsigned __int64 nValue)
	{
		ValueEx vTmp;
		vTmp.SetUInt64(nValue);
		int nErrorNo = _Store(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	VarLocatorEx& operator=(bool bValue)
	{
		ValueEx vTmp;
		vTmp.SetLogical(bValue);
		int nErrorNo = _Store(*this, vTmp);
		if (nErrorNo)
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
		int nErrorNo = _DBReplace(*this, &pValue);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(ValueEx& pValue)
	{
		int nErrorNo = _DBReplace(*this, pValue);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(LocatorEx& pLoc)
	{
		ValueEx pValue;
		pValue = pLoc;
		int nErrorNo = _DBReplace(*this, pValue);
		pValue.Release();
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(int nValue)
	{
		ValueEx vTmp;
		vTmp.SetInt(nValue);
		int nErrorNo = _DBReplace(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(long nValue)
	{
		ValueEx vTmp;
		vTmp.SetInt(nValue);
		int nErrorNo = _DBReplace(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(unsigned int nValue)
	{
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		int nErrorNo = _DBReplace(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(unsigned long nValue)
	{
		ValueEx vTmp;
		vTmp.SetUInt(nValue);
		int nErrorNo = _DBReplace(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(void* pPointer)
	{
		ValueEx vTmp;
		vTmp.SetPointer(pPointer);
		int nErrorNo = _DBReplace(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(double nValue)
	{
		ValueEx vTmp;
		vTmp.SetDouble(nValue);
		int nErrorNo = _DBReplace(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(__int64 nValue)
	{
		ValueEx vTmp;
		vTmp.SetInt64(nValue);
		int nErrorNo = _DBReplace(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(unsigned __int64 nValue)
	{
		ValueEx vTmp;
		vTmp.SetUInt64(nValue);
		int nErrorNo = _DBReplace(*this, vTmp);
		if (nErrorNo)
			throw nErrorNo;
		return *this;
	}

	FieldLocatorEx& operator=(bool bValue)
	{
		ValueEx vTmp;
		vTmp.SetLogical(bValue);
		int nErrorNo = _DBReplace(*this, vTmp);
		if (nErrorNo)
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

	inline bool IsVariableRef() const { return loc.l_type == 'R' && loc.l_where == -1; }
	inline bool IsMemoRef() const { return loc.l_type == 'R' && loc.l_where != -1; }

	inline ValueEx* operator->() { return static_cast<ValueEx*>(&val); }
	inline operator ValueEx& () { return val; }
	inline operator LocatorEx& () { return static_cast<LocatorEx&>(loc); }
} FoxParameterEx;

/* extended ParamBlkEx struct with operator() for easy parameter access */
typedef struct _ParamBlkEx
{
	short 		pCount;			/* Number of Parameters PASSED.			*/
	FoxParameterEx	p[1];		/* pCount Parameters.					*/
	inline FoxParameterEx& operator()(int nIndex) { return p[nIndex - 1]; }
	inline short PCount() const { return pCount; }
} ParamBlkEx;

#endif // _VFP2CTYPES_H__