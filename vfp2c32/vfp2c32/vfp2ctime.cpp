#include <windows.h>
#include <math.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2ctime.h"
#include "vfp2ccppapi.h"
#include "vfp2chelpers.h"

// Datetime to FILETIME
void _fastcall DT2FT(ParamBlkEx& parm)
{
	LPFILETIME pFileTime = parm(2)->Ptr<LPFILETIME>();
	FILETIME sFileTime;

	if (parm.PCount() == 2 || !parm(3)->ev_length)
	{
		DateTimeToFileTime(parm(1),&sFileTime);
		if (!LocalFileTimeToFileTime(&sFileTime,pFileTime))
			RaiseWin32Error("LocalFileTimeToFileTime", GetLastError());
	}
	else
		DateTimeToFileTime(parm(1),pFileTime);
}

// FILETIME to Datetime
void _fastcall FT2DT(ParamBlkEx& parm)
{
try
{
	LPFILETIME pFileTime = parm(1)->Ptr<LPFILETIME>();
	FoxDateTime pTime(*pFileTime);

	if (parm.PCount() == 1 || !parm(2)->ev_length)
		pTime.ToLocal();

	pTime.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

// DATETIME to SystemTime
void _fastcall DT2ST(ParamBlkEx& parm)
{
	FILETIME sFileTime, sUTCTime;
	LPSYSTEMTIME pSysTime = parm(2)->Ptr<LPSYSTEMTIME>();

	DateTimeToFileTime(parm(1),&sFileTime);

	if (parm.PCount() == 2 || !parm(3)->ev_length)
	{
		if (!LocalFileTimeToFileTime(&sFileTime,&sUTCTime))
			RaiseWin32Error("LocalFileTimeToFileTime", GetLastError());

        if (!FileTimeToSystemTime(&sUTCTime,pSysTime))
			RaiseWin32Error("FileTimeToSystemTime", GetLastError());
	}
	else
	{
		if (!FileTimeToSystemTime(&sFileTime,pSysTime))
			RaiseWin32Error("FileTimeToSystemTime", GetLastError());
	}
}

// SystemTime to DATETIME
void _fastcall ST2DT(ParamBlkEx& parm)
{
try
{
	LPSYSTEMTIME pTime = parm(1)->Ptr<LPSYSTEMTIME>();
	FoxDateTime pDateTime(*pTime);

	if (parm.PCount() == 1 || !parm(2)->ev_length)
		pDateTime.ToLocal();

	pDateTime.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall DT2UTC(ParamBlkEx& parm)
{
	FoxDateTime pTime(parm(1));
	pTime.ToUTC().Return();
}

void _fastcall UTC2DT(ParamBlkEx& parm)
{
	FoxDateTime pTime(parm(1));
	pTime.ToLocal().Return();
}

void _fastcall DT2Timet(ParamBlkEx& parm)
{
	// time_t bounds: - don't ask we why they don't have choosen unsigned long to widen the range
	// 0 = 1970/01/01 00:00:00 - Fox = 2440588.0
	// 2147483647 = 2038/01/19 03:14:07 - Fox = 2465443.1348032407
	long nTime_t; // time_t is typedef for long .. so we can use long and don't have to include <time.h>
	double dTime, dDays, dSecs;

	if (parm.PCount() == 0)
	{
		SYSTEMTIME currentTime, january1st1970;
		ULARGE_INTEGER currentTimeX, january1st1970X;
		GetSystemTime(&currentTime);
		january1st1970.wYear = 1970;
		january1st1970.wMonth = 1;
		january1st1970.wDay = 1;
		january1st1970.wHour = 0;
		january1st1970.wMinute = 0;
		january1st1970.wSecond = 0;
		january1st1970.wMilliseconds = 0;

		if (!SystemTimeToFileTime(&currentTime, (LPFILETIME)&currentTimeX))
			RaiseWin32Error("SystemTimeToFileTime", GetLastError());
		if (!SystemTimeToFileTime(&january1st1970, (LPFILETIME)&january1st1970X))
			RaiseWin32Error("SystemTimeToFileTime", GetLastError());

		currentTimeX.QuadPart -= january1st1970X.QuadPart;
		currentTimeX.QuadPart /= 10000000;
		Return(currentTimeX.LowPart);
		return;
	}

	if (parm.PCount() == 2 && parm(2)->ev_length)
	{
		FoxDateTime tmp(parm(1));
		tmp.ToUTC();
		dTime = tmp->ev_real;
	}
	else 
		dTime = parm(1)->ev_real;

	if (dTime < 2440588.0 || dTime > 2465443.1348032407) // bound check
		RaiseError(E_INVALIDPARAMS);

	dSecs = modf(dTime,&dDays);
	dDays -= 2440588.0; // base is 1970/01/01
	dSecs = floor(dSecs * 86400.0 + 0.5); // round seconds
	nTime_t = (long)dDays * 86400 + (long)dSecs;

	Return(nTime_t);
}

void _fastcall Timet2DT(ParamBlkEx& parm)
{
	ValueEx vTime;
	vTime.SetDateTime();
	LARGE_INTEGER nFileTime;
	FILETIME ftUTCTime;

	if (parm(1)->ev_long < 0)
		RaiseError(E_INVALIDPARAMS);

	if (parm.PCount() == 1 || !parm(2)->ev_length) // convert from UCT/GMT to local time?
	{
		nFileTime.QuadPart = Int32x32To64(parm(1)->ev_long,10000000) + 116444736000000000;
		ftUTCTime.dwLowDateTime = nFileTime.LowPart;
		ftUTCTime.dwHighDateTime = nFileTime.HighPart;
		if (!FileTimeToLocalFileTime(&ftUTCTime,(LPFILETIME)&nFileTime))
			RaiseWin32Error("FileTimeToLocalFileTime", GetLastError());

		nFileTime.QuadPart /= 10000000; // gives us seconds since 1601/01/01
		vTime.ev_real = 2305814.0 + (double)(nFileTime.QuadPart / 86400); // 1601/01/01 + number of seconds / 86400 (= number of days)
		vTime.ev_real += ((double)(nFileTime.QuadPart % 86400)) / 86400;
	}
	else
	{
		vTime.ev_real = ((double)(2440588 + parm(1)->ev_long / 86400));
		vTime.ev_real += ((double)(parm(1)->ev_long % 86400)) / 86400.0;
	}

	Return(vTime);
}

void _fastcall DT2Double(ParamBlkEx& parm)
{
	Return(parm(1)->ev_real);
}

void _fastcall Double2DT(ParamBlkEx& parm)
{
	FoxDateTime pTime(parm(1)->ev_real);
	pTime.Return();
}

void _fastcall SetSystemTimeLib(ParamBlkEx& parm)
{
try
{
	FoxDateTime pTime(parm(1));
	SYSTEMTIME sSysTime;

	sSysTime = pTime;

	if (parm.PCount() == 1 || !parm(2)->ev_length)
	{
		if (!SetLocalTime(&sSysTime))
		{
			SaveWin32Error("SetLocalTime", GetLastError());
			throw E_APIERROR;
		}
	}
	else
	{
		if (!SetSystemTime(&sSysTime))
		{
			SaveWin32Error("SetSystemTime", GetLastError());
			throw E_APIERROR;
		}
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall GetSystemTimeLib(ParamBlkEx& parm)
{
	SYSTEMTIME sSysTime;
	FoxDateTime pTime;

	if (parm.PCount() == 0 || !parm(1)->ev_length)
		GetLocalTime(&sSysTime);
	else
		GetSystemTime(&sSysTime);

	pTime = sSysTime;
	pTime.Return();
}

void _fastcall ATimeZones(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1));
	FoxString pTimeZone(VFP2C_MAX_TIMEZONE_NAME);
	const char* TIMEZONE_REG_KEY = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones";

	RegistryKey hRegKey, hTZKey;
	DWORD nSubKeys, nMaxLen, nLen;
	bool bRet;	
	REG_TIMEZONE_INFORMATION sTZInfo;
	
	hRegKey.Open(HKEY_LOCAL_MACHINE, TIMEZONE_REG_KEY, KEY_ALL_ACCESS);
	hRegKey.QueryInfo(0,0,&nSubKeys);

	pArray.Dimension(nSubKeys,15);

	nLen = VFP2C_MAX_TIMEZONE_NAME;
	bRet = hRegKey.EnumFirstKey(pTimeZone,&nLen);

	unsigned int nRow = 0;
	while(bRet)
	{
		nRow++;
		pArray(nRow,1) = pTimeZone.Len(nLen);

		hTZKey = hRegKey.OpenSubKey(pTimeZone,KEY_QUERY_VALUE);

		nLen = VFP2C_MAX_TIMEZONE_NAME;
		hTZKey.QueryValue("Display",pTimeZone,&nLen);
		pArray(nRow,2) = pTimeZone.Len(nLen);

		nLen = VFP2C_MAX_TIMEZONE_NAME;
		hTZKey.QueryValue("Std",pTimeZone,&nLen);
		pArray(nRow,3) = pTimeZone.Len(nLen);

		nLen = VFP2C_MAX_TIMEZONE_NAME;
		hTZKey.QueryValue("Dlt",pTimeZone,&nLen);
		pArray(nRow,4) = pTimeZone.Len(nLen);

		nMaxLen = sizeof(sTZInfo);
		hTZKey.QueryValue("TZI",(LPBYTE)&sTZInfo,&nMaxLen);

		pArray(nRow,5) = sTZInfo.Bias;
		pArray(nRow,6) = sTZInfo.StandardBias;
		pArray(nRow,7) = sTZInfo.DayligthBias;
		pArray(nRow,8) = sTZInfo.StandardDate.wMonth;
		pArray(nRow,9) = sTZInfo.StandardDate.wDay;
		pArray(nRow,10) = sTZInfo.StandardDate.wDayOfWeek;
		pArray(nRow,11) = sTZInfo.StandardDate.wHour;
		pArray(nRow,12) = sTZInfo.DayligthDate.wMonth;
		pArray(nRow,13) = sTZInfo.DayligthDate.wDay;
		pArray(nRow,14) = sTZInfo.DayligthDate.wDayOfWeek;
		pArray(nRow,15) = sTZInfo.DayligthDate.wHour;

		nLen = VFP2C_MAX_TIMEZONE_NAME;
		bRet = hRegKey.EnumNextKey(pTimeZone,&nLen);
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}