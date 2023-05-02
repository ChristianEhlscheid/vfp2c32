#ifndef _VFP2CIPHELPER_H__
#define _VFP2CIPHELPER_H__

#include <iphlpapi.h>
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

class IcmpFile
{
public:
	IcmpFile();
	~IcmpFile();

	void SetOptions(BYTE nTTL, BYTE nTos, DWORD nTimeOut, WORD nDataSize, bool bDontFragment);
	bool Ping(long Ip);
	char* Address();
	int RoundTripTime();
	int Status();
	bool ValidData();

private:
	HANDLE m_Handle;
	CBuffer m_Data;
	CBuffer m_Reply;
    LPICMPECHO m_pEcho;
	IPINFO m_IpOptions;
	DWORD m_ReplySize;
	DWORD m_TimeOut;
	WORD m_DataSize;
};

#ifdef __cplusplus
extern "C" {
#endif

typedef DWORD (_stdcall *PSENDARP)(ULONG, ULONG,PULONG, PULONG); // SendArp
typedef HANDLE (_stdcall *PICMPCREATEFILE)(void); // IcmpCreateFile
typedef BOOL (_stdcall *PICMPCLOSEHANDLE)(HANDLE); // IcmpCloseHandle
typedef DWORD (_stdcall *PICMPSENDECHO)(HANDLE, long, LPVOID, WORD, PIPINFO, LPVOID, DWORD, DWORD);

const int MAC_ADDRESS_LEN	= 17; // FF:00:FF:00:FF:00

int _stdcall VFP2C_Init_IpHelper();

void _fastcall Ip2MacAddress(ParamBlkEx& parm);
void _fastcall IcmpPing(ParamBlkEx& parm);

void _stdcall Binary2Mac(char *pBuffer, unsigned char *pBinMac);

#ifdef __cplusplus
} // extern C
#endif

#endif // _VFP2CIPHELPER_H__