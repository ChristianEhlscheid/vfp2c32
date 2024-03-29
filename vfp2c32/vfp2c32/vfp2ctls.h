#ifndef _VFP2CTLS_H__
#define _VFP2CTLS_H__

#include <windows.h>
#include <atlcoll.h>
#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2cdatastructure.h"
#include "vfp2ctls.h"

const int VFP2C_MAX_ERRORS					= 8;
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

class VFP2CTls
{
public:
	VFP2CTls();
	~VFP2CTls();

	/* array of VFP2CERROR structs for storing Win32, custom & ODBC errors */
	VFP2CERROR ErrorInfo[VFP2C_MAX_ERRORS];
	/* No. of error messages in gaErrorInfo */
	int ErrorCount; 
	/* Initialization status */
	DWORD InitStatus;
	/* codepage for Unicode character conversion */
	UINT ConvCP; 
	/* convert Unicode data to ANSI in SQLExecEx */
	BOOL SqlUnicodeConversion;

#ifdef _DEBUG
	LPDBGALLOCINFO DbgInfo;
	BOOL TrackAlloc;
#endif
	/* handle to process api library */
	
	/* flag is winsock was initialized with WSAStartup */
	BOOL WinsockInited;
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

private:
	HANDLE JitHeap;
	static void _stdcall FreeGlobalResources();
};

#endif // _VFP2CTLS_H__

