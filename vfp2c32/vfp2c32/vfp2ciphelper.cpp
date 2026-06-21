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
static PICMPSENDECHO2 fpIcmpSendEcho2 = 0;
static PICMPSENDECHO2EX fpIcmpSendEcho2Ex = 0;
static PICMP6CREATEFILE fpIcmp6CreateFile = 0;
static PICMP6SENDECHO2 fpIcmp6SendEcho2 = 0;

#pragma warning( disable : 4189)
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
		fpIcmpSendEcho2 = (PICMPSENDECHO2)GetProcAddress(tls.IpHlpApi, "IcmpSendEcho2");
		fpIcmpSendEcho2Ex = (PICMPSENDECHO2EX)GetProcAddress(tls.IpHlpApi, "IcmpSendEcho2Ex");
		fpIcmp6CreateFile = (PICMP6CREATEFILE)GetProcAddress(tls.IpHlpApi, "Icmp6CreateFile");
		fpIcmp6SendEcho2 = (PICMP6SENDECHO2)GetProcAddress(tls.IpHlpApi, "Icmp6SendEcho2");
		if (fpIcmpCreateFile && fpIcmpCloseHandle && (fpIcmpSendEcho || fpIcmpSendEcho2))
			return 0;

		SaveWin32Error("GetProcAddress", GetLastError());
		FreeLibrary(tls.IpHlpApi);
		fpSendARP = 0;
		tls.IpHlpApi = 0;
	}
	else
	{
		// Icmp functions not found, then try to load icmp.dll
		if (tls.Icmp == 0)
			tls.Icmp = LoadLibrary("icmp.dll");

		if (tls.Icmp)
		{
			fpIcmpCreateFile = (PICMPCREATEFILE)GetProcAddress(tls.Icmp, "IcmpCreateFile");
			fpIcmpCloseHandle = (PICMPCLOSEHANDLE)GetProcAddress(tls.Icmp, "IcmpCloseHandle");
			fpIcmpSendEcho = (PICMPSENDECHO)GetProcAddress(tls.Icmp, "IcmpSendEcho");
			fpIcmpSendEcho2 = (PICMPSENDECHO2)GetProcAddress(tls.Icmp, "IcmpSendEcho2");
			fpIcmpSendEcho2Ex = (PICMPSENDECHO2EX)GetProcAddress(tls.Icmp, "IcmpSendEcho2Ex");
			fpIcmp6CreateFile = (PICMP6CREATEFILE)GetProcAddress(tls.Icmp, "Icmp6CreateFile");
			fpIcmp6SendEcho2 = (PICMP6SENDECHO2)GetProcAddress(tls.Icmp, "Icmp6SendEcho2");
			if (fpIcmpCreateFile && fpIcmpCloseHandle && (fpIcmpSendEcho || fpIcmpSendEcho2))
				return 0;

			SaveWin32Error("GetProcAddress", GetLastError());
			FreeLibrary(tls.Icmp);
			tls.Icmp = 0;
		}
		else
		{
			SaveWin32Error("LoadLibrary", GetLastError());
		}
	}

	fpIcmpCreateFile = 0;
	fpIcmpCloseHandle = 0;
	fpIcmpSendEcho = 0;
	fpIcmpSendEcho2 = 0;
	fpIcmpSendEcho2Ex = 0;
	fpIcmp6CreateFile = 0;
	fpIcmp6SendEcho2 = 0;
	return E_APIERROR;
}
#pragma warning( default : 4189)

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

		FoxString vIp(parm, 1);
		FoxString vMac(MAC_ADDRESS_LEN);
		ULONG aMacAddr[2], nLen = 6, nIpAddr;
		DWORD nApiRet;

		if (!fpSendARP || !fpInet_pton)
			throw E_NOENTRYPOINT;

		nIpAddr = fpInet_pton(AF_INET, vIp.Ptr<PCTSTR>(), &nIpAddr);
		if (nIpAddr == INADDR_NONE)
		{
			SaveCustomError("InetPton", "Invalid IP address passed.");
			throw E_APIERROR;
		}

		nApiRet = fpSendARP(nIpAddr, 0, aMacAddr, &nLen);
		if (nApiRet != NO_ERROR)
		{
			SaveWin32Error("SendARP", nApiRet);
			throw E_APIERROR;
		}

		// Convert the binary MAC address into human-readable
		Binary2Mac(vMac.Ptr<char*>(), (unsigned char*)aMacAddr);
		vMac.Len(MAC_ADDRESS_LEN);
		vMac.Return();
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

// NOLINTBEGIN(bugprone-narrowing-conversions)
void _stdcall Binary2Mac(char* pBuffer, unsigned char* pBinMac)
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
// NOLINTEND(bugprone-narrowing-conversions)

void _fastcall IcmpPing(ParamBlkEx& parm)
{
	try
	{
		int nErrorNo = VFP2C_Init_IpHelper();
		if (nErrorNo)
			throw nErrorNo;

		nErrorNo = VFP2C_Init_Winsock();
		if (nErrorNo)
			throw nErrorNo;

		FoxArray pArray(parm(1));
		FoxString pHost(parm(2));
		FoxString pSourceIp(parm, 10);
		FoxString pIpBuffer(VFP2C_MAX_IP_LEN);
		IcmpFile pIcmp;

		BYTE nTTL = parm.PCount() >= 3 && parm(3)->ev_long ? static_cast<BYTE>(parm(3)->ev_long) : 30;
		BYTE nTos = parm.PCount() >= 4 && parm(4)->ev_long ? static_cast<BYTE>(parm(4)->ev_long) : 0;
		DWORD dwTimeout = parm.PCount() >= 5 && parm(5)->ev_long ? parm(5)->ev_long : 3000;
		WORD nDataSize = parm.PCount() >= 6 && parm(6)->ev_long ? static_cast<WORD>(parm(6)->ev_long) : 32;
		bool bDontFragment = parm.PCount() >= 7 && parm(7)->ev_length;
		int nPingCount = parm.PCount() >= 8 && parm(8)->ev_long ? parm(8)->ev_long : 1;
		IPType nIpType = parm.PCount() >= 9 ? (IPType)parm(9)->ev_long : IpUnspec;
		
		IPAddr SourceIp = 0;
		IPAddr DestIp = 0;
		struct sockaddr_in6 *SourceIp6 = 0;
		struct sockaddr_in6* DestIp6 = 0;
		struct in_addr ip4addr; 

		if (fpInet_pton && fpGetaddrinfo)
		{
			if (nIpType == IpUnspec || nIpType == IpV4 || nIpType == IpV46 || nIpType == IpV64)
			{
				// NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
				nErrorNo = fpInet_pton(AF_INET, pHost.Ptr<PCSTR>(), &ip4addr);
				if (nErrorNo == 1)
				{
					DestIp = ip4addr.S_un.S_addr;
				}
				else if (nErrorNo == 0)
				{
					// NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
					nErrorNo = fpInet_pton(AF_INET6, pHost.Ptr<PCSTR>(), &DestIp6);
				}
			}
			else
			{
				// NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
				nErrorNo = fpInet_pton(AF_INET6, pHost.Ptr<PCSTR>(), &DestIp6);
			}

			if (nErrorNo == 0)
			{
				struct addrinfo hints;
				struct addrinfo *addrinfo, *addr;
				ZeroMemory(&hints, sizeof(hints));
				hints.ai_family = nIpType == IpV4 ? AF_INET : (nIpType == IpV6 ? AF_INET6 : AF_UNSPEC);
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_protocol = IPPROTO_TCP;
				nErrorNo = fpGetaddrinfo(pHost.Ptr<PCSTR>(), 0, &hints, &addrinfo);
				if (nErrorNo)
				{
					SaveWinsockError("Getaddrinfo");
					throw E_APIERROR;
				}

				switch (nIpType)
				{
					case IpUnspec:
						for (addr = addrinfo; addr != NULL; addr = addr->ai_next)
						{
							if (addr->ai_family == AF_INET)
							{
								struct sockaddr_in* sockadr = (struct sockaddr_in*)addr->ai_addr;
								DestIp = sockadr->sin_addr.S_un.S_addr;
								break;
							}
							else if (addr->ai_family == AF_INET6)
							{
								DestIp6 = (struct sockaddr_in6*)addr->ai_addr;
								break;
							}
						}
						break;
 
					case IpV4:
						for (addr = addrinfo; addr != NULL; addr = addr->ai_next)
						{
							if (addr->ai_family == AF_INET)
							{
								struct sockaddr_in* sockadr = (struct sockaddr_in*)addr->ai_addr;
								DestIp = sockadr->sin_addr.S_un.S_addr;
								break;
							}
						}
						break;

					case IpV6:
						for (addr = addrinfo; addr != NULL; addr = addr->ai_next)
						{
							if (addr->ai_family == AF_INET6)
							{
								DestIp6 = (struct sockaddr_in6*)addr->ai_addr;
								break;
							}
						}
						break;

					case IpV46:
						for (addr = addrinfo; addr != NULL; addr = addr->ai_next)
						{
							if (addr->ai_family == AF_INET)
							{
								struct sockaddr_in* sockadr = (struct sockaddr_in*)addr->ai_addr;
								DestIp = sockadr->sin_addr.S_un.S_addr;
								break;
							}
						}

						if (DestIp == 0)
						{
							for (addr = addrinfo; addr != NULL; addr = addr->ai_next)
							{
								if (addr->ai_family == AF_INET6)
								{
									DestIp6 = (struct sockaddr_in6*)addr->ai_addr;
									break;
								}
							}
						}
						break;

					case IpV64:
						for (addr = addrinfo; addr != NULL; addr = addr->ai_next)
						{
							if (addr->ai_family == AF_INET6)
							{
								DestIp6 = (struct sockaddr_in6*)addr->ai_addr;
								break;
							}
						}

						if (DestIp6 == 0)
						{
							for (addr = addrinfo; addr != NULL; addr = addr->ai_next)
							{
								if (addr->ai_family == AF_INET)
								{
									struct sockaddr_in* sockadr = (struct sockaddr_in*)addr->ai_addr;
									DestIp = sockadr->sin_addr.S_un.S_addr;
									break;
								}
							}
						}
				}
			}
			else if (nErrorNo != 1)
			{
				SaveWinsockError("inet_pton");
				throw E_APIERROR;
			}
		}
		else if (fpInet_addr && fpGethostbyname)
		{
			DestIp = fpInet_addr(pHost.Ptr<LPCSTR>());
			if (DestIp == INADDR_NONE)
			{
				// Not a dotted address, then do a lookup of the name
				LPHOSTENT lpHostEnt = fpGethostbyname(pHost.Ptr<LPCSTR>());
				if (lpHostEnt)
					DestIp = ((LPIN_ADDR)lpHostEnt->h_addr)->s_addr;
				else
				{
					SaveCustomError("gethostbyname", "Host not found.");
					throw E_APIERROR;
				}
			}
		}

		if (parm.PCount() >= 10)
		{
			if (DestIp)
			{
				// NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
				nErrorNo = fpInet_pton(AF_INET, pSourceIp.Ptr<PCSTR>(), &ip4addr);
				if (nErrorNo == 1)
				{
					SourceIp = ip4addr.S_un.S_addr;
				}
				else
				{
					SaveWinsockError("inet_pton");
					throw E_APIERROR;
				}
			}
			else
			{
				// NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
				nErrorNo = fpInet_pton(AF_INET6, pSourceIp.Ptr<PCSTR>(), &SourceIp6);
				if (nErrorNo != 1)
				{
					SaveWinsockError("inet_pton");
					throw E_APIERROR;
				}
			}
		}

		pArray.Dimension(nPingCount, 4);
		pIcmp.SetOptions(DestIp != 0, nTTL, nTos, dwTimeout, nDataSize, bDontFragment);

		for (int xj = 1; xj <= nPingCount; xj++)
		{
			if (DestIp != 0)
			{
				// NOLINTNEXTLINE(bugprone-narrowing-conversions)
				if (pIcmp.Ping(DestIp, SourceIp))
				{
					pArray(xj, 1) = pIpBuffer = pIcmp.Address();
					pArray(xj, 2) = pIcmp.RoundTripTime();
					pArray(xj, 3) = pIcmp.Status();
					pArray(xj, 4) = pIcmp.ValidData();
				}
				else
				{
					pArray(xj, 1) = pIpBuffer.Len(0);
					pArray(xj, 2) = -1;
					pArray(xj, 3) = -1;
					pArray(xj, 4) = false;
				}
			}
			else
			{
				if (pIcmp.Ping(DestIp6, SourceIp6))
				{
					pArray(xj, 1) = pIpBuffer = pIcmp.Address();
					pArray(xj, 2) = pIcmp.RoundTripTime();
					pArray(xj, 3) = pIcmp.Status();
					pArray(xj, 4) = pIcmp.ValidData();
				}
				else
				{
					pArray(xj, 1) = pIpBuffer.Len(0);
					pArray(xj, 2) = -1;
					pArray(xj, 3) = -1;
					pArray(xj, 4) = false;
				}
			}
		}

		Return(nPingCount);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

IcmpFile::IcmpFile() : m_Handle(INVALID_HANDLE_VALUE), m_TimeOut(0), m_pEcho(0), m_pEcho2(0), m_pEcho6(0)
{
	RtlZeroMemory(&m_IpInfo, sizeof(IPINFO));
#if defined(_WIN64)
	RtlZeroMemory(&m_IpOptions, sizeof(IP_OPTION_INFORMATION32));
#else
	RtlZeroMemory(&m_IpOptions, sizeof(IP_OPTION_INFORMATION));
#endif
}

IcmpFile::~IcmpFile()
{
	if (m_Handle != INVALID_HANDLE_VALUE)
		fpIcmpCloseHandle(m_Handle);
}

void IcmpFile::SetOptions(bool bIp4, BYTE nTTL, BYTE nTos, DWORD nTimeOut, WORD nDataSize, bool bDontFragment)
{
	if (bIp4)
	{
		if (fpIcmpSendEcho2)
		{
			m_IpOptions.Ttl = nTTL;
			m_IpOptions.Tos = nTos;
			if (bDontFragment)
			{
				m_IpOptions.Flags = IP_FLAG_DF;
			}
#if defined(_WIN64)
			m_Reply.Size(sizeof(ICMP_ECHO_REPLY32) + nDataSize + 8);
#else
			m_Reply.Size(sizeof(ICMP_ECHO_REPLY) + nDataSize + 8);
#endif
		}
		else
		{
			m_IpInfo.bTimeToLive = nTTL;
			m_IpInfo.bTypeOfService = nTos;
			if (bDontFragment)
			{
				m_IpInfo.bIpFlags = IP_FLAG_DF;
			}
			m_Reply.Size(sizeof(ICMPECHO) + nDataSize + 8);
		}

		m_Handle = fpIcmpCreateFile();
		if (m_Handle == INVALID_HANDLE_VALUE)
		{
			SaveWin32Error("IcmpCreateFile", GetLastError());
			throw E_APIERROR;
		}
	}
	else
	{
		m_IpOptions.Ttl = nTTL;
		m_IpOptions.Tos = nTos;
		if (bDontFragment)
		{
			m_IpOptions.Flags = IP_FLAG_DF;
		}
		m_Reply.Size(sizeof(ICMPV6_ECHO_REPLY) + nDataSize + 8);
		m_Handle = fpIcmp6CreateFile();
		if (m_Handle == INVALID_HANDLE_VALUE)
		{
			SaveWin32Error("Icmp6CreateFile", GetLastError());
			throw E_APIERROR;
		}
	}

	m_TimeOut = nTimeOut;
	m_Data.Size(nDataSize);
	memset(m_Data.Ptr<void*>(), 'E', nDataSize);
}

bool IcmpFile::Ping(IPAddr DestIp, IPAddr SourceIp)
{
	DWORD nPackets;
	if (SourceIp)
	{
		if (fpIcmpSendEcho2Ex)
		{
			nPackets = fpIcmpSendEcho2Ex(m_Handle, 0, 0, 0, SourceIp, DestIp, m_Data.Ptr<LPVOID>(), m_Data.Size(), &m_IpOptions, m_Reply.Ptr<LPVOID>(), m_Reply.Size(), m_TimeOut);
			if (nPackets > 0)
			{
#if defined(_WIN64)
				m_pEcho2 = m_Reply.Ptr<PICMP_ECHO_REPLY32>();
#else
				m_pEcho2 = m_Reply.Ptr<PICMP_ECHO_REPLY>();
#endif
				return true;
			}
		}
		else
		{
			SaveCustomErrorEx("IcmpPing", "SourceIp parameter specified, but the function 'IcmpSendEcho2Ex' is not supported on this operating system.", E_NOENTRYPOINT);
			throw E_APIERROR;
		}
	}
	else if (fpIcmpSendEcho2)
	{
		nPackets = fpIcmpSendEcho2(m_Handle, 0, 0, 0, DestIp, m_Data.Ptr<LPVOID>(), m_Data.Size(), &m_IpOptions, m_Reply.Ptr<LPVOID>(), m_Reply.Size(), m_TimeOut);
		if (nPackets > 0)
		{
#if defined(_WIN64)
				m_pEcho2 = m_Reply.Ptr<PICMP_ECHO_REPLY32>();
#else
				m_pEcho2 = m_Reply.Ptr<PICMP_ECHO_REPLY>();
#endif
				return true;
		}
	}
	else
	{
		nPackets = fpIcmpSendEcho(m_Handle, DestIp, m_Data.Ptr<LPVOID>(), m_Data.Size(), &m_IpInfo, m_Reply.Ptr<LPVOID>(), m_Reply.Size(), m_TimeOut);
		if (nPackets > 0)
		{
			m_pEcho = m_Reply.Ptr<LPICMPECHO>();
			return true;
		}
	}
	return false;
}

bool IcmpFile::Ping(struct sockaddr_in6 *DestIp, struct sockaddr_in6* SourceIp)
{
	DWORD nPackets;
	struct sockaddr_in6 src;
	if (SourceIp == 0)
	{
		src.sin6_addr = in6addr_any;
		src.sin6_family = AF_INET6;
		src.sin6_flowinfo = 0;
		src.sin6_port = 0;
		SourceIp = &src;
	}
	nPackets = fpIcmp6SendEcho2(m_Handle, 0, 0, 0, SourceIp, DestIp, m_Data.Ptr<LPVOID>(), m_Data.Size(), &m_IpOptions, m_Reply.Ptr<LPVOID>(), m_Reply.Size(), m_TimeOut);
	if (nPackets > 0)
	{
		m_pEcho6 = m_Reply.Ptr<PICMPV6_ECHO_REPLY_LH>();
		return true;
	}
	else
	{
		SaveWin32Error("Icmp6SendEcho2", GetLastError());
	}
	return false;
}

const char* IcmpFile::Address()
{
	in_addr sAddr;
	if (m_pEcho2)
	{
		memcpy(&sAddr, &m_pEcho2->Address, sizeof(DWORD));
		return fpInet_ntoa(sAddr);
	}
	else if (m_pEcho6)
		return fpInet_ntop(AF_INET6, &m_pEcho6->Address.sin6_addr, IpBuffer, sizeof(IpBuffer));
	else if (m_pEcho)
	{
		memcpy(&sAddr, &m_pEcho->dwSource, sizeof(DWORD));
		return fpInet_ntoa(sAddr);
	}
	else
		return "";
}

int IcmpFile::RoundTripTime()
{
	if (m_pEcho2)
		return static_cast<int>(m_pEcho2->RoundTripTime);
	else if (m_pEcho6)
		return static_cast<int>(m_pEcho6->RoundTripTime);
	else if (m_pEcho)
		return static_cast<int>(m_pEcho->dwRTTime);
	else
		return 0;
}

int IcmpFile::Status()
{
	if (m_pEcho2)
		return static_cast<int>(m_pEcho2->Status);
	else if (m_pEcho6)
		return static_cast<int>(m_pEcho6->Status);
	else if (m_pEcho)
		return static_cast<int>(m_pEcho->dwStatus);
	else
		return 0;
}

bool IcmpFile::ValidData()
{
	if (m_pEcho2)
		return memcmp(m_pEcho2->Data, m_Data.Ptr<void*>(), min(m_pEcho2->DataSize, m_Data.Size())) == 0;
	else if (m_pEcho6)
		return true;
	else if (m_pEcho)
		return memcmp(m_pEcho->pData, m_Data.Ptr<void*>(), min(m_pEcho->wDataSize, m_Data.Size())) == 0;
	else
		return false;
}