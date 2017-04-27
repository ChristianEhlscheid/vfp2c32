#define _WIN32_DCOM

#include <windows.h>
#include <atlbase.h>
#include <atlsafe.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2ccom.h"
#include "vfp2casynccom.h"
#include "vfp2cutil.h"
#include "vfpmacros.h"
#include "vfp2ccppapi.h"

CComCallInfo::CComCallInfo() 
{
	m_DispParams.rgvarg = 0;
	m_DispParams.rgdispidNamedArgs = 0;
	m_DispParams.cArgs = 0;
	m_DispParams.cNamedArgs = 0;
	m_AbortEvent = 0;
	m_Aborted = false;
	m_StartTime.QuadPart = 0;
}

CComCallInfo::~CComCallInfo()
{
	if (m_DispParams.rgvarg)
	{
		for (unsigned int xj = 0; xj < m_DispParams.cArgs; xj++)
		{
			if (m_DispParams.rgvarg[xj].vt == VT_STREAM)
			{
				if (m_DispParams.rgvarg[xj].punkVal)
					m_DispParams.rgvarg[xj].punkVal->Release();
			}
			else
				VariantClear(&m_DispParams.rgvarg[xj]);
		}
		delete m_DispParams.rgvarg;
	}
	if (m_Aborted)
		ResetEvent(m_AbortEvent);
}

STDMETHODIMP CComCallInfo::QueryInterface(REFIID riid, void **ppvObject)
{
	*ppvObject = NULL;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CComCallInfo::AddRef()
{
	return 1;
}

STDMETHODIMP_(ULONG) CComCallInfo::Release()
{
	return 1;
}

STDMETHODIMP CComCallInfo::GetTypeInfoCount(UINT* pctinfo)
{
	*pctinfo = 0; 
	return E_NOTIMPL;
}

STDMETHODIMP CComCallInfo::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
	if (pptinfo)
        *pptinfo = 0;
	return E_NOTIMPL;
}

STDMETHODIMP CComCallInfo::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
	HRESULT hr = DISP_E_UNKNOWNNAME;
	*rgdispid = DISPID_UNKNOWN;
	if (wcsicmp(L"AbortEvent", *rgszNames) == 0)
	{
		*rgdispid = DISPID_AbortEvent;
		hr = S_OK;
	}
	else if (wcsicmp(L"Aborted", *rgszNames) == 0)
	{
		*rgdispid = DISPID_Aborted;
		hr = S_OK;
	}
	else if (wcsicmp(L"CallContext", *rgszNames) == 0)
	{
		*rgdispid = DISPID_CallContext;
		hr = S_OK;
	}
	else if (wcsicmp(L"CallId", *rgszNames) == 0)
	{
		*rgdispid = DISPID_CallId;
		hr = S_OK;
	}
	return hr;
}

STDMETHODIMP CComCallInfo::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, 
										DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	HRESULT hr;

	switch(dispidMember)
	{
		case DISPID_AbortEvent:
			if (wFlags & DISPATCH_PROPERTYGET)
			{
				if (pvarResult)
				{
					pvarResult->vt = VT_I4;
					pvarResult->lVal = (LONG)m_AbortEvent;
				}
				hr = S_OK;
			}
			else
				hr = DISP_E_MEMBERNOTFOUND;
			break;

		case DISPID_Aborted:
			hr = Aborted(pvarResult);
			break;

		case DISPID_CallContext:
			if (wFlags & DISPATCH_PROPERTYGET)
			{
				switch(pdispparams->cArgs)
				{
					case 0:
						if (pvarResult)
							hr = VariantCopy(pvarResult, &m_CallContext);
						else
							hr = S_OK;
						break;

					case 1:
					{
						VARTYPE vt = pdispparams->rgvarg[0].vt;
						if ((vt & VT_ARRAY) > 0 || (vt & VT_BYREF) > 0 || vt == VT_DISPATCH)
							return DISP_E_TYPEMISMATCH;
						hr = m_CallContext.Copy(&pdispparams->rgvarg[0]);
						if (SUCCEEDED(hr) && pvarResult)
						{
							pvarResult->vt = VT_BOOL;
							pvarResult->boolVal = VARIANT_TRUE;
						}
						break;
					}

					default:
						return DISP_E_BADPARAMCOUNT;
				}
			}
			else if (wFlags & DISPATCH_PROPERTYPUT)
			{
				VARTYPE vt = pdispparams->rgvarg[0].vt;
				if ((vt & VT_ARRAY) > 0 || (vt & VT_BYREF) > 0 || vt == VT_DISPATCH)
					return DISP_E_TYPEMISMATCH;

				hr = m_CallContext.Copy(&pdispparams->rgvarg[0]);
				if (SUCCEEDED(hr) && pvarResult)
				{
					pvarResult->vt = VT_BOOL;
					pvarResult->boolVal = VARIANT_TRUE;
				}
			}
			else
				hr = DISP_E_MEMBERNOTFOUND;
			break;
	}

	return hr;
}

HRESULT CComCallInfo::MarshalParameters(DISPPARAMS* pdispparams)
{
	HRESULT hr = S_OK;
	m_DispParams.cArgs = pdispparams->cArgs;
	if (m_DispParams.cArgs > 0)
	{
		m_DispParams.rgvarg = new VARIANTARG[m_DispParams.cArgs];
		if (!m_DispParams.rgvarg)
			return E_OUTOFMEMORY;

		for (unsigned int xj = 0; xj < m_DispParams.cArgs; xj++)
		{
			VariantInit(&m_DispParams.rgvarg[xj]);
			if (pdispparams->rgvarg[xj].vt & VT_BYREF)
				hr = DISP_E_BADVARTYPE;
		}

		if (FAILED(hr))
			return hr;

		for (unsigned int xj = 0; xj < m_DispParams.cArgs; xj++)
		{
			if (pdispparams->rgvarg[xj].vt != VT_DISPATCH)
			{
				hr = VariantCopy(&m_DispParams.rgvarg[xj], &pdispparams->rgvarg[xj]);
				if (FAILED(hr))
					return hr;
			}
			else
			{
				m_DispParams.rgvarg[xj].vt = VT_STREAM;
				hr = CoMarshalInterThreadInterfaceInStream(IID_IDispatch, pdispparams->rgvarg[xj].pdispVal, (LPSTREAM*)&m_DispParams.rgvarg[xj].punkVal);
				if (FAILED(hr))
					return hr;
			}
		}
	}
	return hr;
}

HRESULT CComCallInfo::UnmarshalParameters()
{
	HRESULT hr = S_OK;
	for (unsigned int xj = 0; xj < m_DispParams.cArgs; xj++)
	{
		if (m_DispParams.rgvarg[xj].vt == VT_STREAM)
		{
			m_DispParams.rgvarg[xj].vt = VT_DISPATCH;
			hr = CoGetInterfaceAndReleaseStream((LPSTREAM)m_DispParams.rgvarg[xj].punkVal, IID_IDispatch, (LPVOID*)&m_DispParams.rgvarg[xj].pdispVal);
			if (FAILED(hr))
				return hr;
		}
	}
	return hr;
}

HRESULT CComCallInfo::Aborted(VARIANT *result)
{
	HRESULT hr = S_OK;
	if (result)
	{
		result->vt = VT_BOOL;
		result->boolVal = m_Aborted ? VARIANT_TRUE : VARIANT_FALSE;
	}
	return hr;
}

LONG CThreadedComObject::m_CallId = 0;

CThreadedComObject::CThreadedComObject(wchar_t *pComClass, bool bSynchronousAccess) : m_RefCount(1), m_Disp(0), m_Callback(0), m_CallbackStream(0),
															m_Callback_OnCallComplete(DISPID_UNKNOWN), m_Callback_OnError(DISPID_UNKNOWN), 
															m_DispId_CallInfo(DISPID_UNKNOWN), m_ComClass(pComClass), m_Thread(0), 
															m_AbortCallEvent(0), m_LastError(0), m_LastErrorFunction(0), 
															m_SynchronousAccess(bSynchronousAccess), m_CallQueue(20), m_ShutdownFlag(false)
{
	VariantInit(&m_CallContext);
}

CThreadedComObject::~CThreadedComObject()
{
	if (m_ShutdownEvent)
	{
		if (m_Thread)
		{
			m_ShutdownFlag = true;
			m_ShutdownEvent.Signal();
			do
			{
				DWORD ret = MsgWaitForMultipleObjects(1, &m_Thread, FALSE, INFINITE, QS_POSTMESSAGE);
				if (ret == WAIT_OBJECT_0)
					break;
				else
				{
					MSG msg;
					while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
						DispatchMessage(&msg);
				}
			}
			while(true);
		}
	}

	m_CallQueue.RemoveAll();
	m_CallContext.Clear();
}

STDMETHODIMP CThreadedComObject::QueryInterface(REFIID riid, void **ppvObject)
{
	*ppvObject = NULL;
	HRESULT hr;

	// IUnknown
	if (::IsEqualIID(riid, __uuidof(IUnknown)))
	{
		*ppvObject = this;
		hr = S_OK;
	}
	// IDispatch
	else if (::IsEqualIID(riid, __uuidof(IDispatch)))
	{
		*ppvObject = static_cast<IDispatch*>(this);
		hr = S_OK;
	}
	else
		hr = E_NOINTERFACE;

	return hr;
}

STDMETHODIMP_(ULONG) CThreadedComObject::AddRef()
{
	return ++m_RefCount;
}

STDMETHODIMP_(ULONG) CThreadedComObject::Release()
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

STDMETHODIMP CThreadedComObject::GetTypeInfoCount(UINT* pctinfo)
{
	*pctinfo = 0; 
	return S_OK;
}

STDMETHODIMP CThreadedComObject::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP CThreadedComObject::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
	HRESULT hr = DISP_E_UNKNOWNNAME;
	*rgdispid = DISPID_UNKNOWN;
	int len = ocslen(rgszNames[0]);
	for (unsigned int xj = 0; xj < m_DispMap.GetCount(); xj++)
	{
		if (m_DispMap[xj].Len == len && m_DispMap[xj].Name == rgszNames[0])
		{
			*rgdispid = m_DispMap[xj].DispId;
			hr = S_OK;
			break;
		}
	}
	return hr;
}

STDMETHODIMP CThreadedComObject::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, 
										DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	HRESULT hr;
	switch(dispidMember)
	{
		case DISPID_AbortCall:
			if (pdispparams->cArgs <= 1)
			{
				if (pdispparams->cArgs == 1)
				{
					hr = VariantChangeType(&pdispparams->rgvarg[0], &pdispparams->rgvarg[0], 0, VT_I4);
					if (SUCCEEDED(hr))
						hr = AbortCall(pdispparams->rgvarg[0].lVal, pvarResult);
				}
				else
					hr = AbortCall(0, pvarResult);
			}
			else
				hr = DISP_E_BADPARAMCOUNT;
			break;

		case DISPID_ContainedObject:
			if (wFlags & DISPATCH_PROPERTYGET)
			{
				if (pvarResult)
				{
					hr = m_Git.CopyTo(&pvarResult->pdispVal);
					if (SUCCEEDED(hr))
						pvarResult->vt = VT_DISPATCH;
				}
				else
					hr = S_OK;
			}
			else
				hr = DISP_E_MEMBERNOTFOUND;

			break;

		case DISPID_ThreadId:
			if (wFlags & DISPATCH_PROPERTYGET)
			{
				if (pvarResult)
				{
					pvarResult->vt = VT_I4;
					pvarResult->lVal = m_ThreadId;
					hr = S_OK;
				}
				else
					hr = S_OK;
			}
			else
				hr = DISP_E_MEMBERNOTFOUND;

			break;

		case DISPID_CallContext:
			if (wFlags & DISPATCH_PROPERTYGET)
			{
				switch(pdispparams->cArgs)
				{
					case 0:
						if (pvarResult)
							hr = VariantCopy(pvarResult, &m_CallContext);
						else
							hr = S_OK;
						break;

					case 1:
					{
						VARTYPE vt = pdispparams->rgvarg[0].vt;
						if ((vt & VT_ARRAY) > 0 || (vt & VT_BYREF) > 0 || vt == VT_DISPATCH)
							return DISP_E_TYPEMISMATCH;
						hr = m_CallContext.Copy(&pdispparams->rgvarg[0]);
						if (SUCCEEDED(hr) && pvarResult)
						{
							AddRef();
							pvarResult->vt = VT_DISPATCH;
							pvarResult->pdispVal = this;
						}
						break;
					}

					default:
						return DISP_E_BADPARAMCOUNT;
				}
			}
			else if (wFlags & DISPATCH_PROPERTYPUT)
			{
				VARTYPE vt = pdispparams->rgvarg[0].vt;
				if ((vt & VT_ARRAY) > 0 || (vt & VT_BYREF) > 0 || vt == VT_DISPATCH)
					return DISP_E_TYPEMISMATCH;

				hr = m_CallContext.Copy(&pdispparams->rgvarg[0]);
				if (SUCCEEDED(hr) && pvarResult)
				{
					AddRef();
					pvarResult->vt = VT_DISPATCH;
					pvarResult->pdispVal = this;
				}
			}
			else
				hr = DISP_E_MEMBERNOTFOUND;
			break;
		
		case DISPID_GetCallQueueSize:
			if (wFlags & DISPATCH_PROPERTYGET) {
				if (pvarResult)
				{
					pvarResult->vt = VT_I4;
					m_CritSect.Enter();
					pvarResult->lVal = m_CallQueue.GetCount() + (m_CurrentCall.m_p ? 1 : 0);
					m_CritSect.Leave();
					hr = S_OK;
				}
				else
					hr = S_OK;
			}
			else
				hr = DISP_E_MEMBERNOTFOUND;
			break;

		case DISPID_GetCallRunTime:
			if (wFlags & DISPATCH_PROPERTYGET) {
				if (pdispparams->cArgs == 1) {
					hr = VariantChangeType(&pdispparams->rgvarg[0], &pdispparams->rgvarg[0], 0, VT_I4);
					if (SUCCEEDED(hr))
						hr = GetCallRunTime(pdispparams->rgvarg[0].lVal, pvarResult);
				} else
					hr = DISP_E_BADPARAMCOUNT;
			}
			else
				hr = DISP_E_MEMBERNOTFOUND;
			break;

		default:
		{
			CComCallInfo* ci = new CComCallInfo();
			if (!ci)
				return E_OUTOFMEMORY;

			CAutoPtr<CComCallInfo> pCallInfo(ci);
			hr = pCallInfo->MarshalParameters(pdispparams);
			if (FAILED(hr))
				return hr;

			if (++m_CallId == 0)
				++m_CallId;

			pCallInfo->m_CallId = m_CallId;
			pCallInfo->m_DispIdMember = dispidMember;
			pCallInfo->m_Flags = wFlags;
			pCallInfo->m_Lcid = lcid;
			m_CallContext.Detach(&pCallInfo->m_CallContext);
			pCallInfo->m_AbortEvent = m_AbortCallEvent;

			hr = QueueCall(pCallInfo);
			if (FAILED(hr))
				return hr;

			if (!m_CallEvent.Signal())
				return HRESULT_FROM_WIN32(GetLastError());

			if (pvarResult)
			{
				pvarResult->vt = VT_I4;
				pvarResult->lVal = m_CallId;
			}
		}
	}
	return hr;
}

void CThreadedComObject::CreateObject(DWORD dwContext, IDispatch* pCallback, DWORD dwStackSize)
{
	HRESULT hr;
	m_CreationContext = dwContext;

	if (!m_CritSect.Initialize(4000))
		throw E_APIERROR;

	if (pCallback)
	{
		LPOLESTR pMethod = L"OnCallComplete";
		hr = pCallback->GetIDsOfNames(IID_NULL, &pMethod, 1, 1033, &m_Callback_OnCallComplete);
		if (hr != S_OK && hr != DISP_E_UNKNOWNNAME)
		{
			SaveWin32Error("IDispatch::GetIDsOfNames", hr);
			throw E_APIERROR;
		}

		pMethod = L"OnError";
		hr = pCallback->GetIDsOfNames(IID_NULL, &pMethod, 1, 1033, &m_Callback_OnError);
		if (hr != S_OK && hr != DISP_E_UNKNOWNNAME)
		{
			SaveWin32Error("IDispatch::GetIDsOfNames", hr);
			throw E_APIERROR;
		}

		if (m_Callback_OnCallComplete == DISPID_UNKNOWN && m_Callback_OnError == DISPID_UNKNOWN)
		{
			SaveCustomError("CreateThreadedObject", "The callback object does not implement 'OnCallComplete' nor 'OnError'!");
			throw E_APIERROR;
		}

		hr = CoMarshalInterThreadInterfaceInStream(IID_IDispatch, pCallback, &m_CallbackStream);
		if (FAILED(hr))
		{
			SaveWin32Error("CoMarshalInterThreadInterfaceInStream", hr);
			throw E_APIERROR;
		}
	}
	
	if (!m_InitEvent.Create(false) || !m_CallEvent.Create(false) || !m_ShutdownEvent.Create(false))
		throw E_APIERROR;

	m_Thread = (HANDLE)_beginthreadex(0, dwStackSize, ThreadProc, (void*)this, 0, (unsigned int*)&m_ThreadId);
	if (!m_Thread)
	{
		SaveCustomError("_beginthreadex", "Function failed with errno: %U _doserrno: %U", errno, _doserrno);
		throw E_APIERROR;
	}

	DWORD ret = WaitForSingleObject(m_InitEvent, INFINITE);
	switch(ret)
	{
		case WAIT_OBJECT_0:
			// close the event here since we don't need it anymore 
			m_InitEvent.Close();

			if (m_LastError != 0)
			{
				SaveWin32Error(m_LastErrorFunction, m_LastError);
				throw E_APIERROR;
			}
			break;

		case WAIT_TIMEOUT:
			SaveCustomError("WaitForSingleObject", "Function timed out.");
			throw E_APIERROR;

		default: // WAIT_FAILED
			SaveWin32Error("WaitForSingleObject", GetLastError());
			throw E_APIERROR;
	}
}

HRESULT CThreadedComObject::AbortCall(long callid, VARIANT* result)
{
	if (!m_AbortCallEvent)
		return E_NOTIMPL;

	HRESULT hr = S_OK;
	CComCallInfo *pCall = 0;
	VARIANT_BOOL aborted = VARIANT_FALSE;
	POSITION curpos, nextpos;

	m_CritSect.Enter();
	if (callid == 0)
	{
		if (m_CurrentCall.m_p)
		{
			m_CurrentCall->m_Aborted = true;
			if (!SetEvent(m_AbortCallEvent))
				hr = HRESULT_FROM_WIN32(GetLastError());
		}
		m_CallQueue.RemoveAll();
		aborted = VARIANT_TRUE;
	}
	else
	{
		if (m_CurrentCall.m_p && m_CurrentCall->m_CallId == callid)
		{
			m_CurrentCall->m_Aborted = true;
			if (SetEvent(m_AbortCallEvent))
				aborted = VARIANT_TRUE;
			else
				hr = HRESULT_FROM_WIN32(GetLastError());
		}
		else
		{
			nextpos = m_CallQueue.GetHeadPosition();
			while (nextpos)
			{
				curpos = nextpos;
				pCall = m_CallQueue.GetNext(nextpos);
				if (pCall->m_CallId == callid)
				{
					m_CallQueue.RemoveAt(curpos);
					aborted = VARIANT_TRUE;
					break;
				}
			}
		}
	}
	m_CritSect.Leave();	

	if (result)
	{
		result->vt = VT_BOOL;
		result->boolVal = aborted;
	}
	return hr;
}

HRESULT CThreadedComObject::GetCallRunTime(long callid, VARIANT* result)
{
	LARGE_INTEGER time = {0};
	m_CritSect.Enter();
	if (m_CurrentCall.m_p && (m_CurrentCall->m_CallId == callid || callid == 0))
		time = m_CurrentCall->m_StartTime;
	m_CritSect.Leave();

	result->vt = VT_R8;
	if (time.QuadPart > 0)
		result->dblVal = (double)PerformanceCounter::GetMillisecondsSince(time);
	else
		result->dblVal = 0;
	return S_OK;
}

bool CThreadedComObject::ThreadInitialize()
{
try
{
	HRESULT hr;
	CLSID ClassId;
	CComPtr<ITypeInfo> pTypeInfo;
	CComBSTR callinfo(L"callinfo");

	hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
	{
		m_LastError = hr;
		m_LastErrorFunction = "CoInitializeEx";
		throw 1;
	}

	// marshal the callback object into this apartment
	if (m_CallbackStream)
	{
		hr = CoGetInterfaceAndReleaseStream(m_CallbackStream, IID_IDispatch, (LPVOID*)&m_Callback);
		// CoGetInterfaceAndReleaseStream will release the stream even if the function fails, set it to 0 after the call so it doesn't get released twice
		m_CallbackStream = 0;
		if (FAILED(hr))
		{
			m_LastError = hr;
			m_LastErrorFunction = "CoGetInterfaceAndReleaseStream";
			throw 1;
		}
	}

    hr = CLSIDFromProgID(m_ComClass, &ClassId);
	if (FAILED(hr))
	{
		m_LastError = hr;
		m_LastErrorFunction = "CLSIDFromProgID";
		throw 1;
	}
	// the m_ComClass member is only valid in this function (it's on the stack of the calling code which waits for the m_InitEvent)
	m_ComClass = 0;

    hr = CoCreateInstance(ClassId, NULL, m_CreationContext, IID_IDispatch, (void**)&m_Disp);
	if (FAILED(hr))
	{
		m_LastError = hr;
		m_LastErrorFunction = "CoCreateInstance";
		throw 1;
	}

	// cache all DISPID's, we read them out of the typelibrary below and add them to the array m_DispMap
	hr = m_Disp->GetTypeInfo(0, 0, &pTypeInfo);
	if (FAILED(hr))
	{
		m_LastError = hr;
		m_LastErrorFunction = "ITypeInfo::GetTypeInfo";
		throw 1;
	}

    CComTypeAttr typeAttr(pTypeInfo);
	hr = pTypeInfo->GetTypeAttr(&typeAttr);
	if (FAILED(hr))
	{
		m_LastError = hr;
		m_LastErrorFunction = "ITypeInfo::GetTypeAttr";
		throw 1;
	}


	size_t entry = 0;
	size_t funcCount = typeAttr->cFuncs - 7 + (m_SynchronousAccess ? 7 : 6); // subtract the 7 IDispatch functions, add custom entries ...
	m_DispMap.SetCount(funcCount, -1);

	m_DispMap[entry].Name = L"abortcall";
	m_DispMap[entry].Len = m_DispMap[entry].Name.Length();
	m_DispMap[entry++].DispId = DISPID_AbortCall;
	if (m_SynchronousAccess)
	{
		m_DispMap[entry].Name = L"object";
		m_DispMap[entry].Len = m_DispMap[entry].Name.Length();
		m_DispMap[entry++].DispId = DISPID_ContainedObject;
	}
	m_DispMap[entry].Name = L"threadid";
	m_DispMap[entry].Len = m_DispMap[entry].Name.Length();
	m_DispMap[entry++].DispId = DISPID_ThreadId;

	m_DispMap[entry].Name = L"callcontext";
	m_DispMap[entry].Len = m_DispMap[entry].Name.Length();
	m_DispMap[entry++].DispId = DISPID_CallContext;

	m_DispMap[entry].Name = L"getcallqueuesize";
	m_DispMap[entry].Len = m_DispMap[entry].Name.Length();
	m_DispMap[entry++].DispId = DISPID_GetCallQueueSize;

	m_DispMap[entry].Name = L"getcallruntime";
	m_DispMap[entry].Len = m_DispMap[entry].Name.Length();
	m_DispMap[entry++].DispId = DISPID_GetCallRunTime;

	for(UINT curFunc = 7; curFunc < typeAttr->cFuncs; ++curFunc)
	{
		CComFuncDesc funcDesc(pTypeInfo);
		hr = pTypeInfo->GetFuncDesc(curFunc, &funcDesc);
		if (FAILED(hr))
		{
			m_LastError = hr;
			m_LastErrorFunction = "ITypeInfo::GetFuncDesc";
			throw 1;
		}
		if (funcDesc->invkind == INVOKE_FUNC || funcDesc->invkind == INVOKE_PROPERTYPUT)
		{
			hr = pTypeInfo->GetDocumentation(funcDesc->memid, &m_DispMap[entry].Name.m_str, 0, 0, 0);
			if (FAILED(hr))
			{
				m_LastError = hr;
				m_LastErrorFunction = "ITypeInfo::GetDocumentation";
				throw 1;
			}
			hr = m_DispMap[entry].Name.ToLower();
			if (FAILED(hr))
			{
				m_LastError = hr;
				m_LastErrorFunction = "CComBSTR::ToLower";
				throw 1;
			}
			m_DispMap[entry].Len = m_DispMap[entry].Name.Length();
			m_DispMap[entry].DispId = funcDesc->memid;

			if (funcDesc->invkind == INVOKE_PROPERTYPUT && m_DispMap[entry].Len == 8 && m_DispMap[entry].Name == callinfo)
				m_DispId_CallInfo = funcDesc->memid;

			entry++;
		}
	}
	m_DispMap.SetCount(entry);

	// if syncronous access is request register the created object in the Global interface table from which we can later marshal 
	// it into any apartment
	if (m_SynchronousAccess)
	{
		hr = m_Git.Attach(m_Disp);
		if (FAILED(hr))
		{
			m_LastError = hr;
			m_LastErrorFunction = "IGlobalInterfaceTable::RegisterInterfaceInGlobal";
			throw 1;
		}
	}

	// the object implements the magic "CallInfo" property
	// create an event and initialize the property
	if (m_DispId_CallInfo != DISPID_UNKNOWN)
	{
		m_AbortCallEvent = CreateEvent(0, FALSE, FALSE, 0);
		if (!m_AbortCallEvent)
		{
			m_LastError = GetLastError();
			m_LastErrorFunction = "CreateEvent";
			throw 1;
		}
	}

	m_InitEvent.Signal();
	return true;
}
catch(...)
{
	m_InitEvent.Signal();
	return false;
}
}

void CThreadedComObject::ThreadCleanup()
{
	if (m_AbortCallEvent)
	{
		CloseHandle(m_AbortCallEvent);
		m_AbortCallEvent = 0;
	}

	if (m_CallbackStream)
	{
		m_CallbackStream->Release();
		m_CallbackStream = 0;
	}

	if (m_Callback)
	{
		m_Callback->Release();
		m_Callback = 0;
	}

	if (m_SynchronousAccess)
		m_Git.Revoke();

	if (m_Disp)
	{
		m_Disp->Release();
		m_Disp = 0;
	}

	CoUninitialize();
}

void CThreadedComObject::MessageLoop()
{
	MSG msg;
	DWORD ret;
	HANDLE handles[2];
	handles[0] = m_CallEvent;
	handles[1] = m_ShutdownEvent;

	do 
	{
		ret = MsgWaitForMultipleObjects(2, handles, FALSE, INFINITE, QS_POSTMESSAGE);
		switch(ret)
		{
			// call event signaled
			case WAIT_OBJECT_0:
				if (!ProcessCalls())
					return;
				break;

			// shutdown event signaled
			case WAIT_OBJECT_0 + 1:
				return;

			case WAIT_OBJECT_0 + 2:
				while (PeekMessage(&msg,0,0,0,PM_REMOVE))
					DispatchMessage(&msg);
				break;

			case WAIT_FAILED:
				m_LastError = GetLastError();
				m_LastErrorFunction = "MsgWaitForMultipleObjects";
				return;
		}
	} while(true);
}

HRESULT CThreadedComObject::QueueCall(CAutoPtr<CComCallInfo> &callInfo)
{
	HRESULT hr = S_OK;
	m_CritSect.Enter();
	try
	{
		m_CallQueue.AddTail(callInfo);
	}
	catch(CAtlException &pExc)
	{
		hr = pExc.m_hr;
	}
	m_CritSect.Leave();
	return hr;
}

bool CThreadedComObject::DequeueCall()
{
	bool ret = true;
	m_CritSect.Enter();
	if (!m_CallQueue.IsEmpty())
		m_CurrentCall = m_CallQueue.RemoveHead();
	else
		ret = false;
	m_CritSect.Leave();
	return ret;
}

void CThreadedComObject::FreeCall()
{
	m_CritSect.Enter();
	m_CurrentCall.Free();
	m_CritSect.Leave();
}

bool CThreadedComObject::ProcessCalls()
{
	HRESULT hr;
	do
	{
		if (DequeueCall())
		{
			CComVariant vResult;
			CComExcepInfo ErrorInfo;
			UINT ErrorArg;

			if (m_DispId_CallInfo != DISPID_UNKNOWN)
			{
				VARIANTARG Arg;
				DISPPARAMS DispParams = {&Arg, 0, 1, 0};
				Arg.vt = VT_DISPATCH;
				Arg.pdispVal = m_CurrentCall.m_p;
				hr = m_Disp->Invoke(m_DispId_CallInfo, IID_NULL,  m_CurrentCall->m_Lcid, DISPATCH_PROPERTYPUT, &DispParams, 0, 0, 0);
			}
			else
				hr = S_OK;

			if (SUCCEEDED(hr))
			{
				hr = m_CurrentCall->UnmarshalParameters();
				if (SUCCEEDED(hr)) {
					m_CurrentCall->m_StartTime = PerformanceCounter::GetCounter();
					hr = m_Disp->Invoke(m_CurrentCall->m_DispIdMember, IID_NULL, m_CurrentCall->m_Lcid, m_CurrentCall->m_Flags, &m_CurrentCall->m_DispParams, &vResult, &ErrorInfo, &ErrorArg);
				}
				else
				{
					ErrorInfo.scode = hr;
					ErrorInfo.bstrDescription = SysAllocString(L"CoGetInterfaceAndReleaseStream");
				}

				if (m_DispId_CallInfo != DISPID_UNKNOWN)
				{
					VARIANTARG Arg;
					DISPPARAMS DispParams = {&Arg, 0, 1, 0};
					Arg.vt = VT_EMPTY;
					m_Disp->Invoke(m_DispId_CallInfo, IID_NULL, m_CurrentCall->m_Lcid, DISPATCH_PROPERTYPUT, &DispParams, 0, 0, 0);
				}
			}
		
			if (m_Callback)
			{
				VARIANTARG CallbackParms[5];
				DISPPARAMS DispParams = {CallbackParms, 0, 0, 0};
				CComExcepInfo CallbackError;
				UINT CallbackErrorArg;
				DispParams.rgvarg = CallbackParms;

				// only callback on DISPATCH_METHOD (a method call) or DISPATCH_PROPERTYGET (a property access)
				if (SUCCEEDED(hr) && m_Callback_OnCallComplete != DISPID_UNKNOWN && ((m_CurrentCall->m_Flags & (DISPATCH_METHOD | DISPATCH_PROPERTYGET)) > 0))
				{
					DispParams.cArgs = 3;
					CallbackParms[2].vt = VT_I4;
					CallbackParms[2].lVal = m_CurrentCall->m_CallId;
					CallbackParms[1] = vResult;
					CallbackParms[0] = m_CurrentCall->m_CallContext;
					hr = m_Callback->Invoke(m_Callback_OnCallComplete, IID_NULL,  m_CurrentCall->m_Lcid, DISPATCH_METHOD, &DispParams, 0, &CallbackError, &CallbackErrorArg);
				}
				else if (FAILED(hr) && m_Callback_OnError != DISPID_UNKNOWN)
				{
					DispParams.cArgs = 5;
					CallbackParms[4].vt = VT_I4;
					CallbackParms[4].lVal = m_CurrentCall->m_CallId;
					CallbackParms[3] = m_CurrentCall->m_CallContext;
					CallbackParms[2].vt = VT_I4;
					CallbackParms[2].lVal = ErrorInfo.wCode ? ErrorInfo.wCode : ErrorInfo.scode;
					CallbackParms[1].vt = ErrorInfo.bstrSource ? VT_BSTR : VT_EMPTY;
					CallbackParms[1].bstrVal = ErrorInfo.bstrSource;
					CallbackParms[0].vt = ErrorInfo.bstrDescription ? VT_BSTR : VT_EMPTY;
					CallbackParms[0].bstrVal = ErrorInfo.bstrDescription;
					hr = m_Callback->Invoke(m_Callback_OnError, IID_NULL,  m_CurrentCall->m_Lcid, DISPATCH_METHOD, &DispParams, 0, &CallbackError, &CallbackErrorArg);
				}
			}
		
			FreeCall();
		}
		else
			break;

		// process any pending window messages
		MSG msg;
		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			DispatchMessage(&msg);

		// check shutdown event
		if (m_ShutdownFlag)
			return false;

	} while(true);

	return true;
}

unsigned int _stdcall CThreadedComObject::ThreadProc(void* lpParameter)
{
	CThreadedComObject *pObject = reinterpret_cast<CThreadedComObject*>(lpParameter);
	if (pObject->ThreadInitialize())
		pObject->MessageLoop();
	pObject->ThreadCleanup();
	return 0;
}

void _fastcall CreateThreadObject(ParamBlk *parm)
{
	CThreadedComObject* pObject = 0;
try
{
	FoxWString pComClass(p1);
	bool bSyncronousAccess = PCount() >= 3 ? (p3.ev_length > 0) : false;
	DWORD dwContext = (PCount() >= 4 && p4.ev_long) ? p4.ev_long : CLSCTX_INPROC_SERVER;
	DWORD dwStackSize = PCount() == 5 ? p5.ev_long : 0x10000; // default to 64KB of thread stack size

	IDispatch* pCallback = 0;
	if (PCount() >= 2)
	{
		if (Vartype(p2) == 'O')
			GetIDispatchFromObject(p2, (void**)&pCallback);
		else if (Vartype(p2) != '0')
			throw E_INVALIDPARAMS;
	}

	pObject = new CThreadedComObject(pComClass, bSyncronousAccess);
	if (!pObject)
		throw E_INSUFMEMORY;

	pObject->CreateObject(dwContext, pCallback, dwStackSize);

	ReturnIDispatch(pObject);
}
catch(int nErrorNo)
{
	if (pObject)
		pObject->Release();
	RaiseError(nErrorNo);
}
}