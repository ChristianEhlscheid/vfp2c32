#ifndef _VFP2CREGISTRY_H__
#define _VFP2CREGISTRY_H__

#include "vfp2ccppapi.h"

#ifdef __cplusplus
extern "C" {
#endif

// custom defines for Registry key/value enumeration
const int REG_ENUMCLASSNAME	= 1;
const int REG_ENUMWRITETIME	= 2;
const int REG_ENUMTYPE		= 1;
const int REG_ENUMVALUE		= 2;

const int REG_DELETE_NORMAL	= 1;
const int REG_DELETE_SHELL	= 2;

const unsigned int REG_INTEGER	= 12;
const unsigned int REG_DOUBLE	= 13;
const unsigned int REG_DATE		= 14;
const unsigned int REG_DATETIME	= 15;
const unsigned int REG_LOGICAL	= 16;
const unsigned int REG_MONEY	= 17;

inline bool REG_KEY_PREDEFINDED(HKEY hKey) { return hKey == HKEY_CLASSES_ROOT || hKey == HKEY_CURRENT_CONFIG || hKey == HKEY_CURRENT_USER || 
													hKey == HKEY_LOCAL_MACHINE || hKey == HKEY_USERS || hKey == HKEY_DYN_DATA; }
inline bool REG_KEY_STRING(DWORD hKeyType) { return hKeyType == REG_SZ || hKeyType == REG_MULTI_SZ || hKeyType == REG_EXPAND_SZ; }
inline bool REG_KEY_CHARACTER(DWORD hKeyType) { return hKeyType == REG_SZ || hKeyType == REG_MULTI_SZ || hKeyType == REG_EXPAND_SZ || hKeyType == REG_BINARY; }
inline bool REG_KEY_NUMERIC(DWORD hKeyType) { return hKeyType == REG_DWORD || hKeyType == REG_QWORD || hKeyType == REG_INTEGER || hKeyType == REG_DOUBLE; }

void _fastcall CreateRegistryKey(ParamBlk *parm);
void _fastcall DeleteRegistryKey(ParamBlk *parm);
void _fastcall OpenRegistryKey(ParamBlk *parm);
void _fastcall CloseRegistryKey(ParamBlk *parm);
void _fastcall ReadRegistryKey(ParamBlk *parm);
void _fastcall WriteRegistryKey(ParamBlk *parm);
void _fastcall ARegistryKeys(ParamBlk *parm);
void _fastcall ARegistryValues(ParamBlk *parm);
void _fastcall RegistryValuesToObject(ParamBlk *parm);
void _fastcall RegistryHiveToObject(ParamBlk *parm);
#pragma warning(disable : 4290) // disable warning 4290 - VC++ doesn't implement throw ...
void _stdcall RegistryHiveSubroutine(HKEY hKey, char *pKey, FoxObject& pObject) throw(int);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif // _VFP2CREGISTRY_H__