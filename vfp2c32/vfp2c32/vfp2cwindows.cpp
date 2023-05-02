#include <windows.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cwindows.h"
#include "vfp2chelpers.h"
#include "vfp2ccppapi.h"

void _fastcall GetWindowTextEx(ParamBlkEx& parm)
{
try
{
	FoxString pRetVal;
	HWND hHwnd = parm(1)->Ptr<HWND>();
	DWORD nApiRet, nLastError;
	DWORD_PTR nLen;

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

void _fastcall GetWindowRectEx(ParamBlkEx& parm)
{
try
{
	FoxArray pCoords(parm(2), 4, 1);
	RECT sRect;
	HWND hwnd = parm(1)->Ptr<HWND>();

	if (!GetWindowRect(hwnd, &sRect))
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

void _fastcall CenterWindowEx(ParamBlkEx& parm)
{
try
{
	HWND hSource, hParent;
	HMONITOR hMon;
	RECT sSourceRect, sParentRect;
	MONITORINFO sMonInfo;
	int nMonitors, nX, nY;

	hSource = parm(1)->Ptr<HWND>();
	hParent = parm.PCount() == 2 ? parm(2)->Ptr<HWND>() : 0;

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

void _fastcall ADesktopArea(ParamBlkEx& parm)
{
try
{
	FoxArray pCoords(parm(1),4,1);
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

void _fastcall MessageBoxExLib(ParamBlkEx& parm)
{
try
{
	FoxString pText(parm(1));
	FoxString pCaption(parm, 3);
	FoxString pIcon(parm, 5);

	MSGBOXPARAMS sParms = {0};
	sParms.cbSize = sizeof(MSGBOXPARAMS);
	sParms.lpszText = pText;
	sParms.dwStyle = parm.PCount() >= 2 ? parm(2)->ev_long : 0;

	if (parm.PCount() >= 3)
	{
		if (parm(3)->Vartype() == 'C')
			sParms.lpszCaption = pCaption;
		else if (parm(3)->Vartype() != '0')
			throw E_INVALIDPARAMS;
	}

	if (parm.PCount() >= 4)
	{
		if (parm(4)->Vartype() == 'I' || parm(4)->Vartype() == 'N')
			sParms.hwndOwner = parm(4)->DynamicPtr<HWND>();
		else if (parm(4)->Vartype() == '0')
			sParms.hwndOwner = WTopHwnd();
		else
			throw E_INVALIDPARAMS;
	}
	else
		sParms.hwndOwner = WTopHwnd();


	if (parm.PCount() >= 5)
	{
		DWORD dwStyleAdd = MB_USERICON;
		DWORD dwStyleRemove = MB_ICONSTOP | MB_ICONQUESTION | MB_ICONEXCLAMATION | MB_ICONINFORMATION;

		if (parm(5)->Vartype() == 'I')
			sParms.lpszIcon = MAKEINTRESOURCE(parm(5)->ev_long);
		else if (parm(5)->Vartype() == 'N')
			sParms.lpszIcon = MAKEINTRESOURCE(static_cast<DWORD>(parm(5)->ev_real));
		else if (parm(5)->Vartype() == 'C')
			sParms.lpszIcon = pIcon;
		else if (parm(5)->Vartype() == '0')
		{
			dwStyleAdd = 0;
			dwStyleRemove = 0;
		}
		else
			throw E_INVALIDPARAMS;

		sParms.dwStyle |= dwStyleAdd;
		sParms.dwStyle &= ~dwStyleRemove;
	}

	if (parm.PCount() >= 6)
	{	
		if (parm(6)->Vartype() == 'I' || parm(6)->Vartype() == 'N')
			sParms.hInstance = parm(6)->DynamicPtr<HINSTANCE>();
		else if (parm(6)->Vartype() == '0')
		{
			if (sParms.dwStyle & MB_USERICON)
				sParms.hInstance = GetModuleHandle(NULL);
		}
		else
			throw E_INVALIDPARAMS;
	}
	else if (sParms.dwStyle & MB_USERICON)
		sParms.hInstance = GetModuleHandle(NULL);

	if (parm.PCount() >= 7)
	{
		if (parm(7)->Vartype() == 'I')
			sParms.dwContextHelpId = parm(7)->ev_long;
		else if (parm(7)->Vartype() == 'N')
			sParms.dwContextHelpId = static_cast<DWORD>(parm(7)->ev_long);
		else if (parm(7)->Vartype() != '0')
			throw E_INVALIDPARAMS;
	}

	if (parm.PCount() == 8)
		sParms.dwLanguageId = parm(8)->ev_long;
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