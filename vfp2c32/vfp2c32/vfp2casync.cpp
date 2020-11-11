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

	FoxString pPath(vp1);
	bool bWatchSubtree = vp2.ev_length > 0;
	DWORD nFilter = vp3.ev_long;
	FoxString pCallback(vp4);

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

	CThread *pThread = reinterpret_cast<CThread*>(vp1.ev_long);
	Return(goThreadManager.AbortThread(pThread));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FindFileChangeEx(ParamBlk *parm)
{
	FindFileChangeExThread *pThread = 0;
	FindFileChangeExEntry *pFFC = 0;
	CComPtr<IDispatch> pCallbackObject;
try
{
	int nErrorNo = VFP2C_Init_Async();
	if (nErrorNo)
		throw nErrorNo;

	FoxString pPath(vp1);
	bool bWatchSubtree = vp2.ev_length > 0;
	DWORD nFilter = vp3.ev_long;
	FoxString pCallback(vp4);
	FoxObject pObject(parm, 5);

    if (pPath.Len() > MAX_PATH-1)
		throw E_INVALIDPARAMS;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION || pCallback.Len() == 0)
		throw E_INVALIDPARAMS;

	if (PCount() > 4)
	{
		pCallbackObject = pObject.GetIDispatch();
	}

	if (goFindFileChangeExThread == 0) {
		pThread = new FindFileChangeExThread(goThreadManager);
		if (pThread == 0)
			throw E_INSUFMEMORY;
		goFindFileChangeExThread = pThread;
	}

	// find free slot
	pFFC = new FindFileChangeExEntry();
	if (pFFC == 0)
		throw E_INSUFMEMORY;

	pFFC->Setup(pPath, bWatchSubtree, nFilter, pCallback, pCallbackObject, goFindFileChangeExThread);

	goFindFileChangeExThread->AddDirectory(pFFC);
	goFindFileChangeExThread->StartThread();
	pCallbackObject.Detach();
	Return(pFFC);
}
catch(int nErrorNo)
{
	if (pFFC) {
		goFindFileChangeExThread->RemoveDirectory(pFFC);
		delete pFFC;
	}
	if (pThread)
	{
		goFindFileChangeExThread = 0;
		delete pThread;
	}
	RaiseError(nErrorNo);
}
}

void _fastcall CancelFileChangeEx(ParamBlk *parm)
{
try
{
	bool ret;
	int nErrorNo = VFP2C_Init_Async();
	if (nErrorNo)
		throw nErrorNo;

	if (goFindFileChangeExThread)
	{
		if (Vartype(vp1) == 'N' || Vartype(vp1) == 'I')
		{
			if (Vartype(vp1) == 'N')
				vp1.ev_long = static_cast<long>(vp1.ev_real);
			FindFileChangeExEntry *pFFC = reinterpret_cast<FindFileChangeExEntry*>(vp1.ev_long);
			if (pFFC)
			{
				ret = goFindFileChangeExThread->RemoveDirectory(pFFC);
			}
			else 
			{
				goFindFileChangeExThread->RemoveAllDirectories();
				ret = true;
			}
		}
		else if (Vartype(vp1) == 'C')
		{
			FoxString pDir(vp1);
			ret = goFindFileChangeExThread->RemoveDirectory(pDir);
		}
		else
			throw E_INVALIDPARAMS;

		if (!goFindFileChangeExThread->IsWatching())
		{
			goThreadManager.AbortThread(goFindFileChangeExThread);
			goFindFileChangeExThread = 0;
		}
		Return(ret);
	} else
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

	HKEY hRoot = reinterpret_cast<HKEY>(vp1.ev_long);
	FoxString pKey(vp2);
	bool bWatchSubtree = vp3.ev_length > 0;
	DWORD dwFilter = vp4.ev_long;
	FoxString pCallback(vp5);

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

	CThread *pThread = reinterpret_cast<CThread*>(vp1.ev_long);
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

	HANDLE hObject = reinterpret_cast<HANDLE>(vp1.ev_long);
	FoxString pCallback(vp2);

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

	CThread *pThread = reinterpret_cast<CThread*>(vp1.ev_long);
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
	char *pCommand;
	LRESULT retval;

	switch(uMsg)
	{
		case WM_CALLBACK:
			pCommand = reinterpret_cast<char*>(wParam);
			if (pCommand)
			{
				_Execute(pCommand);
				delete[] pCommand;
			}
			return 0;

		case WM_CALLBACKRESULT:
			Value vRetVal;
			vRetVal.ev_type = '0';
			retval = 0;
			pCommand = reinterpret_cast<char*>(wParam);
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
			return retval;

		case WM_CALLBACK_FFEX:
			FindFileChangeExThread *pThread = reinterpret_cast<FindFileChangeExThread*>(wParam);
			if (goFindFileChangeExThread != pThread)
			{
				pThread->Release();
				return 0;
			}
			if (pThread->PeekCallback())
			{
				AutoYieldLock ay;

				FindFileChangeExCallback cb;
				while(pThread->PopCallback(cb))
				{
					switch(cb.Action)
					{
						case ActionCallback:
							if (!cb.Entry->IsAborted()) 
							{
								LPBYTE pStart = cb.Buffer->Address(cb.Offset);
								LPBYTE pEnd = pStart + cb.Bytes - sizeof(FILE_NOTIFY_INFORMATION);
								DWORD nOffset = 0;
								PFILE_NOTIFY_INFORMATION pInfo = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(pStart);
								while(true)
								{
									cb.Entry->Callback(pInfo);
									if (cb.Entry->IsAborted() || pInfo->NextEntryOffset == 0)
										break;

									nOffset += pInfo->NextEntryOffset;
									pInfo = (PFILE_NOTIFY_INFORMATION)(pStart + nOffset);
									if ((LPBYTE)pInfo >= pEnd)
										break;
								}
							}
							break;

						case ActionFreeEntry:
							delete cb.Entry;
							break;

						case ActionFreeBuffer:
							delete cb.Buffer;
							break;

						case ActionError:
							cb.Entry->ErrorCallback(cb.ErrorFunction, cb.ErrorCode);
							pThread->RemoveDirectory(cb.Entry->GetIdentity());
							break;
					}
					
				}
			}
			pThread->Release();
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
	m_Callback.SetFormatBase();

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

void FindFileChangeThread::Release()
{
	delete this;
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
				m_Callback.AppendFormatBase("(%U,'%S',%U)", this, (char*)m_Path, 0);
				pCallback = m_Callback.Strdup();
				if (pCallback)
					PostMessage(ghAsyncHwnd, WM_CALLBACK, reinterpret_cast<WPARAM>(pCallback), 0);

				if (!FindNextChangeNotification(m_FileEvent))
				{
					bLoop = false;
					m_Callback.AppendFormatBase("(%U,'%S',%U)", this, "FindNextChangeNotification", GetLastError());
					pCallback = m_Callback.Strdup();
					if (pCallback)
				        PostMessage(ghAsyncHwnd, WM_CALLBACK, reinterpret_cast<WPARAM>(pCallback), 0);
				}
				break;

			case WAIT_OBJECT_0 + 1: // notification canceled
				bLoop = false;
				break;

			case WAIT_FAILED:
				bLoop = false;
				m_Callback.AppendFormatBase("(%U,'%S',%U)", this, "WaitForMultipleObjects", GetLastError());
				pCallback = m_Callback.Strdup();
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
	m_Aborted = FALSE;
	m_Stopped.Store(TRUE);
}

FindFileChangeExEntry::~FindFileChangeExEntry()
{
	Cancel(false);
}

void FindFileChangeExEntry::Setup(FoxString &pPath, bool bWatchSubtree, DWORD nFilter, FoxString &pMethod, 
								  IDispatch *pCallbackObject, FindFileChangeExThread *pThread)
{
	m_Handle = CreateFile(pPath, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
	if (!m_Handle)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}
	m_Path = pPath;
	m_Path.LongPathName().AddBs();
	m_Path.SetFormatBase();
	m_Path2 = m_Path;
	m_Path2.SetFormatBase();
	if (pCallbackObject)
	{
		m_Callback.Initialize(pCallbackObject, pMethod);
	}
	else
	{
		m_CallbackBuffer = pMethod;
		m_CallbackBuffer.SetFormatBase();
	}
	m_ReadDirBuffer.Attach(new FindFileChangeExBuffer());
	if (m_ReadDirBuffer == 0)
		throw E_INSUFMEMORY;

	m_Subtree = bWatchSubtree;
	m_Filter = nFilter;
	m_Identity = pThread->GetFileChangeIdentity();

	if (!CreateIoCompletionPort(m_Handle, pThread->GetIoPort(), (ULONG_PTR)m_Identity, 1))
	{
		SaveWin32Error("CreateIoCompletionPort", GetLastError());
		throw E_APIERROR;
	}

	pThread->AddIO();
	m_Stopped.Store(FALSE);

	if (!ReadDirectoryChangesW(m_Handle, m_ReadDirBuffer->Base(), m_ReadDirBuffer->Size(), bWatchSubtree, nFilter, 0, (LPOVERLAPPED)this, 0))
	{
		m_Stopped.Store(TRUE);
		pThread->ReleaseIO();
		SaveWin32Error("ReadDirectoryChangesW", GetLastError());
		throw E_APIERROR;
	}
}

bool FindFileChangeExEntry::OnFileChangeEvent(DWORD dwBytes, FindFileChangeExThread *pThread)
{
	DWORD errorCode = 0;
	char *errorFunc;
	FindFileChangeExCallback cb;

	cb.Action = ActionCallback;
	cb.Entry = this;
	cb.Buffer = m_ReadDirBuffer;
	cb.Bytes = static_cast<USHORT>(dwBytes);
	cb.Offset = m_ReadDirBuffer->Offset();

	if (m_ReadDirBuffer->NextOffset(dwBytes))
	{
		pThread->PushCallback(cb);
	}
	else
	{
		pThread->PushCallback(cb);
		cb.Action = ActionFreeBuffer;
		cb.Entry = this;
		cb.Buffer = m_ReadDirBuffer.Detach();
		cb.Bytes = 0;
		cb.Offset = 0;
		pThread->PushCallback(cb);
		m_ReadDirBuffer.Attach(new FindFileChangeExBuffer());
	}

	if (m_ReadDirBuffer == 0)
	{
		errorFunc = "new";
		errorCode = E_INSUFMEMORY;
	}
	else if (!ReadDirectoryChangesW(m_Handle, m_ReadDirBuffer->Base(), m_ReadDirBuffer->Size(), m_Subtree, m_Filter, 0, (LPOVERLAPPED)this, 0))
	{
		errorFunc = "ReadDirectoryChangesW";
		errorCode = GetLastError();
	}
	else
	{
		pThread->AddPostMessage();
		return true;
	}

	Cancel(false);

	cb.Action = ActionError;
	cb.Entry = this;
	cb.ErrorCode = errorCode;
	cb.ErrorFunction = errorFunc;
	pThread->PushCallback(cb);

	pThread->AddPostMessage();
	return false;
}

void FindFileChangeExEntry::Callback(PFILE_NOTIFY_INFORMATION pInfo)
{
	switch(pInfo->Action)
	{
		case FILE_ACTION_ADDED:
		case FILE_ACTION_REMOVED:
		case FILE_ACTION_MODIFIED:
			m_Path.AppendFormatBase("%W", &pInfo->FileName, pInfo->FileNameLength / 2);
			if (pInfo->Action != FILE_ACTION_REMOVED)
			{
				GetLongPathName(m_Path, m_Path, m_Path.Size());
			}
			if (m_Callback)
			{
				m_Callback.Call((void*)this, pInfo->Action, (CStrView)m_Path);
			} 
			else
			{
				m_CallbackBuffer.AppendFormatBase("(%U,%U,ReadCString(%U))", this, pInfo->Action, (char*)m_Path);
				_Execute(m_CallbackBuffer);
			}
			break;

		case FILE_ACTION_RENAMED_OLD_NAME:
			memcpy(&m_OldFilename, &pInfo->FileName, pInfo->FileNameLength);
			m_OldFilenameLength = pInfo->FileNameLength;
			break;

		case FILE_ACTION_RENAMED_NEW_NAME:
			m_Path.AppendFormatBase("%W", &m_OldFilename, m_OldFilenameLength / 2);
			m_Path2.AppendFormatBase("%W", &pInfo->FileName, pInfo->FileNameLength / 2);
			if (m_Callback)
			{
				m_Callback.Call((void*)this, FILE_ACTION_RENAMED_OLD_NAME, (CStrView)m_Path, (CStrView)m_Path2);
			} 
			else
			{
				// GetLongPathName(m_Path2, m_Path2, m_Path2.Size());
				m_CallbackBuffer.AppendFormatBase("(%U,%U,ReadCString(%U),ReadCString(%U))", this, FILE_ACTION_RENAMED_OLD_NAME, (char*)m_Path, (char*)m_Path2);
				_Execute(m_CallbackBuffer);
			}
			m_OldFilenameLength = 0;
			break;
	}
}

void FindFileChangeExEntry::ErrorCallback(char *pFunc, DWORD nError)
{
	if (m_Callback)
	{
		m_Callback.Call((void*)this, 0, pFunc, nError);
	}
	else
	{
		m_CallbackBuffer.AppendFormatBase("(%U,%U,ReadCString(%U),%U)", this, 0, pFunc, nError);
		_Execute(m_CallbackBuffer);
	}
}

bool FindFileChangeExEntry::IsValid()
{
	return m_Stopped.Load() == FALSE;
}

bool FindFileChangeExEntry::IsAborted()
{
	return m_Aborted;
}

bool FindFileChangeExEntry::CheckIdentity(DWORD nIdentity)
{
	return m_Identity == nIdentity;
}

USHORT FindFileChangeExEntry::GetIdentity()
{
	return m_Identity;
}

bool FindFileChangeExEntry::CompareDirectory(CStrBuilder<MAX_PATH+1> &pDirectory)
{
	return m_Path.CompareToBase(pDirectory);
}

void FindFileChangeExEntry::Cancel(bool bSetAbort)
{
	if (bSetAbort)
	{
		m_Aborted = true;
		m_Callback.Release();
	}
	if (m_Handle)
	{
        if (m_Stopped.Exchange(TRUE) == FALSE)
			CancelIo(m_Handle);
		m_Handle.Close();
	} 
}

FindFileChangeExThread::FindFileChangeExThread(CThreadManager &pManager) : CThread(pManager)
{
	m_OutstandingIOs.Store(1);
	m_RefCount.Store(1);
	m_IoPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
	if (!m_IoPort)
	{
		SaveWin32Error("CreateIoCompletionPort", GetLastError());
		throw E_APIERROR;
	}
}

void FindFileChangeExThread::AddDirectory(FindFileChangeExEntry *pFFC)
{
	m_FileChangeEntries[pFFC->GetIdentity()] = pFFC;
}

bool FindFileChangeExThread::RemoveDirectory(FindFileChangeExEntry *pFFC)
{
	bool ret = false;
	POSITION nPos = m_FileChangeEntries.GetStartPosition();
	while (nPos)
	{
		if (ret = m_FileChangeEntries.GetValueAt(nPos) == pFFC)
		{
			m_FileChangeEntries.RemoveAtPos(nPos);
			break;
		}
		m_FileChangeEntries.GetNextValue(nPos);
	}
	if (ret)
	{
		pFFC->Cancel(true);
	}
	return ret;
}

bool FindFileChangeExThread::RemoveDirectory(USHORT nFileChangeIdentity)
{
	bool ret;
	FindFileChangeExEntry *pFFC;
	if (ret = m_FileChangeEntries.Lookup(nFileChangeIdentity, pFFC))
	{
		m_FileChangeEntries.RemoveKey(nFileChangeIdentity);
	}
	if (ret)
	{
		pFFC->Cancel(true);
	}
	return ret;
}

bool FindFileChangeExThread::RemoveDirectory(char *pDirectory)
{
	CStrBuilder<MAX_PATH+1> path;
	path = pDirectory;
	path.LongPathName().AddBs();
	FindFileChangeExEntry *pFFC;
	bool ret = false;
	POSITION nPos = m_FileChangeEntries.GetStartPosition();
	if (nPos)
	{
		pFFC = m_FileChangeEntries.GetValueAt(nPos);
		do 
		{
			if (ret = pFFC->CompareDirectory(path))
			{
				m_FileChangeEntries.RemoveAtPos(nPos);
				break;
			}
			pFFC = m_FileChangeEntries.GetNextValue(nPos);
		}
		while(nPos);
	}
	if (ret)
	{
		pFFC->Cancel(true);
	}
	return ret;
}


void FindFileChangeExThread::RemoveAllDirectories()
{
	FindFileChangeExEntry *pFFC;
	POSITION nPos;
	while(nPos = m_FileChangeEntries.GetStartPosition())
	{
		pFFC = m_FileChangeEntries.GetValueAt(nPos);
		m_FileChangeEntries.RemoveAtPos(nPos);
		pFFC->Cancel(true);
	}
}

bool FindFileChangeExThread::IsWatching() const
{
	return m_FileChangeEntries.GetCount() > 0;
}

USHORT FindFileChangeExThread::m_FileChangeIdentity = 0;
USHORT FindFileChangeExThread::GetFileChangeIdentity()
{
	USHORT nIdentity;
	bool bFound;
	FindFileChangeExEntry* pFFC;
	do 
	{
		nIdentity = ++m_FileChangeIdentity;
		bFound = m_FileChangeEntries.Lookup(nIdentity, pFFC);
	} while(bFound);
	return nIdentity;
}

void FindFileChangeExThread::PushCallback(const FindFileChangeExCallback &pCallback)
{
	m_CallbackQueue.Push(pCallback);
}

bool FindFileChangeExThread::PeekCallback()
{
	return m_CallbackQueue.Peek();
}

bool FindFileChangeExThread::PopCallback(FindFileChangeExCallback &pCallback)
{
	return m_CallbackQueue.Pop(pCallback);
}

void FindFileChangeExThread::AddPostMessage()
{
	LONG count = m_RefCount.Load();
	if (count < 3)
	{
		m_RefCount.Increment();
		if (!PostMessage(ghAsyncHwnd, WM_CALLBACK_FFEX, (WPARAM)this, 0))
		{
			m_RefCount.Decrement();
		}
	}
}

void FindFileChangeExThread::AddIO()
{
	m_OutstandingIOs.Increment();
}

void FindFileChangeExThread::ReleaseIO()
{
	m_OutstandingIOs.Decrement();
}

LONG FindFileChangeExThread::OutstandingIOs()
{
	return m_OutstandingIOs.Load();
}

void FindFileChangeExThread::SignalThreadAbort()
{
	PostQueuedCompletionStatus(m_IoPort, 0, 0, 0);
}

void FindFileChangeExThread::Release()
{
	DWORD count = m_RefCount.Decrement();
	if (count == 0)
		delete this;
}

DWORD FindFileChangeExThread::Run()
{
	DWORD dwBytes, nIdentity;
	FindFileChangeExEntry *pFFC;
	BOOL result;
	DWORD lastError;
	FindFileChangeExCallback cb;
	cb.ErrorCode = 0;
	cb.ErrorFunction = 0;

	while(OutstandingIOs() > 0)
	{
		result = GetQueuedCompletionStatus(m_IoPort, &dwBytes, (PULONG_PTR)&nIdentity, (LPOVERLAPPED*)&pFFC, INFINITE);
		if (result)
		{
			if (pFFC)
			{
				if (dwBytes > 0)
				{
					if (pFFC->IsValid() && pFFC->CheckIdentity(nIdentity))
					{
						if (pFFC->OnFileChangeEvent(dwBytes, this))
							AddIO();
					}
				}
				else 
				{
					cb.Action = ActionFreeEntry;
					cb.Entry = pFFC;
					PushCallback(cb);
					AddPostMessage();
				}
				ReleaseIO();
			} 
			else if (/* pFFC == 0 && */ dwBytes == 0 && nIdentity == 0)
				ReleaseIO();
		}
		else
		{
			lastError = GetLastError();
			if (lastError == ERROR_OPERATION_ABORTED || lastError == 735 /* ERROR_ABANDONED_WAIT_0 */)
			{
				if (pFFC)
				{
					cb.Action = ActionFreeEntry;
					cb.Entry = pFFC;
					PushCallback(cb);
					AddPostMessage();
				}
				ReleaseIO();
			}
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

bool FindRegistryChangeThread::Setup(HKEY hRoot, char *pKey, bool bWatchSubtree, DWORD dwFilter, FoxString &pCallback)
{
	m_WatchSubtree = bWatchSubtree;
	m_Filter = dwFilter;
	m_Callback = pCallback;
	m_Callback.SetFormatBase();

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

void FindRegistryChangeThread::Release()
{
	delete this;
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
				m_Callback.AppendFormatBase("(%U,%I)", this, 0);
				pCallback = m_Callback.Strdup();
				if (pCallback)
					PostMessage(ghAsyncHwnd, WM_CALLBACK, (WPARAM)pCallback, 0); // we need to use PostMessage cause it's asynchronous .. SendMessage can bring us into a deadlock situation

				nRetVal = RegNotifyChangeKeyValue(m_RegKey, m_WatchSubtree, m_Filter, m_RegistryEvent, TRUE); // continue watching
				if (nRetVal != ERROR_SUCCESS)
				{
					bLoop = false;
					m_Callback.AppendFormatBase("(%U,%I)", this, nRetVal);
					pCallback = m_Callback.Strdup();
					if (pCallback)
				        PostMessage(ghAsyncHwnd, WM_CALLBACK, (WPARAM)pCallback, 0);
				}
				break;

			case WAIT_OBJECT_0 + 1: // notification canceled
				bLoop = false;
				break;

			case WAIT_FAILED:
				bLoop = false;
				m_Callback.AppendFormatBase("(%U,%I)", this, GetLastError());
				pCallback = m_Callback.Strdup();
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

void WaitForObjectThread::Release()
{
	delete this;
}

bool WaitForObjectThread::Setup(HANDLE hObject, FoxString &pCallback)
{
	m_Object = hObject;
	m_Callback = pCallback;
	m_Callback.SetFormatBase();

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
			m_Callback.AppendFormatBase("(%U)", 0);
			pCallback = m_Callback.Strdup();
			if (pCallback)
				PostMessage(ghAsyncHwnd, WM_CALLBACK, reinterpret_cast<WPARAM>(pCallback), 0); // we need to use PostMessage cause it's asynchronous .. SendMessage can bring us into a deadlock situation
			break;

		case WAIT_OBJECT_0 + 1: // notification canceled
			break;

		case WAIT_FAILED:
			m_Callback.AppendFormatBase("(%U)", GetLastError());
			pCallback = m_Callback.Strdup();
			if (pCallback)
				PostMessage(ghAsyncHwnd, WM_CALLBACK, reinterpret_cast<WPARAM>(pCallback), 0);
	}

	return 0;
}

#endif // _THREADSAFE