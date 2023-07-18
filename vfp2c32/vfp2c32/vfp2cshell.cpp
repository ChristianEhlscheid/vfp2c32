#include "vfp2c32.h"
#include "vfp2cshell.h"
#include "vfp2cfile.h"

#include <propidl.h>
#include <propkey.h>
#include <propvarutil.h>

// dynamic function pointers for runtime linking
static PGETSPECIALFOLDER fpGetSpecialFolder = 0;
static PSHILCREATEFROMPATH fpSHILCreateFromPath = 0;
static PSHILCREATEFROMPATHEX fpSHILCreateFromPathEx = 0;
static PSHCREATEITEMFROMPARSINGNAME fpSHCreateItemFromParsingName = 0;

void _fastcall SHSpecialFolder(ParamBlkEx& parm)
{
	try
	{
		if (fpGetSpecialFolder == 0)
		{
			HMODULE hDll;
			hDll = GetModuleHandle("shell32.dll");
			if (hDll)
			{
				fpGetSpecialFolder = (PGETSPECIALFOLDER)GetProcAddress(hDll, "SHGetSpecialFolderPathA");
				if (fpGetSpecialFolder == 0)
					throw E_NOENTRYPOINT;
			}
			else
			{
				SaveWin32Error("GetModuleHandle", GetLastError());
				throw E_APIERROR;
			}
		}

		FoxString pFolder(MAX_PATH);
		LocatorEx& pRef = parm(2);
		BOOL bCreateDir = parm.PCount() >= 3 ? parm(3)->ev_length : FALSE;

		if (fpGetSpecialFolder(WTopHwnd(), pFolder, parm(1)->ev_long, bCreateDir))
		{
			pFolder.Len(strlen(pFolder));
			pRef = pFolder;
			Return(true);
		}
		else
			Return(false);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

void _fastcall SHCopyFiles(ParamBlkEx& parm)
{
	try
	{
		FoxString pFrom(parm(1), 2);
		FoxString pTo(parm(2), 2);
		FoxString pTitle(parm, 4);

		SHFILEOPSTRUCT sFileOp = { 0 };

		sFileOp.wFunc = FO_COPY;
		sFileOp.fFlags = (FILEOP_FLAGS)parm(3)->ev_long;
		sFileOp.hwnd = WTopHwnd();

		sFileOp.pFrom = pFrom;
		sFileOp.pTo = pTo;

		if (pTitle.Len())
		{
			sFileOp.fFlags |= FOF_SIMPLEPROGRESS;
			sFileOp.lpszProgressTitle = pTitle;
		}

		int nRet = SHFileOperation(&sFileOp);
		if (nRet == 0 && sFileOp.fAnyOperationsAborted == FALSE)
			Return(1);
		else if (sFileOp.fAnyOperationsAborted)
			Return(0);
		else
			Return(nRet);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

void _fastcall SHDeleteFiles(ParamBlkEx& parm)
{
	try
	{
		FoxString pFile(parm(1), 2);
		FoxString pTitle(parm, 3);

		SHFILEOPSTRUCT sFileOp = { 0 };

		sFileOp.wFunc = FO_DELETE;
		sFileOp.fFlags = (FILEOP_FLAGS)parm(2)->ev_long;
		sFileOp.hwnd = WTopHwnd();

		sFileOp.pFrom = pFile;

		if (pTitle.Len())
		{
			sFileOp.fFlags |= FOF_SIMPLEPROGRESS;
			sFileOp.lpszProgressTitle = pTitle;
		}

		int nRet = SHFileOperation(&sFileOp);
		if (nRet == 0 && sFileOp.fAnyOperationsAborted == FALSE)
			Return(1);
		else if (sFileOp.fAnyOperationsAborted)
			Return(0);
		else
			Return(nRet);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

void _fastcall SHMoveFiles(ParamBlkEx& parm)
{
	try
	{
		FoxString pFrom(parm(1), 2);
		FoxString pTo(parm(2), 2);
		FoxString pTitle(parm, 4);

		SHFILEOPSTRUCT sFileOp = { 0 };

		sFileOp.wFunc = FO_MOVE;
		sFileOp.fFlags = (FILEOP_FLAGS)parm(3)->ev_long;
		sFileOp.hwnd = WTopHwnd();

		sFileOp.pFrom = pFrom;
		sFileOp.pTo = pTo;

		if (pTitle.Len())
		{
			sFileOp.fFlags |= FOF_SIMPLEPROGRESS;
			sFileOp.lpszProgressTitle = pTitle;
		}

		int nRet = SHFileOperation(&sFileOp);
		if (nRet == 0 && sFileOp.fAnyOperationsAborted == FALSE)
			Return(1);
		else if (sFileOp.fAnyOperationsAborted)
			Return(0);
		else
			Return(nRet);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

void _fastcall SHRenameFiles(ParamBlkEx& parm)
{
	try
	{
		FoxString pFrom(parm(1), 2);
		FoxString pTo(parm(2), 2);
		FoxString pTitle(parm, 4);

		SHFILEOPSTRUCT sFileOp = { 0 };

		sFileOp.wFunc = FO_RENAME;
		sFileOp.fFlags = (FILEOP_FLAGS)parm(3)->ev_long;
		sFileOp.hwnd = WTopHwnd();

		sFileOp.pFrom = parm(1)->HandleToPtr();
		sFileOp.pTo = parm(2)->HandleToPtr();

		if (pTitle.Len())
		{
			sFileOp.fFlags |= FOF_SIMPLEPROGRESS;
			sFileOp.lpszProgressTitle = pTitle;
		}

		int nRet = SHFileOperation(&sFileOp);
		if (nRet == 0 && sFileOp.fAnyOperationsAborted == FALSE)
			Return(1);
		else if (sFileOp.fAnyOperationsAborted)
			Return(0);
		else
			Return(nRet);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

void _fastcall SHBrowseFolder(ParamBlkEx& parm)
{
	try
	{
		FoxString pTitle(parm(1));
		FoxWString<MAX_PATH> pRootFolder(parm, 4);
		FoxString pCallback(parm, 5);
		FoxString pFolder(MAX_PATH);
		CoTaskPtr pIdl, pRootIdl;

		BROWSEINFO sBrow;
		CFoxCallback sCallback;
		char aDisplayName[MAX_PATH];
		HRESULT hr;
		DWORD nRootAttr = 0;

		if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
		{
			SaveCustomError("SHBrowseFolder", "Callback function length is greater than maximum length of 1024.");
			throw E_INVALIDPARAMS;
		}

		if (pRootFolder)
		{
			if (fpSHILCreateFromPath == 0 && fpSHILCreateFromPathEx == 0)
			{
				HMODULE hDll;
				hDll = GetModuleHandle("shell32.dll");
				if (hDll)
				{
					fpSHILCreateFromPath = (PSHILCREATEFROMPATH)GetProcAddress(hDll, "SHILCreateFromPath");
					fpSHILCreateFromPathEx = (PSHILCREATEFROMPATHEX)GetProcAddress(hDll, (LPCSTR)SHILCREATEFROMPATHEXID);
					if (fpSHILCreateFromPath == 0 && fpSHILCreateFromPathEx == 0)
						throw E_NOENTRYPOINT;
				}
				else
				{
					SaveWin32Error("GetModuleHandle", GetLastError());
					throw E_APIERROR;
				}
			}

			if (fpSHILCreateFromPath)
			{
				hr = fpSHILCreateFromPath(pRootFolder, pRootIdl, &nRootAttr);
				if (FAILED(hr))
				{
					SaveCustomError("SHILCreateFromPath", "Function failed. HRESULT: %I", hr);
					throw E_APIERROR;
				}
			}
			else
				pRootIdl = fpSHILCreateFromPathEx(pRootFolder);

			sBrow.pidlRoot = pRootIdl;
		}
		else
			sBrow.pidlRoot = 0;

		if (pCallback.Len())
			sCallback.SetCallback(pCallback);

		sBrow.lpfn = pCallback.Len() ? SHBrowseCallback : 0;
		sBrow.lParam = pCallback.Len() ? reinterpret_cast<LPARAM>(&sCallback) : 0;
		sBrow.iImage = 0;
		sBrow.hwndOwner = WTopHwnd();
		sBrow.pszDisplayName = aDisplayName;
		sBrow.lpszTitle = pTitle;
		sBrow.ulFlags = parm(2)->ev_long;

		pIdl = SHBrowseForFolder(&sBrow);

		if (pIdl)
		{
			if (!SHGetPathFromIDList(pIdl, pFolder))
			{
				SaveCustomError("SHGetPathFromIDList", "Function failed.");
				throw E_APIERROR;
			}
			LocatorEx& pRef = parm(3);
			pRef = pFolder.StringLen();
			Return(true);
		}
		else
			Return(false);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

int _stdcall SHBrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	int nRetVal = 0;
	FoxValue vRetVal;
	CFoxCallback* pCallback = reinterpret_cast<CFoxCallback*>(lpData);
	int nErrorNo = pCallback->Evaluate(vRetVal, hwnd, uMsg, lParam);
	if (nErrorNo == 0)
	{
		if (vRetVal.Vartype() == 'I')
			nRetVal = vRetVal->ev_long;
		else if (vRetVal.Vartype() == 'L')
			nRetVal = vRetVal->ev_length;
		else if (vRetVal.Vartype() == 'N')
			nRetVal = static_cast<int>(vRetVal->ev_real);
	}
	return nRetVal;
}

void _fastcall SHGetShellItem(ParamBlkEx& parm)
{
	try
	{
		if (fpSHCreateItemFromParsingName == 0)
		{
			HMODULE hDll;
			hDll = GetModuleHandle("shell32.dll");
			if (hDll)
			{
				fpSHCreateItemFromParsingName = (PSHCREATEITEMFROMPARSINGNAME)GetProcAddress(hDll, "SHCreateItemFromParsingName");
				if (fpSHCreateItemFromParsingName == 0)
					throw E_NOENTRYPOINT;
			}
			else
			{
				SaveWin32Error("GetModuleHandle", GetLastError());
				throw E_APIERROR;
			}
		}

		FoxWString<MAX_WIDE_PATH> pFileName(parm(1));
		IShellItem2* pShellItem;
		HRESULT hr = fpSHCreateItemFromParsingName(pFileName, 0, IID_IShellItem2, (void**)&pShellItem);
		if (FAILED(hr))
		{
			SaveCustomError("SHCreateItemFromParsingName", "Function failed. HRESULT: %I", hr);
			throw E_APIERROR;
		}

		CShellItem* pItem = new CShellItem(pShellItem);
		ReturnIDispatch(pItem);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

CPropertyDescription::CPropertyDescription(IPropertyDescription* pDesc)
{
	m_Desc = pDesc;
}

STDMETHODIMP CPropertyDescription::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
	HRESULT hr = S_OK;
	if (_wcsicmp(L"GetCanonicalName", *rgszNames) == 0)
		*rgdispid = DISPID_GetCanonicalName;
	else if (_wcsicmp(L"GetPropertyType", *rgszNames) == 0)
		*rgdispid = DISPID_GetPropertyType;
	else if (_wcsicmp(L"GetDisplayName", *rgszNames) == 0)
		*rgdispid = DISPID_GetDisplayName;
	else
		hr = DISP_E_UNKNOWNNAME;
	return hr;
}

STDMETHODIMP CPropertyDescription::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
	DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	HRESULT hr;
	switch (dispidMember)
	{
	case DISPID_GetCanonicalName:
		if (pdispparams->cArgs == 0)
		{
			LPWSTR pName;
			hr = m_Desc->GetCanonicalName(&pName);
			if (SUCCEEDED(hr))
			{
				pvarResult->vt = VT_BSTR;
				pvarResult->bstrVal = SysAllocString(pName);
				CoTaskMemFree(pName);
			}
		}
		else
			hr = DISP_E_BADPARAMCOUNT;
		break;

	case DISPID_GetPropertyType:
		if (pdispparams->cArgs == 0)
		{
			VARTYPE vt;
			hr = m_Desc->GetPropertyType(&vt);
			if (SUCCEEDED(hr))
			{
				pvarResult->vt = VT_I4;
				pvarResult->lVal = (LONG)vt;
			}
		}
		else
			hr = DISP_E_BADPARAMCOUNT;
		break;

	case DISPID_GetDisplayName:
		if (pdispparams->cArgs == 0)
		{
			LPWSTR pName;
			hr = m_Desc->GetDisplayName(&pName);
			if (SUCCEEDED(hr))
			{
				pvarResult->vt = VT_BSTR;
				pvarResult->bstrVal = SysAllocString(pName);
				CoTaskMemFree(pName);
			}
		}
		else
			hr = DISP_E_BADPARAMCOUNT;
		break;
	}
	return hr;
}

CPropertyDescriptionList::CPropertyDescriptionList(IPropertyDescriptionList* pList)
{
	m_List = pList;
}

STDMETHODIMP CPropertyDescriptionList::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
	HRESULT hr = S_OK;
	if (_wcsicmp(L"GetAt", *rgszNames) == 0)
		*rgdispid = DISPID_GetAt;
	else if (_wcsicmp(L"GetCount", *rgszNames) == 0)
		*rgdispid = DISPID_GetCount;
	else
		hr = DISP_E_UNKNOWNNAME;
	return hr;
}

STDMETHODIMP CPropertyDescriptionList::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
	DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	HRESULT hr;
	switch (dispidMember)
	{
	case DISPID_GetAt:
		if (pdispparams->cArgs != 1)
			hr = DISP_E_BADPARAMCOUNT;
		else if (pdispparams->rgvarg[0].vt != VT_I4)
			hr = DISP_E_TYPEMISMATCH;
		else
		{
			IPropertyDescription* pPD;
			hr = m_List->GetAt(pdispparams->rgvarg[0].lVal, IID_IPropertyDescription, (void**)&pPD);
			if (SUCCEEDED(hr))
			{
				pvarResult->vt = VT_DISPATCH;
				pvarResult->pdispVal = new CPropertyDescription(pPD);
			}
		}
		break;

	case DISPID_GetCount:
		if (pdispparams->cArgs == 0)
		{
			hr = m_List->GetCount(&pvarResult->uintVal);
			if (SUCCEEDED(hr))
			{
				pvarResult->vt = VT_I4;
				pvarResult->lVal = (long)pvarResult->uintVal;
			}
		}
		else
			hr = DISP_E_BADPARAMCOUNT;
		break;

	case DISPID_NEWENUM:
		pvarResult->vt = VT_UNKNOWN;
		pvarResult->punkVal = new CPropertyDescriptionListEnumerator(m_List);
		break;
	}
	return hr;
}

CPropertyDescriptionListEnumerator::CPropertyDescriptionListEnumerator(IPropertyDescriptionList* pList) : m_RefCount(2)
{
	m_List = pList;
	m_Index = 0;
	pList->AddRef();
}

CPropertyDescriptionListEnumerator::CPropertyDescriptionListEnumerator(CPropertyDescriptionListEnumerator& pEnum) : m_RefCount(2)
{
	m_List = pEnum.m_List;
	m_Index = pEnum.m_Index;
	pEnum.m_List->AddRef();
}

STDMETHODIMP CPropertyDescriptionListEnumerator::QueryInterface(REFIID riid, void** ppvObject)
{
	*ppvObject = NULL;
	HRESULT hr;

	// IUnknown
	if (::IsEqualIID(riid, __uuidof(IUnknown)))
	{
		*ppvObject = this;
		hr = S_OK;
	}
	else if (::IsEqualIID(riid, __uuidof(IEnumVARIANT)))
	{
		*ppvObject = this;
		hr = S_OK;
	}
	else
		hr = E_NOINTERFACE;

	return hr;
}

STDMETHODIMP_(ULONG) CPropertyDescriptionListEnumerator::AddRef()
{
	return ++m_RefCount;
}

STDMETHODIMP_(ULONG) CPropertyDescriptionListEnumerator::Release()
{
	m_RefCount--;
	if (m_RefCount == 0)
	{
		delete this;
		return 0;
	}
	else
		return m_RefCount;
}

STDMETHODIMP CPropertyDescriptionListEnumerator::Next(ULONG celt, VARIANT* rgVar, ULONG* pCeltFetched)
{
	HRESULT hr = S_OK;
	IPropertyDescription* pPD;
	for (ULONG xj = 0; xj < celt; xj++)
	{
		hr = m_List->GetAt(m_Index++, IID_IPropertyDescription, (void**)&pPD);
		if (SUCCEEDED(hr))
		{
			rgVar[xj].vt = VT_DISPATCH;
			rgVar[xj].pdispVal = new CPropertyDescription(pPD);
		}
		else
		{
			for (ULONG xi = 0; xi = xj; xi++)
				VariantClear(&rgVar[xi]);
			if (hr == TYPE_E_OUTOFBOUNDS)
				hr = S_FALSE;
			break;
		}
	}
	*pCeltFetched = SUCCEEDED(hr) ? celt : 0;
	return hr;
}

STDMETHODIMP CPropertyDescriptionListEnumerator::Skip(ULONG celt)
{
	HRESULT hr;
	UINT nCount, nIndex;
	hr = m_List->GetCount(&nCount);
	if (SUCCEEDED(hr))
	{
		nIndex = m_Index += celt;
		if (nIndex < nCount)
		{
			m_Index = nIndex;
			hr = S_OK;
		}
		else
		{
			m_Index = nCount - 1;
			hr = S_FALSE;
		}
	}
	return hr;
}

STDMETHODIMP CPropertyDescriptionListEnumerator::Reset()
{
	m_Index = 0;
	return S_OK;
}

STDMETHODIMP CPropertyDescriptionListEnumerator::Clone(IEnumVARIANT** ppEnum)
{
	*ppEnum = new CPropertyDescriptionListEnumerator(*this);
	return S_OK;
}

CPropertyStore::CPropertyStore(IPropertyStore* pStore)
{
	m_Store = pStore;
}

STDMETHODIMP CPropertyStore::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
	HRESULT hr = S_OK;
	*rgdispid = DISPID_UNKNOWN;
	if (_wcsicmp(L"GetValue", *rgszNames) == 0)
		*rgdispid = DISPID_GetValue;
	else if (_wcsicmp(L"SetValue", *rgszNames) == 0)
		*rgdispid = DISPID_SetValue;
	else if (_wcsicmp(L"Commit", *rgszNames) == 0)
		*rgdispid = DISPID_Commit;
	else
		hr = DISP_E_UNKNOWNNAME;
	return hr;
}

STDMETHODIMP CPropertyStore::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
	DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	HRESULT hr;
	switch (dispidMember)
	{
	case DISPID_GetValue:
		if (pdispparams->cArgs != 1)
			hr = DISP_E_BADPARAMCOUNT;
		else if (pdispparams->rgvarg[0].vt != VT_BSTR)
			hr = DISP_E_TYPEMISMATCH;
		else
		{
			CPropVariant pProp;
			PROPERTYKEY pKey;
			PCWSTR pProperty = pdispparams->rgvarg[0].bstrVal ? pdispparams->rgvarg[0].bstrVal : 0;
			hr = PSGetPropertyKeyFromName(pProperty, &pKey);
			if (SUCCEEDED(hr))
			{
				hr = m_Store->GetValue(pKey, &pProp);
				if (SUCCEEDED(hr))
				{
					hr = PropVariantToVariant(&pProp, pvarResult);
					// VT_UI8 cannot be marshaled back to FoxPro, convert it to a VT_DECIMAL instead
					if (SUCCEEDED(hr) && pvarResult->vt == VT_UI8)
					{
						pvarResult->vt = VT_DECIMAL;
						pvarResult->decVal.Lo64 = pvarResult->ullVal;
						pvarResult->decVal.Hi32 = 0;
						pvarResult->decVal.scale = 0;
						pvarResult->decVal.sign = 0;
					}
				}
			}
		}
		break;

	case DISPID_SetValue:
		if (pdispparams->cArgs != 2)
			hr = DISP_E_BADPARAMCOUNT;
		else if (pdispparams->rgvarg[1].vt != VT_BSTR)
			hr = DISP_E_TYPEMISMATCH;
		else
		{
			CPropVariant pProp;
			PROPERTYKEY pKey;
			PCWSTR pProperty = pdispparams->rgvarg[1].bstrVal ? pdispparams->rgvarg[1].bstrVal : 0;
			hr = PSGetPropertyKeyFromName(pProperty, &pKey);
			if (SUCCEEDED(hr))
			{
				hr = VariantToPropVariant(&pdispparams->rgvarg[0], &pProp);
				if (SUCCEEDED(hr))
					hr = m_Store->SetValue(pKey, pProp);
			}
		}
		break;

	case DISPID_Commit:
		if (pdispparams->cArgs == 0)
			hr = m_Store->Commit();
		else
			hr = DISP_E_BADPARAMCOUNT;
	}
	return hr;
}


CShellItem::CShellItem(IShellItem2* pShellItem)
{
	m_ShellItem = pShellItem;
}

STDMETHODIMP CShellItem::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
	HRESULT hr = S_OK;
	if (_wcsicmp(L"GetProperty", *rgszNames) == 0)
		*rgdispid = DISPID_GetProperty;
	else if (_wcsicmp(L"GetPropertyDescriptionList", *rgszNames) == 0)
		*rgdispid = DISPID_GetPropertyDescriptionList;
	else if (_wcsicmp(L"GetPropertyStore", *rgszNames) == 0)
		*rgdispid = DISPID_GetPropertyStore;
	else
		hr = DISP_E_UNKNOWNNAME;
	return hr;
}

STDMETHODIMP CShellItem::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
	DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	HRESULT hr;
	PROPERTYKEY pKey;
	switch (dispidMember)
	{
	case DISPID_GetProperty:
		if (pdispparams->cArgs == 1)
		{
			CPropVariant pProp;
			PCWSTR pProperty = pdispparams->rgvarg[0].bstrVal ? pdispparams->rgvarg[0].bstrVal : 0;
			hr = PSGetPropertyKeyFromName(pProperty, &pKey);
			if (SUCCEEDED(hr))
			{
				hr = m_ShellItem->GetProperty(pKey, &pProp);
				if (SUCCEEDED(hr))
				{
					hr = PropVariantToVariant(&pProp, pvarResult);
					// VT_UI8 cannot be marshaled back to FoxPro, convert it to a VT_DECIMAL instead
					if (SUCCEEDED(hr) && pvarResult->vt == VT_UI8)
					{
						pvarResult->vt = VT_DECIMAL;
						pvarResult->decVal.Lo64 = pvarResult->ullVal;
						pvarResult->decVal.Hi32 = 0;
						pvarResult->decVal.scale = 0;
						pvarResult->decVal.sign = 0;
					}
				}
			}
		}
		else
			hr = DISP_E_BADPARAMCOUNT;
		break;

	case DISPID_GetPropertyDescriptionList:
		if (pdispparams->cArgs == 1)
		{
			PCWSTR pProperty = pdispparams->rgvarg[0].bstrVal ? pdispparams->rgvarg[0].bstrVal : 0;
			hr = PSGetPropertyKeyFromName(pProperty, &pKey);
		}
		else if (pdispparams->cArgs == 0)
		{
			pKey = PKEY_PropList_FullDetails;
			hr = S_OK;
		}
		else
			hr = DISP_E_BADPARAMCOUNT;
		if (SUCCEEDED(hr))
		{
			IPropertyDescriptionList* pPropList;
			hr = m_ShellItem->GetPropertyDescriptionList(pKey, IID_IPropertyDescriptionList, (void**)&pPropList);
			if (SUCCEEDED(hr))
			{
				pvarResult->vt = VT_DISPATCH;
				pvarResult->pdispVal = new CPropertyDescriptionList(pPropList);
			}
		}
		break;

	case DISPID_GetPropertyStore:
		if (pdispparams->cArgs == 0)
		{
			IPropertyStore* pPropStore;
			GETPROPERTYSTOREFLAGS nFlags = GPS_DEFAULT;
			if (pdispparams->cArgs == 1 && pdispparams->rgvarg[0].vt == VT_I4)
				nFlags = (GETPROPERTYSTOREFLAGS)pdispparams->rgvarg[0].lVal;
			hr = m_ShellItem->GetPropertyStore(nFlags, IID_IPropertyStore, (void**)&pPropStore);
			if (SUCCEEDED(hr))
			{
				pvarResult->vt = VT_DISPATCH;
				pvarResult->pdispVal = new CPropertyStore(pPropStore);
			}
		}
		else
			hr = DISP_E_BADPARAMCOUNT;
		break;
	}
	return hr;
}