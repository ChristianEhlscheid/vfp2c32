#ifndef _VFP2CUTIL_H__
#define _VFP2CUTIL_H__

#include <windows.h>
#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2ccppapi.h"

const int SECONDSPERDAY				= 86400;
const __int64 MAXVFPFILETIME		= 2650467743990000000;
const double VFPFILETIMEBASE		= 2305814.0;
const double VFPOLETIMEBASE			= 2415019.0;
const int NANOINTERVALSPERSECOND	= 10000000;
const __int64 NANOINTERVALSPERDAY	= 864000000000;

const unsigned long VFP2C_VFP_MAX_ARRAY_ROWS		= 65000;
const unsigned long VFP2C_VFP_MAX_PROPERTY_NAME		= 253;
const unsigned long VFP2C_VFP_MAX_COLUMN_NAME		= 128;
const unsigned long VFP2C_VFP_MAX_CURSOR_NAME		= 128;
const unsigned long VFP2C_VFP_MAX_CHARCOLUMN		= 254;
const unsigned long VFP2C_VFP_MAX_COLUMNS			= 255;
const unsigned long VFP2C_VFP_MAX_FIELDNAME			= VFP2C_VFP_MAX_CURSOR_NAME + 1 + VFP2C_VFP_MAX_COLUMN_NAME;

inline bool StrEqual(char *pString, char *pString2) { return strcmp(pString, pString2) == 0; }
inline bool StrIEqual(char *pString, char *pString2) { return _stricmp(pString, pString2) == 0; }

inline bool IsDigit(char pChar) { return pChar >= '0' && pChar <= '9'; }
inline bool IsCharacter(char pChar) { return (pChar >= 'a' && pChar <= 'z') || (pChar >= 'A' && pChar <= 'Z'); }

inline char ToUpper(char pChar) { return pChar >= 97 && pChar <= 122 ? pChar - 32 : pChar; }
inline char ToLower(char pChar) { return pChar >= 65 && pChar <= 90 ? pChar + 32 : pChar; }

inline int SwapEndian(int nValue)
{
    return (nValue >> 24) | ((nValue << 8) & 0x00FF0000) | ((nValue >> 8) & 0x0000FF00) | (nValue << 24);
}

inline unsigned int SwapEndian(unsigned long nValue)
{
    return (nValue >> 24) | ((nValue << 8) & 0x00FF0000) | ((nValue >> 8) & 0x0000FF00) | (nValue << 24);
}

// __int64 for MSVC, "long long" for gcc
inline unsigned __int64 SwapEndian(unsigned __int64 nValue)
{
	return (nValue >> 56) | 
        ((nValue << 40) & 0x00FF000000000000) |
        ((nValue << 24) & 0x0000FF0000000000) |
        ((nValue << 8)  & 0x000000FF00000000) |
        ((nValue >> 8)  & 0x00000000FF000000) |
        ((nValue >> 24) & 0x0000000000FF0000) |
        ((nValue >> 40) & 0x000000000000FF00) |
        (nValue << 56);
}

// misc VFP related helper/conversion functions
int _stdcall Dimension(char *pArrayName, unsigned long nRows, unsigned long nDims);
int _stdcall DimensionEx(char *pArrayName, Locator *lArrayLoc, unsigned long nRows, unsigned long nDims);
int _stdcall ASubscripts(Locator *pLoc, int *nRows, int *nDims);
int _stdcall CreateFoxVar(char *pName, Locator *pLoc, int nScope);
int _stdcall FindFoxVar(char *pName, Locator *pLoc);
int _stdcall FindFoxField(char *pName, Locator *pLoc, int nWorkarea);
int _stdcall FindFoxFieldEx(int nFieldNo, Locator *pLoc, int nWorkarea);
int _stdcall FindFoxFieldC(char *pName, Locator *pLoc, char *pCursor);
int _stdcall FindFoxVarOrField(char *pName, Locator *pLoc);
int _stdcall FindFoxVarOrFieldEx(char *pName, Locator *pLoc);
int _stdcall StoreEx(Locator *pLoc, Value *pValue);
int _stdcall AllocMemo(Locator *pLoc, int nSize, long *nLoc);
int _stdcall GetMemoContent(Value *pValue, char *pData);
int _stdcall GetMemoContentN(Value *pValue, char *pData, int nLen, int nOffset);
int _stdcall GetMemoContentEx(Locator *pLoc, char **pData, int *nErrorNo);
int _stdcall ReplaceMemo(Locator *pLoc, char *pData, int nLen);
int _stdcall ReplaceMemoEx(Locator *pLoc, char *pData, int nLen, FCHAN hFile);
int _stdcall AppendMemo(char *pData, int nLen, FCHAN hFile, long *nLoc);
int _stdcall MemoChan(int nWorkarea, FCHAN *nChan);
int _stdcall DBAppendRecords(int nWorkArea, unsigned int nRecords);
int _stdcall Zap(CStringView pCursor);
bool _stdcall Used(CStringView pCursor);
int _stdcall Select(CStringView pCursor);

void _stdcall FileTimeToDateTime(LPFILETIME pFileTime, ValueEx& pDateTime);
void _stdcall DateTimeToFileTime(ValueEx& pDateTime, LPFILETIME pFileTime);
BOOL _stdcall FileTimeToDateTimeEx(LPFILETIME pFileTime, ValueEx& pDateTime, BOOL bToLocal);
BOOL _stdcall DateTimeToFileTimeEx(ValueEx& pDateTime, LPFILETIME pFileTime, BOOL bToUTC);
BOOL _stdcall SystemTimeToDateTime(LPSYSTEMTIME pSysTime, ValueEx& pDateTime, BOOL bToLocal);
void _stdcall SystemTimeToDateTimeEx(LPSYSTEMTIME pSysTime, ValueEx& pDateTime);
BOOL _stdcall DateTimeToSystemTime(ValueEx& pDateTime, LPSYSTEMTIME pSysTime, BOOL bToLocal);
void _stdcall DateTimeToSystemTimeEx(ValueEx& pDateTime, LPSYSTEMTIME pSysTime);
void _stdcall DateTimeToLocalDateTime(ValueEx& pDateTime);
void _stdcall LocalDateTimeToDateTime(ValueEx& pDateTime);

BOOL _stdcall FileTimeToDateLiteral(LPFILETIME pFileTime, char *pBuffer, BOOL bToLocal);
BOOL _stdcall SystemTimeToDateLiteral(LPSYSTEMTIME pSysTime, char *pBuffer, BOOL bToLocal);

long _stdcall CalcJulianDay(WORD Year, WORD Month, WORD Day);
void _stdcall GetGregorianDate(long JulianDay, LPWORD Year, LPWORD Month, LPWORD Day);

unsigned int _fastcall DoubleToUInt(double nValue);
int _fastcall DoubleToInt(double nValue);
unsigned __int64 _fastcall DoubleToUInt64(double nValue);
__int64 _fastcall DoubleToInt64(double nValue);

#endif // _VFP2CUTIL_H__
