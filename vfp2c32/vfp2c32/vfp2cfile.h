#ifndef _VFP2CFILE_H__
#define _VFP2CFILE_H__

#include <shlwapi.h>
#include <shlobj.h>

#include "vfp2chelpers.h"
#include "vfp2ccppapi.h"

const int ADIREX_DEST_ARRAY		= 0x01;
const int ADIREX_DEST_CURSOR	= 0x02;
const int ADIREX_DEST_CALLBACK	= 0x04;
const int ADIREX_FILTER_ALL		= 0x08;
const int ADIREX_FILTER_NONE	= 0x10;
const int ADIREX_FILTER_EXACT	= 0x20;
const int ADIREX_UTC_TIMES		= 0x40;

const int FILE_OVERWRITE_ASK		= 0;
const int FILE_OVERWRITE_YES		= 1;
const int FILE_OVERWRITE_YESALL		= 2;
const int FILE_OVERWRITE_NO			= 3;
const int FILE_OVERWRITE_NOALL		= 4;
const int FILE_OVERWRITE_IFNEWER	= 5;
const int FILE_OVERWRITE_ABORT		= 6;

const int CALLBACK_FILE_PROGRESS	= 1;
const int CALLBACK_FILE_CHANGE		= 2;
const int CALLBACK_FILE_OVERWRITE	= 3;

const int MAX_OPENFILENAME_BUFFER	= 32768;
const int MAX_ENVSTRING_BUFFER		= 4096;

const int VFP2C_MAX_VOLUME_NAME		= 128;
const int VFP2C_MAX_MOUNTPOINT_NAME	= 512;
const int VFP2C_MAX_DEVICE_NAME		= 1024;

const int VFP2C_FILE_LINE_BUFFER	= 32;

const int MAXFILESEARCHPARAM		= MAX_PATH - 3;

// additional file attribute to filter "." & ".." directory's
const int FILE_ATTRIBUTE_FAKEDIRECTORY	= 0x80000000;
const int SECURITY_DESCRIPTOR_LEN		= 256;

class FileSearch
{
public:
	FileSearch() : m_Handle(INVALID_HANDLE_VALUE) {}
	~FileSearch() { if (m_Handle != INVALID_HANDLE_VALUE) FindClose(m_Handle); }

	bool FindFirst(char *pSearch);
	bool FindNext();
	bool IsFakeDir() const;
	const char* Filename() const;
	unsigned __int64 Filesize() const;

	WIN32_FIND_DATA File;
private:
	HANDLE m_Handle;
};

inline const char* FileSearch::Filename() const
{
	if (File.cFileName[0] != '\0')
		return &File.cFileName[0];
	else
		return &File.cAlternateFileName[0];
}

inline unsigned __int64 FileSearch::Filesize() const
{
	ULARGE_INTEGER nSize;
	nSize.HighPart = File.nFileSizeHigh;
	nSize.LowPart = File.nFileSizeLow;
	return nSize.QuadPart;
}

class SwitchErrorMode
{
public:
	SwitchErrorMode(unsigned int nMode);
	~SwitchErrorMode();
private:
	UINT m_Errormode;
};

inline SwitchErrorMode::SwitchErrorMode(unsigned int nMode) : m_Errormode(0xFFFFFFFF)
{
	m_Errormode = SetErrorMode(0);
	SetErrorMode(m_Errormode | nMode);
}

inline SwitchErrorMode::~SwitchErrorMode()
{
	if (m_Errormode != 0xFFFFFFFF)
		SetErrorMode(m_Errormode);
}

class VolumeSearch
{
public:
	VolumeSearch() : m_Handle(INVALID_HANDLE_VALUE), m_Volume(VFP2C_MAX_VOLUME_NAME) { }
	~VolumeSearch();

	bool FindFirst();
	bool FindNext();
	FoxString& Volume() { return m_Volume; }

private:
	FoxString m_Volume;
	HANDLE m_Handle;
};

class VolumeMountPointSearch
{
public:
	VolumeMountPointSearch() :  m_Handle(INVALID_HANDLE_VALUE), m_Mode(SEM_FAILCRITICALERRORS), m_MountPoint(VFP2C_MAX_MOUNTPOINT_NAME) { }
	~VolumeMountPointSearch();

	bool FindFirst(char *pVolume);
	bool FindNext();
	FoxString& MountPoint() { return m_MountPoint; }

private:
	HANDLE m_Handle;
	SwitchErrorMode m_Mode;
	FoxString m_MountPoint;
};

// custom defines and types for file operations
typedef struct _FILEPROGRESSPARAM {
	char pFileProgress[VFP2C_MAX_CALLBACKBUFFER];
	char pCallback[VFP2C_MAX_CALLBACKBUFFER];
	bool bCallback;
	bool bAborted;
	Value vRetVal;
} FILEPROGRESSPARAM, *LPFILEPROGRESSPARAM;

typedef struct _DIRECTORYINFO {
	int nNumberOfFiles;
	int nNumberOfSubDirs;
	double nDirSize;
	int nErrorNo;
} DIRECTORYINFO, *LPDIRECTORYINFO;

typedef struct _FILEATTRIBUTEINFO {
	DWORD nAttrib;
	DWORD nFlags;
	int nErrorNo;
} FILEATTRIBUTEINFO, *LPFILEATTRIBUTEINFO;

class BrowseCallback
{
public:
	CStrBuilder<VFP2C_MAX_CALLBACKBUFFER> pCallback;
};

class OpenfileCallback
{
public:
	OpenfileCallback() { vRetVal.ev_type = '0'; }
	int nErrorNo;
	Value vRetVal;
	CStrBuilder<VFP2C_MAX_CALLBACKBUFFER> pCallback;
};

// typedef's for runtime dynamic linking
typedef BOOL (_stdcall *PGETSPECIALFOLDER)(HWND,LPSTR,int,BOOL); // SHGetSpecialFolderPathA (shell32.dll)
typedef HRESULT (_stdcall *PSHILCREATEFROMPATH)(LPCWSTR,LPITEMIDLIST*,DWORD*); // SHILCreateFromPath (shell32.dll)
typedef LPITEMIDLIST (_stdcall *PSHILCREATEFROMPATHEX)(LPCWSTR); // undocumented func #162 on shell32.dll
const int SHILCREATEFROMPATHEXID = 162;

typedef BOOL (_stdcall *PGETVOLUMEPATHNAMESFORVOLUMENAME)(LPCTSTR, LPTSTR, DWORD, PDWORD); // GetVolumePathNamesForVolumeName

typedef bool (_stdcall *PADIREXFILTER)(DWORD, DWORD);

/*
typedef HANDLE (_stdcall *CREATETRANSACTION)(LPSECURITY_ATTRIBUTES, LPGUID, DWORD, DWORD, DWORD, DWORD, LPWSTR); // CreateTransaction
typedef BOOL (_stdcall *COMMITTRANSACTION)(HANDLE); // CommitTransaction
typedef BOOL (_stdcall *ROLLBACKTRANSACTION)(HANDLE);  // RollbackTransaction(
*/

#ifdef __cplusplus
extern "C" {
#endif

// function prototypes of vfp2cfile.c
void _stdcall VFP2C_Destroy_File(VFP2CTls& tls);

void _fastcall ADirEx(ParamBlk *parm);
bool _stdcall AdirExFilter_All(DWORD nAttributes, DWORD nFilter);
bool _stdcall AdirExFilter_One(DWORD nAttributes, DWORD nFilter);
bool _stdcall AdirExFilter_None(DWORD nAttributes, DWORD nFilter);
bool _stdcall AdirExFilter_Exact(DWORD nAttributes, DWORD nFilter);

void _fastcall AFileAttributes(ParamBlk *parm);
void _fastcall AFileAttributesEx(ParamBlk *parm);
void _fastcall ADirectoryInfo(ParamBlk *parm);
#pragma warning(disable : 4290) // disable warning 4290 - VC++ doesn't implement throw ...
void _stdcall ADirectoryInfoSubRoutine(LPDIRECTORYINFO pDirInfo, CStrBuilder<MAX_PATH+1>& pDirectory) throw(int);
void _fastcall GetFileTimes(ParamBlk *parm);
void _fastcall SetFileTimes(ParamBlk *parm);
void _fastcall GetFileSizeLib(ParamBlk *parm);
void _fastcall GetFileAttributesLib(ParamBlk *parm);
void _fastcall SetFileAttributesLib(ParamBlk *parm);
void _fastcall GetFileOwner(ParamBlk *parm);
void _fastcall GetLongPathNameLib(ParamBlk *parm);
void _fastcall GetShortPathNameLib(ParamBlk *parm);
void _fastcall GetOpenFileNameLib(ParamBlk *parm);
void _fastcall GetSaveFileNameLib(ParamBlk *parm);
UINT_PTR _stdcall GetFileNameCallback(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void _fastcall ADriveInfo(ParamBlk *parm);
void _fastcall AVolumes(ParamBlk *parm);
void _fastcall AVolumeMountPoints(ParamBlk *parm);
void _fastcall AVolumePaths(ParamBlk *parm);
void _fastcall AVolumeInformation(ParamBlk *parm);
void _fastcall GetWindowsDirectoryLib(ParamBlk *parm);
void _fastcall GetSystemDirectoryLib(ParamBlk *parm);
void _fastcall ExpandEnvironmentStringsLib(ParamBlk *parm);
void _fastcall CopyFileExLib(ParamBlk *parm);
void _fastcall MoveFileExLib(ParamBlk *parm);
bool _stdcall CopyFileProgress(char *pSource, char *pDest, LPFILEPROGRESSPARAM pProgress, DWORD dwShareMode) throw(int);
bool _stdcall MoveFileProgress(char *pSourceFile, char *pDestFile, DWORD nAttributes, bool bCrossVolume,
				  LPFILEPROGRESSPARAM pProgress, DWORD dwShareMode) throw(int);
void _fastcall CompareFileTimes(ParamBlk *parm);
void _fastcall DeleteFileEx(ParamBlk *parm);
void _fastcall DeleteDirectory(ParamBlk *parm);

// extended VFP like file functions
void _fastcall FCreateEx(ParamBlk *parm);
void _fastcall FOpenEx(ParamBlk *parm);
void _fastcall FCloseEx(ParamBlk *parm);
void _fastcall FReadEx(ParamBlk *parm);
void _fastcall FWriteEx(ParamBlk *parm);
void _fastcall FGetsEx(ParamBlk *parm);
void _fastcall FPutsEx(ParamBlk *parm);
void _fastcall FSeekEx(ParamBlk *parm);
void _fastcall FEoFEx(ParamBlk *parm);
void _fastcall FChSizeEx(ParamBlk *parm);
void _fastcall FFlushEx(ParamBlk *parm);
void _fastcall FLockFile(ParamBlk *parm);
void _fastcall FUnlockFile(ParamBlk *parm);
void _fastcall FLockFileEx(ParamBlk *parm);
void _fastcall FUnlockFileEx(ParamBlk *parm);
void _fastcall AFHandlesEx(ParamBlk *parm);
void _stdcall MapFileAccessFlags(int nFileAttribs, int nAccess, int nShare, LPDWORD pAccess, LPDWORD pShare, LPDWORD pFlags) throw(int);

// shell api wrappers
void _fastcall SHSpecialFolder(ParamBlk *parm);
void _fastcall SHMoveFiles(ParamBlk *parm);
void _fastcall SHCopyFiles(ParamBlk *parm);
void _fastcall SHDeleteFiles(ParamBlk *parm);
void _fastcall SHRenameFiles(ParamBlk *parm);
void _fastcall SHBrowseFolder(ParamBlk *parm);
int _stdcall SHBrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

bool _stdcall PathIsSameVolume(const char *pPath1, const char *pPath2) throw(int);
bool _stdcall FileExists(const char *pFileName) throw(int);
void _stdcall CreateDirectoryExEx(const char *pDirectory) throw(int);
void _stdcall RemoveDirectoryEx(const char *pDirectory) throw(int);
void _stdcall DeleteDirectoryEx(const char *pDirectory) throw(int);
void _stdcall RemoveReadOnlyAttrib(const char *pFileName, DWORD nFileAttribs) throw(int);
void _stdcall RemoveReadOnlyAttribW(const wchar_t *pFileName, DWORD nFileAttribs) throw(int);
void _stdcall DeleteFileExEx(const char *pFileName, DWORD nFileAttribs) throw(int);
void _stdcall DeleteFileExExW(const wchar_t *pFileName, DWORD nFileAttribs) throw(int);
int _stdcall CompareFileTimesEx(const char *pSourceFile, const char *pDestFile)  throw(int);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif // _VFP2CFILE_H__