#define _WINSOCKAPI_ // we're using winsock2 .. so this is neccessary to exclude winsock.h 

#include <windows.h>
#include <stdio.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2cwininet.h"
#include "vfpmacros.h"
#include "vfp2ccppapi.h"
#include "vfp2ccallback.h"

/*

// main WinInet handle
static HINTERNET ghInternet = 0;
static CThreadManager goWinInetThreads;

bool _stdcall VFP2C_Init_WinInet()
{
	return goWinInetThreads.Initialize();
}

void _stdcall VFP2C_Destroy_WinInet()
{
	goWinInetThreads.ShutdownThreads();

	if (ghInternet)
		InternetCloseHandle(ghInternet);
}

// WinInet functions

// Errorhandler
void _stdcall SaveWininetError(char *pFunction, DWORD nLastError)
{
	gnErrorCount = 0;
	gaErrorInfo[gnErrorCount].nErrorType = VFP2C_ERRORTYPE_WIN32;
	strncpy(gaErrorInfo[gnErrorCount].aErrorFunction,pFunction,VFP2C_ERROR_FUNCTION_LEN);

	if (nLastError == ERROR_INTERNET_EXTENDED_ERROR)
	{
		DWORD dwBuffer = VFP2C_ERROR_MESSAGE_LEN;
		InternetGetLastResponseInfo(&gaErrorInfo[gnErrorCount].nErrorNo,
									gaErrorInfo[gnErrorCount].aErrorMessage,&dwBuffer);
	}
	else
	{
		gaErrorInfo[gnErrorCount].nErrorNo = nLastError;
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,nLastError,0,gaErrorInfo[gnErrorCount].aErrorMessage,
			VFP2C_ERROR_MESSAGE_LEN,0);
	}
}

void _fastcall InitWinInet(ParamBlk *parm)
{
try
{
	FoxString pAgent(parm,1);
	DWORD dwFlags = PCount() >= 2 ? p2.ev_long : 0;
	FoxString pProxy(parm,3);
	FoxString pProxyByPass(parm,4);
	DWORD dwAccess;

	if (PCount() >= 5)
		dwAccess = p5.ev_long;
	else if (pProxy > 0 || pProxyByPass > 0)
		dwAccess = INTERNET_OPEN_TYPE_PROXY;
	else
		dwAccess = INTERNET_OPEN_TYPE_DIRECT;

	if (dwFlags & INTERNET_FLAG_ASYNC)
		throw E_INVALIDPARAMS;

	if (ghInternet)
	{
		if (!InternetCloseHandle(ghInternet))
		{
			SaveWininetError("InternetCloseHandle", GetLastError());
			throw E_APIERROR;
		}
		ghInternet = 0;
	}

	if (!pAgent)
		pAgent = "VFP2C32";

	ghInternet = InternetOpen(pAgent,dwAccess,pProxy,pProxyByPass,dwFlags);
	if (!ghInternet)
	{
		SaveWininetError("InternetOpen", GetLastError());
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall SetWinInetOptions(ParamBlk *parm)
{
try
{
	if (!ghInternet)
	{
		SaveCustomError("WinInetOptions", "Library not initialized.");
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FTPConnect(ParamBlk *parm)
{
try
{
	if (!ghInternet)
	{
		SaveCustomError("FTPConnect","Library not initialized.");
		throw E_APIERROR;
	}

	FoxString pServer(p1);
	FoxString pUser(parm,2);
	FoxString pPassword(parm,3);
	INTERNET_PORT nPort = PCount() >= 4 && p4.ev_long ? (INTERNET_PORT)p4.ev_long : INTERNET_DEFAULT_FTP_PORT;
	DWORD nFlags = PCount() >= 5 ? p5.ev_long : 0;

	HINTERNET hConn = InternetConnect(ghInternet,pServer,nPort,pUser,pPassword,INTERNET_SERVICE_FTP,nFlags,0);
	
	if (!hConn)
	{
		SaveWininetError("InternetOpen", GetLastError());
		throw E_APIERROR;
	}

	Return(hConn);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FTPDisconnect(ParamBlk *parm)
{
try
{
	if (!ghInternet)
	{
		SaveCustomError("FTPDisconnect","Library not initialized.");
		throw E_APIERROR;
	}

	HINTERNET hConn = (HINTERNET)p1.ev_long;
	if (!InternetCloseHandle(hConn))
	{
		SaveWininetError("InternetCloseHandle", GetLastError());
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FTPGetFileLib(ParamBlk *parm)
{
try
{
	if (!ghInternet)
	{
		SaveCustomError("FTPGetFile","Library not initialized.");
		throw E_APIERROR;
	}

	HINTERNET hConn = (HINTERNET)p1.ev_long;
	FoxString pSource(p2);
	FoxString pDest(p3);
	FoxString pCallback(parm,4);
	bool bAsync = p5.ev_length > 0;

	if (pCallback > VFP2C_MAX_CALLBACKFUNCTION)
		throw E_INVALIDPARAMS;

	if (pCallback)
	{
		if (!InternetSetStatusCallback(hConn,WinInetStatusCallback))
		{
			SaveWininetError("InternetSetStatusCallback, GetLastError());
			throw E_APIERROR;
		}
	}
	else
	{
		if (!InternetSetStatusCallback(hConn,0))
		{
			SaveWininetError("InternetSetStatusCallback", GetLastError());
			throw E_APIERROR;
		}
	}

	if (!bAsync)
	{
		if (!FtpGetFile(hConn,pSource,pDest,FALSE,FILE_ATTRIBUTE_NORMAL,
			FTP_TRANSFER_TYPE_BINARY,(DWORD_PTR)pCallbackStruct))
		{
			SaveWininetError("FtpGetFile", GetLastError());
			throw E_APIERROR;
		}
	}
	else
	{


	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FTPPutFileLib(ParamBlk *parm)
{
try
{
	if (!ghInternet)
	{
		SaveCustomError("FTPGetFile","Library not initialized.");
		throw E_APIERROR;
	}


}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _stdcall WinInetStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext,
							DWORD dwInternetStatus, LPVOID lpvStatusInformation,
                            DWORD dwStatusInformationLength)
{

	switch (dwInternetStatus)
	{
		case INTERNET_STATUS_RESPONSE_RECEIVED:
			break;

		case INTERNET_STATUS_REQUEST_COMPLETE:
			break;

		default:
			break;
	}

}

void _fastcall FTPGetDirectory(ParamBlk *parm)
{
try
{
	if (!ghInternet)
	{
		SaveCustomError("FTPGetDirectory","Library not initialized.");
		throw E_APIERROR;
	}

	HINTERNET hConn = (HINTERNET)p1.ev_long;
	DWORD dwLen = MAX_PATH;
    FoxString pDirectory(dwLen);
	if (FtpGetCurrentDirectory(hConn,pDirectory,&dwLen))
		pDirectory.Length(dwLen).Return();
	else
	{
		SaveWininetError("FtpGetCurrentDirectory", GetLastError());
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FTPSetDirectory(ParamBlk *parm)
{
try
{
	if (!ghInternet)
	{
		SaveCustomError("FTPSetDirectory","Library not initialized.");
		throw E_APIERROR;
	}

	HINTERNET hConn = (HINTERNET)p1.ev_long;
    FoxString pDirectory(p2);
	if (!FtpSetCurrentDirectory(hConn,pDirectory))
	{
		SaveWininetError("FtpSetCurrentDirectory", GetLastError());
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AFTPFiles(ParamBlk *parm)
{
	HINTERNET hConn = (HINTERNET)p1.ev_long;
	HINTERNET hSearch = 0;
	int nErrorNo = 0, nFileCount = 0, nDest;
	char *pDestination, *pSearchString, *pFileName;
	DWORD nFlags, nLastError, nFileFilter;
	BOOL bEnumFakeDirs;
	Locator lFileName, lDosFileName, lCreationTime, lAccessTime, lWriteTime, lFileSize, lFileAttribs, lArrayLoc;
	V_STRING(vFileName);
	DATETIME(vTime);
	INT64DOUBLE(vFileSize);
	V_INTEGER(vFileAttribs);
	V_VALUE(vWorkArea);
	PADIREXFILTER fpFilterFunc;
	WIN32_FIND_DATA sFiles;
	char aExeBuffer[VFP2C_MAX_FUNCTIONBUFFER];

	if (!NullTerminateHandle(p2) || !NullTerminateHandle(p3))
		RaiseError(E_INSUFMEMORY);

	LockHandle(p2);
	LockHandle(p3);

	pDestination = HandleToPtr(p2);
	pSearchString = HandleToPtr(p3);
	
	nFlags = PCount() >= 4 ? p4.ev_long : 0;
	nDest = PCount() >= 5 ? p5.ev_long : ADIREX_DEST_ARRAY;

	if (!(nDest & (ADIREX_DEST_ARRAY | ADIREX_DEST_CURSOR)))
		nDest |= ADIREX_DEST_ARRAY;

	nFileFilter = p6.ev_long ? p6.ev_long : ~FILE_ATTRIBUTE_FAKEDIRECTORY;
	bEnumFakeDirs = nFileFilter & FILE_ATTRIBUTE_FAKEDIRECTORY;
	nFileFilter &= ~FILE_ATTRIBUTE_FAKEDIRECTORY;

	if (nDest & ADIREX_FILTER_ALL)
		fpFilterFunc = AdirExFilter_All;
	else if (nDest & ADIREX_FILTER_NONE)
		fpFilterFunc = AdirExFilter_None;
	else if (nDest & ADIREX_FILTER_EXACT)
		fpFilterFunc = AdirExFilter_Exact;
	else
		fpFilterFunc = AdirExFilter_One;

	if (nDest & ADIREX_DEST_ARRAY) // destination is array
	{
		if (nErrorNo = DimensionEx(pDestination,&lArrayLoc,1,7))
			goto ErrorOut;
	} // else if destination is cursor
	else if (nDest & ADIREX_DEST_CURSOR)
	{
		sprintfex(aExeBuffer,"SELECT('%S')",pDestination);
		if (nErrorNo = EVALUATE(vWorkArea,aExeBuffer))
			goto ErrorOut;

		if(vWorkArea.ev_long == 0)
		{
			sprintfex(aExeBuffer,"CREATE CURSOR %S (filename C(254), dosfilename C(13), creationtime T, accesstime T, writetime T, filesize N(20,0), fileattribs I)",pDestination);
			if (nErrorNo = EXECUTE(aExeBuffer))
				goto ErrorOut;

			if (nErrorNo = EVALUATE(vWorkArea,"SELECT(0)"))
				goto ErrorOut;
		}
		
		if ((nErrorNo = FindFoxFieldEx(1,&lFileName,vWorkArea.ev_long)) ||
			(nErrorNo = FindFoxFieldEx(2,&lDosFileName,vWorkArea.ev_long)) ||
			(nErrorNo = FindFoxFieldEx(3,&lCreationTime,vWorkArea.ev_long)) || 
			(nErrorNo = FindFoxFieldEx(4,&lAccessTime,vWorkArea.ev_long)) ||
			(nErrorNo = FindFoxFieldEx(5,&lWriteTime,vWorkArea.ev_long)) ||
			(nErrorNo = FindFoxFieldEx(6,&lFileSize,vWorkArea.ev_long)) ||
			(nErrorNo = FindFoxFieldEx(7,&lFileAttribs,vWorkArea.ev_long)))
				goto ErrorOut;
	}

	if (!AllocHandleEx(vFileName,MAX_PATH))
	{
		nErrorNo = E_INSUFMEMORY;
		goto ErrorOut;
	}
	LockHandle(vFileName);
	pFileName = HandleToPtr(vFileName);

	hSearch = FtpFindFirstFile(hConn,pSearchString,&sFiles,nFlags,0);
	if (!hSearch)
	{
		nLastError = GetLastError();
		if (nLastError == ERROR_NO_MORE_FILES)
			goto Success;

		SaveWininetError("FtpFindFirstFile", nLastError);
		goto ErrorOut;
	}

	if (nDest & ADIREX_DEST_ARRAY) // if destination is array
	{
		do
		{
			if (fpFilterFunc(sFiles.dwFileAttributes,nFileFilter))
			{

			if (bEnumFakeDirs == FALSE && (sFiles.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && ISFAKEDIR(sFiles.cFileName))
				continue;
						
			if (nErrorNo = Dimension(pDestination,++nFileCount,7))
				break;

			AROW(lArrayLoc)++;

			ADIM(lArrayLoc) = 1;
			vFileName.ev_length = strcpyex(pFileName,sFiles.cFileName);
			if (nErrorNo = STORE(lArrayLoc,vFileName))
				goto ErrorOut;

			ADIM(lArrayLoc) = 2;
			vFileName.ev_length = strcpyex(pFileName,sFiles.cAlternateFileName);
			if (nErrorNo = STORE(lArrayLoc,vFileName))
				goto ErrorOut;

			ADIM(lArrayLoc) = 3;
			if (!FileTimeToDateTimeEx(&sFiles.ftCreationTime,&vTime,FALSE))
				goto ErrorOut;
			if (nErrorNo = STORE(lArrayLoc,vTime))
				goto ErrorOut;

			ADIM(lArrayLoc) = 4;
			if (!FileTimeToDateTimeEx(&sFiles.ftLastAccessTime,&vTime,FALSE))
				goto ErrorOut;
			if (nErrorNo = STORE(lArrayLoc,vTime))
				goto ErrorOut;

			ADIM(lArrayLoc) = 5;
			if (!FileTimeToDateTimeEx(&sFiles.ftLastWriteTime,&vTime,FALSE))
				goto ErrorOut;
			if (nErrorNo = STORE(lArrayLoc,vTime))
				goto ErrorOut;

			ADIM(lArrayLoc) = 6;
			vFileSize.ev_real = INTS2DOUBLE(sFiles.nFileSizeLow,sFiles.nFileSizeHigh);
			if (nErrorNo = STORE(lArrayLoc,vFileSize))
				goto ErrorOut;

			ADIM(lArrayLoc) = 7;
			vFileAttribs.ev_long = sFiles.dwFileAttributes;
			if (nErrorNo = STORE(lArrayLoc,vFileAttribs))
				goto ErrorOut;

			}
		} while (InternetFindNextFile(hSearch,&sFiles));
	}
	else	// destination is cursor ...
	{
		do 
		{
			if (fpFilterFunc(sFiles.dwFileAttributes,nFileFilter))
			{

			if (bEnumFakeDirs == FALSE && (sFiles.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && ISFAKEDIR(sFiles.cFileName))
				continue;

			nFileCount++;
			if (nErrorNo = APPENDBLANKW(vWorkArea.ev_long))
				goto ErrorOut;

			vFileName.ev_length = strcpyex(pFileName,sFiles.cFileName);
			if (nErrorNo = _DBReplace(lFileName,vFileName))
				goto ErrorOut;

			vFileName.ev_length = strcpyex(pFileName,sFiles.cAlternateFileName);
			if (nErrorNo = _DBReplace(lDosFileName,vFileName))
				goto ErrorOut;

			if (!FileTimeToDateTimeEx(&sFiles.ftCreationTime,&vTime,FALSE))
				goto ErrorOut;
			if (nErrorNo = _DBReplace(lCreationTime,vTime))
				goto ErrorOut;

			if (!FileTimeToDateTimeEx(&sFiles.ftLastAccessTime,&vTime,FALSE))
				goto ErrorOut;
			if (nErrorNo = _DBReplace(lAccessTime,vTime))
				goto ErrorOut;

			if (!FileTimeToDateTimeEx(&sFiles.ftLastWriteTime,&vTime,FALSE))
				goto ErrorOut;
			if (nErrorNo = _DBReplace(lWriteTime,vTime))
				goto ErrorOut;

			vFileSize.ev_real = INTS2DOUBLE(sFiles.nFileSizeLow,sFiles.nFileSizeHigh);
			if (nErrorNo = _DBReplace(lFileSize,vFileSize))
				goto ErrorOut;

			vFileAttribs.ev_long = sFiles.dwFileAttributes;
			if (nErrorNo = _DBReplace(lFileAttribs,vFileAttribs))
				goto ErrorOut;

			} // endif nFileFilter
		} while(InternetFindNextFile(hSearch,&sFiles));
	}

	nLastError = GetLastError();
	if (nLastError != ERROR_NO_MORE_FILES)
	{
		SaveWininetError("InternetFindNextFile", nLastError);
		goto ErrorOut;
	}

	if (!InternetCloseHandle(hSearch))
	{
		hSearch = 0;
		SaveWininetError("InternetCloseHandle", GetLastError());
		goto ErrorOut;
	}

Success:

	UnlockHandle(p2);
	UnlockHandle(p3);
	FreeHandleEx(vFileName);
	RET_INTEGER(nFileCount);
	return;
	
	ErrorOut:
		UnlockHandle(p2);
		UnlockHandle(p3);
		FreeHandleEx(vFileName);
		if (hSearch)
			InternetCloseHandle(hSearch);
		RaiseError(nErrorNo);
}

void _fastcall HTTPGetFile(ParamBlk *parm)
{

}
*/
