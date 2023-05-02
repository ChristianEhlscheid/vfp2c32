#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <mmintrin.h>

#include "vfp2c32.h"

// returns position of first occurrence of character in some string
// to map character positions to an array dimension (e.g. in AWindowsEx) 
unsigned short _fastcall CharPosN(const char* pString, const char pSearched, unsigned short nMaxPos)
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
unsigned int _fastcall wstrnlen(const wchar_t* pString, unsigned int nMaxLen)
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
unsigned int _fastcall strcpyex(char* pBuffer, const char* pSource)
{
	register const char* pStart = pSource;
	while (*pBuffer++ = *pSource++);
	return pSource - pStart - 1;
}
// copies up to nMaxLen characters from source string to a buffer
// returns count characters copied NOT including the terminating null character
unsigned int _fastcall strncpyex(char* pBuffer, const char* pSource, unsigned int nMaxLen)
{
	register const char* pStart = pBuffer;
	while ((*pBuffer++ = *pSource++) && --nMaxLen);
	if (nMaxLen)
		return pBuffer - pStart - 1;
	else
		return pBuffer - pStart;
}


// returns the no. of tokens in 'pString' which are seperated by 'pSeperator'
unsigned int _fastcall GetWordCount(const char* pString, const char pSeperator)
{
	unsigned int nTokens = 1;
	const char* pSource = pString;
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
unsigned int _fastcall GetWordNumN(char* pBuffer, const char* pString, const char pSeperator, unsigned int nToken, unsigned int nMaxLen)
{
	int nTokenNo = 1;
	const char* pBuffPtr = pBuffer;

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
void _fastcall Alltrim(char* pString)
{
	char* pStart;
	char* pEnd;

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

char* _fastcall AtEx(char* pString, char pSearch, int nOccurence)
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

char* _fastcall ReplicateEx(char* pBuffer, char pExpr, int nCount)
{
	while (nCount--)
		*pBuffer++ = pExpr;
	return pBuffer;
}

char* _fastcall strend(char* pString)
{
	while ((*pString)) pString++;
	return pString;
}

#define CVTBUFSIZE        (309 + 43)

char* _fastcall ShortToStr(char* pString, short nNumber)
{
	char aBuffer[VFP2C_MAX_SHORT_LITERAL];
	char* pTmp = aBuffer;

	if (nNumber < 0)
	{
		*pString++ = '-';
		nNumber = -nNumber;
	}

	if (nNumber != 0)
	{
		unsigned short nNum = (unsigned short)nNumber;
		do
		{
			*pTmp++ = '0' + nNum % 10;
			nNum /= 10;
		} while (nNum != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';

	return pString;
}

char* _fastcall ShortToHex(char* pString, short nNumber)
{
	register char c;
	unsigned short nValue;

	if (nNumber < 0)
	{
		nValue = ~static_cast<unsigned short>(nNumber) + 1;
		*pString++ = '-';
	}
	else
	{
		nValue = static_cast<unsigned short>(nNumber);
	}
	*pString++ = '0';
	*pString = 'x';
	pString += 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;

	return pString + 5;
}

char* _fastcall UShortToHex(char* pString, unsigned short nValue)
{
	register char c;
	*pString++ = '0';
	*pString = 'x';
	pString += 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;

	return pString + 5;
}

char* _fastcall IntToStr(char* pString, int nNumber)
{
	char aBuffer[VFP2C_MAX_INT_LITERAL];
	char* pTmp = aBuffer;

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
		} while (nNum != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';

	return pString;
}

char* _fastcall IntToHex(char* pString, int nNumber)
{
	register char c;
	unsigned int nValue;
	char* pEnd;

	if (nNumber < 0)
	{
		nValue = ~static_cast<unsigned int>(nNumber) + 1;
		*pString++ = '-';
	}
	else
	{
		nValue = static_cast<unsigned int>(nNumber);
	}

	*pString++ = '0';
	*pString = 'x';

	if ((nValue & 0xFFFF0000) > 0)
	{
		pEnd = pString += 8;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;
	}
	else
	{
		pEnd = pString += 4;
	}

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString = c;

	return pEnd + 1;
}

char* _fastcall UIntToStr(char* pString, unsigned int nNumber)
{
	char aBuffer[VFP2C_MAX_INT_LITERAL];
	char* pTmp = aBuffer;

	if (nNumber != 0)
	{
		do
		{
			*pTmp++ = '0' + nNumber % 10;
			nNumber /= 10;
		} while (nNumber != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';

	return pString;
}

char* _fastcall UIntToHex(char* pString, unsigned int nValue)
{
	register char c;
	char* pEnd;

	*pString++ = '0';
	*pString = 'x';

	if ((nValue & 0xFFFF0000) > 0)
	{
		pEnd = pString += 8;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;
	}
	else
	{
		pEnd = pString += 4;
	}

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString = c;

	return pEnd + 1;
}


char* _fastcall Int64ToStr(char* pString, __int64 nNumber)
{
	char aBuffer[VFP2C_MAX_BIGINT_LITERAL];
	char* pTmp = aBuffer;

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
		} while (nNum != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';

	return pString;
}

char* _fastcall Int64ToHex(char* pString, __int64 nNumber)
{
	register char c;
	char* pEnd;
	unsigned __int64 nValue;

	*pString++ = '0';
	*pString = 'x';

	if (nNumber < 0)
	{
		nValue = ~static_cast<unsigned __int64>(nNumber) + 1;
		*pString++ = '-';
	}
	else
	{
		nValue = static_cast<unsigned __int64>(nNumber);
	}

	if ((nValue & 0xFFFFFFFF00000000) > 0)
	{
		pEnd = pString += 16;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;
	}
	else if ((nValue & 0x00000000FFFF0000) > 0)
	{
		pEnd = pString += 8;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
	}
	else
	{
		pEnd = pString += 4;
	}

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString = c;

	return pEnd + 1;
}

union __Int64ToVarbinary
{
	char aBinary[8];
	__int64 nNumber;
};

char* _fastcall Int64ToVarbinary(char* pString, __int64 nNumber)
{
	union __Int64ToVarbinary nInt;
	nInt.nNumber = nNumber;

	const char* HexTable = "0123456789ABCDEF";
	register char* pSource = nInt.aBinary;
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

char* _fastcall Int64ToCurrency(char* pString, __int64 nNumber)
{
	char aBuffer[VFP2C_MAX_BIGINT_LITERAL];
	char* pTmp = aBuffer;

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
		} while (nNum != 0);

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
			while (pTmp != aBuffer + 4) *pString++ = *--pTmp;
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

char* _fastcall UInt64ToStr(char* pString, unsigned __int64 nNumber)
{
	char aBuffer[VFP2C_MAX_BIGINT_LITERAL];
	char* pTmp = aBuffer;

	if (nNumber != 0)
	{
		do
		{
			*pTmp++ = '0' + (char)(nNumber % 10);
			nNumber /= 10;
		} while (nNumber != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';

	return pString;
}

char* _fastcall UInt64ToHex(char* pString, unsigned __int64 nValue)
{
	register char c;
	char* pEnd;
	*pString++ = '0';
	*pString = 'x';

	if ((nValue & 0xFFFFFFFF00000000) > 0)
	{
		pEnd = pString += 16;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;
	}
	else if ((nValue & 0x00000000FFFF0000) > 0)
	{
		pEnd = pString += 8;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
		nValue >>= 4;

		c = (nValue & 0xF) + '0';
		c += -(c > '9') & 0x7;
		*pString-- = c;
	}
	else
	{
		pEnd = pString += 4;
	}

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString-- = c;
	nValue >>= 4;

	c = (nValue & 0xF) + '0';
	c += -(c > '9') & 0x7;
	*pString = c;

	return pEnd;
}
/*
char* _fastcall UInt64ToHexSSE(char* pString, __int64 nNumber)
{
	__m64 mMask = _mm_set1_pi8(0xF);
	__m64 mAsci0 = _mm_set1_pi8('0');
	__m64 mAsci9 = _mm_set1_pi8('9');
	__m64 m7 = _mm_set1_pi8(7);
	__m64 mValue;
	mValue.m64_i64 = nNumber;
	// 1. c & 0xF
	// 2. c + '0';
	// 3. c > '9'
	// 4. mask & 0x7;
	// 5. add 

	__m64 m1 = _mm_and_si64(mMask, mValue); // OR lower bits
	__m64 m2 = _mm_add_pi8(m1, mAsci0); // add '0'
	__m64 m3 = _mm_cmpgt_pi8(m1, mAsci9); // compare greater than 8bit integers
	__m64 m4 = _mm_and_si64(m3, m7); // AND 7
	__m64 m5 = _mm_add_pi8(m1, m4);

	// __m64 _mm_and_si64(__m64 a, __m64 b) // logical and 8bit integers
	// _mm_empty (void) // reset MMX 
}
*/

char* _fastcall BoolToStr(char* pString, int nBool)
{
	*pString++ = '.';
	*pString++ = nBool ? 'T' : 'F';
	*pString++ = '.';
	return pString;
}

char* _fastcall FloatToStr(char* pString, float nValue, int nPrecision)
{
	int decpt, sign, pos;
	char* digits = NULL;
	char cvtbuf[80];
	double nValueEx = (double)nValue;

	digits = cvt(nValueEx, nPrecision, &decpt, &sign, cvtbuf, 0);

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

char* _fastcall DoubleToStr(char* pString, double nValue, int nPrecision)
{
	int decpt, sign, pos;
	char* digits = NULL;
	char cvtbuf[80];

	digits = cvt(nValue, nPrecision, &decpt, &sign, cvtbuf, 0);

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

void _fastcall Int64ToStrBuffer(char* pString, __int64 nNumber)
{
	char aBuffer[VFP2C_MAX_BIGINT_LITERAL];
	char* pTmp = aBuffer;

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
		} while (nNum != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';

	*pString = '\0';
}

#pragma warning(disable : 4290)
__int64 _fastcall StringToInt64(char* pString, unsigned int nLen) throw(int)
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
#pragma warning(default : 4290)

void _fastcall UInt64ToStrBuffer(char* pString, unsigned __int64 nNumber)
{
	char aBuffer[VFP2C_MAX_BIGINT_LITERAL];
	char* pTmp = aBuffer;

	if (nNumber != 0)
	{
		do
		{
			*pTmp++ = '0' + (char)(nNumber % 10);
			nNumber /= 10;
		} while (nNumber != 0);

		while (pTmp != aBuffer) *pString++ = *--pTmp;
	}
	else
		*pString++ = '0';
}

#pragma warning(disable : 4290)
unsigned __int64 _fastcall StringToUInt64(char* pString, unsigned int nLen) throw(int)
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
#pragma warning(default : 4290)

#if !defined(_WIN64)
#define PtrToStr(pString, pPtr)	UIntToStr(pString, (UINT_PTR)pPtr);
#else
#define PtrToStr(pString, pPtr) UInt64ToStr(pString, (UINT_PTR)pPtr);
#endif

#if !defined(_WIN64)
unsigned int _fastcall strnlen(const char* s, unsigned long count)
{
	const char* sc;
	for (sc = s; *sc != '\0' && count--; ++sc);
	return sc - s;
}
#endif

unsigned int _fastcall strdblnlen(const char* s, unsigned long count)
{
	const char* sc = s;

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

unsigned int _fastcall strdblcount(const char* pString, unsigned long nMaxLen)
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

int _fastcall skip_atoi(const char** s)
{
	int i = 0;
	while (IsDigit(**s)) i = i * 10 + *((*s)++) - '0';
	return i;
}

char* _stdcall cvt(double arg, int ndigits, int* decpt, int* sign, char* buf, int eflag)
{
	int rTmp;
	double fi, fj;
	char* p, * pTmp;

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
		*p++ = (int)fj + '0';
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
			++* --pTmp;
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

unsigned int _cdecl sprintfex(char* lpBuffer, const char* lpFormat, ...)
{
	unsigned int nRet;
	va_list lpArgs;
	va_start(lpArgs, lpFormat);
	nRet = printfex(lpBuffer, lpFormat, lpArgs);
	va_end(lpArgs);
	return nRet;
}

unsigned int _stdcall printfex(char* lpBuffer, const char* lpFormat, va_list lpArgs)
{
	char* lpString;
	char* lpStringParm;
	// wchar_t* lpWStringParm;
	CStringView* pStringView;
	CWideStringView* pWideStringView;
	double nDouble;
	int nPrecision, nUseLength, nChars;

	for (lpString = lpBuffer; *lpFormat; lpFormat++)
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
			lpString = IntToStr(lpString, va_arg(lpArgs, int));
			continue;

		case 'U':
			lpString = UIntToStr(lpString, va_arg(lpArgs, unsigned int));
			continue;

		case 'i':
			lpString = IntToStr(lpString, va_arg(lpArgs, short));
			continue;

		case 'u':
			lpString = UIntToStr(lpString, va_arg(lpArgs, unsigned short));
			continue;

		case 'F':
			lpString = DoubleToStr(lpString, va_arg(lpArgs, double), nPrecision);
			continue;

		case 'f':
			nDouble = (double)va_arg(lpArgs, float);
			lpString = DoubleToStr(lpString, nDouble, nPrecision);
			continue;

		case 'b':
			lpString = Int64ToStr(lpString, va_arg(lpArgs, __int64));
			continue;

		case 'B':
			lpString = UInt64ToStr(lpString, va_arg(lpArgs, unsigned __int64));
			continue;

		case 'P':
			lpString = PtrToStr(lpString, va_arg(lpArgs, void*));
			continue;

		case 'L':
			lpString = BoolToStr(lpString, va_arg(lpArgs, int));
			continue;

		case 'V':
			pStringView = va_arg(lpArgs, CStringView*);
			if (pStringView)
			{
				memcpy(lpString, pStringView->Data, pStringView->Len);
				lpString += pStringView->Len;
			}
			continue;

		case 'W':
			pWideStringView = va_arg(lpArgs, CWideStringView*);
			if (pWideStringView && pWideStringView->Len > 0)
			{
				nChars = WideCharToMultiByte(CP_ACP, 0, pWideStringView->Data, pWideStringView->Len, lpString, pWideStringView->Len, 0, 0);
				lpString += nChars;
			}
			continue;

		case 'S':
			lpStringParm = va_arg(lpArgs, char*);
			if (lpStringParm)
			{
				if (!nUseLength)
					while ((*lpStringParm)) *lpString++ = *lpStringParm++;
				else
					while ((*lpStringParm) && nPrecision--) *lpString++ = *lpStringParm++;
			}
			continue;

		case 's':
			*lpString++ = va_arg(lpArgs, char);
			continue;

		case 'T':
			lpStringParm = va_arg(lpArgs, char*);
			nPrecision = va_arg(lpArgs, int);
			if (lpStringParm)
			{
				while ((*lpStringParm) && nPrecision--) *lpString++ = *lpStringParm++;
			}
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

unsigned int _cdecl snprintfex(char* lpBuffer, const char* lpFormat, int nBufferSize, ...)
{
	unsigned int nRet;
	va_list lpArgs;
	va_start(lpArgs, nBufferSize);
	nRet = nprintfex(lpBuffer, lpFormat, nBufferSize, lpArgs);
	va_end(lpArgs);
	return nRet;
}

unsigned int _stdcall nprintfex(char* lpBuffer, const char* lpFormat, int nBufferSize, va_list lpArgs)
{
	char* lpString;
	char* lpStringParm;
	char* lpStringEnd;
	CStringView* pStringView;
	CWideStringView* pWideStringView;
	double nDouble;
	int nPrecision, nUseLength, nChars;

	for (lpString = lpBuffer; *lpFormat; lpFormat++)
	{
		if (nBufferSize <= 0)
		{
			*lpString = '\0';
			return lpString - lpBuffer;
		}

		if (*lpFormat != '%')
		{
			*lpString++ = *lpFormat;
			nBufferSize--;
			continue;
		}

		lpFormat++;

		if (nUseLength = IsDigit(*lpFormat))
		{
			nPrecision = skip_atoi(&lpFormat);
		}

		switch (*lpFormat)
		{
		case 'I':
			if (nBufferSize - VFP2C_MAX_INT_LITERAL > 0)
			{
				lpStringEnd = IntToStr(lpString, va_arg(lpArgs, int));
				nBufferSize -= lpStringEnd - lpString;
				lpString = lpStringEnd;
			}
			else
			{
				nBufferSize = 0;
			}
			continue;

		case 'U':
			if (nBufferSize - VFP2C_MAX_INT_LITERAL > 0)
			{
				lpStringEnd = UIntToStr(lpString, va_arg(lpArgs, unsigned int));
				nBufferSize -= lpStringEnd - lpString;
				lpString = lpStringEnd;
			}
			else
			{
				nBufferSize = 0;
			}
			continue;

		case 'P':
#if !defined(_WIN64)
			if (nBufferSize - VFP2C_MAX_INT_LITERAL > 0)
#else
			if (nBufferSize - VFP2C_MAX_BIGINT_LITERAL > 0)
#endif
			{
				lpStringEnd = PtrToStr(lpString, va_arg(lpArgs, void*));
				nBufferSize -= lpStringEnd - lpString;
				lpString = lpStringEnd;
			}
			else
			{
				nBufferSize = 0;
			}
			continue;

		case 'V':
			pStringView = va_arg(lpArgs, CStringView*);
			if (pStringView && pStringView->Len)
			{
				if (nBufferSize - pStringView->Len > 0)
				{
					memcpy(lpString, pStringView->Data, pStringView->Len);
					lpString += pStringView->Len;
					nBufferSize -= pStringView->Len;
				}
				else
				{
					nBufferSize = 0;
				}
			}
			continue;

		case 'W':
			pWideStringView = va_arg(lpArgs, CWideStringView*);
			if (pWideStringView && pWideStringView->Len)
			{
				if (nBufferSize - pWideStringView->Len > 0)
				{
					nChars = WideCharToMultiByte(CP_ACP, 0, pWideStringView->Data, pWideStringView->Len, lpString, nBufferSize, 0, 0);
					lpString += nChars;
					nBufferSize -= nChars;
				}
				else
				{
					nBufferSize = 0;
				}
			}
			continue;

		case 'S':
			lpStringParm = va_arg(lpArgs, char*);
			if (lpStringParm)
			{
				if (!nUseLength)
				{
					while ((*lpStringParm) && nBufferSize--) *lpString++ = *lpStringParm++;
				}
				else
				{
					while ((*lpStringParm) && nPrecision-- && nBufferSize--) *lpString++ = *lpStringParm++;
				}
			}
			continue;

		case 's':
			nBufferSize--;
			if (nBufferSize > 0)
			{
				*lpString++ = va_arg(lpArgs, char);
			}
			continue;
			
		case 'b':
			if (nBufferSize - VFP2C_MAX_BIGINT_LITERAL > 0)
			{
				lpStringEnd = Int64ToStr(lpString, va_arg(lpArgs, __int64));
				nBufferSize -= lpStringEnd - lpString;
				lpString = lpStringEnd;
			}
			else
			{
				nBufferSize = 0;
			}
			continue;

		case 'B':
			if (nBufferSize - VFP2C_MAX_BIGINT_LITERAL > 0)
			{
				lpStringEnd = UInt64ToStr(lpString, va_arg(lpArgs, unsigned __int64));
				nBufferSize -= lpStringEnd - lpString;
				lpString = lpStringEnd;
			}
			else
			{
				nBufferSize = 0;
			}
			continue;

		case 'L':
			if (nBufferSize - VFP2C_MAX_BOOL_LITERAL > 0)
			{
				lpStringEnd = BoolToStr(lpString, va_arg(lpArgs, int));
				nBufferSize -= lpStringEnd - lpString;
				lpString = lpStringEnd;
			}
			else
			{
				nBufferSize = 0;
			}
			continue;

		case 'F':
			if (nBufferSize - VFP2C_MAX_DOUBLE_LITERAL > 0)
			{
				lpStringEnd = DoubleToStr(lpString, va_arg(lpArgs, double), nUseLength ? nPrecision : 6);
				nBufferSize -= lpStringEnd - lpString;
				lpString = lpStringEnd;
			}
			else
			{
				nBufferSize = 0;
			}
			continue;

		case 'f':
			if (nBufferSize - VFP2C_MAX_DOUBLE_LITERAL > 0)
			{
				nDouble = (double)va_arg(lpArgs, float);
				lpStringEnd = DoubleToStr(lpString, nDouble, nUseLength ? nPrecision : 6);
				nBufferSize -= lpStringEnd - lpString;
				lpString = lpStringEnd;
			}
			else
			{
				nBufferSize = 0;
			}
			continue;

		case 'i':
			if (nBufferSize - VFP2C_MAX_SHORT_LITERAL > 0)
			{
				lpStringEnd = ShortToStr(lpString, va_arg(lpArgs, short));
				nBufferSize -= lpStringEnd - lpString;
				lpString = lpStringEnd;
			}
			else
			{
				nBufferSize = 0;
			}
			continue;

		case 'u':
			if (nBufferSize - VFP2C_MAX_SHORT_LITERAL > 0)
			{
				lpStringEnd = UIntToStr(lpString, va_arg(lpArgs, unsigned short));
				nBufferSize -= lpStringEnd - lpString;
				lpString = lpStringEnd;
			}
			else
			{
				nBufferSize = 0;
			}
			continue;

		default:
			if (*lpFormat != '%')
			{
				*lpString++ = '%';
				nBufferSize--;
			}
			if (*lpFormat)
			{
				*lpString++ = *lpFormat;
				nBufferSize--;
			}
			else
			{
				--lpFormat;
			}
			continue;
		}
	}

	*lpString = '\0';
	return lpString - lpBuffer;
}

// skips over whitespace (space & tab)
void _fastcall skip_ws(char** pString)
{
	char* pStringEx = *pString;
	while (*pStringEx == ' ' || *pStringEx == '\t') pStringEx++;
	*pString = pStringEx;
}

// matches an identifier of up to nMaxLen and stores it into pBuffer
BOOL _fastcall match_identifier(char** pString, char* pBuffer, int nMaxLen)
{
	char* pStringEx;

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

BOOL _fastcall match_dotted_identifier(char** pString, char* pBuffer, int nMaxLen)
{
	char* pStringEx;

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
BOOL _fastcall match_quoted_identifier(char** pString, char* pBuffer, int nMaxLen)
{
	char* pStringEx;
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
BOOL _fastcall match_quoted_identifier_ex(char** pString, char* pBuffer, int nMaxLen)
{
	char* pStringEx;
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
BOOL _fastcall match_str(char** pString, char* pSearch)
{
	char* pStringEx;
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
BOOL _fastcall match_istr(char** pString, char* pSearch)
{
	char* pStringEx;

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
BOOL _fastcall match_chr(char** pString, char pChar)
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
BOOL _fastcall match_one_chr(char** pString, char* pChars, char* pFound)
{
	char* pStringEx;
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
BOOL _fastcall match_int(char** pString, int* nInt)
{
	char* pStringEx;
	char aBuffer[VFP2C_MAX_INT_LITERAL];
	char* pTmp = aBuffer, nBuffLen = VFP2C_MAX_INT_LITERAL;

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
BOOL _fastcall match_short(char** pString, short* nShort)
{
	char* pStringEx;
	char aBuffer[VFP2C_MAX_SHORT_LITERAL];
	char* pTmp = aBuffer, nBuffLen = VFP2C_MAX_SHORT_LITERAL;

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
char* _fastcall str_append(char* pBuffer, char* pString)
{
	while (*pString)
		*pBuffer++ = *pString++;

	*pBuffer = '\0';
	return pBuffer;
}

// returns the number of occurences of the character pChar in pString
int _fastcall str_charcount(char* pString, char pChar)
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