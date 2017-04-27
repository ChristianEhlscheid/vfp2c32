#ifndef _VFP2CENUM_H__
#define _VFP2CENUM_H__

#include <tlhelp32.h>
#include <vdmdbg.h>

#include "vfp2ccppapi.h"
#include "vfp2chelpers.h"

// custom class windowstation and desktop enumeration functions
class EnumParameter
{
public:
	EnumParameter() { pName.Size(1024); }
	FoxArray pArray;
	FoxString pName;
};

// custom types and defines for window enumeration & window property enumeration functions 
const int WINDOW_ENUM_CLASSLEN	= 128;
const int WINDOW_ENUM_TEXTLEN	= 4096;
const int WINDOW_ENUM_TOPLEVEL	= 1;
const int WINDOW_ENUM_CHILD		= 2;
const int WINDOW_ENUM_THREAD	= 4;
const int WINDOW_ENUM_DESKTOP	= 8;
const int WINDOW_ENUM_CALLBACK	= 16;
const int WINDOW_ENUM_FLAGS		= 15;
const int WINDOWPROP_ENUM_LEN	= 1024;

class WindowEnumParam
{
public:
	FoxArray pArray;
	CStr pBuffer;
	CStr pCallback;
};

class WindowEnumParamEx
{
public:
	WindowEnumParamEx() { pBuffer.Size(WINDOW_ENUM_CLASSLEN); }
	FoxArray pArray;
	FoxString pBuffer;
	unsigned short aFlags[WINDOW_ENUM_FLAGS];
};

// custom types and defines for resource enumeration functions
const int RESOURCE_ENUM_TYPELEN	= 512;
const int RESOURCE_ENUM_NAMELEN	= 2048;

typedef struct _RESOURCEENUMPARAM {
	FoxArray pArray;
	FoxString pBuffer;
} RESOURCEENUMPARAM, *LPRESOURCEENUMPARAM;

typedef struct _PROCESS_BASIC_INFORMATION_EX {
    int ExitStatus;
    void* PebBaseAddress;
    unsigned int AffinityMask;
    int BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION_EX, *LPPROCESS_BASIC_INFORMATION_EX;

const int DISPLAYDEVICE_ENUM_LEN = 128;

#ifdef __cplusplus
extern "C" {
#endif

// function prototypes of vfp2cenum.c
void _fastcall AWindowStations(ParamBlk *parm);
BOOL _stdcall WindowStationEnumCallback(LPSTR lpszWinSta, LPARAM nParam);
void _fastcall ADesktops(ParamBlk *parm);
BOOL _stdcall DesktopEnumCallback(LPCSTR lpszDesktop, LPARAM nParam);
void _fastcall AWindows(ParamBlk *parm);
BOOL _stdcall WindowEnumCallback(HWND nHwnd, LPARAM nParam);
#pragma warning(disable : 4290) // disable warning 4290 - VC++ doesn't implement throw ...
BOOL _stdcall WindowEnumCallbackCall(HWND nHwnd, LPARAM nParam) throw(int);
void _fastcall AWindowsEx(ParamBlk *parm);
BOOL _stdcall WindowEnumCallbackEx(HWND nHwnd, LPARAM nParam);
void _fastcall AWindowProps(ParamBlk *parm);
BOOL _stdcall WindowPropEnumCallback(HWND nHwnd, LPCSTR pPropName, HANDLE hData, DWORD nParam);
void _fastcall AProcesses(ParamBlk *parm);
void _fastcall AProcessesPSAPI(ParamBlk *parm);
void _fastcall AProcessThreads(ParamBlk *parm);
void _fastcall AProcessModules(ParamBlk *parm);
void _fastcall AProcessHeaps(ParamBlk *parm);
void _fastcall AHeapBlocks(ParamBlk *parm);
void _fastcall ReadProcessMemoryEx(ParamBlk *parm);
void _fastcall AResourceTypes(ParamBlk *parm);
BOOL _stdcall ResourceTypesEnumCallback(HANDLE hModule, LPSTR lpszType, LONG nParam);
void _fastcall AResourceNames(ParamBlk *parm);
BOOL _stdcall ResourceNamesEnumCallback(HANDLE hModule, LPCSTR lpszType, LPSTR lpszName, LONG_PTR nParam);
void _fastcall AResourceLanguages(ParamBlk *parm);
BOOL _stdcall ResourceLangEnumCallback(HANDLE hModule, LPCSTR lpszType, LPCSTR lpszName,
									   WORD wIDLanguage, LONG nParam);
void _fastcall AResolutions(ParamBlk *parm);
void _fastcall ADisplayDevices(ParamBlk *parm);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif // _VFP2CENUM_H__