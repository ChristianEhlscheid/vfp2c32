#include <windows.h>
#include <shlwapi.h>
#include <assert.h>
#include <new>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2ccppapi.h"
#include "vfp2cutil.h"
#include "vfp2chelpers.h"

WindowsVersion COs::m_Version;
OSVERSIONINFOEX COs::m_osver = {0};

void COs::Init()
{
	PGETNATIVESYSTEMINFO pGetNativeSystemInfo = 0;
	SYSTEM_INFO sysInfo;
	bool bVersionEx = true;

	if (m_osver.dwOSVersionInfoSize != 0)
		return;

	m_osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if (!GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&m_osver)))
	{
		bVersionEx = false;
		m_osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&m_osver));
	}

	pGetNativeSystemInfo = (PGETNATIVESYSTEMINFO)GetProcAddress(GetModuleHandle("kernel32.dll"), "GetNativeSystemInfo");
	if (pGetNativeSystemInfo)
		pGetNativeSystemInfo(&sysInfo);
	else
        GetSystemInfo(&sysInfo);

	if(bVersionEx)
	{
		if (m_osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{
			switch(m_osver.dwMajorVersion)
			{
				case 4:
				{
					switch(m_osver.dwMinorVersion)
					{
						case 0:
							if (m_osver.szCSDVersion[0] == 'B' || m_osver.szCSDVersion[0] == 'C')
								m_Version = Windows95OSR2;
							else
								m_Version = Windows95;
							break;
						case 10:
							if (m_osver.szCSDVersion[0] == 'A')
								m_Version = Windows98SE;
							else
								m_Version = Windows98;
							break;
						case 90:
							m_Version = WindowsMillennium;
							break;       
					}
				}
			}
		}
		else if (m_osver.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			switch (m_osver.dwMajorVersion)
			{
				case 3:
					m_Version = WindowsNT351;
					break;

				case 4:
					switch (m_osver.wProductType)
					{
						case 1:
							m_Version = WindowsNT40;
							break;
						case 3:
							m_Version = WindowsNT40Server;
							break;
					}
					break;

				case 5:
					switch (m_osver.dwMinorVersion)
					{
						case 0:
							m_Version = Windows2000;
							break;
						case 1:
							m_Version = WindowsXP;
							break;
						case 2:
							if (m_osver.wSuiteMask == VER_SUITE_WH_SERVER)
								m_Version = WindowsHomeServer;
							else if (m_osver.wProductType == VER_NT_WORKSTATION && sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
								m_Version = WindowsXPProfessionalx64;
							else
								m_Version = GetSystemMetrics(89) == 0 ? WindowsServer2003 : WindowsServer2003R2;
							break;
					}
					break;

				case 6:
					switch (m_osver.dwMinorVersion)
					{
						case 0:
							m_Version = m_osver.wProductType == VER_NT_WORKSTATION ? WindowsVista : WindowsServer2008;
							break;

						case 1:
							m_Version = m_osver.wProductType == VER_NT_WORKSTATION ? Windows7 : WindowsServer2008R2;
							break;

						default:
							m_Version = WindowsX;
					}
					break;

				default:
					m_Version = WindowsX;
			}
		}
	}
	else // Test for specific product on Windows NT 4.0 SP5 and earlier
	{
		if (m_osver.dwMajorVersion == 3)
			m_Version = WindowsNT351;
		else if (m_osver.dwMajorVersion == 4)
			m_Version = WindowsNT40;
		else if (m_osver.dwMajorVersion == 5)
		{
			if (m_osver.dwMinorVersion == 0)
				m_Version = Windows2000;
			else if (m_osver.dwMinorVersion == 2)
				m_Version = WindowsServer2003;
		}
	}
}

CStr::CStr(unsigned int nSize)
{
	m_String = new char[nSize];
	if (m_String == 0)
		throw E_INSUFMEMORY;
	
	*m_String = '\0';
	m_Size = nSize;
	m_Length = 0;
}

CStr::~CStr()
{
	if (m_String)
		delete[] m_String;
}

void CStr::Size(unsigned int nSize)
{
	if (m_String == 0)
	{
		if (nSize == 0)
			nSize = 1;
		m_String = new char[nSize];
		if (m_String == 0)
			throw E_INSUFMEMORY;
		*m_String = '\0';
		m_Size = nSize;
		m_Length = 0;
	}
	else if (nSize > m_Size)
	{
		char *pTmp;
		pTmp =  new char[nSize];
		if (pTmp == 0)
			throw E_INSUFMEMORY;

		if (m_Length > nSize)
			m_Length = nSize - 1;
	
		memcpy(pTmp,m_String,m_Length);
		pTmp[m_Length] = '\0';
		delete[] m_String;
		m_String = pTmp;
		m_Size = nSize;
	}
}

CStr& CStr::AddBs()
{
	if (m_Length && m_String[m_Length-1] != '\\')
	{
		Size(m_Length+1);
		m_String[m_Length] = '\\';
		m_String[m_Length+1] = '\0';
		m_Length++;
	}
	return *this;
}

CStr& CStr::AddBsWc()
{
	assert(m_String);
	if (m_Length && m_String[m_Length-1] != '\\')
	{
		Size(m_Length+2);
		m_String[m_Length] = '\\';
		m_String[m_Length+1] = '*';
		m_String[m_Length+2] = '\0';
		m_Length += 2;
	}
	else if (m_Length && m_String[m_Length-1] != '*')
	{
		Size(m_Length+1);
		m_String[m_Length] = '*';
		m_String[m_Length+1] = '\0';
		m_Length++;
	}
	return *this;
}

CStr& CStr::AddLastPath(const char *pPath)
{
	assert(m_String);
	int nStrLen = strlen(pPath) + 1;
	char *pLastBS = strrchr(pPath,'\\');

	AddBs();

	if (pLastBS)
	{
		int nPathLen = nStrLen - (++pLastBS - pPath);
		Size(m_Length + nPathLen);
		memcpy(&m_String[m_Length],pLastBS,nPathLen);
		m_Length += nPathLen - 1;
	}
	return *this;
}

CStr& CStr::Justpath()
{
	assert(m_String);
	char *pLastBS = strrchr(m_String,'\\');
	if (pLastBS)
	{
		m_Length = pLastBS - m_String + 1;
		m_String[m_Length] = '\0';
	}
	return *this;
}

bool CStr::IsWildcardPath() const
{
	assert(m_String);
	return strchr(m_String,'*') > 0 || strchr(m_String,'?') > 0;
}

char* CStr::Strdup() const
{
	assert(m_String);
	char *pString = (char*)new char[m_Length+1];
	if (pString)
		memcpy(pString, m_String, m_Length+1);
    return pString;
}

CStr& CStr::RegValueToPropertyName()
{
	assert(m_String);
	int nMaxLen = VFP_MAX_PROPERTY_NAME; 
	char *pName = m_String;

	if (*pName == '\0')
	{
		Size(sizeof("Standard"));
		strcpy(m_String,"Standard");
		return *this;
	}	

	if (!(*pName >= 'a' && *m_String <= 'z') && 
		!(*pName >= 'A' && *pName <= 'Z') &&
		*pName != '_' && (*pName >= '0' && *pName <= '9'))
	{
		*pName++ = '_';
		nMaxLen--;
	}

	while (*pName && nMaxLen--)
	{
		if ((*pName >= 'a' && *pName <= 'z') || 
			(*pName >= 'A' && *pName <= 'Z') ||
			(*pName >= '0' && *pName <= '9'))
			pName++;	
		else
			*pName++ = '_';
	}
	*pName = '\0';
	m_Length = pName - m_String;
	return *this;
}

unsigned int CStr::Format(const char *lpFormat,...)
{
	assert(m_Size);
	char *lpString;
	char *lpStringParm;
	double nDouble;
	int nPrecision, nUseLength;
	unsigned int nSize = m_Size;

	va_list lpArgs;
	va_start(lpArgs, lpFormat);

	for (lpString = m_String ; *lpFormat ; lpFormat++)
	{
		if (--nSize <= 0)
			break;

		if (*lpFormat != '%')
		{
			*lpString++ = *lpFormat;
			continue;
		}
                  
    lpFormat++; 

	if (nUseLength = IsDigit(*lpFormat))
		nPrecision = skip_atoi(&lpFormat);
	else
		nPrecision = 6;

    switch (*lpFormat)
    {

		case 'I':
			lpString = IntToStr(lpString,va_arg(lpArgs,int));
			continue;

		case 'U':
			lpString = UIntToStr(lpString,va_arg(lpArgs,unsigned int));
			continue;

		case 'i':
			lpString = IntToStr(lpString,va_arg(lpArgs,short));
			continue;

		case 'u':
			lpString = UIntToStr(lpString,va_arg(lpArgs,unsigned short));
			continue;

		case 'F':
			lpString = DoubleToStr(lpString,va_arg(lpArgs, double),nPrecision);
			continue;

		case 'f':
			nDouble = (double)va_arg(lpArgs,float);
			lpString = DoubleToStr(lpString,nDouble,nPrecision);
			continue;

		case 'b':
			lpString = Int64ToStr(lpString,va_arg(lpArgs, __int64));
			continue;

		case 'B':
			lpString = UInt64ToStr(lpString,va_arg(lpArgs, unsigned __int64));
		
		case 'L':
			lpString = BoolToStr(lpString,va_arg(lpArgs,int));
			continue;

		case 'S':
			lpStringParm = va_arg(lpArgs,char*);
			if (lpStringParm)
			{
				if (!nUseLength)
					while ((*lpStringParm)) *lpString++ = *lpStringParm++;
				else
					while ((*lpStringParm) && nPrecision--) *lpString++ = *lpStringParm++;
			}
			continue;

		case 's':
			*lpString++ = va_arg(lpArgs,char);
			continue;
			
		default:
			if (*lpFormat != '%') *lpString++ = '%';
			if (*lpFormat)
				*lpString++ = *lpFormat;
			else
				--lpFormat;
			continue;
    }

   }

  *lpString = '\0';
  va_end(lpArgs);

  m_Length = lpString - m_String;
  return lpString - m_String;
}

CStr& CStr::operator=(const CStr &pString)
{
	unsigned int nSize = pString.Size();
	Size(nSize);
	m_Length = pString.Len();
	memcpy(m_String, pString, m_Length+1);
	return *this;
}

CStr& CStr::operator=(const char *pString)
{
	unsigned int nLen = strlen(pString) + 1;
	Size(nLen);
	memcpy(m_String,pString,nLen);
	m_Length = nLen - 1;
	return *this;
}

CStr& CStr::operator+=(const CStr &pString)
{
	unsigned int nLen = pString.Len() + 1;
	Size(nLen + m_Length);
	memcpy(m_String + m_Length, pString, nLen);
	m_Length += nLen - 1;
	return *this;
}

CStr& CStr::operator+=(const char *pString)
{
	unsigned int nLen = strlen(pString) + 1;
	Size(nLen + m_Length);
	memcpy(m_String + m_Length, pString, nLen);
	m_Length += nLen - 1;
	return *this;
}

bool CStr::operator==(const char *pString) const
{
	int nRet = strcmp(m_String,pString);
	return nRet == 0;
}

CBuffer::CBuffer(unsigned int nSize)
{
	m_Pointer = new char[nSize];
	if (m_Pointer == 0)
		throw E_INSUFMEMORY;
	m_Size = nSize;
}

CBuffer::~CBuffer()
{
	if (m_Pointer)
		delete[] m_Pointer;
}

void CBuffer::Size(unsigned int nSize)
{
	if (m_Pointer != 0)
		delete[] m_Pointer;

	m_Pointer = new char[nSize];
	if (m_Pointer == 0)
		throw E_INSUFMEMORY;
    m_Size = nSize;
}

bool RegistryKey::Create(HKEY hKey, LPCSTR lpKey, LPSTR lpClass, DWORD nOptions, REGSAM samDesired)
{
	LONG nApiRet;
	DWORD bCreated;

	if (m_Handle && m_Owner)
	{
		RegCloseKey(m_Handle);
		m_Handle = 0;
	}

	m_Owner = true;
	nApiRet = RegCreateKeyEx(hKey,lpKey,0,lpClass,nOptions,samDesired,0,&m_Handle,&bCreated);
	if (nApiRet != ERROR_SUCCESS)
	{
		SaveWin32Error("RegCreateKeyEx", nApiRet);
		throw E_APIERROR;
	}

	return bCreated == REG_CREATED_NEW_KEY;
}

void RegistryKey::Open(HKEY hKey, LPCSTR lpKey, REGSAM samDesired, DWORD ulOptions)
{
	LONG nApiRet;

	if (m_Handle && m_Owner)
	{
		RegCloseKey(m_Handle);
		m_Handle = 0;
	}
	
	if (lpKey)
	{
		nApiRet = RegOpenKeyEx(hKey, lpKey, ulOptions, samDesired, &m_Handle);
		if (nApiRet != ERROR_SUCCESS)
		{
			SaveWin32Error("RegOpenKeyEx", nApiRet);
			throw E_APIERROR;
		}
		m_Owner = true;
	}
	else
	{
		m_Handle = hKey;
		m_Owner = false;
	}
}

HKEY RegistryKey::OpenSubKey(LPCSTR lpSubKey, REGSAM samDesired, DWORD ulOptions)
{
	LONG nApiRet; HKEY hSubKey;
	nApiRet = RegOpenKeyEx(m_Handle, lpSubKey, ulOptions, samDesired, &hSubKey);
	if (nApiRet != ERROR_SUCCESS)
	{
		SaveWin32Error("RegOpenKeyEx", nApiRet);
		throw E_APIERROR;
	}
	return hSubKey;
}

bool RegistryKey::Delete(HKEY hKey, LPCSTR lpKeyName, bool bShellVersion)
{
	LONG nApiRet;

	if (!bShellVersion)
	{
		if ((nApiRet = RegDeleteKey(hKey,lpKeyName)) != ERROR_SUCCESS)
		{
			SaveWin32Error("RegDeleteKey", nApiRet);
			return false;
		}
	}
	else
	{
		if ((nApiRet = SHDeleteKey(hKey,lpKeyName)) != ERROR_SUCCESS)
		{
			SaveWin32Error("SHDeleteKey", nApiRet);
			return false;
		}
	}
	return true;
}

void RegistryKey::QueryInfo(LPSTR lpClass, LPDWORD lpcbClass, LPDWORD lpSubKeys, 
								  LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, LPDWORD lpcValues,
								  LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, 
								  LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime)
{
	LONG nApiRet;
	nApiRet = RegQueryInfoKey(m_Handle,lpClass,lpcbClass,0,lpSubKeys,lpcbMaxSubKeyLen,lpcbMaxClassLen,
		lpcValues,lpcbMaxValueNameLen,lpcbMaxValueLen,lpcbSecurityDescriptor,lpftLastWriteTime);
	if (nApiRet != ERROR_SUCCESS)
	{
		SaveWin32Error("RegQueryInfoKey", nApiRet);
		throw E_APIERROR;
	}
	if (lpcbMaxSubKeyLen)
		(*lpcbMaxSubKeyLen)++;
	if (lpcbMaxClassLen)
		(*lpcbMaxClassLen)++;
	if (lpcbMaxValueNameLen)
		(*lpcbMaxValueNameLen)++;
}

bool RegistryKey::EnumFirstKey(LPSTR lpKey, LPDWORD lpKeyLen, LPSTR lpClass, LPDWORD lpClassLen,
							   PFILETIME lpftLastWriteTime)
{
	LONG nApiRet;
	m_KeyIndex = 0;
	nApiRet = RegEnumKeyEx(m_Handle,m_KeyIndex,lpKey,lpKeyLen,0,lpClass,lpClassLen,lpftLastWriteTime);

	if (nApiRet == ERROR_SUCCESS)
		return true;
	else if (nApiRet == ERROR_NO_MORE_ITEMS)
		return false;
	else
	{
		SaveWin32Error("RegEnumKeyEx", nApiRet);
		throw E_APIERROR;
	}
}

bool RegistryKey::EnumNextKey(LPSTR lpKey, LPDWORD lpKeyLen, LPSTR lpClass, LPDWORD lpClassLen,
							   PFILETIME lpftLastWriteTime)
{
	LONG nApiRet;
	m_KeyIndex++;
	nApiRet = RegEnumKeyEx(m_Handle,m_KeyIndex,lpKey,lpKeyLen,0,lpClass,lpClassLen,lpftLastWriteTime);

	if (nApiRet == ERROR_SUCCESS)
		return true;
	else if (nApiRet == ERROR_NO_MORE_ITEMS)
		return false;
	else
	{
		SaveWin32Error("RegEnumKeyEx", nApiRet);
		throw E_APIERROR;
	}
}

bool RegistryKey::EnumFirstValue(LPSTR lpValueName, LPDWORD lpcbValueName, LPBYTE lpData, LPDWORD lpcbData, LPDWORD lpType)
{
	LONG nApiRet;
	m_ValueIndex = 0;
	nApiRet = RegEnumValue(m_Handle,m_ValueIndex,lpValueName,lpcbValueName,0,lpType,lpData,lpcbData);
	if (nApiRet == ERROR_SUCCESS)
	{
		if (*lpType == REG_SZ || *lpType == REG_MULTI_SZ || *lpType == REG_EXPAND_SZ)
			(*lpcbData)--;
		return true;
	}
	else if (nApiRet == ERROR_NO_MORE_ITEMS)
		return false;
	else
	{
		SaveWin32Error("RegEnumValue", nApiRet);
		throw E_APIERROR;
	}
}

bool RegistryKey::EnumNextValue(LPSTR lpValueName, LPDWORD lpcbValueName, LPBYTE lpData, LPDWORD lpcbData, LPDWORD lpType)
{
	LONG nApiRet;
	m_ValueIndex++;
	nApiRet = RegEnumValue(m_Handle,m_ValueIndex,lpValueName,lpcbValueName,0,lpType,lpData,lpcbData);
	if (nApiRet == ERROR_SUCCESS)
	{
		if (*lpType == REG_SZ || *lpType == REG_MULTI_SZ || *lpType == REG_EXPAND_SZ)
			(*lpcbData)--;
		return true;
	}
	else if (nApiRet == ERROR_NO_MORE_ITEMS)
		return false;
	else
	{
		SaveWin32Error("RegEnumValue", nApiRet);
		throw E_APIERROR;
	}
}

DWORD RegistryKey::QueryValueInfo(LPSTR lpValue, DWORD &lpType)
{
	LONG nApiRet;
	DWORD dwLen;
	nApiRet = RegQueryValueEx(m_Handle,lpValue,0,&lpType,0,&dwLen);
	if (nApiRet != ERROR_SUCCESS)
	{
		SaveWin32Error("RegQueryValueEx", nApiRet);
		throw E_APIERROR;
	}
	return dwLen;
}

void RegistryKey::QueryValue(LPSTR lpValue, LPBYTE lpData, LPDWORD lpDataLen, LPDWORD lpType)
{
	LONG nApiRet;
	DWORD dwType;
	
	if (lpType == 0)
		lpType = &dwType;

	nApiRet = RegQueryValueEx(m_Handle,lpValue,0,lpType,lpData,lpDataLen);
	if (nApiRet != ERROR_SUCCESS)
	{
		SaveWin32Error("RegQueryValueEx", nApiRet);
		throw E_APIERROR;
	}
	if (*lpType == REG_SZ || *lpType == REG_MULTI_SZ || *lpType == REG_EXPAND_SZ)
		(*lpDataLen)--;
}

void RegistryKey::SetValue(LPCSTR lpValueName, LPBYTE lpData, DWORD dwDataLen, DWORD dwType)
{
	LONG nApiRet;
	nApiRet = RegSetValueEx(m_Handle,lpValueName,0,dwType,lpData,dwDataLen);
	if (nApiRet != ERROR_SUCCESS)
	{
		SaveWin32Error("RegSetValueEx", nApiRet);
		throw E_APIERROR;
	}
}

RegistryKey& RegistryKey::operator=(HKEY hKey)
{
	if (m_Handle != 0 && m_Owner)
		RegCloseKey(m_Handle);
	m_Handle = hKey;
	m_Owner = true;
	return *this;
}

bool CCriticalSection::Initialize(DWORD spinCount)
{
	if (!InitializeCriticalSectionAndSpinCount(&m_Section, spinCount | 0xFF000000))
	{
		SaveWin32Error("InitializeCriticalSectionAndSpinCount", GetLastError());
		return false;
	}
	m_Inited = true;
	return true;
}

bool CCriticalSection::Initialized()
{
	return m_Inited;
}

void CCriticalSection::Enter()
{
	assert(m_Inited);
	EnterCriticalSection(&m_Section);
}

void CCriticalSection::Leave()
{
	assert(m_Inited);
	LeaveCriticalSection(&m_Section);
}

CThread::~CThread()
{
	if (m_ThreadHandle)
	{
		m_ThreadManager->RemoveThread(this);
		CloseHandle(m_ThreadHandle);
	}
}

void CThread::WaitForThreadShutdown()
{
	WaitForSingleObject(m_ThreadHandle, INFINITE);
}

void CThread::StartThread()
{
	DWORD dwThreadId; 
	m_ThreadHandle = CreateThread(0, THREAD_STACKSIZE, ThreadProc, (LPVOID)this, 0, &dwThreadId);
	if (m_ThreadHandle == 0)
	{
		SaveWin32Error("CreateThread", GetLastError());
		throw E_APIERROR;
	}
	m_ThreadManager->AddThread(this);
}

DWORD _stdcall CThread::ThreadProc(LPVOID lpParm)
{
	CThread *pThread = (CThread*)lpParm;
	DWORD dwRet = pThread->Run();

	CThreadManager *pManager = pThread->GetThreadManager();
	pManager->EnterCriticalSection();
	if (!pThread->IsShutdown())
		delete pThread;
	pManager->LeaveCriticalSection();

	return dwRet;
}

bool CThreadManager::Initialize()
{
	return m_CritSect.Initialize();
}

bool CThreadManager::Initialized()
{
	return m_CritSect.Initialized();
}

void CThreadManager::EnterCriticalSection()
{
	m_CritSect.Enter();
}

void CThreadManager::LeaveCriticalSection()
{
	m_CritSect.Leave();
}

void CThreadManager::ShutdownThreads()
{
	if (!m_CritSect.Initialized())
		return;

 	while (true)
	{
		m_CritSect.Enter();
		if (!m_Threads.IsEmpty())
		{
			CThread *pThread = m_Threads.RemoveHead();
			pThread->AbortThread(THREAD_SHUTDOWN_FLAG);
			m_CritSect.Leave();
			pThread->WaitForThreadShutdown();
			delete pThread;
		}
		else
		{
			m_CritSect.Leave();
			break;
		}
	}
}

void CThreadManager::AddThread(CThread *pThread)
{
	m_CritSect.Enter();
	m_Threads.AddTail(pThread);
	m_CritSect.Leave();
}

void CThreadManager::RemoveThread(CThread *pThread)
{
	m_CritSect.Enter();
	POSITION pos = m_Threads.Find(pThread);
	if (pos)
		m_Threads.RemoveAt(pos);
	m_CritSect.Leave();
}

bool CThreadManager::AbortThread(CThread *pThread)
{
	bool bRetval = false;

	m_CritSect.Enter();
	POSITION pos = m_Threads.Find(pThread);
	if (pos)
	{
        pThread->AbortThread(THREAD_ABORT_FLAG);
		bRetval = true;
	}
	m_CritSect.Leave();

	return bRetval;
}
