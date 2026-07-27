#include "vfp2c32.h"
#include "vfp2cfile.h"

// dynamic function pointers for runtime linking
static PGETVOLUMEPATHNAMESFORVOLUMENAME fpGetVolumePathNamesForVolumeName = 0;
static PWOW64FSREDIRECTION fpWow64DisableWow64FsRedirection = 0;
static PWOW64FSREDIRECTION fpWow64RevertWow64FsRedirection = 0;
static PGETFILEINFORMATIONBYHANDLEEX fpGetFileInformationByHandleEx = 0;

static PNTOPENFILE fpNtOpenFile = 0;
static PNTCLOSE fpNtClose = 0;
static PNTQUERYDIRECTORYFILE fpNtQueryDirectoryFile = 0;
static PNTQUERYDIRECTORYFILEEX fpNtQueryDirectoryFileEx = 0;
static PRTLDOSPATHNAMETONTPATHNAME_U fpRtlDosPathNameToNtPathName_U = 0;
static PRTLDOSPATHNAMETONTPATHNAME_U_WITHSTATUS fpRtlDosPathNameToNtPathName_U_WithStatus = 0;
static PRTLDOSLONGPATHNAMETONTPATHNAME_U_WITHSTATUS fpRtlDosLongPathNameToNtPathName_U_WithStatus = 0;
static PRTLINITUNICODESTRING fpRtlInitUnicodeString = 0;
static PRTLFREEUNICODESTRING fpRtlFreeUnicodeString = 0;
static PRTLNTSTATUSTODOSERROR fpRtlNtStatusToDosError = 0;

// Initialization
void _stdcall VFP2C_Init_File(VFP2CTls& tls)
{
	unused(tls);
	HMODULE hDll;
	hDll = GetModuleHandle("kernel32.dll");
	if (hDll)
	{
		fpGetVolumePathNamesForVolumeName = (PGETVOLUMEPATHNAMESFORVOLUMENAME)GetProcAddress(hDll, "GetVolumePathNamesForVolumeNameA");
		fpWow64DisableWow64FsRedirection = (PWOW64FSREDIRECTION)GetProcAddress(hDll, "Wow64DisableWow64FsRedirection");
		fpWow64RevertWow64FsRedirection = (PWOW64FSREDIRECTION)GetProcAddress(hDll, "Wow64RevertWow64FsRedirection");
		fpGetFileInformationByHandleEx = (PGETFILEINFORMATIONBYHANDLEEX)GetProcAddress(hDll, "GetFileInformationByHandleEx");
	}
	else
	{
		AddWin32Error("GetModuleHandle", GetLastError());
	}

	hDll = GetModuleHandle("ntdll.dll");
	if (hDll)
	{
		fpNtOpenFile = (PNTOPENFILE)GetProcAddress(hDll, "NtOpenFile");
		fpNtClose = (PNTCLOSE)GetProcAddress(hDll, "NtClose");
		fpNtQueryDirectoryFile = (PNTQUERYDIRECTORYFILE)GetProcAddress(hDll, "NtQueryDirectoryFile");
		fpNtQueryDirectoryFileEx = (PNTQUERYDIRECTORYFILEEX)GetProcAddress(hDll, "NtQueryDirectoryFileEx");
		fpRtlDosPathNameToNtPathName_U = (PRTLDOSPATHNAMETONTPATHNAME_U)GetProcAddress(hDll, "RtlDosPathNameToNtPathName_U");
		fpRtlDosPathNameToNtPathName_U_WithStatus = (PRTLDOSPATHNAMETONTPATHNAME_U_WITHSTATUS)GetProcAddress(hDll, "RtlDosPathNameToNtPathName_U_WithStatus");
		fpRtlDosLongPathNameToNtPathName_U_WithStatus = (PRTLDOSLONGPATHNAMETONTPATHNAME_U_WITHSTATUS)GetProcAddress(hDll, "RtlDosLongPathNameToNtPathName_U_WithStatus");
		fpRtlInitUnicodeString = (PRTLINITUNICODESTRING)GetProcAddress(hDll, "RtlInitUnicodeString");
		fpRtlFreeUnicodeString = (PRTLFREEUNICODESTRING)GetProcAddress(hDll, "RtlFreeUnicodeString");
		fpRtlNtStatusToDosError = (PRTLNTSTATUSTODOSERROR)GetProcAddress(hDll, "RtlNtStatusToDosError");
	}
	else
	{
		AddWin32Error("GetModuleHandle", GetLastError());
	}
}

// cleanup
void _stdcall VFP2C_Destroy_File(VFP2CTls& tls)
{
	unsigned int nCount = tls.FileHandles.GetCount();
	for (unsigned int xj = 0; xj < nCount; xj++)
	{
		// release all file handles
		CloseHandle(tls.FileHandles[xj]);
	}
	tls.FileHandles.ResetIndex();
}

bool Win32FileSearchData::IsFakePathName() const
{
	if (m_FileData.cFileName[0] == L'.')
		return (m_FileData.cFileName[1] == L'\0' || (m_FileData.cFileName[1] == L'.' && m_FileData.cFileName[2] == L'\0'));
	return false;
}

bool Win32FileSearchData::IsRealPath() const
{
	return (m_FileData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | ((m_Flags & ADIREX_INCLUDE_REPARSE_POINT) ? 0 : FILE_ATTRIBUTE_REPARSE_POINT))) == FILE_ATTRIBUTE_DIRECTORY && !IsFakePathName();
}

// a fake directry ("." & "..")
bool Win32FileSearchData::IsFakePath() const
{
	return (m_FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY && IsFakePathName();
}

FoxString& Win32FileSearchData::Filename()
{
	if ((m_Flags & ADIREX_JUSTFNAME) == 0)
	{
		SyncPath((m_Flags & ADIREX_FULLPATH) ? 0 : 1, -1, true);
		if (m_FileData.cFileName[0] != L'\0')
			m_Filename += CWideStringView(m_FileData.cFileName);
		else
			m_Filename += CWideStringView(m_FileData.cAlternateFileName);

		return m_Filename;
	}
	else
	{
		if (m_FileData.cFileName[0] != L'\0')
			m_JustFName = CWideStringView(m_FileData.cFileName);
		else
			m_JustFName = CWideStringView(m_FileData.cAlternateFileName);

		return m_JustFName;
	}
}

FoxString& Win32FileSearchData::AlternateFilename()
{
	m_AlternateName = CWideStringView(m_FileData.cAlternateFileName);
	return m_AlternateName;
}

CWideStringView Win32FileSearchData::WideAlternateFilename() const
{
	return CWideStringView(m_FileData.cAlternateFileName);
}

unsigned __int64 Win32FileSearchData::FileSize() const
{
	ULARGE_INTEGER nSize;
	nSize.HighPart = m_FileData.nFileSizeHigh;
	nSize.LowPart = m_FileData.nFileSizeLow;
	return nSize.QuadPart;
}

DWORD Win32FileSearchData::FileAttributes() const
{
	return m_FileData.dwFileAttributes;
}

FILETIME Win32FileSearchData::CreationTime() const
{
	return m_FileData.ftCreationTime;
}

FILETIME Win32FileSearchData::LastAccessTime() const
{
	return m_FileData.ftLastAccessTime;
}

FILETIME Win32FileSearchData::LastWriteTime() const
{
	return m_FileData.ftLastWriteTime;
}

bool Win32FileSearchData::MatchFile() const
{
	// skip filtering if not in recursive mode and if wildcard count is 1, since then the filtering is done by the file enumeration API itself
	if ((m_Flags & ADIREX_RECURSIVE) == 0 && m_WildcardsCount == 1)
		return true;

	if ((m_Flags & ADIREX_NODOSNAME) || m_FileData.cAlternateFileName[0] == L'\0')
	{
		for (unsigned int xj = 0; xj < m_WildcardsCount; xj++)
		{
			if (PathMatchSpecW(m_FileData.cFileName, m_Wildcards[xj]))
				return true;
		}
	}
	else
	{
		for (unsigned int xj = 0; xj < m_WildcardsCount; xj++)
		{
			if (PathMatchSpecW(m_FileData.cFileName, m_Wildcards[xj]) || PathMatchSpecW(m_FileData.cAlternateFileName, m_Wildcards[xj]))
				return true;
		}
	}
	return false;
}

bool NtFileSearchData::IsRealPath() const
{
	return (m_FileInfo->FileAttributes & (FILE_ATTRIBUTE_DIRECTORY | ((m_Flags & ADIREX_INCLUDE_REPARSE_POINT) ? 0 : FILE_ATTRIBUTE_REPARSE_POINT))) == FILE_ATTRIBUTE_DIRECTORY && !IsFakePathName();
}

// a fake directry ("." & "..")
bool NtFileSearchData::IsFakePath() const
{
	return (m_FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY && IsFakePathName();
}

bool NtFileSearchData::IsFakePathName() const
{
	WCHAR* filename = FileNamePtr();
	return (m_FileInfo->FileNameLength == 2 && filename[0] == L'.') ||
		(m_FileInfo->FileNameLength == 4 && filename[0] == L'.' && filename[1] == L'.');
}

FoxString& NtFileSearchData::Filename()
{
	WCHAR* filename = FileNamePtr();
	if ((m_Flags & ADIREX_JUSTFNAME) == 0)
	{
		SyncPath((m_Flags & ADIREX_FULLPATH) ? 0 : 1, -1, true);
		if (m_FileInfo->FileNameLength > 0)
			m_Filename += CWideStringView(filename, m_FileInfo->FileNameLength / 2);
		else if (m_FileInfoClass == FileBothDirectoryInformationEx && m_FileInfo->ShortNameLength > 0)
			m_Filename += CWideStringView(m_FileInfo->ShortName, m_FileInfo->ShortNameLength / 2);

		return m_Filename;
	}
	else
	{
		if (m_FileInfo->FileNameLength > 0)
			m_JustFName = CWideStringView(filename, m_FileInfo->FileNameLength / 2);
		else if (m_FileInfoClass == FileBothDirectoryInformationEx && m_FileInfo->ShortNameLength > 0)
			m_JustFName = CWideStringView(m_FileInfo->ShortName, m_FileInfo->ShortNameLength / 2);
		else
			m_JustFName.Len(0);

		return m_JustFName;
	}
}

FoxString& NtFileSearchData::AlternateFilename()
{
	if (m_FileInfoClass == FileBothDirectoryInformationEx)
	{
		if (m_FileInfo->ShortNameLength > 0)
			m_AlternateName = CWideStringView(m_FileInfo->ShortName, m_FileInfo->ShortNameLength / 2);
		else
			m_AlternateName.Len(0);
	}
	return m_AlternateName;
}

CWideStringView NtFileSearchData::WideAlternateFilename() const
{
	if (m_FileInfoClass == FileBothDirectoryInformationEx && m_FileInfo->ShortNameLength > 0)
		return CWideStringView(&m_FileInfo->ShortName[0], m_FileInfo->ShortNameLength / 2);

	return 0;
}

bool NtFileSearchData::MatchFile() const
{
	// skip filtering if not in recursive mode and if wildcard count is 1, since then the filtering is done by the file enumeration API itself
	if ((m_Flags & ADIREX_RECURSIVE) == 0 && m_WildcardsCount == 1)
		return true;

	WCHAR* filename = FileNamePtr();
	// the filenames are not nullterminated, to avoid a costly copy we nullterminate them inline, and then restore the previous data afterwards
	bool ret = false;
	wchar_t* pEnd = &filename[m_FileInfo->FileNameLength / 2];
	wchar_t pPrev = *pEnd;
	*pEnd = L'\0';
	for (unsigned int xj = 0; xj < m_WildcardsCount; xj++)
	{
		ret = (PathMatchSpecW(filename, m_Wildcards[xj]) == TRUE);
		if (ret)
			break;
	}
	*pEnd = pPrev;
	if (!ret && m_FileInfoClass == FileBothDirectoryInformationEx && m_FileInfo->ShortNameLength)
	{
		pEnd = &m_FileInfo->ShortName[m_FileInfo->ShortNameLength / 2];
		pPrev = *pEnd;
		*pEnd = L'\0';
		for (unsigned int xj = 0; xj < m_WildcardsCount; xj++)
		{
			ret = (PathMatchSpecW(&m_FileInfo->ShortName[0], m_Wildcards[xj]) == TRUE);
			if (ret)
				break;
		}
		*pEnd = pPrev;
	}
	return ret;
}

unsigned __int64 NtFileSearchData::FileSize() const
{
	return m_FileInfo->EndOfFile.QuadPart;
}

DWORD NtFileSearchData::FileAttributes() const
{
	return m_FileInfo->FileAttributes;
}

FILETIME NtFileSearchData::CreationTime() const
{
	FILETIME time;
	time.dwLowDateTime = m_FileInfo->CreationTime.LowPart;
	time.dwHighDateTime = m_FileInfo->CreationTime.HighPart;
	return time;
}

FILETIME NtFileSearchData::LastAccessTime() const
{
	FILETIME time;
	time.dwLowDateTime = m_FileInfo->LastAccessTime.LowPart;
	time.dwHighDateTime = m_FileInfo->LastAccessTime.HighPart;
	return time;
}

FILETIME NtFileSearchData::LastWriteTime() const
{
	FILETIME time;
	time.dwLowDateTime = m_FileInfo->LastWriteTime.LowPart;
	time.dwHighDateTime = m_FileInfo->LastWriteTime.HighPart;
	return time;
}

void FileSearchStorage<>::Initialize(CStringView pDestination, DWORD& nFlags, CStringView pFields)
{
	m_FileName.Size(MAX_PATH);
	m_Flags = nFlags;
}

template<class TData>
void FileSearchStorageArray<TData>::Initialize(CStringView pDestination, DWORD& nFlags, CStringView pFields)
{
	FileSearchStorage<>::Initialize(pDestination, nFlags, pFields);
	unsigned int nFieldCount = 7;
	if (pFields)
	{
		m_Index_FileId = m_Index_ParentId = m_Index_Filename = m_Index_Dosfilename = m_Index_Accesstime = m_Index_Creationtime =
			m_Index_Writetime = m_Index_Filesize = m_Index_Fileattribs = m_Index_StringFileattribs = -1;
		pFields = pFields.Alltrim();
		CStringView pFieldName = pFields.GetWordNum(1, ',');
		unsigned int nFieldNo = 1;
		do 
		{
			CStringView pField = pFieldName.Alltrim();
			if (pField.ICompare(__sv("filename")))
			{
				m_Index_Filename = nFieldNo;
			}
			else if (pField.ICompare(__sv("dosfilename")))
			{
				m_Index_Dosfilename = nFieldNo;
			}
			else if (pField.ICompare(__sv("creationtime")))
			{
				m_Index_Creationtime = nFieldNo;
			}
			else if (pField.ICompare(__sv("accesstime")))
			{
				m_Index_Accesstime = nFieldNo;
			}
			else if (pField.ICompare(__sv("writetime")))
			{
				m_Index_Writetime = nFieldNo;
			}
			else if (pField.ICompare(__sv("filesize")))
			{
				m_Index_Filesize = nFieldNo;
			}
			else if (pField.ICompare(__sv("fileattribs")))
			{
				m_Index_Fileattribs = nFieldNo;
			}
			else if (pField.ICompare(__sv("cfileattribs")))
			{
				m_Index_StringFileattribs = nFieldNo;
			}
			else if (pField.ICompare(__sv("fileid")))
			{
				m_Index_FileId = nFieldNo;
			}
			else if (pField.ICompare(__sv("parentid")))
			{
				m_Index_ParentId = nFieldNo;
			}
			else
			{
				SaveCustomError("AdirEx", "Invalid parameter name '%V' in field list", &pField);
				throw E_APIERROR;
			}

			pFields = pFields + (pFieldName.Len + 1);
			pFieldName = pFields.GetWordNum(1, ',');
			nFieldNo++;
		} while (pFieldName);
		nFieldCount = nFieldNo - 1;
	}
	else
	{
		int nIndex = 1;
		m_Index_Filename = nIndex++;

		if ((nFlags & ADIREX_NODOSNAME))
			m_Index_Dosfilename = -1;
		else
			m_Index_Dosfilename = nIndex++;

		m_Index_Creationtime = nIndex++;
		m_Index_Accesstime = nIndex++;
		m_Index_Writetime = nIndex++;
		m_Index_Filesize = nIndex++;

		m_Index_Fileattribs = (nFlags & ADIREX_STRING_FILEATTRIBUTES) ? -1 : nIndex;
		m_Index_StringFileattribs = (nFlags & ADIREX_STRING_FILEATTRIBUTES) ? nIndex : -1;
		nIndex++;

		if (nFlags & ADIREX_FILEID)
			m_Index_FileId = nIndex++;
		else
			m_Index_FileId = -1;

		if (nFlags & ADIREX_PARENTID)
			m_Index_ParentId = nIndex++;
		else
			m_Index_ParentId = -1;

		nFieldCount = nIndex;
	}

	nFlags |= m_Index_Dosfilename == -1 ? ADIREX_NODOSNAME : 0;
	nFlags |= m_Index_ParentId >= 0 ? ADIREX_PARENTID : 0;

	m_Array.AutoGrow(256);
	m_Array.Dimension(pDestination, m_Array.AutoGrow(), nFieldCount);
	m_Array.AutoOverflow(CFoxVersion::MajorVersion() >= 9);
}

template<class TData>
void FileSearchStorageArray<TData>::Finalize()
{
	m_Array.CompactOverflow();
}

template<class TData>
bool FileSearchStorageArray<TData>::Store(void* pFileSearch, int nFileId)
{
	TData* pFS = (TData*)pFileSearch;
	int nRow = m_Array.Grow();

	if (m_Index_Filename >= 1)
		m_Array(nRow, m_Index_Filename) = pFS->Filename();

	if (m_Index_Dosfilename >= 1)
		m_Array(nRow, m_Index_Dosfilename) = pFS->AlternateFilename();

	if (m_Index_Creationtime >= 1)
	{
		m_FileTime = pFS->CreationTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Array(nRow, m_Index_Creationtime) = m_FileTime;
	}

	if (m_Index_Accesstime >= 1)
	{
		m_FileTime = pFS->LastAccessTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Array(nRow, m_Index_Accesstime) = m_FileTime;
	}

	if (m_Index_Writetime >= 1)
	{
		m_FileTime = pFS->LastWriteTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Array(nRow, m_Index_Writetime) = m_FileTime;
	}

	if (m_Index_Filesize >= 1)
		m_Array(nRow, m_Index_Filesize) = m_FileSize = pFS->FileSize();

	if (m_Index_Fileattribs >= 1)
		m_Array(nRow, m_Index_Fileattribs) = pFS->FileAttributes();
	
	if (m_Index_StringFileattribs >= 1)
		m_Array(nRow, m_Index_StringFileattribs) = m_FileName.FileAttributesToString(pFS->FileAttributes());

	if (m_Index_FileId >= 1)
		m_Array(nRow, m_Index_FileId) = nFileId;

	if (m_Index_ParentId >= 1)
		m_Array(nRow, m_Index_ParentId) = pFS->PathId();

	return false;
}

template<class TData>
bool FileSearchStorageArray<TData>::StorePath(PathSearchEntry* pPath, void* pFileSearch, int nLevel)
{
	int nRow = m_Array.Grow();
	TData* pData = (TData*)pFileSearch;

	if (m_Index_Filename >= 1)
	{
		if (m_Flags & ADIREX_PARENT_JUSTFNAME)
			m_Array(nRow, m_Index_Filename) = m_FileName = pPath->Filename();
		else
			m_Array(nRow, m_Index_Filename) = m_FileName = pData->GetPath((m_Flags & ADIREX_PARENT_FULLPATH) ? 0 : 1, nLevel);
	}

	if (m_Index_Dosfilename >= 1)
		m_Array(nRow, m_Index_Dosfilename) = pPath->AlternateFilename();

	if (m_Index_Creationtime >= 1)
	{
		m_FileTime = pPath->CreationTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Array(nRow, m_Index_Creationtime) = m_FileTime;
	}

	if (m_Index_Accesstime >= 1)
	{
		m_FileTime = pPath->LastAccessTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Array(nRow, m_Index_Accesstime) = m_FileTime;
	}

	if (m_Index_Writetime >= 1)
	{
		m_FileTime = pPath->LastWriteTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Array(nRow, m_Index_Writetime) = m_FileTime;
	}

	if (m_Index_Filesize >= 1)
		m_Array(nRow, m_Index_Filesize) = m_FileSize = pPath->FileSize();

	if (m_Index_Fileattribs >= 1)
		m_Array(nRow, m_Index_Fileattribs) = pPath->FileAttributes();

	if (m_Index_StringFileattribs >= 1)
		m_Array(nRow, m_Index_StringFileattribs) = m_FileName.FileAttributesToString(pPath->FileAttributes());

	if (m_Index_FileId >= 1)
		m_Array(nRow, m_Index_FileId) = pPath->PathId();

	if (m_Index_ParentId >= 1)
	{
		if (m_Flags & ADIREX_PARENT_JUSTFNAME)
			m_Array(nRow, m_Index_ParentId) = pPath->ParentId();
		else
			m_Array(nRow, m_Index_ParentId) = 0;
	}

	return false;
}

template<class TData>
void FileSearchStorageCursor<TData>::Initialize(CStringView pDestination, DWORD& nFlags, CStringView pFields)
{
	FileSearchStorage<>::Initialize(pDestination, nFlags, pFields);
	unsigned int nFieldNo = 0;
	int nErrorNo;
	CStringView pField;
	CStringView pFieldName;
	CStrBuilder<256> pCursorDefinition;

	m_Index_FileId = m_Index_ParentId = m_Index_Filename = m_Index_Dosfilename = m_Index_Accesstime = m_Index_Creationtime =
		m_Index_Writetime = m_Index_Filesize = m_Index_Fileattribs = m_Index_StringFileattribs = -1;

	if (pFields)
	{
		pFields = pFields.Alltrim();
		nErrorNo = m_Cursor.Attach(pDestination, pFields);
		if (nErrorNo == 0)
		{
			pFieldName = pFields.GetWordNum(1, ',');
			while (pFieldName)
			{
				pField = pFieldName.Alltrim();

				if (pField.ICompare(__sv("filename")))
					m_Index_Filename = nFieldNo;
				else if (pField.ICompare(__sv("dosfilename")))
					m_Index_Dosfilename = nFieldNo;
				else if (pField.ICompare(__sv("creationtime")))
					m_Index_Creationtime = nFieldNo;
				else if (pField.ICompare(__sv("accesstime")))
					m_Index_Accesstime = nFieldNo;
				else if (pField.ICompare(__sv("writetime")))
					m_Index_Writetime = nFieldNo;
				else if (pField.ICompare(__sv("filesize")))
					m_Index_Filesize = nFieldNo;
				else if (pField.ICompare(__sv("fileattribs")))
					m_Index_Fileattribs = nFieldNo;
				else if (pField.ICompare(__sv("cfileattribs")))
					m_Index_StringFileattribs = nFieldNo;
				else if (pField.ICompare(__sv("fileid")))
					m_Index_FileId = nFieldNo;
				else if (pField.ICompare(__sv("parentid")))
					m_Index_ParentId = nFieldNo;

				pFields = pFields + (pFieldName.Len + 1);
				pFieldName = pFields.GetWordNum(1, ',');
				nFieldNo++;
			}
		}
		else
		{
			pFieldName = pFields.GetWordNum(1, ',');
			while (pFieldName)
			{
				pField = pFieldName.Alltrim();

				if (pCursorDefinition.Len() > 0)
					pCursorDefinition.Append(',');

				if (pField.ICompare(__sv("filename")))
				{
					m_Index_Filename = nFieldNo;
					pCursorDefinition.Append(__sv("filename M NOT NULL"));
				}
				else if (pField.ICompare(__sv("dosfilename")))
				{
					m_Index_Dosfilename = nFieldNo;
					pCursorDefinition.Append(__sv("dosfilename C(13) NOT NULL"));
				}
				else if (pField.ICompare(__sv("creationtime")))
				{
					m_Index_Creationtime = nFieldNo;
					pCursorDefinition.Append(__sv("creationtime T NOT NULL"));
				}
				else if (pField.ICompare(__sv("accesstime")))
				{
					m_Index_Accesstime = nFieldNo;
					pCursorDefinition.Append(__sv("accesstime T NOT NULL"));
				}
				else if (pField.ICompare(__sv("writetime")))
				{
					m_Index_Writetime = nFieldNo;
					pCursorDefinition.Append(__sv("writetime T NOT NULL"));
				}
				else if (pField.ICompare(__sv("filesize")))
				{
					m_Index_Filesize = nFieldNo;
					pCursorDefinition.Append(__sv("filesize N(20,0) NOT NULL"));
				}
				else if (pField.ICompare(__sv("fileattribs")))
				{
					m_Index_Fileattribs = nFieldNo;
					pCursorDefinition.Append(__sv("fileattribs I NOT NULL"));
				}
				else if (pField.ICompare(__sv("cfileattribs")))
				{
					m_Index_StringFileattribs = nFieldNo;
					if (CFoxVersion::MajorVersion() >= 9)
						pCursorDefinition.Append(__sv("cfileattribs V(10) NOT NULL"));
					else
						pCursorDefinition.Append(__sv("cfileattribs C(10) NOT NULL"));
				}
				else if (pField.ICompare(__sv("fileid")))
				{
					m_Index_FileId = nFieldNo;
					pCursorDefinition.Append(__sv("fileid I NOT NULL"));
				}
				else if (pField.ICompare(__sv("parentid")))
				{
					m_Index_ParentId = nFieldNo;
					pCursorDefinition.Append(__sv("parentid I NOT NULL"));
				}
				else
				{
					SaveCustomError("AdirEx", "Invalid name '%V' in field list", &pField);
					throw E_APIERROR;
				}

				pFields = pFields + (pFieldName.Len + 1);
				pFieldName = pFields.GetWordNum(1, ',');
				nFieldNo++;
			}

			m_Cursor.Create(pDestination, pCursorDefinition);
		}
	}
	else
	{
		if (nFlags & ADIREX_FILEID)
		{
			pCursorDefinition = __sv("fileid,");
			m_Index_FileId = nFieldNo++;
		}
			
		if (nFlags & ADIREX_PARENTID)
		{
			pCursorDefinition.Append(__sv("parentid,"));
			m_Index_ParentId = nFieldNo++;
		}

		pCursorDefinition.Append(__sv("filename,"));
		m_Index_Filename = nFieldNo++;

		if (!(nFlags & ADIREX_NODOSNAME))
		{
			pCursorDefinition.Append(__sv("dosfilename,"));
			m_Index_Dosfilename = nFieldNo++;
		}
			
		pCursorDefinition.Append(__sv("creationtime,accesstime,writetime,filesize,"));
		m_Index_Creationtime = nFieldNo++;
		m_Index_Accesstime = nFieldNo++;
		m_Index_Writetime = nFieldNo++;
		m_Index_Filesize = nFieldNo++;

		if (nFlags & ADIREX_STRING_FILEATTRIBUTES)
		{
			pCursorDefinition.Append(__sv("cfileattribs"));
			m_Index_StringFileattribs = nFieldNo++;
			m_Index_Fileattribs = -1;
		}
		else
		{
			pCursorDefinition.Append(__sv("fileattribs"));
			m_Index_Fileattribs = nFieldNo++;
			m_Index_StringFileattribs = -1;
			
		}

		nErrorNo = m_Cursor.Attach(pDestination, pCursorDefinition);
		if (nErrorNo != 0)
		{
			if (nFlags & ADIREX_FILEID)
				pCursorDefinition = __sv("fileid I NOT NULL,");
			if (nFlags & ADIREX_PARENTID)
				pCursorDefinition.Append(__sv("parentid I NOT NULL,"));
			pCursorDefinition.Append(__sv("filename M NOT NULL,"));
			if (!(nFlags & ADIREX_NODOSNAME))
				pCursorDefinition.Append(__sv("dosfilename C(13) NOT NULL,"));
			pCursorDefinition.Append(__sv("creationtime T NOT NULL,accesstime T NOT NULL,writetime T NOT NULL,filesize N(20, 0) NOT NULL,"));
			if (nFlags & ADIREX_STRING_FILEATTRIBUTES)
			{
				if (CFoxVersion::MajorVersion() >= 9)
					pCursorDefinition.Append(__sv("cfileattribs V(10) NOT NULL"));
				else
					pCursorDefinition.Append(__sv("cfileattribs C(10) NOT NULL"));
			}
			else
				pCursorDefinition.Append(__sv("fileattribs I NOT NULL"));

			m_Cursor.Create(pDestination, pCursorDefinition);
		}
	}

	nFlags |= m_Index_Dosfilename == -1 ? ADIREX_NODOSNAME : 0;
	nFlags |= m_Index_ParentId >= 0 ? ADIREX_PARENTID : 0;

	m_Cursor.AutoFLock();
}

template<class TData>
bool FileSearchStorageCursor<TData>::Store(void* pFileSearch, int nFileId)
{
	TData* pFS = (TData*)pFileSearch;
	m_Cursor.AppendBlank();

	if (m_Index_FileId >= 0)
		m_Cursor(m_Index_FileId) = nFileId;

	if (m_Index_ParentId >= 0)
		m_Cursor(m_Index_ParentId) = pFS->PathId();

	if (m_Index_Filename >= 0)
		m_Cursor(m_Index_Filename) = pFS->Filename();

	if (m_Index_Dosfilename >= 0)
		m_Cursor(m_Index_Dosfilename) = pFS->AlternateFilename();

	if (m_Index_Creationtime >= 0)
	{
		m_FileTime = pFS->CreationTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Cursor(m_Index_Creationtime) = m_FileTime;
	}

	if (m_Index_Accesstime >= 0)
	{
		m_FileTime = pFS->LastAccessTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Cursor(m_Index_Accesstime) = m_FileTime;
	}

	if (m_Index_Writetime >= 0)
	{
		m_FileTime = pFS->LastWriteTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Cursor(m_Index_Writetime) = m_FileTime;
	}

	if (m_Index_Filesize >= 0)
		m_Cursor(m_Index_Filesize) = m_FileSize = pFS->FileSize();
	
	if (m_Index_Fileattribs >= 0)
		m_Cursor(m_Index_Fileattribs) = pFS->FileAttributes();
	
	if (m_Index_StringFileattribs >= 0)
		m_Cursor(m_Index_StringFileattribs) = m_FileName.FileAttributesToString(pFS->FileAttributes());

	return false;
}

template<class TData>
bool FileSearchStorageCursor<TData>::StorePath(PathSearchEntry* pPath, void* pFileSearch, int nLevel)
{
	m_Cursor.AppendBlank();
	TData* pData = (TData*)pFileSearch;

	if (m_Index_FileId >= 0)
		m_Cursor(m_Index_FileId) = pPath->PathId();

	if (m_Index_ParentId >= 0)
	{
		if (m_Flags & ADIREX_PARENT_JUSTFNAME)
			m_Cursor(m_Index_ParentId) = pPath->ParentId();
		else
			m_Cursor(m_Index_ParentId) = 0;
	}

	if (m_Index_Filename >= 0)
	{
		if (m_Flags & ADIREX_PARENT_JUSTFNAME)
			m_Cursor(m_Index_Filename) = m_FileName = pPath->Filename();
		else
			m_Cursor(m_Index_Filename) = m_FileName = pData->GetPath((m_Flags & ADIREX_PARENT_FULLPATH) ? 0 : 1, nLevel);
	}

	if (m_Index_Dosfilename >= 0)
		m_Cursor(m_Index_Dosfilename) = m_FileName = pPath->AlternateFilename();

	if (m_Index_Creationtime >= 0)
	{
		m_FileTime = pPath->CreationTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Cursor(m_Index_Creationtime) = m_FileTime;
	}

	if (m_Index_Accesstime >= 0)
	{
		m_FileTime = pPath->LastAccessTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Cursor(m_Index_Accesstime) = m_FileTime;
	}

	if (m_Index_Writetime >= 0)
	{
		m_FileTime = pPath->LastWriteTime();
		if ((m_Flags & ADIREX_UTC_TIMES) == 0)
			m_FileTime.ToLocal();
		m_Cursor(m_Index_Writetime) = m_FileTime;
	}

	if (m_Index_Filesize >= 0)
		m_Cursor(m_Index_Filesize) = m_FileSize = pPath->FileSize();

	if (m_Index_Fileattribs >= 0)
		m_Cursor(m_Index_Fileattribs) = pPath->FileAttributes();

	if (m_Index_StringFileattribs >= 0)
		m_Cursor(m_Index_StringFileattribs) = m_FileName.FileAttributesToString(pPath->FileAttributes());

	return false;
}

template<class TData>
void FileSearchStorageCallback<TData>::Initialize(CStringView pDestination, DWORD& nFlags, CStringView pFields)
{
	FileSearchStorage<>::Initialize(pDestination, nFlags, pFields);
	CStringView pFieldName;
	CStringView pField;
	if (pDestination.Len > 1024 /* VFP2C_MAX_CALLBACKFUNCTION */ || pDestination.Len == 0)
	{
		SaveCustomErrorEx("AdirEx", "Callback function length is 0 or greater than maximum length of 1024.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}
	m_Callback.SetCallback(pDestination);
	if (pFields)
	{
		m_Index_FileId = m_Index_ParentId = m_Index_Filename = m_Index_Dosfilename = m_Index_Accesstime = m_Index_Creationtime =
			m_Index_Writetime = m_Index_Filesize = m_Index_Fileattribs = m_Index_StringFileattribs = -1;
		pFields = pFields.Alltrim();
		pFieldName = pFields.GetWordNum(1, ',');
		unsigned int nParmPos = 0;
		while (pFieldName)
		{
			pField = pFieldName.Alltrim();
			if (pField.ICompare(__sv("filename")))
			{
				m_Index_Filename = nParmPos;
			}
			else if (pField.ICompare(__sv("dosfilename")))
			{
				m_Index_Dosfilename = nParmPos;
			}
			else if (pField.ICompare(__sv("creationtime")))
			{
				m_Index_Creationtime = nParmPos;
			}
			else if (pField.ICompare(__sv("accesstime")))
			{
				m_Index_Accesstime = nParmPos;
			}
			else if (pField.ICompare(__sv("writetime")))
			{
				m_Index_Writetime = nParmPos;
			}
			else if (pField.ICompare(__sv("filesize")))
			{
				m_Index_Filesize = nParmPos;
			}
			else if (pField.ICompare(__sv("fileattribs")))
			{
				m_Index_Fileattribs = nParmPos;
			}
			else if (pField.ICompare(__sv("cfileattribs")))
			{
				m_Index_StringFileattribs = nParmPos;
			}
			else if (pField.ICompare(__sv("fileid")))
			{
				m_Index_FileId = nParmPos;
			}
			else if (pField.ICompare(__sv("parentid")))
			{
				m_Index_ParentId = nParmPos;
			}
			else
			{
				SaveCustomErrorEx("AdirEx", "Invalid parameter name '%V' in callback parameter list", E_VFP_INVALIDPARAMS, &pField);
				throw E_APIERROR;
			}

			pFields = pFields + (pFieldName.Len + 1);
			pFieldName = pFields.GetWordNum(1, ',');
			nParmPos++;
		}

		nFlags |= m_Index_Dosfilename == -1 ? ADIREX_NODOSNAME : 0;
		nFlags |= m_Index_ParentId >= 0 ? ADIREX_PARENTID : 0;
	}
	else
	{
		int nIndex = 1;
		if (nFlags & ADIREX_FILEID)
			m_Index_FileId = nIndex++;
		else
			m_Index_FileId = -1;

		if (nFlags & ADIREX_PARENTID)
			m_Index_ParentId = nIndex++;
		else
			m_Index_ParentId = -1;

		m_Index_Filename = nIndex++;

		if (nFlags & ADIREX_NODOSNAME)
			m_Index_Dosfilename = -1;
		else
			m_Index_Dosfilename = nIndex++;

		m_Index_Creationtime = nIndex++;
		m_Index_Accesstime = nIndex++;
		m_Index_Writetime = nIndex++;
		m_Index_Filesize = nIndex++;
		m_Index_Fileattribs = (nFlags & ADIREX_STRING_FILEATTRIBUTES) ? -1 : nIndex;
		m_Index_StringFileattribs = (nFlags & ADIREX_STRING_FILEATTRIBUTES) ? nIndex : -1;
	}

	m_Callback.SetParameterPosition(0, m_Index_FileId);
	m_Callback.SetParameterPosition(1, m_Index_ParentId);
	m_Callback.SetParameterPosition(2, m_Index_Filename);
	m_Callback.SetParameterPosition(3, m_Index_Dosfilename);
	m_Callback.SetParameterPosition(4, m_Index_Creationtime);
	m_Callback.SetParameterPosition(5, m_Index_Accesstime);
	m_Callback.SetParameterPosition(6, m_Index_Writetime);
	m_Callback.SetParameterPosition(7, m_Index_Filesize);
	m_Callback.SetParameterPosition(8, m_Index_Fileattribs);
	m_Callback.SetParameterPosition(9, m_Index_StringFileattribs);
	m_Callback.OptimizeParameterPosition();
}

template<class TData>
bool FileSearchStorageCallback<TData>::Store(void* pFileSearch, int nFileId)
{
	TData* pFS = (TData*)pFileSearch;
	ValueEx vRetVal;
	CStringView pStringFileAttributes;
	CStringView pFileName = pFS->Filename();
	CStringView pAlternateFileName = pFS->AlternateFilename();
	m_CreationTime = pFS->CreationTime();
	m_AccessTime = pFS->LastAccessTime();
	m_WriteTime = pFS->LastWriteTime();
	if ((m_Flags & ADIREX_UTC_TIMES) == 0)
	{
		m_CreationTime.ToLocal();
		m_AccessTime.ToLocal();
		m_WriteTime.ToLocal();
	}
	m_FileSize = pFS->FileSize();
	DWORD nFileAttributes = pFS->FileAttributes();
	if (m_Index_StringFileattribs >= 0)
	{
		m_FileNameAttributes.FileAttributesToString(nFileAttributes);
		pStringFileAttributes = m_FileNameAttributes;
	}
	int nErrorNo = m_Callback.Evaluate(vRetVal, nFileId, pFS->PathId(), pFileName, pAlternateFileName, &m_CreationTime, &m_AccessTime, &m_WriteTime, &m_FileSize, nFileAttributes, pStringFileAttributes);
	if (nErrorNo)
		throw nErrorNo;
	if (vRetVal.Vartype() == 'L')
		return vRetVal.ev_length == 0;
	else if (vRetVal.Numeric())
		return vRetVal.NumericCast<int>() == 0;
	else
		vRetVal.Release();

	return false;
}

template<class TData>
bool FileSearchStorageCallback<TData>::StorePath(PathSearchEntry* pPath, void* pFilesearch, int nLevel)
{
	TData* pData = (TData*)pFilesearch;
	ValueEx vRetVal;
	CStringView pStringFileAttributes;
	CStringView pFileName;
	int nParentId;

	if (m_Flags & ADIREX_PARENT_JUSTFNAME)
	{
		pFileName = m_FileName = pPath->Filename();
		nParentId = pPath->ParentId();
	}
	else
	{
		pFileName = m_FileName = pData->GetPath((m_Flags & ADIREX_PARENT_FULLPATH) ? 0 : 1, nLevel);
		nParentId = 0;
	}

	CStringView pAlternateFileName = m_AlternateName = pPath->AlternateFilename();
	m_CreationTime = pPath->CreationTime();
	m_AccessTime = pPath->LastAccessTime();
	m_WriteTime = pPath->LastWriteTime();
	if ((m_Flags & ADIREX_UTC_TIMES) == 0)
	{
		m_CreationTime.ToLocal();
		m_AccessTime.ToLocal();
		m_WriteTime.ToLocal();
	}
	m_FileSize = pPath->FileSize();
	DWORD nFileAttributes = pPath->FileAttributes();
	if (m_Index_StringFileattribs >= 0)
	{
		m_FileNameAttributes.FileAttributesToString(pPath->FileAttributes());
		pStringFileAttributes = m_FileNameAttributes;
	}
	int nErrorNo = m_Callback.Evaluate(vRetVal, pPath->PathId(), nParentId, pFileName, pAlternateFileName, &m_CreationTime, &m_AccessTime, &m_WriteTime, &m_FileSize, nFileAttributes, pStringFileAttributes);
	if (nErrorNo)
		throw nErrorNo;
	if (vRetVal.Vartype() == 'L')
		return vRetVal.ev_length == 0;
	else if (vRetVal.Numeric())
		return vRetVal.NumericCast<int>() == 0;
	else
		vRetVal.Release();

	return false;
}

bool FileSearchBase<>::Filter_True(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch)
{
	unused(nAttributes);
	unused(nFilter);
	unused(nFilterMatch);
	return true;
}

bool FileSearchBase<>::Filter_Match(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch)
{
	return (nAttributes & nFilter) == nFilterMatch;
}

bool FileSearchBase<>::Filter_Exact(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch)
{
	unused(nFilterMatch);
	return nAttributes == nFilter;
}

bool FileSearchBase<>::Filter_Any(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch)
{
	unused(nFilterMatch);
	return (nAttributes & nFilter) > 0;
}

void FileSearchBase<>::DisableFsRedirection()
{
	PVOID oldValue;
	if (fpWow64DisableWow64FsRedirection == 0)
		return;
	fpWow64DisableWow64FsRedirection(&oldValue);
}

void FileSearchBase<>::RevertFsRedirection()
{
	PVOID oldValue;
	if (fpWow64RevertWow64FsRedirection == 0)
		return;
	fpWow64RevertWow64FsRedirection(&oldValue);
}

DWORD Win32FileSearch::FindFirstFlags = 0xFFFFFFFF;
FINDEX_INFO_LEVELS Win32FileSearch::FindFirstInfoLevel = (FINDEX_INFO_LEVELS)0xFFFFFFFF;

Win32FileSearch::Win32FileSearch(CStringView pSearchPath, DWORD nFileFilter = 0, DWORD nFilterMatch = 0, CStringView pDestination = 0,
	DWORD nFlags = 0, int nMaxRecursion = 0,  CStringView pFields = 0) : 
	FileSearch<Win32FileSearch, Win32FileSearchData>(pSearchPath, nFileFilter, nFilterMatch, pDestination, nFlags, nMaxRecursion, pFields),
	m_Handle(INVALID_HANDLE_VALUE)
{

}

Win32FileSearch::~Win32FileSearch()
{
	if (m_Handle != INVALID_HANDLE_VALUE)
		FindClose(m_Handle);
}

bool Win32FileSearch::FindFirstInternal()
{
	m_Path.PrependIfNotPresent(__wsv(LFILE_UNICODE_EXTENSION));
	m_FileSearchData.SetRootPath(m_Path.StringView());
	CWideStringView pSearchPattern = m_FileSearchData.SearchPattern();

try_again_FindFirstFileEx:
	m_Handle = FindFirstFileExW(pSearchPattern.Data, GetFindFirstInfoLevel(), m_FileSearchData.Ptr(), m_FilterPathOnly ? FindExSearchLimitToDirectories : FindExSearchNameMatch, NULL, GetFindFirstFlags());

	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_INVALID_PARAMETER && GetFindFirstFlags() == FIND_FIRST_EX_LARGE_FETCH)
		{
			SetFindFirstFlags(0);
			goto try_again_FindFirstFileEx;
		}
		else if (IgnorableSearchError(nLastError, false))
			return false;
		else
		{
			SaveWin32Error("FindFirstFile", nLastError);
			throw E_APIERROR;
		}
	}

	if (m_FilterFakePath && m_FileSearchData.IsFakePath())
		return FindNext();

	m_FileId++;
	return true;
}

bool Win32FileSearch::FindFirstRecurse()
{
	m_RecursionLevel = 0;
	m_Path.PrependIfNotPresent(__wsv(LFILE_UNICODE_EXTENSION));
	m_FileSearchData.SetRootPath(m_Path.StringView());
	CWideStringView pSearchPattern = m_FileSearchData.SearchPattern(__wsv(L"*"));

try_again_FindFirstFileEx:
	m_Handle = FindFirstFileExW(pSearchPattern.Data, GetFindFirstInfoLevel(), m_FileSearchData.Ptr(), m_FilterPathOnly ? FindExSearchLimitToDirectories : FindExSearchNameMatch, NULL, GetFindFirstFlags());

	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_INVALID_PARAMETER && GetFindFirstFlags() == FIND_FIRST_EX_LARGE_FETCH)
		{
			SetFindFirstFlags(0);
			goto try_again_FindFirstFileEx;
		}
		else if (IgnorableSearchError(nLastError, false))
			return false;

		SaveWin32Error("FindFirstFile", nLastError);
		throw E_APIERROR;
	}

	if (m_FilterFakePath && m_FileSearchData.IsFakePath())
		return FindNextRecurse();

	m_FileId++;

	// add subdirectory to queue
	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && m_FileSearchData.IsRealPath())
		QueuePath();

	return true;
}

bool Win32FileSearch::FindNextInternal()
{
	BOOL	bNext;
	while (true)
	{
		bNext = FindNextFileW(m_Handle, m_FileSearchData.Ptr());
		if (bNext == FALSE)
		{
			DWORD nLastError = GetLastError();
			if (nLastError == ERROR_NO_MORE_FILES)
			{
				return false;
			}
			else
			{
				SaveWin32Error("FindNextFile", nLastError);
				throw E_APIERROR;
			}
		}
		if (m_FilterFakePath && m_FileSearchData.IsFakePath())
			continue;

		m_FileId++;
		return true;
	}
}

bool Win32FileSearch::FindNextRecurse()
{
	BOOL	bNext;

find_next:
	bNext = FindNextFileW(m_Handle, m_FileSearchData.Ptr());

	if (bNext == FALSE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_NO_MORE_FILES)
		{
			if (!FindClose(m_Handle))
			{
				SaveWin32Error("FindClose", GetLastError());
				throw E_APIERROR;
			}
			m_Handle = INVALID_HANDLE_VALUE;

			if (DequeuePath())
				goto find_first;

			return false;
		}
		else
		{
			SaveWin32Error("FindNextFile", nLastError);
			throw E_APIERROR;
		}
	}

	if (m_FilterFakePath && m_FileSearchData.IsFakePath())
		goto find_next;

	m_FileId++;
	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && m_FileSearchData.IsRealPath())
		QueuePath();

	return true;

find_first:
	CWideStringView pSearchPattern = m_FileSearchData.SearchPattern(__wsv(L"*"));

try_again_FindFirstFileEx:
	m_Handle = FindFirstFileExW(pSearchPattern.Data, GetFindFirstInfoLevel(), m_FileSearchData.Ptr(), m_FilterPathOnly ? FindExSearchLimitToDirectories : FindExSearchNameMatch, NULL, GetFindFirstFlags());
	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_INVALID_PARAMETER && GetFindFirstFlags() == FIND_FIRST_EX_LARGE_FETCH)
		{
			SetFindFirstFlags(0);
			goto try_again_FindFirstFileEx;
		}

		if (IgnorableSearchError(nLastError, m_RecursionLevel > 0))
		{
			// Did we access something we can't access while recursing?
			if (DequeuePath())
				goto find_first;

			return false;
		}
		else
		{
			SaveWin32Error("FindFirstFile", nLastError);
			throw E_APIERROR;
		}
	}

	if (m_FilterFakePath && m_FileSearchData.IsFakePath())
		goto find_next;

	m_FileId++;
	// add subdirectory to queue
	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && m_FileSearchData.IsRealPath())
		QueuePath();

	return true;
}

/* like FindFirst, but drills through all subdirectories and returns the files in the deepest directories first */
bool Win32FileSearch::FindFirstReverse()
{
	m_RecursionLevel = 0;
	m_Path.PrependIfNotPresent(__wsv(LFILE_UNICODE_EXTENSION));
	m_FileSearchData.SetRootPath(m_Path.StringView());
	CWideStringView pSearchPattern = m_FileSearchData.SearchPattern(__wsv(L"*"));

find_first:
	m_Handle = FindFirstFileExW(pSearchPattern.Data, GetFindFirstInfoLevel(), m_FileSearchData.Ptr(), m_FilterPathOnly ? FindExSearchLimitToDirectories : FindExSearchNameMatch, NULL, GetFindFirstFlags());

	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_INVALID_PARAMETER && GetFindFirstFlags() == FIND_FIRST_EX_LARGE_FETCH)
		{
			SetFindFirstFlags(0);
			goto find_first;
		}
		else if (IgnorableSearchError(nLastError, false))
			return false;
		
		SaveWin32Error("FindFirstFile", nLastError);
		throw E_APIERROR;
	}

	if (m_FileSearchData.IsFakePath())
		return FindNextReverse();

	m_FileId++;
	// add subdirectory to queque
	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && m_FileSearchData.IsRealPath())
		QueuePath();

	return true;
}

bool Win32FileSearch::FindNextReverse()
{
	BOOL	bNext;
find_next:
	bNext = FindNextFileW(m_Handle, m_FileSearchData.Ptr());
	if (bNext == FALSE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_NO_MORE_FILES)
		{
			if (!FindClose(m_Handle))
			{
				SaveWin32Error("FindClose", GetLastError());
				throw E_APIERROR;
			}

			m_Handle = INVALID_HANDLE_VALUE;
			if (DequeuePath())
				goto find_first;

			return false;
		}
		else
		{
			SaveWin32Error("FindNextFile", nLastError);
			throw E_APIERROR;
		}
	}

	if (m_FileSearchData.IsFakePath())
		goto find_next;

	m_FileId++;
	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && m_FileSearchData.IsRealPath())
		QueuePath();
	
	return true;

find_first:
	CWideStringView pSearchPattern = m_FileSearchData.SearchPattern(__wsv(L"*"));
	m_Handle = FindFirstFileExW(pSearchPattern.Data, GetFindFirstInfoLevel(), m_FileSearchData.Ptr(), m_FilterPathOnly ? FindExSearchLimitToDirectories : FindExSearchNameMatch, NULL, GetFindFirstFlags());

	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_INVALID_PARAMETER && GetFindFirstFlags() == FIND_FIRST_EX_LARGE_FETCH)
		{
			SetFindFirstFlags(0);
			goto find_first;
		}
		else if (IgnorableSearchError(nLastError, false))
			return false;

		SaveWin32Error("FindFirstFile", nLastError);
		throw E_APIERROR;
	}

	if (m_FileSearchData.IsFakePath())
		goto find_next;

	m_FileId++;
	// add subdirectory to queque
	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && m_FileSearchData.IsRealPath())
		QueuePath();

	return true;
}

bool Win32FileSearch::GetPathIdentifier(PathIdentifier& pid)
{
	DWORD nLastError;
	CWideStringView dirname = m_FileSearchData.JustFName();
	ApiHandle dirHandle;
	m_FileSearchData.AddPath(dirname);
	CWideStringView pPath = m_FileSearchData.Path();
	dirHandle = CreateFileW(pPath.Data, FILE_LIST_DIRECTORY, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	m_FileSearchData.RemovePath();

	if (!dirHandle)
	{
		nLastError = GetLastError();
		if (nLastError == ERROR_ACCESS_DENIED)
			return false;

		SaveWin32Error("CreateFile", nLastError);
		throw E_APIERROR;
	}

	return m_FileSearchData.GetPathIdentifier(pid, dirHandle);
}

FINDEX_INFO_LEVELS Win32FileSearch::GetFindFirstInfoLevel()
{
	if (FindFirstInfoLevel == (FINDEX_INFO_LEVELS)0xFFFFFFFF)
	{
		WindowsVersion winver = COs::GetVersion();
		if (winver == Windows7 || winver == WindowsServer2008R2 || winver == WindowsX)
			FindFirstInfoLevel = FindExInfoStandard;
		else
			FindFirstInfoLevel = FindExInfoBasic;
	}
	return (m_Flags & ADIREX_NODOSNAME) ? FindFirstInfoLevel : FindExInfoStandard;
}

DWORD Win32FileSearch::GetFindFirstFlags()
{
	if (FindFirstFlags == 0xFFFFFFFF)
	{
		WindowsVersion winver = COs::GetVersion();
		if (winver == Windows7 || winver == WindowsServer2008R2 || winver == WindowsX)
			FindFirstFlags = FIND_FIRST_EX_LARGE_FETCH;
		else
			FindFirstFlags = 0;
	}
	return FindFirstFlags;
}

void Win32FileSearch::SetFindFirstFlags(DWORD dwFlags)
{
	FindFirstFlags = dwFlags;
}

NtFileSearch::NtFileSearch(CStringView pSearchPath, DWORD nFileFilter = 0, DWORD nFilterMatch = 0, CStringView pDestination = 0,
	DWORD nFlags = 0, int nMaxRecursion = 0, CStringView pFields = 0) :
	FileSearch<NtFileSearch, NtFileSearchData>(pSearchPath, nFileFilter, nFilterMatch, pDestination, nFlags, nMaxRecursion, pFields)
{
	m_PathHandles[0] = 0;
	m_FileInfoClass = (m_Flags & ADIREX_NODOSNAME) ? FileDirectoryInformationEx : FileBothDirectoryInformationEx;
	m_FileSearchData.SetFileInfoClass(m_FileInfoClass);
}

NtFileSearch::~NtFileSearch()
{
	for (unsigned int xj = 0; xj <= m_RecursionLevel; xj++)
	{
		if (m_PathHandles[xj])
			fpNtClose(m_PathHandles[xj]);
	}
}

bool NtFileSearch::FindFirstInternal()
{
	m_Path.PrependIfNotPresent(__wsv(LFILE_UNICODE_EXTENSION));
	m_FileSearchData.SetRootPath(m_Path);
	m_FileSearchData.DosPathToNtPath();

	CWideStringView pRoot = m_FileSearchData.Path();
	OBJECT_ATTRIBUTES objattribs;
	UNICODE_STRING uPath;
	uPath.Buffer = pRoot.Data;
	uPath.MaximumLength = uPath.Length = pRoot.Len * 2;

	InitializeObjectAttributes(&objattribs, &uPath, OBJ_OPENIF, 0, 0);
	m_IoStatusBlock.Status = 0;
	m_IoStatusBlock.Information = 0;

	m_IoResult = fpNtOpenFile(&m_PathHandles[0], FILE_LIST_DIRECTORY | SYNCHRONIZE, &objattribs, &m_IoStatusBlock, FILE_SHARE_READ, FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT);
	if (m_IoResult != STATUS_SUCCESS)
	{
		if (m_IoResult == STATUS_NO_SUCH_FILE)
			return false;

		SaveWin32Error("NtOpenFile", fpRtlNtStatusToDosError(m_IoResult));
		throw E_APIERROR;
	}

	m_FileSearchData.SetFileInfo(m_FileInfoBuffer.Ptr());

	UNICODE_STRING pWildCard;
	PUNICODE_STRING ppWildCard = 0;
	if (m_FileSearchData.GetWildCardCount() == 1)
	{
		fpRtlInitUnicodeString(&pWildCard, m_FileSearchData.GetWildCard().Data);
		ppWildCard = &pWildCard;
	}

	m_IoStatusBlock.Status = 0;
	m_IoStatusBlock.Information = 0;

	if (fpNtQueryDirectoryFileEx)
		m_IoResult = fpNtQueryDirectoryFileEx(m_PathHandles[0], 0, 0, 0, &m_IoStatusBlock, m_FileInfoBuffer.Ptr(), m_FileInfoBuffer.Size(), m_FileInfoClass, SL_RESTART_SCAN, ppWildCard);
	else
		m_IoResult = fpNtQueryDirectoryFile(m_PathHandles[0], 0, 0, 0, &m_IoStatusBlock, m_FileInfoBuffer.Ptr(), m_FileInfoBuffer.Size(), m_FileInfoClass, FALSE, ppWildCard, TRUE);

	if (m_IoResult == STATUS_NO_MORE_FILES || m_IoResult == STATUS_NO_SUCH_FILE) {
		// we're done!
		return false;
	}
	else if (m_IoResult != STATUS_SUCCESS)
	{
		SaveWin32Error("NtQueryDirectoryFile", fpRtlNtStatusToDosError(m_IoResult));
		throw E_APIERROR;
	}

	if (!SkipFakeDirectories())
		return false;

	m_FileId++;
	return true;
}

bool NtFileSearch::FindNextInternal()
{
	if (!m_FileSearchData.NextEntry())
	{
		m_IoStatusBlock.Status = 0;
		m_IoStatusBlock.Information = 0;
		if (fpNtQueryDirectoryFileEx)
			m_IoResult = fpNtQueryDirectoryFileEx(m_PathHandles[0], 0, 0, 0, &m_IoStatusBlock, m_FileInfoBuffer.Ptr(), m_FileInfoBuffer.Size(), m_FileInfoClass, 0, 0);
		else
			m_IoResult = fpNtQueryDirectoryFile(m_PathHandles[0], 0, 0, 0, &m_IoStatusBlock, m_FileInfoBuffer.Ptr(), m_FileInfoBuffer.Size(), m_FileInfoClass, FALSE, 0, FALSE);

		if (m_IoResult == STATUS_NO_MORE_FILES)
		{
			// we're done!
			return false;
		}
		else if (m_IoResult != STATUS_SUCCESS)
		{
			SaveWin32Error("NtQueryDirectoryFile", fpRtlNtStatusToDosError(m_IoResult));
			throw E_APIERROR;
		}
		m_FileSearchData.SetFileInfo(m_FileInfoBuffer.Ptr());
	}
	m_FileId++;
	return true;
}

bool NtFileSearch::FindFirstRecurse()
{
	m_Path.PrependIfNotPresent(__wsv(LFILE_UNICODE_EXTENSION));
	m_FileSearchData.SetRootPath(m_Path);
	m_FileSearchData.DosPathToNtPath();
	m_RecursionLevel = 0;

	CWideStringView pRoot = m_FileSearchData.Path();
	OBJECT_ATTRIBUTES objAttributes;
	UNICODE_STRING uniPath;
	uniPath.Buffer = pRoot.Data;
	uniPath.MaximumLength = uniPath.Length = pRoot.Len * 2;

	InitializeObjectAttributes(&objAttributes, &uniPath, OBJ_OPENIF, 0, 0);
	m_IoStatusBlock.Status = 0;
	m_IoStatusBlock.Information = 0;
	m_IoResult = fpNtOpenFile(&m_PathHandles[0], FILE_LIST_DIRECTORY | SYNCHRONIZE, &objAttributes, &m_IoStatusBlock, FILE_SHARE_READ, FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT);
	if (m_IoResult != STATUS_SUCCESS)
	{
		if (m_IoResult == STATUS_NO_SUCH_FILE)
			return false;

		SaveWin32Error("NtOpenFile", fpRtlNtStatusToDosError(m_IoResult));
		throw E_APIERROR;
	}

	m_FileSearchData.SetFileInfo(m_FileInfoBuffer.Ptr());

	m_IoStatusBlock.Status = 0;
	m_IoStatusBlock.Information = 0;
	if (fpNtQueryDirectoryFileEx)
		m_IoResult = fpNtQueryDirectoryFileEx(m_PathHandles[0], 0, 0, 0, &m_IoStatusBlock, m_FileInfoBuffer.Ptr(), m_FileInfoBuffer.Size(), m_FileInfoClass, SL_RESTART_SCAN, 0);
	else
		m_IoResult = fpNtQueryDirectoryFile(m_PathHandles[0], 0, 0, 0, &m_IoStatusBlock, m_FileInfoBuffer.Ptr(), m_FileInfoBuffer.Size(), m_FileInfoClass, FALSE, 0, TRUE);

	if (m_IoResult == STATUS_NO_MORE_FILES) {
		// we're done!
		return false;
	}
	else if (m_IoResult != STATUS_SUCCESS)
	{
		SaveWin32Error("NtQueryDirectoryFile", fpRtlNtStatusToDosError(m_IoResult));
		throw E_APIERROR;
	}

	if (m_FilterFakePath && m_FileSearchData.IsFakePath())
		return FindNextRecurse();

	m_FileId++;
	// add subdirectory to queue
	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && m_FileSearchData.IsRealPath())
		QueuePath();

	return true;
}

bool NtFileSearch::FindNextRecurse()
{
	unsigned int nLastRecursionLevel;

find_next:
	if (!m_FileSearchData.NextEntry())
	{
		m_IoStatusBlock.Status = 0;
		m_IoStatusBlock.Information = 0;
		if (fpNtQueryDirectoryFileEx)
			m_IoResult = fpNtQueryDirectoryFileEx(m_PathHandles[m_RecursionLevel], 0, 0, 0, &m_IoStatusBlock, m_FileInfoBuffer.Ptr(), m_FileInfoBuffer.Size(), m_FileInfoClass, 0, 0);
		else
			m_IoResult = fpNtQueryDirectoryFile(m_PathHandles[m_RecursionLevel], 0, 0, 0, &m_IoStatusBlock, m_FileInfoBuffer.Ptr(), m_FileInfoBuffer.Size(), m_FileInfoClass, FALSE, 0, FALSE);

		if (m_IoResult == STATUS_NO_MORE_FILES)
		{
			nLastRecursionLevel = m_RecursionLevel;
			m_PathHandles[m_RecursionLevel + 1] = 0;
			if (DequeuePath())
				goto find_first;

			// we're done!
			return false;
		}
		else if (m_IoResult != STATUS_SUCCESS)
		{
			SaveWin32Error("NtQueryDirectoryFile", fpRtlNtStatusToDosError(m_IoResult));
			throw E_APIERROR;
		}

		m_FileSearchData.SetFileInfo(m_FileInfoBuffer.Ptr());
	}

	if (m_FilterFakePath && m_FileSearchData.IsFakePath())
		goto find_next;

	m_FileId++;
	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && m_FileSearchData.IsRealPath())
		QueuePath();

	return true;

find_first:
	CWideStringView pPath = m_FileSearchData.Path((int)m_RecursionLevel);
	OBJECT_ATTRIBUTES objAttributes;
	UNICODE_STRING uniPath;
	uniPath.Buffer = pPath.Data;
	uniPath.MaximumLength = uniPath.Length = pPath.Len * 2;

	while(nLastRecursionLevel >= m_RecursionLevel)
	{
		HANDLE handle = m_PathHandles[nLastRecursionLevel];
		if (handle)
		{
			m_PathHandles[nLastRecursionLevel] = 0;
			m_IoResult = fpNtClose(handle);
			if (m_IoResult != STATUS_SUCCESS)
			{
				SaveWin32Error("NtClose", fpRtlNtStatusToDosError(m_IoResult));
				throw E_APIERROR;
			}
		}
		nLastRecursionLevel--;
	}

	InitializeObjectAttributes(&objAttributes, &uniPath, OBJ_OPENIF, m_PathHandles[m_RecursionLevel-1], 0);
	m_IoStatusBlock.Status = 0;
	m_IoStatusBlock.Information = 0;
	m_IoResult = fpNtOpenFile(&m_PathHandles[m_RecursionLevel], FILE_LIST_DIRECTORY | SYNCHRONIZE, &objAttributes, &m_IoStatusBlock, FILE_SHARE_READ, FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT);
	if (m_IoResult != STATUS_SUCCESS)
	{
		// Did we access something we can't access while recursing?
		if (m_IoResult == STATUS_NO_SUCH_FILE || m_IoResult == STATUS_ACCESS_DENIED || m_IoResult == STATUS_SHARING_VIOLATION)
		{
			if (DequeuePath())
				goto find_first;

			return false;
		}

		SaveWin32Error("NtOpenFile", fpRtlNtStatusToDosError(m_IoStatusBlock.Status));
		throw E_APIERROR;
	}

	m_FileSearchData.SetFileInfo(m_FileInfoBuffer.Ptr());

	m_IoStatusBlock.Status = 0;
	m_IoStatusBlock.Information = 0;
	if (fpNtQueryDirectoryFileEx)
		m_IoResult = fpNtQueryDirectoryFileEx(m_PathHandles[m_RecursionLevel], 0, 0, 0, &m_IoStatusBlock, m_FileInfoBuffer.Ptr(), m_FileInfoBuffer.Size(), m_FileInfoClass, SL_RESTART_SCAN, 0);
	else
		m_IoResult = fpNtQueryDirectoryFile(m_PathHandles[m_RecursionLevel], 0, 0, 0, &m_IoStatusBlock, m_FileInfoBuffer.Ptr(), m_FileInfoBuffer.Size(), m_FileInfoClass, FALSE, 0, TRUE);

	if (m_IoResult != STATUS_SUCCESS)
	{
		SaveWin32Error("NtQueryDirectoryFile", fpRtlNtStatusToDosError(m_IoStatusBlock.Status));
		throw E_APIERROR;
	}

	if (m_FilterFakePath && m_FileSearchData.IsFakePath())
		goto find_next;

	m_FileId++;
	// add subdirectory to queue
	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && m_FileSearchData.IsRealPath())
		QueuePath();

	return true;
}

bool NtFileSearch::FindFirstReverse()
{
	return false;
}

bool NtFileSearch::FindNextReverse()
{
	return false;
}

bool NtFileSearch::GetPathIdentifier(PathIdentifier& pid)
{
	DWORD nLastError;
	CWideStringView dirname = m_FileSearchData.JustFName();
	ApiHandle dirHandle;
	m_FileSearchData.AddPath(dirname);
	m_FileSearchData.DosToNtPrefix();
	
	CWideStringView pPath = m_FileSearchData.Path();
	dirHandle = CreateFileW(pPath.Data, FILE_LIST_DIRECTORY, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	
	m_FileSearchData.NtToDosPrefix();
	m_FileSearchData.RemovePath();

	if (!dirHandle)
	{
		nLastError = GetLastError();
		if (nLastError == ERROR_ACCESS_DENIED)
			return false;

		SaveWin32Error("CreateFile", nLastError);
		throw E_APIERROR;
	}

	return m_FileSearchData.GetPathIdentifier(pid, dirHandle);
}

bool NtFileSearch::SkipFakeDirectories()
{
	if (m_FilterFakePath)
	{
		while (m_FileSearchData.IsFakePath())
		{
			if (!m_FileSearchData.NextEntry())
				return false;
		}
	}
	return true;
}

VolumeSearch::~VolumeSearch()
{ 
	if (m_Handle != INVALID_HANDLE_VALUE)
		FindVolumeClose(m_Handle);
}

bool VolumeSearch::FindFirst()
{
	m_Handle = FindFirstVolume(m_Volume.Ptr<LPSTR>(), m_Volume.Size());
	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_FILE_NOT_FOUND)
			return false;
		else
		{
			SaveWin32Error("FindFirstVolume", nLastError);
			throw E_APIERROR;
		}
	}
	m_Volume.StringLen();
	return true;
}

bool VolumeSearch::FindNext()
{
	BOOL bNext = FindNextVolume(m_Handle, m_Volume.Ptr<LPSTR>(), m_Volume.Size());
	if (bNext == FALSE)
	{
		DWORD nLastError = GetLastError();
        if (nLastError == ERROR_NO_MORE_FILES)
		{
			FindVolumeClose(m_Handle);
			m_Handle = INVALID_HANDLE_VALUE;
			return false;
		}
		else
		{
			SaveWin32Error("FindNextVolume", nLastError);
			throw E_APIERROR;
		}
	}
	m_Volume.StringLen();
	return true;
}

VolumeMountPointSearch::~VolumeMountPointSearch()
{ 
	if (m_Handle != INVALID_HANDLE_VALUE)
		FindVolumeMountPointClose(m_Handle);
}

bool VolumeMountPointSearch::FindFirst(char *pVolume)
{
	m_Handle = FindFirstVolumeMountPoint(pVolume, m_MountPoint.Ptr<LPSTR>(), m_MountPoint.Size());
	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError != ERROR_NO_MORE_FILES && nLastError != ERROR_NOT_READY)
		{
			SaveWin32Error("FindFirstVolumeMountPoint", nLastError);
			throw E_APIERROR;
		}
		else
			return false;
	}
	m_MountPoint.StringLen();
	return true;
}

bool VolumeMountPointSearch::FindNext()
{
	BOOL bNext = FindNextVolumeMountPoint(m_Handle, m_MountPoint.Ptr<LPSTR>(), m_MountPoint.Size());
	if (bNext == FALSE)
	{
		DWORD nLastError = GetLastError();
        if (nLastError == ERROR_NO_MORE_FILES)
		{
			FindVolumeMountPointClose(m_Handle);
			m_Handle = INVALID_HANDLE_VALUE;
			return false;
		}
		else
		{
			SaveWin32Error("FindNextVolumeMountPoint", nLastError);
			throw E_APIERROR;
		}
	}
	m_MountPoint.StringLen();
	return true;
}

DWORD _fastcall AdirEx_FileFilter(FoxString& pFileFilter, DWORD& nMatch)
{
	DWORD nFileFilter = 0;
	nMatch = 0;
	bool bNegate = false;
	for (unsigned long xj = 0; xj < pFileFilter.Len(); xj++)
	{
		char filterchar = pFileFilter[xj];
		if (filterchar == '-')
		{
			bNegate = true;
			continue;
		}
		switch (filterchar)
		{
		case 'R':
		case 'r':
			nFileFilter |= FILE_ATTRIBUTE_READONLY;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_READONLY;
			break;
		case 'H':
		case 'h':
			nFileFilter |= FILE_ATTRIBUTE_HIDDEN;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_HIDDEN;
			break;
		case 'S':
		case 's':
			nFileFilter |= FILE_ATTRIBUTE_SYSTEM;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_SYSTEM;
			break;
		case 'D':
		case 'd':
			nFileFilter |= FILE_ATTRIBUTE_DIRECTORY;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_DIRECTORY;
			break;
		case 'A':
		case 'a':
			nFileFilter |= FILE_ATTRIBUTE_ARCHIVE;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_ARCHIVE;
			break;
		case 'T':
		case 't':
			nFileFilter |= FILE_ATTRIBUTE_TEMPORARY;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_TEMPORARY;
			break;
		case 'P':
		case 'p':
			nFileFilter |= FILE_ATTRIBUTE_SPARSE_FILE;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_SPARSE_FILE;
			break;
		case 'L':
		case 'l':
			nFileFilter |= FILE_ATTRIBUTE_REPARSE_POINT;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_REPARSE_POINT;
			break;
		case 'C':
		case 'c':
			nFileFilter |= FILE_ATTRIBUTE_COMPRESSED;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_COMPRESSED;
			break;
		case 'O':
		case 'o':
			nFileFilter |= FILE_ATTRIBUTE_OFFLINE;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_OFFLINE;
			break;
		case 'I':
		case 'i':
			nFileFilter |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
			break;
		case 'E':
		case 'e':
			nFileFilter |= FILE_ATTRIBUTE_ENCRYPTED;
			if (bNegate == false)
				nMatch |= FILE_ATTRIBUTE_ENCRYPTED;
			break;
		case 'K':
		case 'k':
			nFileFilter |= FILE_ATTRIBUTE_FAKEDIRECTORY;
			break;
		default:
			SaveCustomErrorEx("AdirEx", "Invalid filter character '%s'", E_VFP_INVALIDPARAMS, filterchar);
			throw E_APIERROR;
		}
		bNegate = false;
	}
	return nFileFilter;
}

void _fastcall ADirEx(ParamBlkEx& parm)
{
	FileSearchBase<>* pFileSearch = 0;
try
{
	FoxString pDestination(parm(1));
	FoxString pSearchString(parm(2));
	FoxString pFields(parm, 6);
	DWORD nFileFilter = 0, nFileMatch = 0;
	int nFlags, nMaxRecursion;

	nFlags = parm.PCount() >= 4 && parm(4)->ev_long ? parm(4)->ev_long : ADIREX_DEST_ARRAY;
	nMaxRecursion = parm.PCount() >= 5 ? parm(5)->ev_long : 0;

	if (parm.PCount() >= 3)
	{
		if (parm(3)->Numeric())
		{
			nFileFilter = parm(3)->NumericCast<DWORD>();
			if (parm.PCount() >= 7)
			{
				nFileMatch = parm(7)->NumericCast<DWORD>();
				nFlags |= ADIREX_FILTER_MATCH;

				if (nFlags & (ADIREX_FILTER_ALL | ADIREX_FILTER_EXACT | ADIREX_FILTER_NONE))
				{
					SaveCustomErrorEx("AdirEx", "Flags ADIREX_FILTER_ALL, ADIREX_FILTER_EXACT or ADIREX_FILTER_NONE are not valid when the parameter 'nFilterMatch' is passed.", E_VFP_INVALIDPARAMS);
					throw E_APIERROR;
				}
			}
		}
		else if (parm(3)->Vartype() == 'C')
		{
			if (parm(3)->Len() > 0)
			{
				FoxString pFileFilter(parm(3), 0);
				nFileFilter = AdirEx_FileFilter(pFileFilter, nFileMatch);
				nFlags |= ADIREX_FILTER_MATCH;

				if (nFlags & (ADIREX_FILTER_ALL | ADIREX_FILTER_EXACT | ADIREX_FILTER_NONE))
				{
					SaveCustomErrorEx("AdirEx", "Flags ADIREX_FILTER_ALL, ADIREX_FILTER_EXACT or ADIREX_FILTER_NONE are not valid when the parameter 'ncFilterFilter' is a string.", E_VFP_INVALIDPARAMS);
					throw E_APIERROR;
				}
			}
		}
		else
			throw E_INVALIDPARAMS;
	}

	if (!(nFlags & (ADIREX_DEST_ARRAY | ADIREX_DEST_CURSOR | ADIREX_DEST_CALLBACK)))
		nFlags |= ADIREX_DEST_ARRAY;

	if (__popcnt(nFlags & (ADIREX_DEST_ARRAY | ADIREX_DEST_CURSOR | ADIREX_DEST_CALLBACK)) > 1)
	{
		SaveCustomErrorEx("AdirEx", "Only one of the flags ADIREX_DEST_ARRAY, ADIREX_DEST_CURSOR or ADIREX_DEST_CALLBACK can be specified.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if (__popcnt(nFlags & (ADIREX_FILTER_ALL | ADIREX_FILTER_EXACT | ADIREX_FILTER_NONE)) > 1)
	{
		SaveCustomErrorEx("AdirEx", "Only one of the flags ADIREX_FILTER_ALL, ADIREX_FILTER_EXACT or ADIREX_FILTER_NONE can be specified.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if (__popcnt(nFlags & (ADIREX_FULLPATH | ADIREX_JUSTFNAME)) > 1)
	{
		SaveCustomErrorEx("AdirEx", "Only one of the flags ADIREX_FULLPATH or ADIREX_JUSTFNAME can be specified.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if (__popcnt(nFlags & (ADIREX_PARENT_FULLPATH | ADIREX_PARENT_JUSTFNAME)) > 1)
	{
		SaveCustomErrorEx("AdirEx", "Only one of the flags ADIREX_PARENT_FULLPATH or ADIREX_PARENT_JUSTFNAME can be specified.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if ((nFlags & (ADIREX_FILEID | ADIREX_PARENTID)) == ADIREX_PARENTID)
	{
		SaveCustomErrorEx("AdirEx", "The flag ADIREX_PARENTID requires ADIREX_FILEID.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if (parm.PCount() >= 6 && parm(6)->Len() && (nFlags & (ADIREX_FILEID | ADIREX_PARENTID | ADIREX_NODOSNAME | ADIREX_STRING_FILEATTRIBUTES)))
	{
		SaveCustomErrorEx("AdirEx", "The flags ADIREX_FILEID, ADIREX_PARENTID, ADIREX_NODOSNAME and ADIREX_STRING_FILEATTRIBUTES are invalid when the parameter 'cFields' is passed.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if ((nFlags & ADIREX_DEST_CALLBACK) && (pDestination.Len() > VFP2C_MAX_CALLBACKFUNCTION || pDestination.Len() == 0))
	{
		SaveCustomErrorEx("ADirEx", "Callback function length is zero or greater than maximum length of 1024.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}
	
	if (fpNtQueryDirectoryFileEx || fpNtQueryDirectoryFile)
		pFileSearch = new(std::nothrow) NtFileSearch(pSearchString, nFileFilter, nFileMatch, pDestination, nFlags, nMaxRecursion, pFields);
	else
		pFileSearch = new(std::nothrow) Win32FileSearch(pSearchString, nFileFilter, nFileMatch, pDestination, nFlags, nMaxRecursion, pFields);

	unsigned int nFileCount = pFileSearch->ExecuteSearch();
	Return(nFileCount);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
	Return(0);
}
if (pFileSearch)
{
	delete pFileSearch;
}
return;
}

void _fastcall ADirectoryInfo(ParamBlkEx& parm)
{
	Win32FileSearch* pFileSearch = 0;
try
{
	FoxArray pArray(parm(1),3,1);
	FoxString pDirectory;
	FoxInt64 pFileSize;
	int nDirectoryCount = 0;
	int nFileCount = 0;
	double nDirectorySize = 0.0;

	if (parm(2)->Vartype() == 'C')
		pDirectory.Attach(parm(2), 1);
	else if (parm(2).IsMemoRef())
		FoxMemo::Read(parm(2), pDirectory, true);
	else
		throw E_INVALIDPARAMS;

	pDirectory.AddBsWildcard();

	pFileSearch = new(std::nothrow) Win32FileSearch(pDirectory, 0, 0, 0, ADIREX_RECURSIVE);
	if (pFileSearch == 0)
		throw E_INSUFMEMORY;

	if (pFileSearch->FindFirst())
	{
		do
		{
			if (pFileSearch->IsRealPath())
			{
				nDirectoryCount++;
			}
			else if (!pFileSearch->IsFakePath())
			{
				nFileCount++;
				nDirectorySize += pFileSearch->FileSize();
			}
		} while (pFileSearch->FindNext());
	}

	pArray(1) = nFileCount;
	pArray(2) = nDirectoryCount;
	pArray(3) = pFileSize = nDirectorySize;
}
catch (int nErrorNo)
{
	RaiseError(nErrorNo);
}
if (pFileSearch)
	delete pFileSearch;
}

void _fastcall AFileAttributes(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),5,1);
	FoxString pFileName;
	bool bToLocal = parm.PCount() == 2 || !parm(3)->ev_length;
	bool bStringFileAttributes = parm.PCount() == 4 && parm(4)->ev_length;
	FoxDateTime pFileTime;
	FoxInt64 pFileSize;
	WIN32_FILE_ATTRIBUTE_DATA sFileAttribs;
	BOOL bRet;

	if (parm(2)->Vartype() == 'C')
		pFileName.Attach(parm(2), 1);
	else if (parm(2).IsMemoRef())
		FoxMemo::Read(parm(2), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		bRet = GetFileAttributesEx(pFileName.Ptr<LPCSTR>(), GetFileExInfoStandard, &sFileAttribs);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		bRet = GetFileAttributesExW(pWideFilename, GetFileExInfoStandard, &sFileAttribs);
	}

	if (!bRet)
	{
		SaveWin32Error("GetFileAttributesEx", GetLastError());
		throw E_APIERROR;
	}

	if (bStringFileAttributes)
		pArray(1) = pFileName.FileAttributesToString(sFileAttribs.dwFileAttributes);
	else
		pArray(1) = sFileAttribs.dwFileAttributes;

	pArray(2) = pFileSize = Ints2Double(sFileAttribs.nFileSizeLow, sFileAttribs.nFileSizeHigh);
	
	pFileTime = sFileAttribs.ftCreationTime;
	if (bToLocal)
		pFileTime.ToLocal();
	pArray(3) = pFileTime;

	pFileTime = sFileAttribs.ftLastAccessTime;
	if (bToLocal)
		pFileTime.ToLocal();
	pArray(4) = pFileTime;

	pFileTime = sFileAttribs.ftLastWriteTime;
	if (bToLocal)
		pFileTime.ToLocal();
	pArray(5) = pFileTime;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AFileAttributesEx(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),9,1);
	FoxString pFileName;
	bool bToLocal = parm.PCount() == 2 || !parm(3)->ev_length;
	bool bStringFileAttributes = parm.PCount() == 4 && parm(4)->ev_length;
	FoxDateTime pFileTime;
	FoxInt64 pFileSize;
	ApiHandle hFile;
	BY_HANDLE_FILE_INFORMATION sFileAttribs;

	if (parm(2)->Vartype() == 'C')
		pFileName.Attach(parm(2), 1);
	else if (parm(2).IsMemoRef())
		FoxMemo::Read(parm(2), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		hFile = CreateFile(pFileName.Ptr<LPCSTR>(), 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		hFile = CreateFileW(pWideFilename, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}

	if (!hFile)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	if (!GetFileInformationByHandle(hFile,&sFileAttribs))
	{
		SaveWin32Error("GetFileInformationByHandle", GetLastError());
		throw E_APIERROR;
	}

	if (bStringFileAttributes)
		pArray(1) = pFileName.FileAttributesToString(sFileAttribs.dwFileAttributes);
	else
		pArray(1) = sFileAttribs.dwFileAttributes;

	pArray(2) = pFileSize = Ints2Double(sFileAttribs.nFileSizeLow, sFileAttribs.nFileSizeHigh);

	pFileTime = sFileAttribs.ftCreationTime;
	if (bToLocal)
		pFileTime.ToLocal();
	pArray(3) = pFileTime;

	pFileTime = sFileAttribs.ftLastAccessTime;
	if (bToLocal)
		pFileTime.ToLocal();
	pArray(4) = pFileTime;

	pFileTime = sFileAttribs.ftLastWriteTime;
	if (bToLocal)
		pFileTime.ToLocal();
	pArray(5) = pFileTime;

	pArray(6) = sFileAttribs.nNumberOfLinks - 1;
	pArray(7) = sFileAttribs.dwVolumeSerialNumber;
	pArray(8) = sFileAttribs.nFileIndexLow;
	pArray(9) = sFileAttribs.nFileIndexHigh;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall GetFileTimes(ParamBlkEx& parm)
{
try
{
	FoxString pFileName;
	bool bToLocal = parm.PCount() < 5 || !parm(5)->ev_length;
	FoxDateTime pFileTime;
	ApiHandle hFile;
	FILETIME sCreationTime, sAccessTime, sWriteTime;

	if (parm(1)->Vartype() == 'C')
		pFileName.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (parm(2)->Vartype() != 'R' && parm(2)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	if (parm.PCount() >= 3 && parm(3)->Vartype() != 'R' && parm(3)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	if (parm.PCount() >= 4 && parm(4)->Vartype() != 'R' && parm(4)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		hFile = CreateFile(pFileName.Ptr<LPCSTR>(), 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		hFile = CreateFileW(pWideFilename, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}

	if (!hFile)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	if (!GetFileTime(hFile,&sCreationTime,&sAccessTime,&sWriteTime))
	{	
		SaveWin32Error("GetFileTime", GetLastError());
		throw E_APIERROR;
	}

	if (parm(2)->Vartype() == 'R')
	{
		pFileTime = sCreationTime;
		if (bToLocal)
			pFileTime.ToLocal();
		LocatorEx& pTime = parm(2);
		pTime = pFileTime;
	}

	if (parm.PCount() >= 3 && parm(3)->Vartype() == 'R')
	{
		pFileTime = sAccessTime;
		if (bToLocal)
			pFileTime.ToLocal();
		LocatorEx& pTime = parm(3);
		pTime = pFileTime;
	}

	if (parm.PCount() >= 4 && parm(4)->Vartype() == 'R')
	{
		pFileTime = sWriteTime;
		if (bToLocal)
			pFileTime.ToLocal();
		LocatorEx& pTime = parm(4);
		pTime = pFileTime;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);	
}
}

void _fastcall SetFileTimes(ParamBlkEx& parm)
{
try
{
	FoxString pFileName;
	ApiHandle hFile;
	FoxDateTime pTime;
	bool bCreation, bAccess, bWrite;
	bool bToUTC = parm.PCount() < 5 || parm(5)->ev_length;

	if (parm(1)->Vartype() == 'C')
		pFileName.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (parm(2)->Vartype() == 'T')
		bCreation = parm(2)->ev_real != 0.0;
	else if (parm(2)->Vartype() == '0')
		bCreation = false;
	else
		throw E_INVALIDPARAMS;

	if (parm.PCount() >= 3)
	{
		if (parm(3)->Vartype() == 'T')
			bAccess = parm(3)->ev_real != 0.0;
		else if (parm(3)->Vartype() == '0')
			bAccess = false;
		else
			throw E_INVALIDPARAMS;
	}
	else
		bAccess = false;

	if (parm.PCount() >= 4)
	{
		if (parm(4)->Vartype() == 'T')
			bWrite = parm(4)->ev_real != 0.0;
		else if (parm(4)->Vartype() == '0')
			bWrite = false;
		else
			throw E_INVALIDPARAMS;
	}
	else
		bWrite = false;

	if (!bCreation && !bAccess && !bWrite)
		throw E_INVALIDPARAMS;

	FILETIME sCreationTime, sAccessTime, sWriteTime;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		hFile = CreateFile(pFileName.Ptr<LPCSTR>(), FILE_WRITE_ATTRIBUTES, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		hFile = CreateFileW(pWideFilename, FILE_WRITE_ATTRIBUTES, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}

	if (!hFile)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	if (bCreation)
	{
		pTime = parm(2);
		if (bToUTC)
			pTime.ToUTC();
		sCreationTime = pTime;
	}

	if (bAccess)
	{
		pTime = parm(3);
		if (bToUTC)
			pTime.ToUTC();
		sAccessTime = pTime;
	}

    if (bWrite)
	{
		pTime = parm(4);
		if (bToUTC)
			pTime.ToUTC();
		sWriteTime = pTime;
	}

	if (!SetFileTime(hFile, bCreation ? &sCreationTime : 0, bAccess ? &sAccessTime : 0, bWrite ? &sWriteTime : 0))
	{
		SaveWin32Error("SetFileTime", GetLastError());
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall GetFileSizeExLib(ParamBlkEx& parm)
{
try
{
	FoxString pFileName;
	FoxInt64 pFileSize;
	ApiHandle hFile;
	LARGE_INTEGER sSize;

	if (parm(1)->Vartype() == 'C')
		pFileName.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		hFile = CreateFile(pFileName.Ptr<LPCSTR>(), 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		hFile = CreateFileW(pWideFilename, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}

	if (!hFile)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	if (!GetFileSizeEx(hFile, &sSize))
	{
		SaveWin32Error("GetFileSizeEx", GetLastError());
		throw E_APIERROR;
	}

	pFileSize = sSize.QuadPart;
	Return(pFileSize);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall GetFileAttributesEx2(ParamBlkEx& parm)
{
try
{
	FoxString pFileName;
	bool bStringFileAttributes = parm.PCount() == 2 && parm(2)->ev_length;
	DWORD nAttribs;
    
	if (parm(1)->Vartype() == 'C')
		pFileName.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		nAttribs = GetFileAttributes(pFileName.Ptr<LPCSTR>());
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		nAttribs = GetFileAttributesW(pWideFilename);
	}

	if (nAttribs == INVALID_FILE_ATTRIBUTES)
	{
		SaveWin32Error("GetFileAttributes", GetLastError());
		throw E_APIERROR;
	}

	if (bStringFileAttributes)
	{
		FoxString pString(10);
		pString.FileAttributesToString(nAttribs);
		pString.Return();
	}
	else
		Return(nAttribs);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _stdcall SetFileAttributesExCallback(CStrBuilder<MAX_WIDE_PATH>& pPath, DWORD nFileAttributes, LPVOID pParam)
{
	BOOL bRet;
	LPSETFILEATTRIBUTEPARAM pFileAttributes = (LPSETFILEATTRIBUTEPARAM)pParam;
	if (pFileAttributes->bClearOrSet)
	{
		nFileAttributes |= pFileAttributes->nFileAttributesSet;
		nFileAttributes &= ~pFileAttributes->nFileAttributesClear;
	}
	else
		nFileAttributes = pFileAttributes->nFileAttributesSet;

	if (pPath.Len() < MAX_PATH)
	{
		bRet = SetFileAttributes(pPath, nFileAttributes);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWidePath;
		pWidePath.ConvertFilePath(pPath, __sv(FILE_UNICODE_EXTENSION));
		bRet = SetFileAttributesW(pWidePath, nFileAttributes);
	}

	if (!bRet)
	{
		SaveWin32Error("SetFileAttributes", GetLastError());
		throw E_APIERROR;
	}
}

void _fastcall SetFileAttributesEx(ParamBlkEx& parm)
{
	Win32FileSearch* pFileSearch = 0;
try
{
	FoxString pFileName;
	FoxWString<MAX_PATH * 2> pWideFilename;
	DWORD dwFileAttributes = 0, dwFileAttributesSet = 0, dwFileAttributesClear = 0;
	int nMaxRecursion = parm.PCount() < 4 ? -2 : parm(4)->ev_long;
	bool bClearOrSet = false;
	BOOL bRet;

	if (parm(1)->Vartype() == 'C')
		pFileName.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (nMaxRecursion == -2)
	{
		if (parm.PCount() == 2)
		{
			if (parm(2)->Vartype() == 'I')
			{
				dwFileAttributes = parm(2)->ev_long;
				bClearOrSet = false;
			}
			else if (parm(2)->Vartype() == 'C')
			{
				FoxString pAttributes(parm(2), 0);
				bClearOrSet = pAttributes.StringToFileAttributes(dwFileAttributesSet, dwFileAttributesClear);
				if (!bClearOrSet)
					dwFileAttributes = dwFileAttributesSet;
			}
			else
				throw E_INVALIDPARAMS;
		}
		else if (parm.PCount() == 3)
		{
			if (parm(2)->Vartype() == 'I')
			{
				dwFileAttributesSet = parm(2)->ev_long;
				dwFileAttributesClear = parm(3)->ev_long;
				bClearOrSet = (dwFileAttributes == 0);
			}
			else
				throw E_INVALIDPARAMS;
		}

		if (pFileName.Len() < MAX_PATH)
			pFileName.Fullpath();

		if (pFileName.Len() >= MAX_PATH)
			pWideFilename.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));

		if (bClearOrSet)
		{
			if (pFileName.Len() < MAX_PATH)
				dwFileAttributes = GetFileAttributes(pFileName.Ptr<LPCSTR>());
			else
				dwFileAttributes = GetFileAttributesW(pWideFilename);
			if (dwFileAttributes == INVALID_FILE_ATTRIBUTES)
			{
				SaveWin32Error("GetFileAttributes", GetLastError());
				throw E_APIERROR;
			}

			dwFileAttributes |= dwFileAttributesSet;
			dwFileAttributes &= ~dwFileAttributesClear;
		}

		if (pFileName.Len() < MAX_PATH)
			bRet = SetFileAttributes(pFileName.Ptr<LPCSTR>(), dwFileAttributes);
		else
			bRet = SetFileAttributesW(pWideFilename, dwFileAttributes);

		if (!bRet)
		{
			SaveWin32Error("SetFileAttributes", GetLastError());
			throw E_APIERROR;
		}
	}
	else
	{
		SETFILEATTRIBUTEPARAM pCallbackParam;

		if (parm(2)->Vartype() == 'I')
		{
			pCallbackParam.nFileAttributesSet = parm(2)->ev_long;
			pCallbackParam.nFileAttributesClear = parm(3)->ev_long;
			pCallbackParam.bClearOrSet = (parm(3)->ev_long == 0);
		}
		else if (parm(2)->Vartype() == 'C')
		{
			FoxString pAttributes(parm(2), 0);
			pCallbackParam.bClearOrSet = pAttributes.StringToFileAttributes(pCallbackParam.nFileAttributesSet, pCallbackParam.nFileAttributesClear);
		}
		else
			throw E_INVALIDPARAMS;

		pFileSearch = new(std::nothrow) Win32FileSearch(pFileName, 0, 0, 0, ADIREX_FULLPATH | (nMaxRecursion >= 0 ? ADIREX_RECURSIVE : 0), nMaxRecursion);
		if (pFileSearch == 0)
			throw E_INSUFMEMORY;

		unsigned int nFileCount = pFileSearch->ExecuteSearchCallback(SetFileAttributesExCallback, (LPVOID)&pCallbackParam);
		Return(nFileCount);
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
if (pFileSearch)
	delete pFileSearch;
}

void _fastcall GetFileOwner(ParamBlkEx& parm)
{
try
{
	FoxString pFileName;
	FoxString pOwner(MAX_PATH);
	FoxString pDomain(MAX_PATH);
	CBuffer pDescBuffer;
	ApiHandle hFile;
	int SidType;
	
	DWORD dwSize = SECURITY_DESCRIPTOR_LEN, nLastError;
	BOOL bOwnerDefaulted;
	PSECURITY_DESCRIPTOR pSecDesc;
	PSID pOwnerId;

	if (parm(1)->Vartype() == 'C')
		pFileName.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		hFile = CreateFile(pFileName.Ptr<LPCSTR>(), READ_CONTROL, 0, 0, OPEN_EXISTING, 0, 0);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		hFile = CreateFileW(pWideFilename, READ_CONTROL, 0, 0, OPEN_EXISTING, 0, 0);
	}
	
	if (!hFile)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	while(true)
	{
		pDescBuffer.Size(dwSize);
		pSecDesc = pDescBuffer.Ptr<PSECURITY_DESCRIPTOR>();

		if (!GetKernelObjectSecurity(hFile, OWNER_SECURITY_INFORMATION, pSecDesc, dwSize, &dwSize))
		{
			nLastError = GetLastError();
			if (nLastError != ERROR_INSUFFICIENT_BUFFER)
			{
				SaveWin32Error("GetKernelObjectSecurity", nLastError);
				throw E_APIERROR;
			}
		}
		else
			break;
	}

	if (!GetSecurityDescriptorOwner(pSecDesc,&pOwnerId,&bOwnerDefaulted))
	{
		SaveWin32Error("GetSecurityDescriptorOwner", GetLastError());
		throw E_APIERROR;
	}

	DWORD dwOwnerLen = MAX_PATH, dwDomainLen = MAX_PATH;
	if (!LookupAccountSid((LPCSTR)0,pOwnerId,pOwner.Ptr<LPSTR>(),&dwOwnerLen,
		pDomain.Ptr<LPSTR>(),&dwDomainLen,(PSID_NAME_USE)&SidType))
	{
		SaveWin32Error("LookupAccountSid", GetLastError());
		throw E_APIERROR;
	}

	pOwner.Len(dwOwnerLen);
	pDomain.Len(dwDomainLen);

	LocatorEx& rOwner = parm(2);
    rOwner = pOwner;

	if (parm.PCount() >= 3)
	{
		LocatorEx& rDomain = parm(3);
		rDomain = pDomain;
	}

	if (parm.PCount() >= 4)
	{
		LocatorEx& rSid = parm(4);
		rSid = SidType;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

#pragma warning( disable: 4815)
void _fastcall GetLongPathNameEx(ParamBlkEx& parm)
{
try
{
	FoxString pFileName;
	FoxString pPathName(MAX_PATH);
	DWORD nLen = 0;

	if (parm(1)->Vartype() == 'C')
		pFileName.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		nLen = GetLongPathName(pFileName.Ptr<LPSTR>(), pPathName.Ptr<LPSTR>(), MAX_PATH);
		pPathName.Len(nLen);
	}

	if (nLen > MAX_PATH)
	{
		FoxWString<MAX_PATH*2> pWideFilename;
		FoxWString<0> pWidePathName;
		pWideFilename.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		pWidePathName.Size(pWideFilename.Len() + MAX_PATH);
		nLen = GetLongPathNameW(pWideFilename, pWidePathName, pWidePathName.Size());
		if (nLen != 0)
		{
			pWidePathName.Len(nLen);
			CWideStringView pPath(pWidePathName, pWidePathName.Len());
			pPath = pPath + 4; // "\\?\"
			pPathName = pPath;
		}
	}

	if (nLen == 0)
	{
		SaveWin32Error("GetLongPathName", GetLastError());
		throw E_APIERROR;
	}
	pPathName.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}
#pragma warning( default: 4815)

#pragma warning( disable: 4815)
void _fastcall GetShortPathNameEx(ParamBlkEx& parm)
{
try
{
	FoxString pFileName;
	FoxString pPathName(MAX_PATH);
	DWORD nLen;

	if (parm(1)->Vartype() == 'C')
		pFileName.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		nLen = GetShortPathName(pFileName.Ptr<LPCSTR>(), pPathName.Ptr<LPSTR>(), MAX_PATH);
		pPathName.Len(nLen);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		FoxWString<0> pWidePathName;
		pWideFilename.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		pWidePathName.Size(pWideFilename.Len());
		nLen = GetShortPathNameW(pWideFilename, pWidePathName, pWidePathName.Size());
		if (nLen != 0)
		{
			pWidePathName.Len(nLen);
			CWideStringView pPath(pWidePathName, pWidePathName.Len());
			pPath = pPath + 4; // "\\?\"
			pPathName = pPath;
		}
	}

	if (nLen == 0)
	{
		SaveWin32Error("GetShortPathName", GetLastError());
		throw E_APIERROR;
	}

	pPathName.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}
#pragma warning( default: 4815)

void _fastcall CopyFileExLib(ParamBlkEx& parm)
{
try
{
	FoxString pSource;
	FoxString pDest;
	FoxString pCallback(parm,3);
	DWORD dwCopyFlags = parm.PCount() >= 4 ? parm(4)->ev_long : COPY_FILE_NO_BUFFERING;
	DWORD dwCallbackLimiter = parm.PCount() == 5 ? parm(5)->ev_long : 100;
	BOOL bRetVal, bCanceled = 0;
	FILEPROGRESSPARAM sProgress;

	if (parm(1)->Vartype() == 'C')
		pSource.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pSource, true);
	else
		throw E_INVALIDPARAMS;

	if (parm(2)->Vartype() == 'C')
		pDest.Attach(parm(2), 1);
	else if (parm(2).IsMemoRef())
		FoxMemo::Read(parm(2), pDest, true);
	else
		throw E_INVALIDPARAMS;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
	{
		SaveCustomErrorEx("ADirEx", "Callback function length is greater than maximum length of 1024.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	bool bCallback = pCallback.Len() > 0;
	if (bCallback)
	{
		sProgress.pCallback.SetCallback(pCallback);
		sProgress.nCallbackLimiter.QuadPart = dwCallbackLimiter;
	}

	if (pSource.Len() < MAX_PATH && pDest.Len() < MAX_PATH)
	{
		bRetVal = CopyFileEx(pSource.Ptr<LPCSTR>(), pDest.Ptr<LPCSTR>(), bCallback ? FileProgressCallback : 0, bCallback ? &sProgress : 0, &bCanceled, dwCopyFlags);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideSource;
		FoxWString<MAX_PATH * 2> pWideDest;
		pWideSource.ConvertFilePath(pSource, __sv(FILE_UNICODE_EXTENSION));
		pWideDest.ConvertFilePath(pDest, __sv(FILE_UNICODE_EXTENSION));
		bRetVal = CopyFileExW(pWideSource, pWideDest, bCallback ? FileProgressCallback : 0, bCallback ? &sProgress : 0, &bCanceled, dwCopyFlags);
	}

	if (!bRetVal)
	{
		DWORD nLastError = GetLastError();
		if (nLastError != ERROR_REQUEST_ABORTED)
		{
			SaveWin32Error("CopyFileEx", nLastError);
			throw E_APIERROR;
		}
	}

	Return(sProgress.nStatus);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall MoveFileExLib(ParamBlkEx& parm)
{
try
{
 	FoxString pSource;
 	FoxString pDest;
	FoxString pCallback(parm,3);
	DWORD dwMoveFlags = parm.PCount() >= 4 ? parm(4)->ev_long : (MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
	DWORD dwCallbackLimiter = parm.PCount() == 5 ? parm(5)->ev_long : 100;
	BOOL bRetVal;
	FILEPROGRESSPARAM sProgress;

	if (parm(1)->Vartype() == 'C')
		pSource.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pSource, true);
	else
		throw E_INVALIDPARAMS;

	if (parm(2)->Vartype() == 'C')
		pDest.Attach(parm(2), 1);
	else if (parm(2).IsMemoRef())
		FoxMemo::Read(parm(2), pDest, true);
	else
		throw E_INVALIDPARAMS;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
	{
		SaveCustomErrorEx("MoveFileEx", "Callback function length is greater than maximum length of 1024.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	bool bCallback = pCallback.Len() > 0;

	// builds the format command to be called back
	if (bCallback)
	{
		sProgress.pCallback.SetCallback(pCallback);
		sProgress.nCallbackLimiter.QuadPart = dwCallbackLimiter;
	}

	if (pSource.Len() < MAX_PATH && pDest.Len() < MAX_PATH)
	{
		bRetVal = MoveFileWithProgress(pSource.Ptr<LPCSTR>(), pDest.Ptr<LPCSTR>(), bCallback ? FileProgressCallback : 0, bCallback ? &sProgress : 0, dwMoveFlags);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideSource;
		FoxWString<MAX_PATH * 2> pWideDest;
		pWideSource.ConvertFilePath(pSource, __sv(FILE_UNICODE_EXTENSION));
		pWideDest.ConvertFilePath(pDest, __sv(FILE_UNICODE_EXTENSION));
		bRetVal = MoveFileWithProgressW(pWideSource, pWideDest, bCallback ? FileProgressCallback : 0, bCallback ? &sProgress : 0, dwMoveFlags);
	}

	if (!bRetVal)
	{
		DWORD nLastError = GetLastError();
		if (nLastError != ERROR_REQUEST_ABORTED)
		{
			SaveWin32Error("MoveFileWithProgress", nLastError);
			throw E_APIERROR;
		}
	}

	Return(sProgress.nStatus);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void FILEPROGRESSPARAM::ConvertRetVal()
{
	if (vRetVal.Vartype() == 'L')
		nStatus = vRetVal.ev_length ? PROGRESS_CONTINUE : PROGRESS_CANCEL;
	else if (vRetVal.Numeric())
		nStatus = vRetVal.NumericCast<DWORD>();
	else
	{
		vRetVal.Release();
		nStatus = PROGRESS_CANCEL;
	}
}

DWORD _stdcall FileProgressCallback(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber,
	DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData)
{
	unused(StreamSize);
	unused(StreamBytesTransferred);
	unused(hSourceFile);
	unused(hDestinationFile);

	LPFILEPROGRESSPARAM pProgress = (LPFILEPROGRESSPARAM)lpData;
	double nPercentCopied;
	int nErrorNo;
	if (dwCallbackReason == CALLBACK_CHUNK_FINISHED || (dwCallbackReason == CALLBACK_STREAM_SWITCH && dwStreamNumber == 1))
	{
		if (TotalBytesTransferred.QuadPart > 0)
		{
			if (PerformanceCounter::GetMillisecondsSince(pProgress->nLastCallbackTime) < pProgress->nCallbackLimiter.QuadPart)
				return pProgress->nStatus;
			else
				pProgress->nLastCallbackTime = PerformanceCounter::GetCounter();

			nPercentCopied = (double)TotalBytesTransferred.QuadPart / (double)TotalFileSize.QuadPart * 100;
		}
		else
		{
			pProgress->nLastCallbackTime = PerformanceCounter::GetCounter();
			nPercentCopied = 0.0;
		}

		nErrorNo = pProgress->pCallback.Evaluate(pProgress->vRetVal, TotalBytesTransferred.QuadPart, TotalFileSize.QuadPart, nPercentCopied);
		if (nErrorNo == 0)
			pProgress->ConvertRetVal();
		else
			pProgress->nStatus = PROGRESS_CANCEL;
	}
	return pProgress->nStatus;
}

void _fastcall CompareFileTimes(ParamBlkEx& parm)
{
try
{
	FoxString pFile1;
	FoxString pFile2;
	ApiHandle hSource, hDest;
	int nTimeType = parm.PCount() >= 3 ? parm(3)->ev_long : 0;
	LARGE_INTEGER sSourceTime, sDestTime;
	FoxWString<MAX_PATH * 2> pWideFile;

	if (parm(1)->Vartype() == 'C')
		pFile1.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFile1, true);
	else
		throw E_INVALIDPARAMS;

	if (parm(2)->Vartype() == 'C')
		pFile2.Attach(parm(2), 1);
	else if (parm(2).IsMemoRef())
		FoxMemo::Read(parm(2), pFile2, true);
	else
		throw E_INVALIDPARAMS;

	pFile1.Fullpath();
	pFile2.Fullpath();

	if (pFile1.Len() < MAX_PATH)
		hSource = CreateFile(pFile1.Ptr<LPCSTR>(), 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	else
	{
		pWideFile.ConvertFilePath(pFile1, __sv(FILE_UNICODE_EXTENSION));
		hSource = CreateFileW(pWideFile, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}

	if (!hSource)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	if (pFile2.Len() < MAX_PATH)
		hDest = CreateFile(pFile2.Ptr<LPCSTR>(), 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	else
	{
		pWideFile.ConvertFilePath(pFile2, __sv(FILE_UNICODE_EXTENSION));
		hDest = CreateFileW(pWideFile, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}

	if (!hDest)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	switch (nTimeType)
	{
		case 0:
			if (!GetFileTime(hSource, 0, 0, (LPFILETIME)&sSourceTime))
			{
				SaveWin32Error("GetFileTime", GetLastError());
				throw E_APIERROR;
			}

			if (!GetFileTime(hDest, 0, 0, (LPFILETIME)&sDestTime))
			{
				SaveWin32Error("GetFileTime", GetLastError());
				throw E_APIERROR;
			}
			break;

		case 1:
			if (!GetFileTime(hSource, 0, (LPFILETIME)&sSourceTime, 0))
			{
				SaveWin32Error("GetFileTime", GetLastError());
				throw E_APIERROR;
			}

			if (!GetFileTime(hDest, 0, (LPFILETIME)&sDestTime, 0))
			{
				SaveWin32Error("GetFileTime", GetLastError());
				throw E_APIERROR;
			}
			break;

		case 2:
			if (!GetFileTime(hSource, (LPFILETIME)&sSourceTime, 0, 0))
			{
				SaveWin32Error("GetFileTime", GetLastError());
				throw E_APIERROR;
			}

			if (!GetFileTime(hDest, (LPFILETIME)&sDestTime, 0, 0))
			{
				SaveWin32Error("GetFileTime", GetLastError());
				throw E_APIERROR;
			}
			break;

		default:
			throw E_INVALIDPARAMS;
	}

	if (sSourceTime.QuadPart == sDestTime.QuadPart)
		Return(0);
	else if (sSourceTime.QuadPart > sDestTime.QuadPart)
		Return(1);
	else
		Return(2);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall DeleteFileEx(ParamBlkEx& parm)
{
try
{
	FoxString pFileName;

	if (parm(1)->Vartype() == 'C')
		pFileName.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (pFileName.Len() < MAX_PATH)
	{
		pFileName.Fullpath();
		DeleteFileExImpl(pFileName, INVALID_FILE_ATTRIBUTES);
	}
	else
	{
		FoxWString<MAX_PATH*2> pWideFileName;
		pWideFileName.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		DeleteFileExImpl(pWideFileName, INVALID_FILE_ATTRIBUTES);
	}
	if (parm.PCount() < 2 || parm(2)->ev_long == 1)
		Return(true);
	else if (parm(2)->ev_long == 2)
		Return(0);
	else
		Return("");
}
catch (int nErrorNo)
{
	if (parm.PCount() < 2)
		RaiseError(nErrorNo);
	else if (parm(2)->ev_long == 1)
		Return(false);
	else
	{
		VFP2CTls& tls = VFP2CTls::Tls();
		LPVFP2CERROR pError = &tls.ErrorInfo[tls.ErrorCount];
		if (parm(2)->ev_long == 2)
			Return(pError->nErrorNo);
		else
			Return(pError->aErrorMessage);
	}
}
}

void _stdcall FileSearchDeleteCallback(CStrBuilder<MAX_WIDE_PATH>& pPath, DWORD nAttributes, LPVOID pParam)
{
	unused(pParam);
	if (nAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		if (pPath.Len() < MAX_PATH)
		{
			RemoveDirectoryEx(pPath);
		}
		else
		{
			FoxWString<MAX_PATH * 2> pWidePath;
			pWidePath.ConvertFilePath(pPath, __sv(FILE_UNICODE_EXTENSION));
			RemoveDirectoryEx(pWidePath);
		}
	}
	else
	{
		if (pPath.Len() < MAX_PATH)
		{
			DeleteFileExImpl(pPath, nAttributes);
		}
		else
		{
			FoxWString<MAX_PATH * 2> pWidePath;
			pWidePath.ConvertFilePath(pPath, __sv(FILE_UNICODE_EXTENSION));
			DeleteFileExImpl(pWidePath, nAttributes);
		}
	}
}

void _fastcall DeleteDirectory(ParamBlkEx& parm)
{
	FileSearchBase<>* pFileSearch = 0;
try
{
	FoxString pDirectory;
	if (parm(1)->Vartype() == 'C')
		pDirectory.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pDirectory, true);
	else
		throw E_INVALIDPARAMS;

	FoxString pSearch(pDirectory);
	pSearch.AddBsWildcard();
	pFileSearch = new(std::nothrow) Win32FileSearch(pSearch, 0, 0, 0, ADIREX_FULLPATH | ADIREX_RECURSIVE);
	if (pFileSearch == 0)
		throw E_INSUFMEMORY;
	pFileSearch->ExecuteReverse(FileSearchDeleteCallback, 0);
	if (pDirectory.Len() < MAX_PATH)
	{
		RemoveDirectoryEx(pDirectory);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideDirectory;
		pWideDirectory.ConvertFilePath(pDirectory, __sv(FILE_UNICODE_EXTENSION));
		RemoveDirectoryEx(pWideDirectory);
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
if (pFileSearch)
	delete pFileSearch;
}

void _fastcall GetOpenFileNameEx(ParamBlkEx& parm)
{
try
{
	FoxString pFilter(parm,2,2);
	FoxString pFileName(parm,3);
	FoxString pInitialDir(parm,4);
	FoxString pTitle(parm,5);
	FoxArray pArray(parm, 7, FoxArray::BindByName);
	FoxString pCallback(parm,8);
	FoxString pFiles(MAX_OPENFILENAME_BUFFER);
	FoxString pFileBuffer;
	OPENFILENAME sOpenFilename;
	RtlZeroMemory(&sOpenFilename, sizeof(OPENFILENAME));
	sOpenFilename.lStructSize = sizeof(OPENFILENAME);
	OpenfileCallback sCallbackParam;
	 
	if (pFileName.Len() > MAX_OPENFILENAME_BUFFER)
	{
		SaveCustomErrorEx("GetOpenFileNameEx", "Filename parameter is greater than maximum of %U bytes.", E_VFP_INVALIDPARAMS, MAX_OPENFILENAME_BUFFER);
		throw E_APIERROR;
	}
		
	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
	{
		SaveCustomErrorEx("GetOpenFileNameEx", "Callback function length is greater than maximum length of 1024.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if (parm.PCount() >= 1 && parm(1)->ev_long)
		sOpenFilename.Flags = parm(1)->ev_long & ~(OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE | OFN_ALLOWMULTISELECT);
	else
		sOpenFilename.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NODEREFERENCELINKS |
			OFN_FILEMUSTEXIST |	OFN_DONTADDTORECENT;

	if (parm.PCount() >= 6)
		sOpenFilename.FlagsEx = parm(6)->ev_long;

	sOpenFilename.Flags |= OFN_ENABLEHOOK;
	sOpenFilename.lpfnHook = &GetFileNameCallback;

	sOpenFilename.hwndOwner = WTopHwnd();

	if (pFileName.Len())
		pFiles = pFileName;
	else
		pFiles[0] = '\0';

	sOpenFilename.lpstrFile = pFiles.Ptr<LPSTR>();
	sOpenFilename.nMaxFile = pFiles.Size();

	if (pFilter.Len())
		sOpenFilename.lpstrFilter = pFilter.Ptr<LPCSTR>();
	else if (!(sOpenFilename.Flags & OFN_EXPLORER))
		sOpenFilename.lpstrFilter = "All\0*.*\0";

	sOpenFilename.lpstrInitialDir = pInitialDir.Ptr<LPCSTR>();
	sOpenFilename.lpstrTitle = pTitle.Ptr<LPCSTR>();

	// if an arrayname is passed for multiselect
	if (pArray)
	{
		// allocate memory for the Value structure to store the filenames
		pFileBuffer.Size(MAX_PATH);
		// set multiselect flag
		sOpenFilename.Flags |= OFN_ALLOWMULTISELECT;
	}

	// if a callback function is passed
	if (pCallback.Len())
	{
		// setup the OPENFILECALLBACK structure
		sOpenFilename.lCustData = (LPARAM)&sCallbackParam;
		// build the callback string passed to sprintfex
		sCallbackParam.pCallback.SetCallback(pCallback);
	}

	if (GetOpenFileName(&sOpenFilename))
	{
		if (sOpenFilename.Flags & OFN_ALLOWMULTISELECT)
		{
			unsigned int nFileCount;
			unsigned int nRow = 0;
			CStringView pFileName;

			if (sOpenFilename.Flags & OFN_EXPLORER)
			{
				pFiles.Len(pFiles.StringDblLen());
				CStringView pFileView(pFiles.Ptr<char*>(), pFiles.Len());
				nFileCount = pFileView.GetWordCount('\0');
				pArray.Dimension(nFileCount);

				while (nFileCount--)
				{
					nRow++;
					pFileName = pFileView.GetWordNum(1, '\0');
					pArray(nRow) = pFileBuffer = pFileName;
					pFileView = pFileView + (pFileName.Len + 1);
				}
			}
			else
			{
				pFiles.StringLen();
				CStringView pFileView(pFiles.Ptr<char*>(), pFiles.Len());
				// when OFN_EXPLORER flag is not set, files are seperated by a space character
				nFileCount = pFileView.GetWordCount(' ');
				pArray.Dimension(nFileCount);
				
				while (nFileCount--)
				{
					nRow++;
					pFileName = pFileView.GetWordNum(1, ' ');
					pArray(nRow) = pFileBuffer = pFileName;
					pFileView = pFileView + (pFileName.Len + 1);
				}
			}
			pArray.ReturnRows();
		}
		else
			pFiles.StringLen().Return();
	}
	else
	{
		DWORD nLastError = CommDlgExtendedError();
		if (nLastError)
		{
			SaveCustomErrorEx("GetOpenFileName", "Function failed.", nLastError);
			Return(-1);
		}
		else
			Return(0);
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall GetSaveFileNameEx(ParamBlkEx& parm)
{
try
{
	FoxString pFilter(parm,2,2);
	FoxString pFileName(parm,3);
	FoxString pInitialDir(parm,4);
	FoxString pTitle(parm,5);
	FoxString pCallback(parm,7);
	FoxString pFiles(MAX_OPENFILENAME_BUFFER);
	OPENFILENAME sFile;
	RtlZeroMemory(&sFile, sizeof(OPENFILENAME));
	sFile.lStructSize = sizeof(OPENFILENAME);
	OpenfileCallback sCallbackParam;

	if (pFileName.Len() > MAX_OPENFILENAME_BUFFER)
	{
		SaveCustomErrorEx("GetSaveFileNameEx", "Filename parameter is greater than maximum of %U bytes.", E_VFP_INVALIDPARAMS, MAX_OPENFILENAME_BUFFER);
		throw E_APIERROR;
	}

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
	{
		SaveCustomErrorEx("GetSaveFileNameEx", "Callback function length is greater than maximum length of 1024.", E_VFP_INVALIDPARAMS);
		throw E_APIERROR;
	}

	if (parm.PCount() >= 1 && parm(1)->ev_long)
		sFile.Flags = parm(1)->ev_long & ~(OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE);
	else
		sFile.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR;

	if (parm.PCount() >= 6)
		sFile.FlagsEx = parm(6)->ev_long;

	// set callback procedure 
	sFile.Flags |= OFN_ENABLEHOOK;
	sFile.lpfnHook = &GetFileNameCallback;

	sFile.hwndOwner = WTopHwnd();

	sFile.lpstrFile = pFiles.Ptr<LPSTR>();
	sFile.nMaxFile = MAX_OPENFILENAME_BUFFER;

	if (pFilter.Len())
		sFile.lpstrFilter = pFilter.Ptr<LPCSTR>();
	else if (!(sFile.Flags & OFN_EXPLORER))
		sFile.lpstrFilter = "All\0*.*\0";

	if (pFileName.Len())
		pFiles = pFileName;
	else
		pFiles.NullTerminate();

	if (pInitialDir.Len())
		sFile.lpstrInitialDir = pInitialDir.Ptr<LPCSTR>();

	if (pTitle.Len())
		sFile.lpstrTitle = pTitle.Ptr<LPCSTR>();

	if (pCallback.Len())
	{
		sFile.lCustData = (LPARAM)&sCallbackParam;
		sCallbackParam.pCallback.SetCallback(pCallback);
	}

	if (GetSaveFileName(&sFile)) {
		pFiles.StringLen();
		if (pFilter.Len() && sFile.nFilterIndex) {
			char *pSelectedFilter = pFilter.Ptr<char*>();
			for (unsigned int xj = 1; xj < sFile.nFilterIndex * 2; xj++)
				pSelectedFilter += strlen(pSelectedFilter) + 1;

			CStringView pSFView(pSelectedFilter);
			FoxString pExtension(pSFView);
			pExtension.SubStr(pExtension.Rat('.'));
			if (!pExtension.ICompare(__sv(".*"))) {
				if (pFiles.At('.')) {
					FoxString pCurrentExtension(pFiles);
					pCurrentExtension.SubStr(pCurrentExtension.Rat('.'));
					if (!pCurrentExtension.ICompare(pExtension))
						pFiles += pExtension;
				}
				else
					pFiles += pExtension;
			}
		}
		pFiles.Return();
	}
	else
	{
		DWORD nLastError = CommDlgExtendedError();
		if (nLastError)
		{
			SaveCustomErrorEx("GetSaveFileName", "Function failed.", nLastError);
			Return(-1);
		}
		else
			Return(0);
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

UINT_PTR _stdcall GetFileNameCallback(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPOPENFILENAME lpOpfn;
	OpenfileCallback *lpCallback;
	NMHDR *lpHdr;

	if (uMsg == WM_INITDIALOG)
	{
		lpOpfn = reinterpret_cast<LPOPENFILENAME>(lParam);
		if (lpOpfn->lCustData)
			SetWindowLongPtr(hDlg, GWLP_USERDATA, static_cast<LONG>(lpOpfn->lCustData));
	}
	else if (uMsg == WM_NOTIFY)
	{
		lpCallback = reinterpret_cast<OpenfileCallback*>(GetWindowLongPtr(hDlg,GWLP_USERDATA));
		if (lpCallback)
		{
			lpHdr = reinterpret_cast<NMHDR*>(lParam);
			lpCallback->nErrorNo = lpCallback->pCallback.Evaluate(lpCallback->vRetVal, lpHdr->hwndFrom, lpHdr->idFrom, lpHdr->code);
			if (lpCallback->nErrorNo == 0)
			{
				if (lpCallback->vRetVal.Numeric())
					return lpCallback->vRetVal.NumericCast<UINT_PTR>();
				else if (lpCallback->vRetVal.Vartype() == 'L')
					return lpCallback->vRetVal.ev_length;
				else
				{
					lpCallback->vRetVal.Release();
					lpCallback->vRetVal.SetNull();
				}
			}
		}
	}
	return FALSE;
}

void _fastcall ADriveInfo(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1));
	FoxString pDrive(__sv("X:\\"));
	ApiHandle hDriveHandle;
	
	STORAGE_DEVICE_NUMBER sDevNo;
	BOOL bApiRet;
	DWORD nDriveMask, nDriveCnt = 0, nMask = 1, dwBytes;	

	char pDriveEx[] = "\\\\.\\X:";

	nDriveMask = GetLogicalDrives();
	for (unsigned int xj = 0; xj <= 31; xj++)
	{
		if (nDriveMask & nMask)
			nDriveCnt++;
		nMask <<= 1;
	}

	pArray.Dimension(nDriveCnt,4);

	nMask = 1;
	unsigned int nRow = 0;
	for (unsigned int xj = 0; xj <= 31; xj++)
	{
		if (nDriveMask & nMask)
		{
			nRow++;

			pDrive[0] = (char)('A' + xj);
			pArray(nRow,1) = pDrive;
			pArray(nRow,2) = GetDriveType(pDrive.Ptr<LPCSTR>());

			// replace X in "\\.\X:" with the drive letter
			pDriveEx[4] = pDrive[0];
			hDriveHandle = CreateFile(pDriveEx,0,0,0,OPEN_EXISTING,0,0);
			if (!hDriveHandle)
			{
				SaveWin32Error("CreateFile", GetLastError());
				throw E_APIERROR;
			}

			bApiRet = DeviceIoControl(hDriveHandle, IOCTL_STORAGE_GET_DEVICE_NUMBER, 0, 0, &sDevNo, sizeof(sDevNo), &dwBytes,0);
			if (bApiRet)
			{
				pArray(nRow,3) = sDevNo.DeviceNumber;
				pArray(nRow,4) = sDevNo.PartitionNumber;
			}
			else
			{
				pArray(nRow,3) = -1;
				pArray(nRow,4) = -1;
			}
		}
		nMask <<= 1;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AVolumes(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1), 1, 1);
	VolumeSearch pSearch;

	if (pSearch.FindFirst())
	{
		do 
		{
			pArray.Grow();
			pArray() = pSearch.Volume();
		} while (pSearch.FindNext());

		pArray.ReturnRows();
	}
	Return(0);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AVolumeMountPoints(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1), 1, 1);
	FoxString pVolume(parm(2));
	VolumeMountPointSearch pSearch;

	if (pSearch.FindFirst(pVolume.Ptr<LPSTR>()))
	{
		do
		{
			pArray.Grow();
			pArray() = pSearch.MountPoint();
		} while (pSearch.FindNext());
		
		pArray.ReturnRows();
	}
	Return(0);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AVolumePaths(ParamBlkEx& parm)
{
try
{
	if (!fpGetVolumePathNamesForVolumeName)
		throw E_NOENTRYPOINT;

	FoxArray pArray(parm(1));
	FoxString pVolume(parm(2));
	FoxString pBuffer(2048);
	DWORD dwLen; 

	do 
	{
		if (!fpGetVolumePathNamesForVolumeName(pVolume.Ptr<LPSTR>(), pBuffer.Ptr<LPSTR>(), pBuffer.Size(), &dwLen))
		{
			DWORD nLastError = GetLastError();
			if (nLastError == ERROR_MORE_DATA)
			{
				pBuffer.Size(dwLen);
			}
			else
			{
				SaveWin32Error("GetVolumePathNamesForVolumeName", GetLastError());
				throw E_APIERROR;
			}
		}
		else
			break;
	} while (true);

	unsigned int nCount = pBuffer.StringDblCount();
	if (nCount == 0)
	{
		Return(0);
		return;
	}

	FoxString pPath(512);
	unsigned int nRow = 0;
	char *pPathPtr = pBuffer.Ptr<char*>();
	pArray.Dimension(nCount);
	while (nCount--)
	{
		nRow++;
		pArray(nRow) = pPath = pPathPtr;
		// advance pointer by length of string + 1 for nullterminator
		pPathPtr += pPath.Len() + 1;
	}	

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AVolumeInformation(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1), 5, 1);
	FoxString pRootPath(parm(2), 2);
	FoxString pVolumeName(MAX_PATH);
	FoxString pFileSystemName(MAX_PATH);
	DWORD dwVolumeSerialNumber, dwMaximumComponentLen, dwFileSystemFlags;

	SwitchErrorMode pErrorMode(SEM_FAILCRITICALERRORS);

	pRootPath.AddBs();
	if (!GetVolumeInformation(pRootPath.Ptr<LPCSTR>(), pVolumeName.Ptr<LPSTR>(), pVolumeName.Size(), &dwVolumeSerialNumber, &dwMaximumComponentLen,
							&dwFileSystemFlags, pFileSystemName.Ptr<LPSTR>(), pFileSystemName.Size()))
	{
		SaveWin32Error("GetVolumeInformation", GetLastError());
		throw E_APIERROR;
	}

	pArray(1) = pVolumeName.StringLen();
	pArray(2) = dwVolumeSerialNumber;
	pArray(3) = dwMaximumComponentLen;
	pArray(4) = dwFileSystemFlags;
	pArray(5) = pFileSystemName.StringLen();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall GetWindowsDirectoryEx(ParamBlkEx& parm)
{
try
{
	FoxString pDir(MAX_PATH);

	pDir.Len(GetWindowsDirectory(pDir.Ptr<LPSTR>(),MAX_PATH));
	if (pDir.Len() == 0)
	{
		SaveWin32Error("GetWindowsDirectory", GetLastError());
		throw E_APIERROR;
	}

	pDir.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall GetSystemDirectoryEx(ParamBlkEx& parm)
{
try
{
	FoxString pDir(MAX_PATH);

	pDir.Len(GetSystemDirectory(pDir.Ptr<LPSTR>(),MAX_PATH));
    if (pDir.Len() == 0)
	{
		SaveWin32Error("GetSystemDirectory", GetLastError());
		throw E_APIERROR;
	}

	pDir.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ExpandEnvironmentStringsEx(ParamBlkEx& parm)
{
try
{
	FoxString pEnvString(parm(1));
	FoxString pEnvBuffer(MAX_ENVSTRING_BUFFER);

	pEnvBuffer.Len(ExpandEnvironmentStrings(pEnvString.Ptr<LPCSTR>(), pEnvBuffer.Ptr<LPSTR>(), pEnvBuffer.Size()));
	if (!pEnvBuffer.Len())
	{
		SaveWin32Error("ExpandEnvironmentStrings", GetLastError());
		throw E_APIERROR;
	}
	else if (pEnvBuffer.Len() > MAX_ENVSTRING_BUFFER)
	{
		pEnvBuffer.Size(pEnvBuffer.Len());
		pEnvBuffer.Len(ExpandEnvironmentStrings(pEnvString.Ptr<LPCSTR>(),pEnvBuffer.Ptr<LPSTR>(),pEnvBuffer.Size()));
		if (!pEnvBuffer.Len())
		{
			SaveWin32Error("ExpandEnvironmentStrings", GetLastError());
			throw E_APIERROR;
		}
	}
	pEnvBuffer.Len(pEnvBuffer.Len()-1); // subtract nullterminator
	pEnvBuffer.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

#pragma warning(disable : 4290)
void _stdcall DeleteFileExImpl(CStringView pFileName, DWORD nFileAttribs) throw(int)
{
	RemoveReadOnlyAttrib(pFileName, nFileAttribs);
	if (!DeleteFile(pFileName.Data))
	{
		SaveWin32Error("DeleteFile", GetLastError());
		throw E_APIERROR;
	}
}

void _stdcall DeleteFileExImpl(CWideStringView pFileName, DWORD nFileAttribs) throw(int)
{
	RemoveReadOnlyAttrib(pFileName, nFileAttribs);
	if (!DeleteFileW(pFileName.Data))
	{
		SaveWin32Error("DeleteFile", GetLastError());
		throw E_APIERROR;
	}
}

void _stdcall RemoveDirectoryEx(CStringView pPath) throw(int)
{
	BOOL bRetVal;
	bRetVal = RemoveDirectory(pPath.Data);
	if (!bRetVal)
	{
		SaveWin32Error("RemoveDirectory", GetLastError());
		throw E_APIERROR;
	}
}

void _stdcall RemoveDirectoryEx(CWideStringView pPath) throw(int)
{
	BOOL bRetVal;
	bRetVal = RemoveDirectoryW(pPath.Data);
	if (!bRetVal)
	{
		SaveWin32Error("RemoveDirectory", GetLastError());
		throw E_APIERROR;
	}
}

void _stdcall RemoveReadOnlyAttrib(CStringView pFileName, DWORD nFileAttribs) throw(int)
{
	if (nFileAttribs == INVALID_FILE_ATTRIBUTES)
	{
		nFileAttribs = GetFileAttributes(pFileName.Data);
		if (nFileAttribs == INVALID_FILE_ATTRIBUTES)
		{
			SaveWin32Error("GetFileAttributes", GetLastError());
			throw E_APIERROR;
		}
	}
	if (nFileAttribs & FILE_ATTRIBUTE_READONLY)
	{
		if (!SetFileAttributes(pFileName.Data,nFileAttribs & ~FILE_ATTRIBUTE_READONLY))
		{
			SaveWin32Error("SetFileAttributes", GetLastError());
			throw E_APIERROR;
		}
	}
}

void _stdcall RemoveReadOnlyAttrib(CWideStringView pFileName, DWORD nFileAttribs) throw(int)
{
	if (nFileAttribs == INVALID_FILE_ATTRIBUTES)
	{
		nFileAttribs = GetFileAttributesW(pFileName.Data);
		if (nFileAttribs == INVALID_FILE_ATTRIBUTES)
		{
			SaveWin32Error("GetFileAttributes", GetLastError());
			throw E_APIERROR;
		}
	}
	if (nFileAttribs & FILE_ATTRIBUTE_READONLY)
	{
		if (!SetFileAttributesW(pFileName.Data, nFileAttribs & ~FILE_ATTRIBUTE_READONLY))
		{
			SaveWin32Error("SetFileAttributes", GetLastError());
			throw E_APIERROR;
		}
	}
}
#pragma warning(default : 4290)

void _fastcall FCreateEx(ParamBlkEx& parm)
{
try
{
	FoxString pFileName;

	if (parm(1)->Vartype() == 'C')
		pFileName.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	HANDLE hFile;
	DWORD dwAccess, dwShare, dwFlags;

	MapFileAccessFlags(parm.PCount() >= 2 ? parm(2)->ev_long : FILE_ATTRIBUTE_NORMAL, parm.PCount() >= 3 ? parm(3)->ev_long : 2, parm.PCount() >= 4 ? parm(4)->ev_long : 0, &dwAccess, &dwShare, &dwFlags);

	if (pFileName.Len() <= MAX_PATH)
		hFile = CreateFile(pFileName.Ptr<LPCSTR>(), dwAccess, dwShare, 0, CREATE_ALWAYS, dwFlags, 0);
	else
	{
		FoxWString<MAX_PATH*2> pWideFileName;
		pWideFileName.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		hFile = CreateFileW(pWideFileName,dwAccess,dwShare,0,CREATE_ALWAYS,dwFlags,0);
	}

	if (hFile == INVALID_HANDLE_VALUE)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	VFP2CTls::Tls().FileHandles.Add(hFile);
	Return(hFile);
}
catch(int nErrorNo)
{
	if (nErrorNo == E_APIERROR)
		Return(-1);
	else
		RaiseError(nErrorNo);
}
}

void _fastcall FOpenEx(ParamBlkEx& parm)
{
try
{
	FoxString pFileName;
	HANDLE hFile;
	DWORD dwAccess, dwShare, dwFlags;

	if (parm(1)->Vartype() == 'C')
		pFileName.Attach(parm(1), 1);
	else if (parm(1).IsMemoRef())
		FoxMemo::Read(parm(1), pFileName, true);
	else
		throw E_INVALIDPARAMS;

	if (pFileName.Len() <= MAX_PATH)
		pFileName.Fullpath();

	MapFileAccessFlags(parm.PCount() >= 2 ? parm(2)->ev_long : 0, parm.PCount() >= 3 ? parm(3)->ev_long : 2, parm.PCount() >= 4 ? parm(4)->ev_long : 0, &dwAccess, &dwShare, &dwFlags);

	if (pFileName.Len() <= MAX_PATH)
        hFile = CreateFile(pFileName.Ptr<LPCSTR>(),dwAccess,dwShare,0,OPEN_EXISTING,dwFlags,0);
	else
	{
		FoxWString<MAX_PATH * 2> pWideFileName;
		pWideFileName.ConvertFilePath(pFileName, __sv(FILE_UNICODE_EXTENSION));
		hFile = CreateFileW(pWideFileName,dwAccess,dwShare,0,OPEN_EXISTING,dwFlags,0);
	}

	if (hFile == INVALID_HANDLE_VALUE)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	VFP2CTls::Tls().FileHandles.Add(hFile);
	Return(hFile);
}
catch(int nErrorNo)
{
	if (nErrorNo == E_APIERROR)
		Return(-1);
	else
		RaiseError(nErrorNo);
}
}

void _fastcall FCloseEx(ParamBlkEx& parm)
{
try
{
	BOOL bApiRet = TRUE, bRet;
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	VFP2CTls& tls = VFP2CTls::Tls();

	if (hFile != INVALID_HANDLE_VALUE)
	{
		bApiRet = CloseHandle(hFile);
		if (!bApiRet)
			SaveWin32Error("CloseHandle", GetLastError());
		else
		{
			unsigned int nIndex = tls.FileHandles.Find(hFile);
			if (nIndex != tls.FileHandles.INVALID_INDEX)
				tls.FileHandles.Remove(nIndex);
		}
	}
	else
	{
		unsigned int nCount = tls.FileHandles.GetCount();
		for (unsigned int xj = 0; xj < nCount; xj++)
		{
			bRet = CloseHandle(tls.FileHandles[xj]);
			if (!bRet) 
			{
				bApiRet = FALSE;
				AddWin32Error("CloseHandle", GetLastError());
			}
		}
		tls.FileHandles.ResetIndex();
	}

	Return(bApiRet == TRUE);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FReadEx(ParamBlkEx& parm)
{
try
{
	BOOL bApiRet;
	DWORD dwRead;
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	FoxString pData(parm(2)->ev_long);

	bApiRet = ReadFile(hFile, pData.Ptr<LPVOID>(), pData.Size(), &dwRead, 0);
	if (!bApiRet)
		SaveWin32Error("ReadFile", GetLastError());

	pData.Len(bApiRet ? dwRead : 0); 
	pData.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FWriteEx(ParamBlkEx& parm)
{
try
{
	if (parm.PCount() == 3 && parm(3)->ev_long < 0)
		throw E_INVALIDPARAMS;

	BOOL bApiRet;
	DWORD dwWritten, dwLength;
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	FoxString pData(parm(2),0);

	if (parm.PCount() == 3 && pData.Len() >= static_cast<DWORD>(parm(3)->ev_long))
		dwLength = parm(3)->ev_long;
	else
		dwLength = pData.Len();

	bApiRet = WriteFile(hFile, pData.Ptr<LPCVOID>(), dwLength, &dwWritten,0);
	if (!bApiRet)
	{
		SaveWin32Error("WriteFile", GetLastError());
		Return(0);
	}
	else
		Return(dwWritten);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FGetsEx(ParamBlkEx& parm)
{
try
{
	BOOL bApiRet;
	unsigned char *pData, *pOrigData;
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	int dwLen = parm.PCount() == 2 ? parm(2)->ev_long : 256;
	int bCarri = 0;
	LONG dwRead, dwBuffer;
	LARGE_INTEGER distance;
	FoxString pBuffer(dwLen);

	pOrigData = pData = pBuffer.Ptr<unsigned char*>();

	dwBuffer = min(dwLen,VFP2C_FILE_LINE_BUFFER);
	while(true)
	{
		bApiRet = ReadFile(hFile, pData, dwBuffer, reinterpret_cast<LPDWORD>(&dwRead), 0);
		if (!bApiRet)
		{
			SaveWin32Error("ReadFile", GetLastError());
			throw E_APIERROR;
		}

		if (dwRead == 0)
			break;

		while (dwRead--)
		{
			if (*pData == '\r') // carriage return detected
			{
				pData++;
				if (*pData == '\n') // skip over linefeeds
				{
					bCarri = 2;
					pData++;
					dwRead--;
				}
				else
					bCarri = 1; // set detect flag

				distance.QuadPart = -dwRead;
				SetFilePointerEx(hFile, distance, 0, FILE_CURRENT); // position filepointer after carriage return and optional linefeed
				break;
			}
			else if (*pData == '\n')
			{
				pData++;
				distance.QuadPart = -dwRead;
				SetFilePointerEx(hFile, distance, 0, FILE_CURRENT); // position filepointer after linefeed
				bCarri = 1; // set detect flag
				break;
			}
			else
				pData++;
		}

		if (bCarri || (pData - pOrigData >= dwLen))
			break;
	}

	pBuffer.Len(pData - pOrigData - bCarri);
	pBuffer.Return();  
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FPutsEx(ParamBlkEx& parm)
{
try
{
	if (parm.PCount() == 3 && parm(3)->ev_long < 0)
		throw E_INVALIDPARAMS;

	BOOL bApiRet;
	DWORD dwWritten, dwLength;
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	FoxString pData(parm(2),2);

	if (parm.PCount() == 3 && pData.Len() >= static_cast<DWORD>(parm(3)->ev_long))
		dwLength = parm(3)->ev_long;
	else
		dwLength = pData.Len();

	// add carriage return & line feed to data
	pData[dwLength] = '\r';
	pData[dwLength+1] = '\n';

	bApiRet = WriteFile(hFile, pData.Ptr<LPCVOID>(), dwLength + 2, &dwWritten,0);
	if (!bApiRet)
		SaveWin32Error("WriteFile", GetLastError());

	if (bApiRet)
		Return(dwWritten);
	else
		Return(0);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FSeekEx(ParamBlkEx& parm)
{
try
{
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	LARGE_INTEGER nFilePos;
	LARGE_INTEGER nNewFilePos;
	FoxInt64 pNewFilePos;
	DWORD dwMove = parm.PCount() == 3 ? parm(3)->ev_long : FILE_BEGIN;

	nFilePos.QuadPart = parm(2)->NumericCast<LONGLONG>();
	if (!SetFilePointerEx(hFile, nFilePos, &nNewFilePos, dwMove))
	{
		SaveWin32Error("SetFilePointerEx", GetLastError());
		throw E_APIERROR;
	}

	pNewFilePos = nNewFilePos.QuadPart;
	Return(pNewFilePos);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FEoFEx(ParamBlkEx& parm)
{
try
{
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	LARGE_INTEGER nMove, nCurr, nEof;

	nMove.QuadPart = 0;
	if (!SetFilePointerEx(hFile, nMove, &nCurr, FILE_CURRENT))
	{
		SaveWin32Error("SetFilePointer", GetLastError());
		throw E_APIERROR;
	}

	if (!GetFileSizeEx(hFile, &nEof))
	{
		SaveWin32Error("GetFileSizeEx", GetLastError());
		throw E_APIERROR;
	}

	Return(nCurr.QuadPart == nEof.QuadPart);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FChSizeEx(ParamBlkEx& parm)
{
try
{
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	LARGE_INTEGER nSize;
	LARGE_INTEGER nMove, nCurrPos;
	FoxInt64 pFilePos;

	nSize.QuadPart = parm(2)->NumericCast<LONGLONG>();
    
	// save current file pointer
	nMove.QuadPart = 0;
	if (!SetFilePointerEx(hFile, nMove, &nCurrPos, FILE_CURRENT))
	{
		SaveWin32Error("SetFilePointerEx", GetLastError());
		throw E_APIERROR;
	}

	// set file pointer to specified size
	if (!SetFilePointerEx(hFile, nSize, 0, FILE_BEGIN))
	{
		SaveWin32Error("SetFilePointerEx", GetLastError());
		throw E_APIERROR;
	}

	// set file size
	if (!SetEndOfFile(hFile))
	{
		SaveWin32Error("SetEndOfFile", GetLastError());
		throw E_APIERROR;
	}

	// reset file pointer to saved position
	if (!SetFilePointerEx(hFile, nCurrPos, 0, FILE_BEGIN))
	{
		SaveWin32Error("SetFilePointerEx", GetLastError());
		throw E_APIERROR;
	}

	pFilePos = nSize.QuadPart;
	Return(pFilePos);
}
catch(int nErrorNo)
{
	if (nErrorNo == E_APIERROR)
		Return(0);
	else
		RaiseError(nErrorNo);
}
}

void _fastcall FFlushEx(ParamBlkEx& parm)
{
try
{
	BOOL bApiRet;
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	bApiRet = FlushFileBuffers(hFile);
	if (!bApiRet)
		SaveWin32Error("FlushFileBuffers", GetLastError());

	Return(bApiRet == TRUE);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FLockFile(ParamBlkEx& parm)
{
try
{
	BOOL bApiRet;
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	LARGE_INTEGER nOffset, nLen;

	if (parm(2)->Numeric())
		nOffset.QuadPart = parm(2)->NumericCast<__int64>();
	else
		throw E_INVALIDPARAMS;

	if (parm(3)->Numeric())
		nLen.QuadPart = parm(3)->NumericCast<__int64>(); 
	else
		throw E_INVALIDPARAMS;

	bApiRet = LockFile(hFile, nOffset.LowPart, nOffset.HighPart, nLen.LowPart, nLen.HighPart);
	if (!bApiRet)
		SaveWin32Error("LockFile", GetLastError());

	Return(bApiRet == TRUE);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FUnlockFile(ParamBlkEx& parm)
{
try
{
	BOOL bApiRet;
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	LARGE_INTEGER nOffset, nLen;

	if (parm(2)->Numeric())
		nOffset.QuadPart = parm(2)->NumericCast<__int64>();
	else
		throw E_INVALIDPARAMS;

	if (parm(3)->Numeric())
		nLen.QuadPart = parm(3)->NumericCast<__int64>();
	else
		throw E_INVALIDPARAMS;

	bApiRet = UnlockFile(hFile, nOffset.LowPart, nOffset.HighPart, nLen.LowPart, nLen.HighPart);
	if (!bApiRet)
		SaveWin32Error("UnlockFile", GetLastError());

	Return(bApiRet == TRUE);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FLockFileEx(ParamBlkEx& parm)
{
try
{
	BOOL bApiRet;
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	DWORD dwFlags = parm.PCount() == 4 ? parm(4)->ev_long : 0;
	LARGE_INTEGER nOffset, nLen;
	OVERLAPPED sOverlap;

	if (parm(2)->Numeric())
		nOffset.QuadPart = parm(2)->NumericCast<__int64>();
	else
		throw E_INVALIDPARAMS;

	if (parm(3)->Numeric())
		nLen.QuadPart = parm(3)->NumericCast<__int64>();
	else
		throw E_INVALIDPARAMS;

	sOverlap.hEvent = 0;
	sOverlap.Offset = nOffset.LowPart;
	sOverlap.OffsetHigh = nOffset.HighPart;

	bApiRet = LockFileEx(hFile, dwFlags, 0, nLen.LowPart, nLen.HighPart, &sOverlap);

	if (!bApiRet)
		SaveWin32Error("LockFileEx", GetLastError());

	Return(bApiRet == TRUE);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall FUnlockFileEx(ParamBlkEx& parm)
{
try
{
	BOOL bApiRet;
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	LARGE_INTEGER nOffset, nLen;
	OVERLAPPED sOverlap;

	if (parm(2)->Numeric())
		nOffset.QuadPart = parm(2)->NumericCast<__int64>();
	else
		throw E_INVALIDPARAMS;

	if (parm(3)->Numeric())
		nLen.QuadPart = parm(3)->NumericCast<__int64>();
	else
		throw E_INVALIDPARAMS;

	sOverlap.hEvent = 0;
	sOverlap.Offset = nOffset.LowPart;
	sOverlap.OffsetHigh = nOffset.HighPart;

	bApiRet = UnlockFileEx(hFile, 0, nLen.LowPart, nLen.HighPart, &sOverlap);
	if (!bApiRet)
		SaveWin32Error("UnlockFileEx", GetLastError());

	Return(bApiRet == TRUE);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AFHandlesEx(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1));	
	VFP2CTls& tls = VFP2CTls::Tls();
	unsigned int nHandleCnt = tls.FileHandles.GetCount();

	if (nHandleCnt == 0)
	{
		Return(0);
		return;
	}

	pArray.Dimension(nHandleCnt, 1);

	for (unsigned int xj = 0; xj < nHandleCnt; xj++)
	{
		pArray(xj+1) = tls.FileHandles[xj];
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall MemoFieldInfo(ParamBlkEx& parm)
{
	try
	{
		ValueEx vMemo;
		vMemo.SetNull();
		LocatorEx& p1 = parm(1);
		vMemo = p1;
		if (parm.PCount() >= 2)
		{
			LocatorEx& p2 = parm(2);
			p2 = vMemo.ev_long;
		}
		Return((int)vMemo.ev_real);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

#pragma warning(disable : 4290)
void _stdcall MapFileAccessFlags(int nFileAttribs, int nAccess, int nShare, LPDWORD pAccess, LPDWORD pShare, LPDWORD pFlags) throw(int)
{
	*pAccess = 0;
	*pShare = 0;
	*pFlags = 0;

	switch (nAccess)
	{
		case 0:
			*pAccess = GENERIC_READ;
			break;
		case 1:
			*pAccess = GENERIC_WRITE;
			break;
		case 2:
			*pAccess = GENERIC_READ | GENERIC_WRITE;
			break;
		default:
			throw E_INVALIDPARAMS;
	}

	if (nFileAttribs & ~(FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_ENCRYPTED | 
						FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NORMAL |
						FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_OFFLINE |
						FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM |
						FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_BACKUP_SEMANTICS |
						FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OPEN_NO_RECALL |
						FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS |
						FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_SEQUENTIAL_SCAN |
						FILE_FLAG_WRITE_THROUGH))
		throw E_INVALIDPARAMS;

	*pFlags |= nFileAttribs;

	if (nShare & ~(FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE))
		throw E_INVALIDPARAMS;

	*pShare = nShare;
}
#pragma warning(default : 4290)