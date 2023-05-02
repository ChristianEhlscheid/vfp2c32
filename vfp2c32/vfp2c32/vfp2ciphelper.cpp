#include <winsock2.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2ciphelper.h"
#include "vfp2cwinsock.h"
#include "vfp2ccppapi.h"
#include "vfp2chelpers.h"

static PSENDARP fpSendARP = 0;
static PICMPCREATEFILE fpIcmpCreateFile = 0;
static PICMPCLOSEHANDLE fpIcmpCloseHandle = 0;
static PICMPSENDECHO fpIcmpSendEcho = 0;

IcmpFile::IcmpFile() : m_Handle(INVALID_HANDLE_VALUE), m_pEcho(0), m_DataSize(0)
{
	m_Handle = fpIcmpCreateFile();
	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		SaveWin32Error("IcmpCreateFile", GetLastError());
		throw E_APIERROR;
	}
	ZeroMemory(&m_IpOptions,sizeof(IPINFO));
}

IcmpFile::~IcmpFile()
{
	if (m_Handle != INVALID_HANDLE_VALUE)
		fpIcmpCloseHandle(m_Handle);
}

void IcmpFile::SetOptions(BYTE nTTL, BYTE nTos, DWORD nTimeOut, WORD nDataSize, bool bDontFragment)
{
	m_IpOptions.bTimeToLive = nTTL;
	m_IpOptions.bTypeOfService = nTos;
	if (bDontFragment)
		m_IpOptions.bIpFlags = IP_FLAG_DF;

	m_TimeOut = nTimeOut;

	m_DataSize = nDataSize;
	m_Data.Size(nDataSize);
	memset(m_Data,'E',nDataSize);

	m_ReplySize = sizeof(ICMPECHO) + max(nDataSize,8);
	m_Reply.Size(m_ReplySize);
}

bool IcmpFile::Ping(long Ip)
{
	DWORD nPackets;
	nPackets = fpIcmpSendEcho(m_Handle,Ip,m_Data,m_DataSize,&m_IpOptions,m_Reply,m_ReplySize,m_TimeOut);
	if (nPackets > 0)
	{
		m_pEcho = reinterpret_cast<LPICMPECHO>(m_Reply.Address());
		return true;
	}
	else
		return false;
}

char* IcmpFile::Address()
{
	in_addr sAddr;
	memcpy(&sAddr,&m_pEcho->dwSource,sizeof(DWORD));
	return inet_ntoa(sAddr);
}

int IcmpFile::RoundTripTime()
{
	return static_cast<int>(m_pEcho->dwRTTime);
}

int IcmpFile::Status()
{
	return static_cast<int>(m_pEcho->dwStatus);
}

bool IcmpFile::ValidData()
{
	return memcmp(m_pEcho->pData, m_Data, min(m_pEcho->wDataSize, m_DataSize)) == 0;
}

int _stdcall VFP2C_Init_IpHelper()
{
	if (fpIcmpCreateFile)
		return 0;

	VFP2CTls& tls = VFP2CTls::Tls();

	if (tls.IpHlpApi == 0)
		tls.IpHlpApi = LoadLibrary("iphlpapi.dll");
	if (tls.IpHlpApi)
	{
		fpSendARP = (PSENDARP)GetProcAddress(tls.IpHlpApi, "SendARP");
		fpIcmpCreateFile = (PICMPCREATEFILE)GetProcAddress(tls.IpHlpApi, "IcmpCreateFile");
		fpIcmpCloseHandle = (PICMPCLOSEHANDLE)GetProcAddress(tls.IpHlpApi, "IcmpCloseHandle");
        fpIcmpSendEcho = (PICMPSENDECHO)GetProcAddress(tls.IpHlpApi, "IcmpSendEcho");
	}
	
	// Icmp functions not found, then try to load icmp.dll
	if (fpIcmpCreateFile == 0)
	{
		if (tls.Icmp == 0)
			tls.Icmp = LoadLibrary("icmp.dll");
		if (tls.Icmp)
		{
			fpIcmpCreateFile = (PICMPCREATEFILE)GetProcAddress(tls.Icmp, "IcmpCreateFile");
			fpIcmpCloseHandle = (PICMPCLOSEHANDLE)GetProcAddress(tls.Icmp, "IcmpCloseHandle");
			fpIcmpSendEcho = (PICMPSENDECHO)GetProcAddress(tls.Icmp, "IcmpSendEcho");
		}
		else
		{
			SaveWin32Error("LoadLibrary", GetLastError());
			return E_APIERROR;
		}
	}

	return 0;
}

void _fastcall Ip2MacAddress(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_IpHelper();
	if (nErrorNo)
		throw nErrorNo;

	nErrorNo = VFP2C_Init_Winsock();
	if (nErrorNo)
		throw nErrorNo;

	FoxString vIp(parm,1);
	FoxString vMac(MAC_ADDRESS_LEN);
	ULONG aMacAddr[2], nLen = 6, nIpAddr;
	HRESULT hr;
	
	if (!fpSendARP || !fpInetPton)
		throw E_NOENTRYPOINT;

	nIpAddr = fpInetPton(AF_INET,vIp,&nIpAddr);
	if (nIpAddr == INADDR_NONE)
	{
		SaveCustomError("inet_addr","Invalid IP address passed.");
		throw E_APIERROR;
	}

	hr = fpSendARP(nIpAddr,0,aMacAddr,&nLen);
	if (hr != NO_ERROR)
	{
		SaveWin32Error("SendARP", hr);
		throw E_APIERROR;
	}

	// Convert the binary MAC address into human-readable
	Binary2Mac(vMac,(unsigned char*)aMacAddr);
	vMac.Len(MAC_ADDRESS_LEN);
	vMac.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _stdcall Binary2Mac(char *pBuffer, unsigned char *pBinMac)
{
	char pHexDigit;

	for (int xj = 0; xj < 5; xj++)
	{
		pHexDigit = (*pBinMac >> 4) + '0';
		if (pHexDigit > '9')
			pHexDigit += 0x7;
		*pBuffer++ = pHexDigit;

		pHexDigit = (*pBinMac & 0xF) + '0';
		if (pHexDigit > '9')
			pHexDigit += 0x7;
		*pBuffer++ = pHexDigit;
		
		*pBuffer++ = ':';
		pBinMac++;
	}
	
	pHexDigit = (*pBinMac >> 4) + '0';
	if (pHexDigit > '9')
		pHexDigit += 0x7;
	*pBuffer++ = pHexDigit;

	pHexDigit = (*pBinMac & 0xF) + '0';
	if (pHexDigit > '9')
		pHexDigit += 0x7;
	*pBuffer++ = pHexDigit;
}

void _fastcall IcmpPing(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_IpHelper();
	if (nErrorNo)
		throw nErrorNo;

	if (!fpIcmpCreateFile)
		throw E_NOENTRYPOINT;

	FoxArray pArray(parm(1));
	FoxString pHost(parm(2));
	FoxString pIpBuffer(VFP2C_MAX_IP_LEN);
	IcmpFile pIcmp;

	BYTE nTTL = parm.PCount() >= 3 && parm(3)->ev_long ? static_cast<BYTE>(parm(3)->ev_long) : 30;
	BYTE nTos = parm.PCount() >= 4 && parm(4)->ev_long ? static_cast<BYTE>(parm(4)->ev_long) : 0;
	DWORD dwTimeout = parm.PCount() >= 5 && parm(5)->ev_long ? parm(5)->ev_long : 3000;
	WORD nDataSize = parm.PCount() >= 6 && parm(6)->ev_long ? static_cast<WORD>(parm(6)->ev_long) : 32;
	bool bDontFragment = parm.PCount() >= 7 && parm(7)->ev_length;
	int nPingCount = parm.PCount() >= 8 && parm(8)->ev_long ? parm(8)->ev_long : 1;

	unsigned long Ip;
	LPHOSTENT lpHostEnt;

	Ip = inet_addr(pHost);
	if (Ip == INADDR_NONE)
	{
		//Not a dotted address, then do a lookup of the name
		lpHostEnt = gethostbyname(pHost);
		if (lpHostEnt)
			Ip = ((LPIN_ADDR)lpHostEnt->h_addr)->s_addr;
		else
		{
			SaveCustomError("gethostbyname","Host not found.");
			throw E_APIERROR;
		}
	}

	pArray.Dimension(nPingCount,4);
	pIcmp.SetOptions(nTTL,nTos, dwTimeout, nDataSize, bDontFragment);

	for (int xj = 1; xj <= nPingCount; xj++)
	{
		if (pIcmp.Ping(Ip))
		{
			pArray(xj,1) = pIpBuffer = pIcmp.Address();
			pArray(xj,2) = pIcmp.RoundTripTime();
			pArray(xj,3) = pIcmp.Status();
			pArray(xj,4) = pIcmp.ValidData();
		}
		else
		{
			pArray(xj,1) = pIpBuffer.Len(0);
			pArray(xj,2) = -1;
			pArray(xj,3) = -1;
			pArray(xj,4) = false;
		}
	}

	Return(nPingCount);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}
