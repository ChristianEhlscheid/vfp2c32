#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2ccppapi.h"

// some helper functions for common tasks
int _stdcall Dimension(char *pArrayName, unsigned long nRows, unsigned long nDims)
{
	char aExeBuffer[256];
	if (nDims > 1)
		sprintfex(aExeBuffer,"DIMENSION %S[%U,%U]",pArrayName,nRows,nDims);
	else
		sprintfex(aExeBuffer,"DIMENSION %S[%U]",pArrayName,nRows);
	return _Execute(aExeBuffer);
}

int _stdcall DimensionEx(char *pArrayName, Locator *lArrayLoc, unsigned long nRows, unsigned long nDims)
{
	int nErrorNo;
	Value vFalse;
	vFalse.ev_type = 'L';

	nErrorNo = FindFoxVar(pArrayName,lArrayLoc);
	if (nErrorNo == E_VARIABLENOTFOUND)
	{
		// create array
		nErrorNo = Dimension(pArrayName,nRows,nDims);
		if (nErrorNo)
			return nErrorNo;

		if (nErrorNo = FindFoxVar(pArrayName,lArrayLoc))
			return nErrorNo;
	}
	else if (nErrorNo == 0)
	{
		// redimension array to passed size
		nErrorNo = Dimension(pArrayName,nRows,nDims);
		if (nErrorNo)
			return nErrorNo;

		// initialize all elements to .F.
		lArrayLoc->l_subs = 0;
		if (nErrorNo = _Store(lArrayLoc,&vFalse))
			return nErrorNo;

		lArrayLoc->l_subs = nDims > 1 ? 2 : 1;
	}
	else
		return nErrorNo;

	lArrayLoc->l_sub1 = 0;
	lArrayLoc->l_sub2 = 0;
	return 0;
}

int _stdcall ASubscripts(Locator *pLoc, int *nRows, int *nDims)
{
	if ((*nRows = _ALen(pLoc->l_NTI,AL_SUBSCRIPT1)) == -1)
		return E_NOTANARRAY;
	*nDims = _ALen(pLoc->l_NTI,AL_SUBSCRIPT2);
	return 0;
}

// fill a Locator with a reference to a FoxPro variable
int _stdcall FindFoxVar(char *pName, Locator *pLoc)
{
	NTI nVarNti;
	nVarNti = _NameTableIndex(pName);
	if (nVarNti == -1)
		return E_VARIABLENOTFOUND;

	if (_FindVar(nVarNti,-1,pLoc))
		return 0;
	else
		return E_VARIABLENOTFOUND;
}

// fill a Locator with a reference to a FoxPro field
int _stdcall FindFoxField(char *pName, Locator *pLoc, int nWorkarea)
{
	NTI nVarNti;
	nVarNti = _NameTableIndex(pName);
	if (nVarNti == -1)
		return E_FIELDNOTFOUND;

	if (_FindVar(nVarNti,nWorkarea,pLoc))
		return 0;
	else
		return E_FIELDNOTFOUND;
}

// like the above .. but instead of the workarea number the cursor/tablename can be passed
int _stdcall FindFoxFieldC(char *pName, Locator *pLoc, char *pCursor)
{
	NTI nVarNti;
	int nErrorNo;
	Value vWorkarea = {'0'};
	char aExeBuffer[VFP2C_MAX_FUNCTIONBUFFER];

	nVarNti = _NameTableIndex(pName);
	if (nVarNti == -1)
		return E_FIELDNOTFOUND;

	sprintfex(aExeBuffer, "SELECT('%S')", pCursor);
	if (nErrorNo = _Evaluate(&vWorkarea, aExeBuffer))
		return nErrorNo;

	if (_FindVar(nVarNti, vWorkarea.ev_long, pLoc))
		return 0;
	else
		return E_FIELDNOTFOUND;

}

// fill a Locator with a reference to a FoxPro field, instead of passing the name one passes the field offset (1st field, 2nd field ..)
int _stdcall FindFoxFieldEx(int nFieldNo, Locator *pLoc, int nWorkarea)
{
	char aExeBuffer[64];
	Value vFieldName = {'0'};
	NTI nVarNti;
	int nErrorNo;

	sprintfex(aExeBuffer, "FIELD(%I,%I)+CHR(0)", nFieldNo, nWorkarea);
	if (nErrorNo = _Evaluate(&vFieldName, aExeBuffer))
		return nErrorNo;

    nVarNti = _NameTableIndex(HandleToPtr(vFieldName));
	FreeHandle(vFieldName);

	if (nVarNti == -1)
		return E_FIELDNOTFOUND;
	
	if (_FindVar(nVarNti,nWorkarea,pLoc))
		return 0;
	else
		return E_FIELDNOTFOUND;
}

int _stdcall FindFoxVarOrField(char *pName, Locator *pLoc)
{
	NTI nVarNti;
	nVarNti = _NameTableIndex(pName);
	if (nVarNti == -1)
		return E_VARIABLENOTFOUND;

	if (_FindVar(nVarNti,0,pLoc))
		return 0;
	else
		return E_VARIABLENOTFOUND;
}

int _stdcall FindFoxVarOrFieldEx(char *pName, Locator *pLoc)
{
	char *pVarOrField = pName;
	int nErrorNo;
	Value vWorkArea = {'0'};
	char aTableOrVar[VFP2C_VFP_MAX_CURSOR_NAME];
	char aColumn[VFP2C_VFP_MAX_COLUMN_NAME];
	char aExeBuffer[VFP2C_MAX_FUNCTIONBUFFER];

	if (match_identifier(&pVarOrField,aTableOrVar,VFP2C_VFP_MAX_CURSOR_NAME))
	{
		if (match_chr(&pVarOrField,'.') && match_identifier(&pVarOrField,aColumn,VFP2C_VFP_MAX_COLUMN_NAME))
		{
			sprintfex(aExeBuffer, "SELECT('%S')", aTableOrVar);
			if (nErrorNo = _Evaluate(&vWorkArea, aExeBuffer))
				return nErrorNo;

			return FindFoxField(aColumn,pLoc,vWorkArea.ev_long);
		}
		else
			return FindFoxVarOrField(aTableOrVar,pLoc);
	}
	else
		return E_VARIABLENOTFOUND;
}

int _stdcall StoreEx(Locator *pLoc, Value *pValue)
{
	if (pLoc->l_where == -1)
		return _Store(pLoc,pValue);
	else
		return _DBReplace(pLoc,pValue);
}



int _stdcall AllocMemo(Locator *pLoc, int nSize, long *nLoc)
{
	*nLoc = _AllocMemo(pLoc,nSize);
	if (*nLoc == -1)
	{
		SaveCustomError("_AllocMemo","Function failed.");
		return E_APIERROR;
	}
	return 0;
}

int _stdcall MemoChan(int nWorkarea, FCHAN *nChan)
{
	*nChan = _MemoChan(nWorkarea);
	if (*nChan == -1)
	{
		SaveCustomError("_MemoChan","Function failed.");
		return E_APIERROR;
	}
	return 0;
}

int _stdcall GetMemoContent(Value *pValue, char *pData)
{
	FCHAN hFile = pValue->ev_width;
	int mLoc = (int)pValue->ev_real;
	_FSeek(hFile,mLoc,FS_FROMBOF);
	if (_FRead(hFile,pData,pValue->ev_long) != pValue->ev_long)
		return _FError();
	else
		return 0;
}

int _stdcall GetMemoContentN(Value *pValue, char *pData, int nLen, int nOffset)
{
	FCHAN hFile = pValue->ev_width;
	int mLoc = ((int)pValue->ev_real) + nOffset;
	_FSeek(hFile,mLoc,FS_FROMBOF);
	if (_FRead(hFile,pData,nLen) != nLen)
		return _FError();
	else
		return 0;
}

int _stdcall GetMemoContentEx(Locator *pLoc, char **pData, int *nErrorNo)
{
	FCHAN hFile;
	int mLoc, mLen;
	
	mLoc = _FindMemo(pLoc);
	if (mLoc < 0)
	{
		*nErrorNo = E_INVALIDPARAMS;
		return -1;
	}

	hFile = _MemoChan(pLoc->l_where);
	if (hFile == -1)
	{
		SaveCustomError("_MemoChan", "Function failed for workarea %I.", pLoc->l_where);
		return -1;
	}

	mLen = _MemoSize(pLoc);
	if (mLen <= 0)
	{
		if (mLen == 0)
			return 0;
		else
		{
			*nErrorNo = mLen;
			return -1;
		}
	}

	*pData = (char*)malloc(mLen);
	if (!*pData)
	{
		*nErrorNo = E_INSUFMEMORY;
		return -1;
	}

	_FSeek(hFile,mLoc,FS_FROMBOF);
	mLen = _FRead(hFile,*pData,mLen);
	return mLen;
}

int _stdcall ReplaceMemo(Locator *pLoc, char *pData, int nLen)
{
	FCHAN hFile;
	long nLoc;

	hFile = _MemoChan(pLoc->l_where);
	if (hFile == -1)
		return E_FIELDNOTFOUND;

	nLoc = _AllocMemo(pLoc,nLen);
	if (nLoc == -1)
		return E_INSUFMEMORY;

	_FSeek(hFile,nLoc,FS_FROMBOF);
	if (_FWrite(hFile,pData,nLen) != nLen)
		return _FError();
	return 0;
}

int _stdcall ReplaceMemoEx(Locator *pLoc, char *pData, int nLen, FCHAN hFile)
{
	long nLoc;
	nLoc = _AllocMemo(pLoc,nLen);
	if (nLoc == -1)
		return E_INSUFMEMORY;

	_FSeek(hFile,nLoc,FS_FROMBOF);
	if (_FWrite(hFile,pData,nLen) != nLen)
		return _FError();
	return 0;
}

int _stdcall AppendMemo(char *pData, int nLen, FCHAN hFile, long *nLoc)
{
	_FSeek(hFile,*nLoc,FS_FROMBOF);
	if (_FWrite(hFile,pData,nLen) != nLen)
		return _FError();
	*nLoc += nLen;
	return 0;
}

int _stdcall Zap(char *pCursor)
{
	char aExeBuffer[VFP2C_MAX_FUNCTIONBUFFER];
	sprintfex(aExeBuffer,"ZAP IN %S",pCursor);
	return _Execute(aExeBuffer);
}

//converts a filetime value to a datetime value .. milliseconds are truncated ..
void _stdcall FileTimeToDateTime(LPFILETIME pFileTime, Value *pDateTime)
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
		pDateTime->ev_real = 0.0;
		return;
	}

	sTime.QuadPart /= NANOINTERVALSPERSECOND; // gives us seconds since 1601/01/01
	pDateTime->ev_real = VFPFILETIMEBASE + (double)(sTime.QuadPart / SECONDSPERDAY); // 1601/01/01 + number of seconds / 86400 (= number of days)
	pDateTime->ev_real += ((double)(sTime.QuadPart % SECONDSPERDAY)) / SECONDSPERDAY; 
}

void _stdcall DateTimeToFileTime(Value *pDateTime, LPFILETIME pFileTime)
{
	LARGE_INTEGER nFileTime;
	double dDays, dSecs, dDateTime;

	if (pDateTime->ev_real >= VFPFILETIMEBASE)
		dDateTime = pDateTime->ev_real;
	else if (pDateTime->ev_real == 0.0) // if empty date .. set filetime to zero
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

BOOL _stdcall FileTimeToDateTimeEx(LPFILETIME pFileTime, Value *pDateTime, BOOL bToLocal)
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

BOOL _stdcall DateTimeToFileTimeEx(Value *pDateTime, LPFILETIME pFileTime, BOOL bToUTC)
{
	FILETIME sTime;
	if (bToUTC)
	{
		DateTimeToFileTime(pDateTime,&sTime);
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

void _stdcall SystemTimeToDateTimeEx(LPSYSTEMTIME pSysTime, Value *pDateTime)
{
	int lnA, lnY, lnM, lnJDay;
	lnA = (14 - pSysTime->wMonth) / 12;
	lnY = pSysTime->wYear + 4800 - lnA;
	lnM = pSysTime->wMonth + 12 * lnA - 3;
	lnJDay = pSysTime->wDay + (153 * lnM + 2) / 5 + lnY * 365 + lnY / 4 - lnY / 100 + lnY / 400 - 32045;
	pDateTime->ev_real = ((double)lnJDay) + (((double)(pSysTime->wHour * 3600 + pSysTime->wMinute * 60 + pSysTime->wSecond)) / 86400);
}

void _stdcall DateTimeToSystemTimeEx(Value *pDateTime, LPSYSTEMTIME pSysTime)
{
	int lnA, lnB, lnC, lnD, lnE, lnM;
	DWORD lnDays, lnSecs;
	double dDays, dSecs;

	dSecs = modf(pDateTime->ev_real,&dDays);
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

void _stdcall DateTimeToLocalDateTime(Value *pDateTime)
{
	TIME_ZONE_INFORMATION TimeZone;
	DWORD nApiRet;
	int nBias;

	nApiRet = GetTimeZoneInformation(&TimeZone);
	if (nApiRet == TIME_ZONE_ID_INVALID)
	{
		SaveWin32Error("GetTimeZoneInformation",GetLastError());
		return;
	}

	if (nApiRet == TIME_ZONE_ID_STANDARD || TIME_ZONE_ID_UNKNOWN)
		nBias = TimeZone.Bias + TimeZone.StandardBias;
	else if (nApiRet == TIME_ZONE_ID_DAYLIGHT)
		nBias = TimeZone.Bias + TimeZone.DaylightBias;

	pDateTime->ev_real -= (((double)nBias * 60) / SECONDSPERDAY);
}

void _stdcall LocalDateTimeToDateTime(Value *pDateTime)
{
	TIME_ZONE_INFORMATION TimeZone;
	DWORD nApiRet;
	int nBias;

	nApiRet = GetTimeZoneInformation(&TimeZone);
	if (nApiRet == TIME_ZONE_ID_INVALID)
	{
		SaveWin32Error("GetTimeZoneInformation",GetLastError());
		return;
	}

	if (nApiRet == TIME_ZONE_ID_STANDARD || TIME_ZONE_ID_UNKNOWN)
		nBias = TimeZone.Bias + TimeZone.StandardBias;
	else if (nApiRet == TIME_ZONE_ID_DAYLIGHT)
		nBias = TimeZone.Bias + TimeZone.DaylightBias;

	pDateTime->ev_real += (((double)nBias) * 60 / SECONDSPERDAY);
}

BOOL _stdcall SystemTimeToDateTime(LPSYSTEMTIME pSysTime, Value *pDateTime, BOOL bToLocal)
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

BOOL _stdcall DateTimeToSystemTime(Value *pDateTime, LPSYSTEMTIME pSysTime, BOOL bToLocal)
{
	FILETIME sFileTime, sUTCTime;

	DateTimeToFileTime(pDateTime,&sFileTime);

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
		_snprintf(pBuffer,VFP2C_MAX_DATE_LITERAL,"{^%04hu-%02hu-%02hu %02hu:%02hu:%02hu}",
		pSysTime->wYear,pSysTime->wMonth,pSysTime->wDay,pSysTime->wHour,pSysTime->wMinute,pSysTime->wSecond);
	}
	else
		strcpy(pBuffer,"{ ::}");
	
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

// converts the ANSI pString to unicode, allocating memory for the unicode string 
// pUnicodePtr must be freed sometime after the conversion with free() from msvcrt
BOOL _stdcall AnsiToUnicodePtr(char *pString, DWORD nStrLen, LPWSTR *pUnicodePtr, int *nErrorNo)
{
	LPWSTR pUnicodeStr;
	DWORD nWChars;

	if (!nStrLen)
	{
		*pUnicodePtr = 0;
		return TRUE;
	}

	nWChars = nStrLen * 2; // no of WCHAR's required
	
	// allocate the required amount of space plus 2 more bytes for L'\0'
	pUnicodeStr = (LPWSTR)malloc(nWChars + 2);
	if (!pUnicodeStr)
	{
		*nErrorNo = E_INSUFMEMORY;
		return FALSE;
	}
		
	nWChars = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,pString,nStrLen,pUnicodeStr,nWChars);
	if (!nWChars)
	{
		SaveWin32Error("MultiByteToWideChar", GetLastError());
		return FALSE;	
	}

	pUnicodeStr[nWChars] = L'\0'; // nullterminate
	*pUnicodePtr = pUnicodeStr;
	
	return TRUE;
}

// converts the ANSI pString to unicode, using pUnicodeBuf as the destination
// nBufferLen should contain the number of unicode chars, not the number of bytes
BOOL _stdcall AnsiToUnicodeBuf(char *pString, DWORD nStrLen, LPWSTR pUnicodeBuf, DWORD nBufferLen)
{
	DWORD nWChars;

	if (!nStrLen)
	{
		*pUnicodeBuf = L'\0';
		return TRUE;
	}

	nWChars = MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,pString,nStrLen,pUnicodeBuf,nBufferLen);
	if (!nWChars)
	{
		SaveWin32Error("MultiByteToWideChar", GetLastError());
		return FALSE;	
	}

	pUnicodeBuf[nWChars] = L'\0'; // nullterminate
	
	return TRUE;
}

BOOL _stdcall UnicodeToAnsiBuf(LPWSTR pWString, DWORD nStrLen, char *pBuffer, DWORD nBufferLen, int *nConverted)
{
	if (!nStrLen)
	{
		nStrLen = wcslen(pWString);
		if (!nStrLen) 
		{
			*nConverted = 0;
        	return TRUE;
		}
	}

	*nConverted = WideCharToMultiByte(CP_ACP,0,pWString,nStrLen,pBuffer,nBufferLen,0,0);
	if (!*nConverted)
	{
		SaveWin32Error("MultiByteToWideChar", GetLastError());
		return FALSE;
	}
	return TRUE;
}

// returns position of first occurrence of character in some string
// to map character positions to an array dimension (e.g. in AWindowsEx) 
unsigned short _stdcall CharPosN(const char *pString, const char pSearched, unsigned short nMaxPos)
{
	unsigned short nPos;
	for (nPos = 1; nPos <= nMaxPos; nPos++) 
	{
		if (*pString == '\0')
			return 0;
		if (*pString == pSearched)
			return nPos;
		pString++;
	}
	return 0;
}

// determine len up to nMaxLen characters of a unicode string
unsigned int _stdcall wstrnlen(const wchar_t *pString, unsigned int nMaxLen)
{
	register const wchar_t* pStart = pString;
	while (nMaxLen--)
	{
		if (*pString)
			pString++;
		else
			break;
	}
	return pString - pStart;
}
// copies null terminated source string to a buffer
// returns count characters copied NOT including the terminating null character
unsigned int _stdcall strcpyex(char *pBuffer, const char* pSource)
{
	register const char* pStart = pSource;
	while (*pBuffer++ = *pSource++);
	return pSource - pStart - 1;
}
// copies up to nMaxLen characters from source string to a buffer
// returns count characters copied NOT including the terminating null character
unsigned int _stdcall strncpyex(char *pBuffer, const char *pSource, unsigned int nMaxLen)
{
	register const char *pStart = pBuffer;
	while ((*pBuffer++ = *pSource++) && --nMaxLen);
	if (nMaxLen)
		return pBuffer - pStart - 1;
	else
		return pBuffer - pStart;
}

unsigned int _stdcall DoubleToUInt(double nValue)
{
	return (unsigned int)nValue; 
}

int _stdcall DoubelToInt(double nValue)
{
	return (int)nValue;
}

unsigned __int64 _stdcall DoubleToUInt64(double nValue)
{
	return (unsigned __int64)nValue; 
}

__int64 _stdcall DoubleToInt64(double nValue)
{
	return (__int64)nValue;
}

// returns the no. of tokens in 'pString' which are seperated by 'pSeperator'
unsigned int _stdcall GetWordCount(char *pString, const char pSeperator)
{
	unsigned int nTokens = 1;
	const char *pSource = pString;
	while (*pString)
	{
		if (*pString++ == pSeperator)
			nTokens++;
	}
	if (pSource != pString)
        return nTokens;
	else
		return 0;
}

// copies the Nth Token from a string seperated by 'pSeperator' to the location pointed to by pBuffer,
// nMaxLen should be the size of the buffer
unsigned int _stdcall GetWordNumN(char *pBuffer, char *pString, const char pSeperator, unsigned int nToken, unsigned int nMaxLen)
{
	int nTokenNo = 1;
	const char *pBuffPtr = pBuffer;

	if (nToken == 1)
		goto TokenCopy;

	while (*pString)
	{
		if (*pString++ == pSeperator)
		{
			nTokenNo++;
			if (nTokenNo == nToken)
				break;
		}
	}

	TokenCopy:
		while ((*pString) && *pString != pSeperator && --nMaxLen)
			*pBuffer++ = *pString++;
	
	*pBuffer = '\0';		
	return pBuffer - pBuffPtr;
}

// trims all spaces from string, the string is modified where it is which is save since it can only shrink
void _stdcall Alltrim(char *pString)
{
	char *pStart;
	char *pEnd;
	
	if (*pString == '\0') /* empty string */
		return;

	pStart = pEnd = pString;

	while (*pStart == ' ') pStart++; /* find start position */

	while (*++pEnd); /* find end of string */

	if (pStart == pEnd) /* entire string consisted of spaces */
	{
		*pString = '\0';
		return;
	}

	while (*--pEnd == ' '); /* find end of valid characters */

	if (pStart != pString) /* need to move string back */
	{
		while (pString != pEnd)
			*pString++ = *pStart++;
		*pString = '\0';
	}
	else				  /* just set nullterminator */
		*++pEnd = '\0';
	/* else string was empty anyway */
}

char * _stdcall AtEx(char *pString, char pSearch, int nOccurence)
{
	int nToken = 0;
	if (nOccurence == 0)
		return pString;

SearchToken:
	while (*pString && *pString != pSearch) pString++;

	if (*pString == pSearch)
	{
		nToken++;
		if (nToken == nOccurence)
			return pString + 1;
		else
			goto SearchToken;
	}
	else
		return 0;
}

char * _stdcall ReplicateEx(char *pBuffer, char pExpr, int nCount)
{
	while (nCount--)
		*pBuffer++ = pExpr;
	return pBuffer;
}

char* _stdcall strend(char *pString)
{
	while ((*pString)) pString++;
		return pString;
}

#define CVTBUFSIZE        (309 + 43)

char* _stdcall IntToStr(char *pString, int nNumber)
{
	char aBuffer[VFP2C_MAX_DOUBLE_LITERAL];
	char *pTmp = aBuffer;

	if (nNumber < 0)
	{
		*pString++ = '-';
		nNumber = -nNumber;
	}

	if (nNumber != 0)
	{
		unsigned int nNum = (unsigned int)nNumber;
		do 
		{
			*pTmp++ = '0' + nNum % 10;
			nNum /= 10;
		}
		while (nNum != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';
	
	return pString;
}

char* _stdcall IntToHex(char *pString, int nNumber)
{
	register int xj;
	register char c;

	if (nNumber < 0)
		*pString++ = '-';

	*pString++ = '0';
	*pString++ = 'x';

	pString += 8;

	for (xj = 8; xj; xj--)
	{
		c = (nNumber & 0xF ) + '0';
		if (c > '9')
			c += 0x7;
		*pString-- = c;
		nNumber >>= 4;
	}

	return pString + 8;
} 

char* _stdcall UIntToStr(char *pString, unsigned int nNumber)
{
	char aBuffer[VFP2C_MAX_INT_LITERAL];
	char *pTmp = aBuffer;

	if (nNumber != 0)
	{
		do 
		{
			*pTmp++ = '0' + nNumber % 10;
			nNumber /= 10;
		}
		while (nNumber != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';
	
	return pString;
}

char* _stdcall UIntToHex(char *pString, unsigned int nNumber)
{
	register int xj;
	register char c;

	*pString++ = '0';
	*pString++ = 'x';

	pString += 8;

	for (xj = 7; xj; xj--)
	{
		c = (nNumber & 0xF) + '0';
		if (c > '9')
			c += 0x7;
		*pString-- = c;
		nNumber >>= 4;
	}

	return pString + 8;
}


char* _stdcall Int64ToStr(char *pString, __int64 nNumber)
{
	char aBuffer[VFP2C_MAX_BIGINT_LITERAL];
	char *pTmp = aBuffer;

	if (nNumber < 0)
	{
		*pString++ = '-';
		nNumber = -nNumber;
	}

	if (nNumber)
	{
		unsigned __int64 nNum = (unsigned __int64)nNumber;
		do 
		{
			*pTmp++ = '0' + (char)(nNum % 10);
			nNum /= 10;
		}
		while (nNum != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';
	
	return pString;
}

union __Int64ToVarbinary
{
	char aBinary[8];
	__int64 nNumber;
};

char* _stdcall Int64ToVarbinary(char *pString, __int64 nNumber)
{
	union __Int64ToVarbinary nInt;
	nInt.nNumber = nNumber;

	const char *HexTable = "0123456789ABCDEF";
	register char *pSource = nInt.aBinary;
	register int xj;
	register char c;

	*pString++ = '0';
	*pString = 'h';

	pString += 16;
	for (xj = 8; xj; xj--)
	{
		c = *pSource++;
		*pString-- = HexTable[(c & 0xF)];
		*pString-- = HexTable[((c >> 4) & 0xF)];
	}
	return pString + 17;
} 

char* _stdcall Int64ToCurrency(char *pString, __int64 nNumber)
{
	char aBuffer[VFP2C_MAX_BIGINT_LITERAL];
	char *pTmp = aBuffer;

	if (nNumber == MIN_INT64)
	{
		int nSize = sizeof("STR2INT64(0h0000000000000080)");
		memcpy(pString, "STR2INT64(0h0000000000000080)", nSize);
		return pString + nSize;
	}

	*pString++ = '$';
	if (nNumber < 0)
	{
		*pString++ = '-';
		nNumber = -nNumber;
	}

	if (nNumber)
	{
		unsigned __int64 nNum = (unsigned __int64)nNumber;
		do 
		{
			*pTmp++ = '0' + (char)(nNum % 10);
			nNum /= 10;
		}
		while (nNum != 0);

		int nLen = pTmp - aBuffer;
		if (nLen <= 4)
		{
			*pString++ = '0';
			*pString++ = '.';
			while (nLen++ != 4) *pString++ = '0';
			while (pTmp != aBuffer) *pString++ = *--pTmp;
		}
		else
		{
			while (pTmp != aBuffer+4) *pString++ = *--pTmp;
			*pString++ = '.';
			*pString++ = *--pTmp;
			*pString++ = *--pTmp;
			*pString++ = *--pTmp;
			*pString++ = *--pTmp;
		}
	}
	else
		*pString++ = '0';
	
	return pString;
}

char* _stdcall UInt64ToStr(char *pString, unsigned __int64 nNumber)
{
	char aBuffer[VFP2C_MAX_BIGINT_LITERAL];
	char *pTmp = aBuffer;

	if (nNumber != 0)
	{
		do 
		{
			*pTmp++ = '0' + (char)(nNumber % 10);
			nNumber /= 10;
		}
		while (nNumber != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';
	
	return pString;
}

char* _stdcall BoolToStr(char *pString, int nBool)
{
	*pString++ = '.';
	*pString++ = nBool ? 'T' : 'F';
	*pString++ = '.';
	return pString;
}

char* _stdcall FloatToStr(char *pString, float nValue, int nPrecision)
{
  int decpt, sign, pos;
  char *digits = NULL;
  char cvtbuf[80];
  double nValueEx = (double)nValue;

  digits = cvt(nValueEx, nPrecision,&decpt,&sign,cvtbuf,0);

  if (sign) *pString++ = '-';

  if (*digits)
  {
     if (decpt <= 0)
     {
       *pString++ = '0';
       *pString++ = '.';
       for (pos = 0; pos < -decpt; pos++) *pString++ = '0';
       while (*digits) *pString++ = *digits++;
     }
     else
     {
       pos = 0;
       while (*digits)
       {
         if (pos++ == decpt) *pString++ = '.';
         *pString++ = *digits++;
       }
     }
   }
   else
   {
      *pString++ = '0';
      if (nPrecision > 0)
      {
        *pString++ = '.';
        for (pos = 0; pos < nPrecision; pos++) *pString++ = '0';
      }
   }

   return pString;
}

char* _stdcall DoubleToStr(char *pString, double nValue, int nPrecision)
{
  int decpt, sign, pos;
  char *digits = NULL;
  char cvtbuf[80];
  
  digits = cvt(nValue, nPrecision,&decpt,&sign,cvtbuf,0);

  if (sign) *pString++ = '-';

  if (*digits)
  {
     if (decpt <= 0)
     {
       *pString++ = '0';
       *pString++ = '.';
       for (pos = 0; pos < -decpt; pos++) *pString++ = '0';
       while (*digits) *pString++ = *digits++;
     }
     else
     {
       pos = 0;
       while (*digits)
       {
         if (pos++ == decpt) *pString++ = '.';
         *pString++ = *digits++;
       }
     }
   }
   else
   {
      *pString++ = '0';
      if (nPrecision > 0)
      {
        *pString++ = '.';
        for (pos = 0; pos < nPrecision; pos++) *pString++ = '0';
      }
   }

   return pString;
}

void _stdcall Int64ToString(char *pString, __int64 nNumber)
{
	char aBuffer[VFP2C_MAX_BIGINT_LITERAL];
	char *pTmp = aBuffer;

	if (nNumber < 0)
	{
		*pString++ = '-';
		nNumber = -nNumber;
	}

	if (nNumber)
	{
		unsigned __int64 nNum = (unsigned __int64)nNumber;
		do 
		{
			*pTmp++ = '0' + (char)(nNum % 10);
			nNum /= 10;
		}
		while (nNum != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';

	*pString = '\0';
}

__int64 _stdcall StringToInt64(char *pString, unsigned int nLen) throw(int)
{
	__int64 nInt = 0;
	char cDigit;
	bool bPositive;

	if (nLen == 0)
		return nInt;

 	while (*pString == ' ' && nLen--) pString++;
	
	if (nLen == 0)
		return nInt;

	if (*pString == '-')
	{
		if (--nLen == 0)
			return nInt;
 		bPositive = false;
		pString++;
	}
	else if (*pString == '+')
	{
		if (--nLen == 0)
			return nInt;
		bPositive = true;
		pString++;
	}
	else
		bPositive = true;

	if (nLen < 19)
	{
		do
		{
			cDigit = *pString++;
			if (IsDigit(cDigit))
				nInt = nInt * 10 + (cDigit - '0');
			else if (cDigit == ' ')
				break;
			else 
				throw E_INVALIDPARAMS;
		} while (--nLen);
		return bPositive ? nInt : -nInt;
	}

	for (int xj = 1; xj < 19; xj++)
	{
		cDigit = *pString++;
		if (IsDigit(cDigit))
			nInt = nInt * 10 + (cDigit - '0');
		else if (cDigit == ' ')
			break;
		else 
			throw E_INVALIDPARAMS;
	}

	cDigit = *pString++;
	if (IsDigit(cDigit))
	{
		nInt = nInt * 10 + (cDigit - '0');
		if (bPositive)
		{
			if (static_cast<unsigned __int64>(nInt) > MAX_INT64)
				throw E_NUMERICOVERFLOW;
		}
		else
		{
			if (static_cast<unsigned __int64>(nInt) > 0x8000000000000000)
				throw E_NUMERICOVERFLOW;
		}
	}
	else if (cDigit != ' ')
		throw E_INVALIDPARAMS;

	nLen -= 19;
	if (nLen)
	{	
		cDigit = *pString;
		if (IsDigit(cDigit))
			throw E_NUMERICOVERFLOW;
		else if (cDigit != ' ')
			throw E_INVALIDPARAMS;
	}

	return bPositive ? nInt : -nInt;
}

void _stdcall UInt64ToString(char *pString, unsigned __int64 nNumber)
{
	char aBuffer[VFP2C_MAX_BIGINT_LITERAL];
	char *pTmp = aBuffer;

	if (nNumber != 0)
	{
		do 
		{
			*pTmp++ = '0' + (char)(nNumber % 10);
			nNumber /= 10;
		}
		while (nNumber != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';
}

unsigned __int64 _stdcall StringToUInt64(char *pString, unsigned int nLen) throw(int)
{
	const unsigned __int64 nMax = MAX_UINT / 10;
	int nMaxRem = MAX_UINT % 10;

	unsigned __int64 nUInt = 0;
	char cDigit;
	int  nNum;

	if (nLen == 0)
		return nUInt;

	while (*pString == ' ' && nLen--) pString++;
	if (nLen == 0)
		return nUInt;

	if (nLen < 20)
	{
		while (nLen--)
		{
			cDigit = *pString++;
			if (IsDigit(cDigit))
				nUInt = nUInt * 10 + (cDigit - '0');
			else if (cDigit == ' ')
				break;
			else
				throw E_INVALIDPARAMS;
		}
		return nUInt;	
	}
	
	for (int xj = 1; xj < 20; xj++)
	{
		cDigit = *pString++;
		if (IsDigit(cDigit))
			nUInt = nUInt * 10 + (cDigit - '0');
		else if (cDigit == ' ')
			break;
		else
			throw E_INVALIDPARAMS;
	}

	cDigit = *pString++;
	if (IsDigit(cDigit))
	{
		nNum = cDigit - '0';
		if (nUInt < nMax || (nUInt == nMax && nNum <= nMaxRem))
			nUInt = nUInt * 10 + nNum;
		else
			throw E_NUMERICOVERFLOW;
	}
	else if (cDigit != ' ')
		throw E_INVALIDPARAMS;

	nLen -= 20;
	if (nLen)
	{	
		cDigit = *pString;
		if (IsDigit(cDigit))
			throw E_NUMERICOVERFLOW;
		else if (cDigit != ' ')
			throw E_INVALIDPARAMS;
	}

	return nUInt;
}

unsigned int _stdcall strnlen(const char *s, unsigned long count)
{
  const char *sc;
  for (sc = s; *sc != '\0' && count--; ++sc);
  return sc - s;
}

unsigned int _stdcall strdblnlen(const char *s, unsigned long count)
{
	const char *sc = s;

Strchk:
	while (*sc != '\0' && count--) ++sc;
	
	sc++;
	if (*sc == '\0')
		return sc - s - 1;
	else
	{
		count--;
		goto Strchk;
	}
}

unsigned int _stdcall strdblcount(const char *pString, unsigned long nMaxLen)
{
	unsigned int nStringCnt = 0;
	while (1)
	{
		while (*pString != '\0' && nMaxLen--) ++pString;
		nStringCnt++;
		pString++;		
		if (!nMaxLen || *pString == '\0')
			break;
		nMaxLen--;
	}
	return nStringCnt;
}

int _stdcall skip_atoi(const char **s)
{
  int i = 0;
  while (IsDigit(**s)) i = i*10 + *((*s)++) - '0';
  return i;
}

static char* _stdcall cvt(double arg, int ndigits, int *decpt, int *sign, char *buf, int eflag)
{
  int rTmp;
  double fi, fj;
  char *p, *pTmp;

  if (ndigits < 0) ndigits = 0;
  if (ndigits >= CVTBUFSIZE - 1) ndigits = CVTBUFSIZE - 2;
  rTmp = 0;
  *sign = 0;
  p = &buf[0];
  if (arg < 0)
  {
    *sign = 1;
    arg = -arg;
  }
  arg = modf(arg, &fi);
  pTmp = &buf[CVTBUFSIZE];

  if (fi != 0) 
  {
    pTmp = &buf[CVTBUFSIZE];
    while (fi != 0) 
    {
      fj = modf(fi / 10, &fi);
      *--pTmp = (int)((fj + .03) * 10) + '0';
      rTmp++;
    }
    while (pTmp < &buf[CVTBUFSIZE]) *p++ = *pTmp++;
  } 
  else if (arg > 0)
  {
    while ((fj = arg * 10) < 1) 
    {
      arg = fj;
      rTmp--;
    }
  }
  pTmp = &buf[ndigits];
  if (eflag == 0) pTmp += rTmp;
  *decpt = rTmp;
  if (pTmp < &buf[0]) 
  {
    buf[0] = '\0';
    return buf;
  }
  while (p <= pTmp && p < &buf[CVTBUFSIZE])
  {
    arg *= 10;
    arg = modf(arg, &fj);
    *p++ = (int) fj + '0';
  }
  if (pTmp >= &buf[CVTBUFSIZE]) 
  {
    buf[CVTBUFSIZE - 1] = '\0';
    return buf;
  }
  p = pTmp;
  *pTmp += 5;
  while (*pTmp > '9') 
  {
    *pTmp = '0';
    if (pTmp > buf)
      ++*--pTmp;
    else 
    {
      *pTmp = '1';
      (*decpt)++;
      if (eflag == 0) 
      {
        if (p > buf) *p = '0';
        p++;
      }
    }
  }
  *p = '\0';
  return buf;
}

unsigned int _cdecl sprintfex(char *lpBuffer, const char *lpFormat, ...)
{
	unsigned int nRet;
	va_list lpArgs;
	va_start(lpArgs, lpFormat);
	nRet = printfex(lpBuffer, lpFormat, lpArgs);
	va_end(lpArgs);
	return nRet;
}

unsigned int _stdcall printfex(char *lpBuffer, const char *lpFormat, va_list lpArgs)
{
	char *lpString;
	char *lpStringParm;
	double nDouble;
	int nPrecision, nUseLength;
	
	for (lpString = lpBuffer ; *lpFormat ; lpFormat++)
	{
		if (*lpFormat != '%')
		{
			*lpString++ = *lpFormat;
			continue;
		}
                  
    lpFormat++; 

	if (nUseLength = IsDigit(*lpFormat))
		nPrecision = skip_atoi(&lpFormat);
	else
		nPrecision = 6;

    switch (*lpFormat)
    {

		case 'I':
			lpString = IntToStr(lpString,va_arg(lpArgs,int));
			continue;

		case 'U':
			lpString = UIntToStr(lpString,va_arg(lpArgs,unsigned int));
			continue;

		case 'i':
			lpString = IntToStr(lpString,va_arg(lpArgs,short));
			continue;

		case 'u':
			lpString = UIntToStr(lpString,va_arg(lpArgs,unsigned short));
			continue;

		case 'F':
			lpString = DoubleToStr(lpString,va_arg(lpArgs, double),nPrecision);
			continue;

		case 'f':
			nDouble = (double)va_arg(lpArgs,float);
			lpString = DoubleToStr(lpString,nDouble,nPrecision);
			continue;

		case 'b':
			lpString = Int64ToStr(lpString,va_arg(lpArgs, __int64));
			continue;

		case 'B':
			lpString = UInt64ToStr(lpString,va_arg(lpArgs, unsigned __int64));
		
		case 'L':
			lpString = BoolToStr(lpString,va_arg(lpArgs,int));
			continue;

		case 'S':
			lpStringParm = va_arg(lpArgs,char*);
			if (lpStringParm)
			{
				if (!nUseLength)
					while ((*lpStringParm)) *lpString++ = *lpStringParm++;
				else
					while ((*lpStringParm) && nPrecision--) *lpString++ = *lpStringParm++;
			}
			continue;

		case 's':
			*lpString++ = va_arg(lpArgs,char);
			continue;
			
		default:
			if (*lpFormat != '%') *lpString++ = '%';
			if (*lpFormat)
				*lpString++ = *lpFormat;
			else
				--lpFormat;
			continue;
    }

   }

  *lpString = '\0';
  return lpString - lpBuffer;
}

// skips over whitespace (space & tab)
void _stdcall skip_ws(char **pString)
{
	char *pStringEx = *pString;
	while (*pStringEx == ' ' || *pStringEx == '\t') pStringEx++;
	*pString = pStringEx;
}

// matches an identifier of up to nMaxLen and stores it into pBuffer
BOOL _stdcall match_identifier(char **pString, char *pBuffer, int nMaxLen)
{
	char *pStringEx;

	skip_ws(pString);
	pStringEx = *pString;

	if (!IsCharacter(*pStringEx) && *pStringEx != '_')
		return FALSE;

	*pBuffer++ = *pStringEx++;
	nMaxLen--;

	while (*pStringEx && --nMaxLen)
	{
		if (IsCharacter(*pStringEx) || IsDigit(*pStringEx) || *pStringEx == '_')
			*pBuffer++ = *pStringEx++;
		else
			break;
	}

	*pBuffer = '\0';
	*pString = pStringEx;
	return TRUE;
}

BOOL _stdcall match_dotted_identifier(char **pString, char *pBuffer, int nMaxLen)
{
	char *pStringEx;

	skip_ws(pString);
	pStringEx = *pString;

	if (!IsCharacter(*pStringEx) && *pStringEx != '_')
		return FALSE;

	*pBuffer++ = *pStringEx++;
	nMaxLen--;

	while (*pStringEx && --nMaxLen)
	{
		if (IsCharacter(*pStringEx) || IsDigit(*pStringEx) || *pStringEx == '_' || *pStringEx == '.')
			*pBuffer++ = *pStringEx++;
		else
			break;
	}

	*pBuffer = '\0';
	*pString = pStringEx;
	return TRUE;
}

// match a string enclosed in ' or "
BOOL _stdcall match_quoted_identifier(char **pString, char *pBuffer, int nMaxLen)
{
	char *pStringEx;
	char pStringDelim;

	skip_ws(pString);
	
	pStringEx = *pString;
	pStringDelim = *pStringEx;

	if (pStringDelim == '\'' || pStringDelim == '"')
	{
		pStringEx++; // skip over ' or "

		while (*pStringEx && *pStringEx != pStringDelim && --nMaxLen)
			*pBuffer++ = *pStringEx++;
		
		if (*pStringEx == pStringDelim)
			pStringEx++; // skip over ending ' or " if it was found .. otherwise we are at the end of the string

		*pBuffer = '\0';
		*pString = pStringEx;
		return TRUE;
	}
	else
		return FALSE;
}

// match a string enclosed in ' or ", but also write the seperator into the buffer
BOOL _stdcall match_quoted_identifier_ex(char **pString, char *pBuffer, int nMaxLen)
{
	char *pStringEx;
	char pStringDelim;

	skip_ws(pString);
	
	pStringEx = *pString;
	pStringDelim = *pStringEx;

	if (pStringDelim == '\'' || pStringDelim == '"')
	{
		*pBuffer++ = *pStringEx++; // skip over ' or "

		while (*pStringEx && *pStringEx != pStringDelim && --nMaxLen)
			*pBuffer++ = *pStringEx++;
		
		if (*pStringEx == pStringDelim)
			*pBuffer++ = *pStringEx++; // skip over ending ' or " if it was found .. otherwise we are at the end of the string

		*pBuffer = '\0';
		*pString = pStringEx;
		return TRUE;
	}
	else
		return FALSE;
}

// match a string, case sensitive
BOOL _stdcall match_str(char **pString, char *pSearch)
{
	char *pStringEx;
	skip_ws(pString);
	pStringEx = *pString;

	while (*pSearch)
	{
		if (*pStringEx++ != *pSearch++)
			return FALSE;
	}
	*pString = --pStringEx;
	return TRUE;
}

//  match a string, case insensitive
BOOL _stdcall match_istr(char **pString, char *pSearch)
{
	char *pStringEx;

	skip_ws(pString);
	pStringEx = *pString;

	while (*pSearch)
	{
		if (ToUpper(*pStringEx) != ToUpper(*pSearch))
			return FALSE;
		else
		{
			pStringEx++;
			pSearch++;
		}
	}
	*pString = pStringEx;
	return TRUE;
}

// match a single character
BOOL _stdcall match_chr(char **pString, char pChar)
{
	skip_ws(pString);

	if (**pString == pChar)
	{
		*pString = *pString + 1;
		return TRUE;
	}
	return FALSE;
}

// match one of several characters and store found character in pFound
BOOL _stdcall match_one_chr(char **pString, char *pChars, char *pFound)
{
	char *pStringEx;
	skip_ws(pString);
	pStringEx = *pString;

	while (*pChars)
	{
		if (*pStringEx == *pChars)
		{
			*pFound = *pChars;
			*pString = pStringEx + 1;
			return TRUE;
		}
		pChars++;
	}
	return FALSE;
}

// match an integer & store it into nInt
BOOL _stdcall match_int(char **pString, int *nInt)
{
	char *pStringEx;
	char aBuffer[VFP2C_MAX_INT_LITERAL];
	char *pTmp = aBuffer, nBuffLen = VFP2C_MAX_INT_LITERAL;

	skip_ws(pString);
	pStringEx = *pString;

	if (*pStringEx == '-')
		*pTmp++ = *pStringEx++;
	
	if (!IsDigit(*pStringEx))
		return FALSE;

	while (IsDigit(*pStringEx) && --nBuffLen)
		*pTmp++ = *pStringEx++;
	*pTmp = '\0';

	*nInt = atoi(aBuffer);
	*pString = pStringEx;

	return TRUE;
}

// match a short and store it into nShort
BOOL _stdcall match_short(char **pString, short *nShort)
{
	char *pStringEx;
	char aBuffer[VFP2C_MAX_SHORT_LITERAL];
	char *pTmp = aBuffer, nBuffLen = VFP2C_MAX_SHORT_LITERAL;

	skip_ws(pString);
	pStringEx = *pString;

	if (*pStringEx == '-')
		*pTmp++ = *pStringEx++;

	if (!IsDigit(*pStringEx))
		return FALSE;

	while (IsDigit(*pStringEx) && --nBuffLen)
		*pTmp++ = *pStringEx++;
	*pTmp = '\0';

	*nShort = (short)atoi(aBuffer);
	*pString = pStringEx;

	return TRUE;
}

// appends a string to the start of the string
// and returns the new end of the string 
// to speed up many strcat operations in succession
char* _stdcall str_append(char *pBuffer, char *pString)
{
	while (*pString)
		*pBuffer++ = *pString++;

	*pBuffer = '\0';
	return pBuffer;
}

// returns the number of occurences of the character pChar in pString
int _stdcall str_charcount(char *pString, char pChar)
{
	int nCount = 0;
	while (*pString)
	{
		if (*pString == pChar)
			nCount++;
		pString++;
	}
	return nCount;
}

// 