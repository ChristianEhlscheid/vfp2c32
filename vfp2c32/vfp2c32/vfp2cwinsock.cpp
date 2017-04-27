#include <winsock2.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfpmacros.h"
#include "vfp2cwinsock.h"
#include "vfp2cutil.h"
#include "vfp2ccppapi.h"

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
		wWinsockVer = MAKEWORD(1,1);
 		nError = WSAStartup(wWinsockVer,&wsaData);
		if (nError != ERROR_SUCCESS)
		{
			SaveWin32Error("WSAStartup", nError);
			return E_APIERROR;
		}
		tls.WinsockInited = TRUE;
	}
	return 0;
}

void _stdcall VFP2C_Destroy_Winsock(VFP2CTls& tls)
{
	if (tls.WinsockInited)
		WSACleanup();
}

void _fastcall AIPAddresses(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Winsock();
	if (nErrorNo)
		throw nErrorNo;

	FoxArray pArray(p1);
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

void _fastcall ResolveHostToIp(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Winsock();
	if (nErrorNo)
		throw nErrorNo;

	FoxString pIp(p1);
	FoxArray pArray;
	FoxString pBuffer(VFP2C_MAX_IP_LEN);
	LPHOSTENT lpHost = 0;
	struct in_addr sInetAdr;

	lpHost = gethostbyname(pIp);
	// host not found?
	if (!lpHost)
	{
		SaveWinsockError("gethostbyname");
		if (PCount() == 1)
			Return("");
		else
			Return(0);
		return;
	}

	if (PCount() == 1)
	{
		memcpy(&sInetAdr,lpHost->h_addr_list[0],4); 
		pBuffer = inet_ntoa(sInetAdr);
		pBuffer.Return();
	}
	else
	{
		unsigned int nCount;
		for (nCount = 0; (lpHost->h_addr_list[nCount]); nCount++);

		pArray.Dimension(p2,nCount);

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
