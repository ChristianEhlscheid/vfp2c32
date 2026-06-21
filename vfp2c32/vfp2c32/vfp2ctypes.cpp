#include "vfp2c32.h"

void ValueEx::StoreObjectRef(char* pName, NTI& nVarNti)
{
	// assert(ev_type == 'O');
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
		lVar = this;
		// ObjectRelease(sObject);
		ObjectRelease();
	}
	else
	{
		// nVarNti = NewVar(pName, lVar, true);
		nVarNti = lVar.NewVar(pName, true);
		// Store(lVar, sObject);
		lVar = *this;
		// ObjectRelease(sObject);
		ObjectRelease();
	}
}

void ValueEx::ReleaseObjectRef(char* pName, NTI nVarNti)
{
	ValueEx vObject;
	vObject.SetNull();
	int nErrorNo;
	if (nVarNti)
	{
		nErrorNo = vObject.Evaluate(pName);
		if (nErrorNo)
			throw nErrorNo;
		nErrorNo = _Release(nVarNti);
		if (nErrorNo < 0)
			throw -nErrorNo;
	}
}

ValueEx& ValueEx::operator<<(FoxObject& pObject)
{
	int nErrorNo = _GetObjectProperty(*this, pObject, pObject.Property());
	if (nErrorNo)
	{
		if (nErrorNo == E_PROPERTYNOTFOUND)
			ev_type = '0';
		else
			throw nErrorNo;
	}
#if defined(_DEBUGALLOCATIONS)
	if (ev_type == 'C')
		VfpAllocationCount++;
#endif
	return *this;
}

LocatorEx& LocatorEx::operator<<(FoxObject& pObject)
{
	int nErrorNo;
	FoxValue pValue;
	pValue << pObject;
	if (l_where == -1)
	{
		nErrorNo = _Store(*this, pValue);
		if (nErrorNo)
			throw nErrorNo;
	}
	else
	{
		nErrorNo = _DBReplace(*this, pValue);
		if (nErrorNo)
			throw nErrorNo;
	}
	return *this;
}

LocatorEx& LocatorEx::operator=(FoxValue& pValue)
{
	int nErrorNo;
	if (l_where == -1)
	{
		nErrorNo = _Store(*this, pValue);
		if (nErrorNo)
			throw nErrorNo;
	}
	else
	{
		nErrorNo = _DBReplace(*this, pValue);
		if (nErrorNo)
			throw nErrorNo;
	}
	return *this;
}

VarLocatorEx& VarLocatorEx::operator<<(FoxObject& pObject)
{
	FoxValue pValue;
	pValue << pObject;
	int nErrorNo = _Store(*this, pValue);
	if (nErrorNo)
		throw nErrorNo;
	return *this;
}

VarLocatorEx& VarLocatorEx::operator=(FoxValue& pValue)
{
	int nErrorNo = _Store(*this, pValue);
	if (nErrorNo)
		throw nErrorNo;
	return *this;
}

FieldLocatorEx& FieldLocatorEx::operator<<(FoxObject& pObject)
{
	FoxValue pValue;
	pValue << pObject;
	int nErrorNo = _DBReplace(*this, pValue);
	if (nErrorNo)
		throw nErrorNo;
	return *this;
}

FieldLocatorEx& FieldLocatorEx::operator=(FoxValue& pValue)
{
	int nErrorNo = _DBReplace(*this, pValue);
	if (nErrorNo)
		throw nErrorNo;
	return *this;
}