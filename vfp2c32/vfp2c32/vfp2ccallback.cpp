#ifndef _THREADSAFE

#include <windows.h>
#include <stddef.h>
#include <malloc.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2cassembly.h"
#include "vfp2chelpers.h"
#include "vfp2ccppapi.h"
#include "vfp2ctls.h"
#include "vfp2ccallback.h"

#define BINDEVENTSEX_OBJECT_SCHEME "__VFP2C_WCBO%U_%U_%U"
#define CALLBACKFUNC_OBJECT_SCHEME "__VFP2C_CBO_%U"

static HWND ghCallbackHwnd = 0;
static ATOM gnCallbackAtom = 0;

int _stdcall VFP2C_Init_Callback()
{
	const char* CALLBACK_WINDOW_CLASS = "__VFP2C_CBWC";

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
	CallbackFunction::ReleaseCallbackFuncs();
	WindowSubclass::ReleaseWindowSubclasses();

	/* destroy window  */
	if (ghCallbackHwnd)
		DestroyWindow(ghCallbackHwnd);
	/* unregister windowclass */
	if (gnCallbackAtom)
		UnregisterClass((LPCSTR)gnCallbackAtom,ghModule);
}

void _fastcall BindEventsEx(ParamBlkEx& parm)
{
	WindowSubclass* lpSubclass = 0;
	WindowMessageCallback* lpMsg = 0;
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pObjectName;
	UINT uMsg = static_cast<UINT>(parm(2)->ev_long);
	try
	{
		int nErrorNo = VFP2C_Init_Callback();
		if (nErrorNo)
			throw nErrorNo;

		HWND hHwnd = parm(1)->Ptr<HWND>();
		FoxString pCallback(parm(4));
		FoxString pParameters(parm, 5);

		DWORD nFlags;
		bool bClassProc;

		if (pCallback.Len() > VFP2C_MAX_CALLBACK_FUNCTION || pCallback.Len() == 0)
		{
			SaveCustomError("BindEventsEx", "Callback function length is zero or greater than maximum length of 1024.");
			throw E_INVALIDPARAMS;
		}

		if (parm(3)->Vartype() != 'O' && parm(3)->Vartype() != '0')
		{
			SaveCustomError("BindEventsEx", "Invalid parameter type '%s' for parameter 3", parm(3)->Vartype());
			throw E_INVALIDPARAMS;
		}

		if (parm.PCount() >= 5 && parm(5)->Vartype() != 'C' && parm(5)->Vartype() != '0')
		{
			SaveCustomError("BindEventsEx", "Invalid parameter type '%s' for parameter 5", parm(5)->Vartype());
			throw E_INVALIDPARAMS;
		}
		if (parm.PCount() >= 6 && parm(6)->ev_long)
		{
			nFlags = parm(6)->ev_long;
			if (!(nFlags & (BINDEVENTSEX_CALL_BEFORE | BINDEVENTSEX_CALL_AFTER | BINDEVENTSEX_RETURN_VALUE)))
				nFlags |= BINDEVENTSEX_CALL_BEFORE;
			// check nFlags for invalid combinations
			if (((nFlags & BINDEVENTSEX_CALL_BEFORE) && (nFlags & (BINDEVENTSEX_CALL_AFTER | BINDEVENTSEX_RETURN_VALUE))) ||
				((nFlags & BINDEVENTSEX_CALL_AFTER) && (nFlags & (BINDEVENTSEX_CALL_BEFORE | BINDEVENTSEX_RETURN_VALUE))) ||
				((nFlags & BINDEVENTSEX_RETURN_VALUE) && (nFlags & (BINDEVENTSEX_CALL_AFTER | BINDEVENTSEX_CALL_BEFORE))))
			{
				SaveCustomError("BindEventsEx", "Invalid nFlags combination - BINDEVENTSEX_CALL_BEFORE, BINDEVENTSEX_CALL_AFTER and BINDEVENTSEX_RETURN_VALUE are mutually exclusive.");
				throw E_INVALIDPARAMS;
			}
		}
		else
			nFlags = BINDEVENTSEX_CALL_BEFORE;

		bClassProc = (nFlags & BINDEVENTSEX_CLASSPROC) > 0;

		lpSubclass = WindowSubclass::FindWindowSubclass(hHwnd, bClassProc);
		if (!lpSubclass)
		{
			lpSubclass = new WindowSubclass();
			if (!lpSubclass)
				throw E_INSUFMEMORY;
			lpSubclass->SubclassWindow(hHwnd, bClassProc);
		}

		// get existing struct for uMsg or create a new one
		lpMsg = lpSubclass->AddMessageCallback(uMsg);
		lpMsg->m_ReturnValue = (nFlags & (BINDEVENTSEX_CALL_AFTER | BINDEVENTSEX_RETURN_VALUE)) > 0;
		lpSubclass->CreateSubclassMsgThunkProc(lpMsg, pCallback, pParameters, nFlags, parm(3)->Vartype() == 'O');

		// if callback on object, store object reference to public variable
		if (parm(3)->Vartype() == 'O')
		{
			pObjectName.Format(BINDEVENTSEX_OBJECT_SCHEME, bClassProc, hHwnd, uMsg);
			parm(3)->StoreObjectRef(pObjectName, lpMsg->m_Object);
		}
		Return(reinterpret_cast<void*>(lpSubclass->m_DefaultWndProc));
	}
	catch (int nErrorNo)
	{
		if (lpSubclass)
		{
			if (lpMsg)
			{
				lpSubclass->RemoveMessageCallback(uMsg);
			}
			if (lpSubclass->m_BoundMessages.GetCount() == 0)
			{
				delete lpSubclass;
			}
		}
		RaiseError(nErrorNo);
	}
}

void _fastcall UnbindEventsEx(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Callback();
	if (nErrorNo)
		throw nErrorNo;

	HWND hHwnd = parm(1)->Ptr<HWND>();
	UINT uMsg = static_cast<UINT>(parm(2)->ev_long);
	WindowSubclass* lpSubclass = 0;
	WindowMessageCallback* lpMsg = 0;
	bool bClassProc = parm.PCount() == 3 && parm(3)->ev_length;

	// get reference to struct for the hWnd, if none is found - the window was not subclassed
	lpSubclass = WindowSubclass::FindWindowSubclass(hHwnd, bClassProc);

	if (!lpSubclass)
	{
		SaveCustomError("UnbindEventsEx", "There are no message bindings for window %I", hHwnd);
		throw E_APIERROR;
	}

	// remove a message hook
	if (parm.PCount() >= 2 && uMsg)
	{
		if (!lpSubclass->RemoveMessageCallback(uMsg))
		{
			SaveCustomError("UnbindEventsEx", "There is no message binding for message no. %I", uMsg);
			throw E_APIERROR;
		}
		// if no more hooks are present, unsubclass window
		if (lpSubclass->m_BoundMessages.GetCount() == 0)
		{
			delete lpSubclass;
		}
	}
	else // unsubclass window and free all hooks
	{
		delete lpSubclass;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

WindowMessageCallback::WindowMessageCallback(UINT msg)
{
	m_ReturnValue = false;
	m_Object = 0;
	m_CallbackThunk = 0;
	m_Msg = msg;
	m_CallbackFunction = (char*)malloc(VFP2C_MAX_CALLBACKBUFFER);
	if (!m_CallbackFunction)
	{
		throw E_INSUFMEMORY;
	}
}

void WindowMessageCallback::Release(WindowSubclass* pSubclass)
{
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pObjectName;

	if (m_CallbackFunction)
		free(m_CallbackFunction);

	if (m_CallbackThunk)
	{
		RuntimeAssembler::UnregisterUnwindInfo(m_CallbackThunk);
		RuntimeAssembler::FreeMemory(m_CallbackThunk);
	}

	if (m_Object != 0)
	{
		pObjectName.Format(BINDEVENTSEX_OBJECT_SCHEME, pSubclass->m_ClassProc, pSubclass->m_Hwnd, m_Msg);
		ValueEx::ReleaseObjectRef(pObjectName, m_Object);
	}
}

WindowSubclass::WindowSubclass()
{
	m_ClassProc = false;
	m_Hwnd = 0;
	m_DefaultWndProc = 0;
	m_WindowThunk = 0;
	VFP2CTls::Tls().WindowSubclasses.Add(this);
}

WindowSubclass::~WindowSubclass()
{
	int nCount = m_BoundMessages.GetCount();
	for (int xj = 0; xj < nCount; xj++)
	{
		m_BoundMessages[xj].Release(this);
	}
	if (m_DefaultWndProc)
	{
		UnsubclassWindow();
	}
	if (m_WindowThunk)
	{
		RuntimeAssembler::UnregisterUnwindInfo(m_WindowThunk);
		RuntimeAssembler::FreeMemory(m_WindowThunk);
	}
	VFP2CTls::Tls().WindowSubclasses.Remove(this);
}

WindowSubclass* WindowSubclass::FindWindowSubclass(HWND hHwnd, bool bClassProc)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	int nCount = tls.WindowSubclasses.GetCount();
	for (int xj = 0; xj < nCount; xj++)
	{
		if (tls.WindowSubclasses[xj]->m_Hwnd == hHwnd && tls.WindowSubclasses[xj]->m_ClassProc == bClassProc)
		{
			return tls.WindowSubclasses[xj];
		}
	}
	return 0;
}

WindowMessageCallback* WindowSubclass::AddMessageCallback(UINT uMsg)
{
	WindowMessageCallback pMsgCallback(uMsg);
	int nIndex = -1;
	for (int xj = 0; xj < m_BoundMessages.GetCount(); xj++)
	{
		if (m_BoundMessages[xj].m_Msg == uMsg)
		{
			nIndex = xj;
			break;
		}
	}
	if (nIndex == -1)
	{
		nIndex = m_BoundMessages.Add(pMsgCallback);
	}
	return &m_BoundMessages[nIndex];
}

bool WindowSubclass::RemoveMessageCallback(UINT uMsg)
{
	int nIndex = -1;
	for (int xj = 0; xj < m_BoundMessages.GetCount(); xj++)
	{
		if (m_BoundMessages[xj].m_Msg == uMsg)
		{
			nIndex = xj;
			break;
		}
	}
	if (nIndex == -1)
		return false;

	WindowMessageCallback pMsgCallback = m_BoundMessages[nIndex];
	m_BoundMessages.Remove(nIndex);
	pMsgCallback.Release(this);
	return true;
}

WindowMessageCallback* _stdcall WindowSubclass::FindMessageCallback(WindowSubclass* pSubclass, UINT uMsg)
{
/*	CStrBuilder<128> mString;
	mString.Format("%U,%U\n", (UINT_PTR)pSubclass, uMsg);
	OutputDebugString(mString);
*/
	int nCount = pSubclass->m_BoundMessages.GetCount();
	for (int xj = 0; xj < nCount; xj++)
	{
		if (pSubclass->m_BoundMessages[xj].m_Msg == uMsg)
			return &pSubclass->m_BoundMessages[xj];
	}
	return 0;
}

void WindowSubclass::SubclassWindow(HWND hHwnd, bool bClassProc)
{
	// window/class already subclassed?
	if (m_DefaultWndProc)
		return;

	m_Hwnd = hHwnd;
	if (!bClassProc)
	{
		m_DefaultWndProc = (WNDPROC)GetWindowLongPtr(hHwnd,GWLP_WNDPROC);
		if (!m_DefaultWndProc)
		{
			SaveWin32Error("GetWindowLongPtr", GetLastError());
			throw E_APIERROR;
		}
		
		CreateSubclassThunkProc();

		if (!SetWindowLongPtr(hHwnd,GWLP_WNDPROC,(LONG_PTR)m_WindowThunk))
		{
			SaveWin32Error("SetWindowLongPtr", GetLastError());
			throw E_APIERROR;
		}
	}
	else
	{
		m_DefaultWndProc = (WNDPROC)GetClassLongPtr(hHwnd,GCLP_WNDPROC);
		if (!m_DefaultWndProc)
		{
			SaveWin32Error("GetClassLongPtr", GetLastError());
			throw E_APIERROR;
		}
		
		CreateSubclassThunkProc();

		if (!SetClassLongPtr(hHwnd,GCLP_WNDPROC,(LONG_PTR)m_WindowThunk))
		{
			SaveWin32Error("SetClassLongPtr", GetLastError());
			throw E_APIERROR;
		}
	}
}

void WindowSubclass::UnsubclassWindow()
{
	if (!m_ClassProc)
	{
		if (!SetWindowLongPtr(m_Hwnd,GWLP_WNDPROC,(LONG_PTR)m_DefaultWndProc))
		{
			SaveWin32Error("SetWindowLongPtr", GetLastError());
			throw E_APIERROR;
		}
	}
	else
	{
		if (!SetClassLongPtr(m_Hwnd,GCLP_WNDPROC,(LONG_PTR)m_DefaultWndProc))
		{
			SaveWin32Error("SetClassLongPtr", GetLastError());
			throw E_APIERROR;
		}
		EnumThreadWindows(GetCurrentThreadId(), (WNDENUMPROC)UnsubclassWindowClassCallback, (LPARAM)this);
	}
}

void WindowSubclass::UnsubclassWindowClassCallback(HWND hHwnd, LPARAM lParam)
{
	WindowSubclass* lpSubclass = (WindowSubclass*)lParam;
	void* nProc;
	
	nProc = (void*)GetWindowLongPtr(hHwnd,GWLP_WNDPROC);
	if (nProc == lpSubclass->m_WindowThunk)
	{
		if (!SetWindowLongPtr(hHwnd,GWLP_WNDPROC,(LONG_PTR)lpSubclass->m_DefaultWndProc))
		{
			SaveWin32Error("SetWindowLongPtr", GetLastError());
			throw E_APIERROR;
		}
	}

	if (!EnumChildWindows(hHwnd,(WNDENUMPROC)WindowSubclass::UnsubclassWindowClassCallbackChild,(LPARAM)lpSubclass))
	{
		if (GetLastError() != NO_ERROR)
		{
			SaveWin32Error("EnumChildWindows", GetLastError());
			throw E_APIERROR;
		}
	}
}

void WindowSubclass::UnsubclassWindowClassCallbackChild(HWND hHwnd, LPARAM lParam)
{
	WindowSubclass* lpSubclass = (WindowSubclass*)lParam;
	void* nProc;
	
	nProc = (void*)GetWindowLongPtr(hHwnd,GWLP_WNDPROC);
	if (nProc == lpSubclass->m_WindowThunk)
	{
		if (!SetWindowLongPtr(hHwnd,GWLP_WNDPROC,(LONG_PTR)lpSubclass->m_DefaultWndProc))
		{
			SaveWin32Error("SetWindowLongPtr", GetLastError());
			throw E_APIERROR;
		}
	}
}

void WindowSubclass::CreateSubclassThunkProc()
{
	RuntimeAssembler rasm(stdcall, true);
	AsmRegister nReturnReg = rasm.ReturnRegister();

	AsmVariable& varHwnd = rasm.Parameter<HWND>();
	AsmVariable& varUmsg = rasm.Parameter<UINT>();
	AsmVariable& varWparam = rasm.Parameter<WPARAM>();
	AsmVariable& varLparam = rasm.Parameter<LPARAM>();
	AsmVariable& bReturn = rasm.LocalVar<bool>();
	const char* pLabelWndProc = "lWinProc";
	const char* pLabelEnd = "lEnd";
	// rasm.BreakPoint();
	rasm.CallConv(thiscall).Call(&WindowSubclass::FindMessageCallback, this, varUmsg);
	rasm.Cmp(nReturnReg,0);
	rasm.Je(pLabelWndProc);
	rasm.Mov(bReturn, AsmVariable::Var<bool>(nReturnReg, offsetof(WindowMessageCallback, m_ReturnValue)));
	rasm.Mov(nReturnReg, AsmVariable::Var<void*>(nReturnReg, offsetof(WindowMessageCallback, m_CallbackThunk)));
	rasm.CallConv(stdcall).Call(nReturnReg, varHwnd, varUmsg, varWparam, varLparam); // call thunk
	rasm.Mov(RCX, bReturn);
	rasm.Cmp(RCX, 0);
	rasm.Je(pLabelEnd);
	rasm.Label(pLabelWndProc);
	rasm.CallConv(stdcall).Call(CallWindowProc, m_DefaultWndProc, varHwnd, varUmsg, varWparam, varLparam);
	rasm.Label(pLabelEnd);
	rasm.Finish();
	m_WindowThunk = rasm.AllocateMemory(rasm.CodeSize());
	rasm.WriteCode(m_WindowThunk);
	rasm.RegisterUnwindInfo(m_WindowThunk);
}

void WindowSubclass::CreateSubclassMsgThunkProc(WindowMessageCallback* lpMsg, CStringView pCallback, CStringView pParameterList, DWORD nFlags, BOOL bObjectCall)
{
	RuntimeAssembler rasm(stdcall, true);
	AsmVariable& varHwnd = rasm.Parameter<HWND>();
	AsmVariable& varUmsg = rasm.Parameter<UINT>();
	AsmVariable& varWparam = rasm.Parameter<WPARAM>();
	AsmVariable& varLparam = rasm.Parameter<LPARAM>();
	AsmVariable varReturn;
	AsmVariable varAutoYield;
	if (nFlags & nFlags & (BINDEVENTSEX_CALL_AFTER | BINDEVENTSEX_RETURN_VALUE))
		varReturn = rasm.LocalVar<Value>();
	if (nFlags & BINDEVENTSEX_NO_RECURSION)
		varAutoYield = rasm.LocalVar<Value>();

	AsmRegister nReg = rasm.ReturnRegister();
	int nParmCount = 6, nParmNo = 0, xj;
	CStrBuilder<VFP2C_MAX_CALLBACK_PARAMETERS> pConvertFlags;

	if (bObjectCall)
	{
		sprintfex(lpMsg->m_CallbackFunction, BINDEVENTSEX_OBJECT_SCHEME, m_ClassProc, m_Hwnd, lpMsg->m_Msg);
#pragma warning(disable : 4996)
		strcat(lpMsg->m_CallbackFunction, ".");
#pragma warning(default : 4996)
	}
	else
		*lpMsg->m_CallbackFunction = '\0';

#pragma warning(disable : 4996)
	strcat(lpMsg->m_CallbackFunction,pCallback.Data);
#pragma warning(default : 4996)

	if (nFlags & BINDEVENTSEX_CALL_AFTER)
	{
		rasm.CallConv(stdcall).Call(CallWindowProc, m_DefaultWndProc, varHwnd, varUmsg, varWparam, varLparam);
	}

	if (nFlags & BINDEVENTSEX_NO_RECURSION)
	{
		// _Evaluate(&vAutoYield,"_VFP.AutoYield");
		rasm.CallConv(fastcall).Call(_Evaluate, &varAutoYield, "_VFP.AutoYield");

		// if (vAutoYield.ev_length)
		//  _Execute("_VFP.AutoYield = .F.")
		rasm.Mov(nReg, rasm.Var<unsigned int>(varAutoYield, offsetof(Value,ev_length)));
		rasm.Cmp(nReg,0);
		rasm.Je("AutoYieldFalse");
		rasm.CallConv(fastcall).Call(_Execute, "_VFP.AutoYield = .F.");
		rasm.Label("AutoYieldFalse");
	}

	if (!pParameterList)
	{
		rasm.StackMode(true);
#pragma warning(disable : 4996)
		strcat(lpMsg->m_CallbackFunction, "(%U,%U,%I,%I)");
#pragma warning(default : 4996)
		rasm.PassParameter(varHwnd, nParmNo);
		rasm.PassParameter(varUmsg, nParmNo);
		rasm.PassParameter(varWparam, nParmNo);
		rasm.PassParameter(varLparam, nParmNo);
		rasm.StackMode(false);
	}
	else
	{
		rasm.StackMode(true);
		rasm.PassParameter(m_CallbackBuffer, nParmNo);
		rasm.PassParameter(lpMsg->m_CallbackFunction, nParmNo);

		nParmCount = pParameterList.GetWordCount(',');
		
		if (nParmCount > VFP2C_MAX_CALLBACK_PARAMETERS)
		{
			SaveCustomError("BindEventsEx", "Parameter count greater than maximum of 27.");
			throw E_INVALIDPARAMS;
		}

		for (xj = 1; xj <= nParmCount; xj++)
		{
			CStringView pParamater = pParameterList.GetWordNum(xj, ',').Alltrim();

			if (pParamater.ICompare("wparam"))
			{
				pConvertFlags.Append('I');
				rasm.PassParameter(varWparam, nParmNo);
			}
			else if (pParamater.ICompare("lparam"))
			{
				pConvertFlags.Append('I');
				rasm.PassParameter(varLparam, nParmNo);
			}
			else if (pParamater.ICompare("umsg"))
			{
				pConvertFlags.Append('U');
				rasm.PassParameter(varUmsg, nParmNo);
			}
			else if (pParamater.ICompare("hwnd"))
			{
				pConvertFlags.Append('U');
				rasm.PassParameter(varHwnd, nParmNo);
			}
			else if (pParamater.ICompare("unsigned(wparam)"))
			{
				pConvertFlags.Append('U');
				rasm.PassParameter(varWparam, nParmNo);
			}
			else if (pParamater.ICompare("unsigned(lparam)"))
			{
				pConvertFlags.Append('U');
				rasm.PassParameter(varLparam, nParmNo);
			}
			else if (pParamater.ICompare("hiword(wparam)"))
			{
				pConvertFlags.Append('i');
				rasm.PassParameter(nReg, nParmNo);
				rasm.Shr(nReg, 16);
				rasm.Mov(nReg, varWparam);
			}
			else if (pParamater.ICompare("loword(wparam)"))
			{
				pConvertFlags.Append('i');
				rasm.PassParameter(nReg, nParmNo);
				rasm.And(nReg, MAX_USHORT);
				rasm.Mov(nReg, varWparam);
			}
			else if (pParamater.ICompare("hiword(lparam)"))
			{
				pConvertFlags.Append('i');
				rasm.PassParameter(nReg, nParmNo);
				rasm.Shr(nReg, 16);
				rasm.Mov(nReg, varLparam);
			}
			else if (pParamater.ICompare("loword(lparam)"))
			{
				pConvertFlags.Append('i');
				rasm.PassParameter(nReg, nParmNo);
				rasm.And(nReg, MAX_USHORT);
				rasm.Mov(nReg, varLparam);
			}
			else if (pParamater.ICompare("unsigned(hiword(wparam))"))
			{
				pConvertFlags.Append('u');
				rasm.PassParameter(nReg, nParmNo);
				rasm.Shr(nReg, 16);
				rasm.Mov(nReg, varWparam);
			}
			else if (pParamater.ICompare("unsigned(loword(wparam))"))
			{
				pConvertFlags.Append('u');
				rasm.PassParameter(nReg, nParmNo);
				rasm.And(nReg, MAX_USHORT);
				rasm.Mov(nReg, varWparam);
			}
			else if (pParamater.ICompare("unsigned(hiword(lparam))"))
			{
				pConvertFlags.Append('u');
				rasm.PassParameter(nReg, nParmNo);
				rasm.Shr(nReg, 16);
				rasm.Mov(nReg, varLparam);
			}
			else if (pParamater.ICompare("unsigned(loword(lparam))"))
			{
				pConvertFlags.Append('u');
				rasm.PassParameter(nReg, nParmNo);
				rasm.And(nReg, MAX_USHORT);
				rasm.Mov(nReg, varLparam);
			}
			else if (pParamater.ICompare("bool(wparam)"))
			{
				pConvertFlags.Append('L');
				rasm.PassParameter(varWparam, nParmNo);
			}
			else if (pParamater.ICompare("bool(lparam)"))
			{
				pConvertFlags.Append('L');
				rasm.PassParameter(varLparam, nParmNo);
			}
			else
			{
				SaveCustomError("BindEventsEx", "Invalid parameter type '%V'", &pParamater);
				throw E_INVALIDPARAMS;
			}
		}

		// build format string
		char* pCallbackTmp = strend(lpMsg->m_CallbackFunction);
		*pCallbackTmp++ = '(';
		for (xj = 1; xj <= nParmCount; xj++)
		{
			*pCallbackTmp++ = '%';
			*pCallbackTmp++ = pConvertFlags[xj-1];
			if (xj != nParmCount)
				*pCallbackTmp++ = ',';
		}
		*pCallbackTmp++ = ')';
		*pCallbackTmp = '\0';

		// two parameters are always passed to sprintfex ..
		nParmCount += 2;
		rasm.StackMode(false, nParmCount <= 2);
	}

	// if any parameters should be passed we need to call sprintfex
	if (nParmCount > 2)
	{
		rasm.CallConv(cdeclaration).ManualCall(sprintfex);
	}

	if (nFlags & BINDEVENTSEX_CALL_BEFORE)
	{
		rasm.CallConv(fastcall).Call(_Execute, nParmCount > 2 ? m_CallbackBuffer : lpMsg->m_CallbackFunction);
		
		if (nFlags & BINDEVENTSEX_NO_RECURSION)
		{
			rasm.Mov(nReg, rasm.Var<unsigned int>(varAutoYield, offsetof(Value,ev_length)));
			rasm.Cmp(nReg,0);
			rasm.Je("AutoYieldBack");
			// set autoyield to .T. again 
			rasm.CallConv(fastcall).Call(_Execute, "_VFP.AutoYield = .T.");
			rasm.Label("AutoYieldBack");
		}
	}
	else if (nFlags & (BINDEVENTSEX_CALL_AFTER | BINDEVENTSEX_RETURN_VALUE))
	{
		rasm.CallConv(fastcall).Call(_Evaluate, &varReturn, nParmCount > 2 ? m_CallbackBuffer : lpMsg->m_CallbackFunction);

		if (nFlags & BINDEVENTSEX_NO_RECURSION)
		{
			rasm.Mov(nReg, rasm.Var<unsigned int>(varAutoYield, offsetof(Value, ev_length)));
			// if autoyield was .F. before we don't need to set it
			rasm.Cmp(nReg, 0);
			rasm.Je("AutoYieldBack");
			// set autoyield to .T. again 
			rasm.CallConv(fastcall).Call(_Execute, "_VFP.AutoYield = .T.");
			rasm.Label("AutoYieldBack");
		}

		rasm.Mov(AL, rasm.Var<char>(varReturn, offsetof(Value, ev_type)));
		rasm.Cmp(AL, 'I');
		rasm.Jne("DoubleConversion");
		rasm.Mov(nReg, rasm.Var<unsigned int>(varReturn, offsetof(Value, ev_long)));
		rasm.Jmp("End");
		rasm.Label("DoubleConversion");
		rasm.CvttSd2Si(nReg, rasm.Var<double>(varReturn, offsetof(Value, ev_real)));
		rasm.Label("End");
	}
	rasm.Finish();

	if (lpMsg->m_CallbackThunk)
	{
		RuntimeAssembler::FreeMemory(lpMsg->m_CallbackThunk);
		RuntimeAssembler::UnregisterUnwindInfo(lpMsg->m_CallbackThunk);
		lpMsg->m_CallbackThunk = 0;
	}

	lpMsg->m_CallbackThunk = rasm.AllocateMemory(rasm.CodeSize());
	rasm.WriteCode(lpMsg->m_CallbackThunk);
	rasm.RegisterUnwindInfo(lpMsg->m_CallbackThunk);
}

void _stdcall WindowSubclass::ReleaseWindowSubclasses()
{
	VFP2CTls& tls = VFP2CTls::Tls();
	int nCount = tls.WindowSubclasses.GetIndex();
	for (int xj = nCount; xj >= 0; xj--)
	{
		delete tls.WindowSubclasses[xj];
	}
}

CallbackFunction::CallbackFunction()
{
	m_CallbackBuffer[0] = '\0';
	m_FuncAddress = 0;
	m_Object = 0;
	VFP2CTls::Tls().CallbackFunctions.Add(this);
}

CallbackFunction::~CallbackFunction()
{
	VFP2CTls::Tls().CallbackFunctions.Remove(this);
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pObjectName;
	if (m_Object != 0)
	{
		pObjectName.Format(CALLBACKFUNC_OBJECT_SCHEME, this);
		ValueEx::ReleaseObjectRef(pObjectName, m_Object);
	}
	if (m_FuncAddress != 0)
	{
		RuntimeAssembler::FreeMemory(m_FuncAddress);
		RuntimeAssembler::UnregisterUnwindInfo(m_FuncAddress);
	}
}

void CallbackFunction::ReleaseCallbackFuncs()
{
	VFP2CTls& tls = VFP2CTls::Tls();
	int nCount = tls.CallbackFunctions.GetIndex();
	for (int xj = nCount; xj >= 0; xj--)
	{
		delete tls.CallbackFunctions[xj];
	}
}

void _fastcall CreateCallbackFunc(ParamBlkEx& parm)
{
	CallbackFunction* pFunc = 0;
try
{
	int nErrorNo = VFP2C_Init_Callback();
	if (nErrorNo)
		throw nErrorNo;

	FoxString pCallback(parm(1));
	FoxString pRetVal(parm(2));
	FoxString pParameterlist(parm(3));
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pObjectName;

	DWORD nSyncFlag = parm.PCount() == 5 ? parm(5)->ev_long : CALLBACK_SYNCRONOUS | CALLBACK_STDCALL;
	AsmCallingConvention nCallConv;
	if ((nSyncFlag & CALLBACK_CDECL) > 0)
		nCallConv = cdeclaration;
	else if ((nSyncFlag & CALLBACK_FASTCALL) > 0)
		nCallConv = fastcall;
	else
		nCallConv = stdcall;

	nSyncFlag &= ~(CALLBACK_CDECL | CALLBACK_FASTCALL | CALLBACK_STDCALL);
	nSyncFlag = nSyncFlag ? nSyncFlag : CALLBACK_SYNCRONOUS; // set nSyncFlag to default CALLBACK_SYNCRONOUS if it is 0

	RuntimeAssembler rasm(nCallConv, false, false);
	AsmRegister ReturnReg = rasm.ReturnRegister();

	int nParmCount, nParmNo, nWordCount, nPrecision;
	CStringView pParm;
	CStringView pParmType;
	CStringView pParmPrec;

	if (pCallback.Len() > VFP2C_MAX_CALLBACK_FUNCTION)
	{
		SaveCustomError("CreateCallbackFunc", "Callback function length greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}
		

	if (nSyncFlag != CALLBACK_SYNCRONOUS &&
		nSyncFlag != CALLBACK_ASYNCRONOUS_POST &&
		nSyncFlag != CALLBACK_ASYNCRONOUS_SEND)
	{
		SaveCustomError("CreateCallbackFunc", "Invalid nFlags parameter '%U'.", nSyncFlag);
		throw E_INVALIDPARAMS;
	}
		
	pFunc = new CallbackFunction();
	if (!pFunc)
		throw E_INSUFMEMORY;

	if (parm.PCount() >= 4)
	{
		if (parm(4)->Vartype() == 'O')
		{
			pObjectName.Format(CALLBACKFUNC_OBJECT_SCHEME, pFunc);
			parm(4)->StoreObjectRef(pObjectName, pFunc->m_Object);
			strcpy(pFunc->m_CallbackBuffer, pObjectName);
			strcat(pFunc->m_CallbackBuffer,".");
		}
		else if (parm(4)->Vartype() != '0')
		{
			SaveCustomError("CreateCallbackFunc", "Invalid type '%s' for parameter oObject, should be an object or .NULL.");
			throw E_INVALIDPARAMS;
		}
	}
 
	nParmCount = pParameterlist.GetWordCount(',');
	if (nParmCount > VFP2C_MAX_CALLBACK_PARAMETERS)
	{
		SaveCustomError("CreateCallbackFunc", "Parameter count greater than maximum of 27.");
		throw E_INVALIDPARAMS;
	}
	
	AsmVariable pReturnVar, pBufferVar;

	// return value needed?
	if (pRetVal.Len() > 0 && !pRetVal.ICompare("void"))
	{
		pReturnVar = rasm.LocalVar<Value>();
	}
	// local buffer variable needed
	if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST | CALLBACK_ASYNCRONOUS_SEND))
	{
		pBufferVar = rasm.LocalVar<void*>();
	}

	if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST|CALLBACK_ASYNCRONOUS_SEND))
	{
		rasm.CallConv(cdeclaration).Call(malloc, VFP2C_MAX_CALLBACK_BUFFER);
		rasm.Cmp(ReturnReg,0);
		rasm.Je("ErrorOut");
		rasm.Mov(pBufferVar, ReturnReg);
	}

	if (nParmCount)
	{
		// fill static part of buffer
		strcat(pFunc->m_CallbackBuffer,pCallback);
		strcat(pFunc->m_CallbackBuffer,"(");
		if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST|CALLBACK_ASYNCRONOUS_SEND))
		{
			rasm.CallConv(cdeclaration).Call(memcpy, pBufferVar, pFunc->m_CallbackBuffer, strlen(pFunc->m_CallbackBuffer));
			rasm.Mov(ReturnReg, pBufferVar);
			rasm.Add(ReturnReg,(int)strlen(pFunc->m_CallbackBuffer));
		}
		else
			rasm.Mov(ReturnReg,(pFunc->m_CallbackBuffer+strlen(pFunc->m_CallbackBuffer)));
	}
	else
	{
		strcat(pFunc->m_CallbackBuffer,pCallback);
		strcat(pFunc->m_CallbackBuffer,"()");
		if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST|CALLBACK_ASYNCRONOUS_SEND))
		{
			rasm.CallConv(cdeclaration).Call(memcpy, pBufferVar, pFunc->m_CallbackBuffer, strlen(pFunc->m_CallbackBuffer) + 1);
		}
	}

	CStringView pParameters = pParameterlist;
	for (nParmNo = 0; nParmNo < nParmCount; ++nParmNo)
	{
		pParm = pParameters.GetWordNum(nParmNo + 1, ',');
		pParmType = pParm.Alltrim();

		nWordCount = pParmType.GetWordCount(' ');
		if (nWordCount == 2)
		{
			pParmPrec = pParmType.GetWordNum(2, ' ').Alltrim();
			pParmType = pParmType.GetWordNum(1, ' ').Alltrim();
		}
		else if (nWordCount > 2)
		{
			SaveCustomError("CreateCallbackFunc", "Invalid parameter type '%V'.", &pParmType);
			throw E_INVALIDPARAMS;
		}

		if (nParmNo >= 1)
		{
			rasm.Mov(AsmVariable(ReturnReg, 0), ',');
			rasm.Add(ReturnReg, 1);
		}
#if !defined(_WIN64)
		if (pParmType.ICompare("integer") || pParmType.ICompare("long"))
#else
		if (pParmType.ICompare("integer"))
#endif
		{
			AsmVariable& pParameter = rasm.Parameter<int>();
			rasm.CallConv(fastcall).Call(IntToHex, ReturnReg, pParameter);
		}
#if !defined(_WIN64)
		else if (pParmType.ICompare("uinteger") || pParmType.ICompare("ulong"))
#else
		else if (pParmType.ICompare("uinteger"))
#endif
		{
			AsmVariable& pParameter = rasm.Parameter<unsigned int>();
			rasm.CallConv(fastcall).Call(UIntToHex, ReturnReg, pParameter);
		}
		else if (pParmType.ICompare("string"))
		{
			AsmVariable& pParameter = rasm.Parameter<void*>();
			if (nWordCount == 1)
			{
				rasm.CallConv(fastcall).Call(UIntToHex, ReturnReg, pParameter);
			}
			else
			{
				char *pFunc;
				if (pParmPrec.ICompare("ansi"))
					pFunc = "ReadCString";
				else if (pParmPrec.ICompare("unicode"))
					pFunc = "ReadWString";
				else
				{
					SaveCustomError("CreateCallbackFunc", "Invalid parameter type '%V %V'.", &pParmType, &pParmPrec);
					throw E_INVALIDPARAMS;
				}
				
				// copy function name into buffer
				int nStringLen = strlen(pFunc);
				rasm.CallConv(cdeclaration).Call(memcpy, ReturnReg, pFunc, nStringLen);

				// adjust EAX (end of string)
				rasm.Add(ReturnReg, nStringLen);
				// copy '(' to end of buffer
				rasm.Mov(AsmVariable(ReturnReg, 0), '(');
				rasm.Add(ReturnReg,1);

				// add pointer parameter to buffer
				rasm.CallConv(fastcall).Call(UIntToHex, ReturnReg, pParameter);
				// copy ')' to end of buffer
				rasm.Mov(AsmVariable(ReturnReg, 0), ')');
				rasm.Add(ReturnReg,1);
			}
		}
		else if (pParmType.ICompare("short"))
		{
			AsmVariable& pParameter = rasm.Parameter<short>();
			rasm.CallConv(fastcall).Call(ShortToHex, ReturnReg, pParameter);
		}
		else if (pParmType.ICompare("ushort"))
		{
			AsmVariable& pParameter = rasm.Parameter<unsigned short>();
			rasm.CallConv(fastcall).Call(UShortToHex, ReturnReg, pParameter);
		}
		else if (pParmType.ICompare("bool"))
		{
			AsmVariable& pParameter = rasm.Parameter<int>();
			rasm.CallConv(fastcall).Call(BoolToStr, ReturnReg, pParameter);
		}
		else if (pParmType.ICompare("single"))
		{
			AsmVariable& pParameter = rasm.Parameter<float>();

			if (nWordCount == 2)
			{
				nPrecision = atoi(pParmPrec.Data);
				if (nPrecision < 0 || nPrecision > 6)
				{
					SaveCustomError("CreateCallbackFunc", "Invalid parameter type '%V %V'.", &pParmType, &pParmPrec);
					throw E_INVALIDPARAMS;
				}
			}
			else
				nPrecision = 6;

			rasm.CallConv(fastcall).Call(FloatToStr, ReturnReg, pParameter, nPrecision);
		}
		else if (pParmType.ICompare("double"))
		{
			AsmVariable& pParameter = rasm.Parameter<double>();

			if (nWordCount == 2)
			{
				nPrecision = atoi(pParmPrec.Data);
				if (nPrecision < 0 || nPrecision > 16)
				{
					SaveCustomError("CreateCallbackFunc", "Invalid parameter type '%V %V'.", &pParmType, &pParmPrec);
					throw E_INVALIDPARAMS;
				}
			}
			else
				nPrecision = 6;

			rasm.CallConv(fastcall).Call(DoubleToStr, ReturnReg, pParameter, nPrecision);
		}
#if !defined(_WIN64)
		else if (pParmType.ICompare("int64"))
#else
		else if (pParmType.ICompare("int64") || pParmType.ICompare("long"))
#endif
		{
			AsmVariable& pParameter = rasm.Parameter<__int64>();
			if (nWordCount == 1)
			{
				rasm.CallConv(fastcall).Call(Int64ToHex, ReturnReg, pParameter);
			}
			else
			{
				if (pParmPrec.ICompare("binary"))
				{
					rasm.CallConv(fastcall).Call(Int64ToVarbinary, ReturnReg, pParameter);
				}
				else if (pParmPrec.ICompare("literal"))
				{
					rasm.Mov(AsmVariable(ReturnReg,0), '"');
					rasm.Add(ReturnReg, 1);
					rasm.CallConv(fastcall).Call(Int64ToStr, ReturnReg, pParameter);
					rasm.Mov(AsmVariable(ReturnReg,0), '"');
					rasm.Add(ReturnReg, 1);
				}
				else if (pParmPrec.ICompare("currency"))
				{
					rasm.CallConv(fastcall).Call(Int64ToCurrency, ReturnReg, pParameter);
				}
				else
				{
					SaveCustomError("CreateCallbackFunc", "Invalid parameter type '%V %V'.", &pParmType, &pParmPrec);
					throw E_INVALIDPARAMS;
				}
					
			}
		}
#if !defined(_WIN64)
		else if (pParmType.ICompare("int64"))
#else
		else if (pParmType.ICompare("uint64") || pParmType.ICompare("ulong"))
#endif
		{
			AsmVariable& pParameter = rasm.Parameter<unsigned __int64>();
			if (nWordCount == 1)
			{
				rasm.CallConv(fastcall).Call(UInt64ToHex, ReturnReg, pParameter);
			}
			else
			{
				if (pParmPrec.ICompare("binary"))
				{
					rasm.CallConv(fastcall).Call(Int64ToVarbinary, ReturnReg, pParameter);
				}
				else if (pParmPrec.ICompare("literal"))
				{
					rasm.Mov(AsmVariable(ReturnReg, 0), '"');
					rasm.Add(ReturnReg, 1);
					rasm.CallConv(fastcall).Call(UInt64ToStr, ReturnReg, pParameter);
					rasm.Mov(AsmVariable(ReturnReg, 0), '"');
					rasm.Add(ReturnReg, 1);
				}
				else if (pParmPrec.ICompare("currency"))
				{
					rasm.CallConv(fastcall).Call(Int64ToCurrency, ReturnReg, pParameter);
				}
				else
				{
					SaveCustomError("CreateCallbackFunc", "Invalid parameter type '%V %V'.", &pParmType, &pParmPrec);
					throw E_INVALIDPARAMS;
				}
			}
		}
		else
		{
			SaveCustomError("CreateCallbackFunc", "Invalid parameter type '%V'.", &pParmType);
			throw E_INVALIDPARAMS;
		}

		pParameters = pParameters + pParm.Len;
	}

	// add ')' and nullterminate
	if (nParmCount)
	{
		rasm.Mov(AsmVariable(ReturnReg, 0), ')');
		rasm.Add(ReturnReg,1);
		rasm.Mov(AsmVariable(ReturnReg, 0), '\0');
	}

	if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST|CALLBACK_ASYNCRONOUS_SEND))
	{
		/* PostMessage(ghCallbackHwnd,WM_ASYNCCALLBACK,pBuffer,0); */
		if (nSyncFlag & CALLBACK_ASYNCRONOUS_POST)
			rasm.CallConv(stdcall).Call(PostMessage, ghCallbackHwnd, WM_ASYNCCALLBACK, pBufferVar, 0);
		else
			rasm.CallConv(stdcall).Call(PostMessage, ghCallbackHwnd, WM_ASYNCCALLBACK, pBufferVar, 0);
	}
	else if (pRetVal.Len() == 0 || pRetVal.ICompare("void"))
	{
		/* EXECUTE(pFunc->aCallbackBuffer); */
		rasm.CallConv(fastcall).Call(_Execute, pFunc->m_CallbackBuffer);
	}
	else
	{
		CStringView pRetType = pRetVal;
		/* EVALUATE(vRetVal,pFunc->aCallbackBuffer) */
		// rasm.Lea(ECX, pReturnVar);
		rasm.CallConv(fastcall).Call(_Evaluate, &pReturnVar, pFunc->m_CallbackBuffer);

		// return value
#if !defined(_WIN64)
		if (pRetType.ICompare("integer") || pRetType.ICompare("long"))
#else
		if (pRetType.ICompare("integer"))
#endif
		{
			rasm.Mov(AL, rasm.Var<char>(pReturnVar, offsetof(Value, ev_type)));
			rasm.Cmp(AL, 'N');
			rasm.Je("DoubleConversion");
			rasm.Mov(ReturnReg, rasm.Var<int>(pReturnVar, offsetof(Value, ev_long)));
			rasm.Jmp("End");
			rasm.Label("DoubleConversion");
			rasm.CvttSd2Si(ReturnReg, rasm.Var<double>(pReturnVar, offsetof(Value, ev_real)));
			rasm.Label("End");
		}
		else if (pRetType.ICompare("uinteger") || pRetType.ICompare("ulong"))
		{
			rasm.Mov(AL, rasm.Var<char>(pReturnVar, offsetof(Value,ev_type)));
			rasm.Cmp(AL,'N');
			rasm.Je("DoubleConversion");
			rasm.Mov(ReturnReg, rasm.Var<unsigned int>(pReturnVar, offsetof(Value,ev_long)));
			rasm.Jmp("End");
			rasm.Label("DoubleConversion");
			rasm.CvttSd2Si(ReturnReg, rasm.Var<double>(pReturnVar, offsetof(Value, ev_real)));
			rasm.Label("End");
		}
		else if (pRetType.ICompare("short"))
			rasm.Mov(AX, rasm.Var<int>(pReturnVar,offsetof(Value,ev_long)));
		else if (pRetType.ICompare("ushort"))
			rasm.Mov(AX, rasm.Var<unsigned int>(pReturnVar,offsetof(Value,ev_long)));
		else if (pRetType.ICompare("single") || pRetType.ICompare("double"))
		{
#if !defined(_WIN64)
			rasm.Fld(rasm.Var<double>(pReturnVar, offsetof(Value, ev_real)));
#else
			rasm.Mov(XMM0, rasm.Var<double>(pReturnVar, offsetof(Value, ev_real)));
#endif
		}
		else if (pRetType.ICompare("bool"))
			rasm.Mov(EAX, rasm.Var<unsigned int>(pReturnVar, offsetof(Value,ev_length)));
#if !defined(_WIN64)
		else if (pRetType.ICompare("int64"))
#else
		else if (pRetType.ICompare("int64") || pRetType.ICompare("long"))
#endif
		{
			rasm.Mov(AL, rasm.Var<char>(pReturnVar, offsetof(Value,ev_type)));
			rasm.Cmp(AL,'N');
			rasm.Je("DoubleConversion");
			rasm.Mov(ReturnReg, rasm.Var<int>(pReturnVar, offsetof(Value,ev_long)));
			rasm.Cdq();
			rasm.Jmp("End");
			rasm.Label("DoubleConversion");
			rasm.CvttSd2Si(ReturnReg, rasm.Var<double>(pReturnVar, offsetof(Value, ev_real)));
			rasm.Label("End");
		}
		else if (pRetType.ICompare("uint64"))
		{
			rasm.Mov(AL, rasm.Var<char>(pReturnVar, offsetof(Value,ev_type)));
			rasm.Cmp(AL,'N');
			rasm.Je("DoubleConversion");
			rasm.Mov(ReturnReg, rasm.Var<unsigned int>(pReturnVar, offsetof(Value,ev_long)));
#if !defined(_WIN64)
			rasm.Xor(EDX,EDX);
#endif
			rasm.Jmp("End");
			rasm.Label("DoubleConversion");
			rasm.CvttSd2Si(ReturnReg, rasm.Var<double>(pReturnVar, offsetof(Value, ev_real)));
			rasm.Label("End");
		}
		else
		{
			SaveCustomError("CreateCallbackFunc", "Invalid return value type '%V'.", &pRetType);
			throw E_INVALIDPARAMS;
		}
	}

	rasm.Label("ErrorOut");
	rasm.Finish();

	pFunc->m_FuncAddress = rasm.AllocateMemory(rasm.CodeSize());
	rasm.WriteCode(pFunc->m_FuncAddress);
	rasm.RegisterUnwindInfo(pFunc->m_FuncAddress);
	Return(pFunc->m_FuncAddress);
}
catch(int nErrorNo)
{
	if (pFunc)
		delete pFunc;
	RaiseError(nErrorNo);
}
}

void _fastcall DestroyCallbackFunc(ParamBlkEx& parm)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	int nCount = tls.CallbackFunctions.GetCount();
	void* pFunc = parm(1)->Ptr<void*>();
	for (int xj = 0; xj < nCount; xj++)
	{
		if (tls.CallbackFunctions[xj]->m_FuncAddress == pFunc)
		{
			CallbackFunction* pFunc = tls.CallbackFunctions[xj];
			delete pFunc;
			Return(true);
		}
	}
	Return(false);
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

#endif // _THREADSAFE