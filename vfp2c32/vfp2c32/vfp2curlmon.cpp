#ifndef _THREADSAFE

#define _WINSOCKAPI_ // we're using winsock2 .. so this is neccessary to exclude winsock.h 

#include <windows.h>
#include <stdio.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2curlmon.h"
#include "vfp2cutil.h"
#include "vfp2ccppapi.h"
#include "vfp2casync.h"

static CThreadManager goUrlThreads;

UrlDownload::UrlDownload() : m_bCallback(false), m_bAsync(false), m_nAborted(0), m_ulObjRefCount(0)
{

}

DWORD UrlDownloadThread::Run()
{
	return static_cast<DWORD>(m_Download.Download());
}

void UrlDownloadThread::SignalThreadAbort()
{
	m_Download.Abort(THREAD_ABORT_FLAG);
}

void UrlDownloadThread::Release()
{
	delete this;
}

void UrlDownloadThread::SetParams(char *pUrl, char *pFile, CStringView pCallback)
{
	m_Download.pUrl = pUrl;
	m_Download.pFile = pFile;
	m_Download.SetCallBack(pCallback);
	m_Download.SetAsync(true);
}

/* implementation of the UrlDownload class for UrlDownloadToFile, syncronous & asyncronous in a seperate thread */
STDMETHODIMP UrlDownload::QueryInterface(REFIID riid, void **ppvObject)
{
	*ppvObject = NULL;
	
	// IUnknown
	if (::IsEqualIID(riid, __uuidof(IUnknown)))
		*ppvObject = this;
	// IBindStatusCallback
	else if (::IsEqualIID(riid, __uuidof(IBindStatusCallback)))
		*ppvObject = static_cast<IBindStatusCallback *>(this);

	if (*ppvObject)
	{
		(*reinterpret_cast<LPUNKNOWN *>(ppvObject))->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) UrlDownload::AddRef()
{
	return ++m_ulObjRefCount;
}

STDMETHODIMP_(ULONG) UrlDownload::Release()
{
	return --m_ulObjRefCount;
}

STDMETHODIMP UrlDownload::OnStartBinding(DWORD, IBinding *)
{
	m_nTickCount = GetTickCount();
	return S_OK;
}

STDMETHODIMP UrlDownload::GetPriority(LONG *)
{
	return E_NOTIMPL;
}

STDMETHODIMP UrlDownload::OnLowResource(DWORD)
{
	return E_NOTIMPL;
}

STDMETHODIMP UrlDownload::OnProgress(ULONG ulProgress,
										 ULONG ulProgressMax,
										 ULONG ulStatusCode,
										 LPCWSTR szStatusText)
{
	if (m_bCallback)
	{
		if (!m_bAsync)
		{
			ValueEx vRetVal;
			if (m_Callback.Evaluate(vRetVal, ulProgress, ulProgressMax, ulStatusCode) == 0)
			{
				if (vRetVal.Vartype() == 'L')
					return vRetVal.ev_length ? S_OK : E_ABORT;
				else if (vRetVal.Vartype() == 'I')
					return vRetVal.ev_long ? S_OK : E_ABORT;
				else if (vRetVal.Vartype() == 'N')
					return vRetVal.ev_real != 0.0 ? S_OK : E_ABORT;
				else
					vRetVal.Release();
			}
			return E_ABORT;
		}
		else
		{
			if (!m_nAborted)
			{
				DWORD nCount = GetTickCount();
				if (nCount - m_nTickCount > 500 || nCount < m_nTickCount)
				{
					m_nTickCount = nCount;
					m_Callback.AsyncExecute(ghAsyncHwnd, WM_CALLBACK, ulProgress, ulProgressMax, ulStatusCode);
				}
				return S_OK;
			}
			else
				return E_ABORT;
		}
	}
	else
		return S_OK;
}

STDMETHODIMP UrlDownload::OnStopBinding(HRESULT hr, LPCWSTR lpStatus)
{
	if (m_bCallback)
	{
		if (!m_bAsync)
		{
			m_Callback.Execute(0, 0, BINDSTATUS_DOWNLOAD_FINISHED);
		}
		else
		{
			if (!m_nAborted)
				m_Callback.AsyncExecute(ghAsyncHwnd, WM_CALLBACK, 0, 0, BINDSTATUS_DOWNLOAD_FINISHED);
			else
				m_Callback.AsyncExecute(ghAsyncHwnd, WM_CALLBACK, 0, 0, BINDSTATUS_DOWNLOAD_ABORTED);
		}
	}
	return S_OK;
}

STDMETHODIMP UrlDownload::GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo)
{
	return S_OK;
}

STDMETHODIMP UrlDownload::OnDataAvailable(DWORD, DWORD, FORMATETC *, STGMEDIUM *)
{
	return E_NOTIMPL;
}

STDMETHODIMP UrlDownload::OnObjectAvailable(REFIID, IUnknown *)
{
	return E_NOTIMPL;
}

void UrlDownload::SetCallBack(CStringView pCallback)
{
	if (pCallback)
	{
		m_Callback.SetCallback(pCallback);
		m_bCallback = true;
	}
	else
		m_bCallback = false;
}

void UrlDownload::SetAsync(bool bAsync)
{
	m_bAsync = bAsync;
}

void UrlDownload::Abort(int nAbortFlag)
{
	m_nAborted = nAbortFlag;
}

HRESULT UrlDownload::Download()
{
	return ::URLDownloadToFile(0,pUrl,pFile,0,(LPBINDSTATUSCALLBACK)this);
}

/* initialisation & cleanup */
int _stdcall VFP2C_Init_Urlmon()
{
	if (goUrlThreads.Initialized() == false)
	{
		if (goUrlThreads.Initialize() == false)
		{
			return E_APIERROR;
		}		
	}
	return 0;
}

void _stdcall VFP2C_Destroy_Urlmon(VFP2CTls& tls)
{
	goUrlThreads.ShutdownThreads();
}

/* UrlDownloadToFile wrapper */
void _fastcall UrlDownloadToFileEx(ParamBlkEx& parm)
{
	UrlDownloadThread *pDownload = 0;
try
{
	int nErrorNo = VFP2C_Init_Urlmon();
	if (nErrorNo)
		throw nErrorNo;

	FoxString pUrl(parm(1));
	FoxString pFile(parm(2));
	FoxString pCallback(parm,3);
	bool bAsync = parm.PCount() == 4 && parm(4)->ev_length > 0;

	if (bAsync && !pCallback.Len())
		throw E_INVALIDPARAMS;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
	{
		SaveCustomError("UrlDownloadToFileEx", "Callback function length is greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}

	if (!bAsync)
	{
		HRESULT hr;
		UrlDownload pStatus;
		pStatus.SetCallBack(pCallback);
		hr = URLDownloadToFile(0,pUrl,pFile,0,&pStatus);

		if (FAILED(hr) && hr != E_ABORT)
			SaveWin32Error("UrlDownloadToFile", hr);			
	
		Return(hr);
	}
	else
	{
		pDownload = new UrlDownloadThread(goUrlThreads);
		if (!pDownload)
			throw E_INSUFMEMORY;

		pDownload->SetParams(pUrl,pFile,pCallback);
		pDownload->StartThread();

		Return(pDownload);
	}
}
catch(int nErrorNo)
{
	if (pDownload)
		delete pDownload;
	RaiseError(nErrorNo);
}
}

void _fastcall AbortUrlDownloadToFileEx(ParamBlkEx& parm)
{
try
{
	CThread* pThread = parm(1)->Ptr<CThread*>();
	Return(goUrlThreads.AbortThread(pThread));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

#endif // _THREADSAFE