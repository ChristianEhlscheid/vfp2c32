#ifndef _VFP2CWINSOCK_H__
#define _VFP2CWINSOCK_H__

const int DEFAULT_WINSOCK_TIMEOUT	= 4000;
const int VFP2C_MAX_IP_LEN			= 48;

#ifdef __cplusplus
extern "C" {
#endif

typedef long(_stdcall* PINET_ADDR)(const char* cp);
typedef char* (_stdcall* PINET_NTOA)(in_addr in);
typedef hostent* (_stdcall* PGETHOSTBYNAME)(const char* name);

typedef int (_stdcall *PINET_PTON)(int Family, PCTSTR pszAddrString, void* pAddrBuf);
typedef PCSTR (_stdcall *PINET_NTOP)(INT Family, const VOID* pAddr, PSTR pStringBuf, size_t StringBufSize);
typedef int (_stdcall *PGETADDRINFO)(PCSTR pNodeName, PCSTR pServiceName, const ADDRINFOA* pHints, PADDRINFOA* ppResult);

extern PINET_ADDR fpInet_addr;
extern PINET_NTOA fpInet_ntoa;
extern PGETHOSTBYNAME fpGethostbyname;

extern PINET_PTON fpInet_pton;
extern PINET_NTOP fpInet_ntop;
extern PGETADDRINFO fpGetaddrinfo;

void _stdcall SaveWinsockError(const char *pFunction);
int _stdcall VFP2C_Init_Winsock();
void _stdcall VFP2C_Destroy_Winsock(VFP2CTls& tls);

void _fastcall AIPAddresses(ParamBlkEx& parm);
void _fastcall ResolveHostToIp(ParamBlkEx& parm);

#ifdef __cplusplus
}
#endif

#endif // _VFP2CWINSOCK_H__