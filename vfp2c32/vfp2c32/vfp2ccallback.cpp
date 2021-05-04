#ifndef _THREADSAFE

#include <windows.h>
#include <stddef.h>
#include <malloc.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2ccallback.h"
#include "vfp2cassembly.h"
#include "vfp2chelpers.h"
#include "vfp2ccppapi.h"
#include "vfpmacros.h"

#define BINDEVENTSEX_OBJECT_SCHEME "__VFP2C_WCBO%U_%U_%U"
#define CALLBACKFUNC_OBJECT_SCHEME "__VFP2C_CBO_%U"

static HANDLE ghThunkHeap = 0;
static LPWINDOWSUBCLASS gpWMSubclasses = 0;
static LPCALLBACKFUNC gpCallbackFuncs = 0;

static HWND ghCallbackHwnd = 0;
static ATOM gnCallbackAtom = 0;

int _stdcall VFP2C_Init_Callback()
{
	const char* CALLBACK_WINDOW_CLASS = "__VFP2C_CBWC";

#ifndef HEAP_CREATE_ENABLE_EXECUTE
	#define HEAP_CREATE_ENABLE_EXECUTE      0x00040000  
#endif

	if (ghThunkHeap == 0)
	{
		if ((ghThunkHeap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, MAX_USHORT, 0)) == 0)
		{
			SaveWin32Error("HeapCreate", GetLastError());
			return E_APIERROR;
		}
	}

	if (gnCallbackAtom == 0)
	{
		WNDCLASSEX wndClass = {0}; /* MO - message only */
		wndClass.cbSize = sizeof(WNDCLASSEX);
		gnCallbackAtom = (ATOM)GetClassInfoEx(ghModule, CALLBACK_WINDOW_CLASS, &wndClass);
		if (!gnCallbackAtom)
		{
			wndClass.cbSize = sizeof(WNDCLASSEX);
			wndClass.hInstance = ghModule;
			wndClass.lpfnWndProc = AsyncCallbackWindowProc;
			wndClass.lpszClassName = CALLBACK_WINDOW_CLASS;
			gnCallbackAtom = RegisterClassEx(&wndClass);
			if (gnCallbackAtom == 0)
			{
				SaveWin32Error("RegisterClassEx", GetLastError());
				return E_APIERROR;
			}
		}
	}

	if (ghCallbackHwnd == 0)
	{
		// message only windows are only available on Win2000 or WinXP
		if (COs::GreaterOrEqual(Windows2000))
			ghCallbackHwnd = CreateWindowEx(0,(LPCSTR)gnCallbackAtom,0,0,0,0,0,0,HWND_MESSAGE,0,ghModule,0);
		else
			ghCallbackHwnd = CreateWindowEx(0,(LPCSTR)gnCallbackAtom,0,WS_POPUP,0,0,0,0,0,0,ghModule,0);

		if (ghCallbackHwnd == 0)
		{
			SaveWin32Error("CreateWindowEx", GetLastError());
			return E_APIERROR;
		}
	}

	return 0;
}

void _stdcall VFP2C_Destroy_Callback(VFP2CTls& tls)
{
	ReleaseCallbackFuncs();
	ReleaseWindowSubclasses();

	if (ghThunkHeap)
		HeapDestroy(ghThunkHeap);

	/* destroy window  */
	if (ghCallbackHwnd)
		DestroyWindow(ghCallbackHwnd);
	/* unregister windowclass */
	if (gnCallbackAtom)
		UnregisterClass((LPCSTR)gnCallbackAtom,ghModule);
}

/* 	BINDEVENT(_VFP.hWnd, WM_USER_SHNOTIFY,this,"HandleMsg") */
void _fastcall BindEventsEx(ParamBlk *parm)
{
	LPWINDOWSUBCLASS lpSubclass = 0;
	LPMSGCALLBACK lpMsg = 0;
	UINT uMsg = vp2.ev_long;
try
{
	int nErrorNo = VFP2C_Init_Callback();
	if (nErrorNo)
		throw nErrorNo;

	HWND hHwnd = reinterpret_cast<HWND>(vp1.ev_long);
	FoxString pCallback(vp4);
	FoxString pParameters(parm,5);

	DWORD nFlags;
	bool bClassProc;
	char aObjectName[VFP2C_MAX_FUNCTIONBUFFER];

	if (uMsg < WM_NULL)
		throw E_INVALIDPARAMS;

	if (pCallback.Len() > VFP2C_MAX_CALLBACK_FUNCTION)
		throw E_INVALIDPARAMS;

	if (Vartype(vp3) != 'O' && Vartype(vp3) != '0')
		throw E_INVALIDPARAMS;

	if (PCount() >= 5 && Vartype(vp5) != 'C' && Vartype(vp5) != '0')
		throw E_INVALIDPARAMS;

	if (PCount() >= 6 && vp6.ev_long)
	{
		nFlags = vp6.ev_long;
		if (!(nFlags & (BINDEVENTSEX_CALL_BEFORE | BINDEVENTSEX_CALL_AFTER | BINDEVENTSEX_RETURN_VALUE)))
			nFlags |= BINDEVENTSEX_CALL_BEFORE;
		// check nFlags for invalid combinations
		if (nFlags & BINDEVENTSEX_CALL_BEFORE && nFlags & (BINDEVENTSEX_CALL_AFTER | BINDEVENTSEX_RETURN_VALUE))
			throw E_INVALIDPARAMS;
		else if (nFlags & BINDEVENTSEX_CALL_AFTER && nFlags & (BINDEVENTSEX_CALL_BEFORE | BINDEVENTSEX_RETURN_VALUE))
			throw E_INVALIDPARAMS;
		else if (nFlags & BINDEVENTSEX_RETURN_VALUE && nFlags & (BINDEVENTSEX_CALL_AFTER | BINDEVENTSEX_CALL_BEFORE))
			throw E_INVALIDPARAMS;
	}
	else
		nFlags = BINDEVENTSEX_CALL_BEFORE;

	bClassProc = (nFlags & BINDEVENTSEX_CLASSPROC) > 0;

	// creates either a new struct or returns an existing struct for the passed hWnd
	lpSubclass = NewWindowSubclass(hHwnd,bClassProc);
	SubclassWindow(lpSubclass);

	// get existing struct for uMsg or create a new one
	lpMsg = AddMsgCallback(lpSubclass,uMsg);

	CreateSubclassMsgThunkProc(lpSubclass,lpMsg,pCallback,pParameters,nFlags,Vartype(vp3) == 'O');
	
	// if callback on object, store object reference to public variable
	if (Vartype(vp3) == 'O')
	{
		sprintfex(aObjectName,BINDEVENTSEX_OBJECT_SCHEME,bClassProc,hHwnd,uMsg);
        StoreObjectRef(aObjectName,lpMsg->nObject,vp3);
	}
	Return(reinterpret_cast<void*>(lpSubclass->pDefaultWndProc));
}
catch(int nErrorNo)
{
	if (lpMsg)
		RemoveMsgCallback(lpSubclass,uMsg);
	if (lpSubclass && !lpSubclass->pBoundMessages)
	{
		UnsubclassWindow(lpSubclass);
		RemoveWindowSubclass(lpSubclass);
	}
	RaiseError(nErrorNo);
}
}

void _fastcall UnbindEventsEx(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Callback();
	if (nErrorNo)
		throw nErrorNo;

	HWND hHwnd = reinterpret_cast<HWND>(vp1.ev_long);
	UINT uMsg = static_cast<UINT>(vp2.ev_long);
	LPWINDOWSUBCLASS lpSubclass;
	LPMSGCALLBACK lpMsg = 0;
	bool bClassProc = PCount() == 3 && vp3.ev_length;

	// get reference to struct for the hWnd, if none is found - the window was not subclassed
	lpSubclass = FindWindowSubclass(hHwnd, bClassProc);
	if (!lpSubclass)
	{
		SaveCustomError("UnbindEventsEx", "There are no message bindings for window %I", hHwnd);
		throw E_APIERROR;
	}

	// remove a message hook
	if (PCount() >= 2 && uMsg)
	{
		if (!RemoveMsgCallback(lpSubclass,uMsg))
		{
			SaveCustomError("UnbindEventsEx", "There is no message binding for message no. %I", uMsg);
			throw E_APIERROR;
		}
		// if no more hooks are present, unsubclass window
		if (!lpSubclass->pBoundMessages)
		{
			UnsubclassWindow(lpSubclass);
			RemoveWindowSubclass(lpSubclass);
		}
	}
	else // unsubclass window and free all hooks
	{
		UnsubclassWindow(lpSubclass);
		RemoveWindowSubclass(lpSubclass);
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

LPWINDOWSUBCLASS _stdcall NewWindowSubclass(HWND hHwnd, bool bClassProc) throw(int)
{
	LPWINDOWSUBCLASS lpSubclass = FindWindowSubclass(hHwnd,bClassProc);
	if (lpSubclass)
		return lpSubclass;

	lpSubclass = (LPWINDOWSUBCLASS)malloc(sizeof(WINDOWSUBCLASS));
	if (lpSubclass)
	{
		ZeroMemory(lpSubclass,sizeof(WINDOWSUBCLASS));
		lpSubclass->hHwnd = hHwnd;
		lpSubclass->bClassProc = bClassProc;
		lpSubclass->next = gpWMSubclasses;
		gpWMSubclasses = lpSubclass;
	}
	else
		throw E_INSUFMEMORY;

	return lpSubclass;
}

void _stdcall FreeWindowSubclass(LPWINDOWSUBCLASS lpSubclass)
{
	LPMSGCALLBACK lpMsg = lpSubclass->pBoundMessages, lpNext;
	while (lpMsg)
	{
		lpNext = lpMsg->next;
		FreeMsgCallback(lpSubclass,lpMsg);
		lpMsg = lpNext;
	}

	if (lpSubclass->pWindowThunk)
		FreeThunk(lpSubclass->pWindowThunk);
	free(lpSubclass);
}

void _stdcall RemoveWindowSubclass(LPWINDOWSUBCLASS lpSubclass)
{
	LPWINDOWSUBCLASS lpClass = gpWMSubclasses, lpNext;
	LPWINDOWSUBCLASS *lpPrev = &gpWMSubclasses;

	while (lpClass)
	{
		lpNext = lpClass->next;
		if (lpClass == lpSubclass)
		{
			FreeWindowSubclass(lpClass);
			*lpPrev = lpNext;
			return;
		}
		else
		{
			lpPrev = &lpClass->next;
			lpClass = lpNext;
		}
	}
}

LPWINDOWSUBCLASS _stdcall FindWindowSubclass(HWND hHwnd, bool bClassProc)
{
	LPWINDOWSUBCLASS lpSubclass = gpWMSubclasses;
	while (lpSubclass)
	{
		if (lpSubclass->hHwnd == hHwnd && lpSubclass->bClassProc == bClassProc)
			break;
		lpSubclass = lpSubclass->next;
	}
	return lpSubclass;
}

LPMSGCALLBACK NewMsgCallback(UINT uMsg)
{
	LPMSGCALLBACK lpMsg;

	lpMsg = (LPMSGCALLBACK)malloc(sizeof(MSGCALLBACK));
	if (!lpMsg)
		throw E_INSUFMEMORY;

	lpMsg->uMsg = uMsg;
	lpMsg->nObject = 0;
	lpMsg->next = 0;
	lpMsg->pCallbackThunk = 0;

	lpMsg->pCallbackFunction = (char*)malloc(VFP2C_MAX_CALLBACKBUFFER);
	if (!lpMsg->pCallbackFunction)
	{
		free(lpMsg);
		throw E_INSUFMEMORY;
	}
	return lpMsg;
}

void _stdcall FreeMsgCallback(LPWINDOWSUBCLASS pSubclass, LPMSGCALLBACK lpMsg)
{
	char aObjectName[VFP2C_MAX_FUNCTIONBUFFER];

	if (lpMsg->pCallbackThunk)
		FreeThunk(lpMsg->pCallbackThunk);
	if (lpMsg->pCallbackFunction)
		free(lpMsg->pCallbackFunction);

	sprintfex(aObjectName,BINDEVENTSEX_OBJECT_SCHEME,pSubclass->bClassProc,pSubclass->hHwnd,lpMsg->uMsg);
	ReleaseObjectRef(aObjectName,lpMsg->nObject);
	free(lpMsg);
}

LPMSGCALLBACK AddMsgCallback(LPWINDOWSUBCLASS pSubclass, UINT uMsg)
{
	LPMSGCALLBACK lpMsg = pSubclass->pBoundMessages, lpNewMsg;

	if (!lpMsg)
	{
		lpNewMsg = NewMsgCallback(uMsg);
		pSubclass->pBoundMessages = lpNewMsg;
		return lpNewMsg;
	}

	while (1)
	{
		if (lpMsg->uMsg == uMsg)
			return lpMsg;
		else if (lpMsg->next)
			lpMsg = lpMsg->next;
		else
		{
			lpNewMsg = NewMsgCallback(uMsg);
			lpMsg->next = lpNewMsg;
			return lpNewMsg;
		}
	}
}

BOOL _stdcall RemoveMsgCallback(LPWINDOWSUBCLASS pSubclass, UINT uMsg)
{
	LPMSGCALLBACK lpMsg = pSubclass->pBoundMessages, lpNext;
	LPMSGCALLBACK *lpPrev = &pSubclass->pBoundMessages;

	while (lpMsg)
	{
		lpNext = lpMsg->next;
		if (lpMsg->uMsg == uMsg)
		{
			FreeMsgCallback(pSubclass,lpMsg);
			*lpPrev = lpNext;
			return TRUE;
		}
		else
		{
			lpPrev = &lpMsg->next;
			lpMsg = lpNext;
		}
	}
	return FALSE;
}

void* _stdcall FindMsgCallbackThunk(LPWINDOWSUBCLASS pSubclass, UINT uMsg)
{
	LPMSGCALLBACK lpMsg = pSubclass->pBoundMessages;
	while (lpMsg)
	{
		if (lpMsg->uMsg == uMsg)
			return lpMsg->pCallbackThunk;
		lpMsg = lpMsg->next;
	}
	return 0;
}

void _stdcall SubclassWindow(LPWINDOWSUBCLASS lpSubclass) throw(int)
{
	// window/class already subclassed?
	if (lpSubclass->pDefaultWndProc)
		return;

	if (!lpSubclass->bClassProc)
	{
		lpSubclass->pDefaultWndProc = (WNDPROC)GetWindowLongPtr(lpSubclass->hHwnd,GWLP_WNDPROC);
		if (!lpSubclass->pDefaultWndProc)
		{
			SaveWin32Error("GetWindowLongPtr", GetLastError());
			throw E_APIERROR;
		}
		
		CreateSubclassThunkProc(lpSubclass);

		if (!SetWindowLongPtr(lpSubclass->hHwnd,GWLP_WNDPROC,(LONG)lpSubclass->pWindowThunk))
		{
			SaveWin32Error("SetWindowLongPtr", GetLastError());
			throw E_APIERROR;
		}
	}
	else
	{
		lpSubclass->pDefaultWndProc = (WNDPROC)GetClassLongPtr(lpSubclass->hHwnd,GCLP_WNDPROC);
		if (!lpSubclass->pDefaultWndProc)
		{
			SaveWin32Error("GetClassLongPtr", GetLastError());
			throw E_APIERROR;
		}
		
		CreateSubclassThunkProc(lpSubclass);

		if (!SetClassLongPtr(lpSubclass->hHwnd,GCLP_WNDPROC,(LONG)lpSubclass->pWindowThunk))
		{
			SaveWin32Error("SetClassLongPtr", GetLastError());
			throw E_APIERROR;
		}
	}
}

void _stdcall UnsubclassWindow(LPWINDOWSUBCLASS lpSubclass)
{
	if (!lpSubclass->bClassProc)
	{
		if (!SetWindowLongPtr(lpSubclass->hHwnd,GWLP_WNDPROC,(LONG)lpSubclass->pDefaultWndProc))
		{
			SaveWin32Error("SetWindowLongPtr", GetLastError());
			throw E_APIERROR;
		}
	}
	else
	{
		if (!SetClassLongPtr(lpSubclass->hHwnd,GCLP_WNDPROC,(LONG)lpSubclass->pDefaultWndProc))
		{
			SaveWin32Error("SetClassLongPtr", GetLastError());
			throw E_APIERROR;
		}
		UnsubclassWindowEx(lpSubclass);
	}
}

void _stdcall UnsubclassWindowEx(LPWINDOWSUBCLASS lpSubclass)
{
	EnumThreadWindows(GetCurrentThreadId(),(WNDENUMPROC)UnsubclassWindowExCallback,(LPARAM)lpSubclass);
}

void _stdcall UnsubclassWindowExCallback(HWND hHwnd, LPARAM lParam)
{
	LPWINDOWSUBCLASS lpSubclass = (LPWINDOWSUBCLASS)lParam;
	void* nProc;
	
	nProc = (void*)GetWindowLongPtr(hHwnd,GWLP_WNDPROC);
	if (nProc == lpSubclass->pWindowThunk)
	{
		if (!SetWindowLongPtr(hHwnd,GWLP_WNDPROC,(LONG)lpSubclass->pDefaultWndProc))
		{
			SaveWin32Error("SetWindowLongPtr", GetLastError());
			throw E_APIERROR;
		}
	}

	if (!EnumChildWindows(hHwnd,(WNDENUMPROC)UnsubclassWindowExCallbackChild,(LPARAM)lpSubclass))
	{
		if (GetLastError() != NO_ERROR)
		{
			SaveWin32Error("EnumChildWindows", GetLastError());
			throw E_APIERROR;
		}
	}
}

void _stdcall UnsubclassWindowExCallbackChild(HWND hHwnd, LPARAM lParam)
{
	LPWINDOWSUBCLASS lpSubclass = (LPWINDOWSUBCLASS)lParam;
	void* nProc;
	
	nProc = (void*)GetWindowLongPtr(hHwnd,GWLP_WNDPROC);
	if (nProc == lpSubclass->pWindowThunk)
	{
		if (!SetWindowLongPtr(hHwnd,GWLP_WNDPROC,(LONG)lpSubclass->pDefaultWndProc))
		{
			SaveWin32Error("SetWindowLongPtr", GetLastError());
			throw E_APIERROR;
		}
	}
}

void _stdcall CreateSubclassThunkProc(LPWINDOWSUBCLASS lpSubclass)
{
	RuntimeAssembler rasm;

	rasm.Parameter("hWnd", T_INT);
	rasm.Parameter("uMsg", T_UINT);
    rasm.Parameter("wParam", T_UINT);
    rasm.Parameter("lParam", T_INT);

	rasm.LocalVar("vRetVal", sizeof(Value), __alignof(Value));
	rasm.LocalVar("vAutoYield", sizeof(Value), __alignof(Value));

	// Function Prolog
	rasm.Prolog();
	// save common registers
	rasm.Push(EBX);
	rasm.Push(ECX);
	rasm.Push(EDX);

	rasm.Push("uMsg");
	rasm.Push((AVALUE)lpSubclass);
	rasm.Call((FUNCPTR)FindMsgCallbackThunk);

	rasm.Cmp(EAX,0); // msg was subclassed?
	rasm.Je("CallWindowProc");
	
	rasm.Jmp(EAX); // jump to thunk
	
	rasm.Label("CallWindowProc");
	//  return CallWindowProc(lpSubclass->pDefaultWndProc,hHwnd,uMsg,wParam,lParam);
	rasm.Push("lParam");
	rasm.Push("wParam");
	rasm.Push("uMsg");
	rasm.Push("hWnd");
	rasm.Push((AVALUE)lpSubclass->pDefaultWndProc);
	rasm.Call((FUNCPTR)CallWindowProc);

	rasm.Label("End");

	// restore registers
	rasm.Pop(EDX);
	rasm.Pop(ECX);
	rasm.Pop(EBX);
	// Function Epilog
	rasm.Epilog();
	
	// backpatch jump instructions
	rasm.Patch();
	
	lpSubclass->pWindowThunk = AllocThunk(rasm.CodeSize());
	rasm.WriteCode(lpSubclass->pWindowThunk);

	if (FlushInstructionCache(GetCurrentProcess(), lpSubclass->pWindowThunk, rasm.CodeSize()))
	{
		SaveWin32Error("FlushInstructionCache", GetLastError());
		throw E_APIERROR;
	}

	lpSubclass->pHookWndRetCall = rasm.LabelAddress("CallWindowProc");
	lpSubclass->pHookWndRetEax = rasm.LabelAddress("End");
}

void _stdcall CreateSubclassMsgThunkProc(LPWINDOWSUBCLASS lpSubclass, LPMSGCALLBACK lpMsg, char *pCallback, char *pParmDef, DWORD nFlags, BOOL bObjectCall)
{
	RuntimeAssembler rasm;

	int nParmCount = 6, xj;
	char aParmValue[VFP2C_MAX_TYPE_LEN];
	char aConvertFlags[VFP2C_MAX_TYPE_LEN] = {0};
	char *pConvertFlags = aConvertFlags;
	char *pCallbackTmp;
	REGISTER nReg = EAX;

	rasm.Parameter("hWnd", T_INT);
	rasm.Parameter("uMsg", T_UINT);
    rasm.Parameter("wParam", T_UINT);
	rasm.Parameter("lParam", T_UINT);

	rasm.LocalVar("vRetVal", sizeof(Value), __alignof(Value));
	rasm.LocalVar("vAutoYield", sizeof(Value), __alignof(Value));

	if (bObjectCall)
	{
		sprintfex(lpMsg->pCallbackFunction, BINDEVENTSEX_OBJECT_SCHEME, lpSubclass->bClassProc, lpSubclass->hHwnd, lpMsg->uMsg);
		strcat(lpMsg->pCallbackFunction, ".");
	}
	else
		*lpMsg->pCallbackFunction = '\0';

	strcat(lpMsg->pCallbackFunction,pCallback);

	if (nFlags & BINDEVENTSEX_CALL_AFTER)
	{
		rasm.Push("lParam");
		rasm.Push("wParam");
		rasm.Push("uMsg");
		rasm.Push("hWnd");
		rasm.Push((AVALUE)lpSubclass->pDefaultWndProc);
		rasm.Call((FUNCPTR)CallWindowProc);	
	}

	if (nFlags & BINDEVENTSEX_NO_RECURSION)
	{
		// _Evaluate(&vAutoYield,"_VFP.AutoYield");
		rasm.Lea(ECX,"vAutoYield",0);
		rasm.Mov(EDX,(AVALUE)"_VFP.AutoYield");
		rasm.Call((FUNCPTR)_Evaluate);

		// if (vAutoYield.ev_length)
		//  _Execute("_VFP.AutoYield = .F.")
		rasm.Mov(EAX,"vAutoYield",T_UINT,offsetof(Value,ev_length));
		rasm.Cmp(EAX,0);
		rasm.Je("AutoYieldFalse");
		rasm.Mov(ECX,(AVALUE)"_VFP.AutoYield = .F.");
		rasm.Call((FUNCPTR)_Execute);
		rasm.Label("AutoYieldFalse");
	}

	if (!pParmDef)
	{
		strcat(lpMsg->pCallbackFunction, "(%U,%U,%I,%I)");
		rasm.Push("lParam");
		rasm.Push("wParam");
		rasm.Push("uMsg");
		rasm.Push("hWnd");
	}
	else
	{
		nParmCount = GetWordCount(pParmDef,',');
		
		if (nParmCount > VFP2C_MAX_TYPE_LEN)
			throw E_INVALIDPARAMS;

		for (xj = nParmCount; xj; xj--)
		{
			GetWordNumN(aParmValue,pParmDef,',',xj,VFP2C_MAX_TYPE_LEN);
           	Alltrim(aParmValue);

			if (StrIEqual("wParam",aParmValue))
			{
				*pConvertFlags++ = 'I';
				rasm.Push("wParam");
			}
			else if (StrIEqual("lParam",aParmValue))
			{
				*pConvertFlags++ = 'I';
				rasm.Push("lParam");
			}
			else if (StrIEqual("uMsg",aParmValue))
			{
				*pConvertFlags++ = 'U';
				rasm.Push("uMsg");
			}
			else if (StrIEqual("hWnd",aParmValue))
			{
				*pConvertFlags++ = 'U';
				rasm.Push("hWnd");
			}
			else if (StrIEqual("UNSIGNED(wParam)",aParmValue))
			{
				*pConvertFlags++ = 'U';
				rasm.Push("wParam");
			}
			else if (StrIEqual("UNSIGNED(lParam)",aParmValue))
			{
				*pConvertFlags++ = 'U';
				rasm.Push("lParam");
			}
			else if (StrIEqual("HIWORD(wParam)",aParmValue))
			{
				*pConvertFlags++ = 'i';
				rasm.Mov(nReg,"wParam");
				rasm.Shr(nReg,16);
				rasm.Push(nReg);
			}
			else if (StrIEqual("LOWORD(wParam)",aParmValue))
			{
				*pConvertFlags++ = 'i';
				rasm.Mov(nReg,"wParam");
				rasm.And(nReg,MAX_USHORT);
				rasm.Push(nReg);
			}
			else if (StrIEqual("HIWORD(lParam)",aParmValue))
			{
				*pConvertFlags++ = 'i';
				rasm.Mov(nReg,"lParam");
				rasm.Shr(nReg,16);
				rasm.Push(nReg);
			}
			else if (StrIEqual("LOWORD(lParam)",aParmValue))
			{
				*pConvertFlags++ = 'i';
				rasm.Mov(nReg,"lParam");
				rasm.And(nReg,MAX_USHORT);
				rasm.Push(nReg);
			}
			else if (StrIEqual("UNSIGNED(HIWORD(wParam))",aParmValue))
			{
				*pConvertFlags++ = 'u';
				rasm.Mov(nReg,"wParam");
				rasm.Shr(nReg,16);
				rasm.Push(nReg);
			}
			else if (StrIEqual("UNSIGNED(LOWORD(wParam))",aParmValue))
			{
				*pConvertFlags++ = 'u';
				rasm.Mov(nReg,"wParam");
				rasm.And(nReg,MAX_USHORT);
				rasm.Push(nReg);
			}
			else if (StrIEqual("UNSIGNED(HIWORD(lParam))",aParmValue))
			{
				*pConvertFlags++ = 'u';
				rasm.Mov(nReg,"lParam");
				rasm.Shr(nReg,16);
				rasm.Push(nReg);
			}
			else if (StrIEqual("UNSIGNED(LOWORD(lParam))",aParmValue))
			{
				*pConvertFlags++ = 'u';
				rasm.Mov(nReg,"lParam");
				rasm.And(nReg,MAX_USHORT);
				rasm.Push(nReg);
			}
			else if (StrIEqual("BOOL(wParam)",aParmValue))
			{
				*pConvertFlags++ = 'L';
				rasm.Push("wParam");
			}
			else if (StrIEqual("BOOL(lParam)",aParmValue))
			{
				*pConvertFlags++ = 'L';
				rasm.Push("lParam");
			}
			else
				throw E_INVALIDPARAMS;

			if (nReg == EAX)
				nReg = EBX;
			else if (nReg == EBX)
				nReg = ECX;
			else if (nReg == ECX)
				nReg = EDX;
			else
				nReg = EAX;
		}

		// build format string
		pCallbackTmp = strend(lpMsg->pCallbackFunction);
		*pCallbackTmp++ = '(';
		for (xj = nParmCount; xj; xj--)
		{
			*pCallbackTmp++ = '%';
			*pCallbackTmp++ = aConvertFlags[xj-1];
			if (xj > 1)
				*pCallbackTmp++ = ',';
		}
		*pCallbackTmp++ = ')';
		*pCallbackTmp = '\0';

		// two parameters are always passed to sprintfex ..
		nParmCount += 2;
	}

	// if any parameters should be passed we need to call sprintfex
	if (nParmCount > 2)
	{
		rasm.Push((AVALUE)lpMsg->pCallbackFunction);
		rasm.Push((AVALUE)lpSubclass->aCallbackBuffer);
		rasm.Call((FUNCPTR)sprintfex);
		rasm.Add(ESP,nParmCount*SIZEOF_INT);	// add esp, no of parameters * sizeof stack increment
	}

	if (nFlags & BINDEVENTSEX_CALL_BEFORE)
	{
		if (nParmCount > 2)
			rasm.Mov(ECX,(AVALUE)lpSubclass->aCallbackBuffer);
		else
			rasm.Mov(ECX,(AVALUE)lpMsg->pCallbackFunction);

		rasm.Call((FUNCPTR)_Execute);
		
		if (nFlags & BINDEVENTSEX_NO_RECURSION)
		{
			rasm.Mov(EAX,"vAutoYield",T_UINT,offsetof(Value,ev_length));
			rasm.Cmp(EAX,0);
			rasm.Je("AutoYieldBack");
			// set autoyield to .T. again 
			rasm.Mov(ECX,(AVALUE)"_VFP.AutoYield = .T.");
			rasm.Call((FUNCPTR)_Execute);
			rasm.Label("AutoYieldBack");
		}
		rasm.Jmp(EBX,(AVALUE)lpSubclass->pHookWndRetCall); // jump back
	}
	else if (nFlags & (BINDEVENTSEX_CALL_AFTER | BINDEVENTSEX_RETURN_VALUE))
	{
		rasm.Lea(ECX,"vRetVal");
		if (nParmCount > 2)
			rasm.Mov(EDX,(AVALUE)lpSubclass->aCallbackBuffer);
		else
			rasm.Mov(EDX,(AVALUE)lpMsg->pCallbackFunction);

		rasm.Call((FUNCPTR)_Evaluate);

		if (nFlags & BINDEVENTSEX_NO_RECURSION)
		{
			rasm.Mov(EAX,"vAutoYield",T_UINT,offsetof(Value,ev_length));
			// if autoyield was .F. before we don't need to set it
			rasm.Cmp(EAX,0);
			rasm.Je("AutoYieldBack");
			// set autoyield to .T. again 
			rasm.Mov(ECX,(AVALUE)"_VFP.AutoYield = .T.");
			rasm.Call((FUNCPTR)_Execute);
			rasm.Label("AutoYieldBack");
		}
		rasm.Mov(EAX,"vRetVal",T_INT,offsetof(Value,ev_long));
		rasm.Jmp(EBX,(AVALUE)lpSubclass->pHookWndRetEax); // jump back
	}

	if (lpMsg->pCallbackThunk)
	{
		FreeThunk(lpMsg->pCallbackThunk);
		lpMsg->pCallbackThunk = 0;
	}

	rasm.Patch();
	lpMsg->pCallbackThunk = AllocThunk(rasm.CodeSize());
	rasm.WriteCode(lpMsg->pCallbackThunk);
}

void _stdcall ReleaseWindowSubclasses()
{
	LPWINDOWSUBCLASS lpSubclass = gpWMSubclasses, lpNext;

	while (lpSubclass)
	{
		lpNext = lpSubclass->next;
		try
		{
			UnsubclassWindow(lpSubclass);
		}
		catch(int) {}
		try
		{
			FreeWindowSubclass(lpSubclass);
		}
		catch(int) {}
		lpSubclass = lpNext;
	}
}

LPCALLBACKFUNC NewCallbackFunc() throw(int)
{
	LPCALLBACKFUNC pFunc = (LPCALLBACKFUNC)malloc(sizeof(CALLBACKFUNC));
	if (pFunc)
	{
		ZeroMemory(pFunc,sizeof(CALLBACKFUNC));
		pFunc->next = gpCallbackFuncs;
		gpCallbackFuncs = pFunc;
		return pFunc;
	}
	else
		throw E_INSUFMEMORY;
}

bool DeleteCallbackFunc(void *pFuncAddress)
{
	LPCALLBACKFUNC pFunc = gpCallbackFuncs, pFuncPrev = 0;
    char aObjectName[VFP2C_MAX_FUNCTIONBUFFER];

	while (pFunc && pFunc->pFuncAddress != pFuncAddress)
	{
		pFuncPrev = pFunc;
		pFunc = pFunc->next;
	}

	if (pFunc)
	{
		if (pFuncPrev)
			pFuncPrev->next = pFunc->next;
		else
			gpCallbackFuncs = pFunc->next;

		sprintfex(aObjectName,CALLBACKFUNC_OBJECT_SCHEME,pFunc);
		ReleaseObjectRef(aObjectName,pFunc->nObject);
		
		FreeThunk(pFuncAddress);
		free(pFunc);
		return true;
	}
	return false;
}

void _stdcall ReleaseCallbackFuncs()
{
	LPCALLBACKFUNC pFunc = gpCallbackFuncs, pFuncNext;
    char aObjectName[VFP2C_MAX_FUNCTIONBUFFER];
  
	while (pFunc)
	{
		pFuncNext = pFunc->next;
		sprintfex(aObjectName,CALLBACKFUNC_OBJECT_SCHEME,pFunc);
		ReleaseObjectRef(aObjectName,pFunc->nObject);
		FreeThunk(pFunc->pFuncAddress);
		free(pFunc);
		pFunc = pFuncNext;
	}
}

void _fastcall CreateCallbackFunc(ParamBlk *parm)
{
	LPCALLBACKFUNC pFunc = 0;
try
{
	int nErrorNo = VFP2C_Init_Callback();
	if (nErrorNo)
		throw nErrorNo;

	FoxString pCallback(vp1);
	FoxString pRetVal(vp2);
	FoxString pParams(vp3);
	
	DWORD nSyncFlag = PCount() == 5 ? vp5.ev_long : CALLBACK_SYNCRONOUS;
	bool bCDeclCallConv = (nSyncFlag & CALLBACK_CDECL) > 0; // is CALLBACK_CDECL set?
	nSyncFlag &= ~CALLBACK_CDECL; // remove CALLBACK_CDECL from nSyncFlag
	nSyncFlag = nSyncFlag ? nSyncFlag : CALLBACK_SYNCRONOUS; // set nSyncFlag to default CALLBACK_SYNCRONOUS if it is 0

	RuntimeAssembler rasm;

	int nParmCount, nWordCount, nPrecision, nParmNo;
	char aParmType[VFP2C_MAX_TYPE_LEN];
	char aParmPrec[VFP2C_MAX_TYPE_LEN];
	char aObjectName[VFP2C_MAX_FUNCTIONBUFFER];

	if (pCallback.Len() > VFP2C_MAX_CALLBACK_FUNCTION)
		throw E_INVALIDPARAMS;

	if (nSyncFlag != CALLBACK_SYNCRONOUS && 
		nSyncFlag != CALLBACK_ASYNCRONOUS_POST && 
		nSyncFlag != CALLBACK_ASYNCRONOUS_SEND)
		throw E_INVALIDPARAMS;

	pFunc = NewCallbackFunc();

	if (PCount() >= 4)
	{
		if (Vartype(vp4) == 'O')
		{
			sprintfex(aObjectName,CALLBACKFUNC_OBJECT_SCHEME,pFunc);
			StoreObjectRef(aObjectName,pFunc->nObject,vp4);
			strcpy(pFunc->aCallbackBuffer, aObjectName);
			strcat(pFunc->aCallbackBuffer,".");
		}
		else if (Vartype(vp4) != '0')
			throw E_INVALIDPARAMS;
	}
 
	nParmCount = pParams.GetWordCount(',');
	if (nParmCount > VFP2C_MAX_CALLBACK_PARAMETERS)
		throw E_INVALIDPARAMS;

	
	// return value needed?
	if (pRetVal.Len() > 0 && !pRetVal.ICompare("void"))
		rasm.LocalVar("vRetVal",sizeof(Value),__alignof(Value));
	// local buffer variable needed
	if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST|CALLBACK_ASYNCRONOUS_SEND))
		rasm.LocalVar("pBuffer",T_UINT);

	rasm.Prolog();
	rasm.Push(EBX);

	if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST|CALLBACK_ASYNCRONOUS_SEND))
	{
		rasm.Push(VFP2C_MAX_CALLBACK_BUFFER);
		rasm.Call((FUNCPTR)malloc);
		rasm.Add(ESP,SIZEOF_INT);

		rasm.Cmp(EAX,0);
		rasm.Je("ErrorOut");
		rasm.Mov("pBuffer",EAX);
	}

	if (nParmCount)
	{
		// fill static part of buffer
		strcat(pFunc->aCallbackBuffer,pCallback);
		strcat(pFunc->aCallbackBuffer,"(");
		if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST|CALLBACK_ASYNCRONOUS_SEND))
		{
			/* memcpy(pBuffer,pFunc->aCallbackBuffer,strlen(pFunc->aCallbackBuffer)
			memcpy uses the cdecl calling convention, thus parameters are pushed from right to left and
			we have to adjust the stack pointer (ESP) after the call */
			rasm.Push(strlen(pFunc->aCallbackBuffer));
			rasm.Push((AVALUE)pFunc->aCallbackBuffer);
			rasm.Push("pBuffer");
			rasm.Call((FUNCPTR)memcpy);
			rasm.Add(ESP,3*sizeof(int));

			rasm.Mov(EAX,"pBuffer");
			rasm.Add(EAX,strlen(pFunc->aCallbackBuffer));
		}
		else
			rasm.Mov(EAX,(AVALUE)(pFunc->aCallbackBuffer+strlen(pFunc->aCallbackBuffer)));
	}
	else
	{
		strcat(pFunc->aCallbackBuffer,pCallback);
		strcat(pFunc->aCallbackBuffer,"()");
		if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST|CALLBACK_ASYNCRONOUS_SEND))
		{
			rasm.Push(strlen(pFunc->aCallbackBuffer)+1);
			rasm.Push((AVALUE)pFunc->aCallbackBuffer);
			rasm.Push("pBuffer");
			rasm.Call((FUNCPTR)memcpy);
			rasm.Add(ESP,3*sizeof(int));
		}
	}
	
	for (nParmNo = 1; nParmNo <= nParmCount; nParmNo++)
	{
		GetWordNumN(aParmType, pParams, ',', nParmNo, VFP2C_MAX_TYPE_LEN);
		Alltrim(aParmType);

		nWordCount = GetWordCount(aParmType, ' ');
		if (nWordCount == 2)
		{
			GetWordNumN(aParmPrec, aParmType, ' ', 2, VFP2C_MAX_TYPE_LEN);
			GetWordNumN(aParmType, aParmType, ' ', 1, VFP2C_MAX_TYPE_LEN);
		}
		else if (nWordCount > 2)
			throw E_INVALIDPARAMS;

		if (nParmNo > 1)
		{
			rasm.Mov(REAX,sizeof(char),',');
			rasm.Add(EAX,1);
		}

		if (StrIEqual(aParmType,"INTEGER") || StrIEqual(aParmType,"LONG"))
		{
			rasm.Parameter(T_INT);
			rasm.Push((PARAMNO)nParmNo);
			rasm.Push(EAX);
			rasm.Call(EBX,(FUNCPTR)IntToStr);
		}
		else if (StrIEqual(aParmType,"UINTEGER") || StrIEqual(aParmType,"ULONG"))
		{
			rasm.Parameter(T_UINT);
			rasm.Push((PARAMNO)nParmNo);
			rasm.Push(EAX);
			rasm.Call(EBX,(FUNCPTR)UIntToStr);
		}
		else if (StrIEqual(aParmType,"STRING"))
		{
			rasm.Parameter(T_UINT);
			if (nWordCount == 1)
			{
				rasm.Push((PARAMNO)nParmNo);
				rasm.Push(EAX);
				rasm.Call(EBX,(FUNCPTR)UIntToStr);
			}
			else
			{
				char *pFunc;
				if (StrIEqual(aParmPrec, "ANSI"))
					pFunc = "ReadCString";
				else if (StrIEqual(aParmPrec, "UNICODE"))
					pFunc = "ReadWString";
				else
					throw E_INVALIDPARAMS;
				
				// copy function name into buffer
				int nStringLen = strlen(pFunc);
				rasm.Push(nStringLen);
				rasm.Push((AVALUE)pFunc);
				rasm.Push(EAX);
				rasm.Call((FUNCPTR)memcpy);
				rasm.Add(ESP, 3 * sizeof(int));

				// adjust EAX (end of string)
				rasm.Add(EAX, nStringLen);
				// copy '(' to end of buffer
				rasm.Mov(REAX, sizeof(char), '(');
				rasm.Add(EAX,1);

				// add pointer parameter to buffer
				rasm.Push((PARAMNO)nParmNo);
				rasm.Push(EAX);
				rasm.Call(EBX,(FUNCPTR)UIntToStr);
				
				// copy ')' to end of buffer
				rasm.Mov(REAX, sizeof(char), ')');
				rasm.Add(EAX,1);
			}
		}
		else if (StrIEqual(aParmType,"SHORT"))
		{
			rasm.Parameter(T_SHORT);
			rasm.Push((PARAMNO)nParmNo);
			rasm.Push(EAX);
			rasm.Call(EBX,(FUNCPTR)IntToStr);
		}
		else if (StrIEqual(aParmType,"USHORT"))
		{
			rasm.Parameter(T_USHORT);
			rasm.Push((PARAMNO)nParmNo);
			rasm.Push(EAX);
			rasm.Call(EBX,(FUNCPTR)UIntToStr);
		}
		else if (StrIEqual(aParmType,"BOOL"))
		{
			rasm.Parameter(T_INT);
			rasm.Push((PARAMNO)nParmNo);
			rasm.Push(EAX);
			rasm.Call(EBX,(FUNCPTR)BoolToStr);
		}
		else if (StrIEqual(aParmType,"SINGLE"))
		{
			rasm.Parameter(T_FLOAT);

			if (nWordCount == 2)
			{
				nPrecision = atoi(aParmPrec);
				if (nPrecision < 0 || nPrecision > 6)
					throw E_INVALIDPARAMS;
			}
			else
				nPrecision = 6;

			rasm.Push(nPrecision);
			rasm.Push((PARAMNO)nParmNo);
			rasm.Push(EAX);
			rasm.Call(EBX,(FUNCPTR)FloatToStr);
		}
		else if (StrIEqual(aParmType,"DOUBLE"))
		{
			rasm.Parameter(T_DOUBLE);

			if (nWordCount == 2)
			{
				nPrecision = atoi(aParmPrec);
				if (nPrecision < 0 || nPrecision > 16)
					throw E_INVALIDPARAMS;
			}
			else
				nPrecision = 6;

			rasm.Push(nPrecision);
			rasm.Push((PARAMNO)nParmNo);			
			rasm.Push(EAX);
			rasm.Call(EBX,(FUNCPTR)DoubleToStr);
		}
		else if (StrIEqual(aParmType,"INT64"))
		{
			rasm.Parameter(T_INT64);
			if (nWordCount == 1)
			{
				rasm.Push((PARAMNO)nParmNo);			
				rasm.Push(EAX);
				rasm.Call(EBX,(FUNCPTR)Int64ToStr);
			}
			else
			{
				if (StrIEqual(aParmPrec, "BINARY"))
				{
					rasm.Push((PARAMNO)nParmNo);			
					rasm.Push(EAX);
					rasm.Call(EBX,(FUNCPTR)Int64ToVarbinary);
				}
				else if (StrIEqual(aParmPrec, "LITERAL"))
				{
					rasm.Mov(REAX,sizeof(char),'"');
					rasm.Add(EAX, 1);
					rasm.Push((PARAMNO)nParmNo);			
					rasm.Push(EAX);
					rasm.Call(EBX,(FUNCPTR)Int64ToStr);
					rasm.Mov(REAX,sizeof(char),'"');
					rasm.Add(EAX, 1);
				}
				else if (StrIEqual(aParmPrec, "CURRENCY"))
				{
					rasm.Push((PARAMNO)nParmNo);			
					rasm.Push(EAX);
					rasm.Call(EBX,(FUNCPTR)Int64ToCurrency);
				}
				else
					throw E_INVALIDPARAMS;
			}
		}
		else if (StrIEqual(aParmType,"UINT64"))
		{
			rasm.Parameter(T_UINT64);
			if (nWordCount == 1)
			{
				rasm.Push((PARAMNO)nParmNo);			
				rasm.Push(EAX);
				rasm.Call(EBX,(FUNCPTR)UInt64ToStr);
			}
			else
			{
				if (StrIEqual(aParmPrec, "BINARY"))
				{
					rasm.Push((PARAMNO)nParmNo);			
					rasm.Push(EAX);
					rasm.Call(EBX,(FUNCPTR)Int64ToVarbinary);
				}
				else if (StrIEqual(aParmPrec, "LITERAL"))
				{
					rasm.Mov(REAX,sizeof(char),'"');
					rasm.Add(EAX, 1);
					rasm.Push((PARAMNO)nParmNo);			
					rasm.Push(EAX);
					rasm.Call(EBX,(FUNCPTR)UInt64ToStr);
					rasm.Mov(REAX,sizeof(char),'"');
					rasm.Add(EAX, 1);
				}
				else if (StrIEqual(aParmPrec, "CURRENCY"))
				{
					rasm.Push((PARAMNO)nParmNo);			
					rasm.Push(EAX);
					rasm.Call(EBX,(FUNCPTR)Int64ToCurrency);
				}
				else
					throw E_INVALIDPARAMS;
			}
		}
		else
			throw E_INVALIDPARAMS;
	}

	// nullterminate
	if (nParmCount)
	{
		rasm.Mov(REAX,sizeof(char),')');
		rasm.Add(EAX,1);
		rasm.Mov(REAX,sizeof(char),'\0');
	}

	if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST|CALLBACK_ASYNCRONOUS_SEND))
	{
		/* PostMessage(ghCallbackHwnd,WM_ASYNCCALLBACK,pBuffer,0); */
		rasm.Push(0);
		rasm.Push("pBuffer");
		rasm.Push(WM_ASYNCCALLBACK);
		rasm.Push(ghCallbackHwnd);
		if (nSyncFlag & CALLBACK_ASYNCRONOUS_POST)
			rasm.Call((FUNCPTR)PostMessage);
		else
			rasm.Call((FUNCPTR)SendMessage);
	}
	else if (pRetVal.Len() == 0 || pRetVal.ICompare("void"))
	{
		/* EXECUTE(pFunc->aCallbackBuffer); */
		rasm.Mov(ECX,(AVALUE)pFunc->aCallbackBuffer);
		rasm.Call((FUNCPTR)_Execute);
	}
	else
	{
		/* EVALUATE(vRetVal,pFunc->aCallbackBuffer) */
		rasm.Lea(ECX,"vRetVal");
		rasm.Mov(EDX,(AVALUE)pFunc->aCallbackBuffer);
		rasm.Call((FUNCPTR)_Evaluate);

		// return value
		if (StrIEqual(pRetVal,"INTEGER") || StrIEqual(pRetVal,"LONG"))
			rasm.Mov(EAX,"vRetVal",T_INT,offsetof(Value,ev_long));
		else if (StrIEqual(pRetVal,"UINTEGER") || StrIEqual(pRetVal,"ULONG"))
		{
			rasm.Mov(AL,"vRetVal",T_CHAR,offsetof(Value,ev_type));
			rasm.Cmp(AL,'N');
			rasm.Je("DConv");
			rasm.Mov(EAX,"vRetVal",T_UINT,offsetof(Value,ev_long));
			rasm.Jmp("End");
			rasm.Label("DConv");
			rasm.Push("vRetVal",T_DOUBLE,offsetof(Value,ev_real));
			rasm.Call((FUNCPTR)DoubleToUInt);
			rasm.Label("End");
		}
		else if (StrIEqual(pRetVal,"SHORT"))
			rasm.Mov(AX,"vRetVal",T_INT,offsetof(Value,ev_long));
		else if (StrIEqual(pRetVal,"USHORT"))
			rasm.Mov(AX,"vRetVal",T_UINT,offsetof(Value,ev_long));
		else if (StrIEqual(pRetVal,"SINGLE") || StrIEqual(pRetVal,"DOUBLE"))
			rasm.Fld("vRetVal",T_DOUBLE,offsetof(Value,ev_real));
		else if (StrIEqual(pRetVal,"BOOL"))
			rasm.Mov(EAX,"vRetVal",T_UINT,offsetof(Value,ev_length));
		else if (StrIEqual(pRetVal,"INT64"))
		{
			rasm.Mov(AL,"vRetVal",T_CHAR,offsetof(Value,ev_type));
			rasm.Cmp(AL,'N');
			rasm.Je("DConv");
			rasm.Mov(EAX,"vRetVal",T_INT,offsetof(Value,ev_long));
			rasm.Cdq();
			rasm.Jmp("End");
			rasm.Label("DConv");
			rasm.Push("vRetVal",T_DOUBLE,offsetof(Value,ev_real));
			rasm.Call((FUNCPTR)DoubleToInt64);
			rasm.Label("End");
		}
		else if (StrIEqual(pRetVal,"UINT64"))
		{
			rasm.Mov(AL,"vRetVal",T_CHAR,offsetof(Value,ev_type));
			rasm.Cmp(AL,'N');
			rasm.Je("DConv");
			rasm.Mov(EAX,"vRetVal",T_UINT,offsetof(Value,ev_long));
			rasm.Xor(EDX,EDX);
			rasm.Jmp("End");
			rasm.Label("DConv");
			rasm.Push("vRetVal",T_DOUBLE,offsetof(Value,ev_real));
			rasm.Call((FUNCPTR)DoubleToUInt64);
			rasm.Label("End");
		}
		else
			throw E_INVALIDPARAMS;
	}

	rasm.Label("ErrorOut");
	rasm.Pop(EBX);
	rasm.Epilog(bCDeclCallConv);

	rasm.Patch();

	pFunc->pFuncAddress = AllocThunk(rasm.CodeSize());
	rasm.WriteCode(pFunc->pFuncAddress);

	Return(pFunc->pFuncAddress);
}
catch(int nErrorNo)
{
	if (pFunc)
		DeleteCallbackFunc(pFunc);
	RaiseError(nErrorNo);
}
}

void _fastcall DestroyCallbackFunc(ParamBlk *parm)
{
	Return(DeleteCallbackFunc(reinterpret_cast<void*>(vp1.ev_long)));
}

LRESULT _stdcall AsyncCallbackWindowProc(HWND nHwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int nErrorNo;
	if (uMsg == WM_ASYNCCALLBACK)
	{
		__try
		{
			char* pCommand = reinterpret_cast<char*>(wParam);
			if (pCommand)
			{
				if (_msize(pCommand) > 0)
				{
					nErrorNo = _Execute(pCommand);
					free(reinterpret_cast<void*>(wParam));
					if (nErrorNo)
						RaiseError(nErrorNo);
					return 0;
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
	}
	return DefWindowProc(nHwnd,uMsg,wParam,lParam);
}

void* _stdcall AllocThunk(int nSize) throw(int)
{
	void *pThunk;
	pThunk = HeapAlloc(ghThunkHeap,0,nSize);
	if (pThunk)
		return pThunk;
	else
		throw E_INSUFMEMORY;
}

BOOL _stdcall FreeThunk(void *lpAddress)
{
	return HeapFree(ghThunkHeap,0,lpAddress);
}

#endif // _THREADSAFE