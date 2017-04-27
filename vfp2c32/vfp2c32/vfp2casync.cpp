#ifndef _THREADSAFE

#include <windows.h>
#include <stdio.h>
#include <malloc.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2casync.h"
#include "vfp2cutil.h"
#include "vfp2ccppapi.h"
#include "vfp2chelpers.h"
#include "vfpmacros.h"

HWND ghAsyncHwnd = 0;
static ATOM gnAsyncAtom = 0;
CThreadManager goThreadManager;

int _stdcall VFP2C_Init_Async()
{
	if (gnAsyncAtom && ghAsyncHwnd && goThreadManager.Initialized())
		return 0;

	WNDCLASSEX wndClass = {0}; /* MO - message only */
	const char* ASYNC_WINDOW_CLASS	= "__VFP2C_ASWC";

	gnAsyncAtom = (ATOM)GetClassInfoEx(ghModule, ASYNC_WINDOW_CLASS, &wndClass);
	if (!gnAsyncAtom)
	{
		wndClass.cbSize = sizeof(WNDCLASSEX);
		wndClass.hInstance = ghModule;
		wndClass.lpfnWndProc = FindChangeWindowProc;
		wndClass.lpszClassName = ASYNC_WINDOW_CLASS;
		gnAsyncAtom = RegisterClassEx(&wndClass);
	}

	if (gnAsyncAtom)
	{
		/* message only windows are only available from Win2000 or later */
		if (COs::GreaterOrEqual(Windows2000))
			ghAsyncHwnd = CreateWindowEx(0,(LPCSTR)gnAsyncAtom,0,0,0,0,0,0,HWND_MESSAGE,0,ghModule,0);
		else
			ghAsyncHwnd = CreateWindowEx(0,(LPCSTR)gnAsyncAtom,0,WS_POPUP,0,0,0,0,0,0,ghModule,0);

		if (!ghAsyncHwnd)
		{
			SaveWin32Error("CreateWindowEx", GetLastError());
			return E_APIERROR;
		}
	}
	else
	{
		SaveWin32Error("RegisterClassEx", GetLastError());
		return E_APIERROR;
	}

	if (goThreadManager.Initialized() == false)
	{
		if (!goThreadManager.Initialize())
			return E_APIERROR;
	}

	return 0;
}

void _stdcall VFP2C_Destroy_Async(VFP2CTls& tls)
{
	goThreadManager.ShutdownThreads();

	/* destroy window */
	if (ghAsyncHwnd)
		DestroyWindow(ghAsyncHwnd);
	/* unregister windowclass */
	if (gnAsyncAtom)
		UnregisterClass((LPCSTR)gnAsyncAtom,ghModule);
}

void _fastcall FindFileChange(ParamBlk *parm)
{
	FindFileChangeThread *pThread = 0;
try
{
	int nErrorNo = VFP2C_Init_Async();
	if (nErrorNo)
		throw nErrorNo;

	FoxString pPath(p1);
	bool bWatchSubtree = p2.ev_length > 0;
	DWORD nFilter = p3.ev_long;
	FoxString pCallback(p4);

	if (!goThreadManager.Initialized())
	{
		SaveCustomError("FindFileChange","Library not initialized!");
		throw E_APIERROR;
	}

    if (pPath.Len() > MAX_PATH-1)
		throw E_INVALIDPARAMS;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION || pCallback.Len() == 0)
		throw E_INVALIDPARAMS;

	// find free slot
	pThread = new FindFileChangeThread(goThreadManager);
	if (!pThread)
		throw E_INSUFMEMORY;

	if (!pThread->Setup(pPath, bWatchSubtree, nFilter, pCallback))
		throw E_APIERROR;

	pThread->StartThread();

	Return(pThread);
}
catch(int nErrorNo)
{
	if (pThread)
		delete pThread;

	RaiseError(nErrorNo);
}
}

void _fastcall CancelFileChange(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Async();
	if (nErrorNo)
		throw nErrorNo;

	CThread *pThread = reinterpret_cast<CThread*>(p1.ev_long);
	Return(goThreadManager.AbortThread(pThread));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FindRegistryChange(ParamBlk *parm)
{
	FindRegistryChangeThread *pThread = 0;
try
{
	int nErrorNo = VFP2C_Init_Async();
	if (nErrorNo)
		throw nErrorNo;

	HKEY hRoot = reinterpret_cast<HKEY>(p1.ev_long);
	FoxString pKey(p2);
	bool bWatchSubtree = p3.ev_length > 0;
	DWORD dwFilter = p4.ev_long;
	FoxString pCallback(p5);

	if (!goThreadManager.Initialized())
	{
		SaveCustomError("FindRegistryChange","Library not initialized!");
		throw E_APIERROR;
	}

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION || pCallback.Len() == 0)
		throw E_INVALIDPARAMS;

	pThread = new FindRegistryChangeThread(goThreadManager);
	if (!pThread)
		throw E_INSUFMEMORY;

	if (!pThread->Setup(hRoot, pKey, bWatchSubtree, dwFilter, pCallback))
		throw E_APIERROR;

	pThread->StartThread();

	Return(pThread);
}
catch(int nErrorNo)
{
	if (pThread)
		delete pThread;

	RaiseError(nErrorNo);
}
}

void _fastcall CancelRegistryChange(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Async();
	if (nErrorNo)
		throw nErrorNo;

	CThread *pThread = reinterpret_cast<CThread*>(p1.ev_long);
	Return(goThreadManager.AbortThread(pThread));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AsyncWaitForObject(ParamBlk *parm)
{
	WaitForObjectThread *pThread = 0;
try
{
	int nErrorNo = VFP2C_Init_Async();
	if (nErrorNo)
		throw nErrorNo;

	HANDLE hObject = reinterpret_cast<HANDLE>(p1.ev_long);
	FoxString pCallback(p2);

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION || pCallback.Len() == 0)
		throw E_INVALIDPARAMS;

	pThread = new WaitForObjectThread(goThreadManager);
	if (!pThread)
		throw E_INSUFMEMORY;

	if (!pThread->Setup(hObject, pCallback))
		throw E_APIERROR;

	pThread->StartThread();

	Return(pThread);
}
catch(int nErrorNo)
{
	if (pThread)
		delete pThread;

	RaiseError(nErrorNo);
}
}

void _fastcall CancelWaitForObject(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Async();
	if (nErrorNo)
		throw nErrorNo;

	CThread *pThread = reinterpret_cast<CThread*>(p1.ev_long);
	Return(goThreadManager.AbortThread(pThread));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

LRESULT _stdcall FindChangeWindowProc(HWND nHwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int nErrorNo;
	if (uMsg == WM_CALLBACK)
	{
		__try
		{
			char *pCommand = reinterpret_cast<char*>(wParam);
			if (pCommand)
			{
				_Execute(pCommand);
				delete[] pCommand;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return 0;
	}
	else if (uMsg == WM_CALLBACKRESULT)
	{
		Value vRetVal;
		vRetVal.ev_type = '0';
		LRESULT retval = 0;
		__try
		{
			char *pCommand = reinterpret_cast<char*>(wParam);
			if (pCommand)
			{
				nErrorNo = _Evaluate(&vRetVal, pCommand);
				delete[] pCommand;
				if (nErrorNo == 0)
				{
					if (Vartype(vRetVal) == 'I')
						retval = vRetVal.ev_long;
					else if (Vartype(vRetVal) == 'N')
						retval = static_cast<LRESULT>(vRetVal.ev_real);
					else if (Vartype(vRetVal) == 'L')
						retval = vRetVal.ev_width;
					else 
						ReleaseValue(vRetVal);
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		return retval;
	}
	else
		return DefWindowProc(nHwnd,uMsg,wParam,lParam);
}

FindFileChangeThread::FindFileChangeThread(CThreadManager &pManager) : CThread(pManager)
{
	m_FileEvent = INVALID_HANDLE_VALUE;
}

FindFileChangeThread::~FindFileChangeThread()
{
	if (m_FileEvent != INVALID_HANDLE_VALUE)
		FindCloseChangeNotification(m_FileEvent);
}

bool FindFileChangeThread::Setup(char *pPath, bool bWatchSubtree, DWORD nFilter, char *pCallback)
{
	m_Path = pPath;
	m_Callback = pCallback;
	m_Callback += "(%U,'%S',%U)";
	m_Buffer.Size(VFP2C_MAX_CALLBACKBUFFER);

	if (!m_AbortEvent.Create())
		return false;

	m_FileEvent = FindFirstChangeNotification(pPath, bWatchSubtree ? TRUE : FALSE, nFilter);
	if (m_FileEvent == INVALID_HANDLE_VALUE)
	{
		SaveWin32Error("FindFirstChangeNotification", GetLastError());
		return false;
	}
	return true;
}

void FindFileChangeThread::SignalThreadAbort()
{
	m_AbortEvent.Signal();
}

DWORD FindFileChangeThread::Run()
{
	bool bLoop = true;
	DWORD nRetVal;
	char *pCallback;

	HANDLE hEvents[2];
	hEvents[0] = m_FileEvent;
	hEvents[1] = m_AbortEvent;

	while (bLoop)
	{
		nRetVal = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		switch (nRetVal)
		{
			case WAIT_OBJECT_0:
				m_Buffer.Format(m_Callback, this, (char*)m_Path, 0);
				pCallback = m_Buffer.Strdup();
				if (pCallback)
					PostMessage(ghAsyncHwnd, WM_CALLBACK, reinterpret_cast<WPARAM>(pCallback), 0);

				if (!FindNextChangeNotification(m_FileEvent))
				{
					bLoop = false;
					m_Buffer.Format(m_Callback, this, "FindNextChangeNotification", GetLastError());
					pCallback = m_Buffer.Strdup();
					if (pCallback)
				        PostMessage(ghAsyncHwnd, WM_CALLBACK, reinterpret_cast<WPARAM>(pCallback), 0);
				}
				break;

			case WAIT_OBJECT_0 + 1: // notification canceled
				bLoop = false;
				break;

			case WAIT_FAILED:
				bLoop = false;
				m_Buffer.Format(m_Callback, this, "WaitForMultipleObjects", GetLastError());
				pCallback = m_Buffer.Strdup();
				if (pCallback)
                    PostMessage(ghAsyncHwnd, WM_CALLBACK, reinterpret_cast<WPARAM>(pCallback), 0);
		}
	}
	return 0;
}

FindRegistryChangeThread::FindRegistryChangeThread(CThreadManager &pManager) : CThread(pManager)
{
	m_RegKey = NULL;
}

FindRegistryChangeThread::~FindRegistryChangeThread()
{
	if (m_RegKey)
		RegCloseKey(m_RegKey);
}

bool FindRegistryChangeThread::Setup(HKEY hRoot, char *pKey, bool bWatchSubtree, DWORD dwFilter, char *pCallback)
{
	m_WatchSubtree = bWatchSubtree;
	m_Filter = dwFilter;
	m_Callback = pCallback;
	m_Callback += "(%U,%I)";
	m_Buffer.Size(VFP2C_MAX_CALLBACKBUFFER);

	DWORD nRetVal;
	nRetVal = RegOpenKeyEx(hRoot, pKey, 0, KEY_NOTIFY, &m_RegKey);
	if (nRetVal != ERROR_SUCCESS)
	{
		SaveWin32Error("RegOpenKeyEx", nRetVal);
		return false;
	}

	// create auto-reset event to be triggered when registry value changes
	if (!m_RegistryEvent.Create(false))
		return false;

	// create manual reset event to abort the thread ..
	if (!m_AbortEvent.Create())
		return false;

	nRetVal = RegNotifyChangeKeyValue(m_RegKey, m_WatchSubtree ? TRUE : FALSE, m_Filter, m_RegistryEvent, TRUE);
	if (nRetVal != ERROR_SUCCESS)
	{
		SaveWin32Error("RegNotifyChangeKeyValue", nRetVal);
		return false;
	}

	return true;
}

void FindRegistryChangeThread::SignalThreadAbort()
{
	m_AbortEvent.Signal();
}

DWORD FindRegistryChangeThread::Run()
{
	bool bLoop = true;
	DWORD nRetVal;
	char *pCallback;

	HANDLE hEvents[2];
	hEvents[0] = m_RegistryEvent;
	hEvents[1] = m_AbortEvent;

	while (bLoop)
	{
		nRetVal = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE); // wait for change/abort event to occur
		switch (nRetVal)
		{
			case WAIT_OBJECT_0:
				m_Buffer.Format(m_Callback, this, 0);
				pCallback = m_Buffer.Strdup();
				if (pCallback)
					PostMessage(ghAsyncHwnd, WM_CALLBACK, (WPARAM)pCallback, 0); // we need to use PostMessage cause it's asynchronous .. SendMessage can bring us into a deadlock situation

				nRetVal = RegNotifyChangeKeyValue(m_RegKey, m_WatchSubtree, m_Filter, m_RegistryEvent, TRUE); // continue watching
				if (nRetVal != ERROR_SUCCESS)
				{
					bLoop = false;
					m_Buffer.Format(m_Callback, this, nRetVal);
					pCallback = m_Buffer.Strdup();
					if (pCallback)
				        PostMessage(ghAsyncHwnd, WM_CALLBACK, (WPARAM)pCallback, 0);
				}
				break;

			case WAIT_OBJECT_0 + 1: // notification canceled
				bLoop = false;
				break;

			case WAIT_FAILED:
				bLoop = false;
				m_Buffer.Format(m_Callback, this, GetLastError());
				pCallback = m_Buffer.Strdup();
				if (pCallback)
					PostMessage(ghAsyncHwnd, WM_CALLBACK, (WPARAM)pCallback, 0);

		}
	}

	return 0;
}

void WaitForObjectThread::SignalThreadAbort()
{
	m_AbortEvent.Signal();
}

bool WaitForObjectThread::Setup(HANDLE hObject, char *pCallback)
{
	m_Object = hObject;
	m_Callback = pCallback;
	m_Callback += "(%U)";
	m_Buffer.Size(VFP2C_MAX_CALLBACKBUFFER);

	// create manual reset event to abort the thread ..
	if (!m_AbortEvent.Create())
		return false;

	return true;
}

DWORD WaitForObjectThread::Run()
{
	DWORD nRetVal;
	char *pCallback;

	HANDLE hEvents[2];
	hEvents[0] = m_Object;
	hEvents[1] = m_AbortEvent;

	nRetVal = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE); // wait for an event to occur
	switch (nRetVal)
	{
		case WAIT_OBJECT_0:
			m_Buffer.Format(m_Callback, 0);
			pCallback = m_Buffer.Strdup();
			if (pCallback)
				PostMessage(ghAsyncHwnd, WM_CALLBACK, reinterpret_cast<WPARAM>(pCallback), 0); // we need to use PostMessage cause it's asynchronous .. SendMessage can bring us into a deadlock situation
			break;

		case WAIT_OBJECT_0 + 1: // notification canceled
			break;

		case WAIT_FAILED:
			m_Buffer.Format(m_Callback, GetLastError());
			pCallback = m_Buffer.Strdup();
			if (pCallback)
				PostMessage(ghAsyncHwnd, WM_CALLBACK, reinterpret_cast<WPARAM>(pCallback), 0);
	}

	return 0;
}

#endif // _THREADSAFE