#ifndef _VFP2CCOM_H__
#define _VFP2CCOM_H__

#ifdef __cplusplus
extern "C" {
#endif

const int GUID_STRING_LEN	= 40;
const int CREATE_GUID_ANSI		= 0;
const int CREATE_GUID_UNICODE	= 1;
const int CREATE_GUID_BINARY	= 2;

void _stdcall VFP2C_Init_Com();
void _stdcall VFP2C_Destroy_Com(VFP2CTls& tls);

void _fastcall CLSIDFromProgIDLib(ParamBlkEx& parm);
void _fastcall ProgIDFromCLSIDLib(ParamBlkEx& parm);
void _fastcall CLSIDFromStringLib(ParamBlkEx& parm);
void _fastcall StringFromCLSIDLib(ParamBlkEx& parm);
void _fastcall IsEqualGUIDLib(ParamBlkEx& parm);
void _fastcall GuidFromString(ParamBlkEx& parm);
void _fastcall CreateGuid(ParamBlkEx& parm);
void _fastcall RegisterActiveObjectLib(ParamBlkEx& parm);
void _fastcall RegisterObjectAsFileMoniker(ParamBlkEx& parm);
void _fastcall RevokeActiveObjectLib(ParamBlkEx& parm);
IDispatch * _stdcall GetIDispatch(IDispatch *pObject);

/*
void _fastcall Sys3095Ex(ParamBlkEx& parm);
void _fastcall IsObjectActive(ParamBlkEx& parm);
void _fastcall CoCreateInstanceExLib(ParamBlkEx& parm);
void _fastcall CoRegisterComDll(ParamBlkEx& parm);
void _fastcall CoUnregisterComDll(ParamBlkEx& parm);
void _fastcall IDispatch_Invoke(ParamBlkEx& parm);
void _fastcall IDispatch_AsyncInvoke(ParamBlkEx& parm);
DWORD _stdcall IDispatch_AsyncInvokeThreadProc(LPVOID lpParam);
*/


#ifdef __cplusplus
}
#endif

#endif /* _VFP2COLE_H__ */
