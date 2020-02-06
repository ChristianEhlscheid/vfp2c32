#ifndef _THREADSAFE

#ifndef _VFP2CASYNC_H__
#define _VFP2CASYNC_H__

#include "vfp2chelpers.h"

const UINT WM_CALLBACK			= (WM_USER+1);
const UINT WM_CALLBACKRESULT	= (WM_USER+2);
const UINT WM_CALLBACK_FFEX		= (WM_USER+3);

class FindFileChangeThread : public CThread
{
public:
	FindFileChangeThread(CThreadManager &pManager);
	~FindFileChangeThread();

	virtual void SignalThreadAbort();
	virtual DWORD Run();

	bool Setup(char *pPath, bool bWatchSubtree, DWORD nFilter, char *pCallback);

private:
	CStr m_Callback;
	CStr m_Buffer;
	CStr m_Path;
	CEvent m_AbortEvent;
	HANDLE m_FileEvent;
};

class FindFileChangeExEntry
{
public:
	FindFileChangeExEntry();
	~FindFileChangeExEntry();

	void Setup(char *pPath, bool bWatchSubtree, DWORD nFilter, char *pCallback, DWORD nBufferSize, HANDLE nIoPort);
	void Callback(PFILE_NOTIFY_INFORMATION pInfo);
	void PostCallback();
	void ReadChanges();
	DWORD BufferSize() { return m_BufferSize; }

private:
	ApiHandle m_Handle;
	OVERLAPPED m_Ol;
	WCHAR m_OldFilename[MAX_PATH];
	DWORD m_OldFilenameLength;
	BOOL m_Subtree;
	DWORD m_Filter;
	DWORD m_BufferSize;
	CStr m_Path;
	CStr m_LongPath;
	CStr m_LongPath2;
	CStr m_Callback;
	CStr m_CallbackError;
	CStr m_PostCallbackBuffer;
	CStr m_CallbackBuffer;
	CBuffer m_ReadDirBuffer;
	CBuffer m_PostBuffer;
};

class FindFileChangeExThread : public CThread
{
public:
	FindFileChangeExThread(CThreadManager &pManager);

	virtual void SignalThreadAbort();
	virtual DWORD Run();

	void AddDirectory(FindFileChangeExEntry *pFFC);
	bool RemoveDirectory(FindFileChangeExEntry *pFFC);
	bool ValidDirectory(FindFileChangeExEntry *pFFC);
	void RemoveAllDirectories();
	bool IsWatching();
	HANDLE GetIoPort() { return m_IoPort; }

private:
	ApiHandle m_IoPort;
	CCriticalSection m_Sect;
	CCriticalSection m_SectDelete;
	CAtlMap<FindFileChangeExEntry*,FindFileChangeExEntry*> m_FileChangeEntries;
};

class FindRegistryChangeThread : public CThread
{
public:
	FindRegistryChangeThread(CThreadManager &pManager);
	~FindRegistryChangeThread();

	virtual void SignalThreadAbort();
	virtual DWORD Run();

	bool Setup(HKEY hRoot, char *pKey, bool bWatchSubtree, DWORD dwFilter, char *pCallback);

private:
	CStr m_Callback;
	CStr m_Buffer;
	CEvent m_RegistryEvent;
	CEvent m_AbortEvent;
	HKEY m_RegKey;
	bool m_WatchSubtree;
	DWORD m_Filter;
};

class WaitForObjectThread : public CThread
{
public:
	WaitForObjectThread(CThreadManager &pManager) : CThread(pManager) { }
	~WaitForObjectThread() { };

	virtual void SignalThreadAbort();
	virtual DWORD Run();

	bool Setup(HANDLE hObject, char *pCallback);

private:
	CStr m_Callback;
	CStr m_Buffer;
	CEvent m_AbortEvent;
	HANDLE m_Object;
};

#ifdef __cplusplus
extern "C" {
#endif

// function forward definitions of vfp2casync.c
int _stdcall VFP2C_Init_Async();
void _stdcall VFP2C_Destroy_Async(VFP2CTls& tls);

void _fastcall FindFileChange(ParamBlk *parm);
void _fastcall CancelFileChange(ParamBlk *parm);

void _fastcall FindFileChangeEx(ParamBlk *parm);
void _fastcall CancelFileChangeEx(ParamBlk *parm);

void _fastcall FindRegistryChange(ParamBlk *parm);
void _fastcall CancelRegistryChange(ParamBlk *parm);

void _fastcall AsyncWaitForObject(ParamBlk *parm);
void _fastcall CancelWaitForObject(ParamBlk *parm);

LRESULT _stdcall FindChangeWindowProc(HWND nHwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern HWND ghAsyncHwnd;
extern CThreadManager goThreadManager;

#ifdef __cplusplus
}
#endif // extern "C"

#endif // _VFP2CASYNC_H__

#endif // _THREADSAFE