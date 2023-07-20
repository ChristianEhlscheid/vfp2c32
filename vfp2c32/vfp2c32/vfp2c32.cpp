#include <windows.h> /* no comment .. */
#include <stdio.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
/* VFP2C specific includes */
#include "vfp2c32.h"  /* VFP2C32 specific types & defines */
#include "vfp2carray.h" /* array functions */
#include "vfp2casync.h" /* asynchronous functions */
#include "vfp2cassembly.h" /* runtime assembler */
#include "vfp2cconv.h" /* misc data conversion functions */
#include "vfp2cenum.h" /* window, process, thread, module enumeration functions */
#include "vfp2cfile.h" /* filesystem related functions */
#include "vfp2cmarshal.h" /* marshaling & memory allocation/read/write routines */
#include "vfp2cnetapi.h" /* NetApi32 wrappers (network user,group,resource enumeration etc.) */
#include "vfp2codbc.h" /* ODBC related functions (datasource enumeration,creation,deletion etc., SQLSetPropEx ..) */
#include "vfp2cprint.h" /* Printer related functions (printjob enumeration etc.) */
#include "vfp2cregistry.h" /* registry functions */
#include "vfp2ctime.h" /* time conversion functions */
#include "vfp2ccom.h" /* COM functions */
#include "vfp2casynccom.h" /* Asynchronous COM calls */
#include "vfp2curlmon.h" /* wrappers around urlmon.dll functions */
#include "vfp2cwininet.h" /* wrappers around wininet.dll functions */
#include "vfp2ccallback.h" /* C Callback function emulation */
#include "vfp2cwinsock.h" /* winsock initialization */
#include "vfp2cshell.h" /* windows shell wrappers */
#include "vfp2csntp.h"	/* SNTP (RFC 1769) implementation */
#include "vfp2cservices.h" /* win service functions */
#include "vfp2cwindows.h" /* some window functions */
#include "vfp2cras.h" /* RAS (dialup management) wrappers (rasapi32.dll) */
#include "vfp2ciphelper.h" /* IP Helper (iphlpapi.dll) wrappers */
#include "vfp2cfont.h" /* Font functions  */
#include "vfp2cutil.h" /* common utility functions */
#include "vfp2cstring.h" /* common string functions */
#include "vfp2ccppapi.h" /* C++ class library over LCK */
#include "vfp2ctls.h" /* VFP2C32 thread local storage */

/* Global variables: module handle for this DLL */
HMODULE ghModule = 0;

void _fastcall OnLoad()
{
	/* get module handle - _GetAPIHandle() doesn't work (unresolved external error from linker) */
	if (!ghModule)
		ghModule = GetModuleHandle(FLLFILENAME);
	VFP2CTls::OnLoad();

	/* get OS information */
	COs::Init();

	VFP2C_Init_Com();
}

void _fastcall OnUnload()
{
	VFP2CTls& tls = VFP2CTls::Tls();

	VFP2C_Destroy_Marshal(tls);

	 // these are not supported in multithreaded version
#ifndef _THREADSAFE
	VFP2C_Destroy_Async(tls);
	VFP2C_Destroy_Callback(tls);
	VFP2C_Destroy_Urlmon(tls);
#endif

	VFP2C_Destroy_Winsock(tls);
	VFP2C_Destroy_File(tls);

	VFP2CTls::OnUnload();
}

/* error handling routine to store the last error occurred in a Win32 API call 
 called through the macros SAVEWIN32ERROR, ADDWIN32ERROR or RAISEWIN32ERROR */
void _stdcall Win32ErrorHandler(char *pFunction, DWORD nErrorNo, bool bAddError, bool bRaise)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	if (bAddError)
	{
		if (tls.ErrorCount == VFP2C_MAX_ERRORS)
			return;
		tls.ErrorCount++;
	}
	else
		tls.ErrorCount = 0;

	LPVFP2CERROR pError = &tls.ErrorInfo[tls.ErrorCount];
	pError->nErrorType = VFP2C_ERRORTYPE_WIN32;
	pError->nErrorNo = nErrorNo;
	strncpyex(pError->aErrorFunction,pFunction,VFP2C_ERROR_FUNCTION_LEN);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, nErrorNo, 0, pError->aErrorMessage,VFP2C_ERROR_MESSAGE_LEN,0);
	if (bRaise)
		_UserError(pError->aErrorMessage);
}

void _stdcall CustomErrorHandler(char *pFunction, char* pErrorMessage, bool bAddError, bool bRaise, va_list lpArgs)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	if (bAddError)
	{
		if (tls.ErrorCount == VFP2C_MAX_ERRORS)
			return;
		tls.ErrorCount++;
	}
	else
		tls.ErrorCount = 0;

	LPVFP2CERROR pError = &tls.ErrorInfo[tls.ErrorCount];
	pError->nErrorType = VFP2C_ERRORTYPE_WIN32;
	pError->nErrorNo = E_CUSTOMERROR;
	strncpyex(pError->aErrorFunction, pFunction, VFP2C_ERROR_FUNCTION_LEN);
	nprintfex(pError->aErrorMessage, pErrorMessage, sizeof(pError->aErrorMessage), lpArgs);
	if (bRaise)
		_UserError(pError->aErrorMessage);
}

void _stdcall CustomErrorHandlerEx(char *pFunction, char *pErrorMessage, int nErrorNo, bool bAddError, bool bRaise, va_list lpArgs)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	if (bAddError)
	{
		if (tls.ErrorCount == VFP2C_MAX_ERRORS)
			return;
		tls.ErrorCount++;
	}
	else
		tls.ErrorCount = 0;

	LPVFP2CERROR pError = &tls.ErrorInfo[tls.ErrorCount];
	pError->nErrorType = VFP2C_ERRORTYPE_WIN32;
	pError->nErrorNo = nErrorNo;
	strncpyex(pError->aErrorFunction, pFunction,VFP2C_ERROR_FUNCTION_LEN);
	nprintfex(pError->aErrorMessage, pErrorMessage, sizeof(pError->aErrorFunction), lpArgs);
	if (bRaise)
		_UserError(pError->aErrorMessage);
}

void _cdecl SaveCustomError(char *pFunction, char *pMessage, ...)
{ 
	va_list lpArgs;
	va_start(lpArgs, pMessage);
	CustomErrorHandler(pFunction, pMessage, false, false, lpArgs);
	va_end(lpArgs);
}

void _cdecl AddCustomError(char *pFunction, char *pMessage, ...)
{
	va_list lpArgs;
	va_start(lpArgs, pMessage);
	CustomErrorHandler(pFunction, pMessage, true, false, lpArgs);
	va_end(lpArgs);
}

void _cdecl RaiseCustomError(char *pFunction, char *pMessage, ...)
{
	va_list lpArgs;
	va_start(lpArgs, pMessage);
	CustomErrorHandler(pFunction, pMessage, false, true, lpArgs);
	va_end(lpArgs);
}

void _cdecl SaveCustomErrorEx(char *pFunction, char *pMessage, int nErrorNo, ...)
{
	va_list lpArgs;
	va_start(lpArgs, nErrorNo);
	CustomErrorHandlerEx(pFunction, pMessage, nErrorNo, false, false, lpArgs);
	va_end(lpArgs);
}

void _cdecl AddCustomErrorEx(char *pFunction, char *pMessage, int nErrorNo, ...)
{ 
	va_list lpArgs;
	va_start(lpArgs, nErrorNo);
	CustomErrorHandlerEx(pFunction, pMessage, nErrorNo, true, false, lpArgs);
	va_end(lpArgs);
}

void _cdecl RaiseCustomErrorEx(char *pFunction, char *pMessage, int nErrorNo, ...)
{ 
	va_list lpArgs;
	va_start(lpArgs, nErrorNo);
	CustomErrorHandlerEx(pFunction, pMessage, nErrorNo, false, true, lpArgs);
	va_end(lpArgs);
}

void _fastcall VFP2CSys(ParamBlkEx& parm)
{
try
{
	switch (parm(1)->ev_long)
	{
		case 1: /* library's HINSTANCE/HMODULE */
			if (parm.PCount() == 2)
			{
				SaveCustomError("VFP2CSys", "Too many parameters.");
				throw E_INVALIDPARAMS;
			}
			Return(ghModule);
			break;
		case 2: /* library heap HANDLE */
			if (parm.PCount() == 2)
			{
				SaveCustomError("VFP2CSys", "Too many parameters.");
				throw E_INVALIDPARAMS;
			}
			Return(VFP2CTls::Heap);
			break;
		case 3: /* set or return Unicode conversion codepage */
			if (parm.PCount() == 2)
			{
				if (parm(2)->Vartype() == 'I' || parm(2)->Vartype() == 'N')
				{
					UINT nCodePage = parm(2)->Vartype() == 'I' ? parm(2)->ev_long : static_cast<UINT>(parm(2)->ev_real);
					if (IsValidCodePage((UINT)parm(2)->ev_long))
					{
						VFP2CTls::Tls().ConvCP = (UINT)parm(2)->ev_long;
						Return(true);
					}
					else
					{
						SaveCustomError("VFP2CSys", "Parameter 2 was an invalid codepage");
						Return(false);
					}
				}
				else
				{
					SaveCustomError("VFP2CSys", "Parameter 2 should be of type I or N.");
					throw E_INVALIDPARAMS;
				}
			}
			else
				Return(VFP2CTls::Tls().ConvCP);
			break;

		case 4:
			if (parm.PCount() == 2)
			{
				if (parm(2)->Vartype() == 'L')
				{
					VFP2CTls::Tls().SqlUnicodeConversion = parm(2)->ev_length;
					Return(true);
				}
				else
				{
					SaveCustomError("VFP2CSys", "Parameter 2 should be of type L.");
					throw E_INVALIDPARAMS;
				}
			}
			else
				Return(VFP2CTls::Tls().SqlUnicodeConversion > 0);
			break;

		default: /* else wrong parameter */
		{
			SaveCustomError("VFP2CSys", "Parameter 1 unknown.");
			throw E_INVALIDPARAMS;
		}
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FormatMessageEx(ParamBlkEx& parm)
{
try
{
	DWORD nLanguage = 0;
	DWORD nFlags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	LPCVOID lpModule = 0;
	FoxString pMessage(VFP2C_ERROR_MESSAGE_LEN);
	
	if (parm.PCount() == 2)
		nLanguage = parm(2)->ev_long;
	else if (parm.PCount() == 3)
	{
		nLanguage = parm(2)->ev_long;
		lpModule = parm(3)->Ptr<LPCVOID>();
		nFlags |= FORMAT_MESSAGE_FROM_HMODULE;
	}

	pMessage.Len(FormatMessage(nFlags, lpModule, parm(1)->ev_long, nLanguage, pMessage, pMessage.Size(), 0));

	if (pMessage.Len())
		pMessage.Return();
	else
	{
		SaveWin32Error("FormatMessage", GetLastError());
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AErrorEx(ParamBlkEx& parm)
{
try
{
	VFP2CTls& tls = VFP2CTls::Tls();
	if (tls.ErrorCount == -1)
	{
		Return(0);
		return;
	}

	FoxArray pArray(parm(1), tls.ErrorCount+1, 4);
	FoxString pErrorInfo(VFP2C_ERROR_MESSAGE_LEN);
	FoxValue vNullValue;

	unsigned int nRow = 0;
	for (int xj = 0; xj <= tls.ErrorCount; xj++)
	{
		nRow++;
		pArray(nRow,1) = tls.ErrorInfo[xj].nErrorNo;
		pArray(nRow,2) = pErrorInfo = tls.ErrorInfo[xj].aErrorFunction;
		pArray(nRow,3) = pErrorInfo = tls.ErrorInfo[xj].aErrorMessage;
		if (tls.ErrorInfo[xj].nErrorType == VFP2C_ERRORTYPE_ODBC)
			pArray(nRow,4) = pErrorInfo = tls.ErrorInfo[xj].aSqlState;
		else
			pArray(nRow,4) = vNullValue;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

#ifdef __cplusplus
extern "C" {
#endif

FoxInfo VFP2CFuncs[] = 
{
	/* common library routines (startup, cleanup & internal settings) */
	{"OnLoad", (FPFI) OnLoad, CALLONLOAD, ""},
	{"OnUnload", (FPFI) OnUnload, CALLONUNLOAD, ""},
	{"VFP2CSys", (FPFI) VFP2CSys, 2, "I.?"},
	
	/* memory management routines */
	{"AllocMem", (FPFI) AllocMem, 1, "I"},
	{"AllocMemTo", (FPFI) AllocMemTo, 2, "II"},
	{"ReAllocMem", (FPFI) ReAllocMem, 2, "II"},
	{"FreeMem", (FPFI) FreeMem, 1, "I"},
	{"FreePMem", (FPFI) FreePMem, 1, "I"},
	{"FreeRefArray", (FPFI) FreeRefArray, 3, "III"},
	{"SizeOfMem", (FPFI) SizeOfMem, 1, "I"},
	{"ValidateMem", (FPFI) ValidateMem, 1, "I"},
	{"CompactMem", (FPFI) CompactMem, 0, ""},
	{"AMemBlocks", (FPFI) AMemBlocks, 1, "C"},

	/* wrappers around Global memory functions */
	{"AllocHGlobal", (FPFI) AllocHGlobal, 2, "I.I"},
	{"FreeHGlobal", (FPFI) FreeHGlobal, 1, "I"},
	{"ReAllocHGlobal", (FPFI) ReAllocHGlobal, 3, "II.I"},
	{"LockHGlobal", (FPFI) LockHGlobal, 1, "I"},
	{"UnlockHGlobal", (FPFI) UnlockHGlobal, 1, "I"},

	/* read/write C types from/to memory */
	{"WriteChar", (FPFI) WriteChar, 2, "IC"},
	{"WritePChar", (FPFI) WritePChar, 2, "IC"},
	{"WriteInt8", (FPFI) WriteInt8, 2, "II"},
	{"WritePInt8", (FPFI) WritePInt8, 2, "II"},
	{"WriteUInt8", (FPFI) WriteUInt8, 2, "II"},
	{"WritePUInt8", (FPFI) WritePUInt8, 2, "II"},
	{"WriteShort", (FPFI) WriteShort, 2, "II"},
	{"WritePShort", (FPFI) WritePShort, 2, "II"},
	{"WriteUShort", (FPFI) WriteUShort, 2, "II"},
	{"WritePUShort", (FPFI) WritePUShort, 2, "II"},
	{"WriteInt", (FPFI) WriteInt, 2, "II"},
	{"WritePInt", (FPFI) WritePInt, 2, "II"},
	{"WriteUInt", (FPFI) WriteUInt, 2, "IN"},
	{"WritePUInt", (FPFI) WritePUInt, 2, "IN"},
	{"WriteInt64", (FPFI) WriteInt64, 2, "I?"},
	{"WritePInt64", (FPFI) WritePInt64, 2, "I?"},
	{"WriteUInt64", (FPFI) WriteUInt64, 2, "I?"},
	{"WritePUInt64", (FPFI) WritePUInt64, 2, "I?"},
	{"WritePointer", (FPFI) WritePointer, 2, "II"},
	{"WritePPointer", (FPFI) WritePPointer, 2, "II"},
	{"WriteFloat", (FPFI) WriteFloat, 2, "IN"},
	{"WritePFloat", (FPFI) WritePFloat, 2, "IN"},
	{"WriteDouble", (FPFI) WriteDouble, 2, "IN"},
	{"WritePDouble", (FPFI) WritePDouble, 2, "IN"},
	{"WriteCString", (FPFI) WriteCString, 2, "IC"},
	{"WritePCString", (FPFI) WritePCString, 2, "I?"},
	{"WriteGPCString", (FPFI) WriteGPCString, 2, "I?"},
	{"WriteCharArray", (FPFI) WriteCharArray, 3, "IC.I"},
	{"WriteWString", (FPFI) WriteWString, 3, "IC.I"},
	{"WritePWString", (FPFI) WritePWString, 3, "I?.I"},
	{"WriteWCharArray", (FPFI) WriteWCharArray, 4, "ICI.I"},
	{"WriteWChar", (FPFI) WriteWChar, 3, "IC.I"},
	{"WritePWChar", (FPFI) WritePWChar, 3, "IC.I"},
	{"WriteBytes", (FPFI) WriteBytes, 3, "IC.I"},
	{"WriteLogical", (FPFI) WriteLogical, 2, "IL"},
	{"WritePLogical", (FPFI) WritePLogical, 2, "IL"},

	{"ReadChar", (FPFI) ReadChar, 1, "I"},
	{"ReadPChar", (FPFI) ReadPChar, 2, "I.?"},
	{"ReadInt8", (FPFI) ReadInt8, 1, "I"},
	{"ReadPInt8", (FPFI) ReadPInt8, 1, "I"},
	{"ReadUInt8", (FPFI) ReadUInt8, 1, "I"},
	{"ReadPUInt8", (FPFI) ReadPUInt8, 1, "I"},
	{"ReadShort", (FPFI) ReadShort, 1, "I"},
	{"ReadPShort", (FPFI) ReadPShort, 1, "I"},
	{"ReadUShort", (FPFI) ReadUShort, 1, "I"},
	{"ReadPUShort", (FPFI) ReadPUShort, 1, "I"},
	{"ReadInt", (FPFI) ReadInt, 1, "I"},
	{"ReadPInt", (FPFI) ReadPInt, 1, "I"},
	{"ReadUInt", (FPFI) ReadUInt, 1, "I"},
	{"ReadPUInt", (FPFI) ReadPUInt, 1, "I"},
	{"ReadInt64", (FPFI) ReadInt64, 2, "I.I"},
	{"ReadPInt64", (FPFI) ReadPInt64, 2, "I.I"},
	{"ReadUInt64", (FPFI) ReadUInt64, 2, "I.I"},
	{"ReadPUInt64", (FPFI) ReadPUInt64, 2, "I.I"},
	{"ReadFloat", (FPFI) ReadFloat, 1, "I"},
	{"ReadPFloat", (FPFI) ReadPFloat, 1, "I"},
	{"ReadDouble", (FPFI) ReadDouble, 1, "I"},
	{"ReadPDouble", (FPFI) ReadPDouble, 1, "I"},
	{"ReadLogical", (FPFI) ReadLogical, 1, "I"},
	{"ReadPLogical", (FPFI) ReadPLogical, 1, "I"},
	{"ReadPointer",(FPFI) ReadPointer, 1, "I"},
	{"ReadPPointer",(FPFI) ReadPPointer, 1, "I"},
	{"ReadCString", (FPFI) ReadCString, 1, "I"},
	{"ReadPCString", (FPFI) ReadPCString, 2, "I.?"},
	{"ReadCharArray", (FPFI) ReadCharArray, 2, "II"},
	{"ReadWString", (FPFI) ReadWString, 2, "I.I"},
	{"ReadPWString", (FPFI) ReadPWString, 3, "I.I.?"},
	{"ReadWCharArray", (FPFI) ReadWCharArray, 3, "II.I"},
	{"ReadBytes", (FPFI) ReadBytes, 2, "II"},
	{"ReadFoxValue", (FPFI)ReadFoxValue, 1, "I"},

	/* convert FoxPro to C array and vice versa */
	{"MarshalFoxArray2CArray", (FPFI) MarshalFoxArray2CArray, 5 ,"IRI.I.I"},
	{"MarshalCArray2FoxArray", (FPFI) MarshalCArray2FoxArray, 5 ,"IRI.I.I"},

	/* convert FoxPro fields in cursor to C array and vice versa */
	{"MarshalCursor2CArray", (FPFI) MarshalCursor2CArray, 5,"ICI.I.I"},
	{"MarshalCArray2Cursor", (FPFI) MarshalCArray2Cursor, 5,"ICI.I.I"},

	/* numeric to binary & vice versa conversion routines */
	{"Str2Short", (FPFI) Str2Short, 1, "C"},
	{"Str2UShort", (FPFI) Str2UShort, 1, "C"},
	{"Str2Long", (FPFI) Str2Long, 1, "C" },
	{"Str2ULong", (FPFI) Str2ULong, 1, "C" },
	{"Str2Double", (FPFI) Str2Double, 1, "C" },
	{"Str2Float", (FPFI) Str2Float, 1, "C" },
	{"Str2Int64", (FPFI) Str2Int64, 2, "C.I" },
	{"Str2UInt64", (FPFI) Str2UInt64, 2, "C.I" },
	{"Short2Str", (FPFI) Short2Str, 1, "I" },
	{"UShort2Str", (FPFI) UShort2Str, 1, "I"},
	{"Long2Str", (FPFI) Long2Str, 1, "I"},
	{"ULong2Str", (FPFI) ULong2Str, 1, "N"},
	{"Double2Str", (FPFI) Double2Str, 1, "N"},
	{"Float2Str", (FPFI) Float2Str, 1, "N"},
	{"Int642Str", (FPFI) Int642Str, 2, "?.I"},
	{"UInt642Str", (FPFI) UInt642Str, 2, "?.I"},

	/* toolhelp32 api wrappers */
	{"AProcesses", (FPFI) AProcesses, 1, "C"},
	{"AProcessThreads", (FPFI) AProcessThreads, 2, "CI"},
	{"AProcessModules", (FPFI) AProcessModules, 2, "CI"},
	{"AProcessHeaps", (FPFI) AProcessHeaps, 2, "CI"},
	{"AHeapBlocks", (FPFI) AHeapBlocks, 3, "CII"},
	{"ReadProcessMemoryEx", (FPFI) ReadProcessMemoryEx, 3, "III"},
	
	/* enumeration routines */
	{"AWindowStations", (FPFI) AWindowStations, 1, "C"},
	{"ADesktops", (FPFI) ADesktops, 2, "C.I"},
	{"AWindows", (FPFI) AWindows, 3, "CI.I"},
	{"AWindowsEx", (FPFI) AWindowsEx, 4, "CCI.I"},
	{"AWindowProps", (FPFI) AWindowProps, 2, "CI"},
	{"AMonitors", (FPFI) AMonitors, 1, "C"},
	{"AResourceTypes", (FPFI) AResourceTypes, 2, "CI"},
	{"AResourceNames", (FPFI) AResourceNames, 3, "CI?"},
	{"AResourceLanguages", (FPFI) AResourceLanguages, 4, "CI??"},
	{"AResolutions", (FPFI) AResolutions, 2, "C.C"},
	{"ADisplayDevices", (FPFI) ADisplayDevices, 2, "C.C"},

	/* ODBC functions */
	{"CreateSQLDataSource", (FPFI) CreateSQLDataSource, 3, "CC.I"},
	{"DeleteSQLDataSource", (FPFI) DeleteSQLDataSource, 3, "CC.I"},
	{"ChangeSQLDataSource", (FPFI) ChangeSQLDataSource, 3, "CC.I"},
	{"ASQLDataSources", (FPFI) ASQLDataSources, 2, "C.I"},
	{"ASQLDrivers", (FPFI) ASQLDrivers, 1, "C"},
	{"SQLGetPropEx", (FPFI) SQLGetPropEx, 3, "?CR"},
	{"SQLSetPropEx", (FPFI) SQLSetPropEx, 3, "?C.?"},
	{"SQLExecEx", (FPFI) SQLExecEx, 9,        "I.C.C.C.I.C.C.C.I"},
	{"SQLPrepareEx", (FPFI) SQLPrepareEx, 9,  "IC.C.C.I.C.C.C.I"},
	{"SQLCancelEx", (FPFI) SQLCancelEx, 1, "I"},
	//{"TableUpdateEx", (FPFI) TableUpdateEx, 6, "IICCC.C"},

	/* printer functions */
	{"APrintersEx", (FPFI) APrintersEx, 5, "C.?.I.I.I"},
	{"APrintJobs", (FPFI) APrintJobs, 3, "CC.I"},
	{"APrinterForms", (FPFI) APrinterForms, 2, "C.C"},
	{"APaperSizes", (FPFI) APaperSizes, 4, "CCC.I"},
	{"APrinterTrays", (FPFI) APrinterTrays, 3, "CCC"},

	/* registry functions */
	{"CreateRegistryKey", (FPFI) CreateRegistryKey, 5, "IC.I.I.C"},
	{"DeleteRegistryKey", (FPFI) DeleteRegistryKey, 3, "IC.I"},
	{"OpenRegistryKey", (FPFI) OpenRegistryKey, 3, "IC.I"},
	{"CloseRegistryKey", (FPFI) CloseRegistryKey, 1, "I"},
	{"ReadRegistryKey", (FPFI) ReadRegistryKey, 4, "I.C.C.C"},
	{"WriteRegistryKey", (FPFI) WriteRegistryKey, 5, "I?.C.C.I"},
	{"ARegistryKeys", (FPFI) ARegistryKeys, 4, "CIC.I"},
	{"ARegistryValues", (FPFI) ARegistryValues, 4, "CIC.I"},
	{"RegistryValuesToObject", (FPFI) RegistryValuesToObject, 3, "ICO"},
	{"RegistryHiveToObject", (FPFI) RegistryHiveToObject, 3, "ICO"},

	/* file system functions */
	{"ADirEx",(FPFI) ADirEx, 7, "CC.?.I.I.C.I"},
	{"AFileAttributes", (FPFI) AFileAttributes, 4, "CC.L.L"},
	{"AFileAttributesEx", (FPFI) AFileAttributesEx, 4, "CC.L.L"},
	{"ADirectoryInfo", (FPFI) ADirectoryInfo, 2, "CC"},
	{"GetFileTimes", (FPFI) GetFileTimes, 5, "C?.?.?.L"},
	{"SetFileTimes", (FPFI) SetFileTimes, 5, "C?.?.?.L"},
	{"GetFileSize", (FPFI) GetFileSizeLib, 1, "C"},
	{"GetFileAttributes", (FPFI) GetFileAttributesLib, 2, "C.L"},
	{"SetFileAttributes", (FPFI) SetFileAttributesLib, 2, "C?"},
	{"GetFileOwner", (FPFI) GetFileOwner, 4, "CR.R.R"},
	{"GetLongPathName", (FPFI) GetLongPathNameLib, 1, "C"},
	{"GetShortPathName", (FPFI) GetShortPathNameLib, 1, "C"},
	{"DeleteDirectory", (FPFI) DeleteDirectory, 1, "C"},
	{"GetWindowsDirectory", (FPFI) GetWindowsDirectoryLib, 0, ""},
	{"GetSystemDirectory", (FPFI) GetSystemDirectoryLib, 0, ""},
	{"ExpandEnvironmentStrings", (FPFI) ExpandEnvironmentStringsLib, 1, "C"},
	{"GetOpenFileName", (FPFI) GetOpenFileNameLib, 8, ".I.C.C.C.C.I.C.C"},
	{"GetSaveFileName", (FPFI) GetSaveFileNameLib, 7, ".I.C.C.C.C.I.C"},
	{"ADriveInfo", (FPFI) ADriveInfo, 1, "C"},
	{"AVolumes", (FPFI) AVolumes, 1, "C"},
	{"AVolumeMountPoints", (FPFI) AVolumeMountPoints, 2, "CC"},
	{"AVolumePaths", (FPFI) AVolumePaths, 2, "CC"},
	{"AVolumeInformation", (FPFI) AVolumeInformation, 2, "CC"},
	{"CopyFileEx", (FPFI) CopyFileExLib, 4, "CC.C.I"},
	{"MoveFileEx", (FPFI) MoveFileExLib, 4, "CC.C.I"},
	{"CompareFileTimes", (FPFI) CompareFileTimes, 2, "CC"},
	{"DeleteFileEx", (FPFI) DeleteFileEx, 1, "C"},
	
	/* extended VFP like file functions */
	{"FCreateEx", (FPFI) FCreateEx, 4, "C.I.I.I"},
	{"FOpenEx", (FPFI) FOpenEx, 4, "C.I.I.I"},
	{"FCloseEx", (FPFI) FCloseEx, 1, "I"},
	{"FReadEx", (FPFI) FReadEx, 2, "II"},
	{"FWriteEx", (FPFI) FWriteEx, 3, "IC.I"},
	{"FGetsEx", (FPFI) FGetsEx, 2, "I.I"},
	{"FPutsEx", (FPFI) FPutsEx, 3, "I.C.I"},
	{"FSeekEx", (FPFI) FSeekEx, 3, "IN.I"},
	{"FEoFEx", (FPFI) FEoFEx, 1, "I"},
	{"FChSizeEx", (FPFI) FChSizeEx, 2, "IN"},
	{"FFlushEx", (FPFI) FFlushEx, 1, "I"},
	{"FLockFile", (FPFI) FLockFile, 3, "I??"},
	{"FUnlockFile", (FPFI) FUnlockFile, 3, "I??"},
	{"FLockFileEx", (FPFI) FLockFileEx, 4, "I??.I"},
	{"FUnlockFileEx", (FPFI) FUnlockFileEx, 3, "I??"},
	{"AFHandlesEx", (FPFI) AFHandlesEx, 1, "C"},

	/* some shell32.dll wrappers */
	{"SHSpecialFolder", (FPFI) SHSpecialFolder, 3, "IR.L"},
	{"SHMoveFiles", (FPFI) SHMoveFiles, 4, "CCI.C"},
	{"SHCopyFiles", (FPFI) SHCopyFiles, 4, "CC.I.C"},
	{"SHDeleteFiles", (FPFI) SHDeleteFiles, 3, "C.I.C"},
	{"SHRenameFiles", (FPFI) SHRenameFiles, 3, "CC.I"},
	{"SHBrowseFolder", (FPFI) SHBrowseFolder, 5, "CIR.C.C"},
	{"SHGetShellItem", (FPFI)SHGetShellItem, 1, "C"},

	/* windows message hooks */
#ifndef _THREADSAFE
	{"BindEventsEx", (FPFI) BindEventsEx, 6, "II?C.?.I"},
	{"UnbindEventsEx", (FPFI) UnbindEventsEx, 3, "I.I.L"},
	/* C callback function emulation */
	{"CreateCallbackFunc", (FPFI) CreateCallbackFunc, 5, "CCC.?.I"},
	{"DestroyCallbackFunc", (FPFI) DestroyCallbackFunc, 1, "I"},
#endif

	// some window functions
	{"GetWindowTextEx", (FPFI) GetWindowTextEx, 2, "I.L"},
	{"GetWindowRectEx", (FPFI) GetWindowRectEx, 2, "IC"},
	{"CenterWindowEx", (FPFI) CenterWindowEx, 2, "I.I"},
	{"ADesktopArea", (FPFI) ADesktopArea, 1, "C"},
	{"MessageBoxEx", (FPFI) MessageBoxExLib, 8, "C.I.C.?.?.?.?.I"},

	/* asynchronous notification functions */
#ifndef _THREADSAFE
	{"FindFileChange", (FPFI) FindFileChange, 4, "CLIC"},
	{"CancelFileChange", (FPFI) CancelFileChange, 1, "I"},
	{"FindFileChangeEx", (FPFI) FindFileChangeEx, 5, "CLIC.O"},
	{"CancelFileChangeEx", (FPFI) CancelFileChangeEx, 1, "?"},
	{"FindRegistryChange", (FPFI) FindRegistryChange, 5, "ICLIC"},
	{"CancelRegistryChange", (FPFI) CancelRegistryChange, 1, "I"},
	{"AsyncWaitForObject", (FPFI) AsyncWaitForObject, 2, "IC"},
	{"CancelWaitForObject", (FPFI) CancelWaitForObject, 1, "I"},
#endif

	/* time conversion routines */
	{"DT2FT", (FPFI) DT2FT, 3, "TI.L"},
	{"FT2DT", (FPFI) FT2DT, 2, "I.L"},
	{"DT2ST", (FPFI) DT2ST, 3, "TI.L"},
	{"ST2DT", (FPFI) ST2DT, 2, "I.L"},
	{"DT2UTC", (FPFI) DT2UTC, 1, "T"},
	{"UTC2DT", (FPFI) UTC2DT, 1, "T"},
	{"DT2Timet", (FPFI) DT2Timet, 2, ".T.L"},
	{"Timet2DT", (FPFI) Timet2DT, 2, "I.L" },
	{"DT2Double", (FPFI) DT2Double, 1, "T"},
	{"Double2DT", (FPFI) Double2DT, 1, "N"},
	{"SetSystemTime", (FPFI) SetSystemTimeLib, 2, "T.L"},
	{"GetSystemTime", (FPFI) GetSystemTimeLib, 1, ".L"},
	{"ATimeZones", (FPFI) ATimeZones, 1, "C"},

	/* netapi32 wrappers */
	{"ANetFiles", (FPFI) ANetFiles, 4, "C.?.?.?"},
	{"ANetServers", (FPFI) ANetServers, 4, "C.I.I.?"},
	{"GetServerTime", (FPFI) GetServerTime, 2, "C.I"},

	/* SNTP */
	{"SyncToSNTPServer", (FPFI) SyncToSNTPServer, 3, "C.I.I"},

	/* COM routines */
	{"CLSIDFromProgID", (FPFI) CLSIDFromProgIDLib, 1, "C"},
	{"ProgIDFromCLSID", (FPFI) ProgIDFromCLSIDLib, 1, "?"},
	{"CLSIDFromString", (FPFI) CLSIDFromStringLib, 1, "C"},
	{"StringFromCLSID", (FPFI) StringFromCLSIDLib, 1, "?"},
	{"IsEqualGuid", (FPFI) IsEqualGUIDLib, 2, "??"},
	{"CreateGuid", (FPFI) CreateGuid, 1, ".I"},
	{"RegisterActiveObject", (FPFI) RegisterActiveObjectLib, 2, "OC"},
	{"RegisterObjectAsFileMoniker", (FPFI) RegisterObjectAsFileMoniker, 3, "OCC"},
	{"RevokeActiveObject", (FPFI) RevokeActiveObjectLib, 1, "I"},
	{"CreateThreadObject", (FPFI) CreateThreadObject, 5, "C.?.L.I.I"},

	/* urlmon wrappers */
#ifndef _THREADSAFE
	{"UrlDownloadToFileEx", (FPFI) UrlDownloadToFileEx, 5, "CC.C.L.L"},
	{"AbortUrlDownloadToFileEx", (FPFI) AbortUrlDownloadToFileEx, 1, "I"},
#endif
	/* winsock functions */
	{"AIPAddresses", (FPFI) AIPAddresses, 1, "C"},
	{"ResolveHostToIp", (FPFI) ResolveHostToIp, 2, "C.C"},
	/* IP Helper */
	{"Ip2MacAddress", (FPFI) Ip2MacAddress, 1, "C"},
	{"IcmpPing", (FPFI)IcmpPing, 8, "CC.I.I.I.I.L.I"},

	/* WinInet wrappers */
	/*
	{"InitWinInet", (FPFI) InitWinInet, 5, ".C.I.C.C.I"},
	{"FTPConnect", (FPFI) FTPConnect, 5, "CCC.I.I"}, 
	{"FTPDisconnect", (FPFI) FTPDisconnect, 1, "I"},
	{"FTPGetFile", (FPFI) FTPGetFileLib, 2, "IC"},
	{"FTPPutFile", (FPFI) FTPPutFileLib, 2, "IC"},
	{"FTPGetDirectory", (FPFI) FTPGetDirectory, 1, "I"},
	{"FTPSetDirectory", (FPFI) FTPSetDirectory, 2, "IC"},
	{"AFTPFiles", (FPFI) AFTPFiles, 3, "C"},
	{"HTTPGetFile", (FPFI) HTTPGetFile, 4, "C.C.C.L"},
	*/

	/* service functions */
	{"OpenService", (FPFI) OpenServiceLib, 4, "C.I.C.C"},
	{"CloseServiceHandle", (FPFI) CloseServiceHandleLib, 1, "I"},
	{"StartService", (FPFI) StartServiceLib, 5, "?.?.?.C.C"},
	{"StopService", (FPFI) StopServiceLib, 5, "?.?.L.C.C"},
	{"PauseService", (FPFI) PauseService, 4, "?.?.C.C"},
	{"ContinueService", (FPFI) ContinueService, 4, "?.?.C.C"},
	{"ControlService", (FPFI) ControlServiceLib, 4, "?.I.C.C"},
	{"AServiceStatus", (FPFI) AServiceStatus, 4, "C?.C.C"},
	{"AServiceConfig", (FPFI) AServiceConfig, 4, "C?.C.C"},
	{"AServices", (FPFI) AServices, 5, "C.C.C.I.I"},
	{"ADependentServices", (FPFI) ADependentServices, 4, "C?.C.C"},
	{"WaitForServiceStatus", (FPFI) WaitForServiceStatus, 5, "?I.I.C.C"},
	{"CreateService", (FPFI) CreateServiceLib, 12, "CC.?.?.?.C.C.C.C.C.C.C"},
	{"DeleteService", (FPFI) DeleteServiceLib, 3, "?.C.C"},

	/* misc data conversion/string functions */
	{"PG_ByteA2Str", (FPFI) PG_ByteA2Str, 1, "?"},
	{"PG_Str2ByteA", (FPFI) PG_Str2ByteA, 2, "?.L"},
	{"RGB2Colors", (FPFI) RGB2Colors, 5, "IRRR.R"},
	{"Colors2RGB", (FPFI) Colors2RGB, 4, "III.I"},
	{"GetCursorPosEx", (FPFI) GetCursorPosEx, 4, "RR.L.?"},
	{"Int64_Add", (FPFI) Int64_Add, 3, "??.I"},
	{"Int64_Sub", (FPFI) Int64_Sub, 3, "??.I"},
	{"Int64_Mul", (FPFI) Int64_Mul, 3, "??.I"},
	{"Int64_Div", (FPFI) Int64_Div, 3, "??.I"},
	{"Int64_Mod", (FPFI) Int64_Mod, 3, "??.I"},
	{"Value2Variant", (FPFI) Value2Variant, 1, "?"},
	{"Variant2Value", (FPFI) Variant2Value, 1, "?"},
	{"Decimals", (FPFI) Decimals, 1, "N"},
	{"Num2Binary", (FPFI) Num2Binary, 1, "N"},
	{"CreatePublicShadowObjReference", (FPFI) CreatePublicShadowObjReference, 2, "CO"},
	{"ReleasePublicShadowObjReference", (FPFI) ReleasePublicShadowObjReference, 1, "C"},
	{"GetLocaleInfoEx", (FPFI) GetLocaleInfoExLib, 2, "I.I"},
	{"OsEx", (FPFI) OsEx, 0, ""},

	/* array routines */
	{"ASum", (FPFI) ASum, 2, "R.I"},
	{"AAverage", (FPFI) AAverage, 2, "R.I"},
	{"AMax", (FPFI) AMax, 2, "R.I"},
	{"AMin", (FPFI) AMin, 2, "R.I"},
	{"ASplitStr", (FPFI) ASplitStr, 3, "CCI"},

	/* RAS wrappers */
	{"ARasConnections", (FPFI) ARasConnections, 1, "C"},
	{"ARasDevices", (FPFI) ARasDevices, 1, "C"},
	{"ARasPhonebookEntries", (FPFI)ARasPhonebookEntries, 2, "C.C"},
	{"RasPhonebookDlgEx", (FPFI) RasPhonebookDlgEx, 4, ".?.?.?.I"},
	{"RasHangUpEx", (FPFI) RasHangUpEx, 1, "I"},
	{"RasGetConnectStatusEx", (FPFI) RasGetConnectStatusEx, 2, "IC"},
	{"RasDialDlgEx", (FPFI) RasDialDlgEx, 5, ".C.C.C.I.I"},
	{"RasClearConnectionStatisticsEx", (FPFI) RasClearConnectionStatisticsEx, 1, "I"},
	{"RasDialEx",(FPFI) RasDialEx, 5, ".?.C.C.I.I"},
#ifndef _THREADSAFE
	{"RasConnectionNotificationEx", (FPFI) RasConnectionNotificationEx, 3, "IIC"},
	{"AbortRasConnectionNotificationEx", (FPFI) AbortRasConnectionNotificationEx, 1, "I"},
#endif

	/* Font routines */
	{"AFontInfo", (FPFI) AFontInfo, 3, "C.I.I"},

	/* error handling routines */
	{"FormatMessageEx", (FPFI) FormatMessageEx, 3, "I.I.I"},
	{"AErrorEx", (FPFI) AErrorEx, 1, "C"}

#ifdef _DEBUG
	,{"AMemLeaks", (FPFI) AMemLeaks, 1, "C"}
	,{"TrackMem", (FPFI) TrackMem, 2, "L.L"}
#endif
};

FoxTable _FoxTable = {
(FoxTable *)0, sizeof(VFP2CFuncs)/sizeof(FoxInfo), VFP2CFuncs
};

#ifdef __cplusplus
}
#endif
