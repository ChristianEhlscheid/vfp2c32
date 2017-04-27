#include <windows.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2cregistry.h"
#include "vfp2ccppapi.h"
#include "vfp2chelpers.h"
#include "vfpmacros.h"

void _fastcall CreateRegistryKey(ParamBlk *parm)
{
try
{
	HKEY hRoot = reinterpret_cast<HKEY>(p1.ev_long);
	FoxString pKey(p2);
	REGSAM nKeyRights = (PCount() < 3) ? KEY_ALL_ACCESS : p3.ev_long;
	DWORD nOptions = (PCount() < 4) ? REG_OPTION_NON_VOLATILE : p4.ev_long;
	FoxString pClass(parm,5);
	RegistryKey hKey;
	hKey.Create(hRoot, pKey, pClass, nOptions, nKeyRights);
	Return(hKey.Detach());
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall DeleteRegistryKey(ParamBlk *parm)
{
try
{
	HKEY hRoot = reinterpret_cast<HKEY>(p1.ev_long);
	FoxString pKey(p2);
	bool bShell = PCount() == 2 || p3.ev_long != REG_DELETE_NORMAL;
	RegistryKey hKey;

	Return(hKey.Delete(hRoot,pKey,bShell));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall OpenRegistryKey(ParamBlk *parm)
{
try
{
	HKEY hRoot = reinterpret_cast<HKEY>(p1.ev_long);
	FoxString pKeyName(p2);
	REGSAM nKeyRights = (PCount() == 2) ? KEY_ALL_ACCESS : (REGSAM)p3.ev_long;

	RegistryKey hKey;

	hKey.Open(hRoot,pKeyName,nKeyRights);
	Return(hKey.Detach());
}
catch(int nErrorNo)
{
		RaiseError(nErrorNo);
}
}

void _fastcall CloseRegistryKey(ParamBlk *parm)
{
	LONG nApiRet = RegCloseKey(reinterpret_cast<HKEY>(p1.ev_long));
	if (nApiRet != ERROR_SUCCESS)
	{
		SaveWin32Error("RegCloseKey", nApiRet);
		RaiseError(E_APIERROR);
	}
}

void _fastcall ReadRegistryKey(ParamBlk *parm)
{
try
{
	HKEY hRoot = reinterpret_cast<HKEY>(p1.ev_long);
	FoxString pValueName(parm,2);
	FoxString pKeyName(parm,3);
	FoxString pType(parm, 4);
	FoxString pBuffer;
 	RegistryKey hKey;

	DWORD nValueType, dwSize;
	char *pValue;
	char cType;

 	hKey.Open(hRoot, pKeyName, KEY_QUERY_VALUE);
	dwSize = hKey.QueryValueInfo(pValueName, nValueType);

	if (pType.Len())
		cType = pType[0];
	else
	{
		switch(nValueType)
		{
			case REG_SZ:
			case REG_EXPAND_SZ:
			case REG_MULTI_SZ:
			case REG_LINK:
			case REG_NONE:
				cType = 'C';
				break;

			case REG_BINARY:
				cType = 'Q';
				break;

			case REG_QWORD:
				cType = 'Y';
				break;

			case REG_DWORD:
			case REG_DWORD_BIG_ENDIAN:
			case REG_DOUBLE:
				cType = 'N';
				break;

			case REG_INTEGER:
				cType = 'I';
				break;

			case REG_LOGICAL:
				cType = 'L';
				break;

			case REG_DATE:
				cType = 'D';
				break;

			case REG_DATETIME:
				cType = 'T';
				break;

			case REG_MONEY:
				cType = 'Y';
				break;

			default:
				cType = 'C';
		}
	}

	pBuffer.Size(dwSize);
	pValue = pBuffer;
 	hKey.QueryValue(pValueName, pBuffer, &dwSize);

	switch(nValueType)
	{
		case REG_DWORD:
		case REG_INTEGER:
		case REG_LOGICAL:
			if (cType == 'N')
				Return(*reinterpret_cast<DWORD*>(pValue));
			else if (cType == 'I')
				Return(*reinterpret_cast<int*>(pValue));
			else if (cType == 'L')
				Return((*reinterpret_cast<DWORD*>(pValue)) > 0);
			else if (cType == 'Q')
			{
				pBuffer.Binary(true);
				pBuffer.Len(dwSize);
				pBuffer.Return();
			}
			else
				throw E_INVALIDPARAMS;
			break;

		case REG_DWORD_BIG_ENDIAN:
			if (cType == 'N')
				Return(SwapEndian(*reinterpret_cast<DWORD*>(pValue)));
			else if (cType == 'I')
				Return(SwapEndian(*reinterpret_cast<int*>(pValue)));
			else if (cType == 'L')
				Return((*reinterpret_cast<DWORD*>(pValue)) > 0);
			else if (cType == 'Q')
			{
				pBuffer.Binary(true);
				pBuffer.Len(dwSize);
				pBuffer.Return();
			}
			else
				throw E_INVALIDPARAMS;
			break;

		case REG_QWORD:
			if (cType == 'Y')
				ReturnInt64AsCurrency(*reinterpret_cast<unsigned __int64*>(pValue));
			else if (cType == 'Q')
			{
				pBuffer.Len(dwSize);
				pBuffer.Return();
			}
			else if (cType == 'C')
				ReturnInt64AsString(*reinterpret_cast<unsigned __int64*>(pValue));
			else if (cType == 'N')
				ReturnInt64AsDouble(*reinterpret_cast<unsigned __int64*>(pValue));
			else
				throw E_INVALIDPARAMS;
			break;

		case REG_DOUBLE:
		case REG_DATE:
		case REG_DATETIME:
			if (cType == 'N')
				Return(*reinterpret_cast<double*>(pValue));
			else if (cType == 'D')
			{
				FoxDate pDate;
				pDate = *reinterpret_cast<double*>(pValue);
				pDate.Return();
			}
			else if (cType == 'T')
			{
				FoxDateTime pDateTime;
				pDateTime = *reinterpret_cast<double*>(pValue);
				pDateTime.Return();
			}
			else if (cType == 'Q')
			{
				pBuffer.Len(dwSize);
				pBuffer.Return();
			}
			else
				throw E_INVALIDPARAMS;
			break;

		case REG_MONEY:
			if (cType == 'Y')
			{
				CCY nMoney;
				nMoney.QuadPart = *reinterpret_cast<__int64*>(pValue);
				Return(nMoney);
			}
			else if (cType == 'Q')
			{
				pBuffer.Len(dwSize);
				pBuffer.Return();
			}
			else
				throw E_INVALIDPARAMS;
			break;

		default:
			if (cType == 'C' || cType == 'Q')
			{
				pBuffer.Len(dwSize);
				pBuffer.Return();
			}
			else
				throw E_INVALIDPARAMS;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall WriteRegistryKey(ParamBlk *parm)
{
try
{
	HKEY hRoot = reinterpret_cast<HKEY>(p1.ev_long);
	FoxString pData(parm,2,0);
	FoxMemo pMemo(parm, 2);
	FoxString pValueName(parm,3);
	FoxString pKeyName(parm,4);
	RegistryKey hKey;
	char cType = Vartype(p2);
	BYTE *pValueData;
	DWORD nValueSize, nValueType = PCount() == 5 ? p5.ev_long : 0;
	DWORD nDWord;
	unsigned __int64 nQWord;

	switch(cType)
	{
		case 'C':
			if (nValueType == 0)
			{
				if (pData.Binary())
					nValueType = REG_BINARY;
				else
					nValueType = REG_SZ;
			}

			if (nValueType == REG_SZ || nValueType == REG_EXPAND_SZ)
				pData.Expand(1);
			else if (nValueType == REG_MULTI_SZ)
				pData.Expand(2);

			nValueSize = pData.Len();
			pValueData = pData;
			break;

		case 'I':
			if (nValueType == 0)
				nValueType = REG_INTEGER;

			if (nValueType == REG_INTEGER || nValueType == REG_DWORD)
			{
				pValueData = reinterpret_cast<BYTE*>(&p2.ev_long);
				nValueSize = sizeof(int);
			}
			else if (nValueType == REG_QWORD)
			{
				nQWord = static_cast<unsigned __int64>(p2.ev_long);
				pValueData = reinterpret_cast<BYTE*>(&nQWord);
				nValueSize = sizeof(unsigned __int64);
			}
			else if (nValueType == REG_DOUBLE)
			{
				p2.ev_real = static_cast<double>(p2.ev_long);
				pValueData = reinterpret_cast<BYTE*>(&p2.ev_real);
				nValueSize = sizeof(double);
			}
			else if (nValueType == REG_BINARY)
			{
				pValueData = reinterpret_cast<BYTE*>(&p2.ev_long);
				nValueSize = sizeof(int);
			}
			else
				throw E_INVALIDPARAMS;
			break;

		case 'N':
			if (nValueType == 0)
				nValueType = REG_DOUBLE;

			if (nValueType == REG_DOUBLE)
			{
				pValueData = reinterpret_cast<BYTE*>(&p2.ev_real);
				nValueSize = sizeof(double);
			}
			else if (nValueType == REG_DWORD)
			{
				nDWord = static_cast<DWORD>(p2.ev_real);
				pValueData = reinterpret_cast<BYTE*>(&nDWord);
				nValueSize = sizeof(DWORD);
			}
			else if (nValueType == REG_QWORD)
			{
				nQWord = static_cast<unsigned __int64>(p2.ev_real);
				pValueData = reinterpret_cast<BYTE*>(&nQWord);
				nValueSize = sizeof(unsigned __int64);
			}
			else if (nValueType == REG_INTEGER)
			{
				p2.ev_long = static_cast<int>(p2.ev_real);
				pValueData = reinterpret_cast<BYTE*>(&p2.ev_long);
				nValueSize = sizeof(int);
			}
			else if (nValueType == REG_BINARY)
			{
				pValueData = reinterpret_cast<BYTE*>(&p2.ev_real);
				nValueSize = sizeof(double);
			}
			else
				throw E_INVALIDPARAMS;

			break;

		case 'D':
			if (nValueType == 0)
				nValueType = REG_DATE;
			
			if (nValueType == REG_DATE || nValueType == REG_DATETIME || nValueType == REG_DOUBLE || nValueType == REG_BINARY)
			{
		        pValueData = reinterpret_cast<BYTE*>(&p2.ev_real);
				nValueSize = sizeof(double);
			}
			else
				throw E_INVALIDPARAMS;
			break;

		case 'T':
			if (nValueType == 0)
				nValueType = REG_DATETIME;

			if (nValueType == REG_DATETIME || nValueType == REG_DOUBLE || nValueType == REG_BINARY)
			{
				pValueData = reinterpret_cast<BYTE*>(&p2.ev_real);
				nValueSize = sizeof(double);
			}
			else
				throw E_INVALIDPARAMS;
			break;

		case 'L':
			if (nValueType == 0)
				nValueType = REG_LOGICAL;

			if (nValueType == REG_LOGICAL || nValueType == REG_DWORD || nValueType == REG_DWORD_BIG_ENDIAN || nValueType == REG_INTEGER || nValueType == REG_BINARY)
			{
				pValueData = reinterpret_cast<BYTE*>(&p2.ev_length);
				nValueSize = sizeof(DWORD);
			}	
			else
				throw E_INVALIDPARAMS;
			break;

		case 'Y':
			if (nValueType == 0)
				nValueType = REG_MONEY;

			if (nValueType == REG_MONEY || nValueType == REG_BINARY)
			{
				pValueData = reinterpret_cast<BYTE*>(&p2.ev_currency.QuadPart);
				nValueSize = sizeof(unsigned __int64);
			}
			else
				throw E_INVALIDPARAMS;
			break;

		case 'R':
			{
				if (!IsMemoRef(r2))
					throw E_INVALIDPARAMS;

				unsigned int nLen;
				if (nValueType == 0)
					nValueType = REG_SZ;

				if (nValueType == REG_BINARY)
					nLen = nValueSize = pMemo.Size();
				else if (nValueType == REG_SZ || nValueType == REG_EXPAND_SZ)
					nLen = nValueSize = pMemo.Size() + 1;
				else if (nValueType == REG_MULTI_SZ)
					nLen = nValueSize = pMemo.Size() + 2;
				else
					throw E_INVALIDPARAMS;

				pValueData = reinterpret_cast<BYTE*>(pMemo.Read(nLen));

				if (nValueType == REG_SZ || nValueType == REG_EXPAND_SZ)
					pValueData[nLen] = '\0';
				else if (nValueType == REG_MULTI_SZ)
				{
					pValueData[nLen] = '\0';
					pValueData[nLen+1] = '\0';
				}
			}
			break;

		default:
			throw E_INVALIDPARAMS;
	}
 
	hKey.Open(hRoot,pKeyName,KEY_SET_VALUE);
	hKey.SetValue(pValueName,pValueData,nValueSize,nValueType);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ARegistryKeys(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1);
	HKEY hRoot = reinterpret_cast<HKEY>(p2.ev_long);
	FoxString pKeyName(p3);
	DWORD dwFlags = PCount() == 4 ? p4.ev_long : 0;

	RegistryKey hKey;

	FoxString pKeyBuffer;
	FoxString pClassBuffer;
	FoxDateTime pTime;

	DWORD nSubKeys, nSubKeyMaxLen, nClassMaxLen, dwKeyLen, dwClassLen;;
	FILETIME sLastWrite;
	int nDimensions = 1, nWriteTimeDim;
	bool bEnumClassName, bEnumWriteTime, bRet;

	bEnumClassName = (dwFlags & REG_ENUMCLASSNAME) > 0;
	bEnumWriteTime = (dwFlags & REG_ENUMWRITETIME) > 0;
	
	if (bEnumClassName)
		nDimensions++;
	if (bEnumWriteTime)
	{
		nDimensions++;
		nWriteTimeDim = bEnumClassName ? 3 : 2;
	}

	hKey.Open(hRoot,pKeyName,KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);

	hKey.QueryInfo(0,0,&nSubKeys,&nSubKeyMaxLen,&nClassMaxLen);

	if (nSubKeys == 0)
	{
		Return(0);
		return;
	}

	pKeyBuffer.Size(nSubKeyMaxLen);
	pClassBuffer.Size(nClassMaxLen);

	dwKeyLen = pKeyBuffer.Size();
	dwClassLen = pClassBuffer.Size();

	pArray.Dimension(nSubKeys,nDimensions);

	bRet = hKey.EnumFirstKey(pKeyBuffer,&dwKeyLen,pClassBuffer,&dwClassLen,&sLastWrite);

	unsigned int nRow = 0;
	while (bRet)
	{
		pKeyBuffer.Len(dwKeyLen);
		pClassBuffer.Len(dwClassLen);

		nRow++;
		pArray(nRow,1) = pKeyBuffer;
		if (bEnumClassName)
			pArray(nRow,2) = pClassBuffer;
		if (bEnumWriteTime)
		{
			pArray(nRow,nWriteTimeDim) = pTime = sLastWrite;
		}

		dwKeyLen = pKeyBuffer.Size();
		dwClassLen = pClassBuffer.Size();
		
		bRet = hKey.EnumNextKey(pKeyBuffer,&dwKeyLen,pClassBuffer,&dwClassLen,&sLastWrite);
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ARegistryValues(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1);
	HKEY hRoot = reinterpret_cast<HKEY>(p2.ev_long);
	FoxString pKeyName(p3);
	DWORD dwFlags = PCount() == 4 ? p4.ev_long : 0;

	RegistryKey hKey;
	FoxString pValueName;
	FoxString pValue;

	FoxDate pDate;
	FoxDateTime pDateTime;
	FoxCurrency pCurrency;

	LPBYTE pRegValue;
	DWORD nValues, nNameLen, nValueLen, nValueType;
	int nValueDim = 2, nDimensions = 1;
	bool bRegType, bRegValue, bRet;

	bRegType = (dwFlags & REG_ENUMTYPE) > 0;
	bRegValue = (dwFlags & REG_ENUMVALUE) > 0;
	if (bRegType)
	{
		nValueDim++;
		nDimensions++;
	}
	if (bRegValue)
		nDimensions++;

	hKey.Open(hRoot,pKeyName,KEY_QUERY_VALUE);
	hKey.QueryInfo(0,0,0,0,0,&nValues,&nNameLen,&nValueLen);

	if (nValues == 0)
	{
		Return(0);
		return;
	}

	pValueName.Size(nNameLen);
	if (bRegValue)
		pValue.Size(nValueLen);

	pRegValue = pValue;
	nValueLen = pValue.Size();

    pArray.Dimension(nValues,nDimensions);
	
	bRet = hKey.EnumFirstValue(pValueName,&nNameLen,pRegValue,&nValueLen,&nValueType);

	unsigned int nRow = 0;
	while (bRet)
	{
		nRow++;

		pValueName.Len(nNameLen);
		pArray(nRow,1) = pValueName;

		if (bRegType)
			pArray(nRow,2) = nValueType;

		if (bRegValue)
		{

			switch (nValueType)
			{
				case REG_NONE:
				case REG_SZ:
				case REG_EXPAND_SZ:
				case REG_BINARY:
				case REG_LINK:
				case REG_MULTI_SZ:
				case REG_RESOURCE_LIST:
				case REG_FULL_RESOURCE_DESCRIPTOR:
				case REG_RESOURCE_REQUIREMENTS_LIST:
					pValue.Len(nValueLen);
					pArray(nRow,nValueDim) = pValue;
					break;

				case REG_QWORD:
				case REG_MONEY:
					pArray(nRow, nValueDim) = pCurrency = *reinterpret_cast<__int64*>(pRegValue);
					break;

				case REG_DWORD:
					pArray(nRow,nValueDim) = *reinterpret_cast<DWORD*>(pRegValue);
					break;

				case REG_DWORD_BIG_ENDIAN:
					pArray(nRow,nValueDim) = SwapEndian(*reinterpret_cast<DWORD*>(pRegValue));
					break;

				case REG_INTEGER:
					pArray(nRow,nValueDim) = *reinterpret_cast<int*>(pRegValue);
					break;
				
				case REG_DOUBLE:
					pArray(nRow,nValueDim) = *reinterpret_cast<double*>(pRegValue);
					break;

				case REG_DATE:
					pArray(nRow,nValueDim) = pDate = *reinterpret_cast<double*>(pRegValue);
					break;

				case REG_DATETIME:
					pArray(nRow,nValueDim) = pDateTime = *reinterpret_cast<double*>(pRegValue);
					break;

				case REG_LOGICAL:
					pArray(nRow,nValueDim) = (*reinterpret_cast<DWORD*>(pRegValue) > 0);
					break;

				default:
					pValue.Len(nValueLen);
					pArray(nRow,nValueDim) = pValue;
			}
		}


		nNameLen = pValueName.Size();
		nValueLen = pValue.Size();

		bRet = hKey.EnumNextValue(pValueName,&nNameLen,pValue,&nValueLen,&nValueType);
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall RegistryValuesToObject(ParamBlk *parm)
{
try
{
	HKEY hRoot = reinterpret_cast<HKEY>(p1.ev_long);
	FoxString pKeyName(p2);
	FoxObject pObject(p3);

	RegistryKey hKey;
	FoxString pValue;
	CStr pValueName;
	FoxCurrency pCurrency;
	FoxDate pDate;
	FoxDateTime pDateTime;
	LPBYTE pRegValue;
	DWORD nValues, nValueNameLen, nValueLen, nValueType;
	bool bRet;

	// open key and query value information
	hKey.Open(hRoot, pKeyName, KEY_QUERY_VALUE);
	hKey.QueryInfo(0, 0, 0, 0, 0, &nValues, &nValueNameLen, &nValueLen);

	if (nValues == 0)
	{
		Return(0);
		return;
	}

	// allocate needed temporary buffers
	pValue.Size(nValueLen);
	pRegValue = pValue;
	pValueName.Size(nValueNameLen);
	nValueNameLen = pValueName.Size();

	// start enumeration
	bRet = hKey.EnumFirstValue(pValueName, &nValueNameLen, pValue, &nValueLen, &nValueType);

	while (bRet)
	{
		// convert value name to a valid VFP property name
		pValueName.RegValueToPropertyName();

		// store value into the object property
		switch(nValueType)
		{
			case REG_NONE:
			case REG_SZ:
			case REG_EXPAND_SZ:
			case REG_BINARY:
			case REG_LINK:
			case REG_MULTI_SZ:
			case REG_RESOURCE_LIST:
			case REG_FULL_RESOURCE_DESCRIPTOR:
			case REG_RESOURCE_REQUIREMENTS_LIST:
				pValue.Len(nValueLen);
				pObject(pValueName) << pValue;
				break;

			case REG_DWORD:
				pObject(pValueName) << *(DWORD*)pRegValue;
				break;

			case REG_DWORD_BIG_ENDIAN:
				pObject(pValueName) << (DWORD)SwapEndian(*(DWORD*)pRegValue);
				break;

			case REG_QWORD:
			case REG_MONEY:
				pObject(pValueName) << (pCurrency = *reinterpret_cast<__int64*>(pRegValue));
				break;

			case REG_INTEGER:
				pObject(pValueName) << *reinterpret_cast<int*>(pRegValue);
				break;

			case REG_DOUBLE:
				pObject(pValueName) << *reinterpret_cast<double*>(pRegValue);
				break;

			case REG_DATE:
				pObject(pValueName) << (pDate = (*reinterpret_cast<double*>(pRegValue)));
				break;

			case REG_DATETIME:
				pObject(pValueName) << (pDateTime = *reinterpret_cast<double*>(pRegValue));
				break;

			case REG_LOGICAL:
				pObject(pValueName) << (*reinterpret_cast<DWORD*>(pRegValue) > 0);
				break;

			default:
				pValue.Len(nValueLen);
				pObject(pValueName) << pValue;
		}

		// reassign max value(name) len to variables for the next enumeration
		nValueNameLen = pValueName.Size();
		nValueLen = pValue.Size();

		bRet = hKey.EnumNextValue(pValueName, &nValueNameLen, pValue, &nValueLen, &nValueType);
	}

	Return((int)nValues);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall RegistryHiveToObject(ParamBlk *parm)
{
try
{
	HKEY hRoot = reinterpret_cast<HKEY>(p1.ev_long);
	FoxString pKeyName(p2);
	FoxObject pObject(p3);
	RegistryHiveSubroutine(hRoot,pKeyName,pObject);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _stdcall RegistryHiveSubroutine(HKEY hRoot, char* pKey, FoxObject& pObject)
{
	RegistryKey hKey;
	FoxString pValue;
	CStr pName, pProperty, pSubKey;
	DWORD nValues, nValueNameLen, nValueLen, nValueType, nSubKeys, nSubKeyLen;
	LPBYTE pRegValue;
	bool bRet;

	hKey.Open(hRoot,pKey,KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);
	hKey.QueryInfo(0,0,&nSubKeys,&nSubKeyLen,0,&nValues,&nValueNameLen,&nValueLen);

	if (nValues > 0)
		pValue.Size(nValueLen);
	pRegValue = pValue;

	pName.Size(max(nSubKeyLen, nValueNameLen));
	if (nSubKeys > 0)
		pSubKey.Size(nSubKeyLen + strlen(pKey) + 1);
	
	pProperty.Size(pName.Size());
	
	if (nValues > 0)
	{
		FoxDate pDate;
		FoxDateTime pDateTime;
		FoxCurrency pCurrency;

		bRet = hKey.EnumFirstValue(pName,&nValueNameLen,pValue,&nValueLen,&nValueType);

		while (bRet)
		{
			pProperty = pName.Len(nValueNameLen);
			pProperty.RegValueToPropertyName();

			switch(nValueType)
			{
				case REG_NONE:
				case REG_SZ:
				case REG_EXPAND_SZ:
				case REG_BINARY:
				case REG_LINK:
				case REG_MULTI_SZ:
				case REG_RESOURCE_LIST:
				case REG_FULL_RESOURCE_DESCRIPTOR:
				case REG_RESOURCE_REQUIREMENTS_LIST:
					pValue.Len(nValueLen);
					pObject(pProperty) << pValue;
					break;

				case REG_DWORD:
					pObject(pProperty) << *(DWORD*)pRegValue;
					break;

				case REG_DWORD_BIG_ENDIAN:
					pObject(pProperty) << (DWORD)SwapEndian(*(DWORD*)pRegValue);
					break;

				case REG_QWORD:
				case REG_MONEY:
					pObject(pProperty) << (pCurrency = *reinterpret_cast<__int64*>(pRegValue));
					break;

				case REG_INTEGER:
					pObject(pProperty) << *reinterpret_cast<int*>(pRegValue);
					break;

				case REG_DOUBLE:
					pObject(pProperty) << *reinterpret_cast<double*>(pRegValue);
					break;

				case REG_DATE:
					pObject(pProperty) << (pDate = *reinterpret_cast<double*>(pRegValue));
					break;

				case REG_DATETIME:
					pObject(pProperty) << (pDateTime = *reinterpret_cast<double*>(pRegValue));
					break;

				case REG_LOGICAL:
					pObject(pProperty) << (*reinterpret_cast<DWORD*>(pRegValue) > 0);
					break;

				default:
					pValue.Len(nValueLen);
					pObject(pProperty) << pValue;
			}

			nValueNameLen = pName.Size();
			nValueLen = pValue.Size();
			bRet = hKey.EnumNextValue(pName,&nValueNameLen,pValue,&nValueLen,&nValueType);
		}
	}

	if (nSubKeys > 0)
	{
		bRet = hKey.EnumFirstKey(pName,&nSubKeyLen);
		while (bRet)
		{
			FoxObject pObjectEx;
			pProperty = pName.Len(nSubKeyLen);
			pProperty.RegValueToPropertyName();
			pObjectEx.EmptyObject();
			pObject(pProperty) << pObjectEx;

			pSubKey = pKey;
			pSubKey += "\\";
			pSubKey += pName;

			RegistryHiveSubroutine(hRoot,pSubKey,pObjectEx);
			
			nSubKeyLen = pName.Size();
			bRet = hKey.EnumNextKey(pName,&nSubKeyLen);
		}
	}
}