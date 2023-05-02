#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cconv.h"
#include "vfp2cutil.h"
#include "vfp2chelpers.h"
#include "vfp2ccppapi.h"
#include "vfp2casmfuncs.h"

void _fastcall PG_ByteA2Str(ParamBlkEx& parm)
{
try
{
	FoxString vRetVal;
	unsigned char *pInput, *pInputEnd, *pRetVal, *pRetValStart;
	unsigned int nMemoLen = 0;

	if (parm(1)->Vartype() == 'C')
	{
		FoxString vInput(parm(1), 0);
		if (vInput.Len() == 0)
		{
			vRetVal.Return();
			return;
		}

		vRetVal.Size(vInput.Len());
		pRetValStart = pRetVal = vRetVal;
		pInput = vInput;
		pInputEnd = pInput + vInput.Len();
	}
	else if (parm(1).IsMemoRef())
	{
		FoxMemo vMemo(parm(1));
		pInput = reinterpret_cast<unsigned char*>(vMemo.Read(nMemoLen));
		if (nMemoLen == 0)
		{
			vRetVal.Return();
			return;
		}

		pInputEnd = pInput + nMemoLen;
		vRetVal.Size(nMemoLen);
		pRetValStart = pRetVal = vRetVal;
	}
	else
	{
		SaveCustomError("PG_ByteA2Str", "Invalid type '%s' for parameter 1", parm(1)->Vartype());	
		throw E_INVALIDPARAMS;
	}

	while (pInput < pInputEnd)
	{
		if (*pInput == '\\')
		{
			if	(pInput[1] >= '0' && pInput[1] <= '7' && 
				pInput[2] >= '0' && pInput[2] <= '7' &&
				pInput[3] >= '0' && pInput[3] <= '7')
			{
				*pRetVal++ = (pInput[1] - '0') * 64	+ (pInput[2] - '0') * 8 + (pInput[3] - '0');
				pInput += 4;
				continue;
			}
			else if (pInput[1] == '\\')
			{
				*pRetVal++ = '\\';
				pInput += 2;
			}
			else // unrecognized sequence after '\', just output as is ..
				*pRetVal++ = *pInput++;
		}
		else
			*pRetVal++ = *pInput++;
	}

	vRetVal.Len(pRetVal - pRetValStart);
	vRetVal.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall PG_Str2ByteA(ParamBlkEx& parm)
{
try
{
	FoxString vRetVal;
	unsigned char *pInput, *pInputEnd, *pRetVal, *pRetValStart;
	unsigned int nMemoLen = 0;
	bool bDouble = parm.PCount() == 2 ? parm(2)->ev_length > 0 : false;
	int nMemMulti = bDouble == false ? 4 : 5;

	if (parm(1)->Vartype() == 'C')
	{
		FoxString vInput(parm(1), 0);
		if (vInput.Len() == 0)
		{
			vRetVal.Return();
			return;
		}
		// allocate 4/5 times the space of the original which is the maximum size
		// the data can grow if all characters have to be translated to octal reprensentation
		vRetVal.Size(vInput.Len() * nMemMulti);

		pRetValStart = pRetVal = vRetVal;
		pInput = vInput;
		pInputEnd = pInput + vInput.Len();
	}
	else if (parm(1).IsMemoRef())
	{
		FoxMemo vMemo(parm(1));
		pInput = reinterpret_cast<unsigned char*>(vMemo.Read(nMemoLen));
		if (nMemoLen == 0)
		{
			vRetVal.Return();
			return;
		}
		pInputEnd = pInput + nMemoLen;

		// 4/5 times .. see comment above ..
		vRetVal.Size(nMemoLen * nMemMulti);
		pRetValStart = pRetVal = vRetVal;
	}
	else
	{
		SaveCustomError("PG_Str2ByteA", "Invalid type '%s' for parameter 1", parm(1)->Vartype());
		throw E_INVALIDPARAMS;
	}

	while (pInput < pInputEnd)
	{
		if (*pInput < 32 || *pInput > 126 || *pInput == '\\' || *pInput == '\'')
		{
			if (bDouble)
				*pRetVal++ = '\\';

			*pRetVal++ = '\\';
			*pRetVal++ = '0' + (*pInput / 64) % 8;
			*pRetVal++ = '0' + (*pInput / 8) % 8;
			*pRetVal++ = '0' + *pInput % 8;
			pInput++;
		}
		else
			*pRetVal++ = *pInput++;
	}

	vRetVal.Len(pRetVal - pRetValStart);
	vRetVal.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall RGB2Colors(ParamBlkEx& parm)
{
try
{
	LocatorEx& rRed = parm(2);
	LocatorEx& rGreen = parm(3);
	LocatorEx& rBlue = parm(4);
	LocatorEx& rAlpha = parm(5);

	rRed = (parm(1)->ev_long & 0xFF);
	rGreen = (parm(1)->ev_long >> 8 & 0xFF);
	rBlue = (parm(1)->ev_long >> 16 & 0xFF);
	if (parm.PCount() == 5)
		rAlpha = (parm(1)->ev_long >> 24);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Colors2RGB(ParamBlkEx& parm)
{
	int nRGB;
	nRGB = (parm(1)->ev_long & 0x000000FF);
	nRGB |= ((parm(2)->ev_long << 8) & 0x0000FF00);
	nRGB |= ((parm(3)->ev_long << 16) & 0x00FF0000);
	if (parm.PCount() == 4)
		nRGB |= (parm(4)->ev_long << 24);
	Return(nRGB);
}

void _fastcall GetCursorPosEx(ParamBlkEx& parm)
{
try
{
	bool bRelative = parm.PCount() >= 3 && parm(3)->ev_length;
	HWND hHwnd;
	POINT sPoint;

	if (!GetCursorPos(&sPoint))
	{
		SaveWin32Error("GetCursorPos", GetLastError());
		throw E_APIERROR;
	}

	if (bRelative)
	{
		if (parm.PCount() == 4)
		{
			if (parm(4)->Vartype() == 'I' || parm(4)->Vartype() == 'N')
				hHwnd = parm(4)->DynamicPtr<HWND>();
			else if (parm(4)->Vartype() == 'C')
			{
				FoxString pWindow(parm(4));
				hHwnd = WHwndByTitle(pWindow);
			}
			else
			{
				SaveCustomError("GetCursorPosEx", "Invalid type '%s' for parameter 4", parm(4)->Vartype());
				throw E_INVALIDPARAMS;
			}

			if (!ScreenToClient(hHwnd,&sPoint))
			{
				SaveWin32Error("ScreenToClient", GetLastError());
				throw E_APIERROR;
			}
		}
		else
		{
			if (!ScreenToClient(WTopHwnd(),&sPoint))
			{
				SaveWin32Error("ScreenToClient", GetLastError());
				throw E_APIERROR;
			}
		}
	}

	LocatorEx& p1 = parm(1);
	LocatorEx& p2 = parm(1);
	p1 = sPoint.x;
	p2 = sPoint.y;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Int64_Add(ParamBlkEx& parm)
{
try
{
	__int64 Op1, Op2, Result;
	bool Overflow;
   	Op1 = Value2Int64(parm(1));
	Op2 = Value2Int64(parm(2));

	Result = Op1 + Op2;
#if !defined(_WIN64)
	__asm seto Overflow;
	if (Overflow)
		throw E_NUMERICOVERFLOW;
#endif
	if (parm.PCount() == 2 || parm(3)->ev_long == 1)
		ReturnInt64AsCurrency(Result);
	else if (parm(3)->ev_long == 2)
		ReturnInt64AsString(Result);
	else
		ReturnInt64AsBinary(Result);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Int64_Sub(ParamBlkEx& parm)
{
try
{
	__int64 Op1, Op2, Result;
	bool Overflow;

	Op1 = Value2Int64(parm(1));
	Op2 = Value2Int64(parm(2));

	Result = Op1 - Op2;
#if !defined(_WIN64)
	__asm seto Overflow;
	if (Overflow)
		throw E_NUMERICOVERFLOW;
#endif

	if (parm.PCount() == 2 || parm(3)->ev_long == 1)
		ReturnInt64AsCurrency(Result);
	else if (parm(3)->ev_long == 2)
		ReturnInt64AsString(Result);
	else
		ReturnInt64AsBinary(Result);

}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Int64_Mul(ParamBlkEx& parm)
{
try
{
	__int64 Op1, Op2, Result;
	bool Overflow;
	Op1 = Value2Int64(parm(1));
	Op2 = Value2Int64(parm(2));

#if !defined(_WIN64)
	Result = Multiply_Int64(Op1, Op2, &Overflow);
	if (Overflow)
		throw E_NUMERICOVERFLOW;
#else
	Result = Op1 * Op2;
#endif

	if (parm.PCount() == 2 || parm(3)->ev_long == 1)
		ReturnInt64AsCurrency(Result);
	else if (parm(3)->ev_long == 2)
		ReturnInt64AsString(Result);
	else
		ReturnInt64AsBinary(Result);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Int64_Div(ParamBlkEx& parm)
{
try
{
	__int64 Op1, Op2, Result;
	
	Op1 = Value2Int64(parm(1));
	Op2 = Value2Int64(parm(2));

	if (Op2 == 0)
		throw E_DIVIDEBYZERO;

	if (Op1 == MIN_INT64 && Op2 == -1)
		throw E_NUMERICOVERFLOW;

	Result = Op1 / Op2;
	
	if (parm.PCount() == 2 || parm(3)->ev_long == 1)
		ReturnInt64AsCurrency(Result);
	else if (parm(3)->ev_long == 2)
		ReturnInt64AsString(Result);
	else
		ReturnInt64AsBinary(Result);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Int64_Mod(ParamBlkEx& parm)
{
try
{
	__int64 Op1, Op2, Result;

	Op1 = Value2Int64(parm(1));
	Op2 = Value2Int64(parm(2));

	if (Op2 == 0)
		throw E_DIVIDEBYZERO;

	Result = Op1 % Op2;

	if (parm.PCount() == 2 || parm(3)->ev_long == 1)
		ReturnInt64AsCurrency(Result);
	else if (parm(3)->ev_long == 2)
		ReturnInt64AsString(Result);
	else
		ReturnInt64AsBinary(Result);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Value2Variant(ParamBlkEx& parm)
{
	ValueEx vVariant;
	vVariant.SetString(sizeof(PackedValue));
	PackedValue vData = {0};
	char *pVariant;
	int nSize = 0;

	vData.ev_type = parm(1)->Vartype();
	switch(parm(1)->Vartype())
	{
		case 'I':
			vData.ev_long = parm(1)->ev_long;
			vData.ev_width = (unsigned char)parm(1)->ev_width;
			break;

		case 'N':
			vData.ev_real = parm(1)->ev_real;
			vData.ev_width = (unsigned char)parm(1)->ev_width;
			vData.ev_decimals = (unsigned char)parm(1)->ev_length;
			break;

		case 'C':
			nSize = vData.ev_length = parm(1)->ev_length;
			vVariant.ev_width = parm(1)->ev_width;
			vVariant.ev_length += nSize;
			break;

		case 'L':
			vData.ev_length = parm(1)->ev_length;
			break;

		case 'T':
		case 'D':
			vData.ev_real = parm(1)->ev_real;
			break;

		case 'Y':
			vData.ev_currency.QuadPart = parm(1)->ev_currency.QuadPart;
			break;
		
		case '0':
			break;

		default:
			RaiseError(E_INVALIDPARAMS);
	}

	if (!vVariant.AllocHandle(vVariant.Len()))
		RaiseError(E_INSUFMEMORY);

	pVariant = vVariant.HandleToPtr();

	memcpy(pVariant,&vData,sizeof(PackedValue));
	if (nSize)
	{
		memcpy(pVariant+sizeof(PackedValue), parm(1)->HandleToPtr(), nSize);
	}
	Return(vVariant);
}

void _fastcall Variant2Value(ParamBlkEx& parm)
{
try
{
	FoxValue vVariant;
	FoxMemo vMemo(parm, 1);
	FoxString vString(parm, 1);
	LPPackedValue pData;
	LPPackedValue pDataTmp;

	if (parm(1).IsMemoRef())
	{
		unsigned int nLen = 0;
		pData = reinterpret_cast<LPPackedValue>(vMemo.Read(nLen));
	}
	else if (parm(1)->Vartype() == 'C')
	{
		pData = (LPPackedValue)(char*)vString;
	}
	else
	{
		SaveCustomError("Variant2Value", "Invalid type '%s' for parameter 1.", parm(1)->Vartype());
		throw E_INVALIDPARAMS;
	}

	vVariant->ev_type = pData->ev_type;

	switch(pData->ev_type)
	{
		case 'I':
			vVariant->ev_long = pData->ev_long;
			vVariant->ev_width = pData->ev_width;
			break;

		case 'N':
			vVariant->ev_real = pData->ev_real;
			vVariant->ev_width = pData->ev_width;
			vVariant->ev_length = pData->ev_decimals;
			break;

		case 'C':
			vVariant->ev_length = pData->ev_length; // len of data
			vVariant->ev_width = pData->ev_width;   // binary or character?
			vVariant.AllocHandle(vVariant->ev_length);
			pDataTmp = pData;
			memcpy(vVariant.HandleToPtr(), ++pDataTmp, vVariant->ev_length);
			break;

		case 'L':
			vVariant->ev_length = pData->ev_length;
			break;

		case 'T':
		case 'D':
			vVariant->ev_real = pData->ev_real;
			break;

		case 'Y':
			vVariant->ev_currency.QuadPart = pData->ev_currency.QuadPart;
			break;
		
		case '0':
			break;

		default:
			throw E_INVALIDPARAMS;
	}

	vVariant.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

/* its as simple as that :) */
void _fastcall Decimals(ParamBlkEx& parm)
{
	Return(static_cast<int>(parm(1)->ev_length));
}

void _fastcall Num2Binary(ParamBlkEx& parm)
{
try
{
	if (parm(1)->ev_real < MIN_INT || parm(1)->ev_real > MAX_UINT)
	{
		SaveCustomError("Num2Binary", "Parameter out of range, < MIN_INT or > MAX_UINT");
		throw E_INVALIDPARAMS;
	}
		
	int nNum = parm(1)->ev_long;
	FoxString vBin(32);
	char *pBin;

	vBin.Len(32);
	pBin = vBin;
	pBin += 31;

	for (int xj = 0; xj <= 31; xj++)
	{
		*pBin = (nNum & 1) ? '1' : '0';
		pBin--;
		nNum >>= 1;
	}

	vBin.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall CreatePublicShadowObjReference(ParamBlkEx& parm)
{
try
{
	FoxString pVarname(parm(1));
	LocatorEx sVar;
	// create the public variable and store the object reference into it
	// this will increase the reference count by one
	sVar.NewVar(pVarname, true);
	sVar.Store(parm(2));
	// release the parameter object, this will decrease the reference count again to the previous value
	parm(2)->ObjectRelease();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ReleasePublicShadowObjReference(ParamBlkEx& parm)
{
try
{
	ValueEx vObject;
	vObject = 0;
    FoxString pVarname(parm(1));
	// increase the reference count by evaluating the variable into a temporary
	vObject.Evaluate(pVarname);
	// release the variable, this will decrease the reference count again
	LocatorEx::ReleaseVarStatic(pVarname);
	// don't release the temporary - this is on purpose!
}
catch(int nErrorNo)
{
    RaiseError(nErrorNo);
}
} 

void _fastcall GetLocaleInfoExLib(ParamBlkEx& parm)
{
try
{
	int nApiRet;
	LCTYPE nType = parm(1)->ev_long;
	LCID nLocale = parm.PCount() >= 2 ? parm(2)->ev_long : LOCALE_USER_DEFAULT;

	if (nType & LOCALE_RETURN_NUMBER)
	{
		int nLocaleInfo;
		nApiRet = GetLocaleInfo(nLocale, nType, reinterpret_cast<LPSTR>(&nLocaleInfo), sizeof(nLocaleInfo));
		if (nApiRet == 0)
		{
			SaveWin32Error("GetLocaleInfo", GetLastError());
			throw E_APIERROR;
		}
		Return(nLocaleInfo);
	}
	else
	{
		FoxString pLocaleInfo(256);
		nApiRet = GetLocaleInfo(nLocale, nType, pLocaleInfo, pLocaleInfo.Size());
		if (nApiRet == 0)
		{
			SaveWin32Error("GetLocaleInfo", GetLastError());
			throw E_APIERROR;
		}
		pLocaleInfo.Len(nApiRet-1);
		pLocaleInfo.Return();
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall OsEx(ParamBlkEx& parm)
{
	Return(static_cast<int>(COs::GetVersion()));
}

void _fastcall Str2Short(ParamBlkEx& parm)
{
	short *pShort;
	pShort = reinterpret_cast<short*>(parm(1)->HandleToPtr());
	Return(*pShort);
}

void _fastcall Short2Str(ParamBlkEx& parm)
{
	ValueEx vRetVal;
	vRetVal.SetString(sizeof(short));
	short *pRetVal;
	
	if (!vRetVal.AllocHandle(sizeof(short)))
		RaiseError(E_INSUFMEMORY);

	pRetVal = reinterpret_cast<short*>(vRetVal.HandleToPtr());
	*pRetVal = static_cast<short>(parm(1)->ev_long);
	Return(vRetVal);
}

void _fastcall Str2UShort(ParamBlkEx& parm)
{
	unsigned short *pUShort;
	pUShort = reinterpret_cast<unsigned short*>(parm(1)->HandleToPtr());
	Return(*pUShort);
}

void _fastcall UShort2Str(ParamBlkEx& parm)
{
	ValueEx vRetVal;
	vRetVal.SetString(sizeof(unsigned short));
	unsigned short *pRetVal;
	
	if (!vRetVal.AllocHandle(sizeof(unsigned short)))
		RaiseError(E_INSUFMEMORY);

	pRetVal = reinterpret_cast<unsigned short*>(vRetVal.HandleToPtr());
	*pRetVal = static_cast<unsigned short>(parm(1)->ev_long);

	Return(vRetVal);
}

void _fastcall Str2Long(ParamBlkEx& parm)
{
	long *pLong;
	pLong = reinterpret_cast<long*>(parm(1)->HandleToPtr());
	Return(*pLong);
}

void _fastcall Long2Str(ParamBlkEx& parm)
{
	ValueEx vRetVal;
	vRetVal.SetString(sizeof(long));
	long *pRetVal;
	
	if (!vRetVal.AllocHandle(sizeof(long)))
		RaiseError(E_INSUFMEMORY);

	pRetVal = reinterpret_cast<long*>(vRetVal.HandleToPtr());
	*pRetVal = parm(1)->ev_long;
	Return(vRetVal);
}

void _fastcall Str2ULong(ParamBlkEx& parm)
{
	unsigned long *pULong;
	pULong = reinterpret_cast<unsigned long*>(parm(1)->HandleToPtr());
	Return(*pULong);
}

void _fastcall ULong2Str(ParamBlkEx& parm)
{
	ValueEx vRetVal;
	unsigned long* pRetVal;
	vRetVal.SetString(sizeof(unsigned long));
	if (!vRetVal.AllocHandle(sizeof(unsigned long)))
		RaiseError(E_INSUFMEMORY);
	
	pRetVal = reinterpret_cast<unsigned long*>(vRetVal.HandleToPtr());
	
	if (parm(1)->Vartype() == 'I')
		*pRetVal = parm(1)->ev_long;
	else if (parm(1)->Vartype() == 'N')
		*pRetVal = static_cast<unsigned long>(parm(1)->ev_real);
	else
	{
		vRetVal.FreeHandle();
		RaiseError(E_INVALIDPARAMS);
	}

	Return(vRetVal);
}

void _fastcall Str2Double(ParamBlkEx& parm)
{
	double *pDouble;
	pDouble = reinterpret_cast<double*>(parm(1)->HandleToPtr());
	Return(*pDouble);
}

void _fastcall Double2Str(ParamBlkEx& parm)
{
	ValueEx vRetVal;
	vRetVal.SetString(sizeof(double));
	double *pRetVal;
	
	if (!vRetVal.AllocHandle(sizeof(double)))
		RaiseError(E_INSUFMEMORY);

	pRetVal = reinterpret_cast<double*>(vRetVal.HandleToPtr());
	*pRetVal = parm(1)->ev_real;
	Return(vRetVal);	
}

void _fastcall Str2Float(ParamBlkEx& parm)
{
	float *pFloat;
	pFloat = reinterpret_cast<float*>(parm(1)->HandleToPtr());
	Return(*pFloat);
}

void _fastcall Float2Str(ParamBlkEx& parm)
{
	ValueEx vRetVal;
	vRetVal.SetString(sizeof(float));
	float *pRetVal;
	
	if (!vRetVal.AllocHandle(sizeof(float)))
		RaiseError(E_INSUFMEMORY);

	pRetVal = reinterpret_cast<float*>(vRetVal.HandleToPtr());
	*pRetVal = static_cast<float>(parm(1)->ev_real);
	Return(vRetVal);	
}

void _fastcall Int642Str(ParamBlkEx& parm)
{
try
{
	__int64 nInt64;
	nInt64 = Value2Int64(parm(1));

	if (parm.PCount() == 1 || parm(2)->ev_long == 1)
		ReturnInt64AsBinary(nInt64);
	else
		ReturnInt64AsString(nInt64);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Str2Int64(ParamBlkEx& parm)
{
	__int64 nInt;
	if (parm(1)->Len() != 8)
		RaiseError(E_INVALIDPARAMS);
	nInt = *reinterpret_cast<__int64*>(parm(1)->HandleToPtr());

	if (parm.PCount() == 1 || parm(2)->ev_long == 1)
		ReturnInt64AsCurrency(nInt);
	else if (parm(2)->ev_long == 2)
		ReturnInt64AsString(nInt);
	else if (parm(2)->ev_long == 3)
		ReturnInt64AsDouble(nInt);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall UInt642Str(ParamBlkEx& parm)
{
try
{
	unsigned __int64 nUInt64;
	nUInt64 = Value2Int64(parm(1));

	if (parm.PCount() == 1 || parm(2)->ev_long == 1)
		ReturnInt64AsBinary(nUInt64);
	else
		ReturnInt64AsString(nUInt64);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Str2UInt64(ParamBlkEx& parm)
{
	unsigned __int64 nInt;
	if (parm(1)->Len() != 8)
		RaiseError(E_INVALIDPARAMS);
	nInt = *reinterpret_cast<unsigned __int64*>(parm(1)->HandleToPtr());

	if (parm.PCount() == 1 || parm(2)->ev_long == 1)
		ReturnInt64AsCurrency(nInt);
	else if (parm(2)->ev_long == 2)
		ReturnInt64AsString(nInt);
	else if (parm(2)->ev_long == 3)
		ReturnInt64AsDouble(nInt);
	else
		RaiseError(E_INVALIDPARAMS);
}