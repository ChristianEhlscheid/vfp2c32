#include <windows.h>
#include <stdio.h>

#include <stdarg.h>
#include <math.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2cstring.h"
#include "vfp2ctime.h"

// some helper functions for common tasks

//converts a filetime value to a datetime value .. milliseconds are truncated ..
void _stdcall FileTimeToDateTime(LPFILETIME pFileTime, double& pDateTime)
{
	// FILETIME base: Januar 1 1601 | C = 0 | FoxPro = 2305814.0
	// 86400 secs a day, 10000000 "100 nanosecond intervals" in one second
    LARGE_INTEGER sTime;
	sTime.LowPart = pFileTime->dwLowDateTime;
	sTime.HighPart = pFileTime->dwHighDateTime;

	if (sTime.QuadPart > MAXVFPFILETIME) //if bigger than max DATETIME - 9999/12/12 23:59:59
		sTime.QuadPart = MAXVFPFILETIME; //set to max date ..
	else if (sTime.QuadPart == 0) // empty Filetime?
	{
		pDateTime = 0.0;
		return;
	}

	sTime.QuadPart /= NANOINTERVALSPERSECOND; // gives us seconds since 1601/01/01
	pDateTime = VFPFILETIMEBASE + (double)(sTime.QuadPart / SECONDSPERDAY); // 1601/01/01 + number of seconds / 86400 (= number of days)
	pDateTime += ((double)(sTime.QuadPart % SECONDSPERDAY)) / SECONDSPERDAY; 
}

void _stdcall DateTimeToFileTime(double pDateTime, LPFILETIME pFileTime)
{
	LARGE_INTEGER nFileTime;
	double dDays, dSecs, dDateTime;

	if (pDateTime >= VFPFILETIMEBASE)
		dDateTime = pDateTime;
	else if (pDateTime == 0.0) // if empty date .. set filetime to zero
	{
		pFileTime->dwLowDateTime = 0;
		pFileTime->dwHighDateTime = 0;
		return;
	}
	else
		dDateTime = VFPFILETIMEBASE; // if before 1601/01/01 00:00:00 set to 1601/01/01 ..

	dSecs = modf(dDateTime,&dDays); // get absolute value and fractional part
	dSecs = floor(dSecs * SECONDSPERDAY + 0.1);
	// cause double arithmetic isn't 100% accurate we have to round down to the nearest integer value (with floor function)
	// + 0.1 cause we may get for example 34.9999899 after 0.xxxx * SECONDSPERDAY, which really stands for 35 seconds after midnigth
	nFileTime.QuadPart = ((LONGLONG)(dDays - VFPFILETIMEBASE)) * NANOINTERVALSPERDAY + ((LONGLONG)dSecs) * NANOINTERVALSPERSECOND;
	pFileTime->dwLowDateTime = nFileTime.LowPart;
	pFileTime->dwHighDateTime = nFileTime.HighPart;
}

BOOL _stdcall FileTimeToDateTime(LPFILETIME pFileTime, double& pDateTime, BOOL bToLocal)
{
	FILETIME sFileTime;
	if (bToLocal)
	{
		if (!FileTimeToLocalFileTime(pFileTime,&sFileTime))
		{
			SaveWin32Error("FileTimeToLocalFileTime",GetLastError());
			return FALSE;
		}
		FileTimeToDateTime(&sFileTime,pDateTime);
	}
	else
		FileTimeToDateTime(pFileTime,pDateTime);

	return TRUE;
}

BOOL _stdcall DateTimeToFileTime(double pDateTime, LPFILETIME pFileTime, BOOL bToUTC)
{
	FILETIME sTime;
	if (bToUTC)
	{
		DateTimeToFileTime(pDateTime, &sTime);
		if (!LocalFileTimeToFileTime(&sTime,pFileTime))
		{
			SaveWin32Error("LocalFileTimeToFileTime",GetLastError());
			return FALSE;
		}
	}
	else
		DateTimeToFileTime(pDateTime,pFileTime);

	return TRUE;
}

void _stdcall DateTimeToSystemTime(double pDateTime, LPSYSTEMTIME pSysTime)
{
	int lnA, lnB, lnC, lnD, lnE, lnM;
	DWORD lnDays, lnSecs;
	double dDays, dSecs;

	dSecs = modf(pDateTime, &dDays);
	lnDays = static_cast<DWORD>(dDays);

	lnA = lnDays + 32044;
	lnB = (4 * lnA + 3) / 146097;
	lnC = lnA - (lnB * 146097) / 4;

	lnD = (4 * lnC + 3) / 1461;
	lnE = lnC - (1461 * lnD) / 4;
	lnM = (5 * lnE + 2) / 153;
	
	pSysTime->wDay = (WORD) lnE - (153 * lnM + 2) / 5 + 1;
	pSysTime->wMonth = (WORD) lnM + 3 - 12 * (lnM / 10);
	pSysTime->wYear = (WORD) lnB * 100 + lnD - 4800 + lnM / 10;

	lnSecs = (int)floor(dSecs * 86400.0 + 0.1);
	pSysTime->wHour = (WORD)lnSecs / 3600;
	lnSecs %= 3600;
	pSysTime->wMinute = (WORD)lnSecs / 60;
	lnSecs %= 60;
	pSysTime->wSecond = (WORD)lnSecs;

	pSysTime->wDayOfWeek = (WORD)((lnDays + 1) % 7);
	pSysTime->wMilliseconds = 0; // FoxPro's datetime doesn't have milliseconds .. so just set to zero
}

void _stdcall SystemTimeToDateTime(LPSYSTEMTIME pSysTime, double& pDateTime)
{
	int lnA, lnY, lnM, lnJDay;
	lnA = (14 - pSysTime->wMonth) / 12;
	lnY = pSysTime->wYear + 4800 - lnA;
	lnM = pSysTime->wMonth + 12 * lnA - 3;
	lnJDay = pSysTime->wDay + (153 * lnM + 2) / 5 + lnY * 365 + lnY / 4 - lnY / 100 + lnY / 400 - 32045;
	pDateTime = ((double)lnJDay) + (((double)(pSysTime->wHour * 3600 + pSysTime->wMinute * 60 + pSysTime->wSecond)) / 86400);
}

BOOL _stdcall SystemTimeToDateTime(LPSYSTEMTIME pSysTime, double& pDateTime, BOOL bToLocal)
{
	FILETIME sUTCTime, sFileTime;
	if (!SystemTimeToFileTime(pSysTime,&sUTCTime))
	{
		SaveWin32Error("SystemTimeToFileTime",GetLastError());
		return FALSE;
	}

    if (bToLocal)
	{
		if (!FileTimeToLocalFileTime(&sUTCTime,&sFileTime))
		{
			SaveWin32Error("FileTimeToLocalFileTime",GetLastError());
			return FALSE;
		}
		FileTimeToDateTime(&sFileTime,pDateTime);
	}
	else
		FileTimeToDateTime(&sUTCTime,pDateTime);

	return TRUE;
}

BOOL _stdcall DateTimeToSystemTime(double pDateTime, LPSYSTEMTIME pSysTime, BOOL bToLocal)
{
	FILETIME sFileTime, sUTCTime;

	DateTimeToFileTime(pDateTime, &sFileTime);

	if (bToLocal)
	{
		if (!LocalFileTimeToFileTime(&sFileTime,&sUTCTime))
		{
			SaveWin32Error("LocalFileTimeToFileTime", GetLastError());
			return FALSE;
		}
        if (!FileTimeToSystemTime(&sUTCTime,pSysTime))
		{
            SaveWin32Error("FileTimeToSystemTime", GetLastError());
			return FALSE;
		}
	}
	else
	{
		if (!FileTimeToSystemTime(&sFileTime,pSysTime))
		{
			SaveWin32Error("FileTimeToSystemTime", GetLastError());
			return FALSE;
		}
	}
	return TRUE;
}

BOOL _stdcall FileTimeToDateLiteral(LPFILETIME pFileTime, char *pBuffer, BOOL bToLocal)
{
	SYSTEMTIME sSysTime;
	FILETIME sFileTime;

	if (bToLocal)
	{
		if (!FileTimeToLocalFileTime(pFileTime,&sFileTime))
		{
			SaveWin32Error("FileTimeToLocalFileTime", GetLastError());
			return FALSE;
		}
		if (!FileTimeToSystemTime(&sFileTime,&sSysTime))
		{
			SaveWin32Error("FileTimeToSystemTime", GetLastError());
			return FALSE;
		}
	}
	else if (!FileTimeToSystemTime(pFileTime,&sSysTime))
	{
		SaveWin32Error("FileTimeToSystemTime", GetLastError());
		return FALSE;
	}

	return SystemTimeToDateLiteral(&sSysTime,pBuffer,FALSE);
}

BOOL _stdcall SystemTimeToDateLiteral(LPSYSTEMTIME pSysTime, char *pBuffer, BOOL bToLocal)
{
	FILETIME sFileTime, sLocalFTime;
	SYSTEMTIME sSysTime;

	if (bToLocal)
	{
		if (!SystemTimeToFileTime(pSysTime,&sFileTime))
		{
			SaveWin32Error("SystemTimeToFileTime", GetLastError());
			return FALSE;
		}
		if (!FileTimeToLocalFileTime(&sFileTime,&sLocalFTime))
		{
			SaveWin32Error("FileTimeToLocalFileTime", GetLastError());
			return FALSE;
		}
		if (!FileTimeToSystemTime(&sLocalFTime,&sSysTime))
		{
			SaveWin32Error("FileTimeToSystemTime", GetLastError());
			return FALSE;
		}
		pSysTime = &sSysTime;
	}
	
	if (pSysTime->wYear > 0 && pSysTime->wYear < 10000)
	{
#pragma warning(disable : 4996)
		_snprintf(pBuffer,VFP2C_MAX_DATE_LITERAL,"{^%04hu-%02hu-%02hu %02hu:%02hu:%02hu}",
		pSysTime->wYear,pSysTime->wMonth,pSysTime->wDay,pSysTime->wHour,pSysTime->wMinute,pSysTime->wSecond);
#pragma warning(default : 4996)
	}
	else
#pragma warning(disable : 4996)
		strcpy(pBuffer,"{ ::}");
#pragma warning(default : 4996)

	return TRUE;
}

long _stdcall CalcJulianDay(WORD Year, WORD Month, WORD Day)
{
	long y = Year, m = Month, d = Day, c, ya;

	if (m > 2) 
		m -= 3;
	else 
	{
		m += 9;
		y--;
	}
	c = y / 100;
	ya = y - 100 * c;
	return (146097L * c) / 4 + (1461L * ya) / 4 + (153L * m + 2) / 5 + d + 1721119L;
}

void _stdcall GetGregorianDate(long JulianDay, LPWORD Year, LPWORD Month, LPWORD Day)
{
  long j, y, d, m;
  
  j = JulianDay - 1721119;
  y = (4 * j - 1) / 146097;
  j = 4 * j - 1 - 146097 * y;
  d = j / 4;
  j = (4 * d + 3) / 1461;
  d = 4 * d + 3 - 1461 * j;
  d = (d + 4) / 4;
  m = (5 * d - 3) / 153;
  d = 5 * d - 3 - 153 * m;
  d = (d + 5) / 5;
  y = 100 * y + j;
  if (m < 10) 
    m += 3;
  else 
  {
    m -= 9;
    y++;
  }

  *Year = (WORD) y;
  *Month = (WORD) m;
  *Day = (WORD) d;
}

unsigned int _fastcall DoubleToUInt(double nValue)
{
	return (unsigned int)nValue; 
}

int _fastcall DoubleToInt(double nValue)
{
	return (int)nValue;
}

unsigned __int64 _fastcall DoubleToUInt64(double nValue)
{
	return (unsigned __int64)nValue; 
}

__int64 _fastcall DoubleToInt64(double nValue)
{
	return (__int64)nValue;
}
