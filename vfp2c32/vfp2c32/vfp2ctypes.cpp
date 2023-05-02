#include "vfp2c32.h"

void ValueEx::StoreObjectRef(char* pName, NTI& nVarNti)
{
	// assert(ev_type == 'O');
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
	vObject = 0;
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


LocatorEx& LocatorEx::operator<<(FoxObject& pObject)
{
	int nErrorNo;
	FoxValue pValue;
	pValue << pObject;
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

LocatorEx& LocatorEx::operator=(FoxValue& pValue)
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

VarLocatorEx& VarLocatorEx::operator<<(FoxObject& pObject)
{
	int nErrorNo;
	FoxValue pValue;
	pValue << pObject;
	if (nErrorNo = _Store(*this, pValue))
		throw nErrorNo;
	return *this;
}


VarLocatorEx& VarLocatorEx::operator=(FoxValue& pValue)
{
	int nErrorNo;
	if (nErrorNo = _Store(*this, pValue))
		throw nErrorNo;
	return *this;
}

FieldLocatorEx& FieldLocatorEx::operator<<(FoxObject& pObject)
{
	int nErrorNo;
	FoxValue pValue;
	pValue << pObject;
	if (nErrorNo = _DBReplace(*this, pValue))
		throw nErrorNo;
	return *this;
}


FieldLocatorEx& FieldLocatorEx::operator=(FoxValue& pValue)
{
	int nErrorNo;
	if (nErrorNo = _DBReplace(*this, pValue))
		throw nErrorNo;
	return *this;
}

