#include <winsock2.h>

#include <ws2ipdef.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cwinsock.h"
#include "vfp2ccppapi.h"

PINET_ADDR fpInet_addr = 0;
PINET_NTOA fpInet_ntoa = 0;
PGETHOSTBYNAME fpGethostbyname = 0;

PINET_PTON fpInet_pton = 0;
PINET_NTOP fpInet_ntop = 0;
PGETADDRINFO fpGetaddrinfo = 0;

void _stdcall SaveWinsockError(const char *pFunction)
{
	int nError = WSAGetLastError();
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

	if (!tls.WinsockInited)
	{
		wWinsockVer = MAKEWORD(2,2);
 		nError = WSAStartup(wWinsockVer,&wsaData);
		if (nError != ERROR_SUCCESS)
		{
			SaveWin32Error("WSAStartup", nError);
			return E_APIERROR;
		}
		tls.WinsockInited = true;
	}

	if (!fpInet_addr && !fpInet_pton)
	{
		HMODULE hDll = GetModuleHandle("ws2_32.dll");
		if (hDll == NULL)
		{
			SaveWin32Error("GetModuleHandle", GetLastError());
			return E_APIERROR;
		}

		fpInet_addr = (PINET_ADDR)GetProcAddress(hDll, "inet_addr");
		fpInet_ntoa = (PINET_NTOA)GetProcAddress(hDll, "inet_ntoa");
		fpGethostbyname = (PGETHOSTBYNAME)GetProcAddress(hDll, "gethostbyname");

		fpInet_pton = (PINET_PTON)GetProcAddress(hDll, "inet_pton");
		fpInet_ntop = (PINET_NTOP)GetProcAddress(hDll, "inet_ntop");
		fpGetaddrinfo = (PGETADDRINFO)GetProcAddress(hDll, "getaddrinfo");
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
	int nIpType = parm.PCount() >= 2 ? parm(2)->ev_long : 1;

	if (nIpType < 1 || nIpType > 3)
		throw E_INVALIDPARAMS;

	int nApiRet;
	unsigned int nCount;
	LPHOSTENT lpHost;
	ADDRINFOA hints, *pInfo;
	struct sockaddr_in* ipv4;
	struct sockaddr_in6* ipv6;
	struct in_addr sInetAdr;
	char aHostname[MAX_PATH];
	
	nApiRet = gethostname(aHostname,MAX_PATH);
	if (nApiRet == SOCKET_ERROR)
	{
		SaveWinsockError("gethostname");
		throw E_APIERROR;
	}

	if (fpGetaddrinfo && fpInet_ntop)
	{
		RtlZeroMemory(&hints, sizeof(hints));
		hints.ai_family = nIpType == 1 ? AF_INET : (nIpType == 2 ? AF_INET6 : AF_UNSPEC);
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		
		nApiRet = fpGetaddrinfo(aHostname, 0, &hints, &pInfo);
		if (nApiRet != 0)
		{
			SaveWinsockError("getaddrinfo");
			throw E_APIERROR;
		}

		nCount = 0;
		for (PADDRINFOA pAdr = pInfo; pAdr != NULL; pAdr = pAdr->ai_next)
		{
			if (pAdr->ai_family == AF_INET || pAdr->ai_family == AF_INET6)
				nCount++;
		}

		if (nCount == 0)
		{
			Return(0);
			return;
		}

		pArray.Dimension(nCount);
		nCount = 0;
		for (PADDRINFOA pAdr = pInfo; pAdr != NULL; pAdr = pAdr->ai_next)
		{
			if (pAdr->ai_family == AF_INET)
			{
				ipv4 = (struct sockaddr_in*)pAdr->ai_addr;
				PCSTR pRet = fpInet_ntop(pAdr->ai_family, &ipv4->sin_addr, pIp.Ptr<PSTR>(), pIp.Size());
				if (pRet == 0)
				{
					SaveWinsockError("inet_ntop");
					throw E_APIERROR;
				}
				pArray(nCount + 1) = pIp.StringLen();
				nCount++;
			}
			else if (pAdr->ai_family == AF_INET6)
			{
				ipv6 = (struct sockaddr_in6*)pAdr->ai_addr;
				PCSTR pRet = fpInet_ntop(pAdr->ai_family, &ipv6->sin6_addr, pIp.Ptr<PSTR>(), pIp.Size());
				if (pRet == 0)
				{
					SaveWinsockError("inet_ntop");
					throw E_APIERROR;
				}
				pArray(nCount + 1) = pIp.StringLen();
				nCount++;
			}
		}
	}
	else if (fpGethostbyname && fpInet_ntoa)
	{
		if (nIpType & 2)
		{
			SaveCustomError("AIPAddresses", "The functions getaddrinfo & inet_ntop are not supported, thus no IPv6 support is available.");
			throw E_APIERROR;
		}

		lpHost = fpGethostbyname(aHostname);
		if (!lpHost)
		{
			SaveWinsockError("gethostbyname");
			throw E_APIERROR;
		}

		for (nCount = 0; (lpHost->h_addr_list[nCount]); nCount++);
		if (nCount == 0)
		{
			Return(0);
			return;
		}

		pArray.Dimension(nCount);
		for (unsigned int xj = 0; xj < nCount; xj++)
		{
			memcpy(&sInetAdr, lpHost->h_addr_list[xj], sizeof(int));
			pArray(xj + 1) = pIp = fpInet_ntoa(sInetAdr);
		}
	}
	else
		throw E_NOENTRYPOINT;

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

	FoxString pHost(parm(1));
	FoxString pIp(VFP2C_MAX_IP_LEN);
	int nIpType = parm.PCount() >= 3 ? parm(3)->ev_long : 1;

	if (nIpType < 1 || nIpType > 3)
		throw E_INVALIDPARAMS;

	int nApiRet;
	LPHOSTENT lpHostEnt = 0;
	ADDRINFOA hints, * pInfo;
	struct sockaddr_in* ipv4;
	struct sockaddr_in6* ipv6;
	struct in_addr sInetAdr;

	if (fpGetaddrinfo && fpInet_ntop)
	{
		RtlZeroMemory(&hints, sizeof(hints));
		hints.ai_family = nIpType == 1 ? AF_INET : (nIpType == 2 ? AF_INET6 : AF_UNSPEC);
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		nApiRet = fpGetaddrinfo(pHost.Ptr<PCSTR>(), 0, &hints, &pInfo);
		if (nApiRet != 0)
		{
			SaveWinsockError("getaddrinfo");
			if (parm.PCount() == 1)
				Return("");
			else
				Return(0);
			return;
		}

		if (parm.PCount() == 1)
		{
			for (PADDRINFOA pAdr = pInfo; pAdr != NULL; pAdr = pAdr->ai_next)
			{
				if (pAdr->ai_family == AF_INET)
				{
					ipv4 = (struct sockaddr_in*)pAdr->ai_addr;
					PCSTR pRet = fpInet_ntop(pAdr->ai_family, &ipv4->sin_addr, pIp.Ptr<PSTR>(), pIp.Size());
					if (pRet == 0)
					{
						SaveWinsockError("inet_ntop");
						throw E_APIERROR;
					}
					pIp.StringLen();
					break;
				}
				else if (pAdr->ai_family == AF_INET6)
				{
					ipv6 = (struct sockaddr_in6*)pAdr->ai_addr;
					PCSTR pRet = fpInet_ntop(pAdr->ai_family, &ipv6->sin6_addr, pIp.Ptr<PSTR>(), pIp.Size());
					if (pRet == 0)
					{
						SaveWinsockError("inet_ntop");
						throw E_APIERROR;
					}
					pIp.StringLen();
					break;
				}
			}

			pIp.Return();
		}
		else
		{
			unsigned int nCount = 0;
			for (PADDRINFOA pAdr = pInfo; pAdr != NULL; pAdr = pAdr->ai_next)
			{
				if ((nIpType & 1) && pAdr->ai_family == AF_INET || (nIpType & 2) && pAdr->ai_family == AF_INET6)
					nCount++;
			}

			FoxArray pArray(parm(2), nCount);
			nCount = 0;
			for (PADDRINFOA pAdr = pInfo; pAdr != NULL; pAdr = pAdr->ai_next)
			{
				if (pAdr->ai_family == AF_INET)
				{
					ipv4 = (struct sockaddr_in*)pAdr->ai_addr;
					PCSTR pRet = fpInet_ntop(pAdr->ai_family, &ipv4->sin_addr, pIp.Ptr<PSTR>(), pIp.Size());
					if (pRet == 0)
					{
						SaveWinsockError("inet_ntop");
						throw E_APIERROR;
					}
					pArray(nCount + 1) = pIp.StringLen();
					nCount++;
				}
				else if (pAdr->ai_family == AF_INET6)
				{
					ipv6 = (struct sockaddr_in6*)pAdr->ai_addr;
					PCSTR pRet = fpInet_ntop(pAdr->ai_family, &ipv6->sin6_addr, pIp.Ptr<PSTR>(), pIp.Size());
					if (pRet == 0)
					{
						SaveWinsockError("inet_ntop");
						throw E_APIERROR;
					}
					pArray(nCount + 1) = pIp.StringLen();
					nCount++;
				}
			}
			pArray.ReturnRows();
		}
	}
	else if (fpGethostbyname && fpInet_ntoa)
	{
		if (nIpType & 2)
		{
			SaveCustomError("ResolveHostToIp", "The functions getaddrinfo & inet_ntop are not supported, thus no IPv6 support is available.");
			throw E_APIERROR;
		}

		lpHostEnt = fpGethostbyname(pHost.Ptr<const char*>());
		// host not found?
		if (!lpHostEnt)
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
			memcpy(&sInetAdr, lpHostEnt->h_addr_list[0], 4);
			pIp = fpInet_ntoa(sInetAdr);
			pIp.Return();
		}
		else
		{
			unsigned int nCount;
			for (nCount = 0; (lpHostEnt->h_addr_list[nCount]); nCount++);

			FoxArray pArray(parm(2), nCount);
			unsigned int nRow = 1;
			for (unsigned int xj = 0; xj < nCount; xj++)
			{
				memcpy(&sInetAdr, lpHostEnt->h_addr_list[xj], sizeof(int));
				pArray(nRow++) = pIp = fpInet_ntoa(sInetAdr);
			}
			pArray.ReturnRows();
		}
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}
