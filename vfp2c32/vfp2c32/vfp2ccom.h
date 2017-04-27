#ifndef _VFP2CCOM_H__
#define _VFP2CCOM_H__

void _stdcall GetIDispatchFromObject(Value &pVal, void** pDisp);

#ifdef __cplusplus
extern "C" {
#endif

const int GUID_STRING_LEN	= 40;
const int CREATE_GUID_ANSI		= 0;
const int CREATE_GUID_UNICODE	= 1;
const int CREATE_GUID_BINARY	= 2;

void _stdcall VFP2C_Init_Com();
void _stdcall VFP2C_Destroy_Com(VFP2CTls& tls);

void _fastcall CLSIDFromProgIDLib(ParamBlk *parm);
void _fastcall ProgIDFromCLSIDLib(ParamBlk *parm);
void _fastcall CLSIDFromStringLib(ParamBlk *parm);
void _fastcall StringFromCLSIDLib(ParamBlk *parm);
void _fastcall IsEqualGUIDLib(ParamBlk *parm);
void _fastcall GuidFromString(ParamBlk *parm);
void _fastcall CreateGuid(ParamBlk *parm);
void _fastcall RegisterActiveObjectLib(ParamBlk *parm);
void _fastcall RegisterObjectAsFileMoniker(ParamBlk *parm);
void _fastcall RevokeActiveObjectLib(ParamBlk *parm);
IDispatch * _stdcall GetIDispatch(IDispatch *pObject);

/*
void _fastcall Sys3095Ex(ParamBlk *parm);
void _fastcall IsObjectActive(ParamBlk *parm);
void _fastcall CoCreateInstanceExLib(ParamBlk *parm);
void _fastcall CoRegisterComDll(ParamBlk *parm);
void _fastcall CoUnregisterComDll(ParamBlk *parm);
void _fastcall IDispatch_Invoke(ParamBlk *parm);
void _fastcall IDispatch_AsyncInvoke(ParamBlk *parm);
DWORD _stdcall IDispatch_AsyncInvokeThreadProc(LPVOID lpParam);
*/


#ifdef __cplusplus
}
#endif

#endif /* _VFP2COLE_H__ */
