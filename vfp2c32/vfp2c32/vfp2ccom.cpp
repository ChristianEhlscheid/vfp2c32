#define _WINSOCKAPI_ // we're using winsock2 .. so this is neccessary to exclude winsock.h 
#define _WIN32_DCOM

#include <windows.h>
#include <stdio.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2ccom.h"
#include "vfp2ccppapi.h"
#include "vfp2chelpers.h"
#include "vfp2ccomcall.h"
#include "vfpmacros.h"

// static list<RegisteredComDll*> glRegisteredComDlls;
// static PLOADTYPELIBEX fpLoadTypeLibEx = 0;

void _stdcall VFP2C_Init_Com()
{
try
{
	FoxVariable pTempVar("__VFP2C_FLL_FILENAME", false);
	FoxString pFllFileName(MAX_PATH);

	pFllFileName.Len(GetModuleFileName(ghModule, pFllFileName, pFllFileName.Size()));
	if (!pFllFileName.Len())
	{
		AddWin32Error("GetModuleFileName", GetLastError());
		return;
	}

	pTempVar = pFllFileName;
	// declare additional functions not nativly exported by the FLL
	Execute("DECLARE INTEGER GetIDispatch IN (m.__VFP2C_FLL_FILENAME) OBJECT");
}
catch(int nErrorNo)
{
	AddCustomErrorEx("_Execute", "Failed to DECLARE function.", nErrorNo);
}
}

void _stdcall GetIDispatchFromObject(Value &pVal, void** pDisp)
{
	char* VarName = "__VFP2C32_TEMP_COMOBJECT";
	HRESULT hr;
	VARIANT vResult;
	FoxVariable pTmpObject(VarName, false);
	FoxValue vObject;
	IDispatch *pVFP;
	VariantInit(&vResult);

	pTmpObject = pVal;
	vObject.Evaluate("SYS(3095,_VFP)");
	pVFP = (IDispatch*)vObject->ev_long;
	LPOLESTR pMethod = L"Eval";
	DISPID dispidEval;
	hr = pVFP->GetIDsOfNames(IID_NULL, &pMethod, 1, 1033, &dispidEval);
	if (SUCCEEDED(hr))
	{
		CComBSTR pCommand(L"m.__VFP2C32_TEMP_COMOBJECT");
		VARIANTARG Arg;
		DISPPARAMS DispParams = {&Arg, 0, 1, 0};
		Arg.vt = VT_BSTR;
		Arg.bstrVal = pCommand;
		hr = pVFP->Invoke(dispidEval, IID_NULL, 0, DISPATCH_METHOD, &DispParams, &vResult, 0, 0);
		if (SUCCEEDED(hr))
			*pDisp = vResult.pdispVal;
	}
}

IDispatch * _stdcall GetIDispatch(IDispatch *pObject)
{
	return pObject;
}

void _fastcall CLSIDFromProgIDLib(ParamBlk *parm)
{
try
{
	FoxWString pProgId(p1);
	FoxString pClsId(sizeof(GUID));
	HRESULT hr;

	hr = CLSIDFromProgID(pProgId,(LPCLSID)(void*)pClsId);
	if (FAILED(hr))
	{
		SaveWin32Error("CLSIDFromProgID", hr);
		throw E_APIERROR;
	}
	
	pClsId.Binary(true);
	pClsId.Len(sizeof(GUID));
	pClsId.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ProgIDFromCLSIDLib(ParamBlk *parm)
{
try
{
	FoxString pClsId(parm,1);
	FoxWString pWideClsId;
	FoxString pProgId;

	CoTaskPtr pWideProgId;
	HRESULT hr;
	LPGUID pGuid;
	GUID sGuid;

	if (Vartype(p1) == 'C')
	{
		if (pClsId.Len() == sizeof(GUID))
			pGuid = (LPGUID)(void*)pClsId;
		else
		{
			pWideClsId = pClsId;
			hr = CLSIDFromString(pWideClsId,&sGuid);
			if (FAILED(hr))
			{
				SaveWin32Error("CLSIDFromString", hr);
				throw E_APIERROR;
			}
			pGuid = &sGuid;
		}
	}
	else if (Vartype(p1) == 'I')
		pGuid = (LPGUID)p1.ev_long;
	else if (Vartype(p1) == 'N')
		pGuid = (LPGUID)(ULONG)p1.ev_real;
	else
		throw E_INVALIDPARAMS;

	hr = ProgIDFromCLSID((REFCLSID)*pGuid,pWideProgId);
	if (FAILED(hr))
	{
		SaveWin32Error("ProgIDFromCLSID", hr);
		throw E_APIERROR;
	}

	pProgId = (LPWSTR)pWideProgId;
	CoTaskMemFree(pWideProgId);
	pProgId.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall CLSIDFromStringLib(ParamBlk *parm)
{
try
{
	FoxWString pString(p1);
	FoxString pClsId(sizeof(GUID));
	HRESULT hr;

	hr = CLSIDFromString(pString,(LPCLSID)(void*)pClsId);
	if (FAILED(hr))
	{
		SaveWin32Error("CLSIDFromString", hr);
		throw E_APIERROR;
	}

	pClsId.Binary(true);
	pClsId.Len(sizeof(GUID));
	pClsId.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall StringFromCLSIDLib(ParamBlk *parm)
{
try
{
	FoxString pClsId(parm, 1, 0);
	FoxString pString;

	int nRetVal;
	LPGUID pGuid;
	wchar_t aGuidString[GUID_STRING_LEN];

	if (Vartype(p1) == 'C' && Len(p1) == sizeof(GUID))
		pGuid = (LPGUID)(void*)pClsId;
	else if (Vartype(p1) == 'I')
		pGuid = (LPGUID)p1.ev_long;
	else if (Vartype(p1) == 'N')
		pGuid = (LPGUID)(ULONG)p1.ev_real;
	else
		throw E_INVALIDPARAMS;

	nRetVal = StringFromGUID2((REFGUID)*pGuid,aGuidString,GUID_STRING_LEN);

	pString = aGuidString;
	pString.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall IsEqualGUIDLib(ParamBlk *parm)
{
try
{
	FoxString pGuidParm1(parm,1,0);
	FoxString pGuidParm2(parm,2,0);
	FoxWString pWideGuid;
	HRESULT hr;
	bool bRetVal;
	LPGUID pGuid1, pGuid2;
	GUID sGuid1, sGuid2;

	// convert GUID1
	if (Vartype(p1) == 'C')
	{	if (Len(p1) == sizeof(GUID))
			pGuid1 = (LPGUID)(void*)pGuidParm1;
		else
		{
			pGuidParm1.NullTerminate();
			pWideGuid = pGuidParm1;
			hr = CLSIDFromString(pWideGuid,&sGuid1);
			if (FAILED(hr))
			{
				SaveWin32Error("CLSIDFromString", hr);
				throw E_APIERROR;
			}
			pGuid1 = &sGuid1;
		}
	}
	else if (Vartype(p1) == 'I')
		pGuid1 = (LPGUID)p1.ev_long;
    else if (Vartype(p1) == 'N')
		pGuid1 = (LPGUID)(ULONG)p1.ev_real;
	else
		throw E_INVALIDPARAMS;

	// convert GUID2
	if (Vartype(p2) == 'C')
	{
		if (Len(p2) == sizeof(GUID))
			pGuid2 = (LPGUID)(void*)pGuidParm2;
		else
		{
			pGuidParm2.Expand();
			pWideGuid = pGuidParm2;
			hr = CLSIDFromString(pWideGuid,&sGuid2);
			if (FAILED(hr))
			{
				SaveWin32Error("CLSIDFromString", hr);
				throw E_APIERROR;
			}
			pGuid2 = &sGuid2;
		}
	}
	else if (Vartype(p2) == 'I')
		pGuid2 = (LPGUID)p2.ev_long;
    else if (Vartype(p2) == 'N')
		pGuid2 = (LPGUID)(ULONG)p2.ev_real;
	else
		throw E_INVALIDPARAMS;

	bRetVal = !memcmp(pGuid1,pGuid2,sizeof(GUID));
 	Return(bRetVal);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall CreateGuid(ParamBlk *parm)
{
try
{
	FoxString pGuidString;
	HRESULT hr;
	GUID sGuid;
	LPGUID pGuid = &sGuid;
	wchar_t aGuidString[GUID_STRING_LEN];
	int nRetVal;

	hr = CoCreateGuid(pGuid);
	if (FAILED(hr))
	{
		SaveWin32Error("CoCreateGuid", hr);
		throw E_APIERROR;
	}

	if (PCount() == 0 || p1.ev_long == CREATE_GUID_ANSI)
	{
		pGuidString.Size(GUID_STRING_LEN);
		nRetVal = StringFromGUID2((REFGUID)*pGuid,aGuidString,GUID_STRING_LEN);
		pGuidString = aGuidString;
	}
	else if (p1.ev_long == CREATE_GUID_UNICODE)
	{
		pGuidString.Size(GUID_STRING_LEN*sizeof(wchar_t));
		nRetVal = StringFromGUID2((REFGUID)*pGuid,(LPOLESTR)(char*)pGuidString,GUID_STRING_LEN);
		pGuidString.Len((nRetVal - 1) * sizeof(wchar_t));
	}
	else if (p1.ev_long == CREATE_GUID_BINARY)
	{
		pGuidString.Size(sizeof(GUID));
		memcpy(pGuidString,pGuid,sizeof(GUID));
		pGuidString.Len(sizeof(GUID));
		pGuidString.Binary(true);
	}

	pGuidString.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall RegisterActiveObjectLib(ParamBlk *parm)
{
try
{
	FoxWString pProgId(p2);

	HRESULT hr;
	DWORD nRotKey = 0;
	IUnknown *pUnk;
	FoxValue vUnk;
	CLSID sClsId;

	GetIDispatchFromObject(p1, (void**)&pUnk);

	hr = CLSIDFromProgID(pProgId,&sClsId);
	if (FAILED(hr))
	{
		SaveWin32Error("CLSIDFromProgID", hr);
		throw E_APIERROR;
	}

	hr = RegisterActiveObject(pUnk, sClsId, ACTIVEOBJECT_STRONG, &nRotKey);
	if (FAILED(hr))
	{
		SaveWin32Error("RegisterActiveObject", hr);
		throw E_APIERROR;
	}

	Return(nRotKey);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall RevokeActiveObjectLib(ParamBlk *parm)
{
	HRESULT hr;
	hr = RevokeActiveObject(p1.ev_long, 0);
	if (FAILED(hr))
	{
		SaveWin32Error("RevokeActiveObject", hr);
		RaiseError(E_APIERROR);
	}
}

void _fastcall RegisterObjectAsFileMoniker(ParamBlk *parm)
{
try
{
	FoxWString pProgId(p2);
	FoxWString pFileName(p3);

	ComPtr<LPRUNNINGOBJECTTABLE> pIRot;
	ComPtr<LPSTORAGE> pIStorage;
	ComPtr<LPMONIKER> pIFileMoniker;

	DWORD nRotKey = 0;
	HRESULT hr;
	IUnknown *pUnk;
	FoxValue vUnk;
	IID sClsId, spClsId;

	GetIDispatchFromObject(p1, (void**)&pUnk);

	hr = CLSIDFromProgID(pProgId,&spClsId);
	if (FAILED(hr))
	{
		SaveWin32Error("CLSIDFromProgID", hr);
		throw E_APIERROR;
	}

	hr = GetRunningObjectTable(0,pIRot);
	if (hr != S_OK)
	{
		SaveWin32Error("GetRunningObjectTable", hr);
		throw E_APIERROR;
	}

	hr = StgOpenStorage(pFileName, NULL, STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE,
							NULL, NULL, pIStorage );
	if (FAILED(hr))
	{
		// File does not exist
		// create a file and write the class id
		hr = StgCreateDocfile(pFileName, STGM_DIRECT|STGM_WRITE|STGM_SHARE_EXCLUSIVE,
							NULL, pIStorage);
		if (FAILED(hr))
		{
			SaveWin32Error("StgCreateDocfile", hr);
			throw E_APIERROR;
		}
		// Since this is a new file we must set the associated CLSID
		hr = WriteClassStg(pIStorage,spClsId);
		if (FAILED(hr))
		{
			SaveWin32Error("WriteClassStg", hr);
			throw E_APIERROR;
		}
	}
	else
	{
		// read the class id, just to make sure that the class id in the file is 
		// same as the what we expected.
		hr = ReadClassStg(pIStorage,&sClsId);
		if (FAILED(hr))
		{
			SaveWin32Error("ReadClassStg", hr);
			throw E_APIERROR;
		}
		if (spClsId != sClsId)
		{
			SaveCustomError("RegisterActiveObjectEx","File contained invalid CLSID.");
			throw E_APIERROR;
		}
	}

	// Create the File Moniker...
	hr = CreateFileMoniker(pFileName, pIFileMoniker);
	if (FAILED(hr))
	{
		SaveWin32Error("CreateFileMoniker", hr);
		throw E_APIERROR;
	}
	
	// Register the object in the ROT, 
	// use ROTFLAGS_ALLOWANYCLIENT so client running with other security context can view this ROT entry.
	// New flag added for DCOM purposes.
	// also note that RunAs interactive, Specified account, or service is need for this flag, to be valid.
	// refer to Win32 SDK documentation.
	hr = pIRot->Register(ROTFLAGS_ALLOWANYCLIENT|ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnk,
						pIFileMoniker, &nRotKey);
	if (FAILED(hr))
	{
		SaveWin32Error("ROT.Register", hr);
		throw E_APIERROR;
	}

	Return(nRotKey);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

/*
void _fastcall IsObjectActive(ParamBlk *parm)
{
try
{
	FoxWString pMoniker(p1);
	FoxString pClassName(parm,2);
	ComPtr<IRunningObjectTable*> pIRot;
	ComPtr<IMoniker*> pIMoniker;
	ComPtr<IBindCtx*> pIBind;
	HRESULT hr;
	//CLSID pClsID;
	ULONG nEaten;

	hr = CreateBindCtx(0,pIBind);
	if (FAILED(hr))
	{
		SaveWin32Error("CreateBindCtx,hr);
		throw E_APIERROR;
	}

	if (pMoniker)
	{
		hr = MkParseDisplayName(pIBind,pMoniker,&nEaten,pIMoniker);
		if (FAILED(hr))
		{
			SaveWin32Error("MkParseDisplayName,hr);
			throw E_APIERROR;
		}

		hr = GetRunningObjectTable(0,pIRot);
		if (FAILED(hr))
		{
			SaveWin32Error("GetRunningObjectTable,hr);
			throw E_APIERROR;
		}

		hr = pIRot->IsRunning(pIMoniker);
		Return(hr == S_OK);
	}
	else if (pClassName.Len())
	{
		FoxWString pClassNameW(pClassName);
		CLSID pClsId;
		hr = CLSIDFromString(pClassNameW,&pClsId);
		if (FAILED(hr))
		{
			SaveWin32Error("CLSIDFromString,hr);
			throw E_APIERROR;
		}

		wchar_t aGuidString[GUID_STRING_LEN+1];
		aGuidString[0] = L'!';
		StringFromGUID2(pClsId,&aGuidString[1],GUID_STRING_LEN);

		hr = CreateItemMoniker(L"!",aGuidString,pIMoniker);
		if (FAILED(hr))
		{
			SaveWin32Error("CLSIDFromString,hr);
			throw E_APIERROR;
		}

		hr = pIMoniker->IsRunning(pIBind,0,0);
		Return(hr == S_OK);
	}
	else
		throw E_INVALIDPARAMS;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}
*/

/*
void _fastcall CoCreateInstanceExLib(ParamBlk *parm)
{
	HMODULE hDll = 0;
	IDispatch *pDisp = 0;
try
{
	if (!fpLoadTypeLibEx)
		throw E_NOENTRYPOINT;

	FoxString pDllName(p1);
	FoxWString pWDllName(p1);
	FoxWString pCoClass(p2);

	ComPtr<ITypeLib*> pLib;
	ComPtr<IClassFactory*> pFactory;
	ComPtrArray<ITypeInfo*,16> pInfo;

	PDLLGETCLASSOBJECT fpDllGetClassObject = 0;
	GUID sCoClassGuid;

	hDll = GetModuleHandle(pDllName);
	if (!hDll)
	{
		hDll = CoLoadLibrary(pWDllName,TRUE);
		if (!hDll)
		{
			SaveWin32Error("LoadLibrary,GetLastError());
			throw E_APIERROR;
		}
	}

	fpDllGetClassObject = (PDLLGETCLASSOBJECT)GetProcAddress(hDll,"DllGetClassObject");
	if (!fpDllGetClassObject)
	{
		SaveCustomError("CoCreateInstanceEx","Function 'DllGetClassObject' not found in dll, it seems not to be a COM dll.");
		throw E_APIERROR;
	}

	HRESULT hr;
	hr = fpLoadTypeLibEx(pWDllName,REGKIND_NONE,pLib);
	if (FAILED(hr))
	{
		SaveWin32Error("LoadTypeLibEx,hr);
		throw E_APIERROR;
	}

	USHORT nTypesFound = 16;
	MEMBERID nMembers[16];
	TYPEATTR *pTypeAttr = 0;

	hr = pLib->FindName(pCoClass,0,pInfo,nMembers,&nTypesFound);
	if (FAILED(hr))
	{
		SaveCustomError("CoCreateInstanceEx, ITypeLib.FindName", "Error: %I", hr);
		throw E_APIERROR;
	}
	
	bool bTypeFound = false;
	for (int xj = 0; xj < nTypesFound && !bTypeFound; xj++)
	{
		hr = pInfo[xj]->GetTypeAttr(&pTypeAttr);

		if (FAILED(hr))
		{
			SaveCustomError("CoCreateInstanceEx, ITypeInfo.GetTypeAttr", "Error: %I", hr);
			throw E_APIERROR;
		}
		
		if (pTypeAttr->typekind == TKIND_COCLASS)
		{
			sCoClassGuid = pTypeAttr->guid;
			bTypeFound = true;
		}

		pInfo[xj]->ReleaseTypeAttr(pTypeAttr);
	}

	if (!bTypeFound)
	{
		SaveCustomError("CoCreateInstanceEx","CoClass not found in Typelibrary!");
		throw E_APIERROR;
	}

	hr = fpDllGetClassObject(sCoClassGuid,IID_IClassFactory,(LPVOID *)&pFactory);
	if (FAILED(hr))
	{
		SaveWin32Error("DllGetClassObject,hr);
		throw E_APIERROR;
	}

	hr = pFactory->CreateInstance(0,IID_IDispatch,(void**)&pDisp);
	if (FAILED(hr))
	{
		SaveCustomError("CoCreateInstanceEx, IClassFactory.CreateInstance", "Error: %I", hr);
		throw E_APIERROR;
	}

	Value pObject;
	pObject.ev_type = '0';
	char aObjectBuffer[32];
	sprintfex(aObjectBuffer,"SYS(3096,%I)",pDisp);
	Evaluate(pObject,aObjectBuffer);
	Return(pObject);
}
catch(int nErrorNo)
{
	if (pDisp)
		pDisp->Release();
	RaiseError(nErrorNo);
}
}
*/

/*
void _fastcall CoRegisterComDll(ParamBlk *parm)
{
	RegisteredComDll *pDll = new RegisteredComDll();
try
{
	FoxString pDllName(p1);
	FoxWString pWDllName(p1);

	pDll->RegisterDll(pDllName,pWDllName);
	glRegisteredComDlls.push_back(pDll);
}
catch(int nErrorNo)
{
	delete pDll;
	RaiseError(nErrorNo);
}
}

void _fastcall CoUnregisterComDll(ParamBlk *parm)
{
	FoxString pDllName(p1);
	HMODULE hDll = GetModuleHandle(pDllName);

	RegisteredComDll *pDll = 0;
	list<RegisteredComDll*>::iterator iDllIterator;

	for (iDllIterator = glRegisteredComDlls.begin(); 
		iDllIterator != glRegisteredComDlls.end(); 
		iDllIterator++)
	{
		pDll = *iDllIterator;
		if (pDll->CompareHandle(hDll))
			break;
		pDll = 0;
	}
	
	if (pDll)
	{
		glRegisteredComDlls.erase(iDllIterator);
		delete pDll;
	}
}

RegisteredComDll::~RegisteredComDll()
{
	vector<DWORD>::iterator iCoClassIterator;
	for (iCoClassIterator = m_CoClasses.begin();
		iCoClassIterator != m_CoClasses.end();
		iCoClassIterator++)
	{
		 CoRevokeClassObject(*iCoClassIterator);
	}
}

bool RegisteredComDll::CompareHandle(HMODULE hDll)
{
	return m_hDll == hDll;
}

void RegisteredComDll::RegisterDll(const char *pDllName, wchar_t *pWDllName)
{
	PDLLGETCLASSOBJECT fpDllGetClassObject = 0;
	DWORD hToken;

	ComPtr<ITypeLib*> pTypeLib;
	ComPtr<ITypeInfo*> pTypeInfo;
	ComPtr<IUnknown*> pUnk;

	m_hDll = GetModuleHandle(pDllName);
	if (!m_hDll)
	{
		m_hDll = CoLoadLibrary(pWDllName,TRUE);
		if (!m_hDll)
		{
			SaveWin32Error("LoadLibrary,GetLastError());
			throw E_APIERROR;
		}
	}

	fpDllGetClassObject = (PDLLGETCLASSOBJECT)GetProcAddress(m_hDll,"DllGetClassObject");
	if (!fpDllGetClassObject)
	{
		SaveCustomError("CoRegisterComDll","Function 'DllGetClassObject' not found in dll, it seems not to be a COM dll.");
		throw E_APIERROR;
	}

	HRESULT hr;
	hr = LoadTypeLibEx(pWDllName,REGKIND_NONE,pTypeLib);
	if (FAILED(hr))
	{
		SaveWin32Error("LoadTypeLibEx,hr);
		throw E_APIERROR;
	}

	unsigned int nTypeCount = pTypeLib->GetTypeInfoCount();
	TYPEATTR *pTypeAttr = 0;
	TYPEKIND nKind;

	for (unsigned int xj = 0; xj < nTypeCount; xj++)
	{
		hr = pTypeLib->GetTypeInfoType(xj,&nKind);
		if (FAILED(hr))
		{
			SaveCustomError("CoRegisterComDll, ITypeLib.GetTypeInfoType", "Error: %I", hr);
			throw E_APIERROR;
		}

		if (nKind == TKIND_COCLASS)
		{
			hr = pTypeLib->GetTypeInfo(xj,pTypeInfo);
			if (FAILED(hr))
			{
				SaveCustomError("CoRegisterComDll, ITypeLib.GetTypeInfo", "Error: %I", hr);
				throw E_APIERROR;
			}

			hr = pTypeInfo->GetTypeAttr(&pTypeAttr);
			if (FAILED(hr))
			{
				SaveCustomError("CoRegisterComDll, ITypeInfo.GetTypeAttr", "Error: %I", hr);
				throw E_APIERROR;
			}

			if (pTypeAttr->wTypeFlags == TYPEFLAG_FAPPOBJECT ||
				pTypeAttr->wTypeFlags == TYPEFLAG_FCANCREATE ||
				pTypeAttr->wTypeFlags == TYPEFLAG_FCONTROL ||
				pTypeAttr->wTypeFlags == TYPEFLAG_FDUAL ||
				pTypeAttr->wTypeFlags == TYPEFLAG_FOLEAUTOMATION)
			{
				hr = fpDllGetClassObject(pTypeAttr->guid,IID_IUnknown,(void**)(IUnknown**)pUnk);
				if (SUCCEEDED(hr))
				{
					hr = CoRegisterClassObject(pTypeAttr->guid,pUnk,CLSCTX_INPROC_SERVER,REGCLS_MULTIPLEUSE,&hToken);
					if (FAILED(hr))
					{
						SaveWin32Error("CoRegisterClassObject,hr);
						throw E_APIERROR;
					}
					m_CoClasses.push_back(hToken);
					pUnk = 0;
				}
			}
			pTypeInfo->ReleaseTypeAttr(pTypeAttr);
			pTypeAttr = 0;
			pTypeInfo = 0;
		}
	}
}
*/

/*
void _fastcall IDispatch_Invoke(ParamBlk *parm)
{
try
{
	FoxWString pMethod(p2);
	FoxArray pParmTypes(parm, 3, '0');
	LCID nLocale = PCount() >= 4 && Vartype(p4) == 'N' && p4.ev_long ? (LCID)p4.ev_long : LOCALE_USER_DEFAULT;

	IDispatch *pDisp;

	if (Vartype(p1) == 'I' || Vartype(p1) == 'N')
		pDisp = (IDispatch*)p1.ev_long;
	else if (Vartype(p1) == 'C')
	{
		FoxString pObject(p1);
		if (pObject.Len() > VFP2C_MAX_CALLBACKFUNCTION)
			throw E_INVALIDPARAMS;
		
		char aCommand[VFP2C_MAX_CALLBACKBUFFER];
		sprintfex(aCommand,"INT(SYS(3095,%S))",(char*)pObject);

		FoxValue vDisp;
		vDisp.Evaluate(aCommand);
		pDisp = reinterpret_cast<IDispatch*>(vDisp->ev_long);
	}
	else
		throw E_INVALIDPARAMS;

	ComCall pCall;
	pCall.SetCallInfo(pDisp, pMethod.Detach(), nLocale);

	unsigned int nParmCount = PCount() - 4;
	pCall.SetParameterCount(nParmCount);

	if (nParmCount)
	{
		unsigned int nParmTypeCount, nDims;
		if (pParmTypes)
			nParmTypeCount = pParmTypes.ALen(nDims);
		else
			nParmTypeCount = 0;

		FoxValue sParmType;
		FoxValue sParmBase;
		VARTYPE vType = VT_EMPTY;
		int nBase = 1;

		for (unsigned int xj = 1; xj <= nParmCount; xj++)
		{
			
			if (nParmTypeCount >= xj)
			{
				sParmType = pParmTypes(xj,1);

				if (sParmType.Vartype() == 'N')
					vType = (VARTYPE)(sParmType->ev_long);
				else
					vType = VT_EMPTY;

				if (nDims == 2)
				{
					sParmBase = pParmTypes(xj,2);
					if (sParmBase.Vartype() == 'N')
						nBase = sParmBase->ev_long;
					else
						nBase = 1;
				}
			}
			else
			{
				vType = VT_EMPTY;
				nBase = 1;
			}

			pCall.MarshalParameter(xj, parm->p[xj+VFP2C_COMCALL_PARMOFFSET], vType, nBase);
		}
	}

	Value vRetVal;
	pCall.CallMethod(vRetVal, parm);

	Return(vRetVal);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}
*/

/*
void _fastcall IDispatch_AsyncInvoke(ParamBlk *parm)
{
	ComCall *pCall = 0;
try
{
	FoxWString pComClass(p1);
	FoxWString pMethod(p2);
	FoxArray pParameters(parm,3,'0');
	FoxString pResultObject(p4);
	FoxWString pResultMethod(p5);
	LCID nLocale = PCount() >= 6 && p6.ev_long ? (LCID)p6.ev_long : LOCALE_USER_DEFAULT;
	DWORD dwContext = PCount() == 7 ? p7.ev_long : CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER;

	FoxValue vUnk;	
	IUnknown *pUnk;
	char aCommand[VFP2C_MAX_CALLBACKBUFFER];

	if (pResultObject.Len() > VFP2C_MAX_CALLBACKFUNCTION)
		throw E_INVALIDPARAMS;

	sprintfex(aCommand,"INT(SYS(3095,%S))",(char*)pResultObject);
	vUnk.Evaluate(aCommand);
	pUnk = reinterpret_cast<IUnknown*>(vUnk->ev_long);

	pCall = new ComCall;
	if (!pCall)
		throw E_INSUFMEMORY;

	pCall->SetCallInfo(pComClass,pMethod.Detach(),dwContext,nLocale);
	pCall->SetResultInfo(pUnk,pResultMethod.Detach());

	if (pParameters)
	{
		unsigned int nParams, nDims;
		nParams = pParameters.ALen(nDims);
		pCall->SetParameterCount(nParams);

		FoxValue sParam;
		FoxString pType;
		VARTYPE vType = VT_EMPTY;
		int nBase = 1;

		for (unsigned int xj = 1; xj <= nParams; xj++)
		{
			if (nDims == 2)
			{
				pType = pParameters(xj,2);
				if (pType.Len() == 0)
					vType = VT_EMPTY;
				else if (pType.Len() == 1)
				{
					vType = pType[0];
					nBase = 1;
				}
				else if (pType.Len() == 2)
				{
					memcpy(&vType,pType,sizeof(VARTYPE));
					nBase = 1;
				}
				else if (pType.Len() == 3)
				{
					memcpy(&vType,pType,sizeof(VARTYPE));
					nBase = pType[2];
				}
				else
					throw E_INVALIDPARAMS;
			}
			sParam = pParameters(xj,1);
			//pCall->MarshalParameter(xj, sParam, vType, nBase);
		}
	}
	else
		pCall->SetParameterCount(0);

	DWORD dwThreadId;
	HANDLE hThread = CreateThread(0,8192,IDispatch_AsyncInvokeThreadProc,(LPVOID)pCall,
			STACK_SIZE_PARAM_IS_A_RESERVATION,&dwThreadId);

	if (hThread == NULL) 
	{
		SaveWin32Error("CreateThread", GetLastError());
		throw E_APIERROR;
	}
	CloseHandle(hThread);
}
catch(int nErrorNo)
{
	if (pCall)
		delete pCall;
	RaiseError(nErrorNo);
}
}

DWORD _stdcall IDispatch_AsyncInvokeThreadProc(LPVOID lpParam)
{
	ComCall *pCall = (ComCall*)lpParam;
	HRESULT hInit = -1;
try
{
	//initialize COM
	hInit = CoInitializeEx(0,COINIT_MULTITHREADED);
	if (FAILED(hInit))
		throw E_APIERROR;
	
	pCall->CallMethodAsync();

	delete pCall;
	CoUninitialize();
	return 0;
}
catch(int nErrorNo)
{
	delete pCall;
	if (!FAILED(hInit))
		CoUninitialize();
	return nErrorNo;
}
}
*/