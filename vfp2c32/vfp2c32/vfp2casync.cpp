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
FindFileChangeExThread *goFindFileChangeExThread = 0;

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

void _fastcall FindFileChangeEx(ParamBlk *parm)
{
	FindFileChangeExEntry *pFFC = 0;
try
{
	int nErrorNo = VFP2C_Init_Async();
	if (nErrorNo)
		throw nErrorNo;

	FoxString pPath(p1);
	bool bWatchSubtree = p2.ev_length > 0;
	DWORD nFilter = p3.ev_long;
	FoxString pCallback(p4);
	DWORD nBufferSize = PCount() > 4 ? p5.ev_long : 32768;

    if (pPath.Len() > MAX_PATH-1)
		throw E_INVALIDPARAMS;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION || pCallback.Len() == 0)
		throw E_INVALIDPARAMS;

	if (goFindFileChangeExThread == 0) {
		goFindFileChangeExThread = new FindFileChangeExThread(goThreadManager);
		if (goFindFileChangeExThread == 0)
			throw E_INSUFMEMORY;
	}

	// find free slot
	pFFC = new FindFileChangeExEntry();
	if (pFFC == 0)
		throw E_INSUFMEMORY;

	pFFC->Setup(pPath, bWatchSubtree, nFilter, pCallback, nBufferSize, goFindFileChangeExThread->GetIoPort());

	goFindFileChangeExThread->AddDirectory(pFFC);
	goFindFileChangeExThread->StartThread();
	Return(pFFC);
}
catch(int nErrorNo)
{
	if (pFFC)
		delete pFFC;
	RaiseError(nErrorNo);
}
}

void _fastcall CancelFileChangeEx(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Async();
	if (nErrorNo)
		throw nErrorNo;


	FindFileChangeExEntry *pFFC = reinterpret_cast<FindFileChangeExEntry *>(p1.ev_long);
	if (goFindFileChangeExThread)
	{
		if (pFFC) 
		{
			if (!goFindFileChangeExThread->RemoveDirectory(pFFC))
				throw E_INVALIDPARAMS;
		}
		else 
		{
			goFindFileChangeExThread->RemoveAllDirectories();
		}

		if (!goFindFileChangeExThread->IsWatching())
		{
			goThreadManager.AbortThread(goFindFileChangeExThread);
			goFindFileChangeExThread = 0;
		}
		Return(true);
	}
	Return(false);
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
	LRESULT retval;

	switch(uMsg)
	{
		case WM_CALLBACK:
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

		case WM_CALLBACKRESULT:
			Value vRetVal;
			vRetVal.ev_type = '0';
			retval = 0;
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

		case WM_CALLBACK_FFEX:
			__try
			{
				FindFileChangeExEntry *pFFC = reinterpret_cast<FindFileChangeExEntry*>(wParam);
				LPBYTE pBuffer = reinterpret_cast<LPBYTE>(lParam);
				if (goFindFileChangeExThread && goFindFileChangeExThread->ValidDirectory(pFFC)) 
				{
					PFILE_NOTIFY_INFORMATION pInfo = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(lParam);
					while(true)
					{
						pFFC->Callback(pInfo);
						if (pInfo->NextEntryOffset == 0)
							break;
				
						pInfo = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pInfo + pInfo->NextEntryOffset);
						if ((LPBYTE)pInfo > pBuffer + pFFC->BufferSize())
							break;
					}
				}
				delete[] pBuffer;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {}
			return 0;
	}

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

FindFileChangeExEntry::FindFileChangeExEntry()
{
	memset(&m_Ol, 0, sizeof(m_Ol));
	m_OldFilename[0] = 0;
	m_OldFilenameLength = 0;
}

FindFileChangeExEntry::~FindFileChangeExEntry()
{
	if (m_Handle)
	{
		CancelIo(m_Handle);
		m_Handle.Close();
	}
}

void FindFileChangeExEntry::Setup(char *pPath, bool bWatchSubtree, DWORD nFilter, char *pCallback, DWORD nBufferSize, HANDLE nIoPort)
{
	m_Handle = CreateFile(pPath, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
	if (!m_Handle)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}
	m_Path = pPath;
	m_Path.AddBs();
	m_LongPath.Size(MAX_PATH + 1);
	m_LongPath2.Size(MAX_PATH + 1);
	m_Callback = pCallback;
	m_Callback += "(%U,%U,'%S','%S')";
	m_CallbackError = pCallback;
	m_CallbackError += "(%U,%U,'%S',%U)";
	m_CallbackBuffer.Size(VFP2C_MAX_CALLBACKBUFFER);
	m_PostCallbackBuffer.Size(VFP2C_MAX_CALLBACKBUFFER);
	m_BufferSize = nBufferSize;
	m_ReadDirBuffer.Size(m_BufferSize);
	m_Subtree = bWatchSubtree;
	m_Filter = nFilter;

	if (!CreateIoCompletionPort(m_Handle, nIoPort, (ULONG_PTR)this, 1))
	{
		SaveWin32Error("CreateIoCompletionPort", GetLastError());
		throw E_APIERROR;
	}

	if (!ReadDirectoryChangesW(m_Handle, m_ReadDirBuffer, m_ReadDirBuffer.Size(), bWatchSubtree, nFilter, 0, &m_Ol, 0))
	{
		SaveWin32Error("ReadDirectoryChangesW", GetLastError());
		throw E_APIERROR;
	}
}

void FindFileChangeExEntry::ReadChanges()
{
	m_ReadDirBuffer.Detach(m_PostBuffer);
	m_ReadDirBuffer.Size(m_BufferSize);
	if (!ReadDirectoryChangesW(m_Handle, m_ReadDirBuffer, m_ReadDirBuffer.Size(), m_Subtree, m_Filter, 0, &m_Ol, 0))
	{
		m_PostCallbackBuffer.Format(m_CallbackError, this, 0, "ReadDirectoryChangesW", GetLastError());
		char *pCallback = m_PostCallbackBuffer.Strdup();
		if (pCallback)
			PostMessage(ghAsyncHwnd, WM_CALLBACK, (WPARAM)pCallback, 0);
	}
}

void FindFileChangeExEntry::Callback(PFILE_NOTIFY_INFORMATION pInfo)
{
	DWORD nLen;
	switch(pInfo->Action)
	{
		case FILE_ACTION_ADDED:
		case FILE_ACTION_REMOVED:
		case FILE_ACTION_MODIFIED:
			m_LongPath.Format("%S%W", (char*)m_Path, &pInfo->FileName, pInfo->FileNameLength / 2);
			m_LongPath2 = "";
			if (pInfo->Action != FILE_ACTION_REMOVED)
			{
				nLen = GetLongPathName(m_LongPath, m_LongPath, m_LongPath.Size());
				if (nLen >= m_LongPath.Size())
				{
					m_LongPath.Size(nLen + 1);
					GetLongPathName(m_LongPath, m_LongPath, m_LongPath.Size());
				}
			}
			m_CallbackBuffer.Format(m_Callback, this, pInfo->Action, (char*)m_LongPath, (char*)m_LongPath2);
			_Execute(m_CallbackBuffer);
			break;

		case FILE_ACTION_RENAMED_OLD_NAME:
			memcpy(&m_OldFilename, &pInfo->FileName, pInfo->FileNameLength);
			m_OldFilenameLength = pInfo->FileNameLength;
			break;

		case FILE_ACTION_RENAMED_NEW_NAME:
			m_LongPath.Format("%S%W", (char*)m_Path, &m_OldFilename, m_OldFilenameLength / 2);
			m_LongPath2.Format("%S%W", (char*)m_Path, &pInfo->FileName, pInfo->FileNameLength / 2);
			nLen = GetLongPathName(m_LongPath2, m_LongPath2, m_LongPath2.Size());
			if (nLen >= m_LongPath2.Size())
			{
				m_LongPath2.Size(nLen + 1);
				GetLongPathName(m_LongPath2, m_LongPath2, m_LongPath2.Size());
			}
			m_CallbackBuffer.Format(m_Callback, this, FILE_ACTION_RENAMED_OLD_NAME, (char*)m_LongPath, (char*)m_LongPath2);
			m_OldFilenameLength = 0;
			_Execute(m_CallbackBuffer);
			break;
	}
}

void FindFileChangeExEntry::PostCallback()
{
	PostMessage(ghAsyncHwnd, WM_CALLBACK_FFEX, (WPARAM)this, (LPARAM)m_PostBuffer.Address());
	m_PostBuffer.Detach();
}

FindFileChangeExThread::FindFileChangeExThread(CThreadManager &pManager) : CThread(pManager)
{
	if (!m_Sect.Initialize(4000))
		throw E_APIERROR;

	if (!m_SectDelete.Initialize(4000))
		throw E_APIERROR;

	m_IoPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
	if (!m_IoPort)
	{
		SaveWin32Error("CreateIoCompletionPort", GetLastError());
		throw E_APIERROR;
	}
}

void FindFileChangeExThread::AddDirectory(FindFileChangeExEntry *pFFC)
{
	m_Sect.Enter();
	m_FileChangeEntries.SetAt(pFFC, pFFC);
	m_Sect.Leave();
}

bool FindFileChangeExThread::RemoveDirectory(FindFileChangeExEntry *pFFC)
{
	bool ret;
	m_SectDelete.Enter();
	m_Sect.Enter();
	if (ret = m_FileChangeEntries.RemoveKey(pFFC))
	{
		delete pFFC;
	}
	m_Sect.Leave();
	m_SectDelete.Leave();
	return ret;
}

void FindFileChangeExThread::RemoveAllDirectories()
{
	FindFileChangeExEntry *pFFC;
	m_SectDelete.Enter();
	m_Sect.Enter();
	while(!m_FileChangeEntries.IsEmpty())
	{
		POSITION nPos = m_FileChangeEntries.GetStartPosition();
		pFFC = m_FileChangeEntries.GetValueAt(nPos);
		m_FileChangeEntries.RemoveAtPos(nPos);
		delete pFFC;
	}
	m_Sect.Leave();
	m_SectDelete.Leave();
}

bool FindFileChangeExThread::ValidDirectory(FindFileChangeExEntry *pFFC)
{
	FindFileChangeExEntry *pDummy;
	bool ret;
	m_Sect.Enter();
	ret = m_FileChangeEntries.Lookup(pFFC, pDummy);
	m_Sect.Leave();
	return ret;
}

bool FindFileChangeExThread::IsWatching()
{
	bool ret;
	m_Sect.Enter();
	ret = !m_FileChangeEntries.IsEmpty();
	m_Sect.Leave();
	return ret;
}

void FindFileChangeExThread::SignalThreadAbort()
{
	PostQueuedCompletionStatus(m_IoPort, 0, 0, 0);
}

DWORD FindFileChangeExThread::Run()
{
	DWORD dwBytes;
	FindFileChangeExEntry *pFFC;
	OVERLAPPED *pOl;
	while(true)
	{
		if (GetQueuedCompletionStatus(m_IoPort, &dwBytes, (PULONG_PTR)&pFFC, &pOl, INFINITE))
		{
			if (pFFC)
			{
				m_SectDelete.Enter();
				if (ValidDirectory(pFFC))
				{
					pFFC->ReadChanges();
					pFFC->PostCallback();
				}
				m_SectDelete.Leave();
			} else
				break;
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