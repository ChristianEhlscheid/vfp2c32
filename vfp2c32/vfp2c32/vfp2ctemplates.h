#ifndef _VFP2CTEMPLATES_H__
#define _VFP2CTEMPLATES_H__

#include <assert.h>

// forward declarations
class FoxString;
class FoxArray;
struct ValueEx;

#pragma warning(disable : 4244)

typedef struct CStringView
{
	CStringView() { Data = 0; Len = 0; };
	CStringView(int nLen) { assert(nLen == 0); Data = 0; Len = 0; }
	CStringView(char* pString) { Data = pString; Len = pString ? strlen(pString) : 0; }
	CStringView(const char* pString) { Data = const_cast<char*>(pString); Len = pString ? strlen(pString) : 0; }
	CStringView(char* pString, unsigned nLen) { Data = pString; Len = nLen; }

	unsigned int SizeOf()
	{
		return sizeof(Data) + sizeof(Len) + Len;
	}

	CStringView Alltrim(const char pWhitespace = ' ')
	{
		if (Data == 0 || Len == 0)
			return 0;
		char* pStart = Data;
		unsigned int nLength = Len;
		while (*pStart == pWhitespace && nLength > 0)
		{
			pStart++;
			nLength--;
		}
		if (nLength > 0)
		{
			char* pEnd = pStart + nLength - 1;
			while (*pEnd == pWhitespace && nLength > 0)
			{
				pEnd--;
				nLength--;
			}
		}
		return CStringView(pStart, nLength);
	}

	CStringView GetWordNum(unsigned int nWordnum, const char pSeperator)
	{
		if (Data == 0 || Len == 0)
			return 0;
		unsigned int nCurrentWord = 1;
		unsigned int nLen = Len;
		char* pWordStart = Data;
		char* pString = Data;
		while (nLen--)
		{
			if (*pString == pSeperator)
			{
				if (nCurrentWord == nWordnum)
					return CStringView(pWordStart, pString - pWordStart);
				nCurrentWord++;
				pWordStart = pString + 1;
			}
			pString++;
		}

		if (nCurrentWord == nWordnum)
			return CStringView(pWordStart, pString - pWordStart);
		else
			return 0;
	}

	unsigned int GetWordCount(const char pSeperator) const
	{
		unsigned int nTokens = 1;
		unsigned int nLen = Len;
		const char* pString = Data;
		while (nLen--)
		{
			if (*pString++ == pSeperator)
				nTokens++;
		}
		if (pString != Data)
			return nTokens;
		else
			return 0;
	}

	CStringView operator+(unsigned int nLen) {
		if (nLen < Len)
			return CStringView(Data + nLen, Len - nLen);
		else
			return 0;
	}

	bool operator==(CStringView pString) { 
		if (Data && pString.Data)
		{
			if (Len != pString.Len)
				return false;
			return memcmp(Data, pString.Data, pString.Len) == 0;
		}
		else if (Data == 0 && pString.Data == 0)
			return true;
		return false;
	}

	bool ICompare(CStringView pString)
	{
		if (Data && pString.Data)
		{
			if (Len != pString.Len)
				return false;

			unsigned char* pStr1 = reinterpret_cast<unsigned char*>(Data);
			unsigned char* pStr2 = reinterpret_cast<unsigned char*>(pString.Data);
			unsigned char pChar1, pChar2;
			for (unsigned int xj = 0; xj < Len; xj++)
			{
				pChar1 = *pStr1;
				pChar2 = *pStr2;
				if (pChar1 >= 'A' && pChar1 <= 'Z')
					pChar1 = 'a' + (pChar1 - 'A');
				if (pChar2 >= 'A' && pChar2 <= 'Z')
					pChar2 = 'a' + (pChar2 - 'A');
				if (pChar1 != pChar2)
					return false;

				pStr1++;
				pStr2++;
			}
			return true;
		}
		else if (Data == 0 && pString.Data == 0)
			return true;
		return false;
	}

	unsigned int BlockSize()
	{
		return sizeof(unsigned int) + 1 + Len;
	}

	void ToBlock(char* pAddress)
	{
		unsigned int* pLen = reinterpret_cast<unsigned int*>(pAddress);
		*pLen = Len;
		if (Len)
		{
			char* pData = (pAddress + 4);
			memcpy(pData, Data, Len);
			pData[Len] = '\0';
		}
	}

	void FromBlock(char* pAddress)
	{
		unsigned int* pLen = reinterpret_cast<unsigned int*>(pAddress);
		Len = *pLen;
		if (Len)
			Data = (pAddress + 4);
		else
			Data = 0;
	}

	operator bool() const { return Data && Len; }
	bool operator!() { return Data == 0 || Len == 0; }

	char* Data;
	unsigned int Len;
} CStringView;

typedef struct CWideStringView
{
	CWideStringView() { Data = 0; Len = 0; };
	CWideStringView(int nLen) { assert(nLen == 0); Data = 0; Len = 0; }
	CWideStringView(wchar_t* pString) { Data = pString; Len = pString ? wcslen(pString) : 0; }
	CWideStringView(wchar_t* pString, unsigned int nLen) { Data = pString; Len = nLen; }

	CWideStringView Alltrim(const wchar_t pWhitespace = ' ')
	{
		if (Data == 0 || Len == 0)
			return 0;
		wchar_t* pStart = Data;
		unsigned int nLength = Len;
		while (*pStart == pWhitespace && nLength > 0)
		{
			pStart++;
			nLength--;
		}
		if (nLength > 0)
		{
			wchar_t* pEnd = pStart + nLength - 1;
			while (*pEnd == pWhitespace && nLength > 0)
			{
				pEnd--;
				nLength--;
			}
		}
		return CWideStringView(pStart, nLength);
	}

	CWideStringView operator+(unsigned int nLen)
	{ 
		if (nLen < Len)
			return CWideStringView(Data + nLen, Len - nLen);
		else
			return 0;
	}

	bool operator==(CWideStringView pString) {
		if (Data && pString.Data)
		{
			if (Len != pString.Len)
				return false;
			return memcmp(Data, pString.Data, pString.Len * sizeof(wchar_t)) == 0;
		}
		else if (Data == 0 && pString.Data == 0)
			return true;
		return false;
	}

	operator bool() const { return Data && Len; }
	bool operator!() const { return Data == 0 || Len == 0; }

	unsigned int Len;
	wchar_t* Data;
} CWideStringView;


template<unsigned int nBufferSize>
class CStrBuilder
{
public:
	CStrBuilder() : m_Length(0), m_Base(0) { m_String[0] = '\0'; }

	unsigned int Len() const { return m_Length; }

	CStrBuilder& Len(unsigned int nLen, bool bNullTerminate = false)
	{
		assert(nLen < nBufferSize);
		m_Length = nLen;
		if (bNullTerminate)
			m_String[m_Length] = '\0';
		return *this;
	}

	unsigned int Size() const { return nBufferSize; }

	CStrBuilder& SetFormatBase(int nLen = -1)
	{
		if (nLen == -1)
			m_Base = m_Length;
		else
			m_Base = nLen;
		return *this;
	}

	unsigned int GetFormatBase()
	{
		return m_Base;
	}

	CStrBuilder& ResetToFormatBase()
	{
		m_Length = m_Base;
		m_String[m_Base] = '\0';
		return *this;
	}

	CStrBuilder& Append(ValueEx& pValue)
	{ 
		assert(pValue.ev_type == 'C' && pValue.ev_handle != 0);
		if (pValue.Len() >= nBufferSize - m_Length)
			throw E_INSUFMEMORY;
		memcpy(m_String + m_Length, pValue.HandleToPtr(), pValue.Len());
		m_Length += pValue.Len();
		m_String[m_Length] = '\0';
		return *this;
	}

	CStrBuilder& Append(const char pString)
	{
		if (sizeof(char) >= nBufferSize - m_Length)
			throw E_INSUFMEMORY;
		m_String[m_Length] = pString;
		m_Length++;
		m_String[m_Length] = '\0';
		return *this;
	}

	CStrBuilder& Append(const CStringView pString)
	{
		if (pString.Len >= nBufferSize - m_Length)
			throw E_INSUFMEMORY;
		memcpy(m_String + m_Length, pString.Data, pString.Len);
		m_Length += pString.Len;
		m_String[m_Length] = '\0';
		return *this;
	}

	CStrBuilder& Format(const char* format, ...)
	{
		va_list lpArgs;
		va_start(lpArgs, format);
		m_Length = nprintfex(m_String, format, nBufferSize, lpArgs);
		va_end(lpArgs);
		return *this;
	}

	CStrBuilder& AppendFormat(const char* format, ...)
	{
		va_list lpArgs;
		va_start(lpArgs, format);
		m_Length += nprintfex(m_String + m_Length, format, nBufferSize - m_Length, lpArgs);
		va_end(lpArgs);
		return *this;
	}

	CStrBuilder& AppendFormatBase(const char* format, ...)
	{
		va_list lpArgs;
		va_start(lpArgs, format);
		m_Length = m_Base + nprintfex(m_String + m_Base, format, nBufferSize - m_Base, lpArgs);
		va_end(lpArgs);
		return *this;
	}

	CStrBuilder& AppendFromBase(const CStringView pString)
	{
		if (pString.Len >= nBufferSize - m_Base)
			throw E_INSUFMEMORY;
		memcpy(m_String + m_Base, pString.Data, pString.Len);
		m_Length = m_Base + pString.Len;
		m_String[m_Length] = '\0';
		return *this;
	}

	CStrBuilder& Prepend(const CStringView pString)
	{
		if (pString.Len + m_Length >= nBufferSize)
			throw E_INSUFMEMORY;
		memmove(m_String + pString.Len, m_String, m_Length + 1);
		memcpy(m_String, pString.Data, len);
		m_Length += len;
		return *this;
	}

	bool StartsWith(const CStringView pString)
	{
		return pString.Len <= m_Length && memcmp(pString.Data, m_String, pString.Len) == 0;
	}

	CStrBuilder& PrependIfNotPresent(const CStringView pString)
	{
		if (pString.Len < m_Length || memcmp(pString.Data, m_String, pString.Len) != 0)
		{
			if (pString.Len + m_Length >= nBufferSize)
				throw E_INSUFMEMORY;
			memmove(m_String + pString.Len, m_String, m_Length + 1);
			memcpy(m_String, pString.Data, pString.Len);
			m_Length += pString.Len;
		}
		return *this;
	}

	CStrBuilder& Alltrim(const char cWhitespace = ' ')
	{
		if (m_Length == 0)
			return *this;

		char* pStart, * pEnd;
		pStart = m_String;
		pEnd = m_String + m_Length;	/* compute end of string */
		unsigned int nLen = m_Length;
		unsigned int nNewLen;

		while (*pStart == cWhitespace && nLen--)
			pStart++; /* skip over spaces at beginning of string */

		if (pStart == pEnd) /* entire string consisted of spaces */
		{
			*m_String = '\0';
			m_Length = 0;
			return *this;
		}

		/* find end of valid characters */
		while (*pEnd == cWhitespace && pEnd > pStart)
			pEnd--;

		/* need to move string back */
		if (pStart != m_String)
		{
			nNewLen = pEnd - pStart;
			memmove(m_String, pStart, nNewLen);
			*++pEnd = '\0';
			m_Length = nNewLen; // set new length
		}
		else if (pEnd != m_String + m_Length)
		{
			nNewLen = pEnd - pStart;
			*++pEnd = '\0';	/* just set nullterminator */
			m_Length = nNewLen; // set 
		}
		return *this;
	}

	CStrBuilder& AddBs()
	{
		unsigned int nLen = m_Length;
		if (nLen && m_String[nLen - 1] != '\\')
		{
			if (nLen + 1 > nBufferSize)
				throw E_INSUFMEMORY;
			m_String[nLen] = '\\';
			m_String[nLen + 1] = '\0';
			m_Length++;
		}
		return *this;
	}

	CStrBuilder& AddBsWildcard()
	{
		if (m_Length)
		{
			unsigned int nLen = m_Length;
			if (nLen > 1 && m_String[nLen - 2] == '\\' && m_String[nLen - 1] == '*')
				return *this;

			if (m_String[nLen - 1] == '\\')
			{
				if (nLen + 1 > nBufferSize)
					throw E_INSUFMEMORY;
				m_String[nLen] = '*';
				m_String[nLen + 1] = '\0';
				Len(nLen + 1);
			}
			else
			{
				if (nLen + 2 > nBufferSize)
					throw E_INSUFMEMORY;
				m_String[nLen] = '\\';
				m_String[nLen + 1] = '*';
				m_String[nLen + 2] = '\0';
				Len(nLen + 2);
			}
		}
		return *this;
	}

	CStrBuilder& LongPathName()
	{
		DWORD count = GetLongPathName(m_String, m_String, nBufferSize);
		if (count >= nBufferSize)
			throw E_INSUFMEMORY;
		m_Length = count;
		return *this;
	}

	CStrBuilder& RemoveLastPath()
	{
		if (m_Length == 0)
			return *this;
		int nPos = m_Length;
		if (m_String[nPos] == '\\')
			nPos--;
		while (nPos > 0 && m_String[nPos] != '\\')
			nPos--;
		m_String[nPos] = '\0';
		m_Length = nPos;
		return *this;
	}

	CStrBuilder& ShiftLeft(unsigned int nLen)
	{
		if (nLen >= m_Length)
		{
			m_String[0] = '\0';
			m_Length = 0;
		}
		else
		{
			memmove(m_String, m_String + nLen, m_Length - nLen + 1);
			m_Length -= nLen;
		}
		return *this;
	}

	CStrBuilder& PathStripPath()
	{
		::PathStripPath(m_String);
		m_Length = strlen(m_String);
		return *this;
	}

	bool CompareToBase(const CStringView pString)
	{
		if (m_Base != pString.Len)
			return false;
		else
			return memcmp(m_String, pString.Data, pString.Len) == 0;
	}

	char* Strdup() const
	{
		char* pString = (char*)new char[m_Length + 1];
		if (pString)
			memcpy(pString, m_String, m_Length + 1);
		return pString;
	}

	CStrBuilder& Marshal(bool nBool)
	{
		char* pString = m_String + m_Length;
		*pString++ = '.';
		*pString++ = nBool ? 'T' : 'F';
		*pString = '.';
		m_Length += 3;
		return *this;
	}

	CStrBuilder& Marshal(short nNumber)
	{
		char* pString = m_String + m_Length;
		register char c;
		unsigned short nValue;

		if (nNumber < 0)
		{
			nValue = ~static_cast<unsigned short>(nNumber) + 1;
			m_Length += 7;
			*pString++ = '-';
		}
		else
		{
			nValue = static_cast<unsigned short>(nNumber);
			m_Length += 6;
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
		*pString = c;
		return *this;
	}

	CStrBuilder& Marshal(unsigned short nValue)
	{
		char* pString = m_String + m_Length;
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
		*pString = c;
		return *this;
	}

	CStrBuilder& Marshal(int nNumber)	
	{
		char* pString = m_String + m_Length;
		register char c;
		unsigned int nValue;

		if (nNumber < 0)
		{
			m_Length++;
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
			m_Length += 10;
			pString += 8;

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
			m_Length += 6;
			pString += 4;
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
		return *this;
	}

	CStrBuilder& Marshal(unsigned int nValue)
	{
		char* pString = m_String + m_Length;
		register char c;
		*pString++ = '0';
		*pString = 'x';

		if ((nValue & 0xFFFF0000) > 0)
		{
			m_Length += 10;
			pString += 8;

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
			m_Length += 6;
			pString += 4;
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
		return *this;
	}

	CStrBuilder& Marshal(long nNumber)
	{
		return Marshal(static_cast<int>(nNumber));
	}

	CStrBuilder& Marshal(unsigned long nNumber)
	{
		return Marshal(static_cast<unsigned int>(nNumber));
	}

	CStrBuilder& Marshal(__int64 nNumber)
	{
		char* pString = m_String + m_Length;
		register char c;
		unsigned __int64 nValue;

		*pString++ = '0';
		*pString = 'x';

		if (nNumber < 0)
		{
			m_Length++;
			nValue = ~static_cast<unsigned __int64>(nNumber) + 1;
			*pString++ = '-';
		}
		else
		{
			nValue = static_cast<unsigned __int64>(nNumber);
		}

		if ((nValue & 0xFFFFFFFF00000000) > 0)
		{
			m_Length += 18;
			pString += 16;

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
			m_Length += 10;
			pString += 8;

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
			m_Length += 6;
			pString += 4;
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
		return *this;
	}

	CStrBuilder& Marshal(unsigned __int64 nValue)
	{
		char* pString = m_String + m_Length;
		register char c;
		*pString++ = '0';
		*pString = 'x';

		if ((nValue & 0xFFFFFFFF00000000) > 0)
		{
			m_Length += 18;
			pString += 16;

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
			m_Length += 10;
			pString += 8;

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
			m_Length += 6;
			pString += 4;
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

		return *this;
	}

	CStrBuilder& Marshal(float nValue, int nPrecision = 6)
	{
		int decpt, sign, pos;
		char* digits = NULL;
		char cvtbuf[80];
		double nValueEx = (double)nValue;
		char* pString = m_String + m_Length;

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

		m_Length = pString - m_String;
		return *this;
	}

	CStrBuilder& Marshal(double nValue, int nPrecision = 6)
	{
		int decpt, sign, pos;
		char* digits = NULL;
		char cvtbuf[80];
		char* pString = m_String + m_Length;

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

		m_Length = pString - m_String;
		return *this;
	}

	CStrBuilder& Marshal(FoxValue* pValue)
	{
		return Append("ReadFoxValue(").Marshal(reinterpret_cast<void*>(pValue)).Append(')');
	}

	CStrBuilder& Marshal(void* pData)
	{
		return Marshal(reinterpret_cast<ULONG_PTR>(pData));
	}

	CStrBuilder& Marshal(const CStringView& pString)
	{
		return Append("ReadBytes(").Marshal(reinterpret_cast<void*>(pString.Data)).Append(',').Marshal(pString.Len).Append(')');
	}

	CStrBuilder& Marshal(const CWideStringView& pString)
	{
		return Append("ReadWString(").Marshal(reinterpret_cast<void*>(pString.Data)).Append(')');
	}

	CStrBuilder& operator=(const CStringView& pString)
	{
		if (pString)
		{
			if (pString.Data == m_String)
				return *this;
			if (pString.Len >= nBufferSize)
				throw E_INSUFMEMORY;
			memcpy(m_String, pString.Data, pString.Len);
			m_Length = pString.Len;
			m_String[m_Length] = '\0';
		}
		else
		{
			m_Length = 0;
			m_String[0] = '\0';
		}
		return *this;
	}

	CStrBuilder& operator=(const CWideStringView& pString)
	{
		if (pString)
		{
			if (pString.Len >= nBufferSize)
					throw E_INSUFMEMORY;

			int nChars = WideCharToMultiByte(CP_ACP, 0, pString.Data, pString.Len, m_String, nBufferSize, 0, 0);
			if (!nChars)
			{
				SaveWin32Error("MultiByteToWideChar", GetLastError());
				throw E_APIERROR;
			}
			m_String[nChars] = '\0';
			m_Length = nChars;
		}
		else
		{
			m_Length = 0;
			m_String[0] = '\0';
		}
		return *this;
	}

	CStrBuilder& operator+=(const CStringView& pString)
	{
		if (pString)
		{
			if (m_Length + pString.Len >= nBufferSize)
				throw E_INSUFMEMORY;

			memcpy(m_String + m_Length, pString.Data, pString.Len + 1);
			m_Length += pString.Len;
		}
		return *this;
	}

	CStrBuilder& operator+=(const CWideStringView& pString)
	{
		if (pString)
		{
			if (m_Length + pString.Len >= nBufferSize)
				throw E_INSUFMEMORY;

			int nChars = WideCharToMultiByte(CP_ACP, 0, pString.Data, pString.Len, m_String + m_Length, nBufferSize - m_Length, 0, 0);
			if (!nChars)
			{
				SaveWin32Error("MultiByteToWideChar", GetLastError());
				throw E_APIERROR;
			}
			m_Length += nChars;
			m_String[m_Length] = '\0';
		}
		return *this;
	}

	bool operator==(CStringView& pString) const
	{
		if (pString.Data == m_String && m_Length == pString.Len)
			return true;
		if (m_Length != pString.Len)
			return false;
		if (pString.Data)
			return strcmp(m_String, pString.Data) == 0;
		return false;
	}

	void* Ptr() const
	{
		return reinterpret_cast<void*>(m_String);
	}
	
	template<typename T>
	T Ptr() const
	{
		return reinterpret_cast<T>(m_String);
	}

	char& operator[](int nIndex) { return m_String[nIndex]; }
	char& operator[](unsigned long nIndex) { return m_String[nIndex]; }

	operator char* () { return reinterpret_cast<char*>(m_String); }
	operator unsigned char* () { return reinterpret_cast<unsigned char*>(m_String); }
	operator const char* () const { return reinterpret_cast<const char*>(m_String); }
	operator const unsigned char* () const { return reinterpret_cast<const unsigned char*>(m_String); }
	operator const CStringView() const { return CStringView(const_cast<char*>(reinterpret_cast<const char*>(m_String)), m_Length ); }

private:
	unsigned int m_Length;
	unsigned int m_Base;
	char m_String[nBufferSize];
};

template<class T>
class ComPtr
{
public:
	ComPtr() : m_Pointer(0) {}
	~ComPtr() { if (m_Pointer) m_Pointer->Release(); }

	operator T() { return m_Pointer; }
	operator T* () { return &m_Pointer; }
	T operator->() { return m_Pointer; }
	T operator=(int nValue) { assert(nValue == 0);  if (m_Pointer) m_Pointer->Release(); m_Pointer = 0; return m_Pointer; }
	T operator=(T pValue) { if (m_Pointer) m_Pointer->Release(); m_Pointer = pValue; return m_Pointer; }
	T Detach() { T pPointer = m_Pointer; m_Pointer = 0; return pPointer; }

private:
	T m_Pointer;
};

template<class T, int count>
class ComPtrArray
{
public:
	ComPtrArray() { ZeroMemory(m_Pointer, sizeof(m_Pointer)); }
	~ComPtrArray() { for (int xj = 0; xj < count; xj++) if (m_Pointer[xj]) m_Pointer[xj]->Release(); }

	T operator[](int nIndex) { return m_Pointer[nIndex]; }
	operator T* () { return m_Pointer; }

private:
	T m_Pointer[count];
};

#endif // _VFP2CTEMPLATES_H__
