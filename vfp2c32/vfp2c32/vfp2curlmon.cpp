#ifndef _THREADSAFE

#define _WINSOCKAPI_ // we're using winsock2 .. so this is neccessary to exclude winsock.h 

#include <windows.h>
#include <stdio.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2curlmon.h"
#include "vfp2cutil.h"
#include "vfp2ccppapi.h"
#include "vfp2casync.h"
#include "vfpmacros.h"

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

void UrlDownloadThread::SetParams(char *pUrl, char *pFile, char *pCallback)
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
		m_Callback.AppendFormatBase("(%U,%U,%U)", ulProgress, ulProgressMax, ulStatusCode);

		if (!m_bAsync)
		{
			FoxValue vRetVal;
			if (_Evaluate(vRetVal, m_Callback) == 0)
			{
				if (vRetVal.Vartype() == 'L')
					return vRetVal->ev_length ? S_OK : E_ABORT;
				else if (vRetVal.Vartype() == 'I')
					return vRetVal->ev_long ? S_OK : E_ABORT;
				else if (vRetVal.Vartype() == 'N')
					return vRetVal->ev_real != 0.0 ? S_OK : E_ABORT;
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
					char *pCommand = m_Callback.Strdup();
					if (pCommand)
						PostMessage(ghAsyncHwnd, WM_CALLBACK, reinterpret_cast<WPARAM>(pCommand), 0);
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
			m_Callback.AppendFormatBase("(%U,%U,%U)", 0, 0, BINDSTATUS_DOWNLOAD_FINISHED);
			_Execute(m_Callback);
		}
		else
		{
			if (!m_nAborted)
				m_Callback.AppendFormatBase("(%U,%U,%U)", 0, 0, BINDSTATUS_DOWNLOAD_FINISHED);
			else
				m_Callback.AppendFormatBase("(%U,%U,%U)", 0, 0, BINDSTATUS_DOWNLOAD_ABORTED);

			char *pCommand = m_Callback.Strdup();
			if (pCommand)
				PostMessage(ghAsyncHwnd, WM_CALLBACK, reinterpret_cast<WPARAM>(pCommand), 0);
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

void UrlDownload::SetCallBack(char *pCallTo)
{
	if (pCallTo && strlen(pCallTo))
	{
		m_Callback = pCallTo;
		m_Callback.SetFormatBase();
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
void _fastcall UrlDownloadToFileEx(ParamBlk *parm)
{
	UrlDownloadThread *pDownload = 0;
try
{
	int nErrorNo = VFP2C_Init_Urlmon();
	if (nErrorNo)
		throw nErrorNo;

	FoxString pUrl(vp1);
	FoxString pFile(vp2);
	FoxString pCallback(parm,3);
	bool bAsync = PCount() == 4 && vp4.ev_length > 0;

	if (bAsync && !pCallback.Len())
		throw E_INVALIDPARAMS;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
		throw E_INVALIDPARAMS;

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

void _fastcall AbortUrlDownloadToFileEx(ParamBlk *parm)
{
try
{
	CThread *pThread = reinterpret_cast<CThread*>(vp1.ev_long);
	Return(goUrlThreads.AbortThread(pThread));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

#endif // _THREADSAFE