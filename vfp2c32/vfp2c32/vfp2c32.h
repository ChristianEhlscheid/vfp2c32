#ifndef _VFP2C32_H__
#define _VFP2C32_H__

#include <assert.h>
#include "windows.h"
#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif

#define countof(lpParam) (sizeof(lpParam) / sizeof(lpParam[0]))

#include "vfp2ctls.h"
#include "vfp2ctypes.h"

#if _MSC_VER >= 1900
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif

const unsigned int VFP2C_MAX_CALLBACKFUNCTION	= 1024;
const unsigned int VFP2C_MAX_CALLBACKBUFFER		= 2048;
const unsigned int VFP2C_MAX_FUNCTIONBUFFER		= 256;

const int E_APIERROR	= 12345678;

#ifdef __cplusplus
extern "C" {
#endif

void _fastcall OnLoad();
void _fastcall OnUnload();

void _fastcall VFP2C32(ParamBlk& parm);
void _fastcall VFP2CSys(ParamBlkEx& parm);
void _fastcall AErrorEx(ParamBlkEx& parm);
void _fastcall FormatMessageEx(ParamBlkEx& parm);

// extern definitions
extern HMODULE ghModule;

#ifdef __cplusplus
}
#endif

// function forward definitions
void _stdcall Win32ErrorHandler(const char* pFunction, DWORD nErrorNo, bool bAddError, bool bRaise);
void _stdcall CustomErrorHandler(const char* pFunction, const char* pErrorMessage, bool bAddError, bool bRaise);
void _stdcall CustomErrorHandler(const char* pFunction, const char* pErrorMessage, bool bAddError, bool bRaise, va_list lpArgs);
void _stdcall CustomErrorHandlerEx(const char* pFunction, const char* pErrorMessage, int nErrorNo, bool bAddError, bool bRaise, va_list lpArgs);

void _cdecl SaveCustomError(const char* pFunction, const char* pMessage, ...);
void _cdecl AddCustomError(const char* pFunction, const char* pMessage, ...);
void _cdecl RaiseCustomError(const char* pFunction, const char* pMessage, ...);

void _cdecl SaveCustomErrorEx(const char* pFunction, const char* pMessage, int nErrorNo, ...);
void _cdecl AddCustomErrorEx(const char* pFunction, const char* pMessage, int nErrorNo, ...);
void _cdecl RaiseCustomErrorEx(const char* pFunction, const char* pMessage, int nErrorNo, ...);

inline void _stdcall RaiseError(int nErrorNo)
{
	if (nErrorNo == E_APIERROR)
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		_UserError(tls.ErrorInfo[tls.ErrorCount].aErrorMessage);
	}
	else
		_Error(nErrorNo);
}

inline void SaveWin32Error(const char *pFunction, unsigned long nErrorNo) { Win32ErrorHandler(pFunction, nErrorNo, false, false); }
inline void AddWin32Error(const char *pFunction, unsigned long nErrorNo) { Win32ErrorHandler(pFunction, nErrorNo, true, false); }
inline void RaiseWin32Error(const char *pFunction, unsigned long nErrorNo) { Win32ErrorHandler(pFunction, nErrorNo, false, true); }

#include "vfp2ctemplates.h"
#include "vfp2cstring.h"
#include "vfp2ccppapi.h"
#include "vfp2cdatastructure.h"
#include "vfp2cutil.h"
#include "vfp2chelpers.h"

#endif // _VFP2C32_H__
