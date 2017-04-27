#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2cconv.h"
#include "vfp2cutil.h"
#include "vfp2chelpers.h"
#include "vfp2ccppapi.h"
#include "vfp2casmfuncs.h"
#include "vfpmacros.h"

void _fastcall PG_ByteA2Str(ParamBlk *parm)
{
try
{
	FoxString vInput(parm, 1, 0);
	FoxMemo vMemo(parm, 1);
	FoxString vRetVal;
	unsigned char *pInput, *pInputEnd, *pRetVal, *pRetValStart;
	unsigned int nMemoLen = 0;

	if (Vartype(p1) == 'C')
	{
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
	else if (IsMemoRef(r1))
	{
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
		throw E_INVALIDPARAMS;

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

void _fastcall PG_Str2ByteA(ParamBlk *parm)
{
try
{
	FoxString vInput(parm, 1, 0);
	FoxMemo vMemo(parm, 1);
	FoxString vRetVal;
	unsigned char *pInput, *pInputEnd, *pRetVal, *pRetValStart;
	unsigned int nMemoLen = 0;
	bool bDouble = PCount() == 2 ? p2.ev_length > 0 : false;
	int nMemMulti = bDouble == false ? 4 : 5;

	if (Vartype(p1) == 'C')
	{
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
	else if (IsMemoRef(r1))
	{
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
		throw E_INVALIDPARAMS;

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

void _fastcall RGB2Colors(ParamBlk *parm)
{
try
{
	FoxReference rRed(r2);
	FoxReference rGreen(r3);
	FoxReference rBlue(r4);
	FoxReference rAlpha(r5);

	rRed = (p1.ev_long & 0xFF);
	rGreen = (p1.ev_long >> 8 & 0xFF);
	rBlue = (p1.ev_long >> 16 & 0xFF);
	if (PCount() == 5)
		rAlpha = (p1.ev_long >> 24);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Colors2RGB(ParamBlk *parm)
{
	int nRGB;
	nRGB = (p1.ev_long & 0x000000FF);
	nRGB |= ((p2.ev_long << 8) & 0x0000FF00);
	nRGB |= ((p3.ev_long << 16) & 0x00FF0000);
	if (PCount() == 4)
		nRGB |= (p4.ev_long << 24);
	Return(nRGB);
}

void _fastcall GetCursorPosEx(ParamBlk *parm)
{
try
{
	FoxReference rX(r1);
	FoxReference rY(r2);
	bool bRelative = PCount() >= 3 && p3.ev_length;
	FoxString pWindow(parm,4);

	HWND hHwnd;
	POINT sPoint;

	if (!GetCursorPos(&sPoint))
	{
		SaveWin32Error("GetCursorPos", GetLastError());
		throw E_APIERROR;
	}

	if (bRelative)
	{
		if (PCount() == 4)
		{
			if (Vartype(p4) == 'I' || Vartype(p4) == 'N')
				hHwnd = Vartype(p4) == 'I' ? reinterpret_cast<HWND>(p4.ev_long) : reinterpret_cast<HWND>(static_cast<int>(p4.ev_real));
			else if (pWindow.Len())
				hHwnd = WHwndByTitle(pWindow);
			else
				throw E_INVALIDPARAMS;

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

	rX = sPoint.x;
	rY = sPoint.y;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Int64_Add(ParamBlk *parm)
{
try
{
	__int64 Op1, Op2, Result;
	bool Overflow;
   	Op1 = Value2Int64(p1);
	Op2 = Value2Int64(p2);

	Result = Op1 + Op2;
	__asm seto Overflow;
	if (Overflow)
		throw E_NUMERICOVERFLOW;

	if (PCount() == 2 || p3.ev_long == 1)
		ReturnInt64AsCurrency(Result);
	else if (p3.ev_long == 2)
		ReturnInt64AsString(Result);
	else
		ReturnInt64AsBinary(Result);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Int64_Sub(ParamBlk *parm)
{
try
{
	__int64 Op1, Op2, Result;
	bool Overflow;

	Op1 = Value2Int64(p1);
	Op2 = Value2Int64(p2);

	Result = Op1 - Op2;
	__asm seto Overflow;
	if (Overflow)
		throw E_NUMERICOVERFLOW;

	if (PCount() == 2 || p3.ev_long == 1)
		ReturnInt64AsCurrency(Result);
	else if (p3.ev_long == 2)
		ReturnInt64AsString(Result);
	else
		ReturnInt64AsBinary(Result);

}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Int64_Mul(ParamBlk *parm)
{
try
{
	__int64 Op1, Op2, Result;
	bool Overflow;
	Op1 = Value2Int64(p1);
	Op2 = Value2Int64(p2);

	Result = Multiply_Int64(Op1, Op2, &Overflow);
	if (Overflow)
		throw E_NUMERICOVERFLOW;

	if (PCount() == 2 || p3.ev_long == 1)
		ReturnInt64AsCurrency(Result);
	else if (p3.ev_long == 2)
		ReturnInt64AsString(Result);
	else
		ReturnInt64AsBinary(Result);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Int64_Div(ParamBlk *parm)
{
try
{
	__int64 Op1, Op2, Result;
	
	Op1 = Value2Int64(p1);
	Op2 = Value2Int64(p2);

	if (Op2 == 0)
		throw E_DIVIDEBYZERO;

	if (Op1 == MIN_INT64 && Op2 == -1)
		throw E_NUMERICOVERFLOW;

	Result = Op1 / Op2;
	
	if (PCount() == 2 || p3.ev_long == 1)
		ReturnInt64AsCurrency(Result);
	else if (p3.ev_long == 2)
		ReturnInt64AsString(Result);
	else
		ReturnInt64AsBinary(Result);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Int64_Mod(ParamBlk *parm)
{
try
{
	__int64 Op1, Op2, Result;

	Op1 = Value2Int64(p1);
	Op2 = Value2Int64(p2);

	if (Op2 == 0)
		throw E_DIVIDEBYZERO;

	Result = Op1 % Op2;

	if (PCount() == 2 || p3.ev_long == 1)
		ReturnInt64AsCurrency(Result);
	else if (p3.ev_long == 2)
		ReturnInt64AsString(Result);
	else
		ReturnInt64AsBinary(Result);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Value2Variant(ParamBlk *parm)
{
	StringValue vVariant(sizeof(VALUEEX));
	VALUEEX vData = {0};
	char *pVariant;
	int nSize = 0;

	vData.ev_type = p1.ev_type;

	switch(p1.ev_type)
	{
		case 'I':
			vData.ev_long = p1.ev_long;
			vData.ev_width = (unsigned char)p1.ev_width;
			break;

		case 'N':
			vData.ev_real = p1.ev_real;
			vData.ev_width = (unsigned char)p1.ev_width;
			vData.ev_decimals = (unsigned char)p1.ev_length;
			break;

		case 'C':
			vVariant.ev_length += nSize = vData.ev_length = p1.ev_length;
			break;

		case 'L':
			vData.ev_length = p1.ev_length;
			break;

		case 'T':
		case 'D':
			vData.ev_real = p1.ev_real;
			break;

		case 'Y':
			vData.ev_currency.QuadPart = p1.ev_currency.QuadPart;
			break;
		
		case '0':
			break;

		default:
			RaiseError(E_INVALIDPARAMS);
	}

	if (!AllocHandleEx(vVariant, Len(vVariant)))
		RaiseError(E_INSUFMEMORY);

	pVariant = HandleToPtr(vVariant);

	memcpy(pVariant,&vData,sizeof(VALUEEX));
	if (nSize)
	{
		memcpy(pVariant+sizeof(VALUEEX),HandleToPtr(p1),nSize);
	}

	Return(vVariant);
}

void _fastcall Variant2Value(ParamBlk *parm)
{
try
{
	FoxValue vVariant;
	FoxMemo vMemo(parm, 1);
	FoxString vString(parm, 1);
	LPVALUEEX pData;
	LPVALUEEX pDataTmp;

	if (IsMemoRef(r1))
	{
		unsigned int nLen = 0;
		pData = reinterpret_cast<LPVALUEEX>(vMemo.Read(nLen));
	}
	else if (Vartype(p1) == 'C')
	{
		pData = (LPVALUEEX)(char*)vString;
	}
	else
		throw E_INVALIDPARAMS;

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
void _fastcall Decimals(ParamBlk *parm)
{
	Return(static_cast<int>(p1.ev_length));
}

void _fastcall Num2Binary(ParamBlk *parm)
{
try
{
	if (p1.ev_real < MIN_INT || p1.ev_real > MAX_UINT)
		throw E_INVALIDPARAMS;

	int nNum = p1.ev_long;
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

void _fastcall CreatePublicShadowObjReference(ParamBlk *parm)
{
try
{
	FoxString pVarname(p1);
	Locator sVar;
	NewVar(pVarname,sVar,true);
	Store(sVar,p2);
	ObjectRelease(p2);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ReleasePublicShadowObjReference(ParamBlk *parm)
{
try
{
	Value vObject = {'0'};
    FoxString pVarname(p1);
    Evaluate(vObject, pVarname);
    ReleaseVar(pVarname);
}
catch(int nErrorNo)
{
    RaiseError(nErrorNo);
}
} 

void _fastcall GetLocaleInfoExLib(ParamBlk *parm)
{
try
{
	int nApiRet;
	LCTYPE nType = p1.ev_long;
	LCID nLocale = PCount() >= 2 ? p2.ev_long : LOCALE_USER_DEFAULT;

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

void _fastcall OsEx(ParamBlk *parm)
{
	Return(static_cast<int>(COs::GetVersion()));
}

void _fastcall Str2Short(ParamBlk *parm)
{
	short *pShort;
	pShort = reinterpret_cast<short*>(HandleToPtr(p1));
	Return(*pShort);
}

void _fastcall Short2Str(ParamBlk *parm)
{
	StringValue vRetVal(sizeof(short));
	short *pRetVal;
	
	if (!AllocHandleEx(vRetVal,sizeof(short)))
		RaiseError(E_INSUFMEMORY);

	pRetVal = reinterpret_cast<short*>(HandleToPtr(vRetVal));
	*pRetVal = static_cast<short>(p1.ev_long);

	Return(vRetVal);
}

void _fastcall Str2UShort(ParamBlk *parm)
{
	unsigned short *pUShort;
	pUShort = reinterpret_cast<unsigned short*>(HandleToPtr(p1));
	Return(*pUShort);
}

void _fastcall UShort2Str(ParamBlk *parm)
{
	StringValue vRetVal(sizeof(unsigned short));
	unsigned short *pRetVal;
	
	if (!AllocHandleEx(vRetVal,sizeof(unsigned short)))
		RaiseError(E_INSUFMEMORY);

	pRetVal = reinterpret_cast<unsigned short*>(HandleToPtr(vRetVal));
	*pRetVal = static_cast<unsigned short>(p1.ev_long);

	Return(vRetVal);
}

void _fastcall Str2Long(ParamBlk *parm)
{
	long *pLong;
	pLong = reinterpret_cast<long*>(HandleToPtr(p1));
	Return(*pLong);
}

void _fastcall Long2Str(ParamBlk *parm)
{
	StringValue vRetVal(sizeof(long));
	long *pRetVal;
	
	if (!AllocHandleEx(vRetVal,sizeof(long)))
		RaiseError(E_INSUFMEMORY);

	pRetVal = reinterpret_cast<long*>(HandleToPtr(vRetVal));
	*pRetVal = p1.ev_long;

	Return(vRetVal);
}

void _fastcall Str2ULong(ParamBlk *parm)
{
	unsigned long *pULong;
	pULong = reinterpret_cast<unsigned long*>(HandleToPtr(p1));
	Return(*pULong);
}

void _fastcall ULong2Str(ParamBlk *parm)
{
	StringValue vRetVal(sizeof(unsigned long));
	unsigned long *pRetVal;
	
	if (!AllocHandleEx(vRetVal,sizeof(unsigned long)))
		RaiseError(E_INSUFMEMORY);

	pRetVal = reinterpret_cast<unsigned long*>(HandleToPtr(vRetVal));
	
	if (Vartype(p1) == 'I')
		*pRetVal = p1.ev_long;
	else if (Vartype(p1) == 'N')
		*pRetVal = static_cast<unsigned long>(p1.ev_real);
	else
	{
		FreeHandle(vRetVal);
		RaiseError(E_INVALIDPARAMS);
	}

	Return(vRetVal);
}

void _fastcall Str2Double(ParamBlk *parm)
{
	double *pDouble;
	pDouble = reinterpret_cast<double*>(HandleToPtr(p1));
	Return(*pDouble);
}

void _fastcall Double2Str(ParamBlk *parm)
{
	StringValue vRetVal(sizeof(double));
	double *pRetVal;
	
	if (!AllocHandleEx(vRetVal,sizeof(double)))
		RaiseError(E_INSUFMEMORY);

	pRetVal = reinterpret_cast<double*>(HandleToPtr(vRetVal));
	*pRetVal = p1.ev_real;

	Return(vRetVal);	
}

void _fastcall Str2Float(ParamBlk *parm)
{
	float *pFloat;
	pFloat = reinterpret_cast<float*>(HandleToPtr(p1));
	Return(*pFloat);
}

void _fastcall Float2Str(ParamBlk *parm)
{
	StringValue vRetVal(sizeof(float));
	float *pRetVal;
	
	if (!AllocHandleEx(vRetVal,sizeof(float)))
		RaiseError(E_INSUFMEMORY);

	pRetVal = reinterpret_cast<float*>(HandleToPtr(vRetVal));
	*pRetVal = static_cast<float>(p1.ev_real);

	Return(vRetVal);	
}

void _fastcall Int642Str(ParamBlk *parm)
{
try
{
	__int64 nInt64;
	nInt64 = Value2Int64(p1);

	if (PCount() == 1 || p2.ev_long == 1)
		ReturnInt64AsBinary(nInt64);
	else
		ReturnInt64AsString(nInt64);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Str2Int64(ParamBlk *parm)
{
	__int64 nInt;
	if (Len(p1) != 8)
		RaiseError(E_INVALIDPARAMS);
	nInt = *reinterpret_cast<__int64*>(HandleToPtr(p1));

	if (PCount() == 1 || p2.ev_long == 1)
		ReturnInt64AsCurrency(nInt);
	else if (p2.ev_long == 2)
		ReturnInt64AsString(nInt);
	else if (p2.ev_long == 3)
		ReturnInt64AsDouble(nInt);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall UInt642Str(ParamBlk *parm)
{
try
{
	unsigned __int64 nUInt64;
	nUInt64 = Value2Int64(p1);

	if (PCount() == 1 || p2.ev_long == 1)
		ReturnInt64AsBinary(nUInt64);
	else
		ReturnInt64AsString(nUInt64);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall Str2UInt64(ParamBlk *parm)
{
	unsigned __int64 nInt;
	if (Len(p1) != 8)
		RaiseError(E_INVALIDPARAMS);
	nInt = *reinterpret_cast<unsigned __int64*>(HandleToPtr(p1));

	if (PCount() == 1 || p2.ev_long == 1)
		ReturnInt64AsCurrency(nInt);
	else if (p2.ev_long == 2)
		ReturnInt64AsString(nInt);
	else if (p2.ev_long == 3)
		ReturnInt64AsDouble(nInt);
	else
		RaiseError(E_INVALIDPARAMS);
}

