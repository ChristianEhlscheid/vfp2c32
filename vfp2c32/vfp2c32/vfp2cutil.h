#ifndef _VFP2CUTIL_H__
#define _VFP2CUTIL_H__

#pragma warning(disable : 4290) // disable warning 4290 - VC++ doesn't implement throw ...

const int SECONDSPERDAY				= 86400;
const __int64 MAXVFPFILETIME		= 2650467743990000000;
const double VFPFILETIMEBASE		= 2305814.0;
const double VFPOLETIMEBASE			= 2415019.0;
const int NANOINTERVALSPERSECOND	= 10000000;
const __int64 NANOINTERVALSPERDAY	= 864000000000;

const int VFP2C_MAX_DATE_LITERAL	= 32;
const int VFP2C_MAX_DOUBLE_LITERAL	= 66;
const int VFP2C_MAX_SHORT_LITERAL	= 6;
const int VFP2C_MAX_INT_LITERAL		= 11;
const int VFP2C_MAX_BIGINT_LITERAL	= 20;

const char MIN_CHAR					= -128;
const char MAX_CHAR					= 127;
const unsigned char MIN_UCHAR		= 0;
const unsigned char MAX_UCHAR		= 255;
const short MIN_SHORT				= (-32768);
const short MAX_SHORT				= 32767;
const unsigned short MIN_USHORT		= 0;
const unsigned short MAX_USHORT		= 65535;
const int MIN_INT					= -2147483647 - 1;
const int MAX_INT					= 2147483647;
const unsigned int MIN_UINT			= 0;
const unsigned int MAX_UINT			= 0xFFFFFFFF;
const __int64 MIN_INT64				= 0x8000000000000000;
const __int64 MAX_INT64				= 0x7FFFFFFFFFFFFFFF;
const unsigned __int64 MIN_UINT64	= 0;
const unsigned __int64 MAX_UINT64	= 0xFFFFFFFFFFFFFFFF;

const int VFP2C_VFP_MAX_ARRAY_ROWS		= 65000;
const int VFP2C_VFP_MAX_PROPERTY_NAME	= 253;
const int VFP2C_VFP_MAX_COLUMN_NAME		= 128;
const int VFP2C_VFP_MAX_CURSOR_NAME		= 128;
const int VFP2C_VFP_MAX_CHARCOLUMN		= 254;
const int VFP2C_VFP_MAX_COLUMNS			= 255;
const int VFP2C_VFP_MAX_FIELDNAME		= VFP2C_VFP_MAX_CURSOR_NAME + 1 + VFP2C_VFP_MAX_COLUMN_NAME;

inline bool StrEqual(char *pString, char *pString2) { return strcmp(pString, pString2) == 0; }
inline bool StrIEqual(char *pString, char *pString2) { return stricmp(pString, pString2) == 0; }

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
int _stdcall Zap(char *pCursor);

void _stdcall FileTimeToDateTime(LPFILETIME pFileTime, Value *pDateTime);
void _stdcall DateTimeToFileTime(Value *pDateTime, LPFILETIME pFileTime);
BOOL _stdcall FileTimeToDateTimeEx(LPFILETIME pFileTime, Value *pDateTime, BOOL bToLocal);
BOOL _stdcall DateTimeToFileTimeEx(Value *pDateTime, LPFILETIME pFileTime, BOOL bToUTC);
BOOL _stdcall SystemTimeToDateTime(LPSYSTEMTIME pSysTime, Value *pDateTime, BOOL bToLocal);
void _stdcall SystemTimeToDateTimeEx(LPSYSTEMTIME pSysTime, Value *pDateTime);
BOOL _stdcall DateTimeToSystemTime(Value *pDateTime, LPSYSTEMTIME pSysTime, BOOL bToLocal);
void _stdcall DateTimeToSystemTimeEx(Value *pDateTime, LPSYSTEMTIME pSysTime);
void _stdcall DateTimeToLocalDateTime(Value *pDateTime);
void _stdcall LocalDateTimeToDateTime(Value *pDateTime);

BOOL _stdcall FileTimeToDateLiteral(LPFILETIME pFileTime, char *pBuffer, BOOL bToLocal);
BOOL _stdcall SystemTimeToDateLiteral(LPSYSTEMTIME pSysTime, char *pBuffer, BOOL bToLocal);

long _stdcall CalcJulianDay(WORD Year, WORD Month, WORD Day);
void _stdcall GetGregorianDate(long JulianDay, LPWORD Year, LPWORD Month, LPWORD Day);

BOOL _stdcall AnsiToUnicodePtr(char *pString, DWORD nStrLen, LPWSTR *pUnicodePtr, int *nErrorNo);
BOOL _stdcall AnsiToUnicodeBuf(char *pString, DWORD nStrLen, LPWSTR pUnicodeBuf, DWORD nBufferLen);
BOOL _stdcall UnicodeToAnsiBuf(LPWSTR pWString, DWORD nStrLen, char *pBuffer, DWORD nBufferLen, int *nConverted);

// misc C utility functions
unsigned short _stdcall CharPosN(const char *pString, const char pSearched, unsigned short nMaxPos);
unsigned int _stdcall strnlen(const char *s, unsigned long count);
unsigned int _stdcall strdblnlen(const char *s, unsigned long count);
unsigned int _stdcall strdblcount(const char *pString, unsigned long nMaxLen);
unsigned int _stdcall wstrnlen(const wchar_t *pString, unsigned int nMaxLen);
unsigned int _stdcall strcpyex(char *pBuffer, const char *pSource);
unsigned int _stdcall strncpyex(char *pBuffer, const char *pSource, unsigned int nMaxLen);
unsigned int _stdcall DoubleToUInt(double nValue);
int _stdcall DoubleToInt(double nValue);
unsigned __int64 _stdcall DoubleToUInt64(double nValue);
__int64 _stdcall DoubleToInt64(double nValue);
unsigned int _cdecl sprintfex(char *lpBuffer, const char *lpFormat, ...);
unsigned int _stdcall printfex(char *lpBuffer, const char *lpFormat, va_list lpArgs);
static char* _stdcall cvt(double arg, int ndigits, int *decpt, int *sign, char *buf, int eflag);
char* _stdcall strend(char *pString);
int _stdcall skip_atoi(const char **s);

// number to string conversion functions for parameter marshaling
char* _stdcall IntToStr(char *pString, int nNumber);
char* _stdcall IntToHex(char *pString, int nNumber);
char* _stdcall UIntToStr(char *pString, unsigned int nNumber);
char* _stdcall UIntToHex(char *pString, unsigned int nNumber);
char* _stdcall Int64ToStr(char *pString, __int64 nNumber);
char* _stdcall Int64ToVarbinary(char *pString, __int64 nNumber);
char* _stdcall Int64ToCurrency(char *pString, __int64 nNumber);
char* _stdcall UInt64ToStr(char *pString, unsigned __int64 nNumber);
char* _stdcall BoolToStr(char *pString, int nBool);
char* _stdcall FloatToStr(char *pString, float nValue, int nPrecision);
char* _stdcall DoubleToStr(char *pString, double nValue, int nPrecision);

// number conversin functions for arithmetic & struct marshaling
void _stdcall Int64ToString(char *pString, __int64 nNumber);
void _stdcall UInt64ToString(char *pString, unsigned __int64 nNumber);
__int64 _stdcall StringToInt64(char *pString, unsigned int nLen) throw(int);
unsigned __int64 _stdcall StringToUInt64(char *pString, unsigned int nLen) throw(int);

// misc VFP like string functions
unsigned int _stdcall GetWordCount(char *pString, const char pSeperator);
unsigned int _stdcall GetWordNumN(char *pBuffer, char *pString, const char pSeperator, unsigned int nToken, unsigned int nMaxLen);
void _stdcall Alltrim(char *pString);
char * _stdcall AtEx(char *pString, char pSearch, int nOccurence);
char * _stdcall ReplicateEx(char *pBuffer, char pExpr, int nCount);

// misc string parsing functions
void _stdcall skip_ws(char **pString);
BOOL _stdcall match_identifier(char **pString, char *pBuffer, int nMaxLen);
BOOL _stdcall match_dotted_identifier(char **pString, char *pBuffer, int nMaxLen);
BOOL _stdcall match_quoted_identifier(char **pString, char *pBuffer, int nMaxLen);
BOOL _stdcall match_quoted_identifier_ex(char **pString, char *pBuffer, int nMaxLen);
BOOL _stdcall match_str(char **pString, char *pSearch);
BOOL _stdcall match_istr(char **pString, char *pSearch);
BOOL _stdcall match_chr(char **pString, char pChar);
BOOL _stdcall match_one_chr(char **pString, char *pChars, char *pFound);
BOOL _stdcall match_int(char **pString, int *nInt);
BOOL _stdcall match_short(char **pString, short *nShort);

char* _stdcall str_append(char *pBuffer, char *pString);
int _stdcall str_charcount(char *pString, char pChar);

#endif // _VFP2CUTIL_H__
