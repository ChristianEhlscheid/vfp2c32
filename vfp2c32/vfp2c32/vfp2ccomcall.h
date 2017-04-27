#ifndef _VFP2CCOMCALL_H__
#define _VFP2CCOMCALL_H__

#include <windows.h>
#include "pro_ext.h"
#include "vfp2c32.h"
#include <vector>

typedef HRESULT (_stdcall *PDLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID*); // DllGetClassObject
typedef HRESULT (_stdcall *PLOADTYPELIBEX)(LPCOLESTR, REGKIND, ITypeLib**); // LoadTypeLibEx

class RegisteredComDll
{
public:
	RegisteredComDll() : m_hDll(0) {}
	~RegisteredComDll();

	bool CompareHandle(HMODULE hDll);
	void RegisterDll(const char *pDll, wchar_t *pDllW);

private:
	HMODULE m_hDll;
	std::vector<DWORD> m_CoClasses;
};

#define VFP2C_COMCALL_PARMOFFSET 3

class ComCall
{
public:
	ComCall();
	~ComCall();

	void SetCallInfo(LPOLESTR pClass, LPOLESTR pMethod, DWORD dwContext, LCID nLocale);
	void SetCallInfo(IDispatch *pDisp, LPOLESTR pMethod, LCID nLocale);
	void SetResultInfo(IUnknown *pResObj, LPOLESTR pMethod);
	void SetParameterCount(unsigned int nParmCount);
	void MarshalParameter(unsigned int nParmNo, Parameter &pParm, VARTYPE vType, int nBase);
	void UnMarshalRefParameters(ParamBlk *parm);
	void UnMarshalIDispatchParameters();
	void CallMethod(Value &vRetVal, ParamBlk *parm);
	void CallMethodAsync();

private:
	IDispatch *m_Disp;
	CLSID m_ClsId;
	DISPPARAMS m_Parameters;
	LPOLESTR m_Method;
	LPOLESTR m_ResultMethod;
	LPSTREAM m_ResultObj;
	DWORD m_Context;
	LCID m_Locale;
	bool m_Async;
};



void _stdcall MarshalVariant(VARIANT &pArg, Value &pValue, VARTYPE vType);
void _stdcall MarshalVariantEx(VARIANT &pArg, Value &pValue, VARTYPE vType, int nBase);
void _stdcall MarshalString(VARIANT &pArg, Value &pValue);
void _stdcall MarshalDate(VARIANT &pArg, Value &pValue);
void _stdcall MarshalIDispatch(VARIANT &pArg, Value &pValue);
void _stdcall MarshalIDispatchCrossThread(VARIANT &pArg, Value &pValue);
void _stdcall MarshalDecimal(VARIANT &pArg, Value &pValue);
void _stdcall MarshalDecimal(DECIMAL &pDec, Value &pValue);

void _stdcall MarshalSafeArrayVariant(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase);
void _stdcall MarshalSafeArrayInt(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase);
void _stdcall MarshalSafeArraySingle(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase);
void _stdcall MarshalSafeArrayDouble(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase);
void _stdcall MarshalSafeArrayBool(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase);
void _stdcall MarshalSafeArrayDate(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase);
void _stdcall MarshalSafeArrayBSTR(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase);
void _stdcall MarshalSafeArrayUI1(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase);
void _stdcall MarshalSafeArrayDecimal(VARIANT &pArg, Value &pArrayName, VARTYPE vType, int nBase);
void _stdcall MarshalSafeArrayEmpty(VARIANT &pArg, VARTYPE vType, int nBase);

void _stdcall UnMarshalVariant(VARIANT &vVar, Value &pVal);
void _stdcall UnMarshalDate(VARIANT &vVar, Value &pVal);
void _stdcall UnMarshalBSTR(VARIANT &vVar, Value &pVal);
void _stdcall UnMarshalIDispatch(VARIANT &vVar, Value &pVal);
void _stdcall UnMarshalIUnknown(VARIANT &vVar, Value &pVal);
void _stdcall UnMarshalDecimal(VARIANT &vVar, Value &pVal);
void _stdcall UnMarshalSafeArrayUI1(VARIANT &vVar, Value &pVal);

void _stdcall ReleaseVarientEx(VARIANT *pArg);

#endif /* _VFP2CCOMCALL_H__ */
