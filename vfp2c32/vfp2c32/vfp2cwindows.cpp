#include <windows.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2cwindows.h"
#include "vfp2chelpers.h"
#include "vfpmacros.h"
#include "vfp2ccppapi.h"

void _fastcall GetWindowTextEx(ParamBlk *parm)
{
try
{
	FoxString pRetVal;
	HWND hHwnd = reinterpret_cast<HWND>(p1.ev_long);
	DWORD nLen, nApiRet, nLastError;

	nApiRet = SendMessageTimeout(hHwnd,WM_GETTEXTLENGTH,0,0,SMTO_BLOCK,1000,&nLen);
	if (!nApiRet)
	{
		nLastError = GetLastError();
		if (nLastError == NO_ERROR)
		{
			pRetVal.Return();
			return;
		}
		SaveWin32Error("SendMessageTimeout",nLastError);
		throw E_APIERROR;
	}

	nLen += 2;
	pRetVal.Size(nLen);
	nApiRet = SendMessageTimeout(hHwnd,WM_GETTEXT,(WPARAM)nLen,(LPARAM)(char*)pRetVal,SMTO_BLOCK,1000,&nLen);
	if (!nApiRet)
	{
		nLastError = GetLastError();
		if (nLastError != NO_ERROR)
		{
			SaveWin32Error("SendMessageTimeout",nLastError);
			throw E_APIERROR;
		}
	}
	pRetVal.Len(nLen).Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall GetWindowRectEx(ParamBlk *parm)
{
try
{
	FoxArray pCoords(p2, 4, 1);
	RECT sRect;

	if (!GetWindowRect(reinterpret_cast<HWND>(p1.ev_long), &sRect))
	{
		SaveWin32Error("GetWindowRect",GetLastError());
		throw E_APIERROR;
	}

	pCoords(1) = sRect.left;
	pCoords(2) = sRect.right;
	pCoords(3) = sRect.top;
	pCoords(4) = sRect.bottom;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall CenterWindowEx(ParamBlk *parm)
{
try
{
	HWND hSource, hParent;
	HMONITOR hMon;
	RECT sSourceRect, sParentRect;
	MONITORINFO sMonInfo;
	int nMonitors, nX, nY;

	hSource = reinterpret_cast<HWND>(p1.ev_long);
	hParent = PCount() == 2 ? reinterpret_cast<HWND>(p2.ev_long) : 0;

	if (hParent)
	{
		if (!GetWindowRect(hParent, &sParentRect))
		{
			SaveWin32Error("GetWindowRect",GetLastError());
			throw E_APIERROR;
		}
	}
	else
	{
		hParent = GetParent(hSource);
		if (hParent)
		{
			if (!GetWindowRect(hParent, &sParentRect))
			{
				SaveWin32Error("GetWindowRect",GetLastError());
				throw E_APIERROR;
			}
		}
		else
		{
			nMonitors = GetSystemMetrics(SM_CMONITORS);
			if (nMonitors <= 1)
			{
				if (!SystemParametersInfo(SPI_GETWORKAREA,0,reinterpret_cast<void*>(&sParentRect),0))
				{
					SaveWin32Error("SystemParametersInfo",GetLastError());
					throw E_APIERROR;
				}
			}
			else
			{
				hMon = MonitorFromWindow(hSource, MONITOR_DEFAULTTONEAREST);
				sMonInfo.cbSize = sizeof(MONITORINFO);
				
				if (!GetMonitorInfo(hMon, &sMonInfo))
				{
					SaveWin32Error("GetMonitorInfo", GetLastError());						
					throw E_APIERROR;
				}
				
				sParentRect.left = sMonInfo.rcWork.left;
				sParentRect.right = sMonInfo.rcWork.right;
				sParentRect.top = sMonInfo.rcWork.top;
				sParentRect.bottom = sMonInfo.rcWork.bottom;
			}
		}
	}

	if (!GetWindowRect(hSource, &sSourceRect))
	{
		SaveWin32Error("GetWindowRect",GetLastError());
		throw E_APIERROR;
	}

	nX = (sParentRect.left + sParentRect.right) / 2 - (sSourceRect.right - sSourceRect.left) / 2;
	nY = (sParentRect.top + sParentRect.bottom) / 2 - (sSourceRect.bottom - sSourceRect.top) / 2;

	if (!SetWindowPos(hSource,0,nX,nY,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE))
	{
		SaveWin32Error("SetWindowPos",GetLastError());
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ADesktopArea(ParamBlk *parm)
{
try
{
	FoxArray pCoords(p1,4,1);
	RECT sRect;
	
	if (!SystemParametersInfo(SPI_GETWORKAREA,0,(PVOID)&sRect,0))
	{
		SaveWin32Error("SystemParametersInfo",GetLastError());
		throw E_APIERROR;
	}

	pCoords(1) = sRect.left;
	pCoords(2) = sRect.right;
	pCoords(3) = sRect.top;
	pCoords(4) = sRect.bottom;
}
catch (int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall MessageBoxExLib(ParamBlk *parm)
{
try
{
	FoxString pText(p1);
	FoxString pCaption(parm, 3);
	FoxString pIcon(parm, 5);

	MSGBOXPARAMS sParms = {0};
	sParms.cbSize = sizeof(MSGBOXPARAMS);
	sParms.lpszText = pText;
	sParms.dwStyle = PCount() >= 2 ? p2.ev_long : 0;

	if (PCount() >= 3)
	{
		if (Vartype(p3) == 'C')
			sParms.lpszCaption = pCaption;
		else if (Vartype(p3) != '0')
			throw E_INVALIDPARAMS;
	}

	if (PCount() >= 4)
	{
		if (Vartype(p4) == 'I')
			sParms.hwndOwner = reinterpret_cast<HWND>(p4.ev_long);
		else if (Vartype(p4) == 'N')
			sParms.hwndOwner = reinterpret_cast<HWND>(static_cast<DWORD>(p4.ev_real));
		else if (Vartype(p4) == '0')
			sParms.hwndOwner = WTopHwnd();
		else
			throw E_INVALIDPARAMS;
	}
	else
		sParms.hwndOwner = WTopHwnd();


	if (PCount() >= 5)
	{
		DWORD dwStyleAdd = MB_USERICON;
		DWORD dwStyleRemove = MB_ICONSTOP | MB_ICONQUESTION | MB_ICONEXCLAMATION | MB_ICONINFORMATION;

		if (Vartype(p5) == 'I')
			sParms.lpszIcon = MAKEINTRESOURCE(p5.ev_long);
		else if (Vartype(p5) == 'N')
			sParms.lpszIcon = MAKEINTRESOURCE(static_cast<DWORD>(p5.ev_real));
		else if (Vartype(p5) == 'C')
			sParms.lpszIcon = pIcon;
		else if (Vartype(p5) == '0')
		{
			dwStyleAdd = 0;
			dwStyleRemove = 0;
		}
		else
			throw E_INVALIDPARAMS;

		sParms.dwStyle |= dwStyleAdd;
		sParms.dwStyle &= ~dwStyleRemove;
	}

	if (PCount() >= 6)
	{	
		if (Vartype(p6) == 'I')
			sParms.hInstance = reinterpret_cast<HINSTANCE>(p6.ev_long);
		else if (Vartype(p6) == 'N')
			sParms.hInstance = reinterpret_cast<HINSTANCE>(static_cast<DWORD>(p6.ev_long));
		else if (Vartype(p6) == '0')
		{
			if (sParms.dwStyle & MB_USERICON)
				sParms.hInstance = GetModuleHandle(NULL);
		}
		else
			throw E_INVALIDPARAMS;
	}
	else if (sParms.dwStyle & MB_USERICON)
		sParms.hInstance = GetModuleHandle(NULL);

	if (PCount() >= 7)
	{
		if (Vartype(p7) == 'I')
			sParms.dwContextHelpId = p7.ev_long;
		else if (Vartype(p7) == 'N')
			sParms.dwContextHelpId = static_cast<DWORD>(p7.ev_long);
		else if (Vartype(p7) != '0')
			throw E_INVALIDPARAMS;
	}

	if (PCount() == 8)
		sParms.dwLanguageId = p8.ev_long;
	else
		sParms.dwLanguageId = GetUserDefaultUILanguage();

	int nResult = MessageBoxIndirect(&sParms);
	if (nResult == 0)
		throw E_INSUFMEMORY;

	Return(nResult);
}
catch (int nErrorNo)
{
	RaiseError(nErrorNo);
}
}