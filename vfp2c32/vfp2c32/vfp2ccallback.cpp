#ifndef _THREADSAFE

#include <windows.h>
#include <malloc.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
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
	if (gnCallbackAtom && ghCallbackHwnd)
		return 0;

	if (gnCallbackAtom == 0)
	{
		/* message only window */
		WNDCLASSEX wndClass;
		RtlZeroMemory(&wndClass, sizeof(WNDCLASSEX));
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
	UINT uMsg = parm(2)->NumericCast<UINT>();
	VFP2CTls& tls = VFP2CTls::Tls();
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
			SaveCustomErrorEx("BindEventsEx", "Callback function length is zero or greater than maximum length of 1024.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}

		if (parm(3)->Vartype() != 'O' && parm(3)->Vartype() != '0')
		{
			SaveCustomErrorEx("BindEventsEx", "Invalid parameter type '%s' for parameter 3", E_VFP_INVALIDPARAMS, parm(3)->Vartype());
			throw E_APIERROR;
		}

		if (parm.PCount() >= 5 && parm(5)->Vartype() != 'C' && parm(5)->Vartype() != '0')
		{
			SaveCustomErrorEx("BindEventsEx", "Invalid parameter type '%s' for parameter 5", E_VFP_INVALIDPARAMS, parm(5)->Vartype());
			throw E_APIERROR;
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
				SaveCustomErrorEx("BindEventsEx", "Invalid nFlags combination - BINDEVENTSEX_CALL_BEFORE, BINDEVENTSEX_CALL_AFTER and BINDEVENTSEX_RETURN_VALUE are mutually exclusive.", E_VFP_INVALIDPARAMS);
				throw E_APIERROR;
			}
		}
		else
			nFlags = BINDEVENTSEX_CALL_BEFORE;

		bClassProc = (nFlags & BINDEVENTSEX_CLASSPROC) > 0;

		lpSubclass = WindowSubclass::FindWindowSubclass(hHwnd, bClassProc);
		if (!lpSubclass)
		{
			lpSubclass = new(std::nothrow) WindowSubclass();
			if (!lpSubclass)
				throw E_INSUFMEMORY;

			tls.WindowSubclasses.Add(lpSubclass);
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
				tls.WindowSubclasses.Remove(lpSubclass);
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
	bool bClassProc = parm.PCount() == 3 && parm(3)->ev_length;
	VFP2CTls& tls = VFP2CTls::Tls();

	// get reference to struct for the hWnd, if none is found - the window was not subclassed
	lpSubclass = WindowSubclass::FindWindowSubclass(hHwnd, bClassProc);

	if (!lpSubclass)
	{
		SaveCustomError("UnbindEventsEx", "There are no message bindings for window %I.", hHwnd);
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
			tls.WindowSubclasses.Remove(lpSubclass);
		}
	}
	else // unsubclass window and free all hooks
	{
		delete lpSubclass;
		tls.WindowSubclasses.Remove(lpSubclass);
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

WindowMessageCallback::WindowMessageCallback(UINT msg)
{
	m_Msg = msg;
	m_Object = 0;
	m_CallbackThunk = 0;
	m_ReturnValue = FALSE;
	m_CallbackFunction = new(std::nothrow) char[VFP2C_MAX_CALLBACKBUFFER];
	if (!m_CallbackFunction)
		throw E_INSUFMEMORY;
}

void WindowMessageCallback::Release(WindowSubclass* pSubclass)
{
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pObjectName;

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

	if (m_CallbackFunction)
		delete[] m_CallbackFunction;
}

WindowSubclass::WindowSubclass()
{
	m_ClassProc = false;
	m_Hwnd = 0;
	m_DefaultWndProc = 0;
	m_WindowThunk = 0;
}

WindowSubclass::~WindowSubclass()
{
	unsigned int nCount = m_BoundMessages.GetCount();
	for (unsigned int xj = 0; xj < nCount; xj++)
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
}

WindowSubclass* WindowSubclass::FindWindowSubclass(HWND hHwnd, bool bClassProc)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	unsigned int nCount = tls.WindowSubclasses.GetCount();
	for (unsigned int xj = 0; xj < nCount; xj++)
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
	unsigned int nIndex = m_BoundMessages.INVALID_INDEX;
	unsigned int nCount = m_BoundMessages.GetCount();
	for (unsigned int xj = 0; xj < nCount; xj++)
	{
		if (m_BoundMessages[xj].m_Msg == uMsg)
		{
			nIndex = xj;
			break;
		}
	}
	if (nIndex == m_BoundMessages.INVALID_INDEX)
	{
		WindowMessageCallback pMsgCallback(uMsg);
		nIndex = m_BoundMessages.Add(pMsgCallback);
	}
	return &m_BoundMessages[nIndex];
}

bool WindowSubclass::RemoveMessageCallback(UINT uMsg)
{
	unsigned int nIndex = m_BoundMessages.INVALID_INDEX;
	unsigned int nCount = m_BoundMessages.GetCount();
	for (unsigned int xj = 0; xj < nCount; xj++)
	{
		if (m_BoundMessages[xj].m_Msg == uMsg)
		{
			nIndex = xj;
			break;
		}
	}
	if (nIndex == m_BoundMessages.INVALID_INDEX)
		return false;

	WindowMessageCallback pMsgCallback(m_BoundMessages[nIndex]);
	m_BoundMessages.Remove(nIndex);
	pMsgCallback.Release(this);
	return true;
}

WindowMessageCallback* _stdcall WindowSubclass::FindMessageCallback(WindowSubclass* pSubclass, UINT uMsg)
{
	unsigned int nCount = pSubclass->m_BoundMessages.GetCount();
	for (unsigned int xj = 0; xj < nCount; xj++)
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
		}
	}
	else
	{
		if (!SetClassLongPtr(m_Hwnd,GCLP_WNDPROC,(LONG_PTR)m_DefaultWndProc))
		{
			SaveWin32Error("SetClassLongPtr", GetLastError());
		}
		EnumThreadWindows(GetCurrentThreadId(), UnsubclassWindowClassCallback, (LPARAM)this);
	}
}

BOOL _stdcall WindowSubclass::UnsubclassWindowClassCallback(HWND hHwnd, LPARAM lParam)
{
	WindowSubclass* lpSubclass = (WindowSubclass*)lParam;
	void* nProc;
	
	nProc = (void*)GetWindowLongPtr(hHwnd,GWLP_WNDPROC);
	if (nProc == lpSubclass->m_WindowThunk)
	{
		if (!SetWindowLongPtr(hHwnd,GWLP_WNDPROC,(LONG_PTR)lpSubclass->m_DefaultWndProc))
		{
			SaveWin32Error("SetWindowLongPtr", GetLastError());
		}
	}

	if (!EnumChildWindows(hHwnd, UnsubclassWindowClassCallbackChild,(LPARAM)lpSubclass))
	{
		if (GetLastError() != NO_ERROR)
		{
			SaveWin32Error("EnumChildWindows", GetLastError());
		}
	}

	return TRUE;
}

BOOL _stdcall WindowSubclass::UnsubclassWindowClassCallbackChild(HWND hHwnd, LPARAM lParam)
{
	WindowSubclass* lpSubclass = (WindowSubclass*)lParam;
	void* nProc;
	
	nProc = (void*)GetWindowLongPtr(hHwnd,GWLP_WNDPROC);
	if (nProc == lpSubclass->m_WindowThunk)
	{
		if (!SetWindowLongPtr(hHwnd,GWLP_WNDPROC,(LONG_PTR)lpSubclass->m_DefaultWndProc))
		{
			SaveWin32Error("SetWindowLongPtr", GetLastError());
		}
	}
	
	return TRUE;
}

void WindowSubclass::CreateSubclassThunkProc()
{
	RuntimeAssembler rasm(stdcall, true);
	AsmRegister nReturnReg = rasm.ReturnRegister();

	AsmVariable& varHwnd = rasm.Parameter<HWND>();
	AsmVariable& varUmsg = rasm.Parameter<UINT>();
	AsmVariable& varWparam = rasm.Parameter<WPARAM>();
	AsmVariable& varLparam = rasm.Parameter<LPARAM>();
	AsmVariable& bReturn = rasm.LocalVar<BOOL>();
	const char* pLabelWndProc = "lWinProc";
	const char* pLabelEnd = "lEnd";
	rasm.CallConv(stdcall).Call(&WindowSubclass::FindMessageCallback, this, varUmsg);
	rasm.Cmp(nReturnReg,0);
	rasm.Je(pLabelWndProc);
	// rasm.BreakPoint();
	AsmVariable pReturnVar = AsmVariable::Var<BOOL>(nReturnReg, offsetof(WindowMessageCallback, m_ReturnValue));
	rasm.Mov(bReturn, pReturnVar);
	AsmVariable pCallbackVar = AsmVariable::Var<void*>(nReturnReg, offsetof(WindowMessageCallback, m_CallbackThunk));
	rasm.Mov(nReturnReg, pCallbackVar);
	rasm.CallConv(stdcall).Call(nReturnReg, varHwnd, varUmsg, varWparam, varLparam); // call thunk
	rasm.Mov(ECX, bReturn);
	rasm.Cmp(ECX, 0);
	rasm.Je(pLabelEnd);
	rasm.Label(pLabelWndProc);
	rasm.CallConv(stdcall).Call(CallWindowProc, (UINT_PTR)m_DefaultWndProc, varHwnd, varUmsg, varWparam, varLparam);
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
	unsigned int nParmCount, nParmNo = 0, xj;
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
		rasm.CallConv(stdcall).Call(CallWindowProc, (UINT_PTR)m_DefaultWndProc, varHwnd, varUmsg, varWparam, varLparam);
	}

	if (nFlags & BINDEVENTSEX_NO_RECURSION)
	{
		// _Evaluate(&vAutoYield,"_VFP.AutoYield");
		rasm.CallConv(fastcall).Call(_Evaluate, &varAutoYield, "_VFP.AutoYield");

		// if (vAutoYield.ev_length)
		//  _Execute("_VFP.AutoYield = .F.")
		AsmVariable pVar = rasm.Var<unsigned int>(varAutoYield, offsetof(Value, ev_length));
		rasm.Mov(nReg, pVar);
		rasm.Cmp(nReg,0);
		rasm.Je("AutoYieldFalse");
		rasm.CallConv(fastcall).Call(_Execute, "_VFP.AutoYield = .F.");
		rasm.Label("AutoYieldFalse");
	}

	

	if (!pParameterList)
	{
		nParmCount = 6;
#pragma warning(disable : 4996)
		strcat(lpMsg->m_CallbackFunction, "(%U,%U,%I,%I)");
#pragma warning(default : 4996)
		rasm.CallConv(cdeclaration);
		rasm.StackMode(true);
		rasm.PassParameter(m_CallbackBuffer, nParmNo);
		rasm.PassParameter(lpMsg->m_CallbackFunction, nParmNo);
		rasm.PassParameter(varHwnd, nParmNo);
		rasm.PassParameter(varUmsg, nParmNo);
		rasm.PassParameter(varWparam, nParmNo);
		rasm.PassParameter(varLparam, nParmNo);
		rasm.StackMode(false);
	}
	else
	{
		nParmCount = pParameterList.GetWordCount(',');
		if (nParmCount > VFP2C_MAX_CALLBACK_PARAMETERS)
		{
			SaveCustomErrorEx("BindEventsEx", "Parameter count greater than maximum of 27.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}
		else if (nParmCount > 0)
		{
			rasm.CallConv(cdeclaration);
			rasm.StackMode(true);
			rasm.PassParameter(m_CallbackBuffer, nParmNo);
			rasm.PassParameter(lpMsg->m_CallbackFunction, nParmNo);
		}

		for (xj = 1; xj <= nParmCount; xj++)
		{
			CStringView pParamater = pParameterList.GetWordNum(xj, ',').Alltrim();

			if (pParamater.ICompare(__sv("wparam")))
			{
				pConvertFlags.Append('I');
				rasm.PassParameter(varWparam, nParmNo);
			}
			else if (pParamater.ICompare(__sv("lparam")))
			{
				pConvertFlags.Append('I');
				rasm.PassParameter(varLparam, nParmNo);
			}
			else if (pParamater.ICompare(__sv("umsg")))
			{
				pConvertFlags.Append('U');
				rasm.PassParameter(varUmsg, nParmNo);
			}
			else if (pParamater.ICompare(__sv("hwnd")))
			{
				pConvertFlags.Append('U');
				rasm.PassParameter(varHwnd, nParmNo);
			}
			else if (pParamater.ICompare(__sv("unsigned(wparam)")))
			{
				pConvertFlags.Append('U');
				rasm.PassParameter(varWparam, nParmNo);
			}
			else if (pParamater.ICompare(__sv("unsigned(lparam)")))
			{
				pConvertFlags.Append('U');
				rasm.PassParameter(varLparam, nParmNo);
			}
			else if (pParamater.ICompare(__sv("hiword(wparam)")))
			{
				pConvertFlags.Append('i');
				rasm.PassParameter(nReg, nParmNo);
				rasm.Shr(nReg, 16);
				rasm.Mov(nReg, varWparam);
			}
			else if (pParamater.ICompare(__sv("loword(wparam)")))
			{
				pConvertFlags.Append('i');
				rasm.PassParameter(nReg, nParmNo);
				rasm.And(nReg, MAX_USHORT);
				rasm.Mov(nReg, varWparam);
			}
			else if (pParamater.ICompare(__sv("hiword(lparam)")))
			{
				pConvertFlags.Append('i');
				rasm.PassParameter(nReg, nParmNo);
				rasm.Shr(nReg, 16);
				rasm.Mov(nReg, varLparam);
			}
			else if (pParamater.ICompare(__sv("loword(lparam)")))
			{
				pConvertFlags.Append('i');
				rasm.PassParameter(nReg, nParmNo);
				rasm.And(nReg, MAX_USHORT);
				rasm.Mov(nReg, varLparam);
			}
			else if (pParamater.ICompare(__sv("unsigned(hiword(wparam))")))
			{
				pConvertFlags.Append('u');
				rasm.PassParameter(nReg, nParmNo);
				rasm.Shr(nReg, 16);
				rasm.Mov(nReg, varWparam);
			}
			else if (pParamater.ICompare(__sv("unsigned(loword(wparam))")))
			{
				pConvertFlags.Append('u');
				rasm.PassParameter(nReg, nParmNo);
				rasm.And(nReg, MAX_USHORT);
				rasm.Mov(nReg, varWparam);
			}
			else if (pParamater.ICompare(__sv("unsigned(hiword(lparam))")))
			{
				pConvertFlags.Append('u');
				rasm.PassParameter(nReg, nParmNo);
				rasm.Shr(nReg, 16);
				rasm.Mov(nReg, varLparam);
			}
			else if (pParamater.ICompare(__sv("unsigned(loword(lparam))")))
			{
				pConvertFlags.Append('u');
				rasm.PassParameter(nReg, nParmNo);
				rasm.And(nReg, MAX_USHORT);
				rasm.Mov(nReg, varLparam);
			}
			else if (pParamater.ICompare(__sv("bool(wparam)")))
			{
				pConvertFlags.Append('L');
				rasm.PassParameter(varWparam, nParmNo);
			}
			else if (pParamater.ICompare(__sv("bool(lparam)")))
			{
				pConvertFlags.Append('L');
				rasm.PassParameter(varLparam, nParmNo);
			}
			else
			{
				SaveCustomErrorEx("BindEventsEx", "Invalid parameter type '%V'", E_VFP_INVALIDPARAMS, &pParamater);
				throw E_APIERROR;
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
		if (nParmCount > 0)
		{
			nParmCount += 2;
			rasm.StackMode(false);
		}
	}

	// if any parameters should be passed we need to call sprintfex
	if (nParmCount > 2)
	{
		rasm.ManualCall(sprintfex);
	}

	if (nFlags & BINDEVENTSEX_CALL_BEFORE)
	{
		rasm.CallConv(fastcall).Call(_Execute, nParmCount > 2 ? m_CallbackBuffer : lpMsg->m_CallbackFunction);
		
		if (nFlags & BINDEVENTSEX_NO_RECURSION)
		{
			AsmVariable pVar;
			pVar = rasm.Var<unsigned int>(varAutoYield, offsetof(Value, ev_length));
			rasm.Mov(nReg, pVar);
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
			AsmVariable pVar;
			pVar = rasm.Var<unsigned int>(varAutoYield, offsetof(Value, ev_length));
			rasm.Mov(nReg, pVar);
			// if autoyield was .F. before we don't need to set it
			rasm.Cmp(nReg, 0);
			rasm.Je("AutoYieldBack");
			// set autoyield to .T. again 
			rasm.CallConv(fastcall).Call(_Execute, "_VFP.AutoYield = .T.");
			rasm.Label("AutoYieldBack");
		}

		AsmVariable pVar;
		pVar = rasm.Var<char>(varReturn, offsetof(Value, ev_type));
		rasm.Mov(AL, pVar);
		rasm.Cmp(AL, 'I');
		rasm.Jne("DoubleConversion");
		pVar = rasm.Var<unsigned int>(varReturn, offsetof(Value, ev_long));
		rasm.Mov(nReg, pVar);
		rasm.Jmp("End");
		rasm.Label("DoubleConversion");
		pVar = rasm.Var<double>(varReturn, offsetof(Value, ev_real));
		rasm.CvttSd2Si(nReg, pVar);
		rasm.Label("End");
	}
	rasm.Finish();

	if (lpMsg->m_CallbackThunk)
	{
		RuntimeAssembler::UnregisterUnwindInfo(lpMsg->m_CallbackThunk);
		RuntimeAssembler::FreeMemory(lpMsg->m_CallbackThunk);
		lpMsg->m_CallbackThunk = 0;
	}

	lpMsg->m_CallbackThunk = rasm.AllocateMemory(rasm.CodeSize());
	rasm.WriteCode(lpMsg->m_CallbackThunk);
	rasm.RegisterUnwindInfo(lpMsg->m_CallbackThunk);
}

void _stdcall WindowSubclass::ReleaseWindowSubclasses()
{
	VFP2CTls& tls = VFP2CTls::Tls();
	unsigned int nCount = tls.WindowSubclasses.GetCount();
	for (unsigned int xj = 0; xj < nCount; xj++)
	{
		WindowSubclass* lpSubclass = tls.WindowSubclasses[xj];
		delete lpSubclass;
	}
}

CallbackFunction::CallbackFunction()
{
	m_FuncAddress = 0;
	m_Object = 0;
	m_CallbackBuffer[0] = '\0';
}

CallbackFunction::~CallbackFunction()
{
	CStrBuilder<VFP2C_MAX_FUNCTIONBUFFER> pObjectName;
	if (m_Object != 0)
	{
		pObjectName.Format(CALLBACKFUNC_OBJECT_SCHEME, this);
		ValueEx::ReleaseObjectRef(pObjectName, m_Object);
	}
	if (m_FuncAddress != 0)
	{
		RuntimeAssembler::UnregisterUnwindInfo(m_FuncAddress);
		RuntimeAssembler::FreeMemory(m_FuncAddress);
	}
}

void CallbackFunction::ReleaseCallbackFuncs()
{
	VFP2CTls& tls = VFP2CTls::Tls();
	unsigned int nCount = tls.CallbackFunctions.GetCount();
	for (unsigned int xj = 0; xj < nCount; xj++)
	{
		CallbackFunction* pFunc = tls.CallbackFunctions[xj];
		delete pFunc;
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

	DWORD nSyncFlag = parm.PCount() == 5 ? parm(5)->ev_long : (CALLBACK_SYNCRONOUS | CALLBACK_STDCALL);
	AsmCallingConvention nCallConv;
	if ((nSyncFlag & CALLBACK_CDECL) > 0)
		nCallConv = cdeclaration;
	else if ((nSyncFlag & CALLBACK_FASTCALL) > 0)
		nCallConv = fastcall;
	else
		nCallConv = stdcall;

	nSyncFlag &= ~(CALLBACK_CDECL | CALLBACK_FASTCALL | CALLBACK_STDCALL);
	nSyncFlag = nSyncFlag ? nSyncFlag : CALLBACK_SYNCRONOUS; // set nSyncFlag to default CALLBACK_SYNCRONOUS if it is 0

	RuntimeAssembler rasm(nCallConv, false);
	AsmRegister ReturnReg = rasm.ReturnRegister();
	AsmVariable pVar;

	int nPrecision;
	unsigned int nParmNo, nParmCount, nWordCount;
	CStringView pParm;
	CStringView pParmType;
	CStringView pParmPrec;

	if (pCallback.Len() > VFP2C_MAX_CALLBACK_FUNCTION)
	{
		SaveCustomErrorEx("CreateCallbackFunc", "Callback function length greater than maximum length of 1024.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}
		

	if (nSyncFlag != CALLBACK_SYNCRONOUS &&
		nSyncFlag != CALLBACK_ASYNCRONOUS_POST &&
		nSyncFlag != CALLBACK_ASYNCRONOUS_SEND)
	{
		SaveCustomErrorEx("CreateCallbackFunc", "Invalid nFlags parameter '%U'.", E_VFP_INVALIDPARAMS, nSyncFlag);
		throw E_APIERROR;
	}
		
	pFunc = new(std::nothrow) CallbackFunction();
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
			SaveCustomErrorEx("CreateCallbackFunc", "Invalid type '%s' for parameter oObject, should be an object or .NULL.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}
	}
 
	nParmCount = pParameterlist.GetWordCount(',');
	if (nParmCount > VFP2C_MAX_CALLBACK_PARAMETERS)
	{
		SaveCustomErrorEx("CreateCallbackFunc", "Parameter count greater than maximum of 27.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}
	
	AsmVariable pReturnVar, pBufferVar = AsmVariable::Init();

	// return value needed?
	if (pRetVal.Len() > 0 && !pRetVal.ICompare(__sv("void")))
	{
		pReturnVar = rasm.LocalVar<Value>();
	}
	// local buffer variable needed
	if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST | CALLBACK_ASYNCRONOUS_SEND))
	{
		pBufferVar = rasm.LocalVar<void*>();
	}

	if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST | CALLBACK_ASYNCRONOUS_SEND))
	{
		rasm.CallConv(cdeclaration).Call(malloc, VFP2C_MAX_CALLBACK_BUFFER);
		rasm.Cmp(ReturnReg,0);
		rasm.Je("ErrorOut");
		rasm.Mov(pBufferVar, ReturnReg);
	}

	if (nParmCount)
	{
		// fill static part of buffer
		strcat(pFunc->m_CallbackBuffer,pCallback.Ptr<LPCSTR>());
		strcat(pFunc->m_CallbackBuffer,"(");
		if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST | CALLBACK_ASYNCRONOUS_SEND))
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
		strcat(pFunc->m_CallbackBuffer,pCallback.Ptr<LPCSTR>());
		strcat(pFunc->m_CallbackBuffer,"()");
		if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST|CALLBACK_ASYNCRONOUS_SEND))
		{
			rasm.CallConv(cdeclaration).Call(memcpy, pBufferVar, pFunc->m_CallbackBuffer, strlen(pFunc->m_CallbackBuffer) + 1);
		}
	}

	CStringView pParameters = pParameterlist;
	for (nParmNo = 0; nParmNo < nParmCount; ++nParmNo)
	{
		pParm = pParameters.GetWordNum(1, ',');
		pParmType = pParm.Alltrim();

		nWordCount = pParmType.GetWordCount(' ');
		if (nWordCount == 2)
		{
			pParmPrec = pParmType.GetWordNum(2, ' ').Alltrim();
			pParmType = pParmType.GetWordNum(1, ' ').Alltrim();
		}
		else if (nWordCount > 2)
		{
			SaveCustomErrorEx("CreateCallbackFunc", "Invalid parameter type '%V'.", E_VFP_INVALIDPARAMS, &pParmType);
			throw E_APIERROR;
		}

		if (nParmNo >= 1)
		{
			pVar = AsmVariable::Var(ReturnReg, 0);
			rasm.Mov(pVar, ',');
			rasm.Add(ReturnReg, 1);
		}
#if !defined(_WIN64)
		if (pParmType.ICompare(__sv("integer")) || pParmType.ICompare(__sv("long")))
#else
		if (pParmType.ICompare(__sv("integer")))
#endif
		{
			AsmVariable& pParameter = rasm.Parameter<int>();
			rasm.CallConv(fastcall).Call(IntToHex, ReturnReg, pParameter);
		}
#if !defined(_WIN64)
		else if (pParmType.ICompare(__sv("uinteger")) || pParmType.ICompare(__sv("ulong")))
#else
		else if (pParmType.ICompare(__sv("uinteger")))
#endif
		{
			AsmVariable& pParameter = rasm.Parameter<unsigned int>();
			rasm.CallConv(fastcall).Call(UIntToHex, ReturnReg, pParameter);
		}
		else if (pParmType.ICompare(__sv("string")))
		{
			AsmVariable& pParameter = rasm.Parameter<void*>();
			if (nWordCount == 1)
			{
				rasm.CallConv(fastcall).Call(UIntToHex, ReturnReg, pParameter);
			}
			else
			{
				const char *pFunc;
				if (pParmPrec.ICompare(__sv("ansi")))
					pFunc = "ReadCString";
				else if (pParmPrec.ICompare(__sv("unicode")))
					pFunc = "ReadWString";
				else
				{
					SaveCustomErrorEx("CreateCallbackFunc", "Invalid parameter type '%V %V'.", E_VFP_INVALIDPARAMS, &pParmType, &pParmPrec);
					throw E_APIERROR;
				}
				
				// copy function name into buffer
				unsigned int nStringLen = (unsigned int)strlen(pFunc);
				rasm.CallConv(cdeclaration).Call(memcpy, ReturnReg, pFunc, nStringLen);

				// adjust EAX (end of string)
				rasm.Add(ReturnReg, nStringLen);
				// copy '(' to end of buffer
				pVar = AsmVariable::Var(ReturnReg, 0);
				rasm.Mov(pVar, '(');
				rasm.Add(ReturnReg,1);

				// add pointer parameter to buffer
				rasm.CallConv(fastcall).Call(UIntToHex, ReturnReg, pParameter);
				// copy ')' to end of buffer
				rasm.Mov(pVar, ')');
				rasm.Add(ReturnReg,1);
			}
		}
		else if (pParmType.ICompare(__sv("short")))
		{
			AsmVariable& pParameter = rasm.Parameter<short>();
			rasm.CallConv(fastcall).Call(ShortToHex, ReturnReg, pParameter);
		}
		else if (pParmType.ICompare(__sv("ushort")))
		{
			AsmVariable& pParameter = rasm.Parameter<unsigned short>();
			rasm.CallConv(fastcall).Call(UShortToHex, ReturnReg, pParameter);
		}
		else if (pParmType.ICompare(__sv("bool")))
		{
			AsmVariable& pParameter = rasm.Parameter<int>();
			rasm.CallConv(fastcall).Call(BoolToStr, ReturnReg, pParameter);
		}
		else if (pParmType.ICompare(__sv("single")))
		{
			AsmVariable& pParameter = rasm.Parameter<float>();

			if (nWordCount == 2)
			{
				nPrecision = atoi(pParmPrec.Data);
				if (nPrecision < 0 || nPrecision > 6)
				{
					SaveCustomErrorEx("CreateCallbackFunc", "Invalid parameter type '%V %V'.", E_VFP_INVALIDPARAMS, &pParmType, &pParmPrec);
					throw E_APIERROR;
				}
			}
			else
				nPrecision = 6;

			rasm.CallConv(fastcall).Call(FloatToStr, ReturnReg, pParameter, nPrecision);
		}
		else if (pParmType.ICompare(__sv("double")))
		{
			AsmVariable& pParameter = rasm.Parameter<double>();

			if (nWordCount == 2)
			{
				nPrecision = atoi(pParmPrec.Data);
				if (nPrecision < 0 || nPrecision > 16)
				{
					SaveCustomErrorEx("CreateCallbackFunc", "Invalid parameter type '%V %V'.", E_VFP_INVALIDPARAMS, &pParmType, &pParmPrec);
					throw E_APIERROR;
				}
			}
			else
				nPrecision = 6;

			rasm.CallConv(fastcall).Call(DoubleToStr, ReturnReg, pParameter, nPrecision);
		}
#if !defined(_WIN64)
		else if (pParmType.ICompare(__sv("int64")))
#else
		else if (pParmType.ICompare(__sv("int64")) || pParmType.ICompare(__sv("long")))
#endif
		{
			AsmVariable& pParameter = rasm.Parameter<__int64>();
			if (nWordCount == 1)
			{
				rasm.CallConv(fastcall).Call(Int64ToHex, ReturnReg, pParameter);
			}
			else
			{
				if (pParmPrec.ICompare(__sv("binary")))
				{
					rasm.CallConv(fastcall).Call(Int64ToVarbinary, ReturnReg, pParameter);
				}
				else if (pParmPrec.ICompare(__sv("literal")))
				{
					pVar = AsmVariable::Var(ReturnReg, 0);
					rasm.Mov(pVar, '"');
					rasm.Add(ReturnReg, 1);
					rasm.CallConv(fastcall).Call(Int64ToStr, ReturnReg, pParameter);
					rasm.Mov(pVar, '"');
					rasm.Add(ReturnReg, 1);
				}
				else if (pParmPrec.ICompare(__sv("currency")))
				{
					rasm.CallConv(fastcall).Call(Int64ToCurrency, ReturnReg, pParameter);
				}
				else
				{
					SaveCustomErrorEx("CreateCallbackFunc", "Invalid parameter type '%V %V'.", E_VFP_INVALIDPARAMS, &pParmType, &pParmPrec);
					throw E_APIERROR;
				}
					
			}
		}
#if !defined(_WIN64)
		else if (pParmType.ICompare(__sv("int64")))
#else
		else if (pParmType.ICompare(__sv("uint64")) || pParmType.ICompare(__sv("ulong")))
#endif
		{
			AsmVariable& pParameter = rasm.Parameter<unsigned __int64>();
			if (nWordCount == 1)
			{
				rasm.CallConv(fastcall).Call(UInt64ToHex, ReturnReg, pParameter);
			}
			else
			{
				if (pParmPrec.ICompare(__sv("binary")))
				{
					rasm.CallConv(fastcall).Call(Int64ToVarbinary, ReturnReg, pParameter);
				}
				else if (pParmPrec.ICompare(__sv("literal")))
				{
					pVar = AsmVariable::Var(ReturnReg, 0);
					rasm.Mov(pVar, '"');
					rasm.Add(ReturnReg, 1);
					rasm.CallConv(fastcall).Call(UInt64ToStr, ReturnReg, pParameter);
					rasm.Mov(pVar, '"');
					rasm.Add(ReturnReg, 1);
				}
				else if (pParmPrec.ICompare(__sv("currency")))
				{
					rasm.CallConv(fastcall).Call(Int64ToCurrency, ReturnReg, pParameter);
				}
				else
				{
					SaveCustomErrorEx("CreateCallbackFunc", "Invalid parameter type '%V %V'.", E_VFP_INVALIDPARAMS, &pParmType, &pParmPrec);
					throw E_APIERROR;
				}
			}
		}
		else
		{
			SaveCustomErrorEx("CreateCallbackFunc", "Invalid parameter type '%V'.", E_VFP_INVALIDPARAMS, &pParmType);
			throw E_APIERROR;
		}

		pParameters = pParameters + (pParm.Len + 1);
	}

	// add ')' and nullterminate
	if (nParmCount)
	{
		pVar = AsmVariable::Var(ReturnReg, 0);
		rasm.Mov(pVar, ')');
		rasm.Add(ReturnReg,1);
		rasm.Mov(pVar, '\0');
	}

	if (nSyncFlag & (CALLBACK_ASYNCRONOUS_POST|CALLBACK_ASYNCRONOUS_SEND))
	{
		/* PostMessage(ghCallbackHwnd,WM_ASYNCCALLBACK,pBuffer,0); */
		if (nSyncFlag & CALLBACK_ASYNCRONOUS_POST)
			rasm.CallConv(stdcall).Call(PostMessage, ghCallbackHwnd, WM_ASYNCCALLBACK, pBufferVar, 0);
		else
			rasm.CallConv(stdcall).Call(SendMessage, ghCallbackHwnd, WM_ASYNCCALLBACK, pBufferVar, 0);
	}
	else if (pRetVal.Len() == 0 || pRetVal.ICompare(__sv("void")))
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
		if (pRetType.ICompare(__sv("integer")) || pRetType.ICompare(__sv("long")))
#else
		if (pRetType.ICompare(__sv("integer")))
#endif
		{
			AsmVariable pVar;
			pVar = rasm.Var<char>(pReturnVar, offsetof(Value, ev_type));
			rasm.Mov(AL, pVar);
			rasm.Cmp(AL, 'N');
			rasm.Je("DoubleConversion");
			pVar = rasm.Var<int>(pReturnVar, offsetof(Value, ev_long));
			rasm.Mov(ReturnReg, pVar);
			rasm.Jmp("End");
			rasm.Label("DoubleConversion");
			pVar = rasm.Var<double>(pReturnVar, offsetof(Value, ev_real));
			rasm.CvttSd2Si(ReturnReg, pVar);
			rasm.Label("End");
		}
		else if (pRetType.ICompare(__sv("uinteger")) || pRetType.ICompare(__sv("ulong")))
		{
			pVar = rasm.Var<char>(pReturnVar, offsetof(Value, ev_type));
			rasm.Mov(AL, pVar);
			rasm.Cmp(AL,'N');
			rasm.Je("DoubleConversion");
			pVar = rasm.Var<unsigned int>(pReturnVar, offsetof(Value, ev_long));
			rasm.Mov(ReturnReg, pVar);
			rasm.Jmp("End");
			rasm.Label("DoubleConversion");
			pVar = rasm.Var<double>(pReturnVar, offsetof(Value, ev_real));
			rasm.CvttSd2Si(ReturnReg, pVar);
			rasm.Label("End");
		}
		else if (pRetType.ICompare(__sv("short")))
		{
			pVar = rasm.Var<int>(pReturnVar, offsetof(Value, ev_long));
			rasm.Mov(AX, pVar);
		}
		else if (pRetType.ICompare(__sv("ushort")))
		{
			pVar = rasm.Var<unsigned int>(pReturnVar, offsetof(Value, ev_long));
			rasm.Mov(AX, pVar);
		}
		else if (pRetType.ICompare(__sv("single")) || pRetType.ICompare(__sv("double")))
		{
#if !defined(_WIN64)
			pVar = rasm.Var<double>(pReturnVar, offsetof(Value, ev_real));
			rasm.Fld(pVar);
#else
			pVar = rasm.Var<double>(pReturnVar, offsetof(Value, ev_real));
			rasm.Mov(XMM0, pVar);
#endif
		}
		else if (pRetType.ICompare(__sv("bool")))
		{
			pVar = rasm.Var<unsigned int>(pReturnVar, offsetof(Value, ev_length));
			rasm.Mov(EAX, pVar);
		}
#if !defined(_WIN64)
		else if (pRetType.ICompare(__sv("int64")))
#else
		else if (pRetType.ICompare(__sv("int64")) || pRetType.ICompare(__sv("long")))
#endif
		{
			pVar = rasm.Var<char>(pReturnVar, offsetof(Value, ev_type));
			rasm.Mov(AL, pVar);
			rasm.Cmp(AL,'N');
			rasm.Je("DoubleConversion");
			pVar = rasm.Var<int>(pReturnVar, offsetof(Value, ev_long));
			rasm.Mov(ReturnReg, pVar);
			rasm.Cdq();
			rasm.Jmp("End");
			rasm.Label("DoubleConversion");
			pVar = rasm.Var<double>(pReturnVar, offsetof(Value, ev_real));
			rasm.CvttSd2Si(ReturnReg, pVar);
			rasm.Label("End");
		}
		else if (pRetType.ICompare(__sv("uint64")))
		{
			pVar = rasm.Var<char>(pReturnVar, offsetof(Value, ev_type));
			rasm.Mov(AL, pVar);
			rasm.Cmp(AL,'N');
			rasm.Je("DoubleConversion");
			pVar = rasm.Var<unsigned int>(pReturnVar, offsetof(Value, ev_long));
			rasm.Mov(ReturnReg, pVar);
#if !defined(_WIN64)
			rasm.Xor(EDX,EDX);
#endif
			rasm.Jmp("End");
			rasm.Label("DoubleConversion");
			pVar = rasm.Var<double>(pReturnVar, offsetof(Value, ev_real));
			rasm.CvttSd2Si(ReturnReg, pVar);
			rasm.Label("End");
		}
		else
		{
			SaveCustomErrorEx("CreateCallbackFunc", "Invalid return value type '%V'.", E_VFP_INVALIDPARAMS, &pRetType);
			throw E_APIERROR;
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
	void* pFunc = parm(1)->Ptr<void*>();
	unsigned int nCount = tls.CallbackFunctions.GetCount();
	for (unsigned int xj = 0; xj < nCount; xj++)
	{
		if (tls.CallbackFunctions[xj]->m_FuncAddress == pFunc)
		{
			CallbackFunction* pFunc = tls.CallbackFunctions[xj];
			delete pFunc;
			tls.CallbackFunctions.Remove(xj);
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