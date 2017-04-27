#ifndef _VFP2CHELPERS_H__
#define _VFP2CHELPERS_H__

#include <windows.h>
#include <shlwapi.h>
#include <assert.h>
#include <atlcoll.h>

#define VER_SUITE_WH_SERVER 0x8000

typedef void (_stdcall *PGETNATIVESYSTEMINFO)(LPSYSTEM_INFO);

typedef enum WindowsVersion
{
	Windows,
	Windows95,
	Windows95OSR2,
	Windows98,
	Windows98SE,
	WindowsMillennium,
	WindowsNT351,
	WindowsNT40,
	WindowsNT40Server,
	Windows2000,
	WindowsXP,
	WindowsXPProfessionalx64,
	WindowsHomeServer,
	WindowsVista,
	WindowsServer2003,
	WindowsServer2003R2,
	Windows7,
	WindowsServer2008,
	WindowsServer2008R2,
	WindowsX
};

class COs
{
public:
	COs();
	~COs();

	static void Init();
	static WindowsVersion GetVersion();
	static bool Is(WindowsVersion vVersion);
	static bool GreaterOrEqual(WindowsVersion vVersion);

private:
	static WindowsVersion m_Version;
	static OSVERSIONINFOEX m_osver;
};

inline WindowsVersion COs::GetVersion()
{
	return m_Version;
}

inline bool COs::Is(WindowsVersion vVersion)
{
	return m_Version == vVersion;
}

inline bool COs::GreaterOrEqual(WindowsVersion vVersion)
{
	return m_Version >= vVersion;
}

/* Some helper classes */
class CStr
{
public:
	CStr() : m_String(0), m_Size(0), m_Length(0) {}
	CStr(unsigned int nSize);
	~CStr();

	unsigned int Len() const { return m_Length; }
	CStr& Len(unsigned int nLen) { assert(nLen <= m_Size); m_Length = nLen; return *this; }
	unsigned int Size() const { return m_Size; }
	void Size(unsigned int nSize);

	CStr& AddBs();
	CStr& AddBsWc();
	CStr& AddLastPath(const char *pPath);
	CStr& Justpath();
	bool IsWildcardPath() const;
	char* Strdup() const;
	CStr& RegValueToPropertyName();
	unsigned int Format(const char *pFormat,...);

	CStr& operator=(const CStr &pString);
	CStr& operator=(const char *pString);
	CStr& operator+=(const CStr &pString);
	CStr& operator+=(const char *pString);
	bool operator==(const char *pString) const;

	operator char*() const { return m_String; }
	operator unsigned char*() const { return reinterpret_cast<unsigned char*>(m_String); }
	operator void*() const { return reinterpret_cast<void*>(m_String); }

private:
	char *m_String;
	unsigned int m_Size;
	unsigned int m_Length;
};

class CBuffer
{
public:
	CBuffer() : m_Pointer(0) {}
	CBuffer(unsigned int nSize);
	~CBuffer();

	void Size(unsigned int nSize);
	unsigned int Size() const {	return m_Size; }
	void* Address() { return static_cast<void*>(m_Pointer); }

	operator void*() const { return static_cast<void*>(m_Pointer); }
	operator char*() const { return m_Pointer; }
	operator unsigned char*() const { return reinterpret_cast<unsigned char*>(m_Pointer); }

private:
	char *m_Pointer;
	unsigned int m_Size;
};

class AbstractHandle
{
public:
 	AbstractHandle() : m_Handle(INVALID_HANDLE_VALUE) { }
	~AbstractHandle() {}

	operator HANDLE() { return m_Handle; }
	operator LPHANDLE() { return &m_Handle; }
	bool operator!() { return m_Handle == INVALID_HANDLE_VALUE; }
	operator bool() { return m_Handle != INVALID_HANDLE_VALUE; }
	HANDLE Detach() { HANDLE hTmp = m_Handle; m_Handle = INVALID_HANDLE_VALUE; return hTmp; }

protected:
	HANDLE m_Handle;
};

class ApiHandle : public AbstractHandle
{
public:
  	ApiHandle() {}
	ApiHandle(HANDLE hHandle) { m_Handle = hHandle; }
	~ApiHandle() { if (m_Handle != INVALID_HANDLE_VALUE) CloseHandle(m_Handle); }
	void operator=(HANDLE hHandle) { if (m_Handle != INVALID_HANDLE_VALUE) CloseHandle(m_Handle); m_Handle = hHandle; }
	void Close() { if (m_Handle != INVALID_HANDLE_VALUE) CloseHandle(m_Handle); m_Handle = INVALID_HANDLE_VALUE; }
};

class PrinterHandle : public AbstractHandle
{
public:
	PrinterHandle() {};
	PrinterHandle(HANDLE hHandle) { m_Handle = hHandle; }
	~PrinterHandle() { if (m_Handle != INVALID_HANDLE_VALUE) ClosePrinter(m_Handle); }
	void operator=(HANDLE hHandle) { if (m_Handle != INVALID_HANDLE_VALUE) ClosePrinter(m_Handle); m_Handle = hHandle; }
};

class RegistryKey
{
public:
	RegistryKey() : m_Handle(0), m_Owner(true) { }
	~RegistryKey() { if (m_Handle != 0 && m_Owner) RegCloseKey(m_Handle); }

	bool Create(HKEY hKey, LPCSTR lpKey, LPSTR lpClass = 0, DWORD nOptions = REG_OPTION_NON_VOLATILE,
		REGSAM samDesired = KEY_ALL_ACCESS);
	void Open(HKEY hKey, LPCSTR lpKey, REGSAM samDesired, DWORD ulOptions = 0);
	HKEY OpenSubKey(LPCSTR lpSubKey, REGSAM samDesired, DWORD ulOptions = 0);
	bool Delete(HKEY hKey, LPCSTR lpKey, bool bShellVersion = false);
	void QueryInfo(LPSTR lpClass, LPDWORD lpcbClass = 0, LPDWORD lpSubKeys = 0, 
				  LPDWORD lpcbMaxSubKeyLen = 0, LPDWORD lpcbMaxClassLen = 0,
				  LPDWORD lpcValues = 0, LPDWORD lpcbMaxValueNameLen = 0,
				  LPDWORD lpcbMaxValueLen = 0, LPDWORD lpcbSecurityDescriptor = 0,
				  PFILETIME lpftLastWriteTime = 0);
	DWORD QueryValueInfo(LPSTR lpValue, DWORD &lpType);
	void QueryValue(LPSTR lpValue, LPBYTE lpData, LPDWORD lpDataLen, LPDWORD lpType = 0);
	void RegistryKey::SetValue(LPCSTR lpValueName, LPBYTE lpData, DWORD dwDataLen, DWORD dwType);
	bool EnumFirstKey(LPSTR lpKey, LPDWORD lpKeyLen, LPSTR lpClass = 0, LPDWORD lpClassLen = 0,
							   PFILETIME lpftLastWriteTime = 0);
	bool EnumNextKey(LPSTR lpKey, LPDWORD lpKeyLen, LPSTR lpClass = 0, LPDWORD lpClassLen = 0,
							   PFILETIME lpftLastWriteTime = 0);
	bool EnumFirstValue(LPSTR lpValueName, LPDWORD lpcbValueName, LPBYTE lpData, LPDWORD lpcbData, LPDWORD lpType);
	bool EnumNextValue(LPSTR lpValueName, LPDWORD lpcbValueName, LPBYTE lpData, LPDWORD lpcbData, LPDWORD lpType);
		
	HKEY Detach() { m_Owner = false; return m_Handle; }
	operator HKEY() { return m_Handle; }
	RegistryKey& operator=(HKEY hKey);
	
private:
	HKEY m_Handle;
	bool m_Owner;
	DWORD m_KeyIndex;
	DWORD m_ValueIndex;
};

template<class T>
class ComPtr
{
public:
	ComPtr() : m_Pointer(0) {}
	~ComPtr() { if (m_Pointer) m_Pointer->Release(); }

	operator T() { return m_Pointer; }
	operator T*() { return &m_Pointer; }
	T operator->() { return m_Pointer; }
	T operator=(int nValue) { if (m_Pointer) m_Pointer->Release(); m_Pointer = 0; return m_Pointer; }
	T operator=(T pValue) { if (m_Pointer) m_Pointer->Release(); m_Pointer = pValue; return m_Pointer; }

private:
	T m_Pointer;
};

template<class T, int count>
class ComPtrArray
{
public:
	ComPtrArray() { ZeroMemory(m_Pointer, sizeof(m_Pointer)); }
	~ComPtrArray() { for(int xj = 0; xj < count; xj++) if (m_Pointer[xj] != 0) m_Pointer[xj]->Release(); }

	T operator[](int nIndex) { return m_Pointer[nIndex]; }
	operator T*() { return m_Pointer; }

private:
	T m_Pointer[count];
};

class CoTaskPtr
{
public:
	CoTaskPtr() : m_Pointer(0) {}
	~CoTaskPtr() { if (m_Pointer != 0) CoTaskMemFree(m_Pointer); }

	operator LPITEMIDLIST() { return reinterpret_cast<LPITEMIDLIST>(m_Pointer); }
	operator LPITEMIDLIST*() { return reinterpret_cast<LPITEMIDLIST*>(&m_Pointer); }
	operator LPWSTR() { return reinterpret_cast<LPWSTR>(m_Pointer); }
	operator LPWSTR*() { return reinterpret_cast<LPWSTR*>(&m_Pointer); }
	operator LPVOID() { return m_Pointer; }
	void operator=(LPITEMIDLIST pList) { if (m_Pointer) CoTaskMemFree(m_Pointer); m_Pointer = pList; }
	operator bool() { return m_Pointer > 0; }

private:
	LPVOID m_Pointer;
};

class CCriticalSection
{
public:
	CCriticalSection() : m_Inited(false) { ZeroMemory(&m_Section,sizeof(CRITICAL_SECTION)); }
	~CCriticalSection() { if (m_Inited) DeleteCriticalSection(&m_Section); }

	bool Initialize(DWORD spinCount = 0);
	bool Initialized();
	void Enter();
	void Leave();

private:
	CRITICAL_SECTION m_Section;
	bool m_Inited;
};

class CEvent
{
public:
	CEvent() : m_Event(0) { }
	CEvent(HANDLE hEvent) : m_Event(hEvent) { }
	~CEvent() { Close(); }

	bool Create(bool bManualReset = true, bool bInitalState = false, char *pName = 0, LPSECURITY_ATTRIBUTES pSecurityAttributes = 0)
	{
		m_Event = CreateEvent(pSecurityAttributes, bManualReset ? TRUE : FALSE, bInitalState ? TRUE : FALSE, pName);
		if (m_Event == 0)
		{
			SaveWin32Error("CreateEvent", GetLastError());
			return false;
		}
		return true;
	}

	BOOL Signal() { return ::SetEvent(m_Event); }
	BOOL Reset() { return ::ResetEvent(m_Event); }
	void Close() { if (m_Event) ::CloseHandle(m_Event); m_Event = 0; }
	void Attach(HANDLE hEvent) { m_Event = hEvent; }
	HANDLE Detach()
	{
		HANDLE hTmp;
		hTmp = m_Event;
		m_Event = 0;
		return hTmp;
	}
	operator HANDLE() { return m_Event; }
	operator bool() { return m_Event != NULL; }

private:
	HANDLE m_Event;
};

const int THREAD_STACKSIZE		= 1024 * 32; /* 32 KB should be sufficient stackspace per thread */
const int THREAD_ABORT_FLAG		= 1;
const int THREAD_SHUTDOWN_FLAG	= 2;

class CThreadManager;

class CThread
{
public:
	CThread(CThreadManager &pManager) : m_ThreadManager(&pManager), m_ThreadHandle(0), m_AbortFlag(0) { }
	~CThread();
	void WaitForThreadShutdown();
	void StartThread();
	CThreadManager* GetThreadManager() { return m_ThreadManager; }
	bool IsShutdown() { return m_AbortFlag == THREAD_SHUTDOWN_FLAG; }
	void AbortThread(long reason) { m_AbortFlag = reason; SignalThreadAbort(); }
	
	virtual void SignalThreadAbort() = 0;
	virtual DWORD Run() = 0;

	static DWORD _stdcall ThreadProc(LPVOID lpParm);

private:
	long m_AbortFlag;
	HANDLE m_ThreadHandle;
	CThreadManager *m_ThreadManager;
};

class CThreadManager
{
public:
	CThreadManager() {};
	~CThreadManager() {};
	bool Initialize();
	bool Initialized();
	void EnterCriticalSection();
	void LeaveCriticalSection();
	void AddThread(CThread *pThread);
	bool AbortThread(CThread *pThread);
	void RemoveThread(CThread *pThread);
	void ShutdownThreads();

private:
	CCriticalSection m_CritSect;
	CAtlList<CThread *> m_Threads;
};

class PerformanceCounter
{
public:
	static LARGE_INTEGER GetCounter();
	static __int64 GetMillisecondsSince(LARGE_INTEGER &refcounter);
private:
	static __int64 CountsPerMillisecond();
};

inline LARGE_INTEGER PerformanceCounter::GetCounter() 
{
	LARGE_INTEGER counter;
	if (QueryPerformanceCounter(&counter) == 0)
		counter.QuadPart = 0;
	return counter;
}

inline __int64 PerformanceCounter::GetMillisecondsSince(LARGE_INTEGER &refcounter)
{
	LARGE_INTEGER counter;
	if (QueryPerformanceCounter(&counter) != 0)
		return (counter.QuadPart - refcounter.QuadPart) / CountsPerMillisecond();
	else
		return -1;
}

inline __int64 PerformanceCounter::CountsPerMillisecond()
{
	static LARGE_INTEGER counts = {0};
	if (counts.QuadPart)
		return counts.QuadPart;

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
    counts.QuadPart = frequency.QuadPart / 1000;
	return counts.QuadPart;
}

#endif // _VFP2CHELPERS_H__