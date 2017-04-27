#ifndef _VFP2CASYNCCOM_H__
#define _VFP2CASYNCCOM_H__

#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include "vfp2chelpers.h"

class CComCallInfo : public IDispatch
{
public:
	CComCallInfo();
	~CComCallInfo();
	friend class CThreadedComObject;

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IDispatch methods
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo);
	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, 
		DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

private:
	HRESULT Aborted(VARIANT *result);
	HRESULT MarshalParameters(DISPPARAMS* pdispparams);
	HRESULT UnmarshalParameters();

	LONG m_CallId;
	DISPID m_DispIdMember; 
	LCID m_Lcid;
	WORD m_Flags;
    DISPPARAMS m_DispParams;
	CComVariant m_CallContext;
	HANDLE m_AbortEvent;
	bool m_Aborted;
	LARGE_INTEGER m_StartTime;

	static const DISPID DISPID_AbortEvent		= 1;
	static const DISPID DISPID_Aborted			= 2;
	static const DISPID DISPID_CallContext		= 3;
	static const DISPID DISPID_CallId			= 4;
};

class CDispEntry
{
public:
	CComBSTR Name;
	int Len;
	DISPID DispId;
};

class CThreadedComObject : public IDispatch
{
public:
	CThreadedComObject(wchar_t *pComClass, bool bSynchronousAccess);
	~CThreadedComObject();
	
	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IDispatch methods
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo);
	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, 
		DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

	void CreateObject(DWORD dwContext, IDispatch* pCallback, DWORD dwStackSize);

private:
	// custom internal methods
	bool ThreadInitialize();
	void ThreadCleanup();
	void MessageLoop();
	HRESULT QueueCall(CAutoPtr<CComCallInfo> &callInfo);
	bool DequeueCall();
	void FreeCall();
	bool ProcessCalls();
	
	// custom methods callable by COM object
	HRESULT AbortCall(long callid, VARIANT* result);
	HRESULT GetCallRunTime(long callid, VARIANT* result);

	static unsigned int _stdcall ThreadProc(void* lpParameter);
	static long m_CallId;	// call id, incremented on each call of a method

	long m_RefCount;	// reference count
	IDispatch* m_Disp;	// the wrapped object 
	IDispatch* m_Callback; // the callback object (only valid inside the thread)
	LPSTREAM m_CallbackStream; // stream to marshal the callback object into the threads apartment
	DISPID m_Callback_OnCallComplete; // DISPID of the callback object's OnCallComplete method
	DISPID m_Callback_OnError;	// DISPID of the callback object's OnError method
	DISPID m_DispId_CallInfo;	// DISPID of CallInfo property in contained object
	wchar_t* m_ComClass;	// the ComClass that should be created
	DWORD m_CreationContext; // parameter for CoCreateInstance
	DWORD m_ThreadId;	// the thread id of the newly created thread
	HANDLE m_Thread;	// handle to the thread on which the wrapped object runs
	CEvent m_InitEvent;	// event  to syncronize object construction 
	CEvent m_CallEvent; // event to signal the thread that a call was queued
	CEvent m_ShutdownEvent; // event to signal the thread to shutdown
	HANDLE m_AbortCallEvent; // event to signal the object to cancel the currently running method
	DWORD m_LastError;	// error number of the last error that occured
	char* m_LastErrorFunction; // name of the function that caused the last error
	CComVariant m_CallContext; // context value which is passed to the OnComplete/OnError callback routine and can be set by setting CallContext
	CCriticalSection m_CritSect; // critical section to protect access the to m_CallQueue
	CAutoPtr<CComCallInfo> m_CurrentCall; // the currently active call
	CAutoPtrList<CComCallInfo> m_CallQueue; // list of issued calls
	CAtlArray<CDispEntry> m_DispMap; // cache of DISPID on the wrapped object
	CComGITPtr<IDispatch> m_Git; // global interface table
	bool m_SynchronousAccess; // register object in global interface table for syncronous access
	bool m_ShutdownFlag;	// flag if the thread is signaled to shutdown (optimization so we don't have to call WaitForSingleObject in the ProcessCalls method)

	static const DISPID DISPID_AbortCall				= -100000;
	static const DISPID DISPID_ContainedObject			= -100001;
	static const DISPID DISPID_ThreadId					= -100002;
	static const DISPID DISPID_CallContext				= -100003;
	static const DISPID DISPID_GetCallQueueSize			= -100004;
	static const DISPID DISPID_GetCallRunTime			= -100005;
};

#ifdef __cplusplus
extern "C" {
#endif

void _fastcall CreateThreadObject(ParamBlk *parm);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif // _VFP2CASYNCCOM_H__