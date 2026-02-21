#ifndef _VFP2CFILE_H__
#define _VFP2CFILE_H__

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winternl.h>

#define SL_RESTART_SCAN 0x1
#define STATUS_SUCCESS 0x0

#define ERRORSTATE_WIN32				0x01
#define ERRORSTATE_VFP					0x02
#define ERRORSTATE_POSTCOMPLETION		0x04

typedef enum _FILE_INFORMATION_CLASSEX {
	FileDirectoryInformationEx = 1,
	FileFullDirectoryInformationEx = 2,
	FileBothDirectoryInformationEx = 3,
	FileReparsePointInformationEx = 33
} FILE_INFORMATION_CLASSEX;

/*
typedef enum _FILE_INFO_BY_HANDLE_CLASS {
	FileBasicInfo,
	FileStandardInfo,
	FileNameInfo,
	FileRenameInfo,
	FileDispositionInfo,
	FileAllocationInfo,
	FileEndOfFileInfo,
	FileStreamInfo,
	FileCompressionInfo,
	FileAttributeTagInfo,
	FileIdBothDirectoryInfo,
	FileIdBothDirectoryRestartInfo,
	FileIoPriorityHintInfo,
	FileRemoteProtocolInfo,
	FileFullDirectoryInfo,
	FileFullDirectoryRestartInfo,
	FileStorageInfo,
	FileAlignmentInfo,
	FileIdInfo,
	FileIdExtdDirectoryInfo,
	FileIdExtdDirectoryRestartInfo,
	FileDispositionInfoEx,
	FileRenameInfoEx,
	FileCaseSensitiveInfo,
	FileNormalizedNameInfo,
	MaximumFileInfoByHandleClass
} FILE_INFO_BY_HANDLE_CLASS, * PFILE_INFO_BY_HANDLE_CLASS;
*/

typedef NTSTATUS(_stdcall* PNTQUERYDIRECTORYFILE)(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation,
	ULONG Length, DWORD FileInformationClass, BOOLEAN ReturnSingleEntry, PUNICODE_STRING FileName, BOOLEAN RestartScan); // NtQueryDirectoryFile

typedef NTSTATUS(_stdcall* PNTQUERYDIRECTORYFILEEX)(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation,
	ULONG Length, DWORD FileInformationClass, ULONG QueryFlags, PUNICODE_STRING FileName); // NtQueryDirectoryFileEx

typedef void(_stdcall* PRTLINITUNICODESTRING)(PUNICODE_STRING DestinationString, PCWSTR SourceString);
typedef ULONG(_stdcall* PRTLNTSTATUSTODOSERROR)(LONG status);

typedef BOOL(_stdcall* PGETVOLUMEPATHNAMESFORVOLUMENAME)(LPCTSTR, LPTSTR, DWORD, PDWORD); // GetVolumePathNamesForVolumeName
// Wow64DisableWow64FsRedirection & Wow64RevertWow64FsRedirection
typedef BOOL(_stdcall* PWOW64FSREDIRECTION)(PVOID* OldValue);
typedef BOOL(_stdcall* PGETFILEINFORMATIONBYHANDLEEX)(HANDLE hFile, int FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize);

/*
typedef PTP_POOL(_stdcall* PCREATETHREADPOOL)(PVOID reserved);
typedef VOID(_stdcall* PCLOSETHREADPOOL)(PTP_POOL ptpp);
typedef VOID(_stdcall* PSETTHREADPOOLTHREADMAXIMUM)(PTP_POOL ptpp,DWORD cthrdMost);
typedef BOOL(_stdcall* PSETTHREADPOOLTHREADMINIMUM)(PTP_POOL ptpp, DWORD cthrdMic);

typedef VOID(_stdcall* PINITIALIZETHREADPOOLENVIRONMENT)(PTP_CALLBACK_ENVIRON pcbe);
typedef VOID(_stdcall* PDESTROYTHREADPOOLENVIRONMENT)(PTP_CALLBACK_ENVIRON pcbe);

typedef VOID(_stdcall* PSETTHREADPOOLCALLBACKCLEANUPGROUP)(PTP_CALLBACK_ENVIRON pcbe, PTP_CLEANUP_GROUP ptpcg, PTP_CLEANUP_GROUP_CANCEL_CALLBACK pfng);
typedef VOID(_stdcall* PSETTHREADPOOLCALLBACKLIBRARY)(PTP_CALLBACK_ENVIRON pcbe, PVOID mod);
typedef VOID(_stdcall* PSETTHREADPOOLCALLBACKPOOL)(PTP_CALLBACK_ENVIRON pcbe, PTP_POOL ptpp);
typedef VOID(_stdcall* PSETTHREADPOOLCALLBACKPRIORITY)(PTP_CALLBACK_ENVIRON pcbe, TP_CALLBACK_PRIORITY Priority);
typedef VOID(_stdcall* PSETTHREADPOOLCALLBACKRUNSLONG)(PTP_CALLBACK_ENVIRON pcbe);

typedef PTP_CLEANUP_GROUP(_stdcall* PCREATETHREADPOOLCLEANUPGROUP)();
typedef VOID(_stdcall* PCLOSETHREADPOOLCLEANUPGROUP)(PTP_CLEANUP_GROUP ptpcg);
typedef VOID(_stdcall* PCLOSETHREADPOOLCLEANUPGROUPMEMBERS)(PTP_CLEANUP_GROUP ptpcg, BOOL fCancelPendingCallbacks, PVOID pvCleanupContext);

typedef PTP_IO(_stdcall* PCREATETHREADPOOLIO)(HANDLE fl, PTP_WIN32_IO_CALLBACK pfnio, PVOID pv, PTP_CALLBACK_ENVIRON  pcbe);
typedef VOID(_stdcall* PSTARTTHREADPOOLIO)(PTP_IO pio);
typedef VOID(_stdcall* PCANCELTHREADPOOLIO)(PTP_IO pio);
typedef VOID(_stdcall* PCLOSETHREADPOOLIO)(PTP_IO pio);
typedef VOID(_stdcall* PWAITFORTHREADPOOLIOCALLBACKS)(PTP_IO pio, BOOL fCancelPendingCallbacks);

typedef PTP_WORK(_stdcall* PCREATETHREADPOOLWORK)(PTP_WORK_CALLBACK pfnwk, PVOID pv, PTP_CALLBACK_ENVIRON pcbe);
typedef VOID(_stdcall* PCLOSETHREADPOOLWORK)(PTP_WORK pwk);
typedef VOID(_stdcall* PSUBMITTHREADPOOLWORK)(PTP_WORK pwk);
typedef BOOL(_stdcall* PTRYSUBMITTHREADPOOLCALLBACK)(PTP_SIMPLE_CALLBACK pfns, PVOID pv, PTP_CALLBACK_ENVIRON pcbe);
typedef VOID(_stdcall* PWAITFORTHREADPOOLWORKCALLBACKS)(PTP_WORK pwk, BOOL fCancelPendingCallbacks);
*/

// See the handy table linked on the page below to learn where these values comes from.
const NTSTATUS STATUS_NO_MORE_FILES = 0x80000006;
const NTSTATUS STATUS_NO_SUCH_FILE = 0xC000000F;

// AdirEx flags
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
const int ADIREX_STRING_FILEATTRIBUTES	= 0x400;
const int ADIREX_INCLUDE_REPARSE_POINT	= 0x800;
const int ADIREX_NODOSNAME				= 0x1000;
const int ADIREX_FILEID					= 0x2000;
const int ADIREX_PARENTID				= 0x4000;
const int ADIREX_JUSTFNAME				= 0x8000;
const int ADIREX_PARENT_JUSTFNAME		= 0x10000;
const int ADIREX_PARENT_FULLPATH		= 0x20000;
const int ADIREX_FILTER_MATCH			= 0x80000000;

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
#define LFILE_UNICODE_EXTENSION	L"\\\\?\\"

// additional file attribute to filter "." & ".." directory's
const unsigned int FILE_ATTRIBUTE_FAKEDIRECTORY	= 0x80000000;
const int SECURITY_DESCRIPTOR_LEN		= 256;

typedef struct _FILE_DIRECTORY_INFORMATION {
	ULONG         NextEntryOffset;
	ULONG         FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG         FileAttributes;
	ULONG         FileNameLength;
	WCHAR         FileName[1];
} FILE_DIRECTORY_INFORMATION, * PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIRECTORY_INFORMATION {
	ULONG			NextEntryOffset;
	ULONG			FileIndex;
	LARGE_INTEGER	CreationTime;
	LARGE_INTEGER	LastAccessTime;
	LARGE_INTEGER	LastWriteTime;
	LARGE_INTEGER	ChangeTime;
	LARGE_INTEGER	EndOfFile;
	LARGE_INTEGER	AllocationSize;
	ULONG			FileAttributes;
	ULONG			FileNameLength;
	ULONG			EaSize;
	WCHAR			FileName[1];
} FILE_FULL_DIRECTORY_INFORMATION, * PFILE_FULL_DIRECTORY_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION {
	ULONG			NextEntryOffset;
	ULONG			FileIndex;
	LARGE_INTEGER	CreationTime;
	LARGE_INTEGER	LastAccessTime;
	LARGE_INTEGER	LastWriteTime;
	LARGE_INTEGER	ChangeTime;
	LARGE_INTEGER	EndOfFile;
	LARGE_INTEGER	AllocationSize;
	ULONG			FileAttributes;
	ULONG			FileNameLength;
	ULONG			EaSize;
	CCHAR			ShortNameLength;
	WCHAR			ShortName[12];
	WCHAR			FileName[1];
} FILE_BOTH_DIR_INFORMATION, * PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_REPARSE_POINT_INFORMATION
{
	__int64 FileReferenceNumber;
	DWORD Tag;
} FILE_REPARSE_POINT_INFORMATION;

typedef struct _FILE_ID_INFO {
	ULONGLONG   VolumeSerialNumber;
	BYTE FileId[16];
} FILE_ID_INFO, * PFILE_ID_INFO;

typedef struct _PathIdentifier
{
	ULONGLONG dwVolumeSerialNumber;
	union
	{
		struct
		{
			DWORD dwFileIndexLow;
			DWORD dwFileIndexHigh;
			DWORD dwFileIdLow;
			DWORD dwFileIdHigh;
		};
		BYTE FileId[16];
	};

	bool operator==(const _PathIdentifier& rhs)
	{
		return dwVolumeSerialNumber == rhs.dwVolumeSerialNumber && dwFileIndexLow == rhs.dwFileIndexLow && dwFileIndexHigh == rhs.dwFileIndexHigh &&
			dwFileIdLow == rhs.dwFileIdLow && dwFileIdHigh == rhs.dwFileIdHigh;
	}

} PathIdentifier;

typedef struct _FileSearchEntry
{
	DWORD		Attributes;
	DWORD		RecursionLevel;
	DWORD		DirectoryId;
	CStringView Path;

	unsigned int BlockSize()
	{
		return sizeof(Attributes) + sizeof(RecursionLevel) + sizeof(DirectoryId) + sizeof(Path) + Path.DataSize();
	}

	void ToBlock(char* pAddress)
	{
		char* pStruct = pAddress;
		*reinterpret_cast<DWORD*>(pAddress) = Attributes;
		pAddress = pStruct + offsetof(FileSearchEntry, RecursionLevel);
		*reinterpret_cast<DWORD*>(pAddress) = RecursionLevel;
		pAddress = pStruct + offsetof(FileSearchEntry, DirectoryId);
		*reinterpret_cast<DWORD*>(pAddress) = DirectoryId;
		pAddress = pStruct + offsetof(FileSearchEntry, Path);
		char* data = pStruct + sizeof(FileSearchEntry);
		Path.ToBlock(pAddress, data);
	}

	void FromBlock(char* pAddress)
	{
		char* pStruct = pAddress;
		Attributes = *reinterpret_cast<DWORD*>(pAddress);
		pAddress = pStruct + offsetof(FileSearchEntry, RecursionLevel);
		RecursionLevel = *reinterpret_cast<DWORD*>(pAddress);
		pAddress = pStruct + offsetof(FileSearchEntry, DirectoryId);
		DirectoryId = *reinterpret_cast<DWORD*>(pAddress);
		pAddress = pStruct + offsetof(FileSearchEntry, Path);
		Path.FromBlock(pAddress);
	}
} FileSearchEntry;

#pragma warning(disable : 4480) // disable warning 4480 - nonstandard extension to specify underlying type of enum - it works fine and it's needed for correct alignment
#if _MSC_VER < 1400
enum DirectoryState
#else
enum DirectoryState : int
#endif
{
	Unsaved = 0,
	Queued,
	Saved
};
#pragma warning(default : 4480)

typedef struct _PathSearchEntry
{
	DWORD		m_Attributes;
	DWORD		m_PathId;
	DWORD		m_ParentId;
	DirectoryState	m_State;
	FILETIME	m_CreationTime;
	FILETIME	m_LastAccessTime;
	FILETIME	m_LastWriteTime;
	CWideStringView m_Name;
	CWideStringView m_DosName;

	unsigned int BlockSize()
	{
		return sizeof(m_Attributes) + sizeof(m_PathId) + sizeof(m_ParentId) + sizeof(m_State) + sizeof(m_CreationTime) + sizeof(m_LastAccessTime) + sizeof(m_LastWriteTime) + sizeof(m_Name) + sizeof(m_DosName) + m_Name.DataSize() + m_DosName.DataSize();
	}

	void ToBlock(char* pAddress)
	{
		char* pStruct = pAddress;
		*reinterpret_cast<DWORD*>(pAddress) = m_Attributes;
		
		pAddress = pStruct + offsetof(PathSearchEntry, m_PathId);
		*reinterpret_cast<DWORD*>(pAddress) = m_PathId;
		
		pAddress = pStruct + offsetof(PathSearchEntry, m_ParentId);
		*reinterpret_cast<DWORD*>(pAddress) = m_ParentId;
		
		pAddress = pStruct + offsetof(PathSearchEntry, m_State);
		*reinterpret_cast<DirectoryState*>(pAddress) = m_State;

		pAddress = pStruct + offsetof(PathSearchEntry, m_CreationTime);;
		*reinterpret_cast<FILETIME*>(pAddress) = m_CreationTime;

		pAddress = pStruct + offsetof(PathSearchEntry, m_LastAccessTime);
		*reinterpret_cast<FILETIME*>(pAddress) = m_LastAccessTime;

		pAddress = pStruct + offsetof(PathSearchEntry, m_LastWriteTime);
		*reinterpret_cast<FILETIME*>(pAddress) = m_LastWriteTime;

		pAddress = pStruct + offsetof(PathSearchEntry, m_Name);
		char* data = pStruct + sizeof(PathSearchEntry);
		data = m_Name.ToBlock(pAddress, data);

		pAddress = pStruct + offsetof(PathSearchEntry, m_DosName);
		m_DosName.ToBlock(pAddress, data);
	}

	void FromBlock(char* pAddress)
	{
		char* pStruct = pAddress;
		m_Attributes = *reinterpret_cast<DWORD*>(pAddress);

		pAddress = pStruct + offsetof(PathSearchEntry, m_PathId);
		m_PathId = *reinterpret_cast<DWORD*>(pAddress);

		pAddress = pStruct + offsetof(PathSearchEntry, m_ParentId);
		m_ParentId = *reinterpret_cast<DWORD*>(pAddress);

		pAddress = pStruct + offsetof(PathSearchEntry, m_State);
		m_State = *reinterpret_cast<DirectoryState*>(pAddress);

		pAddress = pStruct + offsetof(PathSearchEntry, m_CreationTime);;
		m_CreationTime = *reinterpret_cast<FILETIME*>(pAddress);

		pAddress = pStruct + offsetof(PathSearchEntry, m_LastAccessTime);
		m_LastAccessTime = *reinterpret_cast<FILETIME*>(pAddress);

		pAddress = pStruct + offsetof(PathSearchEntry, m_LastWriteTime);
		m_LastWriteTime = *reinterpret_cast<FILETIME*>(pAddress);

		pAddress = pStruct + offsetof(PathSearchEntry, m_Name);
		m_Name.FromBlock(pAddress);

		pAddress = pStruct + offsetof(PathSearchEntry, m_DosName);
		m_DosName.FromBlock(pAddress);
	}

	DWORD FileAttributes() const { return m_Attributes; }
	int PathId() const { return m_PathId; }
	int ParentId() const { return m_ParentId; }
	FILETIME CreationTime() const { return m_CreationTime; }
	FILETIME LastAccessTime() const { return m_LastAccessTime; }
	FILETIME LastWriteTime() const { return m_LastWriteTime; }
	CWideStringView Filename() const { return m_Name; }
	CWideStringView AlternateFilename() const { return m_DosName; }
	unsigned __int64 FileSize() const { return 1; }

} PathSearchEntry;

typedef CAlignedBuffer<8> CFileSearchBuffer;

class CPathBuilder : public FoxString
{
public:
	CPathBuilder() : FoxString(MAX_WIDE_PATH)
	{
		m_PathIndex = 0xFFFF;
		m_PathIndices[0] = 0;
	};

	void AddPath(CWideStringView pPath)
	{
		unsigned short nCurrentLen = m_PathIndex != 0xFFFF ? m_PathIndices[m_PathIndex] : 0;
		if (pPath.Len == 0 || pPath.Data == 0)
		{
			m_PathIndex++;
			m_PathIndices[m_PathIndex] = nCurrentLen;
			return;
		}

		int nLen = WideCharToMultiByte(CP_ACP, 0, pPath.Data, pPath.Len, &m_String[nCurrentLen], MAX_WIDE_PATH - m_Value.ev_length, 0, 0);
		if (!nLen)
		{
			SaveWin32Error("MultiByteToWideChar", GetLastError());
			throw E_APIERROR;
		}
		
		if (pPath.Data[pPath.Len - 1] != '\\')
		{
			nLen += nCurrentLen + 1;
			m_String[nLen - 1] = '\\';
		}
		else
			nLen += nCurrentLen;

		m_PathIndex++;
		m_PathIndices[m_PathIndex] = nLen;
		m_Value.ev_length = nLen;
	}

	void ReplacePath(CWideStringView pPath, unsigned short nIndex)
	{
		m_PathIndex = nIndex;
		unsigned short nCurrentLen = m_PathIndices[nIndex - 1];
		int nLen = WideCharToMultiByte(CP_ACP, 0, pPath.Data, pPath.Len, &m_String[nCurrentLen], MAX_WIDE_PATH - m_Value.ev_length, 0, 0);
		if (!nLen)
		{
			SaveWin32Error("MultiByteToWideChar", GetLastError());
			throw E_APIERROR;
		}
		nLen += nCurrentLen + 1;
		m_PathIndices[nIndex] = nLen;
		m_Value.ev_length = nLen;
		m_String[nLen - 1] = '\\';
	}

	void ReplaceFilename(CWideStringView pFilename)
	{
		unsigned short nCurrentLen = m_PathIndex != 0xFFFF ? m_PathIndices[m_PathIndex] : 0;
		int nLen = WideCharToMultiByte(CP_ACP, 0, pFilename.Data, pFilename.Len, &m_String[nCurrentLen], MAX_WIDE_PATH - m_Value.ev_length, 0, 0);
		if (!nLen)
		{
			SaveWin32Error("MultiByteToWideChar", GetLastError());
			throw E_APIERROR;
		}
		m_Value.ev_length = nLen + nCurrentLen;
	}

	CStringView GetPath(int nLevel, bool bIncludeBackSlash = false)
	{
		return CStringView(m_String, m_PathIndices[nLevel] - (bIncludeBackSlash ? 0 : 1));
	}

	void RemovePath()
	{
		m_PathIndex--;
		m_Value.ev_length = m_PathIndices[m_PathIndex];
	}

	CPathBuilder& operator=(const CWideStringView& pString)
	{
		FoxString::operator = (pString);
		return *this;
	}

private:
	unsigned short m_PathIndex;
	unsigned short m_PathIndices[512];
};

class CWidePathBuilder : public CWideStrBuilder<MAX_WIDE_PATH>
{
public:
	CWidePathBuilder()
	{
		m_PathIndex = 0xFFFF;
		m_PathIndices[0] = 0;
	};

	void AddPath(CWideStringView pPath)
	{
		unsigned short nCurrentLen = m_PathIndex != 0xFFFF ? m_PathIndices[m_PathIndex] : 0;
		if (pPath.Len == 0 || pPath.Data == 0)
		{
			m_PathIndex++;
			m_PathIndices[m_PathIndex] = nCurrentLen;
			return;
		}
			
		unsigned short nLen = pPath.Len + nCurrentLen;
		if (pPath.Data[pPath.Len - 1] != L'\\')
			nLen += 1;

		m_PathIndex++;
		m_PathIndices[m_PathIndex] = nLen;
		memcpy((void*)(&m_String[nCurrentLen]), pPath.Data, pPath.Len * sizeof(wchar_t));
		m_Length = nLen;
		if (m_String[nLen - 1] != L'\\')
		{
			m_String[nLen - 1] = L'\\';
			m_String[nLen] = L'\0';
		}
		else
		{
			m_String[nLen] = L'\0';
		}
	}

	void ReplacePath(CWideStringView pPath, unsigned short nLevel)
	{
		m_PathIndex = nLevel;
		unsigned short nCurrentLen = m_PathIndices[nLevel - 1];
		unsigned short nLen = pPath.Len + nCurrentLen;
		m_PathIndices[nLevel] = nLen + 1;
		memcpy((void*)(&m_String[nCurrentLen]), pPath.Data, pPath.Len * sizeof(wchar_t));
		m_Length = nLen + 1;
		m_String[nLen] = L'\\';
		m_String[nLen + 1] = L'\0';
	}

	void ReplaceFilename(CWideStringView pFileName)
	{
		unsigned short nCurrentLen = m_PathIndex != 0xFFFF ? m_PathIndices[m_PathIndex] : 0;
		unsigned short nLen = pFileName.Len + nCurrentLen;
		memcpy((void*)(&m_String[nCurrentLen]), pFileName.Data, pFileName.Len * sizeof(wchar_t));
		m_Length = nLen;
		m_String[nLen] = L'\0';
	}

	CWideStringView GetPath(int nRoot, int nLevel, bool bIncludeBackSlash = false)
	{
		nLevel = nLevel == -1 ? m_PathIndex : nLevel;
		unsigned int offset = nRoot == 0 ? 0 : m_PathIndices[nRoot - 1];
		return CWideStringView(m_String + offset, m_PathIndices[nLevel] - (bIncludeBackSlash ? 0 : 1) - offset);
	}

	CWideStringView GetPathWithoutUnicodeExtension(int nRoot, int nLevel, bool bIncludeBackSlash = false)
	{
		nLevel = nLevel == -1 ? m_PathIndex : nLevel;
		unsigned int offset = nRoot == 0 ? (countof(FILE_UNICODE_EXTENSION) - 1) : m_PathIndices[nRoot - 1];
		return CWideStringView(m_String + offset, m_PathIndices[nLevel] - (bIncludeBackSlash ? 0 : 1) - offset);
	}

	void RemovePath()
	{
		m_PathIndex--;
		m_Length = m_PathIndices[m_PathIndex];
		m_String[m_Length] = L'\0';
	}

private:
	unsigned short m_PathIndex;
	unsigned short m_PathIndices[512];
};

template<class Derived>
class FileSearchData
{
public:
	FileSearchData()
	{
		m_PathId = 0;
		m_PathId_Filename = 0;
	};
	~FileSearchData() {};

	void SetWildCard(CWideStringView pWildCard) { m_Wildcard = pWildCard; }
	CWideStringView GetWildCard() { return m_Wildcard; };
	inline void SetStoreFlags(DWORD nFlags) { m_Flags = nFlags;	};
	inline int PathId() const { return m_PathId; }

	void SetRootPath(CWideStringView pDirectory)
	{
		m_PathId = 0;
		m_PathId_Filename = 0;
		m_Filename = (pDirectory + (countof(FILE_UNICODE_EXTENSION) - 1 + ((m_Flags & ADIREX_FULLPATH) ? 0 : pDirectory.Len)));
		m_Path_Filename_Len = m_Filename.Len();
		m_Path.AddPath(pDirectory);
	};

	void AddPath(PathSearchEntry& pEntry)
	{
		m_PathId = pEntry.PathId();
		m_Path.AddPath(pEntry.Filename());
	}

	void ReplacePath(PathSearchEntry& pEntry, unsigned short nLevel)
	{
		m_PathId = pEntry.PathId();
		m_Path.ReplacePath(pEntry.Filename(), nLevel);
	}

	CWideStringView Path()
	{
		return m_Path.StringView();
	}

	CStringView GetPath(int nRoot, int nLevel)
	{
		SyncPath(nRoot, nLevel);
		return m_Filename;
	}

	bool GetPathIdentifier(PathIdentifier& pid)
	{
		BY_HANDLE_FILE_INFORMATION by_handle_info;
		FILE_ID_INFO file_id_info;
		DWORD nLastError;
		CWideStringView dirname = static_cast<Derived*>(this)->JustFName();
		bool retval = true;

		m_Path.AddPath(dirname);
		HANDLE dir = CreateFileW(m_Path, FILE_LIST_DIRECTORY, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
		m_Path.RemovePath();

		if (dir == INVALID_HANDLE_VALUE)
		{
			nLastError = GetLastError();
			if (nLastError == ERROR_ACCESS_DENIED)
				return false;

			SaveWin32Error("CreateFile", nLastError);
			throw E_APIERROR;
		}

		if (fpGetFileInformationByHandleEx)
		{
			if (fpGetFileInformationByHandleEx(dir, 18 /*FileIdInfo*/, &file_id_info, sizeof(file_id_info)))
			{
				pid.dwVolumeSerialNumber = file_id_info.VolumeSerialNumber;
				memcpy(&pid.FileId, &file_id_info.FileId, sizeof(pid.FileId));
				goto close;
			}

			nLastError = GetLastError();
			if (nLastError != ERROR_INVALID_PARAMETER)
			{
				SaveWin32Error("GetFileInformationByHandleEx", nLastError);
				retval = false;
				goto close;
			}
		}

		if (GetFileInformationByHandle(dir, &by_handle_info))
		{
			pid.dwVolumeSerialNumber = by_handle_info.dwVolumeSerialNumber;
			pid.dwFileIndexLow = by_handle_info.nFileIndexLow;
			pid.dwFileIndexHigh = by_handle_info.nFileIndexHigh;
			pid.dwFileIdLow = 0;
			pid.dwFileIdHigh = 0;
		}
		else
		{
			nLastError = GetLastError();
			SaveWin32Error("GetFileInformationByHandle", nLastError);
			retval = false;
		}

	close:
		CloseHandle(dir);
		if (retval)
			return retval;
		else
			throw E_APIERROR;
	}

	CStringView StringFileAttributes(DWORD dwAttributes)
	{
		static struct { char cAttribute; unsigned char nSize; } alist[16] =
		{
			{ '\0', 0 },
			{ 'R', 1 },
			{ 'H', 1 },
			{ 'S', 1 },
			{ '\0', 0 },
			{ 'D', 1 },
			{ 'A', 1 },
			{ '\0', 0 },
			{ 'T', 1 },
			{ 'P', 1 },
			{ 'L', 1 },
			{ 'C', 1 },
			{ 'O', 1 },
			{ 'I', 1 },
			{ 'E', 1 },
			{ 'K', 1 }
		};

		DWORD nListIndex = 0, nIndex = 0;
		char* pAttributes = m_StringFileAttributes;
		for (unsigned int xj = 0; xj < countof(alist); xj++)
		{
			nListIndex = (dwAttributes & (1u << xj)) ? xj : 0;
			pAttributes[nIndex] = alist[nListIndex].cAttribute;
			nIndex += alist[nListIndex].nSize;
		}

		m_StringFileAttributes.Len(nIndex);
		return m_StringFileAttributes;
	}

protected:

	void SyncPath(int nRoot, int nLevel, bool bIncludeBackSlash = false)
	{
		if (m_PathId != m_PathId_Filename)
		{
			m_Filename = m_Path.GetPathWithoutUnicodeExtension(nRoot, nLevel, bIncludeBackSlash);
			m_Path_Filename_Len = m_Filename.Len();
			m_PathId_Filename = m_PathId;
		}
		else
		{
			m_Filename.Len(m_Path_Filename_Len);
		}
	}

	DWORD						m_Flags;
	int							m_PathId;
	int							m_PathId_Filename;
	unsigned int				m_Path_Filename_Len;
	CStrBuilder<16>				m_StringFileAttributes;
	CWideStrBuilder<MAX_PATH>	m_Wildcard;
	FoxString					m_JustFName;
	FoxString					m_AlternateName;
	FoxString					m_Filename;
	CWidePathBuilder			m_Path;
};

class Win32FileSearchData : public FileSearchData<Win32FileSearchData>
{
public:
	Win32FileSearchData() {};
	~Win32FileSearchData() {};
	bool IsFakePath() const;
	bool IsFakePathName() const;
	bool IsRealPath() const;
	FoxString& Filename();
	FoxString& AlternateFilename();
	CWideStringView WideAlternateFilename() const;
	unsigned __int64 FileSize() const;
	DWORD FileAttributes() const;
	FILETIME CreationTime() const;
	FILETIME LastAccessTime() const;
	FILETIME LastWriteTime() const;

	bool MatchFile() const;
	WIN32_FIND_DATAW* Ptr() { return &m_FileData; } const

	CWideStringView JustFName() const
	{
		if (m_FileData.cFileName[0] != '\0')
			return m_FileData.cFileName;
		else
			return m_FileData.cAlternateFileName;
	}

	CWideStringView SearchPattern(CWideStringView pPattern = 0)
	{
		m_Path.ReplaceFilename(pPattern.Data == 0 ? m_Wildcard : pPattern);
		return m_Path.StringView();
	}

private:
	WIN32_FIND_DATAW m_FileData;
};

class NtFileSearchData : public FileSearchData<NtFileSearchData>
{
public:
	NtFileSearchData() {};
	~NtFileSearchData() {};
	bool IsFakePath() const;
	bool IsFakePathName() const;
	bool IsRealPath() const;
	FoxString& Filename();
	FoxString& AlternateFilename();
	CWideStringView WideAlternateFilename() const;
	unsigned __int64 FileSize() const;
	DWORD FileAttributes() const;
	FILETIME CreationTime() const;
	FILETIME LastAccessTime() const;
	FILETIME LastWriteTime() const;
	bool MatchFile() const;

	inline void SetFileInfoClass(FILE_INFORMATION_CLASSEX fileinfoclass)
	{
		m_FileInfoClass = fileinfoclass;
		switch (fileinfoclass)
		{
		case FileDirectoryInformationEx:
			m_FileNameOffset = offsetof(FILE_DIRECTORY_INFORMATION, FileName);
			break;
		case FileFullDirectoryInformationEx:
			m_FileNameOffset = offsetof(FILE_FULL_DIRECTORY_INFORMATION, FileName);
			break;
		case FileBothDirectoryInformationEx:
			m_FileNameOffset = offsetof(FILE_BOTH_DIR_INFORMATION, FileName);
			break;
		}
	}

	inline void SetFileInfo(void* pBuffer) { m_FileInfo = (PFILE_BOTH_DIR_INFORMATION)pBuffer; }
	inline PFILE_BOTH_DIR_INFORMATION GetFileInfo() const { return m_FileInfo; }
	inline bool NextEntry()
	{
		if (m_FileInfo->NextEntryOffset)
		{ 
			m_FileInfo = AdvancePtr<PFILE_BOTH_DIR_INFORMATION>(m_FileInfo, m_FileInfo->NextEntryOffset); 
			return true;
		}
		return false;
	}

	CWideStringView JustFName() const
	{
		WCHAR* filename = FileNamePtr();
		return CWideStringView(filename, m_FileInfo->FileNameLength / 2);
	} 

private:
	inline WCHAR* FileNamePtr() const
	{ 
		return (WCHAR*)((unsigned char*)m_FileInfo + m_FileNameOffset);
	} 

	FILE_INFORMATION_CLASSEX		m_FileInfoClass;
	PFILE_BOTH_DIR_INFORMATION		m_FileInfo;
	size_t							m_FileNameOffset;
};

template<typename TBase = void>
class FileSearchStorage;

template<>
class FileSearchStorage<>
{
public:
	FileSearchStorage() { }

	// we need a virtual destructor - otherwise the destructor of objects in the descending classes don't run and cause memory leaks
	virtual ~FileSearchStorage() { };
	virtual void Initialize(CStringView pDestination, DWORD& nFlags, CStringView pFields) = 0;
	virtual void Finalize() = 0;
	virtual bool Store(void* pFileSearch, int nFileId) = 0;
	virtual bool StorePath(PathSearchEntry* pPath, void* pFileSearch, int nLevel) = 0;

protected:
	FoxString	m_FileName;
	FoxDateTime m_FileTime;
	FoxInt64	m_FileSize;
	DWORD		m_Flags;
	int m_Index_FileId;
	int m_Index_ParentId;
	int m_Index_Filename;
	int m_Index_Dosfilename;
	int m_Index_Accesstime;
	int m_Index_Creationtime;
	int m_Index_Writetime;
	int m_Index_Filesize;
	int m_Index_Fileattribs;
	int m_Index_StringFileattribs;
};

template<class TData>
class FileSearchStorageArray : public FileSearchStorage<>
{
	friend class FileSearchStorage<>;
public:
	void Initialize(CStringView pDestination, DWORD& nFlags, CStringView pFields);
	void Finalize();
	bool Store(void* pFileSearch, int nFileId);
	bool StorePath(PathSearchEntry* pPath, void* pFileSearch, int nLevel);

private:
	FoxArray	m_Array;
};

template<class TData>
class FileSearchStorageCursor : public FileSearchStorage<>
{
	friend class FileSearchStorage<>;
public:
	void Initialize(CStringView pDestination, DWORD& nFlags, CStringView pFields);
	void Finalize() { };
	bool Store(void* pFileSearch, int nFileId);
	bool StorePath(PathSearchEntry* pPath, void* pFileSearch, int nLevel);

private:
	FoxCursor	m_Cursor;
};

template<class TData>
class FileSearchStorageCallback : public FileSearchStorage<>
{
	friend class FileSearchStorage<>;
public:
	void Initialize(CStringView pDestination, DWORD& nFlags, CStringView pFields);
	void Finalize() {};
	bool Store(void* pFileSearch, int nFileId);
	bool StorePath(PathSearchEntry* pPath, void* pFileSearch, int nLevel);

private:
	FoxString m_FileNameAttributes;
	FoxDateTime m_CreationTime, m_AccessTime, m_WriteTime;
	CDynamicFoxCallback m_Callback;
};

typedef void(_stdcall *FileSearchCallbackFunc)(CStrBuilder<MAX_WIDE_PATH>& pPathName, DWORD nFileAttributes, LPVOID pParam);

template <typename TBase = void>
class FileSearchBase;

template<>
class FileSearchBase<>
{
public:
	virtual ~FileSearchBase() {};
	virtual unsigned int ExecuteSearch() = 0;
	virtual unsigned int ExecuteSearchCallback(FileSearchCallbackFunc pCallback, LPVOID pParam) = 0;
	virtual void ExecuteReverse(FileSearchCallbackFunc pCallback, LPVOID pParam) = 0;
	virtual bool FindFirst() = 0;
	virtual bool FindNext() = 0;

	void DisableFsRedirection();
	void RevertFsRedirection();

	typedef bool(*FileFilterFunc)(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch);
	static bool Filter_True(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch);
	static bool Filter_Match(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch);
	static bool Filter_Exact(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch);
	static bool Filter_Any(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch);
};

template<class TBase, class TData>
class FileSearch : public FileSearchBase<>
{
	friend class FileSearchBase<>;
public:
	FileSearch(CStringView pSearchPath = 0, DWORD nFileFilter = 0, DWORD nFilterMatch = 0, CStringView pDestination = 0,
		DWORD nFlags = 0, int nMaxRecursion = 0, CStringView pFields = 0)
	{
		m_FilterAttributes = (nFileFilter & ~FILE_ATTRIBUTE_FAKEDIRECTORY);
		m_FilterMatch = nFilterMatch;
		m_FilterFakePath = (nFileFilter & FILE_ATTRIBUTE_FAKEDIRECTORY) == 0;
		m_Flags = nFlags;
		m_Storage = 0;
		m_MaxRecursion = nMaxRecursion;
		m_Path = pSearchPath;
		m_WildCard = m_Path;
		m_WildCard.PathStripPath();

		if (m_Path.Len() == m_WildCard.Len())
		{
			m_Path.Len(GetCurrentDirectoryW(m_Path.Size(), m_Path));
			m_Path.AddBs();
		}
		else
		{
			m_Path.Len(m_Path.Len() - m_WildCard.Len(), true);
		}

		if (nFlags & ADIREX_DEST_ARRAY)
		{
			m_Storage = new FileSearchStorageArray<TData>();
			m_Storage->Initialize(pDestination, m_Flags, pFields);
		}
		else if (nFlags & ADIREX_DEST_CURSOR)
		{
			m_Storage = new FileSearchStorageCursor<TData>();
			m_Storage->Initialize(pDestination, m_Flags, pFields);
		}
		else if (nFlags & ADIREX_DEST_CALLBACK)
		{
			m_Storage = new FileSearchStorageCallback<TData>();
			m_Storage->Initialize(pDestination, m_Flags, pFields);
		}

		m_FilterFunc = FileSearchBase<>::Filter_True;
		m_FilterPathOnly = false;
		if (nFlags & ADIREX_FILTER_ALL)
		{
			m_FilterFunc = FileSearchBase<>::Filter_Match;
			m_FilterMatch = m_FilterAttributes;
			m_FilterPathOnly = (m_FilterMatch & FILE_ATTRIBUTE_DIRECTORY) > 0;
		}
		else if (nFlags & ADIREX_FILTER_NONE)
		{
			m_FilterFunc = FileSearchBase<>::Filter_Match;
			m_FilterMatch = 0;
		}
		else if (nFlags & ADIREX_FILTER_EXACT)
		{
			m_FilterFunc = FileSearchBase<>::Filter_Exact;
			m_FilterPathOnly = (m_FilterMatch & FILE_ATTRIBUTE_DIRECTORY) > 0;
		}
		else if (nFlags & ADIREX_FILTER_MATCH)
		{
			m_FilterFunc = FileSearchBase<>::Filter_Match;
			m_FilterPathOnly = ((m_FilterAttributes & m_FilterMatch) & FILE_ATTRIBUTE_DIRECTORY) > 0;
		}
		else if (m_FilterAttributes != 0)
		{
			m_FilterFunc = FileSearchBase<>::Filter_Any;
			m_FilterPathOnly = (m_FilterAttributes == FILE_ATTRIBUTE_DIRECTORY);
		}

		m_FileSearchData.SetWildCard(m_WildCard.StringView());
		m_FileSearchData.SetStoreFlags(m_Flags);
		m_ReparsePoints.AutoGrow(16);
	}

	~FileSearch()
	{
		if (m_Storage)
			delete m_Storage;
	}

	unsigned int ExecuteSearch()
	{
		m_FileCount = 0;
		m_FileId = 0;
		if ((m_Flags & ADIREX_DISABLE_FSREDIRECTION))
			DisableFsRedirection();

		bool (TBase:: * findfirst)() = (m_Flags & ADIREX_RECURSIVE) ? &TBase::FindFirstRecurse : &TBase::FindFirstInternal;
		bool (TBase:: * findnext)() = (m_Flags & ADIREX_RECURSIVE) ? &TBase::FindNextRecurse : &TBase::FindNextInternal;

		try
		{
			if (!(static_cast<TBase&>(*this).*findfirst)())
				return m_FileCount;

			do
			{
				// (m_FileSearchData.FileAttributes() & FILE_ATTRIBUTE_DIRECTORY)
				if (!m_FilterFunc(m_FileSearchData.FileAttributes(), m_FilterAttributes, m_FilterMatch) || !m_FileSearchData.MatchFile())
					continue;

				if ((m_Flags & (ADIREX_RECURSIVE | ADIREX_PARENTID | ADIREX_JUSTFNAME)) == (ADIREX_RECURSIVE | ADIREX_PARENTID | ADIREX_JUSTFNAME))
					StorePath();

				m_FileCount++;
				if (m_Storage->Store((void*)&m_FileSearchData, m_FileId))
					break;

			} while ((static_cast<TBase&>(*this).*findnext)());
		}
		catch (int nErrorNo)
		{
			if ((m_Flags & ADIREX_DISABLE_FSREDIRECTION))
				RevertFsRedirection();

			throw nErrorNo;
		}

		if ((m_Flags & ADIREX_DISABLE_FSREDIRECTION))
			RevertFsRedirection();

		if (m_Storage)
			m_Storage->Finalize();

		return m_FileCount;
	}

	unsigned int ExecuteSearchCallback(FileSearchCallbackFunc pCallback, LPVOID pParam)
	{
		m_FileCount = 0;
		m_FileId = 0;
		if ((m_Flags & ADIREX_DISABLE_FSREDIRECTION))
			DisableFsRedirection();

		bool (TBase:: * findfirst)() = (m_Flags & ADIREX_RECURSIVE) ? &TBase::FindFirstRecurse : &TBase::FindFirstInternal;
		bool (TBase::*findnext)() = (m_Flags & ADIREX_RECURSIVE) ? &TBase::FindNextRecurse : &TBase::FindNextInternal;

		try
		{
			if (!(static_cast<TBase&>(*this).*findfirst)())
				return m_FileCount;

			do
			{
				if (m_FilterFunc(m_FileSearchData.FileAttributes(), m_FilterAttributes, m_FilterMatch))
				{
					if ((m_Flags & ADIREX_RECURSIVE) && !m_FileSearchData.MatchFile())
						continue;

					m_FileCount++;
					m_StrBuilder = m_FileSearchData.Filename();
					(*pCallback)(m_StrBuilder, m_FileSearchData.FileAttributes(), pParam);
				}
			} while ((static_cast<TBase&>(*this).*findnext)());
		}
		catch (int nErrorNo)
		{
			if ((m_Flags & ADIREX_DISABLE_FSREDIRECTION))
				RevertFsRedirection();

			throw nErrorNo;
		}

		if ((m_Flags & ADIREX_DISABLE_FSREDIRECTION))
			RevertFsRedirection();

		if (m_Storage)
			m_Storage->Finalize();

		return m_FileCount;
	}

	void ExecuteReverse(FileSearchCallbackFunc pCallback, LPVOID pParam)
	{
		m_FileCount = 0;
		m_FileId = 0;
		if ((m_Flags & ADIREX_DISABLE_FSREDIRECTION))
			DisableFsRedirection();

		CUnboundBlockStack<FileSearchEntry, 36864, 36864 / 64>  pDirectories;
		CUnboundBlockStack<FileSearchEntry, 36864, 36864 / 64>  pFiles;
		FileSearchEntry pSearchEntry;
		pSearchEntry.RecursionLevel = 0;

		try
		{
			if (!static_cast<TBase*>(this)->FindFirstReverse())
				return;

			do
			{
				pSearchEntry.Attributes = m_FileSearchData.FileAttributes();
				pSearchEntry.Path = m_FileSearchData.Filename();
				if (m_FileSearchData.IsRealPath())
				{
					pDirectories.Enqueue(pSearchEntry);
				}
				else
				{
					pFiles.Enqueue(pSearchEntry);
				}
			} while (static_cast<TBase*>(this)->FindNextReverse());

			while (pFiles.Dequeue(pSearchEntry))
			{
				m_StrBuilder = pSearchEntry.Path;
				(*pCallback)(m_StrBuilder, pSearchEntry.Attributes, pParam);
			}

			while (pDirectories.Dequeue(pSearchEntry))
			{
				m_StrBuilder = pSearchEntry.Path;
				(*pCallback)(m_StrBuilder, pSearchEntry.Attributes, pParam);
			}
		}
		catch (int nErrorNo)
		{
			if ((m_Flags & ADIREX_DISABLE_FSREDIRECTION))
				RevertFsRedirection();
			throw nErrorNo;
		}

		if ((m_Flags & ADIREX_DISABLE_FSREDIRECTION))
			RevertFsRedirection();
	}

	bool FindFirst()
	{
		if ((m_Flags & ADIREX_RECURSIVE) == 0)
			return static_cast<TBase*>(this)->FindFirstInternal();
		else
			return static_cast<TBase*>(this)->FindFirstRecurse();
	}

	bool FindNext()
	{
		if ((m_Flags & ADIREX_RECURSIVE) == 0)
			return static_cast<TBase*>(this)->FindNextInternal();
		else
			return static_cast<TBase*>(this)->FindNextRecurse();
	}

	bool IsRealPath()
	{
		return m_FileSearchData.IsRealPath();
	}

	bool IsFakePath()
	{
		return m_FileSearchData.IsFakePath();
	}

	int FileSize()
	{
		return m_FileSearchData.FileSize();
	}

protected:

	void QueuePath()
	{
		PathSearchEntry pPathEntry;
		PathIdentifier pid;
		if (m_FileSearchData.FileAttributes() & FILE_ATTRIBUTE_REPARSE_POINT)
		{
			if (m_FileSearchData.GetPathIdentifier(pid))
			{
				if (m_ReparsePoints.Find(pid) == -1)
					m_ReparsePoints.Add(pid);
				else
					return;
			}
		}

		pPathEntry.m_Attributes = m_FileSearchData.FileAttributes();
		pPathEntry.m_PathId = m_FileId;
		pPathEntry.m_ParentId = m_FileSearchData.PathId();
		pPathEntry.m_State = Unsaved;
		pPathEntry.m_CreationTime = m_FileSearchData.CreationTime();
		pPathEntry.m_LastAccessTime = m_FileSearchData.LastAccessTime();
		pPathEntry.m_LastWriteTime = m_FileSearchData.LastWriteTime();
		pPathEntry.m_Name = m_FileSearchData.JustFName();
		pPathEntry.m_DosName = m_FileSearchData.WideAlternateFilename();
		if (m_RecursionLevel + 1 > m_PathQueue.Max())
		{
			SaveCustomError("AdirEx", "Directory depth exceeds %I limit.", m_PathQueue.Max());
			throw E_APIERROR;
		}
		m_PathQueue[m_RecursionLevel + 1].Enqueue(pPathEntry);
	}

	bool DequeuePath()
	{
		PathSearchEntry pEntry;
		for (unsigned int xj = m_RecursionLevel + 1; xj > 0; xj--)
		{
			if (m_PathQueue[xj].Dequeue(pEntry))
			{
				if (xj > m_RecursionLevel)
					m_FileSearchData.AddPath(pEntry);
				else
					m_FileSearchData.ReplacePath(pEntry, xj);

				m_RecursionLevel = xj;
				return true;
			}
		}
		return false;
	}

	void StorePath()
	{
		int pathid;
		PathSearchEntry* pDir;
		unsigned int recurseLevel, saveLevel;

		if (m_FileSearchData.FileAttributes() & FILE_ATTRIBUTE_DIRECTORY)
		{
			pDir = m_PathQueue[m_RecursionLevel + 1].PeekLast();
			if (pDir && pDir->PathId() == m_FileId)
				pDir->m_State = Saved;
		}

		pathid = m_FileSearchData.PathId();
		saveLevel = m_RecursionLevel + 1;
		recurseLevel = (m_Flags & ADIREX_PARENT_JUSTFNAME) ? 0 : m_RecursionLevel;
		for (unsigned int xj = m_RecursionLevel; xj >= recurseLevel; xj--)
		{
			pDir = m_PathQueue[xj].Peek();
			if (pDir && pDir->PathId() == pathid && pDir->m_State == Unsaved)
			{
				pDir->m_State = Queued;
				pathid = pDir->m_ParentId;
				saveLevel = xj;
			}
			else
				break;
		}

		for (unsigned int xj = saveLevel; xj <= m_RecursionLevel; xj++)
		{
			pDir = m_PathQueue[xj].Peek();
			if (pDir && pDir->m_State == Queued)
			{
				m_FileCount++;
				m_Storage->StorePath(pDir, &m_FileSearchData, xj);
				pDir->m_State = Saved;
			}
		}
	}

	bool IgnorableSearchError(DWORD nLastError, bool bIgnoreAccessDenied) const
	{
		return nLastError == ERROR_FILE_NOT_FOUND || (bIgnoreAccessDenied && (nLastError == ERROR_ACCESS_DENIED || nLastError == ERROR_SHARING_VIOLATION));
	}

	DWORD				m_Flags;
	FileFilterFunc		m_FilterFunc;
	DWORD				m_FilterAttributes;
	DWORD				m_FilterMatch;
	int					m_FileCount;
	int					m_FileId;
	unsigned int		m_MaxRecursion;
	unsigned int		m_RecursionLevel;
	bool				m_FilterFakePath;
	bool				m_FilterPathOnly;
	FileSearchStorage<>*  m_Storage;
	TData				m_FileSearchData;
	CArray<PathIdentifier>	m_ReparsePoints;
	typedef CUnboundBlockQueue<PathSearchEntry, 4096, 64> PathQueue;
	StaggeredArray<PathQueue, 64, 64 * 32> m_PathQueue;
	CWideStrBuilder<MAX_PATH>		m_WildCard;
	CStrBuilder<MAX_WIDE_PATH>	m_StrBuilder;
	CWideStrBuilder<MAX_WIDE_PATH>	m_Path;
};

class Win32FileSearch : public FileSearch<Win32FileSearch, Win32FileSearchData>
{
	friend class FileSearch<Win32FileSearch, Win32FileSearchData>;
public:
	Win32FileSearch(CStringView pSearchPath, DWORD nFileFilter, DWORD nFilterMatch, CStringView pDestination,
		DWORD nFlags, int nMaxRecursion, CStringView pFields);
	~Win32FileSearch();

protected:
	bool FindFirstInternal();
	bool FindNextInternal();
	bool FindFirstRecurse();
	bool FindNextRecurse();
	bool FindFirstReverse();
	bool FindNextReverse();
	
	static DWORD GetFindFirstFlags();
	static void SetFindFirstFlags(DWORD dwFlags);
	static DWORD FindFirstFlags;

	FINDEX_INFO_LEVELS GetFindFirstInfoLevel();
	static FINDEX_INFO_LEVELS FindFirstInfoLevel;

	HANDLE	m_Handle;
};

class NtFileSearch : public FileSearch<NtFileSearch, NtFileSearchData>
{
	friend class FileSearch<NtFileSearch, NtFileSearchData>;
public:
	NtFileSearch(CStringView pSearchPath, DWORD nFileFilter, DWORD nFilterMatch, CStringView pDestination, DWORD nFlags, int nMaxRecursion, CStringView pFields);
	~NtFileSearch();
	 
protected:
	bool FindFirstInternal();
	bool FindNextInternal();
	bool FindFirstRecurse();
	bool FindNextRecurse();
	bool FindFirstReverse();
	bool FindNextReverse();
	bool SkipFakeDirectories();

	enum
	{
		NTQUERYDIRECTORY_BUFFERSIZE = 1024 * 64
	};

	HANDLE						m_DirectoryHandle;
	NTSTATUS					m_IoResult;
	IO_STATUS_BLOCK				m_IoStatusBlock;
	FILE_INFORMATION_CLASSEX	m_FileInfoClass;
	PFILE_BOTH_DIR_INFORMATION	m_FileInfo;
	CAlignedBlob<NTQUERYDIRECTORY_BUFFERSIZE>		m_FileInfoBuffer;
};

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

typedef struct _SETFILEATTRIBUTEPARAM
{
	bool bClearOrSet;
	DWORD nFileAttributesSet;
	DWORD nFileAttributesClear;
} SETFILEATTRIBUTEPARAM, *LPSETFILEATTRIBUTEPARAM;


#ifdef __cplusplus
extern "C" {
#endif

// function prototypes of vfp2cfile.c
void _stdcall VFP2C_Init_File(VFP2CTls& tls);
void _stdcall VFP2C_Destroy_File(VFP2CTls& tls);

void _fastcall ADirEx(ParamBlkEx& parm);
void _fastcall AFileAttributes(ParamBlkEx& parm);
void _fastcall AFileAttributesEx(ParamBlkEx& parm);
void _fastcall ADirectoryInfo(ParamBlkEx& parm);
void _fastcall GetFileTimes(ParamBlkEx& parm);
void _fastcall SetFileTimes(ParamBlkEx& parm);
void _fastcall GetFileSizeExLib(ParamBlkEx& parm);
void _fastcall GetFileAttributesEx2(ParamBlkEx& parm);
void _fastcall SetFileAttributesEx(ParamBlkEx& parm);
void _fastcall GetFileOwner(ParamBlkEx& parm);
void _fastcall GetLongPathNameEx(ParamBlkEx& parm);
void _fastcall GetShortPathNameEx(ParamBlkEx& parm);
void _fastcall GetOpenFileNameEx(ParamBlkEx& parm);
void _fastcall GetSaveFileNameEx(ParamBlkEx& parm);
UINT_PTR _stdcall GetFileNameCallback(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void _fastcall ADriveInfo(ParamBlkEx& parm);
void _fastcall AVolumes(ParamBlkEx& parm);
void _fastcall AVolumeMountPoints(ParamBlkEx& parm);
void _fastcall AVolumePaths(ParamBlkEx& parm);
void _fastcall AVolumeInformation(ParamBlkEx& parm);
void _fastcall GetWindowsDirectoryEx(ParamBlkEx& parm);
void _fastcall GetSystemDirectoryEx(ParamBlkEx& parm);
void _fastcall ExpandEnvironmentStringsEx(ParamBlkEx& parm);
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

void _fastcall MemoFieldInfo(ParamBlkEx& parm);

#pragma warning(disable : 4290) // disable warning 4290 - VC++ doesn't implement throw ...
void _stdcall MapFileAccessFlags(int nFileAttribs, int nAccess, int nShare, LPDWORD pAccess, LPDWORD pShare, LPDWORD pFlags) throw(int);
#pragma warning(default : 4290) // enable warning 4290 - VC++ doesn't implement throw ...

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