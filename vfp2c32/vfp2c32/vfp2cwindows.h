#ifndef _VFP2CWINDOWS_H__
#define _VFP2CWINDOWS_H__

#ifdef __cplusplus
extern "C" {
#endif

void _fastcall GetWindowTextEx(ParamBlkEx& parm);
void _fastcall GetWindowRectEx(ParamBlkEx& parm);
void _fastcall CenterWindowEx(ParamBlkEx& parm);
void _fastcall ADesktopArea(ParamBlkEx& parm);
void _fastcall ColorOfPoint(ParamBlkEx& parm);
void _fastcall MessageBoxExLib(ParamBlkEx& parm);

#ifdef __cplusplus
}
#endif

#endif // _VFP2CWINDOWS_H__