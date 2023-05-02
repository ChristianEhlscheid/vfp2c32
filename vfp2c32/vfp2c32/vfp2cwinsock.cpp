#include <winsock2.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cwinsock.h"
#include "vfp2cutil.h"
#include "vfp2ccppapi.h"

PINET_PTON fpInetPton = (PINET_PTON)0;

void _stdcall SaveWinsockError(char *pFunction)
{
	int nError;
	nError = WSAGetLastError();

	VFP2CTls& tls = VFP2CTls::Tls();
	tls.ErrorCount = 0;
	LPVFP2CERROR pError = tls.ErrorInfo;

	strcpy(pError->aErrorFunction, pFunction);
	pError->nErrorNo = nError;
	pError->nErrorType = VFP2C_ERRORTYPE_WIN32;

	switch(nError)
	{
		case WSANOTINITIALISED:
			strcpy(pError->aErrorMessage,"WSANOTINITIALIZED: Winsock library is not initialized.");
			break;
		case WSAENETDOWN:
			strcpy(pError->aErrorMessage,"WSANETDOWN: Network is down.");
			break;
		case WSAEINPROGRESS:
			strcpy(pError->aErrorMessage,"WSAINPROGRESS: Another blocking socket operation is in progress.");
			break;
		default:
			pError->aErrorMessage[0] = '\0';
	}
}

int _stdcall VFP2C_Init_Winsock()
{
	VFP2CTls& tls = VFP2CTls::Tls();

	WORD wWinsockVer;
	WSADATA wsaData;
	int nError;

	if (tls.WinsockInited == FALSE)
	{
		wWinsockVer = MAKEWORD(2,2);
 		nError = WSAStartup(wWinsockVer,&wsaData);
		if (nError != ERROR_SUCCESS)
		{
			SaveWin32Error("WSAStartup", nError);
			return E_APIERROR;
		}
		tls.WinsockInited = TRUE;
	}
	if (!fpInetPton)
	{
		HMODULE hWSock = GetModuleHandle("ws2_32.dll");
		if (hWSock == NULL)
		{
			SaveWin32Error("GetModuleHandle", GetLastError());
			return E_APIERROR;
		}
		fpInetPton = (PINET_PTON)GetProcAddress(hWSock, "inet_pton");
	}
	return 0;
}

void _stdcall VFP2C_Destroy_Winsock(VFP2CTls& tls)
{
	if (tls.WinsockInited)
		WSACleanup();
}

void _fastcall AIPAddresses(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Winsock();
	if (nErrorNo)
		throw nErrorNo;

	FoxArray pArray(parm(1));
	FoxString pIp(VFP2C_MAX_IP_LEN);

	LPHOSTENT lpHost;
	struct in_addr sInetAdr;
	int nApiRet;
	char aHostname[MAX_PATH];
	
	nApiRet = gethostname(aHostname,MAX_PATH);
	if (nApiRet == SOCKET_ERROR)
	{
		SaveWinsockError("gethostname");
		throw E_APIERROR;
	}

	lpHost = gethostbyname(aHostname);
	if (!lpHost)
	{
		SaveWinsockError("gethostbyname");
		throw E_APIERROR;
	}

	// count number of valid IP's
	unsigned int nCount;
	for (nCount = 0; (lpHost->h_addr_list[nCount]); nCount++);

	if (nCount == 0)
	{
		Return(0);
		return;
	}

	pArray.Dimension(nCount);
	for (unsigned int xj = 0; xj < nCount; xj++)
	{
		memcpy(&sInetAdr,lpHost->h_addr_list[xj],sizeof(int)); 
		pArray(xj+1) = pIp = inet_ntoa(sInetAdr);
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ResolveHostToIp(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Winsock();
	if (nErrorNo)
		throw nErrorNo;

	FoxString pIp(parm(1));
	FoxArray pArray;
	FoxString pBuffer(VFP2C_MAX_IP_LEN);
	LPHOSTENT lpHost = 0;
	struct in_addr sInetAdr;

	lpHost = gethostbyname(pIp);
	// host not found?
	if (!lpHost)
	{
		SaveWinsockError("gethostbyname");
		if (parm.PCount() == 1)
			Return("");
		else
			Return(0);
		return;
	}

	if (parm.PCount() == 1)
	{
		memcpy(&sInetAdr,lpHost->h_addr_list[0],4); 
		pBuffer = inet_ntoa(sInetAdr);
		pBuffer.Return();
	}
	else
	{
		unsigned int nCount;
		for (nCount = 0; (lpHost->h_addr_list[nCount]); nCount++);

		pArray.Dimension(parm(2),nCount);

		unsigned int nRow = 1;
		for (unsigned int xj = 0; xj < nCount; xj++)
		{
			memcpy(&sInetAdr,lpHost->h_addr_list[xj],sizeof(int)); 
			pArray(nRow++) = pBuffer = inet_ntoa(sInetAdr);
		}
		pArray.ReturnRows();
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}
