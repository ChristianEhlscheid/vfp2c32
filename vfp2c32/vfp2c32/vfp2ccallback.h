#ifndef _VFP2CCALLBACK_H__
#define _VFP2CCALLBACK_H__

#include "vfp2ctls.h"

const int VFP2C_MAX_TYPE_LEN				=  32;
const int VFP2C_MAX_CALLBACK_FUNCTION		= 768;
const int VFP2C_MAX_CALLBACK_FUNCTION_EX	= 1024;
const int VFP2C_MAX_CALLBACK_BUFFER			= 4096;
const int VFP2C_MAX_CALLBACK_PARAMETERS		= 27;

const unsigned int BINDEVENTSEX_CALL_BEFORE		= 0x0001;
const unsigned int BINDEVENTSEX_CALL_AFTER		= 0x0002;
const unsigned int BINDEVENTSEX_RETURN_VALUE	= 0x0004;
const unsigned int BINDEVENTSEX_NO_RECURSION	= 0x0008;
const unsigned int BINDEVENTSEX_CLASSPROC		= 0x0010;

const unsigned int CALLBACK_SYNCRONOUS			= 0x01;
const unsigned int CALLBACK_ASYNCRONOUS_POST	= 0x02;
const unsigned int CALLBACK_ASYNCRONOUS_SEND	= 0x04;
const unsigned int CALLBACK_CDECL				= 0x08;
const unsigned int CALLBACK_STDCALL				= 0x10;
const unsigned int CALLBACK_FASTCALL			= 0x20;

const UINT WM_ASYNCCALLBACK	= WM_USER + 1;

class WindowMessageCallback
{
public:
	WindowMessageCallback(UINT uMsg = 0);
	void Release(WindowSubclass* pSubclass);
	UINT m_Msg;
	NTI m_Object;
	void* m_CallbackThunk;
	char* m_CallbackFunction;
	bool m_ReturnValue;
};

class WindowSubclass
{
public:
	WindowSubclass();
	~WindowSubclass();
	void SubclassWindow(HWND hHwnd, bool bClassProc);
	WindowMessageCallback* AddMessageCallback(UINT uMsg);
	bool RemoveMessageCallback(UINT uMsg);
	void CreateSubclassThunkProc();
	void CreateSubclassMsgThunkProc(WindowMessageCallback* lpMsg, CStringView pCallback, CStringView pParameterList, DWORD nFlags, BOOL bObjectCall);
	static WindowMessageCallback* _stdcall FindMessageCallback(WindowSubclass* pSubclass, UINT uMsg);
	static WindowSubclass* _stdcall FindWindowSubclass(HWND hHwnd, bool bClassProc);
	static void _stdcall ReleaseWindowSubclasses();

	HWND m_Hwnd;
	WNDPROC m_DefaultWndProc;
	void* m_WindowThunk;
	CArray<WindowMessageCallback> m_BoundMessages;
	bool m_ClassProc;
	char m_CallbackBuffer[VFP2C_MAX_CALLBACK_BUFFER];

private:
	void UnsubclassWindow();
	static void UnsubclassWindowClassCallback(HWND hHwnd, LPARAM lParam);
	static void UnsubclassWindowClassCallbackChild(HWND hHwnd, LPARAM lParam);
};

class CallbackFunction {
public:
	CallbackFunction();
	~CallbackFunction();
	static void ReleaseCallbackFuncs();
	char m_CallbackBuffer[VFP2C_MAX_CALLBACK_BUFFER];
	NTI m_Object;
	void *m_FuncAddress;
};

#ifdef __cplusplus
extern "C" {
#endif

int _stdcall VFP2C_Init_Callback();
void _stdcall VFP2C_Destroy_Callback(VFP2CTls& tls);

void _fastcall CreateCallbackFunc(ParamBlkEx& parm);
void _fastcall DestroyCallbackFunc(ParamBlkEx& parm);

void _fastcall BindEventsEx(ParamBlkEx& parm);
void _fastcall UnbindEventsEx(ParamBlkEx& parm);

#ifdef __cplusplus
} // extern C
#endif

LRESULT _stdcall AsyncCallbackWindowProc(HWND nHwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif // _VFP2CCALLBACK_H__