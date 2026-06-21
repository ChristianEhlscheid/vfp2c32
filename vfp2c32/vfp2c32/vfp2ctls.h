#ifndef _VFP2CTLS_H__
#define _VFP2CTLS_H__

#include <windows.h>
#include "vfp2cdatastructure.h"

const int VFP2C_MAX_ERRORS					= 32;
const unsigned int VFP2C_ERROR_MESSAGE_LEN	= 1024;
const unsigned int VFP2C_ERROR_FUNCTION_LEN	= 64;
const int VFP2C_ODBC_STATE_LEN				= 8;

const unsigned int VFP2C_ERRORTYPE_WIN32	= 1;
const unsigned int VFP2C_ERRORTYPE_ODBC		= 2;

typedef struct _VFP2CERROR {
	unsigned int nErrorType; // one of VFP2C_ERRORTYPE_ constants
	unsigned long nErrorNo;
	char aErrorFunction[VFP2C_ERROR_FUNCTION_LEN];
	char aErrorMessage[VFP2C_ERROR_MESSAGE_LEN];
	char aSqlState[VFP2C_ODBC_STATE_LEN];
	INT_PTR nRowNumber;
	long nColumnNumber;
} VFP2CERROR, *LPVFP2CERROR;

#ifdef _DEBUG
typedef struct _DBGALLOCINFO {
	void* pPointer;
	char* pProgInfo;
	int nSize;
	struct _DBGALLOCINFO *next;
} DBGALLOCINFO, *LPDBGALLOCINFO;
#endif

class WindowSubclass;
class CallbackFunction;

class CriticalSectionGuard
{
public:
	CriticalSectionGuard(CRITICAL_SECTION& section)
	{
		cs = &section;
		EnterCriticalSection(cs);
	}

	~CriticalSectionGuard()
	{
		LeaveCriticalSection(cs);
	}

private:
	CRITICAL_SECTION* cs;
};

class VFP2CTls
{
public:
	VFP2CTls();
	~VFP2CTls();

	/* array of VFP2CERROR structs for storing Win32, custom & ODBC errors */
	VFP2CERROR ErrorInfo[VFP2C_MAX_ERRORS];
	/* No. of error messages in gaErrorInfo */
	int ErrorCount; 
	/* codepage for Unicode character conversion */
	UINT ConvCP; 
	/* convert Unicode data to ANSI in SQLExecEx */
	bool SqlUnicodeConversion;
	/* store error information for all ODBC api calls that return SQL_SUCCESS_WITH_INFO */
	bool SqlStoreInfoAsError;
	/* store SQL_VARBINARY(0) and SQL_LONGVARBINARY into general (G) fields */
	bool SqlMapBinaryToGeneral;

#ifdef _DEBUG
	LPDBGALLOCINFO DbgInfo;
	bool TrackAlloc;
#endif
	/* handle to process api library */
	
	/* flag if winsock was initialized with WSAStartup */
	bool WinsockInited;
	/* default timeout for winsock calls */
	DWORD DefaultWinsockTimeout;

	CArray<WindowSubclass*> WindowSubclasses;
	CArray<CallbackFunction*> CallbackFunctions;
	CArray<HANDLE> FileHandles;
	HANDLE GetJitHeap();
#if defined(_WIN64)
	CArray<RUNTIME_FUNCTION> RuntimeFunctions;
	DWORD64 JitBaseAddress;
#endif
	static void _stdcall OnLoad();
	static void _stdcall OnUnload();
	static VFP2CTls& _stdcall Tls();

#ifdef _THREADSAFE
	static CriticalSectionGuard Guard();
	static LONG ThreadCount;
	static DWORD TlsIndex;
#endif
	/* handle to our default heap which is created at load time */
	static HANDLE Heap; 
	/* handles to dynamically loaded modules */
	static HMODULE RasApi32;
	static HMODULE RasDlg;
	static HMODULE IpHlpApi;
	static HMODULE Icmp;
	static HMODULE NetApi32;
	static HMODULE Propsys;
	static HMODULE HsDll;
	static bool HyperScanInitialized;
	
private:
	HANDLE JitHeap;
#ifdef _THREADSAFE
	static CRITICAL_SECTION CritSection;
	static void _stdcall InitCriticalSection();
	static void _stdcall FreeCriticalSection();
#endif
	static void _stdcall FreeGlobalResources();
};

#endif // _VFP2CTLS_H__

