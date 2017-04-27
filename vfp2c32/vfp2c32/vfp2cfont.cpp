#include <windows.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2cfont.h"
#include "vfp2ccppapi.h"
#include "vfp2chelpers.h"
#include "vfpmacros.h"

void _fastcall AFontInfo(ParamBlk *parm)
{
try
{
	FoxString pFileName(p1);
	FoxObject pFontInfo;
	pFontInfo.EmptyObject();

	LANGID dwLanguage;
	USHORT dwPlatform;

	if (PCount() < 2 || Vartype(p2) == '0')
		dwLanguage = GetSystemDefaultLangID();

	if (PCount() < 3 || Vartype(p3) == '0')
		dwPlatform = PLATFORMID_WINDOWS; // default to Windows platform

	BOOL bApiRet;
	DWORD dwRetVal, dwRead;
	ApiHandle hFile;

	hFile = CreateFile(pFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (!hFile)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	TT_TABLE_OFFSET ttOffsetTable;
	bApiRet = ReadFile(hFile, &ttOffsetTable, sizeof(TT_TABLE_OFFSET), &dwRead, 0);
	if (!bApiRet)
	{
		SaveWin32Error("ReadFile", GetLastError());
		throw E_APIERROR;
	}

	ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
	ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
	ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);
	//check if this is a TrueType font and the version is 1.0
	if( ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0)
	{
		FoxValue vNull;
		Return(vNull);
		return;
	}

	TT_TABLE_DIRECTORY *tblDir;
	bool bFound = false;
	CBuffer pTableBuffer(sizeof(TT_TABLE_DIRECTORY) * ttOffsetTable.uNumOfTables);
	FoxString pNameBuffer(5);
	ULONG OffsetOS2 = 0, OffsetName = 0, OffsetHead = 0, OffsetPost = 0;

	bApiRet = ReadFile(hFile, pTableBuffer, pTableBuffer.Size(), &dwRead, 0);
	if (!bApiRet)
	{
		SaveWin32Error("ReadFile", GetLastError());
		throw E_APIERROR;
	}

	tblDir = (TT_TABLE_DIRECTORY*)pTableBuffer.Address();

	for(int i=0; i< ttOffsetTable.uNumOfTables; i++)
	{
		//the table's tag cannot exceed 4 characters
		pNameBuffer.StrnCpy(tblDir->szTag, 4);

		if (pNameBuffer.ICompare("OS/2"))
			OffsetOS2 = SWAPLONG(tblDir->uOffset);
		else if (pNameBuffer.ICompare("name"))
			OffsetName = SWAPLONG(tblDir->uOffset);
		else if (pNameBuffer.ICompare("head"))
			OffsetHead = SWAPLONG(tblDir->uOffset);
		else if (pNameBuffer.ICompare("post"))
			OffsetPost = SWAPLONG(tblDir->uOffset);

		tblDir++;
	}

	// OS2 table found
	USHORT fStyle = 0;
	SHORT yStrikeoutPosition = 0, yStrikeoutSize;

	if (OffsetOS2)
	{
		//move to offset we got from Offsets Table
        dwRetVal = SetFilePointer(hFile, OffsetOS2, 0, FILE_BEGIN);
        if (dwRetVal == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		{
			SaveWin32Error("SetFilePointer", GetLastError());
			throw E_APIERROR;
		}

		TT_OS2_TABLE ttOS2;
		bApiRet = ReadFile(hFile, &ttOS2, sizeof(TT_OS2_TABLE), &dwRead, 0);
		if (!bApiRet)
		{
			SaveWin32Error("ReadFile", GetLastError());
			throw E_APIERROR;
		}

		fStyle = SWAPWORD(ttOS2.fsSelection);
		yStrikeoutPosition = SWAPWORD(ttOS2.yStrikeoutPosition);
		yStrikeoutSize = SWAPWORD(ttOS2.yStrikeoutSize);
	}

	pFontInfo("strikeoutposition") << yStrikeoutPosition;
	pFontInfo("strikeoutsize") << yStrikeoutSize;
	pFontInfo("italic") << ((fStyle & 1) > 0); // italic
	pFontInfo("underscore") << ((fStyle & 2) > 0); // underscore
	pFontInfo("negative") << ((fStyle & 4) > 0); // negative
	pFontInfo("outline") << ((fStyle & 8) > 0); // outline
	pFontInfo("strikeout") << ((fStyle & 16) > 0); // strikeout
	pFontInfo("bold") << ((fStyle & 32) > 0); // bold
	pFontInfo("regular") << ((fStyle & 64) > 0); // regular
	pFontInfo("typometrics") << ((fStyle & 128) > 0); // use typo metrics
	pFontInfo("wws") << ((fStyle & 256) > 0); // wws The font has ‘name’ table strings consistent with a weight/width/slope family without requiring use of ‘name’ IDs 21 and 22. (Please see more detailed description below.)
	pFontInfo("oblique") << ((fStyle & 512) > 0); // oblique

	// Name table found ?
	if(OffsetName)
	{
		//move to offset we got from Offsets Table
		dwRetVal = SetFilePointer(hFile, OffsetName, 0, FILE_BEGIN);
        if (dwRetVal == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		{
			SaveWin32Error("SetFilePointer", GetLastError());
			throw E_APIERROR;
		}

		TT_NAME_HEADER_TABLE ttNTHeader;
		bApiRet = ReadFile(hFile, &ttNTHeader, sizeof(TT_NAME_HEADER_TABLE), &dwRead, 0);
		if (!bApiRet)
		{
			SaveWin32Error("ReadFile", GetLastError());
			throw E_APIERROR;
		}

		//again, don't forget to swap bytes!
		ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
		ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);

		CBuffer pRecordBuffer(sizeof(TT_NAME_RECORD) * ttNTHeader.uNRCount);
		TT_NAME_RECORD *pRecord = (TT_NAME_RECORD*)pRecordBuffer.Address();

		bApiRet = ReadFile(hFile, pRecordBuffer, pRecordBuffer.Size(), &dwRead, 0);
		if (!bApiRet)
		{
			SaveWin32Error("ReadFile", GetLastError());
			throw E_APIERROR;
		}

		TT_NAME_RECORD *pName = pRecord;
		// fix all structure fields first
		for(int i=0; i < ttNTHeader.uNRCount; i++)
		{
			pName->uPlatformID = SWAPWORD(pName->uPlatformID);
			pName->uLanguageID = SWAPWORD(pName->uLanguageID);
			pName->uEncodingID = SWAPWORD(pName->uEncodingID);
			pName->uNameID = SWAPWORD(pName->uNameID);
			pName->uStringLength = SWAPWORD(pName->uStringLength);
			pName->uStringOffset = SWAPWORD(pName->uStringOffset);
			pName++;
		}
		
		FoxString pNameRecordBuffer;
		FoxString pNameRecordBufferAnsi;
		USHORT nameIDs[6] = {0, 1, 2, 3, 4, 6};
		char nameProps[6][20] = {"copyright", "fontfamily", "fontsubfamily", "uniqueidentifier", "fullname", "postscriptname"};

		for (USHORT xj = 0; xj < sizeof(nameIDs) / sizeof(USHORT); xj++)
		{
			if((pName = GetFontNameRecord(pRecord, ttNTHeader.uNRCount, nameIDs[xj], PLATFORMID_WINDOWS, dwLanguage)) ||
				(pName = GetFontNameRecord(pRecord, ttNTHeader.uNRCount, nameIDs[xj], PLATFORMID_WINDOWS, LANGID_ENGLISH_US)) ||
				(pName = GetFontNameRecord(pRecord, ttNTHeader.uNRCount, nameIDs[xj], PLATFORMID_WINDOWS, 0)))
			{
				
				pNameRecordBuffer.Size(pName->uStringLength);

				dwRetVal = SetFilePointer(hFile, OffsetName + pName->uStringOffset + ttNTHeader.uStorageOffset, 0, FILE_BEGIN);
				if (dwRetVal == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
				{
					SaveWin32Error("SetFilePointer", GetLastError());
					throw E_APIERROR;
				}

				bApiRet = ReadFile(hFile, pNameRecordBuffer, pNameRecordBuffer.Size(), &dwRead, 0);
				if (!bApiRet)
				{
					SaveWin32Error("ReadFile", GetLastError());
					throw E_APIERROR;
				}

				pNameRecordBufferAnsi.Size(pName->uStringLength / 2);
				pNameRecordBufferAnsi.Len(pName->uStringLength / 2);

				BigEndianWideCharToMultiByte(pNameRecordBuffer, pNameRecordBuffer.Size() / 2, pNameRecordBufferAnsi, pNameRecordBufferAnsi.Size());

				pFontInfo(nameProps[xj]) << pNameRecordBufferAnsi;
			}
			else
			{
				pNameRecordBuffer.Len(0);
				pFontInfo(nameProps[xj]) << pNameRecordBuffer;
			}
		}
	}

	if (OffsetHead)
	{
		//move to offset we got from Offsets Table
		dwRetVal = SetFilePointer(hFile, OffsetHead, 0, FILE_BEGIN);
        if (dwRetVal == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		{
			SaveWin32Error("SetFilePointer", GetLastError());
			throw E_APIERROR;
		}

		TT_HEAD_TABLE ttHead;
		bApiRet = ReadFile(hFile, &ttHead, sizeof(TT_HEAD_TABLE), &dwRead, 0);
		if (!bApiRet)
		{
			SaveWin32Error("ReadFile", GetLastError());
			throw E_APIERROR;
		}

		ttHead.unitsPerEm = SWAPWORD(ttHead.unitsPerEm);
		pFontInfo("unitsperem") << ttHead.unitsPerEm;
	}

	if (OffsetPost)
	{
		//move to offset we got from Offsets Table
		dwRetVal = SetFilePointer(hFile, OffsetPost, 0, FILE_BEGIN);
        if (dwRetVal == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		{
			SaveWin32Error("SetFilePointer", GetLastError());
			throw E_APIERROR;
		}

		TT_POST_TABLE ttPost;
		bApiRet = ReadFile(hFile, &ttPost, sizeof(TT_POST_TABLE), &dwRead, 0);
		if (!bApiRet)
		{
			SaveWin32Error("ReadFile", GetLastError());
			throw E_APIERROR;
		}

		ttPost.underlinePosition = SWAPWORD(ttPost.underlinePosition);
		ttPost.underlineThickness = SWAPWORD(ttPost.underlineThickness);
		pFontInfo("underlineposition") << ttPost.underlinePosition;
		pFontInfo("underlinethickness") << ttPost.underlineThickness;
	}

	pFontInfo.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

TT_NAME_RECORD * __stdcall GetFontNameRecord(TT_NAME_RECORD *pRecords, USHORT nCount, USHORT uNameID, USHORT dwPlatform, LANGID dwLanguage)
{
	TT_NAME_RECORD *pRec = 0;
	for(unsigned int i=0; i < nCount; i++)
	{
		if (dwPlatform != pRecords->uPlatformID || (dwLanguage && pRecords->uLanguageID != dwLanguage)) /* || ttRecord->uPlatformID != dwPlatform */
		{
			pRecords++;
			continue;
		}

		if (uNameID == pRecords->uNameID)
		{
			pRec = pRecords;
			break;
		}
		pRecords++;
	}
	return pRec;
}

void __stdcall BigEndianWideCharToMultiByte(wchar_t *pWString, unsigned int nWideChars, char *pBuffer, unsigned int nBytes)
{
	wchar_t *pTmp = pWString;
	for(unsigned int xj = 0; xj < nWideChars; xj++)
	{
		*pTmp = SWAPWORD(*pTmp);
		pTmp++;
	}
	WideCharToMultiByte(CP_ACP, 0, pWString, nWideChars, pBuffer, nBytes, 0, 0);
}