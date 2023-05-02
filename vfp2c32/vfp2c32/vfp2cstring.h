#ifndef _VFP2CSTRING_H__
#define _VFP2CSTRING_H__

#include <Windows.h>

const int VFP2C_MAX_BOOL_LITERAL = 3;
const int VFP2C_MAX_DATE_LITERAL = 32;
const int VFP2C_MAX_DOUBLE_LITERAL = 66;
const int VFP2C_MAX_SHORT_LITERAL = 6;
const int VFP2C_MAX_INT_LITERAL = 11;
const int VFP2C_MAX_BIGINT_LITERAL = 20;

const char MIN_CHAR = -128;
const char MAX_CHAR = 127;
const unsigned char MIN_UCHAR = 0;
const unsigned char MAX_UCHAR = 255;
const short MIN_SHORT = (-32768);
const short MAX_SHORT = 32767;
const unsigned short MIN_USHORT = 0;
const unsigned short MAX_USHORT = 65535;
const int MIN_INT = -2147483647 - 1;
const int MAX_INT = 2147483647;
const unsigned int MIN_UINT = 0;
const unsigned int MAX_UINT = 0xFFFFFFFF;
const __int64 MIN_INT64 = 0x8000000000000000;
const __int64 MAX_INT64 = 0x7FFFFFFFFFFFFFFF;
const unsigned __int64 MIN_UINT64 = 0;
const unsigned __int64 MAX_UINT64 = 0xFFFFFFFFFFFFFFFF;

// misc C utility functions
unsigned short _fastcall CharPosN(const char* pString, const char pSearched, unsigned short nMaxPos);
#if !defined(_WIN64)
unsigned int _fastcall strnlen(const char* s, unsigned long count);
#endif
unsigned int _fastcall strdblnlen(const char* s, unsigned long count);
unsigned int _fastcall strdblcount(const char* pString, unsigned long nMaxLen);
unsigned int _fastcall wstrnlen(const wchar_t* pString, unsigned int nMaxLen);
unsigned int _fastcall strcpyex(char* pBuffer, const char* pSource);
unsigned int _fastcall strncpyex(char* pBuffer, const char* pSource, unsigned int nMaxLen);
unsigned int _cdecl sprintfex(char* lpBuffer, const char* lpFormat, ...);
unsigned int _stdcall printfex(char* lpBuffer, const char* lpFormat, va_list lpArgs);
unsigned int _cdecl snprintfex(char* lpBuffer, const char* lpFormat, int nBufferSize, ...);
unsigned int _stdcall nprintfex(char* lpBuffer, const char* lpFormat, int nBufferSize, va_list lpArgs);
char* _stdcall cvt(double arg, int ndigits, int* decpt, int* sign, char* buf, int eflag);
char* _fastcall strend(char* pString);
int _fastcall skip_atoi(const char** s);

// number to string conversion functions for parameter marshaling
char* _fastcall ShortToStr(char* pString, short nNumber);
char* _fastcall ShortToHex(char* pString, short nNumber);
char* _fastcall UShortToStr(char* pString, unsigned short nNumber);
char* _fastcall UShortToHex(char* pString, unsigned short nNumber);
char* _fastcall IntToStr(char* pString, int nNumber);
char* _fastcall IntToHex(char* pString, int nNumber);
char* _fastcall UIntToStr(char* pString, unsigned int nNumber);
char* _fastcall UIntToHex(char* pString, unsigned int nNumber);
char* _fastcall Int64ToStr(char* pString, __int64 nNumber);
char* _fastcall Int64ToHex(char* pString, __int64 nNumber);
char* _fastcall Int64ToVarbinary(char* pString, __int64 nNumber);
char* _fastcall Int64ToCurrency(char* pString, __int64 nNumber);
char* _fastcall UInt64ToStr(char* pString, unsigned __int64 nNumber);
char* _fastcall UInt64ToHex(char* pString, unsigned __int64 nNumber);
char* _fastcall UInt64ToHexSSE(char* pString, __int64 nNumber);
char* _fastcall BoolToStr(char* pString, int nBool);
char* _fastcall FloatToStr(char* pString, float nValue, int nPrecision);
char* _fastcall DoubleToStr(char* pString, double nValue, int nPrecision);

// number conversin functions for arithmetic & struct marshaling
void _fastcall Int64ToStrBuffer(char* pString, __int64 nNumber);
void _fastcall UInt64ToStrBuffer(char* pString, unsigned __int64 nNumber);
#pragma warning(disable : 4290) // disable warning 4290 - VC++ doesn't implement throw ...
__int64 _fastcall StringToInt64(char* pString, unsigned int nLen);
unsigned __int64 _fastcall StringToUInt64(char* pString, unsigned int nLen);
#pragma warning(default : 4290) // enable warning 4290 - VC++ doesn't implement throw ...

// misc VFP like string functions
unsigned int _fastcall GetWordCount(const char* pString, const char pSeperator);
unsigned int _fastcall GetWordNumN(char* pBuffer, const char* pString, const char pSeperator, unsigned int nToken, unsigned int nMaxLen);
void _fastcall Alltrim(char* pString);
char* _fastcall AtEx(char* pString, char pSearch, int nOccurence);
char* _fastcall ReplicateEx(char* pBuffer, char pExpr, int nCount);

// misc string parsing functions
void _fastcall skip_ws(char** pString);
BOOL _fastcall match_identifier(char** pString, char* pBuffer, int nMaxLen);
BOOL _fastcall match_dotted_identifier(char** pString, char* pBuffer, int nMaxLen);
BOOL _fastcall match_quoted_identifier(char** pString, char* pBuffer, int nMaxLen);
BOOL _fastcall match_quoted_identifier_ex(char** pString, char* pBuffer, int nMaxLen);
BOOL _fastcall match_str(char** pString, char* pSearch);
BOOL _fastcall match_istr(char** pString, char* pSearch);
BOOL _fastcall match_chr(char** pString, char pChar);
BOOL _fastcall match_one_chr(char** pString, char* pChars, char* pFound);
BOOL _fastcall match_int(char** pString, int* nInt);
BOOL _fastcall match_short(char** pString, short* nShort);

char* _fastcall str_append(char* pBuffer, char* pString);
int _fastcall str_charcount(char* pString, char pChar);

#endif // _VFP2CSTRING_H__