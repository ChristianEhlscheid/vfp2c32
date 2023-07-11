#ifndef _VFP2C32_H__
#define _VFP2C32_H__

#include <assert.h>
#include "windows.h"
#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2ctls.h"
#include "vfp2ctypes.h"

// filename of FLL
#if !defined(_WIN64)
#if !defined(_DEBUG)
	#if defined(_THREADSAFE)
		#define FLLFILENAME "vfp2c32t.fll"
	#else
		#define FLLFILENAME "vfp2c32.fll"
	#endif
#else
	#if defined(_THREADSAFE)
		#define FLLFILENAME "vfp2c32dt.fll"
	#else
		#define FLLFILENAME "vfp2c32d.fll"
	#endif
#endif
#else
#if !defined(_DEBUG)
	#if defined(_THREADSAFE)
		#define FLLFILENAME "vfp2c64t.fll"
	#else
		#define FLLFILENAME "vfp2c64.fll"
	#endif
#else
	#if defined(_THREADSAFE)
		#define FLLFILENAME "vfp2c64dt.fll"
	#else
		#define FLLFILENAME "vfp2c64d.fll"
	#endif
#endif
#endif

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

void _fastcall VFP2CSys(ParamBlkEx& parm);
void _fastcall AErrorEx(ParamBlkEx& parm);
void _fastcall FormatMessageEx(ParamBlkEx& parm);

// extern definitions
extern HMODULE ghModule;

#ifdef __cplusplus
}
#endif

// function forward definitions
void _stdcall Win32ErrorHandler(char* pFunction, DWORD nErrorNo, bool bAddError, bool bRaise);
void _stdcall CustomErrorHandler(char* pFunction, char* pErrorMessage, bool bAddError, bool bRaise);
void _stdcall CustomErrorHandler(char* pFunction, char* pErrorMessage, bool bAddError, bool bRaise, va_list lpArgs);
void _stdcall CustomErrorHandlerEx(char* pFunction, char* pErrorMessage, int nErrorNo, bool bAddError, bool bRaise, va_list lpArgs);

void _cdecl SaveCustomError(char* pFunction, char* pMessage, ...);
void _cdecl AddCustomError(char* pFunction, char* pMessage, ...);
void _cdecl RaiseCustomError(char* pFunction, char* pMessage, ...);

void _cdecl SaveCustomErrorEx(char* pFunction, char* pMessage, int nErrorNo, ...);
void _cdecl AddCustomErrorEx(char* pFunction, char* pMessage, int nErrorNo, ...);
void _cdecl RaiseCustomErrorEx(char* pFunction, char* pMessage, int nErrorNo, ...);

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

inline void SaveWin32Error(char *pFunction, unsigned long nErrorNo) { Win32ErrorHandler(pFunction, nErrorNo, false, false); }
inline void AddWin32Error(char *pFunction, unsigned long nErrorNo) { Win32ErrorHandler(pFunction, nErrorNo, true, false); }
inline void RaiseWin32Error(char *pFunction, unsigned long nErrorNo) { Win32ErrorHandler(pFunction, nErrorNo, false, true); }

#include "vfp2cstring.h"
#include "vfp2ctemplates.h"
#include "vfp2ccppapi.h"
#include "vfp2cdatastructure.h"
#include "vfp2cutil.h"
#include "vfp2chelpers.h"

#endif // _VFP2C32_H__
