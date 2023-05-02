#ifndef _VFP2CFILE_H__
#define _VFP2CFILE_H__

// #include <shlwapi.h>
#include <shlobj.h>

const int ADIREX_DEST_ARRAY				= 0x01;
const int ADIREX_DEST_CURSOR			= 0x02;
const int ADIREX_DEST_CALLBACK			= 0x04;
const int ADIREX_FILTER_ALL				= 0x08;
const int ADIREX_FILTER_NONE			= 0x10;
const int ADIREX_FILTER_EXACT			= 0x20;
const int ADIREX_UTC_TIMES				= 0x40;
const int ADIREX_RECURSIVE				= 0x80;
const int ADIREX_DISABLE_FSREDIRECTION	= 0x100;
const int ADIREX_FULLPATH				= 0x200;

// not defined in the old VS 6
#if !defined(FIND_FIRST_EX_LARGE_FETCH)
#define FIND_FIRST_EX_LARGE_FETCH	0x00000002
#endif

#if !defined(COPY_FILE_NO_BUFFERING)
#define COPY_FILE_NO_BUFFERING		0x00001000
#endif

const int MAX_WIDE_PATH				= 32767 + 1;

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

#define FILE_UNICODE_EXTENSION	"\\\\?\\"

// additional file attribute to filter "." & ".." directory's
const unsigned int FILE_ATTRIBUTE_FAKEDIRECTORY	= 0x80000000;
const int SECURITY_DESCRIPTOR_LEN		= 256;

class FileSearch; // forward declartion

class FileSearchStorage
{
public:
	FileSearchStorage(int nOffset = 0) : m_ToLocalTime(false), m_Index_Filename(0 + nOffset),
		m_Index_Dosfilename(1 + nOffset), m_Index_Creationtime(2 + nOffset), m_Index_Accesstime(3 + nOffset),
		m_Index_Writetime(4 + nOffset), m_Index_Filesize(5 + nOffset), m_Index_Fileattribs(6 + nOffset) { }
	virtual void Initialize(CStringView pDestination, bool bToLocalTime, CStringView pFields);
	virtual void Finalize() { };
	virtual bool Store(FileSearch* pFileSearch) { return true; };
	// we need a virtual destructor - otherwise the destructor of objects in the descending classes don't run and then causing potential memory leaks
	virtual ~FileSearchStorage() { };

protected:
	FoxString	m_FileName;
	FoxDateTime m_FileTime;
	FoxInt64	m_FileSize;
	bool		m_ToLocalTime;
	int m_Index_Filename;
	int m_Index_Dosfilename;
	int m_Index_Accesstime;
	int m_Index_Creationtime;
	int m_Index_Writetime;
	int m_Index_Filesize;
	int m_Index_Fileattribs;
};

class FileSearchStorageArray : public FileSearchStorage
{
public:
	FileSearchStorageArray() : FileSearchStorage(1) {}
	void Initialize(CStringView pDestination, bool bToLocalTime, CStringView pFields);
	void Finalize();
	bool Store(FileSearch* pFileSearch);
private:
	FoxArray	m_Array;
};

class FileSearchStorageCursor : public FileSearchStorage
{
public:
	FileSearchStorageCursor() : FileSearchStorage(0) {}
	void Initialize(CStringView pDestination, bool bToLocalTime, CStringView pFields);
	void Finalize() { };
	bool Store(FileSearch* pFileSearch);
private:
	FoxCursor	m_Cursor;
};

class FileSearchStorageCallback : public FileSearchStorage
{
public:
	FileSearchStorageCallback() : FileSearchStorage(0) {}
	void Initialize(CStringView pDestination, bool bToLocalTime, CStringView pFields);
	void Finalize() {};
	bool Store(FileSearch* pFileSearch);
private:
	FoxDateTimeLiteral m_CreationTime, m_AccessTime, m_WriteTime;
	CDynamicFoxCallback m_Callback;
};

typedef struct FileSearchEntry
{
public:
	DWORD		Attributes;
	CStringView Path;

	unsigned int BlockSize()
	{
		return sizeof(DWORD) + Path.BlockSize();
	}

	void ToBlock(char* pAddress)
	{
		*reinterpret_cast<DWORD*>(pAddress) = Attributes;
		pAddress += sizeof(DWORD);
		Path.ToBlock(pAddress);
	}

	void FromBlock(char* pAddress)
	{
		Attributes = *reinterpret_cast<DWORD*>(pAddress);
		Path.FromBlock(pAddress + sizeof(DWORD));
	}
} FileSearchEntry;

typedef void(*FileSearchReverseFunc)(CStrBuilder<MAX_WIDE_PATH>& pPathName, DWORD nFileAttributes);

class FileSearch
{
public:
	FileSearch(bool lRecurse, CStringView pSearchPath, DWORD nFileFilter, CStringView pDestination, int nDest, bool bToLocalTime, int nMaxRecursion, bool bDisableFsRedirection, CStringView pFields);
	~FileSearch();

	unsigned int ExecuteSearch();
	void ExecuteReverse(FileSearchReverseFunc pCallback);
	bool FindFirst();
	bool FindNext();
	bool IsRealDirectory() const;
	bool IsFakeDir() const;
	CStringView FileName();
	CStringView AlternateFileName();
	unsigned __int64 FileSize() const;
	int FileAttributes() const;
	FILETIME CreationTime() const;
	FILETIME LastAccessTime() const;
	FILETIME LastWriteTime() const;

private:
	static DWORD GetFindFirstFlags();
	static void SetFindFirstFlags(DWORD dwFlags);
	static DWORD FindFirstFlags;
	bool FindFirstRecurse();
	bool FindNextRecurse();
	bool FindFirstReverse();
	bool FindNextReverse();
	bool MatchFile() const;
	void DisableFsRedirection();
	void RevertFsRedirection();
	bool IsFakeDirName() const;
	bool IgnorableSearchError(DWORD nLastError, bool bIgnoreAccessDenied = false) const;

	typedef bool(*FileFilterFunc)(DWORD nAttributes, DWORD nFilter);
	static bool Filter_All(DWORD nAttributes, DWORD nFilter);
	static bool Filter_One(DWORD nAttributes, DWORD nFilter);
	static bool Filter_None(DWORD nAttributes, DWORD nFilter);
	static bool Filter_Exact(DWORD nAttributes, DWORD nFilter);


	FileFilterFunc		m_FilterFunc;
	DWORD				m_FilterFilter;
	int					m_FileCount;
	int					m_MaxRecursion;
	int					m_RecursionLevel;
	bool				m_Recurse;
	bool				m_FilterFakeDirectory;
	bool				m_DisableFsRedirection;
	bool				m_WSearch;
	bool				m_StoreFullPath;
	FileSearchStorage* m_Storage;
	HANDLE				m_Handle;
	WIN32_FIND_DATA		m_Filedata;
	WIN32_FIND_DATAW	m_WideFiledata;
	WIN32_FIND_DATA*	m_CurrentFiledata;
	CStrBuilder<MAX_WIDE_PATH>	m_Directory;
	CStrBuilder<MAX_WIDE_PATH>	m_SubDirectory;
	CStrBuilder<MAX_WIDE_PATH>	m_CompleteFilename;
	CStrBuilder<MAX_WIDE_PATH>	m_StrBuilder;
	CStrBuilder<MAX_WIDE_PATH>	m_SearchPattern;
	CStrBuilder<MAX_PATH>	m_Wildcard;
	CStrBuilder<MAX_PATH>	m_WStringConversion;
	FoxWString<MAX_PATH>	m_WideWildcard;
	FoxWString<MAX_PATH*2>	m_WideSearchPattern;
	CUnboundBlockQueue<CStringView, 1024 * 64>  m_Subdirectories;
};

// no reparse points and fake directries ("." & "..")
inline bool FileSearch::IsRealDirectory() const
{
	return (FileAttributes() & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == FILE_ATTRIBUTE_DIRECTORY && !IsFakeDirName();
}

// reparse point or a fake directry ("." & "..")
inline bool FileSearch::IsFakeDir() const
{
	return (FileAttributes() & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == FILE_ATTRIBUTE_DIRECTORY && IsFakeDirName();
}

inline bool FileSearch::IsFakeDirName() const
{
	if (m_WSearch == false)
		return (m_Filedata.cFileName[0] == '.' && m_Filedata.cFileName[1] == '\0') || (m_Filedata.cFileName[0] == '.' && m_Filedata.cFileName[1] == '.' && m_Filedata.cFileName[2] == '\0');
	else
		return (m_WideFiledata.cFileName[0] == L'.' && m_WideFiledata.cFileName[1] == L'\0') || (m_WideFiledata.cFileName[0] == L'.' && m_WideFiledata.cFileName[1] == L'.' && m_WideFiledata.cFileName[2] == L'\0');
}

inline CStringView FileSearch::FileName()
{
	if (m_WSearch == false)
	{
		if (m_SubDirectory.Len() == 0 && m_StoreFullPath == false)
		{
			if (m_Filedata.cFileName[0] != '\0')
				return m_Filedata.cFileName;
			else
				return m_Filedata.cAlternateFileName;
		}
		else
		{
			if (m_Filedata.cFileName[0] != '\0')
				m_CompleteFilename.AppendFromBase(m_Filedata.cFileName);
			else
				m_CompleteFilename.AppendFromBase(m_Filedata.cAlternateFileName);

			return m_CompleteFilename;
		}
	}
	else
	{
		if (m_SubDirectory.Len() == 0 && m_StoreFullPath == false)
		{
			if (m_WideFiledata.cFileName[0] != L'\0')
				m_CompleteFilename = m_WideFiledata.cFileName;
			else
				m_CompleteFilename = m_WideFiledata.cAlternateFileName;
		}
		else
		{
			m_CompleteFilename.ResetToFormatBase();
			if (m_WideFiledata.cFileName[0] != L'\0')
				m_CompleteFilename += m_WideFiledata.cFileName;
			else
				m_CompleteFilename += m_WideFiledata.cAlternateFileName;
		}
		return m_CompleteFilename;
	}
}

inline CStringView FileSearch::AlternateFileName()
{
	if (m_WSearch == false)
		return m_Filedata.cAlternateFileName;
	m_WStringConversion = m_WideFiledata.cAlternateFileName;
	return m_WStringConversion;
}

inline bool FileSearch::MatchFile() const
{
	if (m_WSearch == false)
		return PathMatchSpec(m_Filedata.cFileName, m_Wildcard) || PathMatchSpec(m_Filedata.cAlternateFileName, m_Wildcard);
	else
		return PathMatchSpecW(m_WideFiledata.cFileName, m_WideWildcard) || PathMatchSpecW(m_WideFiledata.cAlternateFileName, m_WideWildcard);
}


inline unsigned __int64 FileSearch::FileSize() const
{
	ULARGE_INTEGER nSize;
	nSize.HighPart = m_CurrentFiledata->nFileSizeHigh;
	nSize.LowPart = m_CurrentFiledata->nFileSizeLow;
	return nSize.QuadPart;
}

inline int FileSearch::FileAttributes() const
{
	return m_CurrentFiledata->dwFileAttributes;
}

inline FILETIME FileSearch::CreationTime() const
{
	return m_CurrentFiledata->ftCreationTime;
}

inline FILETIME FileSearch::LastAccessTime() const
{
	return m_CurrentFiledata->ftLastAccessTime;
}

inline FILETIME FileSearch::LastWriteTime() const
{
	return m_CurrentFiledata->ftLastWriteTime;
}

inline bool FileSearch::IgnorableSearchError(DWORD nLastError, bool bIgnoreAccessDenied) const
{
	return nLastError == ERROR_FILE_NOT_FOUND || (bIgnoreAccessDenied && nLastError == ERROR_ACCESS_DENIED);
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
typedef class FILEPROGRESSPARAM
{
	public:
		FILEPROGRESSPARAM()
		{
			nStatus = 0;
			vRetVal = 0;
		}
		void ConvertRetVal();

		CFoxCallback pCallback;
		LARGE_INTEGER nLastCallbackTime;
		LARGE_INTEGER nCallbackLimiter;
		DWORD  nStatus;
		ValueEx vRetVal;
} *LPFILEPROGRESSPARAM;

typedef struct _FILEATTRIBUTEINFO {
	DWORD nAttrib;
	DWORD nFlags;
	int nErrorNo;
} FILEATTRIBUTEINFO, *LPFILEATTRIBUTEINFO;

class OpenfileCallback
{
public:
	OpenfileCallback() { vRetVal = 0; }
	int nErrorNo;
	ValueEx vRetVal;
	CFoxCallback pCallback;
};

// typedef's for runtime dynamic linking
typedef BOOL (_stdcall *PGETSPECIALFOLDER)(HWND,LPSTR,int,BOOL); // SHGetSpecialFolderPathA (shell32.dll)
typedef HRESULT (_stdcall *PSHILCREATEFROMPATH)(LPCWSTR,LPITEMIDLIST*,DWORD*); // SHILCreateFromPath (shell32.dll)
typedef LPITEMIDLIST (_stdcall *PSHILCREATEFROMPATHEX)(LPCWSTR); // undocumented func #162 on shell32.dll
#define SHILCREATEFROMPATHEXID	162

typedef BOOL (_stdcall *PGETVOLUMEPATHNAMESFORVOLUMENAME)(LPCTSTR, LPTSTR, DWORD, PDWORD); // GetVolumePathNamesForVolumeName

// Wow64DisableWow64FsRedirection & Wow64RevertWow64FsRedirection
typedef BOOL (_stdcall * PWOW64FSREDIRECTION)(PVOID* OldValue);

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

void _fastcall ADirEx(ParamBlkEx& parm);
void _fastcall AFileAttributes(ParamBlkEx& parm);
void _fastcall AFileAttributesEx(ParamBlkEx& parm);
void _fastcall ADirectoryInfo(ParamBlkEx& parm);
void _fastcall GetFileTimes(ParamBlkEx& parm);
void _fastcall SetFileTimes(ParamBlkEx& parm);
void _fastcall GetFileSizeLib(ParamBlkEx& parm);
void _fastcall GetFileAttributesLib(ParamBlkEx& parm);
void _fastcall SetFileAttributesLib(ParamBlkEx& parm);
void _fastcall GetFileOwner(ParamBlkEx& parm);
void _fastcall GetLongPathNameLib(ParamBlkEx& parm);
void _fastcall GetShortPathNameLib(ParamBlkEx& parm);
void _fastcall GetOpenFileNameLib(ParamBlkEx& parm);
void _fastcall GetSaveFileNameLib(ParamBlkEx& parm);
UINT_PTR _stdcall GetFileNameCallback(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void _fastcall ADriveInfo(ParamBlkEx& parm);
void _fastcall AVolumes(ParamBlkEx& parm);
void _fastcall AVolumeMountPoints(ParamBlkEx& parm);
void _fastcall AVolumePaths(ParamBlkEx& parm);
void _fastcall AVolumeInformation(ParamBlkEx& parm);
void _fastcall GetWindowsDirectoryLib(ParamBlkEx& parm);
void _fastcall GetSystemDirectoryLib(ParamBlkEx& parm);
void _fastcall ExpandEnvironmentStringsLib(ParamBlkEx& parm);
void _fastcall CopyFileExLib(ParamBlkEx& parm);
void _fastcall MoveFileExLib(ParamBlkEx& parm);
void _fastcall CompareFileTimes(ParamBlkEx& parm);
void _fastcall DeleteFileEx(ParamBlkEx& parm);
void _fastcall DeleteDirectory(ParamBlkEx& parm);

// extended VFP like file functions
void _fastcall FCreateEx(ParamBlkEx& parm);
void _fastcall FOpenEx(ParamBlkEx& parm);
void _fastcall FCloseEx(ParamBlkEx& parm);
void _fastcall FReadEx(ParamBlkEx& parm);
void _fastcall FWriteEx(ParamBlkEx& parm);
void _fastcall FGetsEx(ParamBlkEx& parm);
void _fastcall FPutsEx(ParamBlkEx& parm);
void _fastcall FSeekEx(ParamBlkEx& parm);
void _fastcall FEoFEx(ParamBlkEx& parm);
void _fastcall FChSizeEx(ParamBlkEx& parm);
void _fastcall FFlushEx(ParamBlkEx& parm);
void _fastcall FLockFile(ParamBlkEx& parm);
void _fastcall FUnlockFile(ParamBlkEx& parm);
void _fastcall FLockFileEx(ParamBlkEx& parm);
void _fastcall FUnlockFileEx(ParamBlkEx& parm);
void _fastcall AFHandlesEx(ParamBlkEx& parm);

#pragma warning(disable : 4290) // disable warning 4290 - VC++ doesn't implement throw ...
void _stdcall MapFileAccessFlags(int nFileAttribs, int nAccess, int nShare, LPDWORD pAccess, LPDWORD pShare, LPDWORD pFlags) throw(int);
#pragma warning(default : 4290) // enable warning 4290 - VC++ doesn't implement throw ...

// shell api wrappers
void _fastcall SHSpecialFolder(ParamBlkEx& parm);
void _fastcall SHMoveFiles(ParamBlkEx& parm);
void _fastcall SHCopyFiles(ParamBlkEx& parm);
void _fastcall SHDeleteFiles(ParamBlkEx& parm);
void _fastcall SHRenameFiles(ParamBlkEx& parm);
void _fastcall SHBrowseFolder(ParamBlkEx& parm);
int _stdcall SHBrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

#ifdef __cplusplus
}
#endif // end of extern "C"

DWORD _stdcall FileProgressCallback(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber,
	DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData);
#pragma warning(disable : 4290) // disable warning 4290 - VC++ doesn't implement throw ...
void _stdcall RemoveDirectoryEx(CStringView pDirectory) throw(int);
void _stdcall RemoveDirectoryEx(CWideStringView pDirectory) throw(int);
void _stdcall RemoveReadOnlyAttrib(CStringView pFileName, DWORD nFileAttribs) throw(int);
void _stdcall RemoveReadOnlyAttrib(CWideStringView pFileName, DWORD nFileAttribs) throw(int);
void _stdcall DeleteFileExImpl(CStringView pFileName, DWORD nFileAttribs) throw(int);
void _stdcall DeleteFileExImpl(CWideStringView pFileName, DWORD nFileAttribs) throw(int);
#pragma warning(default : 4290) // enable warning 4290 - VC++ doesn't implement throw ...

#endif // _VFP2CFILE_H__