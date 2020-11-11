#include <windows.h>
#include <math.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfpmacros.h"
#include "vfp2cutil.h"
#include "vfp2ctime.h"
#include "vfp2ccppapi.h"
#include "vfp2chelpers.h"

// Datetime to FILETIME
void _fastcall DT2FT(ParamBlk *parm)
{
	LPFILETIME pFileTime = reinterpret_cast<LPFILETIME>(vp2.ev_long);
	FILETIME sFileTime;

	if (PCount() == 2 || !vp3.ev_length)
	{
		DateTimeToFileTime(&vp1,&sFileTime);
		if (!LocalFileTimeToFileTime(&sFileTime,pFileTime))
			RaiseWin32Error("LocalFileTimeToFileTime", GetLastError());
	}
	else
		DateTimeToFileTime(&vp1,pFileTime);
}

// FILETIME to Datetime
void _fastcall FT2DT(ParamBlk *parm)
{
try
{
	LPFILETIME pFileTime = reinterpret_cast<LPFILETIME>(vp1.ev_long);
	FoxDateTime pTime(*pFileTime);

	if (PCount() == 1 || !vp2.ev_length)
		pTime.ToLocal();

	pTime.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

// DATETIME to SystemTime
void _fastcall DT2ST(ParamBlk *parm)
{
	FILETIME sFileTime, sUTCTime;
	LPSYSTEMTIME pSysTime = reinterpret_cast<LPSYSTEMTIME>(vp2.ev_long);

	DateTimeToFileTime(&vp1,&sFileTime);

	if (PCount() == 2 || !vp3.ev_length)
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
void _fastcall ST2DT(ParamBlk *parm)
{
try
{
	LPSYSTEMTIME pTime = reinterpret_cast<LPSYSTEMTIME>(vp1.ev_long);
	FoxDateTime pDateTime(*pTime);

	if (PCount() == 1 || !vp2.ev_length)
		pDateTime.ToLocal();

	pDateTime.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall DT2UTC(ParamBlk *parm)
{
	FoxDateTime pTime(vp1);
	pTime.ToUTC().Return();
}

void _fastcall UTC2DT(ParamBlk *parm)
{
	FoxDateTime pTime(vp1);
	pTime.ToLocal().Return();
}

void _fastcall DT2Timet(ParamBlk *parm)
{
	// time_t bounds: - don't ask we why they don't have choosen unsigned long to widen the range
	// 0 = 1970/01/01 00:00:00 - Fox = 2440588.0
	// 2147483647 = 2038/01/19 03:14:07 - Fox = 2465443.1348032407
	long nTime_t; // time_t is typedef for long .. so we can use long and don't have to include <time.h>
	double dTime, dDays, dSecs;

	if (PCount() == 0)
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

	if (PCount() == 2 && vp2.ev_length)
	{
		FoxDateTime tmp(vp1);
		tmp.ToUTC();
		dTime = tmp->ev_real;
	}
	else 
		dTime = vp1.ev_real;

	if (dTime < 2440588.0 || dTime > 2465443.1348032407) // bound check
		RaiseError(E_INVALIDPARAMS);

	dSecs = modf(dTime,&dDays);
	dDays -= 2440588.0; // base is 1970/01/01
	dSecs = floor(dSecs * 86400.0 + 0.5); // round seconds
	nTime_t = (long)dDays * 86400 + (long)dSecs;

	Return(nTime_t);
}

void _fastcall Timet2DT(ParamBlk *parm)
{
	DateTimeValue vTime;
	LARGE_INTEGER nFileTime;
	FILETIME ftUTCTime;

	if (vp1.ev_long < 0)
		RaiseError(E_INVALIDPARAMS);

	if (PCount() == 1 || !vp2.ev_length) // convert from UCT/GMT to local time?
	{
		nFileTime.QuadPart = Int32x32To64(vp1.ev_long,10000000) + 116444736000000000;
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
		vTime.ev_real = ((double)(2440588 + vp1.ev_long / 86400));
		vTime.ev_real += ((double)(vp1.ev_long % 86400)) / 86400.0;
	}

	Return(vTime);
}

void _fastcall DT2Double(ParamBlk *parm)
{
	Return(vp1.ev_real);
}

void _fastcall Double2DT(ParamBlk *parm)
{
	FoxDateTime pTime(vp1.ev_real);
	pTime.Return();
}

void _fastcall SetSystemTimeLib(ParamBlk *parm)
{
try
{
	FoxDateTime pTime(vp1);
	SYSTEMTIME sSysTime;

	sSysTime = pTime;

	if (PCount() == 1 || !vp2.ev_length)
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

void _fastcall GetSystemTimeLib(ParamBlk *parm)
{
	SYSTEMTIME sSysTime;
	FoxDateTime pTime;

	if (PCount() == 0 || !vp1.ev_length)
		GetLocalTime(&sSysTime);
	else
		GetSystemTime(&sSysTime);

	pTime = sSysTime;
	pTime.Return();
}

void _fastcall ATimeZones(ParamBlk *parm)
{
try
{
	FoxArray pArray(vp1);
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