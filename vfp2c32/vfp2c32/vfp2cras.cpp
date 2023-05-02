#include <windows.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2cras.h"
#include "vfp2ccppapi.h"

static PRASGETERRORSTRING fpRasGetErrorString = 0;
static PRASENUMCONNECTIONS fpRasEnumConnections = 0;
static PRASGETPROJECTIONINFO fpRasGetProjectionInfo = 0;
static PRASENUMENTRIES fpRasEnumEntries = 0;
static PRASENUMDEVICES fpRasEnumDevices = 0;
static PRASPHONEBOOKDLG fpRasPhonebookDlg = 0;
static PRASDIAL fpRasDial = 0;
static PRASHANGUP fpRasHangUp = 0;
static PRASGETENTRYDIALPARAMS fpRasGetEntryDialParams = 0;
static PRASGETEAPUSERIDENTITY fpRasGetEapUserIdentity = 0;
static PRASFREEEAPUSERIDENTITY fpRasFreeEapUserIdentity = 0;

#ifndef _THREADSAFE
static PRASCONNECTIONNOTIFICATION fpRasConnectionNotification = 0;
#endif

static PRASGETCONNECTSTATUS fpRasGetConnectStatus = 0;
static PRASDIALDLG fpRasDialDlg = 0;
static PRASCLEARCONNECTIONSTATISTICS fpRasClearConnectionStatistics = 0;

// RAS error handler
void _stdcall SaveRas32Error(char *pFunction, DWORD nErrorNo)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	if (tls.ErrorCount == VFP2C_MAX_ERRORS)
		return;
	tls.ErrorCount++;

	LPVFP2CERROR pError = &tls.ErrorInfo[tls.ErrorCount];
	pError->nErrorType = VFP2C_ERRORTYPE_WIN32;
	pError->nErrorNo = nErrorNo;
	strncpyex(pError->aErrorFunction, pFunction, VFP2C_ERROR_FUNCTION_LEN);
	fpRasGetErrorString(nErrorNo, pError->aErrorMessage, VFP2C_ERROR_MESSAGE_LEN);
}

// initialisation 
int _stdcall VFP2C_Init_Ras()
{
	VFP2CTls& tls = VFP2CTls::Tls();
	if (tls.RasApi32)
		return 0;

	tls.RasApi32 = LoadLibrary("rasapi32.dll");
	if (tls.RasApi32)
	{
		fpRasGetErrorString = (PRASGETERRORSTRING)GetProcAddress(tls.RasApi32, "RasGetErrorStringA");
		fpRasEnumConnections = (PRASENUMCONNECTIONS)GetProcAddress(tls.RasApi32, "RasEnumConnectionsA");
		fpRasGetProjectionInfo = (PRASGETPROJECTIONINFO)GetProcAddress(tls.RasApi32, "RasGetProjectionInfoA");
		fpRasEnumDevices = (PRASENUMDEVICES)GetProcAddress(tls.RasApi32, "RasEnumDevicesA");
		fpRasEnumEntries = (PRASENUMENTRIES)GetProcAddress(tls.RasApi32, "RasEnumEntriesA");
		fpRasDial = (PRASDIAL)GetProcAddress(tls.RasApi32, "RasDialA");
		fpRasHangUp = (PRASHANGUP)GetProcAddress(tls.RasApi32, "RasHangUp");
		fpRasGetEntryDialParams = (PRASGETENTRYDIALPARAMS)GetProcAddress(tls.RasApi32, "RasGetEntryDialParamsA");
		fpRasGetEapUserIdentity = (PRASGETEAPUSERIDENTITY)GetProcAddress(tls.RasApi32, "RasGetEapUserIdentityA");
		fpRasFreeEapUserIdentity = (PRASFREEEAPUSERIDENTITY)GetProcAddress(tls.RasApi32, "RasFreeEapUserIdentityA");
#ifndef _THREADSAFE
		fpRasConnectionNotification = (PRASCONNECTIONNOTIFICATION)GetProcAddress(tls.RasApi32, "RasConnectionNotificationA");
#endif
		fpRasGetConnectStatus = (PRASGETCONNECTSTATUS)GetProcAddress(tls.RasApi32, "RasGetConnectStatusA");
		fpRasDialDlg = (PRASDIALDLG)GetProcAddress(tls.RasApi32, "RasDialDlgA");
		fpRasClearConnectionStatistics = (PRASCLEARCONNECTIONSTATISTICS)GetProcAddress(tls.RasApi32, "RasClearConnectionStatistics");
	}
	else
	{
		SaveWin32Error("LoadLibrary", GetLastError());
		return E_APIERROR;
	}

	if (!tls.RasDlg)
		tls.RasDlg = LoadLibrary("rasdlg.dll");
	if (tls.RasDlg)
	{
		fpRasPhonebookDlg = (PRASPHONEBOOKDLG)GetProcAddress(tls.RasDlg,"RasPhonebookDlgA");
	}
	else
	{
		SaveWin32Error("LoadLibrary", GetLastError());
		return E_APIERROR;
	}

	return 0;
}

// enumerate active dialup connections into an array
void _fastcall ARasConnections(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Ras();
	if (nErrorNo)
		throw nErrorNo;

	if (!fpRasEnumConnections || !fpRasGetProjectionInfo)
		throw E_NOENTRYPOINT;

	FoxArray pArray(parm(1));
	FoxString pEntry(RAS_MaxEntryName);
	RASCONN *lpRas;
	RASPPPIP sRasIp;
	DWORD dwBytes = sizeof(RASCONN) * 32;
	DWORD dwConns, nApiRet;

	CBuffer pBuffer(dwBytes);
	lpRas = reinterpret_cast<LPRASCONN>(pBuffer.Address());
	lpRas->dwSize = sizeof(RASCONN);

	nApiRet = fpRasEnumConnections(lpRas, &dwBytes, &dwConns);
	if (nApiRet == ERROR_BUFFER_TOO_SMALL)
	{
		pBuffer.Size(dwBytes);
		lpRas = reinterpret_cast<LPRASCONN>(pBuffer.Address());
		lpRas->dwSize = sizeof(RASCONN);
		nApiRet = fpRasEnumConnections(lpRas, &dwBytes, &dwConns);
	}

	if (nApiRet == ERROR_SUCCESS)
	{
		if (dwConns == 0)
		{
			Return(0);
			return;
		}

		pArray.Dimension(dwConns, 5);

		for (unsigned int xj = 1; xj <= dwConns; xj++)
		{
			pArray(xj,1) = pEntry = lpRas->szEntryName;
			pArray(xj,2) = pEntry = lpRas->szDeviceName;
			pArray(xj,3) = pEntry = lpRas->szDeviceType;			

			sRasIp.dwSize = sizeof(DWORD) + sizeof(DWORD) + RAS_MaxIpAddress + 1;
			dwBytes = sizeof(RASPPPIP);
			nApiRet = fpRasGetProjectionInfo(lpRas->hrasconn,RASP_PppIp,&sRasIp,&dwBytes);
			if (nApiRet != ERROR_SUCCESS)
			{
				SaveRas32Error("RasGetProjectionInfo", nApiRet);
				throw E_APIERROR;
			}
			pArray(xj,4) = pEntry = sRasIp.szIpAddress;
			pArray(xj,5) = reinterpret_cast<UINT_PTR>(lpRas->hrasconn);
			lpRas++;
		}
	}
	else
	{
		if (nApiRet == ERROR_NOT_ENOUGH_MEMORY)
			SaveWin32Error("RasEnumConnections", nApiRet);
		else
			SaveRas32Error("RasEnumConnections", nApiRet);
		
		throw E_APIERROR;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ARasDevices(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Ras();
	if (nErrorNo)
		throw nErrorNo;

	if (!fpRasEnumDevices)
		throw E_NOENTRYPOINT;

	FoxArray pArray(parm(1));
	FoxString pEntry(RAS_MaxDeviceName+1);

	DWORD dwSize = 1024, dwEntries, nApiRet;
	CBuffer pBuffer(dwSize);
	LPRASDEVINFO lpRasDev = reinterpret_cast<LPRASDEVINFO>(pBuffer.Address());
	lpRasDev->dwSize = sizeof(RASDEVINFO);

	nApiRet = fpRasEnumDevices(lpRasDev,&dwSize,&dwEntries);
	if (nApiRet == ERROR_BUFFER_TOO_SMALL)
	{
		pBuffer.Size(dwSize);
		lpRasDev = reinterpret_cast<LPRASDEVINFO>(pBuffer.Address());
		lpRasDev->dwSize = sizeof(RASDEVINFO);
		nApiRet = fpRasEnumDevices(lpRasDev,&dwSize,&dwEntries);
	}

	if (nApiRet != ERROR_SUCCESS)
	{
		if (nApiRet == ERROR_NOT_ENOUGH_MEMORY)
			SaveWin32Error("RasEnumDevices", nApiRet);
		else
			SaveRas32Error("RasEnumDevices", nApiRet);

		throw E_APIERROR;
	}

	if (dwEntries == 0)
	{
		Return(0);
		return;
	}

	pArray.Dimension(dwEntries,2);
	for (unsigned int xj = 1; xj <= dwEntries; xj++)
	{
		pArray(xj,1) = pEntry = lpRasDev->szDeviceType;
		pArray(xj,2) = pEntry = lpRasDev->szDeviceName;
		lpRasDev++;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}


void _fastcall ARasPhonebookEntries(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Ras();
	if (nErrorNo)
		throw nErrorNo;

	if (!fpRasEnumEntries)
		throw E_NOENTRYPOINT;

	FoxArray pArray(parm(1));
	FoxString pPhonebookFile(parm,2);
	FoxString pEntryName(RAS_MaxEntryName+1);

	DWORD dwSize = 2048, dwEntries, nApiRet;
	CBuffer pBuffer(dwSize);
	LPRASENTRYNAME lpRasEntry = reinterpret_cast<LPRASENTRYNAME>(pBuffer.Address());
	lpRasEntry->dwSize = sizeof(RASENTRYNAME);

	nApiRet = fpRasEnumEntries(0,pPhonebookFile,lpRasEntry,&dwSize,&dwEntries);
	if (nApiRet == ERROR_BUFFER_TOO_SMALL)
	{
		pBuffer.Size(dwSize);
		lpRasEntry = reinterpret_cast<LPRASENTRYNAME>(pBuffer.Address());
		lpRasEntry->dwSize = sizeof(RASENTRYNAME);
		nApiRet = fpRasEnumEntries(0,pPhonebookFile,lpRasEntry,&dwSize,&dwEntries);
	}

	if (nApiRet != ERROR_SUCCESS)
	{
		if (nApiRet == ERROR_NOT_ENOUGH_MEMORY)
			SaveWin32Error("RasEnumEntries", nApiRet);
		else
			SaveRas32Error("RasEnumEntries", nApiRet);

		throw E_APIERROR;
	}

	if (dwEntries == 0)
	{
		Return(0);
		return;
	}

	pArray.Dimension(dwEntries,2);
	for (unsigned int xj = 0; xj < dwEntries; xj++)
	{
		pArray(xj,1) = pEntryName = lpRasEntry->szEntryName;
        pArray(xj,2) = lpRasEntry->dwFlags;
		lpRasEntry++;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

// implementation of RasDlgCallback - for callbacks from the RasDlgPhonebook function
// SetCallback to setup the callback function
void RasPhonebookDlgCallback::SetCallback(CStringView pCallback)
{
	m_Callback.SetCallback(pCallback);
}

// the actual callback function
void _stdcall RasPhonebookDlgCallback::RasPhonebookDlgCallbackFunc(ULONG_PTR dwCallbackId, DWORD dwEvent,
														 LPTSTR pszText, LPVOID pData)
{
	RasPhonebookDlgCallback *pCallback = reinterpret_cast<RasPhonebookDlgCallback*>(dwCallbackId);
	switch(dwEvent)
	{
		case RASPBDEVENT_AddEntry:
		case RASPBDEVENT_EditEntry:
		case RASPBDEVENT_RemoveEntry:
		case RASPBDEVENT_DialEntry:
		case RASPBDEVENT_EditGlobals:
			pCallback->CallbackTxt(dwEvent,pszText);
			break;
		case RASPBDEVENT_NoUser:
		case RASPBDEVENT_NoUserEdit:
			pCallback->Callback(dwEvent,pData);
	}
}

void RasPhonebookDlgCallback::CallbackTxt(DWORD dwEvent, LPTSTR pszText)
{
	int nParm3 = 0;
	m_Callback.Execute(dwEvent, pszText);
}

void RasPhonebookDlgCallback::Callback(DWORD dwEvent, LPVOID pData)
{
	CStringView pParm2 = "";
	m_Callback.Execute(dwEvent, pParm2, pData);
}

void _fastcall RasPhonebookDlgEx(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Ras();
	if (nErrorNo)
		throw nErrorNo;

	if (!fpRasPhonebookDlg)
		throw E_NOENTRYPOINT;

	FoxString pEntry(parm,1);
	FoxString pPhonebookFile(parm,2);
	FoxString pCallback(parm,3);

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
	{
		SaveCustomError("RasPhonebookDlgEx", "Callback function length is greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}

	RASPBDLG sDialog = {0};
	sDialog.dwSize = sizeof(RASPBDLG);
	sDialog.hwndOwner = WTopHwnd();
	sDialog.dwFlags = parm.PCount() >= 4 ? parm(4)->ev_long : 0;

	RasPhonebookDlgCallback sCallback;

	// if callback is used setup callback struct and set necessary RASPBDLG members
	if (pCallback.Len())
	{
		sCallback.SetCallback(pCallback);
		sDialog.pCallback = RasPhonebookDlgCallback::RasPhonebookDlgCallbackFunc;
		sDialog.dwCallbackId = reinterpret_cast<ULONG_PTR>(&sCallback);
	}

	BOOL bRetVal = fpRasPhonebookDlg(pPhonebookFile,pEntry,&sDialog);
	if (!bRetVal)
	{
		// an error occured?
		if (sDialog.dwError)
		{
			SaveRas32Error("RasPhonebookDlg", sDialog.dwError);
			throw E_APIERROR;
		}
		Return(false); // else no connection was established
	}
	else
		Return(true); // connection established

}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void RasDialCallback::SetCallback(CStringView pCallback)
{
	m_Callback.SetCallback(pCallback);
}

DWORD RasDialCallback::Callback(DWORD dwSubEntry, HRASCONN hrasconn,
								UINT unMsg,	RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError)
{
	ValueEx vRetVal;
	vRetVal = 0;
	if (m_Callback.Evaluate(vRetVal, dwSubEntry, hrasconn, unMsg, rascs, dwError, dwExtendedError) == 0)
	{
		if (vRetVal.Vartype() == 'I')
			return vRetVal.ev_long;
		else if (vRetVal.Vartype() == 'N')
			return static_cast<DWORD>(vRetVal.ev_real);
		else if (vRetVal.Vartype() == 'L')
			return vRetVal.ev_length;
		else
			vRetVal.Release();
	}
	return 1;
}

DWORD _stdcall RasDialCallback::RasDialCallbackFunc(ULONG_PTR dwCallbackId, DWORD dwSubEntry, HRASCONN hrasconn,
										UINT unMsg,	RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError)
{

	RasDialCallback *pCall = reinterpret_cast<RasDialCallback*>(dwCallbackId);
	DWORD dwRet = pCall->Callback(dwSubEntry,hrasconn,unMsg,rascs,dwError,dwExtendedError);
	
	if (dwError)
		fpRasHangUp(hrasconn);

	if (dwRet == 0 || unMsg == RASCS_Connected)
		delete pCall;

	return dwRet;
}

void _fastcall RasDialEx(ParamBlkEx& parm)
{
	RASEAPUSERIDENTITY *pUserIdentity = 0;
try
{
	int nErrorNo = VFP2C_Init_Ras();
	if (nErrorNo)
		throw nErrorNo;

	if (!fpRasDial || !fpRasGetEntryDialParams)
		throw E_NOENTRYPOINT;

	// parameter validation
	if (parm(1)->Vartype() != 'C' && parm(1)->Vartype() != 'N' && parm(1)->Vartype() != 'I')
		throw E_INVALIDPARAMS;
	if (parm(2)->Vartype() != 'C' && parm(2)->Vartype() != '0')
		throw E_INVALIDPARAMS;
	if (parm(3)->Vartype() != 'C' && parm(3)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	FoxString pEntry(parm,1);
	FoxString pPhonebookFile(parm,2);
	FoxString pCallback(parm,3);
	HRASCONN hConn = parm.PCount() >= 5 ? parm(5)->Ptr<HRASCONN>() : 0;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
	{
		SaveCustomError("RasPhonebookDlgEx", "Callback function length is greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}

	// local variables
	DWORD nApiRet;

	char *pPhonebook = 0;
	pPhonebook = pPhonebookFile;

	RASDIALPARAMS sDialParams = {0};
	sDialParams.dwSize = sizeof(RASDIALPARAMS);
	LPRASDIALPARAMS pDialParamsPtr = &sDialParams;

	RASDIALEXTENSIONS sDialExtensions = {0};
	sDialExtensions.dwSize = sizeof(RASDIALEXTENSIONS);
	sDialExtensions.dwfOptions = parm.PCount() >= 4 ? parm(4)->ev_long : 0;

	if (pEntry.Len())
	{
		// get RASDIAL parameters
		BOOL bPassword;
		nApiRet = fpRasGetEntryDialParams(pEntry,&sDialParams,&bPassword);
		if (nApiRet != ERROR_SUCCESS)
		{
			SaveRas32Error("RasGetEntryDialParams", nApiRet);
			throw E_APIERROR;
		}

		if (fpRasGetEapUserIdentity && fpRasFreeEapUserIdentity)
		{
			nApiRet = fpRasGetEapUserIdentity(pPhonebook,pEntry,0,0,&pUserIdentity);
			if (nApiRet == ERROR_SUCCESS)
			{
				strcpy(sDialParams.szUserName,pUserIdentity->szUserName);
				sDialExtensions.RasEapInfo.dwSizeofEapInfo = pUserIdentity->dwSizeofEapInfo;
				sDialExtensions.RasEapInfo.pbEapInfo = pUserIdentity->pbEapInfo;
				fpRasFreeEapUserIdentity(pUserIdentity);
			}
			else if (nApiRet == ERROR_INVALID_FUNCTION_FOR_ENTRY);
			else
			{
				SaveRas32Error("RasGetEapUserIdentity", nApiRet);
				throw E_APIERROR;
			}
		}
	}
	else
		pDialParamsPtr = parm(1)->DynamicPtr<LPRASDIALPARAMS>();

	// setup callback parameters
	DWORD dwNotifier = 0; LPVOID lpNotifier = 0;
	if (pCallback.Len())
	{
		dwNotifier = 2;
		lpNotifier = reinterpret_cast<LPVOID>(RasDialCallback::RasDialCallbackFunc);

		RasDialCallback *pRasCallback = new RasDialCallback;
		if (!pRasCallback)
			throw E_INSUFMEMORY;
			
		try
		{
			pRasCallback->SetCallback(pCallback);
			pDialParamsPtr->dwCallbackId = reinterpret_cast<ULONG_PTR>(pRasCallback);
		}
		catch(int nErrorNo)
		{
			delete pRasCallback;
			throw nErrorNo;
		}
	}

	nApiRet = fpRasDial(&sDialExtensions,pPhonebook,&sDialParams,dwNotifier,lpNotifier,&hConn);
	if (nApiRet != ERROR_SUCCESS)
	{
		SaveRas32Error("RasDial", nApiRet);
		
		// call RasHangUp to cleanup the RAS handle
		if (hConn && !pCallback.Len())
		{
			RASCONNSTATUS sStatus;
			sStatus.dwSize = sizeof(RASCONNSTATUS);
			nApiRet = fpRasGetConnectStatus(hConn,&sStatus);
			if (nApiRet != ERROR_INVALID_HANDLE)
			{
				fpRasHangUp(hConn);
			
				while(true)
				{
					Sleep(50);
					nApiRet = fpRasGetConnectStatus(hConn,&sStatus);
					if (nApiRet == ERROR_INVALID_HANDLE)
						break;
					else if (nApiRet == ERROR_SUCCESS);
					else
						break;
				}
			}
		}
		throw E_APIERROR;
	}

	if (pUserIdentity)
		fpRasFreeEapUserIdentity(pUserIdentity);

	Return(hConn);
}
catch(int nErrorNo)
{
	if (pUserIdentity)
		fpRasFreeEapUserIdentity(pUserIdentity);

	RaiseError(nErrorNo);
}
}

void _fastcall RasDialDlgEx(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Ras();
	if (nErrorNo)
		throw nErrorNo;

	if (!fpRasDialDlg)
		throw E_NOENTRYPOINT;

	FoxString pPhonebook(parm,1);
	FoxString pEntry(parm,2);
	FoxString pPhonenumber(parm,3);

	RASDIALDLG sDialog = {0};
	sDialog.dwSize = sizeof(RASDIALDLG);
	if (parm.PCount() >= 4)
		sDialog.dwSubEntry = parm(4)->ev_long;
	if (parm.PCount() >= 5)
		sDialog.hwndOwner = parm(5)->Ptr<HWND>();

	BOOL bRetVal = fpRasDialDlg(pPhonebook,pEntry,pPhonenumber,&sDialog);
	if (!bRetVal)
	{
		if (sDialog.dwError)
		{
			SaveRas32Error("RasDialDlg", sDialog.dwError);
			throw E_APIERROR;
		}
		Return(false);
	}
	else
		Return(true);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall RasHangUpEx(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Ras();
	if (nErrorNo)
		throw nErrorNo;

	if (!fpRasHangUp || !fpRasGetConnectStatus)
		throw E_NOENTRYPOINT;

	HRASCONN hConn = parm(1)->Ptr<HRASCONN>();
	
	DWORD nApiRet = fpRasHangUp(hConn);
	if (nApiRet != ERROR_SUCCESS)
	{
		SaveRas32Error("RasHangUp", nApiRet);
		throw E_APIERROR;
	}

	RASCONNSTATUS sStatus;
	sStatus.dwSize = sizeof(RASCONNSTATUS);
	while(true)
	{
        nApiRet = fpRasGetConnectStatus(hConn,&sStatus);
		if (nApiRet == ERROR_INVALID_HANDLE)
			break;
		else if (nApiRet != ERROR_SUCCESS)
		{
			if (nApiRet == ERROR_BUFFER_TOO_SMALL || nApiRet == ERROR_NOT_ENOUGH_MEMORY)
				SaveWin32Error("RasGetConnectStatus", nApiRet);
			else
		    	SaveRas32Error("RasGetConnectStatus", nApiRet);
			throw E_APIERROR;
		}
		else
			Sleep(50);
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall RasGetConnectStatusEx(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Ras();
	if (nErrorNo)
		throw nErrorNo;

	if (!fpRasGetConnectStatus)
		throw E_NOENTRYPOINT;

	HRASCONN hConn = parm(1)->Ptr<HRASCONN>();
	FoxArray pArray(parm(2),5,1);

	FoxString pValue(RAS_MaxPhoneNumber+1);
	RASCONNSTATUS sStatus = {0};
	sStatus.dwSize = sizeof(RASCONNSTATUS);

	DWORD nApiRet = fpRasGetConnectStatus(hConn,&sStatus);
	if (nApiRet != ERROR_SUCCESS)
	{
		if (nApiRet == ERROR_BUFFER_TOO_SMALL || nApiRet == ERROR_NOT_ENOUGH_MEMORY)
			SaveWin32Error("RasGetConnectStatus", nApiRet);
		else
        	SaveRas32Error("RasGetConnectStatus", nApiRet);
		throw E_APIERROR;
	}

	pArray(1) = static_cast<int>(sStatus.rasconnstate);
	pArray(2) = sStatus.dwError;
	pArray(3) = pValue = sStatus.szDeviceType;
	pArray(4) = pValue = sStatus.szDeviceName;
	pArray(5) = pValue = sStatus.szPhoneNumber;

}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

#ifndef _THREADSAFE
void RasNotifyThread::SignalThreadAbort()
{
	m_AbortEvent.Signal();
}

void RasNotifyThread::Release()
{
	delete this;
}

bool RasNotifyThread::Setup(HRASCONN hConn, DWORD dwFlags, FoxString &pCallback)
{
	m_Conn = hConn;
	m_Flags = dwFlags;
	m_Callback.SetCallback(pCallback);

	if (!m_RasEvent.Create(false))
		return false;

	if (!m_AbortEvent.Create())
		return false;

	return true;
}

DWORD RasNotifyThread::Run()
{
	DWORD nApiRet;
	HANDLE hEvents[2];
	hEvents[0] = m_RasEvent;
	hEvents[1] = m_AbortEvent;

	while (true)
	{
		nApiRet = fpRasConnectionNotification(m_Conn, m_RasEvent, m_Flags);
		if (nApiRet)
		{
			m_Callback.AsyncExecute(ghAsyncHwnd, WM_CALLBACK, m_Conn, nApiRet);
			return 0;
		}

        nApiRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		switch(nApiRet)
		{
			case WAIT_OBJECT_0:
				m_Callback.AsyncExecute(ghAsyncHwnd, WM_CALLBACK, m_Conn, 0);
				if (m_Conn != INVALID_HANDLE_VALUE)
					return 0;
				else
					break;
			
			case WAIT_OBJECT_0 + 1:
				return 0;
			
			default:
				m_Callback.AsyncExecute(ghAsyncHwnd, WM_CALLBACK, m_Conn, nApiRet);
				return 0;
		}
	}
}

void _fastcall RasConnectionNotificationEx(ParamBlkEx& parm)
{
	RasNotifyThread *pNotifyThread = 0;
try
{
	int nErrorNo = VFP2C_Init_Ras();
	if (nErrorNo)
		throw nErrorNo;

	if (!fpRasConnectionNotification)
		throw E_NOENTRYPOINT;

	HRASCONN hConn = parm(1)->Ptr<HRASCONN>();
	DWORD dwFlags = parm(2)->ev_long;
	FoxString pCallback(parm(3));

	if (!goThreadManager.Initialized())
	{
		SaveCustomError("RasConnectionNotificationEx","Library not initialized.");
		throw E_APIERROR;
	}

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION || pCallback.Len() == 0)
	{
		SaveCustomError("RasConnectionNotificationEx", "Callback function length is zero or greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}

	RasNotifyThread *pNotifyThread = new RasNotifyThread(goThreadManager);
	if (!pNotifyThread)
		throw E_INSUFMEMORY;

	if (!pNotifyThread->Setup(hConn,dwFlags,pCallback))
		throw E_APIERROR;
	
	pNotifyThread->StartThread();
    
	Return(pNotifyThread);
}
catch(int nErrorNo)
{
	if (pNotifyThread)
		delete pNotifyThread;

	RaiseError(nErrorNo);
}
}

void _fastcall AbortRasConnectionNotificationEx(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Ras();
	if (nErrorNo)
		throw nErrorNo;

	CThread *pThread = parm(1)->Ptr<CThread*>();
	Return(goThreadManager.AbortThread(pThread));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}
#endif

void _fastcall RasClearConnectionStatisticsEx(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Ras();
	if (nErrorNo)
		throw nErrorNo;

	if (!fpRasClearConnectionStatistics)
		throw E_NOENTRYPOINT;

	HRASCONN hConn = parm(1)->Ptr<HRASCONN>();
	DWORD nApiRet = fpRasClearConnectionStatistics(hConn);
	if (nApiRet != ERROR_SUCCESS)
	{
		SaveWin32Error("RasClearConnectionStatistics", nApiRet);
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}