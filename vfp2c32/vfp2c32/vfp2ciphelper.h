#ifndef _VFP2CIPHELPER_H__
#define _VFP2CIPHELPER_H__

#include <winternl.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include "vfp2chelpers.h"

// Definition der IP-Optionenstruktur
typedef struct tagIPINFO
{
  BYTE bTimeToLive;    // Time To Live
  BYTE bTypeOfService; // Type Of Service
  BYTE bIpFlags;       // IP-Flags
  BYTE OptSize;        // Size of options buffer
  BYTE *Options;   // pointer to options buffer
} IPINFO, *PIPINFO;

// Definition der ICMP-Echo Antwortstruktur
typedef struct tagICMPECHO
{
  DWORD dwSource;     // destination address
  DWORD dwStatus;     // IP status
  DWORD dwRTTime;     // Round Trip Time in milliseconds
  WORD  wDataSize;    // size of receive buffer
  WORD  wReserved;
  void *pData;			// pointer to receive data
  IPINFO ipInfo;      // receive options
} ICMPECHO, *LPICMPECHO; 

typedef enum tagIpType
{
	IpUnspec = 0,
	IpV4,
	IpV6,
	IpV46,
	IpV64
} IPType;

/*
typedef struct sockaddr_in6 {
	ADDRESS_FAMILY sin6_family;
	USHORT         sin6_port;
	ULONG          sin6_flowinfo;
	IN6_ADDR       sin6_addr;
	union {
		ULONG    sin6_scope_id;
		SCOPE_ID sin6_scope_struct;
	};
} SOCKADDR_IN6_LH, *PSOCKADDR_IN6_LH, *LPSOCKADDR_IN6_LH;
*/

class IcmpFile
{
public:
	IcmpFile();
	~IcmpFile();

	void SetOptions(bool bIp4, BYTE nTTL, BYTE nTos, DWORD nTimeOut, WORD nDataSize, bool bDontFragment);
	bool Ping(IPAddr DestIp, IPAddr SourceIp);
	bool Ping(struct sockaddr_in6* DestIp, struct sockaddr_in6* SourceIp);
	const char* Address();
	int RoundTripTime();
	int Status();
	bool ValidData();

private:
	HANDLE m_Handle;
	DWORD m_TimeOut;
	CBuffer m_Data;
	CBuffer m_Reply;
	IPINFO m_IpInfo;
	LPICMPECHO m_pEcho;
	PICMPV6_ECHO_REPLY_LH m_pEcho6;
#if defined(_WIN64)
	PICMP_ECHO_REPLY32 m_pEcho2;
	IP_OPTION_INFORMATION32 m_IpOptions;
#else
	PICMP_ECHO_REPLY m_pEcho2;
	IP_OPTION_INFORMATION m_IpOptions;
#endif
	char IpBuffer[64];
};

#ifdef __cplusplus
extern "C" {
#endif

typedef DWORD (_stdcall *PSENDARP)(ULONG, ULONG,PULONG, PULONG);
typedef HANDLE (_stdcall *PICMPCREATEFILE)();
typedef HANDLE(_stdcall *PICMP6CREATEFILE)();
typedef BOOL (_stdcall *PICMPCLOSEHANDLE)(HANDLE);
typedef DWORD (_stdcall *PICMPSENDECHO)(HANDLE IcmpHandle, IPAddr DestinationAddress, LPVOID RequestData, WORD RequestSize, PIPINFO RequestOptions, LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);

#if defined(_WIN64)
typedef DWORD (_stdcall *PICMPSENDECHO2)(HANDLE IcmpHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, IPAddr DestinationAddress, LPVOID RequestData, 
	WORD RequestSize, IP_OPTION_INFORMATION32 *RequestOptions, LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);
typedef DWORD(_stdcall* PICMPSENDECHO2EX)(HANDLE IcmpHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, IPAddr SourceAddress, IPAddr DestinationAddress, LPVOID RequestData,
	WORD RequestSize, IP_OPTION_INFORMATION32 *RequestOptions, LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);
typedef DWORD(_stdcall* PICMP6SENDECHO2)(HANDLE IcmpHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, sockaddr_in6* SourceAddress,
	sockaddr_in6* DestinationAddress, LPVOID RequestData, WORD RequestSize, IP_OPTION_INFORMATION32 *RequestOptions, LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);
#else
typedef DWORD(_stdcall* PICMPSENDECHO2)(HANDLE IcmpHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, IPAddr DestinationAddress, LPVOID RequestData,
	WORD RequestSize, PIP_OPTION_INFORMATION RequestOptions, LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);
typedef DWORD(_stdcall* PICMPSENDECHO2EX)(HANDLE IcmpHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, IPAddr SourceAddress, IPAddr DestinationAddress, LPVOID RequestData,
	WORD RequestSize, PIP_OPTION_INFORMATION RequestOptions, LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);
typedef DWORD(_stdcall* PICMP6SENDECHO2)(HANDLE IcmpHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, sockaddr_in6* SourceAddress,
	sockaddr_in6* DestinationAddress, LPVOID RequestData, WORD RequestSize, PIP_OPTION_INFORMATION RequestOptions, LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout);
#endif

const int MAC_ADDRESS_LEN	= 17; // FF:00:FF:00:FF:00

int _stdcall VFP2C_Init_IpHelper();
void _fastcall Ip2MacAddress(ParamBlkEx& parm);
void _fastcall IcmpPing(ParamBlkEx& parm);
void _stdcall Binary2Mac(char *pBuffer, unsigned char *pBinMac);

#ifdef __cplusplus
} // extern C
#endif

#endif // _VFP2CIPHELPER_H__