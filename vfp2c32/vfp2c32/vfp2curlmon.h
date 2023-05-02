#ifndef _THREADSAFE

#ifndef _VFP2CINET_H__
#define _VFP2CINET_H__

#include "urlmon.h"
#include "vfp2chelpers.h"

const int BINDSTATUS_DOWNLOAD_FINISHED	= 99;
const int BINDSTATUS_DOWNLOAD_ABORTED	= 100;

class UrlDownload : public IBindStatusCallback
{
public:
	UrlDownload();
	~UrlDownload() {}

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IBindStatusCallback methods
	STDMETHOD(OnStartBinding)(DWORD, IBinding *);
	STDMETHOD(GetPriority)(LONG *);
	STDMETHOD(OnLowResource)(DWORD);
	STDMETHOD(OnProgress)(ULONG ulProgress,
						  ULONG ulProgressMax,
						  ULONG ulStatusCode,
						  LPCWSTR szStatusText);
	STDMETHOD(OnStopBinding)(HRESULT, LPCWSTR);
	STDMETHOD(GetBindInfo)(DWORD *, BINDINFO *);
	STDMETHOD(OnDataAvailable)(DWORD, DWORD, FORMATETC *, STGMEDIUM *);
	STDMETHOD(OnObjectAvailable)(REFIID, IUnknown *);

	void SetCallBack(CStringView pCallback);
	void SetAsync(bool bAsync);
	void Abort(int nAbortFlag);
	HRESULT Download();

	CStr pUrl;
	CStr pFile;

private:
	int m_nAborted;
	DWORD m_nTickCount;
	bool m_bCallback;
	bool m_bAsync;
	UINT m_ulObjRefCount;
	CFoxCallback m_Callback;
};

class UrlDownloadThread : public CThread
{
public:
	UrlDownloadThread(CThreadManager &pPool) : CThread(pPool) { }
	~UrlDownloadThread() {}

	virtual void SignalThreadAbort();
	virtual DWORD Run();
	virtual void Release();
	void SetParams(char *pUrl, char *pFile, CStringView pCallback);

protected:
	UrlDownload m_Download;
};

#ifdef __cplusplus
extern "C" {
#endif

int _stdcall VFP2C_Init_Urlmon();
void _stdcall VFP2C_Destroy_Urlmon(VFP2CTls& tls);

/* UrlDownloadToFile related functions */
void _fastcall UrlDownloadToFileEx(ParamBlkEx& parm);
void _fastcall AbortUrlDownloadToFileEx(ParamBlkEx& parm);
DWORD _stdcall UrlDownloadToFileThreadProc(LPVOID lpParam);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif // _VFP2CINET_H__

#endif // _THREADSAFE