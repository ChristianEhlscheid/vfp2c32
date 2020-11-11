#include <windows.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2cnetapi.h"
#include "vfp2chelpers.h"
#include "vfp2ccppapi.h"
#include "vfpmacros.h"

static PNETAPIBUFFERALLOCATE fpNetApiBufferAllocate = 0;
static PNETAPIBUFFERFREE fpNetApiBufferFree = 0;
static PNETAPIBUFFERREALLOCATE fpNetApiBufferReallocate = 0;
static PNETAPIBUFFERSIZE fpNetApiBufferSize = 0;
static PNETFILEENUM fpNetFileEnum = 0;
static PNETREMOTETOD fpNetRemoteTOD = 0;
static PNETSERVERENUM fpNetServerEnum = 0;

NetApiBuffer::~NetApiBuffer()
{
	if (m_Buffer)
		fpNetApiBufferFree(m_Buffer);
}

int _stdcall VFP2C_Init_Netapi()
{
	VFP2CTls& tls = VFP2CTls::Tls();
	if (tls.NetApi32)
		return 0;

	tls.NetApi32 = LoadLibrary("netapi32.dll");
	if (!tls.NetApi32)
	{
		SaveWin32Error("LoadLibrary", GetLastError());
		return E_APIERROR;
	}

	fpNetApiBufferAllocate = (PNETAPIBUFFERALLOCATE)GetProcAddress(tls.NetApi32,"NetApiBufferAllocate");
	fpNetApiBufferFree = (PNETAPIBUFFERFREE)GetProcAddress(tls.NetApi32,"NetApiBufferFree");
	fpNetApiBufferReallocate = (PNETAPIBUFFERREALLOCATE)GetProcAddress(tls.NetApi32,"NetApiBufferReallocate");
	fpNetApiBufferSize = (PNETAPIBUFFERSIZE)GetProcAddress(tls.NetApi32,"NetApiBufferSize");
	fpNetRemoteTOD = (PNETREMOTETOD)GetProcAddress(tls.NetApi32,"NetRemoteTOD");
	fpNetFileEnum = (PNETFILEENUM)GetProcAddress(tls.NetApi32,"NetFileEnum");
	fpNetServerEnum = (PNETSERVERENUM)GetProcAddress(tls.NetApi32,"NetServerEnum");
	return 0;
}

void _fastcall ANetFiles(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Netapi();
	if (nErrorNo)
		throw nErrorNo;

	// entry point valid?
	if (fpNetFileEnum == 0)
		throw E_NOENTRYPOINT;

	FoxArray pArray(vp1,1,5);
	FoxWString pServerName(parm,2, '0');
	FoxWString pBasePath(parm,3, '0');
	FoxWString pUserName(parm,4, '0');
	FoxString pNetInfo(NETAPI_INFO_SIZE);
	NetApiBuffer pBuffer;

	DWORD nRow = 1, dwTotal = 0, dwEntries = 0, dwRows = 0;
	DWORD_PTR hResume = 0;
	LPFILE_INFO_3 pFileInfo3;
	NET_API_STATUS nApiRet;

	do 
	{
		nApiRet = fpNetFileEnum(pServerName,pBasePath,pUserName,3,
			pBuffer, NETAPI_BUFFER_SIZE, &dwEntries, &dwTotal, &hResume);
		
		if (nApiRet == NERR_Success || nApiRet == ERROR_MORE_DATA)
		{
			if (dwEntries == 0)
			{
				Return((int)dwRows);
				return;
			}

			dwRows += dwEntries;
			pArray.Dimension(dwRows,5);
			pFileInfo3 = (LPFILE_INFO_3)(LPBYTE)pBuffer;

			while (dwEntries--)
			{
				pArray(nRow,1) = pNetInfo = (LPWSTR)pFileInfo3->fi3_pathname;
				pArray(nRow,2) = pNetInfo = (LPWSTR)pFileInfo3->fi3_username;
				pArray(nRow,3) = (int)pFileInfo3->fi3_id;		
				pArray(nRow,4) = (int)pFileInfo3->fi3_permissions;
				pArray(nRow,5) = (int)pFileInfo3->fi3_num_locks;
				pFileInfo3++;
				nRow++;
			}
		}
		else
		{
			SaveWin32Error("NetFileEnum", nApiRet);
			throw E_APIERROR;
		}
	} while (nApiRet == ERROR_MORE_DATA);

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ANetServers(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Netapi();
	if (nErrorNo)
		throw nErrorNo;

	if (fpNetServerEnum == 0)
		throw E_NOENTRYPOINT;

	FoxArray pArray(vp1);
	DWORD dwServerType = PCount() >= 2 && vp2.ev_long ? vp2.ev_long : SV_TYPE_SERVER;
	DWORD dwLevel = PCount() >= 3 && vp3.ev_long ? (vp3.ev_long == 1 ? 101 : 100) : 101;
	FoxWString pDomain(parm, 4, '0');

	NetApiBuffer pBuffer;
	FoxString pData(NETAPI_INFO_SIZE);

	DWORD nRow = 1, hResume = 0, dwTotal = 0, dwEntries = 0, dwRows = 0;	
    NET_API_STATUS nApiRet;
	LPSERVER_INFO_101 pInfo101;
	LPSERVER_INFO_100 pInfo100;

	pArray.Dimension(1, dwLevel == 101 ? 6 : 2);

	do
	{
		nApiRet = fpNetServerEnum(0, dwLevel, pBuffer, MAX_PREFERRED_LENGTH, &dwEntries, &dwTotal, dwServerType, pDomain, &hResume);
		if (nApiRet == NERR_Success || nApiRet == ERROR_MORE_DATA)
		{
			if (dwEntries == 0)
			{
				Return((int)dwRows);
				return;
			}

			dwRows += dwEntries;

			if (dwLevel == 101)
			{
				pArray.Dimension(dwRows,6);
				pInfo101 = (LPSERVER_INFO_101)(LPBYTE)pBuffer;
				while(dwEntries--)
				{
					pArray(nRow,1) = pInfo101->sv101_platform_id;
					pArray(nRow,2) = pData = (LPWSTR)pInfo101->sv101_name;
					pArray(nRow,3) = pInfo101->sv101_version_major;
					pArray(nRow,4) = pInfo101->sv101_version_minor;
					pArray(nRow,5) = pInfo101->sv101_type;
					pArray(nRow,6) = pData = (LPWSTR)pInfo101->sv101_comment;
					nRow++;
				}
			}
			else
			{
				pArray.Dimension(dwRows,2);
				pInfo100 = (LPSERVER_INFO_100)(LPBYTE)pBuffer;
				while(dwEntries--)
				{
					pArray(nRow,1) = pInfo100->sv100_platform_id;
					pArray(nRow,2) = pData = (LPWSTR)pInfo100->sv100_name;
					nRow++;
				}
			}
		}
		else
		{
			SaveWin32Error("NetServerEnum", nApiRet);
			throw E_APIERROR;
		}
	}
	while(nApiRet == ERROR_MORE_DATA);

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall GetServerTime(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Netapi();
	if (nErrorNo)
		throw nErrorNo;

	if (fpNetRemoteTOD == 0)
		throw E_NOENTRYPOINT;

	FoxWString pServerName(vp1);
	FoxDateTime pTime;
	TimeZone eTimeZone;
	if (PCount() == 2)
	{
		if (vp2.ev_long < 1 || vp2.ev_long > 3)
			throw E_INVALIDPARAMS;
		eTimeZone = static_cast<TimeZone>(vp2.ev_long);
	}
	else
		eTimeZone = UTC;

	NetApiBuffer pBuffer;
	LPTIME_OF_DAY_INFO pServTime;
	NET_API_STATUS nApiRet;

	nApiRet = fpNetRemoteTOD(pServerName,pBuffer);
	if (nApiRet == NERR_Success)
	{
		pServTime = pBuffer;
		pTime = TimeOfDayInfoToDateTime(pServTime, eTimeZone);
		pTime.Return();
	}
	else
	{
		SaveWin32Error("NetRemoteTOD", nApiRet);
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

double _stdcall TimeOfDayInfoToDateTime(LPTIME_OF_DAY_INFO pTimeOfDay, TimeZone eTargetTimeZone)
{
	double dDateTime;
	int lnA, lnY, lnM, lnJDay, lnSeconds;
	lnA = (14 - pTimeOfDay->tod_month) / 12;
	lnY = pTimeOfDay->tod_year + 4800 - lnA;
	lnM = pTimeOfDay->tod_month + 12 * lnA - 3;
	lnJDay = pTimeOfDay->tod_day + (153 * lnM + 2) / 5 + lnY * 365 + lnY / 4 - lnY / 100 + lnY / 400 - 32045;
	lnSeconds = pTimeOfDay->tod_hours * 3600 + pTimeOfDay->tod_mins * 60 + pTimeOfDay->tod_secs;
	dDateTime = ((double)lnJDay) + ((double)lnSeconds / SECONDSPERDAY);
	if (eTargetTimeZone == UTC)
	{
		if (pTimeOfDay->tod_timezone != -1)
			dDateTime -= (((double)pTimeOfDay->tod_timezone * 60) / SECONDSPERDAY);
	}
	else if (eTargetTimeZone == LocalTimeZone)
	{
		if (pTimeOfDay->tod_timezone != -1)
			dDateTime -= (((double)pTimeOfDay->tod_timezone * 60) / SECONDSPERDAY);
		dDateTime -= TimeZoneInfo::GetTsi().Bias;
	}
	return dDateTime;
}