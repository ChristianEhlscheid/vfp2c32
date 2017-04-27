#include "pro_ext.h"
#include "vfpmacros.h"
#include "vfp2ccppapi.h"
#include "vfp2ccomcall.h"
#include "vfp2cutil.h"

ComCall::ComCall()
{
	m_Parameters.cArgs = 0;
	m_Parameters.cNamedArgs = 0;
	m_Parameters.rgdispidNamedArgs = 0;
	m_Parameters.rgvarg = 0;
	m_Method = 0;
	m_ResultMethod = 0;
	m_ResultObj = 0;
}

ComCall::~ComCall()
{
	if (m_Parameters.rgvarg)
	{
		VARIANTARG *pArg = m_Parameters.rgvarg;
		for (unsigned int xj = 0; xj < m_Parameters.cArgs; xj++)
		{
			ReleaseVarientEx(pArg);
			pArg++;
		}
		delete[] m_Parameters.rgvarg;
	}
	if (m_Method)
		delete[] m_Method;
	if (m_ResultMethod)
		delete[] m_ResultMethod;
	if (m_ResultObj)
		m_ResultObj->Release();
}

void ComCall::SetCallInfo(LPOLESTR pClass, LPOLESTR pMethod, DWORD dwContext, LCID nLocale)
{
	m_Async = true;
	m_Method = pMethod;
	m_Context = dwContext;
	m_Locale = nLocale;
	HRESULT hr = CLSIDFromProgID(pClass,&m_ClsId);
	if (FAILED(hr))
	{
		SaveWin32Error("CLSIDFromProgID", hr);
		throw E_APIERROR;
	}
}

void ComCall::SetCallInfo(IDispatch *pDisp, LPOLESTR pMethod, LCID nLocale)
{
	m_Async = false;
	m_Disp = pDisp;
	m_Method = pMethod;
	m_Locale = nLocale;
}

void ComCall::SetResultInfo(IUnknown *pResObj, LPOLESTR pMethod)
{
	m_ResultMethod = pMethod;
	HRESULT hr = CoMarshalInterThreadInterfaceInStream(IID_IDispatch,pResObj,&m_ResultObj);
	if (FAILED(hr))
	{
		SaveWin32Error("CoMarshalInterThreadInterfaceInStream", hr);
		throw E_APIERROR;
	}
}

void ComCall::SetParameterCount(unsigned int nParmCount)
{
	m_Parameters.cArgs = nParmCount;
	if (nParmCount > 0)
	{
		m_Parameters.rgvarg = new VARIANTARG[nParmCount];
		if (!m_Parameters.rgvarg)
			throw E_INSUFMEMORY;

		VARIANTARG *pArg = m_Parameters.rgvarg;
		for (unsigned int xj = 0; xj < nParmCount; xj++)
		{
			VariantInit(pArg);
			pArg++;
		}
	}
}

void ComCall::MarshalParameter(unsigned int nParmNo, Parameter &pParm, VARTYPE vType, int nBase)
{
	VARIANTARG* pArg = m_Parameters.rgvarg;
	pArg += (m_Parameters.cArgs - nParmNo);
	bool bAutoType;

	if (Vartype(pParm) == 'R')
	{
		bAutoType = ((vType & VT_TYPEMASK) == VT_EMPTY);

		FoxValue sTmp(pParm.loc);
		vType |= VT_BYREF;

		if (bAutoType)
			MarshalVariant(*pArg, sTmp, vType);
		else
			MarshalVariantEx(*pArg, sTmp, vType, nBase);
	}
	else
	{
		bAutoType = (vType & VT_TYPEMASK) == VT_EMPTY && (vType & VT_ARRAY) == 0;

		if (bAutoType)
			MarshalVariant(*pArg, pParm.val, vType);
		else
			MarshalVariantEx(*pArg, pParm.val, vType, nBase);
	}
}

void ComCall::UnMarshalIDispatchParameters()
{
	HRESULT hr;
	LPSTREAM pStream;
	VARIANTARG* pArg = m_Parameters.rgvarg;
	for (unsigned int xj = 1; xj <= m_Parameters.cArgs; xj++)
	{
		if (pArg->vt == VT_STREAM)
		{
			pStream = (LPSTREAM)pArg->byref;
			hr = CoGetInterfaceAndReleaseStream(pStream,IID_IDispatch,(LPVOID*)&pArg->pdispVal);
			if (FAILED(hr))
				throw hr;
			pArg->vt = VT_DISPATCH;
		}
		pArg++;
	}
}

void ComCall::UnMarshalRefParameters(ParamBlk *parm)
{
	VARIANTARG* pArg = m_Parameters.rgvarg;
	for (unsigned int xj = m_Parameters.cArgs; xj > 0; xj--)
	{
		if (pArg->vt & VT_BYREF)
		{
			FoxValue pTmp;
			FoxReference pRef(parm->p[xj+VFP2C_COMCALL_PARMOFFSET].loc);
			UnMarshalVariant(*pArg, pTmp);
			pRef = pTmp;
		}
		pArg++;
	}
}

void ComCall::CallMethodAsync()
{
	IDispatch *pDisp = 0;
	IDispatch *pRetObject = 0;
	ComException sException;
	HRESULT hr;
	DISPID nID;
	VARIANT vReturnValue, vResult;
	VariantInit(&vReturnValue);
	VariantInit(&vResult);
	DISPPARAMS sParams;
	UINT nArgError = 0;
try
{
	UnMarshalIDispatchParameters();

	hr = CoGetInterfaceAndReleaseStream(m_ResultObj,IID_IDispatch,(LPVOID*)&pRetObject);
	if (FAILED(hr))
		throw 0;
	m_ResultObj = 0;

	hr = CoCreateInstance(m_ClsId,0,m_Context,IID_IDispatch,(LPVOID*)&pDisp);
	if (FAILED(hr))
	{
		sException.bstrSource = SysAllocString(L"CoCreateInstance");
		sException.bstrDescription = SysAllocString(L"Function failed!");
		sException.scode = hr;
		throw E_APIERROR;
	}

	hr = pDisp->GetIDsOfNames(IID_NULL,&m_Method,1,m_Locale,&nID);
	if (FAILED(hr))
	{
		sException.bstrSource = SysAllocString(L"IDispatch.GetIDsOfNames");
		sException.bstrDescription = SysAllocString(L"Function failed!");
		sException.scode = hr;
		throw E_APIERROR;
	}

	hr = pDisp->Invoke(nID, IID_NULL, m_Locale, DISPATCH_METHOD, &m_Parameters, &vReturnValue, &sException, &nArgError);

	if (FAILED(hr))
		throw E_APIERROR;

	pDisp->Release();
	pDisp = 0;

	if (m_ResultMethod)
	{
		hr = pRetObject->GetIDsOfNames(IID_NULL,&m_ResultMethod,1,m_Locale,&nID);
		if (FAILED(hr))
		{
			sException.bstrSource = SysAllocString(L"IDispatch.GetIDsOfNames");
			sException.bstrDescription = SysAllocString(L"Function failed!");
			sException.scode = hr;
			throw E_APIERROR;
		}

		sParams.cArgs = 1;
		sParams.cNamedArgs = 0;
		sParams.rgdispidNamedArgs = 0;
		sParams.rgvarg = &vReturnValue;

		hr = pRetObject->Invoke(nID,IID_NULL,m_Locale,DISPATCH_METHOD,
								&sParams,&vResult,0,0);
	}

	VariantClear(&vResult);
	VariantClear(&vReturnValue);
}
catch(int nErrorNo)
{
	if (pDisp)
		pDisp->Release();
	if (nErrorNo == E_APIERROR)
	{
		LPOLESTR pMethod = L"OnError";
		hr = pRetObject->GetIDsOfNames(IID_NULL,&pMethod,1,m_Locale,&nID);
		if (SUCCEEDED(hr))
		{
			VARIANTARG sVarArray[4];
			sParams.cArgs = 4;
			sParams.cNamedArgs = 0;
			sParams.rgdispidNamedArgs = 0;
			sParams.rgvarg = sVarArray;

			if (sException.pfnDeferredFillIn)
				sException.pfnDeferredFillIn(&sException);

			sVarArray[0].vt = VT_INT;
			sVarArray[0].intVal = nArgError;
			sVarArray[1].vt = VT_INT;
			sVarArray[1].intVal = sException.wCode ? sException.wCode : sException.scode;
			sVarArray[2].vt = VT_BSTR;
			sVarArray[2].bstrVal = sException.bstrDescription;			
			sVarArray[3].vt = VT_BSTR;
			sVarArray[3].bstrVal = sException.bstrSource;
		
			pRetObject->Invoke(nID,IID_NULL,m_Locale,DISPATCH_METHOD,&sParams,&vResult,0,0);
		}
	}
	VariantClear(&vReturnValue);
	VariantClear(&vResult);
}
}

void ComCall::CallMethod(Value &vRetVal, ParamBlk *parm)
{
	HRESULT hr;
	ComException sException;
	DISPID nID;
	UINT nArgError = 0;
	VARIANT vResult;
	VariantInit(&vResult);

	hr = m_Disp->GetIDsOfNames(IID_NULL,&m_Method,1,m_Locale,&nID);
	if (FAILED(hr))
	{
		SaveWin32Error("IDispatch.GetIDsOfNames",hr);
		throw E_APIERROR;
	}

	hr = m_Disp->Invoke(nID,IID_NULL,m_Locale,DISPATCH_METHOD,
					&m_Parameters,&vResult,&sException,&nArgError);

	if (FAILED(hr))
	{
		FoxString pSource(sException.bstrSource);
		FoxString pDesc(sException.bstrDescription);
		int nError = sException.wCode ? sException.wCode : sException.scode;
		SaveCustomErrorEx(pSource, pDesc, nError);
		throw E_APIERROR;
	}

	UnMarshalRefParameters(parm);
	UnMarshalVariant(vResult,vRetVal);
	VariantClear(&vResult);
}

void _stdcall MarshalString(VARIANT &pArg, Value &pValue)
{
	FoxString pString;
	try
	{
		pString.Attach(pValue);
		if (!pString.Binary())
		{
			pArg.bstrVal = pString.ToBSTR();
			pArg.vt = VT_BSTR;
		}
		else
		{
			pArg.parray = pString.ToU1SafeArray();
			pArg.vt = VT_ARRAY | VT_UI1;
		}
	}
	catch(int nErrorNo)
	{
		pString.Detach();
		throw nErrorNo;
	}
	pString.Detach();
}

void _stdcall MarshalDate(VARIANT &pArg, Value &pValue)
{
	if (pValue.ev_real == 0)
		pArg.vt = VT_EMPTY;
	else
	{
		pArg.vt = VT_DATE;
		pArg.date = pValue.ev_real - VFPOLETIMEBASE; 
	}
}

void _stdcall MarshalIDispatch(VARIANT &pArg, Value &pValue)
{
	IDispatch *pDisp;
	Value vDispatch = {'0'};
	FoxString pObject;
	pObject.Attach(pValue,1);
	if (pObject.Len() > VFP_MAX_VARIABLE_NAME)
		throw E_INVALIDPARAMS;
	char aCommand[256];
	sprintfex(aCommand,"INT(SYS(3095,%S))",(char*)pObject);
	Evaluate(vDispatch,aCommand);
	pDisp = reinterpret_cast<IDispatch*>(vDispatch.ev_long);
	pDisp->AddRef();
	pArg.vt = VT_DISPATCH;
	pArg.pdispVal = pDisp;
}

void _stdcall MarshalIDispatchCrossThread(VARIANT &pArg, Value &pValue)
{
	HRESULT hr;
	IUnknown *pUnk;
	LPSTREAM pStream = 0;
	Value vUnknown = {'0'};
	FoxString pObject;
	pObject.Attach(pValue,1);
	char aCommand[256];

	sprintfex(aCommand,"INT(SYS(3095,%S))",(char*)pObject);
	Evaluate(vUnknown,aCommand);
	pUnk = reinterpret_cast<IUnknown*>(vUnknown.ev_long);

	hr = CoMarshalInterThreadInterfaceInStream(IID_IDispatch,pUnk,&pStream);
	if (FAILED(hr))
	{
		SaveWin32Error("CoMarshalInterThreadInterfaceInStream", hr);
		throw E_APIERROR;
	}
	pArg.vt = VT_STREAM;
	pArg.byref = (PVOID)pStream;
}

void _stdcall MarshalVariant(VARIANT &pArg, Value &pValue, VARTYPE vType)
{
	switch(pValue.ev_type)
	{
		case 'I':
			pArg.vt = VT_INT;
			pArg.intVal = pValue.ev_long;
			break;

		case 'N':
			pArg.vt = VT_R8;
			pArg.dblVal = pValue.ev_real;
			break;

		case 'D':
		case 'T':
			MarshalDate(pArg,pValue);
			break;

		case 'C':
			MarshalString(pArg,pValue);
			break;

		case 'L':
			pArg.vt = VT_BOOL;
			pArg.boolVal = pValue.ev_length ? VARIANT_TRUE : VARIANT_FALSE;
			break;

		case 'Y':
			MarshalDecimal(pArg,pValue);
			break;

		case '0':
			pArg.vt = VT_NULL;
			break;

		default:
			throw E_INVALIDPARAMS;
	}
}

void _stdcall MarshalVariantEx(VARIANT &pArg, Value &pValue, VARTYPE vType, int nBase)
{
	bool bArray = (vType & VT_ARRAY) > 0;
	bool bReference = (vType & VT_BYREF) > 0;
	VARTYPE vTypeX = (vType & VT_TYPEMASK);

	if (bArray)
	{
		switch(vTypeX)
		{
			case VT_VARIANT:
				MarshalSafeArrayVariant(pArg, pValue, vType, nBase);
				break;
			case VT_INT:
			case VT_I4:
				MarshalSafeArrayInt(pArg, pValue, vType, nBase);
				break;
			case VT_R4:
				MarshalSafeArraySingle(pArg, pValue, vType, nBase);
				break;
			case VT_R8:
				MarshalSafeArrayDouble(pArg, pValue, vType, nBase);
				break;
			case VT_BOOL:
				MarshalSafeArrayBool(pArg, pValue, vType, nBase);
				break;
			case VT_DECIMAL:
				MarshalSafeArrayDecimal(pArg, pValue, vType, nBase);
				break;
			case VT_DATE:
				MarshalSafeArrayDate(pArg, pValue, vType, nBase);
				break;
			case VT_BSTR:
				MarshalSafeArrayBSTR(pArg, pValue, vType, nBase);
				break;
			case VT_UI1:
				MarshalSafeArrayUI1(pArg, pValue, vType, nBase);
				break;
			case VT_EMPTY:
				MarshalSafeArrayEmpty(pArg, vType, nBase);
				break;
			default:
				SaveCustomError("IDispatch_(Async)Invoke", "Invalid parameter type: %I", vType);
				throw E_APIERROR;
		}
	}
	/*	else if (bReference)
	{
		SaveCustomError("AsyncInvoke","Cannot marshal reference parameters in an asyncronous call!");
		throw E_APIERROR;
	}*/
	else
	{
		switch(vTypeX)
		{
			case VT_EMPTY:
				pArg.vt = VT_EMPTY;
				break;
			case VT_NULL:
				pArg.vt = VT_NULL;
				break;
			case VT_I2:
				pArg.vt = VT_I2;
				if (Vartype(pValue) == 'N')
					pArg.iVal = (short)pValue.ev_real;
				else if (Vartype(pValue) == 'I')
					pArg.iVal = (short)pValue.ev_long;
				else
				{
					SaveCustomError("IDispatch_AsyncInvoke", "Cannot convert from type %C to VT_I4", Vartype(pValue));
					throw E_APIERROR;
				}
				break;
			case VT_I4:
				pArg.vt = VT_I4;
				if (Vartype(pValue) == 'N')
					pArg.lVal = (int)pValue.ev_real;
				else if (Vartype(pValue) == 'I')
					pArg.lVal = pValue.ev_long;
				else
				{
					SaveCustomError("IDispatch_AsyncInvoke", "Cannot convert from type %C to VT_I4", Vartype(pValue));
					throw E_APIERROR;
				}
				break;
			case VT_R4:
				pArg.vt = VT_R4;
				if (Vartype(pValue) == 'N')
					pArg.fltVal = static_cast<float>(pValue.ev_real);
				else if (Vartype(pValue) == 'I')
					pArg.fltVal = static_cast<float>(pValue.ev_long);
				else
				{
					SaveCustomError("IDispatch_AsyncInvoke", "Cannot convert from type %C to VT_R4", Vartype(pValue));
					throw E_APIERROR;
				}
				break;
			case VT_R8:
				pArg.vt = VT_R8;
				if (Vartype(pValue) == 'N')
					pArg.dblVal = pValue.ev_real;
				else if (Vartype(pValue) == 'I')
					pArg.dblVal = (double)pValue.ev_long;
                else
				{
					SaveCustomError("IDispatch_AsyncInvoke", "Cannot convert from type %C to VT_R8", Vartype(pValue));
					throw E_APIERROR;
				}
				break;
			case VT_CY:
				pArg.vt = VT_CY;
				pArg.cyVal.int64 = pValue.ev_currency.QuadPart;
				break;
			case VT_DATE:
				MarshalDate(pArg,pValue);
				break;
			case VT_BSTR:
				MarshalString(pArg,pValue);
				break;
			case VT_DISPATCH:
				MarshalIDispatch(pArg,pValue);
				break;
			case VT_ERROR:
				pArg.vt = VT_ERROR;
				break;
			case VT_BOOL:
				pArg.vt = VT_BOOL;
				pArg.boolVal = pValue.ev_length ? VARIANT_TRUE : VARIANT_FALSE;
				break;
			case VT_VARIANT:
				MarshalVariant(pArg, pValue, pArg.vt);
				break;
			case VT_UNKNOWN:
				SaveCustomError("IDispatch_AsyncInvoke", "Parameter type VT_UNKNOWN not supported!");
				throw E_APIERROR;
			case VT_DECIMAL:
				MarshalDecimal(pArg,pValue);
				break;
			case VT_I1:
				SaveCustomError("IDispatch_AsyncInvoke", "Parameter type VT_I1 not supported!");
				throw E_APIERROR;
			case VT_UI1:
				SaveCustomError("IDispatch_AsyncInvoke", "Parameter type VT_UI1 not supported!");
				throw E_APIERROR;
			case VT_UI2:
				SaveCustomError("IDispatch_AsyncInvoke", "Parameter type VT_UI2 not supported!");
				throw E_APIERROR;
			case VT_UI4:
				pArg.vt = VT_UI4;
				if (Vartype(pValue) == 'N')
					pArg.ulVal = static_cast<unsigned long>(pValue.ev_real);
				else if (Vartype(pValue) == 'I')
					pArg.ulVal = static_cast<unsigned long>(pValue.ev_long);
				else
				{
					SaveCustomError("IDispatch_AsyncInvoke", "Cannot convert from type %C to VT_UI4", Vartype(pValue));
					throw E_APIERROR;
				}
				break;
			case VT_INT:
				pArg.vt = VT_INT;
				if (Vartype(pValue) == 'N')
					pArg.intVal = static_cast<int>(pValue.ev_real);
				else if (Vartype(pValue) == 'I')
					pArg.intVal = pValue.ev_long;
				else
				{
					SaveCustomError("IDispatch_AsyncInvoke", "Cannot convert from type %C to VT_INT", Vartype(pValue));
					throw E_APIERROR;
				}
				break;
			case VT_UINT:
				pArg.vt = VT_UINT;
				if (Vartype(pValue) == 'N')
					pArg.uintVal = static_cast<unsigned int>(pValue.ev_real);
				else if (Vartype(pValue) == 'I')
					pArg.uintVal = static_cast<unsigned int>(pValue.ev_long);
				else
				{
					SaveCustomError("IDispatch_AsyncInvoke", "Cannot convert from type %C to VT_UINT", Vartype(pValue));
					throw E_APIERROR;
				}
				break;
			case VT_RECORD:
				SaveCustomError("IDispatch_AsyncInvoke", "Parameter type VT_RECORD not supported!");
				throw E_APIERROR;
			default:
				SaveCustomError("IDispatch_AsyncInvoke", "Invalid parameter type %I", vType);
				throw E_APIERROR;
		}
	}
}

void _stdcall MarshalDecimal(VARIANT &pArg, Value &pValue)
{
	pArg.vt = VT_DECIMAL;
	MarshalDecimal(pArg.decVal,pValue);
}

void _stdcall MarshalDecimal(DECIMAL &pDec, Value &pValue)
{
	if (Vartype(pValue) == 'N')
	{
		HRESULT hr = VarDecFromR8(pValue.ev_real,&pDec);
		if (FAILED(hr))
		{
			SaveWin32Error("VarDecFromR8", hr);
			throw E_APIERROR;
		}
	}
	else if (Vartype(pValue) == 'Y')
	{
		pDec.Lo32 = pValue.ev_currency.LowPart;
		pDec.Mid32 = pValue.ev_currency.HighPart;
		pDec.Hi32 = 0;
        pDec.scale = 4;
		pDec.sign = pValue.ev_currency.HighPart >= 0 ? 1 : -1;
	}
	else if (Vartype(pValue) == 'I')
	{
		HRESULT hr = VarDecFromI4(pValue.ev_long,&pDec);
		if (FAILED(hr))
		{
			SaveWin32Error("VarDecFromI8", hr);
			throw E_APIERROR;
		}
	}
	else
	{
		SaveCustomError("IDispatch_AsyncInvoke", "Cannot conver from type '%C' to VT_DECIMAL.", Vartype(pValue));
		throw E_APIERROR;
	}
}

void _stdcall MarshalSafeArrayVariant(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase)
{
	SAFEARRAYBOUND *pArrayBounds = 0;
try
{
	FoxArray pArray(pArrayName);
	FoxValue pElement;
	unsigned int nRows, nDims;
	nRows = pArray.ALen(nDims);

	pArrayBounds = new SAFEARRAYBOUND[nDims];
	if (!pArrayBounds)
		throw E_INSUFMEMORY;
	for (unsigned int xj = 0; xj < nDims; xj++)
	{
		pArrayBounds[xj].lLbound = nBase;
		pArrayBounds[xj].cElements = nRows;
	}

	pArg.parray = SafeArrayCreate(VT_VARIANT,nDims,pArrayBounds);
	if (pArg.parray == 0)
		throw E_INSUFMEMORY;

	pArg.vt = VT_ARRAY | VT_VARIANT;

	VARIANT* pOleElement;
	HRESULT hr;
	
	hr = SafeArrayAccessData(pArg.parray,(void**)&pOleElement);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayAccessData", hr);
		throw E_APIERROR;
	}

	VARIANT *pTmpElement = pOleElement;
	for (unsigned int nDim = 1; nDim <= nDims; nDim++)
	{
		for (unsigned int nRow = 1; nRow <= nRows; nRow++)
		{
			VariantInit(pTmpElement);
			pTmpElement++;
		}
	}

	for (unsigned int nDim = 1; nDim <= nDims; nDim++)
	{
		for (unsigned int nRow = 1; nRow <= nRows; nRow++)
		{
			pElement = pArray(nRow, nDim);
			MarshalVariant(*pOleElement, pElement, pOleElement->vt);
			pOleElement++;
		}
	}

	hr = SafeArrayUnaccessData(pArg.parray);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayUnaccessData", hr);
		throw E_APIERROR;
	}
	
	delete[] pArrayBounds;
}
catch(int nErrorNo)
{
	if (pArrayBounds)
		delete[] pArrayBounds;
	throw nErrorNo;
}
}

void _stdcall MarshalSafeArrayBool(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase)
{
	SAFEARRAYBOUND *pArrayBounds = 0;
try
{
	FoxArray pArray(pArrayName);
	FoxValue pElement;
	unsigned int nRows, nDims;
	nRows = pArray.ALen(nDims);

	pArrayBounds = new SAFEARRAYBOUND[nDims];
	if (!pArrayBounds)
		throw E_INSUFMEMORY;
	for (unsigned int xj = 0; xj < nDims; xj++)
	{
		pArrayBounds[xj].lLbound = nBase;
		pArrayBounds[xj].cElements = nRows;
	}

	pArg.parray = SafeArrayCreate(VT_BOOL,nDims,pArrayBounds);
	if (pArg.parray == 0)
		throw E_INSUFMEMORY;
    
	pArg.vt = VT_ARRAY | VT_BOOL;

	VARIANT_BOOL* pOleElement;
	HRESULT hr;
	
	hr = SafeArrayAccessData(pArg.parray,(void**)&pOleElement);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayAccessData", hr);
		throw E_APIERROR;
	}

	for (unsigned int nDim = 1; nDim <= nDims; nDim++)
	{
		for (unsigned int nRow = 1; nRow <= nRows; nRow++)
		{
			pElement = pArray(nRow, nDim);
			if (pElement.Vartype() == 'L')
				*pOleElement = pElement->ev_length > 0 ? VARIANT_TRUE : VARIANT_FALSE;
			else
			{
				SafeArrayUnaccessData(pArg.parray);
				SaveCustomError("AsyncInvoke","Datatype mismatch during array marshaling!");
				throw E_APIERROR;
			}
			pOleElement++;
		}
	}

	hr = SafeArrayUnaccessData(pArg.parray);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayUnaccessData", hr);
		throw E_APIERROR;
	}
	
	delete[] pArrayBounds;
}
catch(int nErrorNo)
{
	if (pArrayBounds)
		delete[] pArrayBounds;
	throw nErrorNo;
}
}

void _stdcall MarshalSafeArrayInt(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase)
{
	SAFEARRAYBOUND *pArrayBounds = 0;
try
{
	FoxArray pArray(pArrayName);
	FoxValue pElement;
	unsigned int nRows, nDims;
	nRows = pArray.ALen(nDims);

	pArrayBounds = new SAFEARRAYBOUND[nDims];
	if (!pArrayBounds)
		throw E_INSUFMEMORY;
	for (unsigned int xj = 0; xj < nDims; xj++)
	{
		pArrayBounds[xj].lLbound = nBase;
		pArrayBounds[xj].cElements = nRows;
	}

	pArg.parray = SafeArrayCreate(vType,nDims,pArrayBounds);
	if (pArg.parray == 0)
		throw E_INSUFMEMORY;
    
	pArg.vt = VT_ARRAY | vType;

	int* pOleElement;
	HRESULT hr;
	
	hr = SafeArrayAccessData(pArg.parray,(void**)&pOleElement);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayAccessData", hr);
		throw E_APIERROR;
	}

	for (unsigned int nDim = 1; nDim <= nDims; nDim++)
	{
		for (unsigned int nRow = 1; nRow <= nRows; nRow++)
		{
			pElement = pArray(nRow,nDim);
			if (pElement.Vartype() == 'N')
				*pOleElement = static_cast<int>(pElement->ev_real);
			else if (pElement.Vartype() == 'I')
				*pOleElement = pElement->ev_long;
			else
			{
				SafeArrayUnaccessData(pArg.parray);
				SaveCustomError("AsyncInvoke","Datatype mismatch during array marshaling!");
				throw E_APIERROR;
			}
			pOleElement++;
		}
	}

	hr = SafeArrayUnaccessData(pArg.parray);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayUnaccessData", hr);
		throw E_APIERROR;
	}
	
	delete[] pArrayBounds;
}
catch(int nErrorNo)
{
	if (pArrayBounds)
		delete[] pArrayBounds;
	throw nErrorNo;
}
}

void _stdcall MarshalSafeArraySingle(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase)
{
	SAFEARRAYBOUND *pArrayBounds = 0;
try
{
	FoxArray pArray(pArrayName);
	FoxValue pElement;
	unsigned int nRows, nDims;
	nRows = pArray.ALen(nDims);

	pArrayBounds = new SAFEARRAYBOUND[nDims];
	if (!pArrayBounds)
		throw E_INSUFMEMORY;
	for (unsigned int xj = 0; xj < nDims; xj++)
	{
		pArrayBounds[xj].lLbound = nBase;
		pArrayBounds[xj].cElements = nRows;
	}

	pArg.parray = SafeArrayCreate(VT_R4,nDims,pArrayBounds);
	if (pArg.parray == 0)
		throw E_INSUFMEMORY;

	pArg.vt = VT_ARRAY | VT_R4;

	float* pOleElement;
	HRESULT hr;
	
	hr = SafeArrayAccessData(pArg.parray,(void**)&pOleElement);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayAccessData", hr);
		throw E_APIERROR;
	}

	for (unsigned int nDim = 1; nDim <= nDims; nDim++)
	{
		for (unsigned int nRow = 1; nRow <= nRows; nRow++)
		{
			pElement = pArray(nRow,nDim);
			if (pElement.Vartype() == 'N')
				*pOleElement = static_cast<float>(pElement->ev_real);
			else if (pElement.Vartype() == 'I')
				*pOleElement = static_cast<float>(pElement->ev_long);
			else
			{
				SafeArrayUnaccessData(pArg.parray);
				SaveCustomError("AsyncInvoke","Datatype mismatch during array marshaling!");
				throw E_APIERROR;
			}
			pOleElement++;
		}
	}

	hr = SafeArrayUnaccessData(pArg.parray);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayUnaccessData", hr);
		throw E_APIERROR;
	}
	
	delete[] pArrayBounds;
}
catch(int nErrorNo)
{
	if (pArrayBounds)
		delete[] pArrayBounds;
	throw nErrorNo;
}
}

void _stdcall MarshalSafeArrayDouble(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase)
{
	SAFEARRAYBOUND *pArrayBounds = 0;
try
{
	FoxArray pArray(pArrayName);
	FoxValue pElement;
	unsigned int nRows, nDims;
	nRows = pArray.ALen(nDims);

	pArrayBounds = new SAFEARRAYBOUND[nDims];
	if (!pArrayBounds)
		throw E_INSUFMEMORY;
	for (unsigned int xj = 0; xj < nDims; xj++)
	{
		pArrayBounds[xj].lLbound = nBase;
		pArrayBounds[xj].cElements = nRows;
	}

	pArg.parray = SafeArrayCreate(VT_R8,nDims,pArrayBounds);
	if (pArg.parray == 0)
		throw E_INSUFMEMORY;

	pArg.vt = VT_ARRAY | VT_R8;

	double* pOleElement;
	HRESULT hr;
	
	hr = SafeArrayAccessData(pArg.parray,(void**)&pOleElement);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayAccessData", hr);
		throw E_APIERROR;
	}

	for (unsigned int nDim = 1; nDim <= nDims; nDim++)
	{
		for (unsigned int nRow = 1; nRow <= nRows; nRow++)
		{
			pElement = pArray(nRow,nDim);
			if (pElement.Vartype() == 'N')
				*pOleElement = pElement->ev_real;
			else if (pElement.Vartype() == 'I')
				*pOleElement = static_cast<double>(pElement->ev_long);
			else
			{
				SafeArrayUnaccessData(pArg.parray);
				SaveCustomError("AsyncInvoke","Datatype mismatch during array marshaling!");
				throw E_APIERROR;
			}
			pOleElement++;
		}
	}

	hr = SafeArrayUnaccessData(pArg.parray);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayUnaccessData", hr);
		throw E_APIERROR;
	}
	
	delete[] pArrayBounds;
}
catch(int nErrorNo)
{
	if (pArrayBounds)
		delete[] pArrayBounds;
	throw nErrorNo;
}
}
	
void _stdcall MarshalSafeArrayDate(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase)
{
	SAFEARRAYBOUND *pArrayBounds = 0;
try
{
	FoxArray pArray(pArrayName);
	FoxValue pElement;
	unsigned int nRows, nDims;
	nRows = pArray.ALen(nDims);

	pArrayBounds = new SAFEARRAYBOUND[nDims];
	if (!pArrayBounds)
		throw E_INSUFMEMORY;
	for (unsigned int xj = 0; xj < nDims; xj++)
	{
		pArrayBounds[xj].lLbound = nBase;
		pArrayBounds[xj].cElements = nRows;
	}

	pArg.parray = SafeArrayCreate(VT_DATE,nDims,pArrayBounds);
	if (pArg.parray == 0)
		throw E_INSUFMEMORY;

	pArg.vt = VT_ARRAY | VT_DATE;

	DATE* pOleElement;
	HRESULT hr;
	
	hr = SafeArrayAccessData(pArg.parray,(void**)&pOleElement);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayAccessData", hr);
		throw E_APIERROR;
	}

	for (unsigned int nDim = 1; nDim <= nDims; nDim++)
	{
		for (unsigned int nRow = 1; nRow <= nRows; nRow++)
		{
			pElement = pArray(nRow,nDim);
			if (pElement.Vartype() == 'T' || pElement.Vartype() == 'D')
				*pOleElement = pElement->ev_real - VFPOLETIMEBASE;
			else
			{
				SafeArrayUnaccessData(pArg.parray);
				SaveCustomError("AsyncInvoke","Datatype mismatch during array marshaling!");
				throw E_APIERROR;
			}
			pOleElement++;
		}
	}

	hr = SafeArrayUnaccessData(pArg.parray);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayUnaccessData", hr);
		throw E_APIERROR;
	}
	
	delete[] pArrayBounds;
}
catch(int nErrorNo)
{
	if (pArrayBounds)
		delete[] pArrayBounds;
	throw nErrorNo;
}

}
	
void _stdcall MarshalSafeArrayBSTR(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase)
{
	SAFEARRAYBOUND *pArrayBounds = 0;
try
{
	FoxArray pArray(pArrayName);
	FoxString pElement;
	unsigned int nRows, nDims;
	nRows = pArray.ALen(nDims);

	pArrayBounds = new SAFEARRAYBOUND[nDims];
	if (!pArrayBounds)
		throw E_INSUFMEMORY;
	for (unsigned int xj = 0; xj < nDims; xj++)
	{
		pArrayBounds[xj].lLbound = nBase;
		pArrayBounds[xj].cElements = nRows;
	}

	pArg.parray = SafeArrayCreate(VT_BSTR,nDims,pArrayBounds);
	if (pArg.parray == 0)
		throw E_INSUFMEMORY;
    
	pArg.vt = VT_ARRAY | VT_BSTR;

	BSTR* pData;
	HRESULT hr;
	
	hr = SafeArrayAccessData(pArg.parray,(void**)&pData);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayAccessData", hr);
		throw E_APIERROR;
	}

	for (unsigned int nDim = 1; nDim <= nDims; nDim++)
	{
		for (unsigned int nRow = 1; nRow <= nRows; nRow++)
		{
			pElement = pArray(nRow,nDim);
			if (pElement.Vartype() == 'C')
			{
				*pData = pElement.ToBSTR();
			}
			else
			{
				SafeArrayUnaccessData(pArg.parray);
				SaveCustomError("AsyncInvoke","Datatype mismatch during array marshaling!");
				throw E_APIERROR;
			}
			pData++;
		}
	}

	hr = SafeArrayUnaccessData(pArg.parray);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayUnaccessData", hr);
		throw E_APIERROR;
	}
	
	delete[] pArrayBounds;
}
catch(int nErrorNo)
{
	if (pArrayBounds)
		delete[] pArrayBounds;
	throw nErrorNo;
}

}
	
void _stdcall MarshalSafeArrayUI1(VARIANT &pArg, Value &pValue, VARTYPE vType, int nBase)
{
try
{
	if (Vartype(pValue) == '0')
	{
		pArg.vt = VT_EMPTY;
		return;
	}

	FoxString pString;
	pString.Attach(pValue);

	if (vType & VT_BYREF)
	{
		pArg.pparray = (SAFEARRAY**)CoTaskMemAlloc(sizeof(SAFEARRAY*));
        *pArg.pparray = pString.ToU1SafeArray();
	}
	else
	{
		pArg.parray = pString.ToU1SafeArray();
	}
	pArg.vt = vType;
}
catch(int nErrorNo)
{
	throw nErrorNo;
}
}

void _stdcall MarshalSafeArrayEmpty(VARIANT &pArg, VARTYPE vType, int nBase)
{
try
{
	SAFEARRAYBOUND  pArrayBounds[1];
	pArrayBounds[0].lLbound = nBase;
	pArrayBounds[0].cElements = 0;

	pArg.parray = SafeArrayCreate(VT_VARIANT,1,pArrayBounds);
	if (pArg.parray == 0)
		throw E_INSUFMEMORY;

	pArg.vt = VT_ARRAY | VT_VARIANT;
}
catch(int nErrorNo)
{
	throw nErrorNo;
}
}

void _stdcall MarshalSafeArrayDecimal(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase)
{
	SAFEARRAYBOUND *pArrayBounds = 0;
try
{
	FoxArray pArray(pArrayName);
	FoxValue pElement;
	unsigned int nRows, nDims;
	nRows = pArray.ALen(nDims);

	pArrayBounds = new SAFEARRAYBOUND[nDims];
	if (!pArrayBounds)
		throw E_INSUFMEMORY;
	for (unsigned int xj = 0; xj < nDims; xj++)
	{
		pArrayBounds[xj].lLbound = nBase;
		pArrayBounds[xj].cElements = nRows;
	}

	pArg.parray = SafeArrayCreate(VT_DECIMAL,nDims,pArrayBounds);
	if (pArg.parray == 0)
		throw E_INSUFMEMORY;

	pArg.vt = VT_ARRAY | VT_DECIMAL;

	DECIMAL* pData;
	HRESULT hr;
	
	hr = SafeArrayAccessData(pArg.parray,(void**)&pData);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayAccessData", hr);
		throw E_APIERROR;
	}

	for (unsigned int nDim = 1; nDim <= nDims; nDim++)
	{
		for (unsigned int nRow = 1; nRow <= nRows; nRow++)
		{
			pElement = pArray(nRow,nDim);
			if (pElement.Vartype() == 'N' || pElement.Vartype() == 'Y' || pElement.Vartype() == 'I')
				MarshalDecimal(*pData, pElement);
			else
			{
				SafeArrayUnaccessData(pArg.parray);
				SaveCustomError("AsyncInvoke","Datatype mismatch during array marshaling!");
				throw E_APIERROR;
			}
			pData++;
		}
	}

	hr = SafeArrayUnaccessData(pArg.parray);
	if (FAILED(hr))
	{
		SaveWin32Error("SafeArrayUnaccessData", hr);
		throw E_APIERROR;
	}
	
	delete[] pArrayBounds;
}
catch(int nErrorNo)
{
	if (pArrayBounds)
		delete[] pArrayBounds;
	throw nErrorNo;
}
}

void _stdcall UnMarshalVariant(VARIANT &vVar, Value &pVal)
{
	if (vVar.vt & VT_ARRAY)
	{
		VARTYPE vt = vVar.vt & VT_TYPEMASK;
		switch(vt)
		{
			case VT_UI1:
				UnMarshalSafeArrayUI1(vVar,pVal);
				break;
		}
		return;
	}

	bool bByValue = (vVar.vt & VT_BYREF) == 0;

 	switch(vVar.vt)
	{
		case VT_EMPTY:
		case VT_NULL:
			pVal.ev_type = '0';
			break;
		case VT_I2:
			pVal.ev_type = 'I';
			pVal.ev_width = 6;
			pVal.ev_long = bByValue ? vVar.iVal : *vVar.piVal;
			break;
		case VT_I4:
			pVal.ev_type = 'I';
			pVal.ev_width = 11;
			pVal.ev_long = 	bByValue ? vVar.lVal : *vVar.plVal;
			break;
		case VT_R4:
			pVal.ev_type = 'N';
			pVal.ev_width = 20;
			pVal.ev_length = 7;
			pVal.ev_real = bByValue ? static_cast<double>(vVar.fltVal) : static_cast<double>(*vVar.pfltVal);
			break;
		case VT_R8:
			pVal.ev_type = 'N';
			pVal.ev_width = 20;
			pVal.ev_length = 16;
			pVal.ev_real = bByValue ? vVar.dblVal : *vVar.pdblVal;
			break;
		case VT_CY:
			pVal.ev_type = 'Y';
			pVal.ev_width = 21;
			pVal.ev_currency.QuadPart = bByValue ? vVar.cyVal.int64 : vVar.pcyVal->int64;
			break;
		case VT_DATE:
			pVal.ev_type = 'T';
			pVal.ev_real = bByValue ? (vVar.date + VFPOLETIMEBASE) : (*vVar.pdate + VFPOLETIMEBASE);
			break;
		case VT_BSTR:
			UnMarshalBSTR(vVar,pVal);
			break;
		case VT_DISPATCH:
			UnMarshalIDispatch(vVar,pVal);
			break;
		case VT_ERROR:
			pVal.ev_type = 'I';
			pVal.ev_width = 11;
			pVal.ev_long = bByValue ? vVar.scode : *vVar.pscode;
			break;
		case VT_BOOL:
			pVal.ev_type = 'L';
			pVal.ev_length = bByValue ? (vVar.boolVal == VARIANT_TRUE) : (*vVar.pboolVal == VARIANT_TRUE);
			break;
		case VT_UNKNOWN:
			UnMarshalIUnknown(vVar,pVal);
			break;
		case VT_DECIMAL:
			UnMarshalDecimal(vVar,pVal);
			break;
		case VT_I1:
			pVal.ev_type = 'I';
			pVal.ev_width = 4;
			pVal.ev_long = bByValue ? vVar.cVal : *vVar.pcVal;
			break;
		case VT_UI1:
			pVal.ev_type = 'I';
			pVal.ev_width = 3;
			pVal.ev_long = bByValue ? vVar.bVal : *vVar.pbVal;
			break;
		case VT_UI2:
			pVal.ev_type = 'I';
			pVal.ev_width = 5;
			pVal.ev_long = bByValue ? vVar.uiVal : *vVar.puiVal;
			break;
		case VT_UI4:
			pVal.ev_type = 'N';
			pVal.ev_width = 10;
			pVal.ev_length = 0;
			pVal.ev_real = bByValue ? static_cast<double>(vVar.ulVal) : static_cast<double>(*vVar.pulVal);
			break;
		case VT_INT:
			pVal.ev_type = 'I';
			pVal.ev_width = 11;
			pVal.ev_long = bByValue ? vVar.intVal : *vVar.pintVal;
			break;
		case VT_UINT:
			pVal.ev_type = 'N';
			pVal.ev_width = 10;
			pVal.ev_length = 0;
			pVal.ev_real = bByValue ? static_cast<double>(vVar.uintVal) : static_cast<double>(*vVar.puintVal);
			break;
		case VT_RECORD:
			pVal.ev_type = 'I';
			pVal.ev_width = 11;
			pVal.ev_long = reinterpret_cast<long>(vVar.pvRecord);
			break;
		default:
			pVal.ev_type = 'L';
			pVal.ev_length = FALSE;
	}
}

void _stdcall UnMarshalBSTR(VARIANT &vVar, Value &pVal)
{
	FoxString pString((vVar.vt & VT_BYREF) > 0 ? *vVar.pbstrVal : vVar.bstrVal);
	pString.Detach(pVal);
}

void _stdcall UnMarshalIDispatch(VARIANT &vVar, Value &pVal)
{
	char aCommand[64];
	sprintfex(aCommand,"SYS(3096,%I)", (vVar.vt & VT_BYREF) > 0 ? *vVar.ppdispVal : vVar.pdispVal);
	Evaluate(pVal,aCommand);
}

void _stdcall UnMarshalIUnknown(VARIANT &vVar, Value &pVal)
{
	HRESULT hr;
	IDispatch *pDisp;
	IUnknown *pUnk;
	char aCommand[64];

	pUnk = (vVar.vt & VT_BYREF) > 0 ? *vVar.ppunkVal : vVar.punkVal;

	hr = pUnk->QueryInterface(IID_IDispatch,(void**)&pDisp);
	if (FAILED(hr))
	{
		SaveWin32Error("IUnknown.QueryInterface", hr);
		throw E_APIERROR;
	}

	sprintfex(aCommand,"SYS(3096,%I)",pDisp);
	try
	{
		Evaluate(pVal,aCommand);
	}
	catch(int nErrorNo)
	{
		pDisp->Release();
		throw nErrorNo;
	}
}

void _stdcall UnMarshalSafeArrayUI1(VARIANT &vVar, Value &pVal)
{
	SAFEARRAY *pArray = (vVar.vt & VT_BYREF) ? *vVar.pparray : vVar.parray;
	FoxString pTemp(pArray);
	pTemp.Detach(pVal);
}

void _stdcall UnMarshalDecimal(VARIANT &vVar, Value &pVal)
{

}

void _stdcall ReleaseVarientEx(VARIANT *pArg)
{
	bool bRef = (pArg->vt & VT_BYREF) > 0;
	if (bRef)
	{
		bool bArray = (pArg->vt & VT_ARRAY) > 0;
		if (bArray)
		{
			HRESULT hr;
			hr = SafeArrayDestroy(*pArg->pparray);
            CoTaskMemFree(pArg->pparray);
		}
		else
		{
			VARTYPE vt = (pArg->vt & VT_TYPEMASK);
			switch(vt)
			{
				case VT_STREAM:
					((LPSTREAM)pArg->byref)->Release();
					break;
				default:
					CoTaskMemFree(pArg->pbstrVal);
			}
		}

		pArg->vt = VT_EMPTY;
	}
	else
		VariantClear(pArg);
}