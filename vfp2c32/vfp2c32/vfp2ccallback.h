#ifndef _THREADSAFE

#ifndef _VFP2CCALLBACK_H__
#define _VFP2CCALLBACK_H__

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

const unsigned int CALLBACK_SYNCRONOUS			= 1;
const unsigned int CALLBACK_ASYNCRONOUS_POST	= 2;
const unsigned int CALLBACK_ASYNCRONOUS_SEND	= 4;
const unsigned int CALLBACK_CDECL				= 8;

const UINT WM_ASYNCCALLBACK	= WM_USER + 1;

typedef struct _MSGCALLBACK {
	UINT uMsg;
	NTI nObject;
	void* pCallbackThunk;
	char* pCallbackFunction;
	struct _MSGCALLBACK *next;
} MSGCALLBACK, *LPMSGCALLBACK;

typedef struct _WINDOWSUBCLASS {
	HWND hHwnd;
	WNDPROC pDefaultWndProc;
	void* pWindowThunk;
	void* pHookWndRetCall;
	void* pHookWndRetEax;
	LPMSGCALLBACK pBoundMessages;
	bool bClassProc;
	char aCallbackBuffer[VFP2C_MAX_CALLBACK_BUFFER];
	struct _WINDOWSUBCLASS *next;
} WINDOWSUBCLASS, *LPWINDOWSUBCLASS;

typedef struct _CALLBACKFUNC {
	char aCallbackBuffer[VFP2C_MAX_CALLBACK_BUFFER];
	NTI nObject;
	void *pFuncAddress;
	struct _CALLBACKFUNC *next;
} CALLBACKFUNC, *LPCALLBACKFUNC;

#ifdef __cplusplus
extern "C" {
#endif

int _stdcall VFP2C_Init_Callback();
void _stdcall VFP2C_Destroy_Callback(VFP2CTls& tls);

void _fastcall CreateCallbackFunc(ParamBlk *parm);
void _fastcall DestroyCallbackFunc(ParamBlk *parm);

void _fastcall BindEventsEx(ParamBlk *parm);
void _fastcall UnbindEventsEx(ParamBlk *parm);

#ifdef __cplusplus
} // extern C
#endif

#pragma warning(disable : 4290) // disable warning 4290 - VC++ doesn't implement throw ...

void _stdcall SubclassWindow(LPWINDOWSUBCLASS lpSubclass) throw(int);
void _stdcall UnsubclassWindow(LPWINDOWSUBCLASS lpSubclass);
void _stdcall UnsubclassWindowEx(LPWINDOWSUBCLASS lpSubclass) throw(int);
void _stdcall UnsubclassWindowExCallback(HWND hHwnd, LPARAM lParam) throw(int);
void _stdcall UnsubclassWindowExCallbackChild(HWND hHwnd, LPARAM lParam) throw(int);

void _stdcall CreateSubclassThunkProc(LPWINDOWSUBCLASS lpSubclass);
void _stdcall CreateSubclassMsgThunkProc(LPWINDOWSUBCLASS lpSubclass, LPMSGCALLBACK lpMsg, char *pCallback,
										char *pParmDef, DWORD nFlags, BOOL bObjectCall);

LPWINDOWSUBCLASS _stdcall NewWindowSubclass(HWND hHwnd, bool bClassProc) throw(int);
void _stdcall FreeWindowSubclass(LPWINDOWSUBCLASS lpSubclass);
void _stdcall RemoveWindowSubclass(LPWINDOWSUBCLASS lpSubclass);
LPWINDOWSUBCLASS _stdcall FindWindowSubclass(HWND hHwnd, bool bClassProc);
void _stdcall ReleaseWindowSubclasses();

LPMSGCALLBACK _stdcall NewMsgCallback(UINT uMsg);
void _stdcall FreeMsgCallback(LPWINDOWSUBCLASS pSubclass, LPMSGCALLBACK lpMsg);
LPMSGCALLBACK AddMsgCallback(LPWINDOWSUBCLASS pSubclass, UINT uMsg);
BOOL _stdcall RemoveMsgCallback(LPWINDOWSUBCLASS pSubclass, UINT uMsg);
void* _stdcall FindMsgCallbackThunk(LPWINDOWSUBCLASS pSubclass, UINT uMsg);

LPCALLBACKFUNC _stdcall NewCallbackFunc() throw(int);
bool _stdcall DeleteCallbackFunc(void *pFuncAddress);
void _stdcall ReleaseCallbackFuncs();
LRESULT _stdcall AsyncCallbackWindowProc(HWND nHwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void* _stdcall AllocThunk(int nSize) throw(int);
BOOL _stdcall FreeThunk(void *lpAddress);

#endif // _VFP2CCALLBACK_H__

#endif // _THREADSAFE