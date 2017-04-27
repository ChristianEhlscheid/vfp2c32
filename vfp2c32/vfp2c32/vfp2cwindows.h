#ifndef _VFP2CWINDOWS_H__
#define _VFP2CWINDOWS_H__

#ifdef __cplusplus
extern "C" {
#endif

void _fastcall GetWindowTextEx(ParamBlk *parm);
void _fastcall GetWindowRectEx(ParamBlk *parm);
void _fastcall CenterWindowEx(ParamBlk *parm);
void _fastcall ADesktopArea(ParamBlk *parm);
void _fastcall ColorOfPoint(ParamBlk *parm);
void _fastcall MessageBoxExLib(ParamBlk *parm);

#ifdef __cplusplus
}
#endif

#endif // _VFP2CWINDOWS_H__