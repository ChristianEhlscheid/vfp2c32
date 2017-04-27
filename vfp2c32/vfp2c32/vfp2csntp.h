#ifndef _VFP2CSNTP_H__
#define _VFP2CSNTP_H__

#define JAN_1ST_1900	2415021
#define SNTP_PORT		123
#define FRACTION_TO_MS ((double)1000.0/0xFFFFFFFF)
#define MS_TO_FRACTION (((double)0xFFFFFFFF) / 1000.0)

#define SNTP_LEAP_OFFSET		6
#define SNTP_LEAP_NOWARN		(0 << SNTP_LEAP_OFFSET)
#define SNTP_LEAP_LASTMIN61		(1 << SNTP_LEAP_OFFSET)
#define SNTP_LEAP_LASTMIN59		(2 << SNTP_LEAP_OFFSET)
#define SNTP_GET_LEAP(sByte)	(sByte >> SNTP_LEAP_OFFSET)
#define SNTP_VERSION_OFFSET		3
#define SNTP_GET_VERSION(sByte)	(sByte & 0x38 >> SNTP_VERSION_OFFSET)
#define SNTP_SET_VERSION(nVer)	(nVer << SNTP_VERSION_OFFSET)
#define SNTP_GET_MODE(sByte)	(sByte & 0x7)
#define SNTP_MODE_SYMACTIVE		1
#define SNTP_MODE_SYMPASSIVE	2
#define SNTP_MODE_CLIENT		3
#define SNTP_MODE_SERVER		4
#define SNTP_MODE_BROADCAST		5

typedef struct _SNTPTIME
{
	DWORD dwSeconds;
	DWORD dwFraction;
} SNTPTIME, *LPSNTPTIME;

typedef struct _SNTPRESPONSE
{
  BYTE LiVnMode;
  BYTE Stratum;
  char Poll;
  char Precision;
  long RootDelay;
  long RootDispersion;
  char ReferenceID[4];
  SNTPTIME ReferenceTime;
  SNTPTIME OriginateTime;
  SNTPTIME ReceiveTime;
  SNTPTIME TransmitTime;
  unsigned long KeyID;
  BYTE MessageDigest[16];
} SNTPRESPONSE, *LPSNTPRESPONSE;

typedef struct _SNTPPACKET
{
  BYTE LiVnMode;
  BYTE Stratum;
  char Poll;
  char Precision;
  long RootDelay;
  long RootDispersion;
  char ReferenceID[4];
  SNTPTIME ReferenceTime;
  SNTPTIME OriginateTime;
  SNTPTIME ReceiveTime;
  SNTPTIME TransmitTime;
} SNTPPACKET, *LPSNTPPACKET;

#ifdef __cplusplus
extern "C" {
#endif

void _fastcall SyncToSNTPServer(ParamBlk *parm);
void _stdcall SystemTimeToSNTPTime(LPSYSTEMTIME pSysTime, LPSNTPTIME pSntpTime);
void _stdcall SNTPTimeToSystemTime(LPSNTPTIME pSntpTime, LPSYSTEMTIME pSysTime);
double _stdcall SNTPTimeToDouble(LPSNTPTIME pSntpTime);
void _stdcall DoubleToSNTPTime(double nTime, LPSNTPTIME pSntpTime);
void _stdcall SNTPTimeToPacket(LPSNTPTIME pSntpTime);
void _stdcall PacketToSNTPTime(LPSNTPTIME pSntpTime);

#ifdef __cplusplus
}
#endif

#endif // _VFP2CSNTP_H__