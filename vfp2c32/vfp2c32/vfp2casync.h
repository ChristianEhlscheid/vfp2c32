#ifndef _THREADSAFE

#ifndef _VFP2CASYNC_H__
#define _VFP2CASYNC_H__

#include <malloc.h>
#include "vfp2chelpers.h"
#include "vfp2clockfree.h"

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
	virtual void Release();

	bool Setup(char *pPath, bool bWatchSubtree, DWORD nFilter, char *pCallback);

private:
	CEvent m_AbortEvent;
	HANDLE m_FileEvent;
	CStrBuilder<MAX_PATH+1> m_Path;
	CStrBuilder<VFP2C_MAX_CALLBACKBUFFER> m_Callback;
};

class FindFileChangeExThread;
class FindFileChangeExBuffer;

class FindFileChangeExEntry
{
public:
	FindFileChangeExEntry();
	~FindFileChangeExEntry();

	void Setup(FoxString &pPath, bool bWatchSubtree, DWORD nFilter, FoxString &pMethod, IDispatch *pCallbackObject, FindFileChangeExThread *pThread);
	void Callback(PFILE_NOTIFY_INFORMATION pInfo);
	void ErrorCallback(char *pFunc, DWORD nError);
	bool OnFileChangeEvent(DWORD dwBytes, FindFileChangeExThread *pThread);
	void Cancel(bool bAbort);
	bool IsValid();
	bool IsAborted();
	bool CompareDirectory(CStrBuilder<MAX_PATH+1> &pDirectory);
	bool CheckIdentity(DWORD nIdentity);
	USHORT GetIdentity();

	void* operator new(size_t sz)
	{
		return _aligned_malloc(sz, 128);
	}

	void operator delete(void* p)
	{
		_aligned_free(p);
	}

private:
	// data used mainly on background thread
	CACHELINE_ALIGN OVERLAPPED m_Ol;
	ApiHandle m_Handle;
	BOOL m_Subtree;
	DWORD m_Filter;
	USHORT m_Identity;
	CAutoPtr<FindFileChangeExBuffer> m_ReadDirBuffer;

	// data used on foreground thread
	CACHELINE_ALIGN bool m_Aborted;
	DWORD m_OldFilenameLength;
	WCHAR m_OldFilename[MAX_PATH];
	CStrBuilder<MAX_PATH+1> m_Path;
	CStrBuilder<MAX_PATH+1> m_Path2;
	FoxComCallback<4> m_Callback;
	CStrBuilder<VFP2C_MAX_CALLBACKBUFFER> m_CallbackBuffer;

	CACHELINE_ALIGN Atomic<LONG> m_Stopped;
};

class FindFileChangeExBuffer
{
public:
	FindFileChangeExBuffer() : m_Offset(0) { }
	LPBYTE Address(USHORT offset) { return reinterpret_cast<LPBYTE>(&m_Data[offset]); }
	void* Base() { return reinterpret_cast<void*>(&m_Data[m_Offset]); }
	USHORT Offset() { return static_cast<USHORT>(m_Offset); }
	unsigned int Size() { return sizeof(m_Data) - m_Offset; }
	bool NextOffset(DWORD dwBytes)
	{ 
		DWORD nOffset;
		nOffset = m_Offset + dwBytes + (128 - dwBytes % 128);
		if (nOffset < sizeof(m_Data) - 4096)
		{
			m_Offset = nOffset;
			return true;
		}
		return false;
	}

private:
	DWORD m_Offset;
	BYTE m_Data[16380];
};

enum FindFileChangeExAction
{
	ActionCallback,
	ActionFreeEntry,
	ActionFreeBuffer,
	ActionError
};

struct FindFileChangeExCallback
{
	FindFileChangeExAction Action;
	FindFileChangeExEntry *Entry;
	union
	{
		struct
		{
			FindFileChangeExBuffer *Buffer;
			USHORT Offset;
			USHORT Bytes;
		};
		struct 
		{
			DWORD ErrorCode;
			char *ErrorFunction;
		};
	};
};

template<>
struct CBoundSPSCQueueDestructor<FindFileChangeExCallback>
{
	static void release(FindFileChangeExCallback &cb)
	{
		if (cb.Action == ActionFreeBuffer)
		{
			delete cb.Buffer;
		} 
		else if (cb.Action == ActionFreeEntry)
		{
			delete cb.Entry;
		}
	}
};

class FindFileChangeExThread : public CThread
{
public:
	FindFileChangeExThread(CThreadManager &pManager);

	virtual void SignalThreadAbort();
	virtual DWORD Run();
	virtual void Release();

	void AddDirectory(FindFileChangeExEntry *pFFC);
	bool RemoveDirectory(char *pDirectory);
	bool RemoveDirectory(FindFileChangeExEntry *pFFC);
	bool RemoveDirectory(USHORT nFileChangeIdentity);
	void RemoveAllDirectories();
	bool IsWatching() const;

	void PushCallback(const FindFileChangeExCallback &pCallback);
	bool PeekCallback();
	bool PopCallback(FindFileChangeExCallback &pCallback);

	void AddIO();
	void ReleaseIO();

	void AddPostMessage();

	USHORT GetFileChangeIdentity();
	HANDLE GetIoPort() { return m_IoPort; }

	void* operator new(size_t sz)
	{
		return _aligned_malloc(sz, 128);
	}

	void operator delete(void* p)
	{
		_aligned_free(p);
	}

private:
	LONG OutstandingIOs();

	static USHORT m_FileChangeIdentity;
	ApiHandle m_IoPort;
	CAtlMap<USHORT, FindFileChangeExEntry*> m_FileChangeEntries;
	CUnboundBlockSPSCQueue<FindFileChangeExCallback, 4096> m_CallbackQueue;
	CACHELINE_ALIGN Atomic<LONG> m_OutstandingIOs;
	CACHELINE_ALIGN Atomic<LONG> m_RefCount;
};

class FindRegistryChangeThread : public CThread
{
public:
	FindRegistryChangeThread(CThreadManager &pManager);
	~FindRegistryChangeThread();

	virtual void SignalThreadAbort();
	virtual DWORD Run();
	virtual void Release();

	bool Setup(HKEY hRoot, char *pKey, bool bWatchSubtree, DWORD dwFilter, FoxString &pCallback);

private:
	CEvent m_RegistryEvent;
	CEvent m_AbortEvent;
	HKEY m_RegKey;
	bool m_WatchSubtree;
	DWORD m_Filter;
	CStrBuilder<VFP2C_MAX_CALLBACKBUFFER> m_Callback;
};

class WaitForObjectThread : public CThread
{
public:
	WaitForObjectThread(CThreadManager &pManager) : CThread(pManager) { }
	~WaitForObjectThread() { };

	virtual void SignalThreadAbort();
	virtual DWORD Run();
	virtual void Release();

	bool Setup(HANDLE hObject, FoxString &pCallback);

private:
	CEvent m_AbortEvent;
	HANDLE m_Object;
	CStrBuilder<VFP2C_MAX_CALLBACKBUFFER> m_Callback;
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