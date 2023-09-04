// to make File operation (CopyFileEx ..) defines and types available
#include "vfp2c32.h"
#include "vfp2cfile.h"

// #include <winioctl.h>
// #include <uxtheme.h>
#include <propidl.h>
#include <propkey.h>
#include <propvarutil.h>

// dynamic function pointers for runtime linking
static PGETVOLUMEPATHNAMESFORVOLUMENAME fpGetVolumePathNamesForVolumeName = 0;
static PWOW64FSREDIRECTION fpWow64DisableWow64FsRedirection = 0;
static PWOW64FSREDIRECTION fpWow64RevertWow64FsRedirection = 0;

void FileSearchStorage::Initialize(CStringView pDestination, bool bToLocalTime, bool bStringFileAttribs, CStringView pFields)
{
	m_FileName.Size(MAX_PATH);
	m_ToLocalTime = bToLocalTime;
}

void FileSearchStorageArray::Initialize(CStringView pDestination, bool bToLocalTime, bool bStringFileAttribs, CStringView pFields)
{
	FileSearchStorage::Initialize(pDestination, bToLocalTime, bStringFileAttribs, pFields);
	unsigned int nFieldCount = 7;
	if (pFields)
	{
		m_Index_Filename = m_Index_Dosfilename = m_Index_Accesstime = m_Index_Creationtime =
			m_Index_Writetime = m_Index_Filesize = m_Index_Fileattribs = m_Index_StringFileattribs = -1;
		pFields = pFields.Alltrim();
		CStringView pFieldName = pFields.GetWordNum(1, ',');
		unsigned int nFieldNo = 1;
		do 
		{
			CStringView pField = pFieldName.Alltrim();
			if (pField.ICompare("filename"))
			{
				m_Index_Filename = nFieldNo;
			}
			else if (pField.ICompare("dosfilename"))
			{
				m_Index_Dosfilename = nFieldNo;
			}
			else if (pField.ICompare("creationtime"))
			{
				m_Index_Creationtime = nFieldNo;
			}
			else if (pField.ICompare("accesstime"))
			{
				m_Index_Accesstime = nFieldNo;
			}
			else if (pField.ICompare("writetime"))
			{
				m_Index_Writetime = nFieldNo;
			}
			else if (pField.ICompare("filesize"))
			{
				m_Index_Filesize = nFieldNo;
			}
			else if (pField.ICompare("fileattribs"))
			{
				m_Index_Fileattribs = nFieldNo;
			}
			else if (pField.ICompare("cfileattribs"))
			{
				m_Index_StringFileattribs = nFieldNo;
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
		m_Index_Fileattribs = bStringFileAttribs ? -1 : 7;
		m_Index_StringFileattribs = bStringFileAttribs ? 7 : -1;
	}

	m_Array.AutoGrow(256);
	m_Array.Dimension(pDestination, m_Array.AutoGrow(), nFieldCount);
	m_Array.AutoOverflow(CFoxVersion::MajorVersion() >= 9);
}

void FileSearchStorageArray::Finalize()
{
	m_Array.CompactOverflow();
}

bool FileSearchStorageArray::Store(FileSearch* pFileSearch)
{
	int nRow = m_Array.Grow();
	if (m_Index_Filename >= 1)
		m_Array(nRow, m_Index_Filename) = m_FileName = pFileSearch->FileName();

	if (m_Index_Dosfilename >= 1)
		m_Array(nRow, m_Index_Dosfilename) = m_FileName = pFileSearch->AlternateFileName();

	if (m_Index_Creationtime >= 1)
	{
		m_FileTime = pFileSearch->CreationTime();
		if (m_ToLocalTime)
			m_FileTime.ToLocal();
		m_Array(nRow, m_Index_Creationtime) = m_FileTime;
	}

	if (m_Index_Accesstime >= 1)
	{
		m_FileTime = pFileSearch->LastAccessTime();
		if (m_ToLocalTime)
			m_FileTime.ToLocal();
		m_Array(nRow, m_Index_Accesstime) = m_FileTime;
	}

	if (m_Index_Writetime >= 1)
	{
		m_FileTime = pFileSearch->LastWriteTime();
		if (m_ToLocalTime)
			m_FileTime.ToLocal();
		m_Array(nRow, m_Index_Writetime) = m_FileTime;
	}

	if (m_Index_Filesize >= 1)
		m_Array(nRow, m_Index_Filesize) = m_FileSize = pFileSearch->FileSize();

	if (m_Index_Fileattribs >= 1)
		m_Array(nRow, m_Index_Fileattribs) = pFileSearch->FileAttributes();
	
	if (m_Index_StringFileattribs >= 1)
		m_Array(nRow, m_Index_StringFileattribs) = m_FileName.FileAttributesToString(pFileSearch->FileAttributes());

	return true;
}

void FileSearchStorageCursor::Initialize(CStringView pDestination, bool bToLocalTime, bool bStringFileAttribs, CStringView pFields)
{
	FileSearchStorage::Initialize(pDestination, bToLocalTime, bStringFileAttribs, pFields);
	bool created;
	CStrBuilder<256> pCursorDefinition;

	if (pFields)
	{
		m_Index_Filename = m_Index_Dosfilename = m_Index_Accesstime = m_Index_Creationtime =
			m_Index_Writetime = m_Index_Filesize = m_Index_Fileattribs = m_Index_StringFileattribs = -1;
		pFields = pFields.Alltrim();
		CStringView pFieldName = pFields.GetWordNum(1, ',');
		unsigned int nFieldNo = 0;
		while (pFieldName)
		{
			CStringView pField = pFieldName.Alltrim();

			if (pCursorDefinition.Len() > 0)
				pCursorDefinition.Append(",");

			if (pField.ICompare("filename"))
			{
				m_Index_Filename = nFieldNo;
				pCursorDefinition.Append("filename M");
			}
			else if (pField.ICompare("dosfilename"))
			{
				m_Index_Dosfilename = nFieldNo;
				pCursorDefinition.Append("dosfilename C(13)");
			}
			else if (pField.ICompare("creationtime"))
			{
				m_Index_Creationtime = nFieldNo;
				pCursorDefinition.Append("creationtime T");
			}
			else if (pField.ICompare("accesstime"))
			{
				m_Index_Accesstime = nFieldNo;
				pCursorDefinition.Append("accesstime T");
			}
			else if (pField.ICompare("writetime"))
			{
				m_Index_Writetime = nFieldNo;
				pCursorDefinition.Append("writetime T");
			}
			else if (pField.ICompare("filesize"))
			{
				m_Index_Filesize = nFieldNo;
				pCursorDefinition.Append("filesize N(20,0)");
			}
			else if (pField.ICompare("fileattribs"))
			{
				m_Index_Fileattribs = nFieldNo;
				pCursorDefinition.Append("fileattribs I");
			}
			else if (pField.ICompare("cfileattribs"))
			{
				m_Index_StringFileattribs = nFieldNo;
				if (CFoxVersion::MajorVersion() >= 9)
					pCursorDefinition.Append("cfileattribs V(10)");
				else
					pCursorDefinition.Append("cfileattribs C(10)");
			}
			else
			{
				SaveCustomError("AdirEx", "Invalid parameter name '%V' in field list", &pField);
				throw E_APIERROR;
			}

			pFields = pFields + (pFieldName.Len + 1);
			pFieldName = pFields.GetWordNum(1, ',');
			nFieldNo++;
		}
		created = m_Cursor.Create(pDestination, pCursorDefinition);
	}
	else
	{
		pCursorDefinition = "filename M, dosfilename C(13), creationtime T, accesstime T, writetime T, filesize N(20, 0), ";
		if (bStringFileAttribs)
		{
			if (CFoxVersion::MajorVersion() >= 9)
				pCursorDefinition.Append("cfileattribs V(10)");
			else
				pCursorDefinition.Append("cfileattribs C(10)");
		}
		else
			pCursorDefinition.Append("fileattribs I");

		created = m_Cursor.Create(pDestination, pCursorDefinition);
		if (created == false)
		{
			m_Index_Filename = m_Cursor.GetFieldNumber("filename");
			m_Index_Dosfilename = m_Cursor.GetFieldNumber("dosfilename");
			m_Index_Creationtime = m_Cursor.GetFieldNumber("creationtime");
			m_Index_Accesstime = m_Cursor.GetFieldNumber("accesstime");
			m_Index_Writetime = m_Cursor.GetFieldNumber("writetime");
			m_Index_Filesize = m_Cursor.GetFieldNumber("filesize");
			if (bStringFileAttribs)
			{
				m_Index_StringFileattribs = m_Cursor.GetFieldNumber("cfileattribs");
				m_Index_Fileattribs = -1;
			}
			else
			{
				m_Index_Fileattribs = m_Cursor.GetFieldNumber("fileattribs");
				m_Index_StringFileattribs = -1;
			}
		}
		else
		{
			m_Index_Fileattribs = bStringFileAttribs ? -1 : 6;
			m_Index_StringFileattribs = bStringFileAttribs ? 6 : -1;
		}
	}
	m_Cursor.AutoFLock();
}

bool FileSearchStorageCursor::Store(FileSearch* pFileSearch)
{
	m_Cursor.AppendBlank();
	if (m_Index_Filename >= 0)
		m_Cursor(m_Index_Filename) = m_FileName = pFileSearch->FileName();

	if (m_Index_Dosfilename >= 0)
		m_Cursor(m_Index_Dosfilename) = m_FileName = pFileSearch->AlternateFileName();

	if (m_Index_Creationtime >= 0)
	{
		m_FileTime = pFileSearch->CreationTime();
		if (m_ToLocalTime)
			m_FileTime.ToLocal();
		m_Cursor(m_Index_Creationtime) = m_FileTime;
	}

	if (m_Index_Accesstime >= 0)
	{
		m_FileTime = pFileSearch->LastAccessTime();
		if (m_ToLocalTime)
			m_FileTime.ToLocal();
		m_Cursor(m_Index_Accesstime) = m_FileTime;
	}

	if (m_Index_Writetime >= 0)
	{
		m_FileTime = pFileSearch->LastWriteTime();
		if (m_ToLocalTime)
			m_FileTime.ToLocal();
		m_Cursor(m_Index_Writetime) = m_FileTime;
	}

	if (m_Index_Filesize >= 0)
		m_Cursor(m_Index_Filesize) = m_FileSize = pFileSearch->FileSize();
	
	if (m_Index_Fileattribs >= 0)
		m_Cursor(m_Index_Fileattribs) = pFileSearch->FileAttributes();
	
	if (m_Index_StringFileattribs >= 0)
		m_Cursor(m_Index_StringFileattribs) = m_FileName.FileAttributesToString(pFileSearch->FileAttributes());

	return true;
}

void FileSearchStorageCallback::Initialize(CStringView pDestination, bool bToLocalTime, bool bStringFileAttribs, CStringView pFields)
{
	FileSearchStorage::Initialize(pDestination, bToLocalTime, bStringFileAttribs, pFields);
	if (pDestination.Len > 1024 /* VFP2C_MAX_CALLBACKFUNCTION */ || pDestination.Len == 0)
	{
		SaveCustomError("AdirEx", "Callback function length is 0 or greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}
	m_Callback.SetCallback(pDestination);
	if (pFields)
	{
		m_Index_Filename = m_Index_Dosfilename = m_Index_Accesstime = m_Index_Creationtime =
			m_Index_Writetime = m_Index_Filesize = m_Index_Fileattribs = m_Index_StringFileattribs = -1;
		pFields = pFields.Alltrim();
		CStringView pFieldName = pFields.GetWordNum(1, ',');
		unsigned int nParmPos = 0;
		while (pFieldName)
		{
			CStringView pField = pFieldName.Alltrim();
			if (pField.ICompare("filename"))
			{
				m_Index_Filename = nParmPos;
			}
			else if (pField.ICompare("dosfilename"))
			{
				m_Index_Dosfilename = nParmPos;
			}
			else if (pField.ICompare("creationtime"))
			{
				m_Index_Creationtime = nParmPos;
			}
			else if (pField.ICompare("accesstime"))
			{
				m_Index_Accesstime = nParmPos;
			}
			else if (pField.ICompare("writetime"))
			{
				m_Index_Writetime = nParmPos;
			}
			else if (pField.ICompare("filesize"))
			{
				m_Index_Filesize = nParmPos;
			}
			else if (pField.ICompare("fileattribs"))
			{
				m_Index_Fileattribs = nParmPos;
			}
			else if (pField.ICompare("cfileattribs"))
			{
				m_Index_StringFileattribs = nParmPos;
			}
			else
			{
				SaveCustomError("AdirEx", "Invalid parameter name '%V' in callback parameter list", &pField);
				throw E_APIERROR;
			}

			pFields = pFields + (pFieldName.Len + 1);
			pFieldName = pFields.GetWordNum(1, ',');
			nParmPos++;
		}
		m_Callback.SetParameterPosition(0, m_Index_Filename);
		m_Callback.SetParameterPosition(1, m_Index_Dosfilename);
		m_Callback.SetParameterPosition(2, m_Index_Creationtime);
		m_Callback.SetParameterPosition(3, m_Index_Accesstime);
		m_Callback.SetParameterPosition(4, m_Index_Writetime);
		m_Callback.SetParameterPosition(5, m_Index_Filesize);
		m_Callback.SetParameterPosition(6, m_Index_Fileattribs);
		m_Callback.SetParameterPosition(7, m_Index_StringFileattribs);
	}
	else
	{
		m_Index_Fileattribs = bStringFileAttribs ? -1 : 6;
		m_Index_StringFileattribs = bStringFileAttribs ? 6 : -1;
		m_Callback.SetParameterPosition(6, m_Index_Fileattribs);
		m_Callback.SetParameterPosition(7, m_Index_StringFileattribs);
		
	}
	m_Callback.OptimizeParameterPosition();
}

bool FileSearchStorageCallback::Store(FileSearch* pFileSearch)
{
	ValueEx vRetVal;
	CStringView pStringFileAttributes;
	CStringView pFileName = pFileSearch->FileName();
	CStringView pAlternateFileName = pFileSearch->AlternateFileName();
	m_CreationTime = pFileSearch->CreationTime();
	m_AccessTime = pFileSearch->LastAccessTime();
	m_WriteTime = pFileSearch->LastWriteTime();
	if (m_ToLocalTime)
	{
		m_CreationTime.ToLocal();
		m_AccessTime.ToLocal();
		m_WriteTime.ToLocal();
	}
	m_FileSize = pFileSearch->FileSize();
	int nFileAttributes = pFileSearch->FileAttributes();
	if (m_Index_StringFileattribs >= 0)
		pStringFileAttributes = pFileSearch->StringFileAttributes();
	int nErrorNo = m_Callback.Evaluate(vRetVal, pFileName, pAlternateFileName, &m_CreationTime, &m_AccessTime, &m_WriteTime, &m_FileSize, nFileAttributes, pStringFileAttributes);
	if (nErrorNo)
		throw nErrorNo;
	if (vRetVal.Vartype() == 'L')
		return vRetVal.ev_length == 1;
	else
		vRetVal.Release();

	return true;
}

DWORD FileSearch::FindFirstFlags = 0xFFFFFFFF;

FileSearch::FileSearch(bool lRecurse = false, CStringView pSearchPath = 0, DWORD nFileFilter = 0, DWORD nFilterMatch = 0, CStringView pDestination = 0,
	int nDest = 0, bool btoLocalTime = false, bool bStringFileAttributes = false, int nMaxRecursion = 0, bool bDisableFsRedirection = false, CStringView pFields = 0)
{
	m_Recurse = lRecurse;
	m_FilterAttributes = nFileFilter;
	m_FilterMatch = nFilterMatch;
	m_FilterFakeDirectory = (nFileFilter & FILE_ATTRIBUTE_FAKEDIRECTORY) == 0; 
	m_FileCount = 0;
	m_Storage = 0;
	m_MaxRecursion = nMaxRecursion;
	m_DisableFsRedirection = bDisableFsRedirection;
	m_WSearch = false;
	m_Handle = INVALID_HANDLE_VALUE;
	m_Directory = pSearchPath;
	m_StrBuilder = m_Directory;
	m_StrBuilder.PathStripPath();
	m_Wildcard = (CStringView)m_StrBuilder;
	if (m_Directory.Len() == m_Wildcard.Len())
	{
		m_Directory.Len(GetCurrentDirectory(m_Directory.Size(), m_Directory));
		m_Directory.AddBs();
	}
	else
	{
		m_Directory.Len(m_Directory.Len() - m_Wildcard.Len(), true);
	}

	if (nDest & ADIREX_DEST_ARRAY)
	{
		m_Storage = new FileSearchStorageArray();
		m_Storage->Initialize(pDestination, btoLocalTime, bStringFileAttributes, pFields);
	}
	else if (nDest & ADIREX_DEST_CURSOR)
	{
		m_Storage = new FileSearchStorageCursor();
		m_Storage->Initialize(pDestination, btoLocalTime, bStringFileAttributes, pFields);
	}
	else if (nDest & ADIREX_DEST_CALLBACK)
	{
		m_Storage = new FileSearchStorageCallback();
		m_Storage->Initialize(pDestination, btoLocalTime, bStringFileAttributes, pFields);
	}
	
	m_FilterFunc = FileSearch::Filter_Default;
	if (nDest & ADIREX_FILTER_ALL)
	{
		m_FilterMatch = m_FilterAttributes;
	}
	else if (nDest & ADIREX_FILTER_NONE)
	{
		m_FilterMatch = 0;
	}
	else if (nDest & ADIREX_FILTER_EXACT)
	{
		m_FilterFunc = FileSearch::Filter_Exact;
		m_FilterMatch = m_FilterAttributes;
	}
	else
	{
		if (m_FilterAttributes != 0 && m_FilterAttributes == m_FilterMatch)
			m_FilterFunc = FileSearch::Filter_Any;
	}
	m_StoreFullPath = (nDest & ADIREX_FULLPATH) > 0;
}

FileSearch::~FileSearch()
{
	if (m_Storage)
		delete m_Storage;

	if (m_Handle != INVALID_HANDLE_VALUE)
		FindClose(m_Handle);
}

unsigned int FileSearch::ExecuteSearch()
{
	m_FileCount = 0;
	if (m_DisableFsRedirection)
		DisableFsRedirection();

	try
	{
		if (!FindFirst())
			return m_FileCount;
		
		do
		{
			if (m_FilterFunc(FileAttributes(), m_FilterAttributes, m_FilterMatch))
			{
				if (m_Recurse && !MatchFile())
					continue;

				m_FileCount++;
				if (m_Storage)
					m_Storage->Store(this);
			}
		} while (FindNext());
	}
	catch (int nErrorNo)
	{
		if (m_DisableFsRedirection)
			RevertFsRedirection();

		throw nErrorNo;
	}

	if (m_DisableFsRedirection)
		RevertFsRedirection();

	if (m_Storage)
		m_Storage->Finalize();

	return m_FileCount;
}

unsigned int FileSearch::ExecuteSearchCallback(FileSearchCallbackFunc pCallback, LPVOID pParam)
{
	m_FileCount = 0;
	if (m_DisableFsRedirection)
		DisableFsRedirection();

	try
	{
		if (!FindFirst())
			return m_FileCount;

		do
		{
			if (m_FilterFunc(FileAttributes(), m_FilterAttributes, m_FilterMatch))
			{
				if (m_Recurse && !MatchFile())
					continue;

				m_FileCount++;
				m_StrBuilder = FileName();
				(*pCallback)(m_StrBuilder, FileAttributes(), pParam);
			}
		} while (FindNext());
	}
	catch (int nErrorNo)
	{
		if (m_DisableFsRedirection)
			RevertFsRedirection();

		throw nErrorNo;
	}

	if (m_DisableFsRedirection)
		RevertFsRedirection();

	if (m_Storage)
		m_Storage->Finalize();

	return m_FileCount;
}

void FileSearch::ExecuteReverse(FileSearchCallbackFunc pCallback, LPVOID pParam)
{
	if (m_DisableFsRedirection)
		DisableFsRedirection();

	CUnboundBlockStack<FileSearchEntry, 1024 * 64>  pDirectories;
	CUnboundBlockStack<FileSearchEntry, 1024 * 64>  pFiles;
	FileSearchEntry pSearchEntry;

	try
	{
		if (!FindFirstReverse())
			return;

		do
		{
			pSearchEntry.Attributes = FileAttributes();
			pSearchEntry.Path = FileName();
			if (IsRealDirectory())
			{
				pDirectories.Enqueue(pSearchEntry);
			}
			else
			{
				pFiles.Enqueue(pSearchEntry);
			}
		} while (FindNextReverse());

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
		if (m_DisableFsRedirection)
			RevertFsRedirection();
		throw nErrorNo;
	}

	if (m_DisableFsRedirection)
		RevertFsRedirection();
}

bool FileSearch::FindFirst()
{
	if (m_Recurse)
		return FindFirstRecurse();

	if (m_StoreFullPath)
	{
		m_CompleteFilename = m_Directory;
		m_CompleteFilename.SetFormatBase();
	}

	m_SearchPattern = m_Directory;
	m_SearchPattern += m_Wildcard;
	if (m_SearchPattern.Len() < MAX_PATH)
	{
		m_CurrentFiledata = &m_Filedata;
		m_WSearch = false;
	try_again_FindFirstFileEx:
		m_Handle = FindFirstFileEx(m_SearchPattern, FindExInfoStandard, &m_Filedata, FindExSearchNameMatch, NULL, GetFindFirstFlags());
	}
	else
	{
		if (!m_WideWildcard)
			m_WideWildcard = m_Wildcard;
		m_WideSearchPattern = m_SearchPattern.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
		m_CurrentFiledata = reinterpret_cast<WIN32_FIND_DATA*>(&m_WideFiledata);
		m_WSearch = true;
	try_again_FindFirstFileExW:
		m_Handle = FindFirstFileExW(m_WideSearchPattern, FindExInfoStandard, &m_WideFiledata, FindExSearchNameMatch, NULL, GetFindFirstFlags());
	}

	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_INVALID_PARAMETER && GetFindFirstFlags() == FIND_FIRST_EX_LARGE_FETCH)
		{
			SetFindFirstFlags(0);
			if (m_WSearch == false)
				goto try_again_FindFirstFileEx;
			else
				goto try_again_FindFirstFileExW;
		}
		else if (IgnorableSearchError(nLastError, false))
			return false;
		else
		{
			SaveWin32Error("FindFirstFile", nLastError);
			throw E_APIERROR;
		}
	}

	if (m_FilterFakeDirectory && IsFakeDir())
		return FindNext();

	return true;
}

bool FileSearch::FindFirstRecurse()
{
	if (m_SubDirectory.Len() == 0)
	{
		if (m_StoreFullPath)
		{
			m_CompleteFilename = m_Directory;
			m_CompleteFilename.SetFormatBase();
		}
		else
		{
			m_StrBuilder = m_Directory;
			m_StrBuilder.SetFormatBase();
		}
		m_SearchPattern = m_Directory;
		m_RecursionLevel = 0;
	}
	else
	{
		m_SearchPattern = m_SubDirectory;
		CStringView pSubdir = m_SubDirectory;
		pSubdir = pSubdir + m_Directory.Len();
		m_RecursionLevel = pSubdir.GetWordCount('\\') - 1;
		if (m_StoreFullPath)
			m_CompleteFilename = m_SubDirectory;
		else
			m_CompleteFilename = pSubdir;
		m_CompleteFilename.SetFormatBase();
	}
	m_SearchPattern += "*";
	if (m_SearchPattern.Len() < MAX_PATH)
	{
		m_CurrentFiledata = &m_Filedata;
		m_WSearch = false;
try_again_FindFirstFileEx:
		m_Handle = FindFirstFileEx(m_SearchPattern, FindExInfoStandard, &m_Filedata, FindExSearchNameMatch, NULL, GetFindFirstFlags());
	}
	else
	{
		if (!m_WideWildcard)
			m_WideWildcard = m_Wildcard;
		m_WideSearchPattern = m_SearchPattern.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
		m_CurrentFiledata = reinterpret_cast<WIN32_FIND_DATA*>(&m_WideFiledata);
		m_WSearch = true;
try_again_FindFirstFileExW:
		m_Handle = FindFirstFileExW(m_WideSearchPattern, FindExInfoStandard, &m_WideFiledata, FindExSearchNameMatch, NULL, GetFindFirstFlags());
	}

	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_INVALID_PARAMETER && GetFindFirstFlags() == FIND_FIRST_EX_LARGE_FETCH)
		{
			SetFindFirstFlags(0);
			if (m_WSearch == false)
				goto try_again_FindFirstFileEx;
			else
				goto try_again_FindFirstFileExW;
		}

		if (IgnorableSearchError(nLastError, m_RecursionLevel > 0))
		{
			// Did we access something we can't access while recursing?
			CStringView pSubdir;
			if (m_Subdirectories.Dequeue(pSubdir))
			{
				m_SubDirectory = pSubdir;
				return FindFirstRecurse();
			}
			return false;
		}
		else
		{
			SaveWin32Error("FindFirstFile", nLastError);
			throw E_APIERROR;
		}
	}

	if (m_FilterFakeDirectory && IsFakeDir())
		return FindNextRecurse();

	// add subdirectory to queue
	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && IsRealDirectory())
	{
		if (m_StoreFullPath)
			m_StrBuilder = FileName();
		else
			m_StrBuilder.AppendFromBase(FileName());
		m_StrBuilder.AddBs();
		CStringView pSubdir = m_StrBuilder;
		m_Subdirectories.Enqueue(pSubdir);
	}

	return true;
}

bool FileSearch::FindNext()
{
	if (m_Recurse)
		return FindNextRecurse();

	BOOL	bNext;
	while (true)
	{
		if (m_WSearch == false)
			bNext = FindNextFile(m_Handle, &m_Filedata);
		else
			bNext = FindNextFileW(m_Handle, &m_WideFiledata);
		
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
		if (m_FilterFakeDirectory && IsFakeDir())
			continue;
		return true;
	}
}

bool FileSearch::FindNextRecurse()
{
	BOOL	bNext;

find_next:
	if (m_WSearch == false)
		bNext = FindNextFile(m_Handle, &m_Filedata);
	else
		bNext = FindNextFileW(m_Handle, &m_WideFiledata);

	if (bNext == FALSE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_NO_MORE_FILES)
		{
			FindClose(m_Handle);
			m_Handle = INVALID_HANDLE_VALUE;
			CStringView pSubdir;
			if (m_Subdirectories.Dequeue(pSubdir))
			{
				m_SubDirectory = pSubdir;
				goto find_first;
			}
			return false;
		}
		else
		{
			SaveWin32Error("FindNextFile", nLastError);
			throw E_APIERROR;
		}
	}

	if (m_FilterFakeDirectory && IsFakeDir())
		goto find_next;

	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && IsRealDirectory())
	{
		if (m_StoreFullPath)
			m_StrBuilder = FileName();
		else
			m_StrBuilder.AppendFromBase(FileName());
		m_StrBuilder.AddBs();
		CStringView pSubdir = m_StrBuilder;
		m_Subdirectories.Enqueue(pSubdir);
	}
	return true;

find_first:
	if (m_SubDirectory.Len() == 0)
	{
		if (m_StoreFullPath)
		{
			m_CompleteFilename = m_Directory;
			m_CompleteFilename.SetFormatBase();
		}
		else
		{
			m_StrBuilder = m_Directory;
			m_StrBuilder.SetFormatBase();
		}
		m_SearchPattern = m_Directory;
		m_RecursionLevel = 0;
	}
	else
	{
		m_SearchPattern = m_SubDirectory;
		CStringView pSubdir = m_SubDirectory;
		pSubdir = pSubdir + m_Directory.Len();
		m_RecursionLevel = pSubdir.GetWordCount('\\') - 1;
		if (m_StoreFullPath)
			m_CompleteFilename = m_SubDirectory;
		else
			m_CompleteFilename = pSubdir;
		m_CompleteFilename.SetFormatBase();
	}
	m_SearchPattern += "*";
	if (m_SearchPattern.Len() < MAX_PATH)
	{
		m_CurrentFiledata = &m_Filedata;
		m_WSearch = false;
	try_again_FindFirstFileEx:
		m_Handle = FindFirstFileEx(m_SearchPattern, FindExInfoStandard, &m_Filedata, FindExSearchNameMatch, NULL, GetFindFirstFlags());
	}
	else
	{
		if (!m_WideWildcard)
			m_WideWildcard = m_Wildcard;
		m_WideSearchPattern = m_SearchPattern.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
		m_CurrentFiledata = reinterpret_cast<WIN32_FIND_DATA*>(&m_WideFiledata);
		m_WSearch = true;
	try_again_FindFirstFileExW:
		m_Handle = FindFirstFileExW(m_WideSearchPattern, FindExInfoStandard, &m_WideFiledata, FindExSearchNameMatch, NULL, GetFindFirstFlags());
	}

	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_INVALID_PARAMETER && GetFindFirstFlags() == FIND_FIRST_EX_LARGE_FETCH)
		{
			SetFindFirstFlags(0);
			if (m_WSearch == false)
				goto try_again_FindFirstFileEx;
			else
				goto try_again_FindFirstFileExW;
		}

		if (IgnorableSearchError(nLastError, m_RecursionLevel > 0))
		{
			// Did we access something we can't access while recursing?
			CStringView pSubdir;
			if (m_Subdirectories.Dequeue(pSubdir))
			{
				m_SubDirectory = pSubdir;
				goto find_first;
			}
			return false;
		}
		else
		{
			SaveWin32Error("FindFirstFile", nLastError);
			throw E_APIERROR;
		}
	}

	if (m_FilterFakeDirectory && IsFakeDir())
		goto find_next;

	// add subdirectory to queue
	if ((m_MaxRecursion == 0 || m_RecursionLevel < m_MaxRecursion) && IsRealDirectory())
	{
		if (m_StoreFullPath)
			m_StrBuilder = FileName();
		else
			m_StrBuilder.AppendFromBase(FileName());
		m_StrBuilder.AddBs();
		CStringView pSubdir = m_StrBuilder;
		m_Subdirectories.Enqueue(pSubdir);
	}
	return true;
}

/* like FindFirst, but drills through all subdirectories and returns the files in the deepest directories first */
bool FileSearch::FindFirstReverse()
{
	if (m_SubDirectory.Len() == 0)
	{
		if (m_StoreFullPath)
		{
			m_CompleteFilename = m_Directory;
			m_CompleteFilename.SetFormatBase();
		}
		else
		{
			m_StrBuilder = m_Directory;
			m_StrBuilder.SetFormatBase();
		}
		m_SearchPattern = m_Directory;
	}
	else
	{
		m_SearchPattern = m_SubDirectory;
		if (m_StoreFullPath)
			m_CompleteFilename = m_SubDirectory;
		else
		{
			CStringView pSubdir = m_SubDirectory;
			pSubdir = pSubdir + m_Directory.Len();
			m_CompleteFilename = pSubdir;
		}
		m_CompleteFilename.SetFormatBase();
	}
	m_SearchPattern += "*";

	if (m_SearchPattern.Len() < MAX_PATH)
	{
		m_CurrentFiledata = &m_Filedata;
		m_WSearch = false;
	try_again_FindFirstFileEx:
		m_Handle = FindFirstFileEx(m_SearchPattern, FindExInfoStandard, &m_Filedata, FindExSearchNameMatch, NULL, GetFindFirstFlags());
	}
	else
	{
		if (!m_WideWildcard)
			m_WideWildcard = m_Wildcard;
		m_WideSearchPattern = m_SearchPattern.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
		m_CurrentFiledata = reinterpret_cast<WIN32_FIND_DATA*>(&m_WideFiledata);
		m_WSearch = true;
	try_again_FindFirstFileExW:
		m_Handle = FindFirstFileExW(m_WideSearchPattern, FindExInfoStandard, &m_WideFiledata, FindExSearchNameMatch, NULL, GetFindFirstFlags());
	}

	if (m_Handle == INVALID_HANDLE_VALUE)
	{
		DWORD nLastError = GetLastError();
		if (nLastError == ERROR_INVALID_PARAMETER && GetFindFirstFlags() == FIND_FIRST_EX_LARGE_FETCH)
		{
			SetFindFirstFlags(0);
			if (m_WSearch == false)
				goto try_again_FindFirstFileEx;
			else
				goto try_again_FindFirstFileExW;
		}
		else
		{
			SaveWin32Error("FindFirstFile", nLastError);
			throw E_APIERROR;
		}
	}

	if (m_FilterFakeDirectory && IsFakeDir())
		return FindNextReverse();

	// add subdirectory to queque
	if (IsRealDirectory())
	{
		if (m_StoreFullPath)
			m_StrBuilder = FileName();
		else
			m_StrBuilder.AppendFromBase(FileName());
		m_StrBuilder.AddBs();
		CStringView pSubdir = m_StrBuilder;
		m_Subdirectories.Enqueue(pSubdir);
	}
	return true;
}

bool FileSearch::FindNextReverse()
{
	BOOL	bNext;
	while (true)
	{
		if (m_WSearch == false)
			bNext = FindNextFile(m_Handle, &m_Filedata);
		else
			bNext = FindNextFileW(m_Handle, &m_WideFiledata);

		if (bNext == FALSE)
		{
			DWORD nLastError = GetLastError();
			if (nLastError == ERROR_NO_MORE_FILES)
			{
				FindClose(m_Handle);
				m_Handle = INVALID_HANDLE_VALUE;
				CStringView pSubdir;
				if (m_Subdirectories.Dequeue(pSubdir))
				{
					m_SubDirectory = pSubdir;
					return FindFirstReverse();
				}
				return false;
			}
			else
			{
				SaveWin32Error("FindNextFile", nLastError);
				throw E_APIERROR;
			}
		}

		if (IsFakeDir())
			return FindNextReverse();

		if (IsRealDirectory())
		{
			if (m_StoreFullPath)
				m_StrBuilder = FileName();
			else
				m_StrBuilder.AppendFromBase(FileName());

			m_StrBuilder.AddBs();
			CStringView pSubdir = m_StrBuilder;
			m_Subdirectories.Enqueue(pSubdir);
		}
		return true;
	}
}

bool FileSearch::Filter_Default(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch)
{
	return (nAttributes & nFilter) == nFilterMatch;
}

bool FileSearch::Filter_Exact(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch)
{
	return nAttributes == nFilter;
}

bool FileSearch::Filter_Any(DWORD nAttributes, DWORD nFilter, DWORD nFilterMatch)
{
	return (nAttributes & nFilter) > 0;
}

void FileSearch::DisableFsRedirection()
{
	if (fpWow64DisableWow64FsRedirection == (PWOW64FSREDIRECTION)-1)
		return;

	if (fpWow64DisableWow64FsRedirection == 0)
	{
		HMODULE hDll = GetModuleHandle("kernel32.dll");
		if (hDll)
		{
			fpWow64DisableWow64FsRedirection = (PWOW64FSREDIRECTION)GetProcAddress(hDll, "Wow64DisableWow64FsRedirection");
			if (fpWow64DisableWow64FsRedirection == 0)
			{
				fpWow64DisableWow64FsRedirection = (PWOW64FSREDIRECTION)-1;
			}
			else
			{
				fpWow64RevertWow64FsRedirection = (PWOW64FSREDIRECTION)GetProcAddress(hDll, "Wow64RevertWow64FsRedirection");
				if (fpWow64RevertWow64FsRedirection == 0)
				{
					fpWow64DisableWow64FsRedirection = (PWOW64FSREDIRECTION)-1;
					fpWow64RevertWow64FsRedirection = (PWOW64FSREDIRECTION)-1;
				}
			}
		}
	}

	PVOID oldValue;
	if (fpWow64DisableWow64FsRedirection != (PWOW64FSREDIRECTION)-1)
		fpWow64DisableWow64FsRedirection(&oldValue);
}

void FileSearch::RevertFsRedirection()
{
	if (fpWow64RevertWow64FsRedirection == (PWOW64FSREDIRECTION)-1)
		return;

	PVOID oldValue;
	fpWow64RevertWow64FsRedirection(&oldValue);
}

DWORD FileSearch::GetFindFirstFlags()
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

void FileSearch::SetFindFirstFlags(DWORD dwFlags)
{
	FindFirstFlags = dwFlags;
}

VolumeSearch::~VolumeSearch()
{ 
	if (m_Handle != INVALID_HANDLE_VALUE)
		FindVolumeClose(m_Handle);
}

bool VolumeSearch::FindFirst()
{
	m_Handle = FindFirstVolume(m_Volume, m_Volume.Size());
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
	BOOL bNext = FindNextVolume(m_Handle, m_Volume, m_Volume.Size());
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
	m_Handle = FindFirstVolumeMountPoint(pVolume, m_MountPoint, m_MountPoint.Size());
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
	BOOL bNext = FindNextVolumeMountPoint(m_Handle, m_MountPoint, m_MountPoint.Size());
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

// cleanup
void _stdcall VFP2C_Destroy_File(VFP2CTls& tls)
{
	int nCount = tls.FileHandles.GetCount();
	for (int xj = 0; xj < nCount; xj++)
	{
		// release all file handles
		CloseHandle(tls.FileHandles[xj]);
	}
	tls.FileHandles.SetIndex(-1);
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
		}
		bNegate = false;
	}
	return nFileFilter;
}

void _fastcall ADirEx(ParamBlkEx& parm)
{
	FileSearch* pFileSearch = 0;
try
{
	FoxString pDestination(parm(1));
	FoxString pSearchString(parm(2));
	FoxString pFields(parm, 6);
	DWORD nFileFilter, nFileMatch;
	nFileFilter = nFileMatch = ~FILE_ATTRIBUTE_FAKEDIRECTORY;
	if (parm.PCount() >= 3)
	{
		if (parm(3)->Vartype() == 'I')
		{
			if (parm(3)->ev_long)
			{
				nFileFilter = parm(3)->ev_long;
				nFileMatch = parm.PCount() >= 7 ? parm(7)->ev_long : nFileFilter;
			}
		}
		else if (parm(3)->Vartype() == 'C')
		{
			if (parm(3)->Len() > 0)
			{
				FoxString pFileFilter(parm(3), 0);
				nFileFilter = AdirEx_FileFilter(pFileFilter, nFileMatch);
			}
		}
		else
			throw E_INVALIDPARAMS;
	}

	int nDest = parm.PCount() >= 4 && parm(4)->ev_long ? parm(4)->ev_long : ADIREX_DEST_ARRAY;
	int nMaxRecursion = parm.PCount() >= 5 ? parm(5)->ev_long : 0;

	bool bToLocalTime = (nDest & ADIREX_UTC_TIMES) == 0;
	nDest &= ~ADIREX_UTC_TIMES;
	bool llRecurse = (nDest & ADIREX_RECURSIVE) > 0;
	nDest &= ~ADIREX_RECURSIVE;
	bool bFsRedirection = (nDest & ADIREX_DISABLE_FSREDIRECTION) > 0;
	nDest &= ~ADIREX_DISABLE_FSREDIRECTION;
	bool bStringFileAttributes = (nDest & ADIREX_STRING_FILEATTRIBUTES) > 0;
	nDest &= ~ADIREX_STRING_FILEATTRIBUTES;

	if (!(nDest & (ADIREX_DEST_ARRAY | ADIREX_DEST_CURSOR | ADIREX_DEST_CALLBACK)))
		nDest |= ADIREX_DEST_ARRAY;

	if ((nDest & ADIREX_DEST_CALLBACK) && (pDestination.Len() > VFP2C_MAX_CALLBACKFUNCTION || pDestination.Len() == 0))
	{
		SaveCustomError("ADirEx", "Callback function length is zero or greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}
	
	pFileSearch = new FileSearch(llRecurse, pSearchString, nFileFilter, nFileMatch, pDestination, nDest, bToLocalTime, bStringFileAttributes, nMaxRecursion, bFsRedirection, pFields);

	unsigned int nFileCount = pFileSearch->ExecuteSearch();
	Return(nFileCount);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
if (pFileSearch)
	delete pFileSearch;
}

void _fastcall ADirectoryInfo(ParamBlkEx& parm)
{
	FileSearch* pFileSearch = 0;
try
{
	FoxArray pArray(parm(1),3,1);
	FoxString pDirectory(parm(2));
	FoxInt64 pFileSize;
	int nDirectoryCount = 0;
	int nFileCount = 0;
	double nDirectorySize = 0.0;

	pDirectory.AddBsWildcard();
	if (pDirectory.Len() >= MAX_PATH)
		pDirectory.PrependIfNotPresent(FILE_UNICODE_EXTENSION);

	pFileSearch = new FileSearch(true, pDirectory);
	if (pFileSearch == 0)
		throw E_INSUFMEMORY;

	if (pFileSearch->FindFirst())
	{
		do
		{
			if (pFileSearch->IsRealDirectory())
			{
				nDirectoryCount++;
			}
			else if (!pFileSearch->IsFakeDir())
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
	FoxString pFileName(parm(2));
	bool bToLocal = parm.PCount() == 2 || !parm(3)->ev_length;
	bool bStringFileAttributes = parm.PCount() == 4 && parm(4)->ev_length;
	FoxDateTime pFileTime;
	FoxInt64 pFileSize;
	WIN32_FILE_ATTRIBUTE_DATA sFileAttribs;
	BOOL bRet;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		bRet = GetFileAttributesEx(pFileName, GetFileExInfoStandard, &sFileAttribs);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename = pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
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
	FoxString pFileName(parm(2));
	bool bToLocal = parm.PCount() == 2 || !parm(3)->ev_length;
	bool bStringFileAttributes = parm.PCount() == 4 && parm(4)->ev_length;
	FoxDateTime pFileTime;
	FoxInt64 pFileSize;
	ApiHandle hFile;
	BY_HANDLE_FILE_INFORMATION sFileAttribs;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		hFile = CreateFile(pFileName, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename = pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
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
	if (parm(2)->Vartype() != 'R' && parm(2)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	if (parm.PCount() >= 3 && parm(3)->Vartype() != 'R' && parm(3)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	if (parm.PCount() >= 4 && parm(4)->Vartype() != 'R' && parm(4)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	FoxString pFileName(parm(1));
	bool bToLocal = parm.PCount() < 5 || !parm(5)->ev_length;
	FoxDateTime pFileTime;
	ApiHandle hFile;

	FILETIME sCreationTime, sAccessTime, sWriteTime;
	
	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		hFile = CreateFile(pFileName, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename = pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
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
	FoxString pFileName(parm(1));
	ApiHandle hFile;
	FoxDateTime pTime;
	bool bCreation, bAccess, bWrite;
	bool bToUTC = parm.PCount() < 5 || parm(5)->ev_length;

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
		hFile = CreateFile(pFileName, FILE_WRITE_ATTRIBUTES, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename = pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
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

void _fastcall GetFileSizeLib(ParamBlkEx& parm)
{
try
{
	FoxString pFileName(parm(1));
	FoxInt64 pFileSize;
	ApiHandle hFile;
	LARGE_INTEGER sSize;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		hFile = CreateFile(pFileName, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename = pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
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
	pFileSize.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall GetFileAttributesLib(ParamBlkEx& parm)
{
try
{
	FoxString pFileName(parm(1));
	bool bStringFileAttributes = parm.PCount() == 2 && parm(2)->ev_length;
	DWORD nAttribs;
    
	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		nAttribs = GetFileAttributes(pFileName);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename = pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
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

void _fastcall SetFileAttributesLib(ParamBlkEx& parm)
{
try
{
	FoxString pFileName(parm(1));
	FoxWString<MAX_PATH * 2> pWideFilename;
	DWORD dwFileAttributes, dwFileAttributesSet, dwFileAttributesClear;
	bool bClearOrSet;
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
		bClearOrSet = true;
		if (parm(2)->Vartype() == 'I')
		{
			dwFileAttributesSet = parm(2)->ev_long;
			dwFileAttributesClear = parm(3)->ev_long;
		}
		else
			throw E_INVALIDPARAMS;
	}

	BOOL bRet;
	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() >= MAX_PATH)
		pWideFilename = pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
	
	if (bClearOrSet)
	{
		if (pFileName.Len() < MAX_PATH)
			dwFileAttributes = GetFileAttributes(pFileName);
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
		bRet = SetFileAttributes(pFileName, dwFileAttributes);
	else
		bRet = SetFileAttributesW(pWideFilename, dwFileAttributes);
	
	if (!bRet)
	{
		SaveWin32Error("SetFileAttributes", GetLastError());
		throw E_APIERROR;
	}
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
		pWidePath = pPath.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
		bRet = SetFileAttributesW(pWidePath, nFileAttributes);
	}

	if (!bRet)
	{
		SaveWin32Error("SetFileAttributes", GetLastError());
		throw E_APIERROR;
	}
}

void _fastcall SetFileAttributesLibEx(ParamBlkEx& parm)
{
FileSearch* pFileSearch = 0;
try
{
	FoxString pWildcard(parm(1));
	SETFILEATTRIBUTEPARAM pCallbackParam;
	if (parm(2)->Vartype() == 'I')
	{
		pCallbackParam.bClearOrSet = false;
		pCallbackParam.nFileAttributesSet = parm(2)->ev_long;
	}
	else if (parm(2)->Vartype() == 'C')
	{
		FoxString pAttributes(parm(2), 0);
		pCallbackParam.bClearOrSet = pAttributes.StringToFileAttributes(pCallbackParam.nFileAttributesSet, pCallbackParam.nFileAttributesClear);
	}
	else
		throw E_INVALIDPARAMS;
	
	int nMaxRecursion = parm.PCount() < 3 ? -1 : parm(3)->ev_long;
	bool bRecurse = nMaxRecursion >= 0;
	pFileSearch = new FileSearch(bRecurse, pWildcard, 0, 0, 0, ADIREX_FULLPATH, false, false, nMaxRecursion);
	if (pFileSearch == 0)
		throw E_INSUFMEMORY;

	unsigned int nFileCount = pFileSearch->ExecuteSearchCallback(SetFileAttributesExCallback, (LPVOID)&pCallbackParam);
	Return(nFileCount);
}
catch (int nErrorNo)
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
	FoxString pFileName(parm(1));
	FoxString pOwner(MAX_PATH);
	FoxString pDomain(MAX_PATH);
	CBuffer pDescBuffer;
	ApiHandle hFile;
	int SidType;
	
	DWORD dwSize = SECURITY_DESCRIPTOR_LEN, nLastError;
	BOOL bOwnerDefaulted;
	PSECURITY_DESCRIPTOR pSecDesc;
	PSID pOwnerId;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		hFile = CreateFile(pFileName, READ_CONTROL, 0, 0, OPEN_EXISTING, 0, 0);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		pWideFilename = pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
		hFile = CreateFileW(pWideFilename, READ_CONTROL, 0, 0, OPEN_EXISTING, 0, 0);
	}
	
	if (!hFile)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	while(1)
	{
		pDescBuffer.Size(dwSize);
		pSecDesc = reinterpret_cast<PSECURITY_DESCRIPTOR>(pDescBuffer.Address());

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
	if (!LookupAccountSid((LPCSTR)0,pOwnerId,pOwner,&dwOwnerLen,
		pDomain,&dwDomainLen,(PSID_NAME_USE)&SidType))	
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

void _fastcall GetLongPathNameLib(ParamBlkEx& parm)
{
try
{
	FoxString pFileName(parm(1));
	FoxString pPathName(MAX_PATH);
	DWORD nLen = 0;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		nLen = GetLongPathName(pFileName, pPathName, MAX_PATH);
		pPathName.Len(nLen);
	}

	if (nLen > MAX_PATH)
	{
		FoxWString<MAX_PATH*2> pWideFilename;
		FoxWString<0> pWidePathName;
		pWideFilename = pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
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

void _fastcall GetShortPathNameLib(ParamBlkEx& parm)
{
try
{
	FoxString pFileName(parm(1));
	FoxString pPathName(MAX_PATH);
	DWORD nLen;

	if (pFileName.Len() < MAX_PATH)
		pFileName.Fullpath();

	if (pFileName.Len() < MAX_PATH)
	{
		nLen = GetShortPathName(pFileName, pPathName, MAX_PATH);
		pPathName.Len(nLen);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideFilename;
		FoxWString<0> pWidePathName;
		pWideFilename = pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
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

void _fastcall CopyFileExLib(ParamBlkEx& parm)
{
try
{
	FoxString pSource(parm(1));
	FoxString pDest(parm(2));
	FoxString pCallback(parm,3);
	DWORD dwCopyFlags = parm.PCount() >= 4 ? parm(4)->ev_long : (COPY_FILE_NO_BUFFERING);
	DWORD dwCallbackLimiter = parm.PCount() == 5 ? parm(5)->ev_long : 100;
	BOOL bRetVal, bCanceled = 0;
	FILEPROGRESSPARAM sProgress;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
	{
		SaveCustomError("ADirEx", "Callback function length is greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}

	bool bCallback = pCallback.Len() > 0;
	if (bCallback)
	{
		sProgress.pCallback.SetCallback(pCallback);
		sProgress.nCallbackLimiter.QuadPart = dwCallbackLimiter;
	}

	if (pSource.Len() >= MAX_PATH)
		pSource.PrependIfNotPresent(FILE_UNICODE_EXTENSION);

	if (pDest.Len() >= MAX_PATH)
		pDest.PrependIfNotPresent(FILE_UNICODE_EXTENSION);

	if (pSource.Len() < MAX_PATH && pDest.Len() < MAX_PATH)
	{
		bRetVal = CopyFileEx(pSource, pDest, bCallback ? FileProgressCallback : 0, bCallback ? &sProgress : 0, &bCanceled, dwCopyFlags);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideSource;
		FoxWString<MAX_PATH * 2> pWideDest;
		pWideSource = pSource;
		pWideDest = pDest;
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
 	FoxString pSource(parm(1));
 	FoxString pDest(parm(2));
	FoxString pCallback(parm,3);
	DWORD dwMoveFlags = parm.PCount() >= 4 ? parm(4)->ev_long : (MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
	DWORD dwCallbackLimiter = parm.PCount() == 5 ? parm(5)->ev_long : 100;
	BOOL bRetVal;
	FILEPROGRESSPARAM sProgress;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
	{
		SaveCustomError("MoveFileEx", "Callback function length is greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}

	bool bCallback = pCallback.Len() > 0;

	// builds the format command to be called back
	if (bCallback)
	{
		sProgress.pCallback.SetCallback(pCallback);
		sProgress.nCallbackLimiter.QuadPart = dwCallbackLimiter;
	}

	if (pSource.Len() >= MAX_PATH)
		pSource.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
	
	if (pDest.Len() >= MAX_PATH)
		pDest.PrependIfNotPresent(FILE_UNICODE_EXTENSION);

	if (pSource.Len() < MAX_PATH && pDest.Len() < MAX_PATH)
	{
		bRetVal = MoveFileWithProgress(pSource, pDest, bCallback ? FileProgressCallback : 0, bCallback ? &sProgress : 0, dwMoveFlags);
	}
	else
	{
		FoxWString<MAX_PATH * 2> pWideSource;
		FoxWString<MAX_PATH * 2> pWideDest;
		pWideSource = pSource;
		pWideDest = pDest;
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
	else if (vRetVal.Vartype() == 'I')
		nStatus = vRetVal.ev_long;
	else if (vRetVal.Vartype() == 'N')
		nStatus = static_cast<DWORD>(vRetVal.ev_real);
	else
	{
		vRetVal.Release();
		nStatus = PROGRESS_CANCEL;
	}
}

DWORD _stdcall FileProgressCallback(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred, LARGE_INTEGER StreamSize, LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber,
	DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData)
{
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
	FoxString pFile1(parm(1));
	FoxString pFile2(parm(2));
	ApiHandle hSource, hDest;
	LARGE_INTEGER sSourceTime, sDestTime;
	FoxWString<MAX_PATH> pWideFile;

	pFile1.Fullpath();
	pFile2.Fullpath();

	if (pFile1.Len() < MAX_PATH)
		hSource = CreateFile(pFile1, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	else
	{
		pFile1.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
		pWideFile = pFile1;
		hSource = CreateFileW(pWideFile, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}

	if (!hSource)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

	if (pFile2.Len() < MAX_PATH)
		hDest = CreateFile(pFile2, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	else
	{
		pFile2.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
		pWideFile = pFile2;
		hDest = CreateFileW(pWideFile, 0, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	}

	if (!hDest)
	{
		SaveWin32Error("CreateFile", GetLastError());
		throw E_APIERROR;
	}

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
	FoxString pFileName(parm(1));

	if (pFileName.Len() < MAX_PATH)
	{
		pFileName.Fullpath();
		DeleteFileExImpl(pFileName, INVALID_FILE_ATTRIBUTES);
	}
	else
	{
		pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
		FoxWString<MAX_PATH*2> pWideFileName;
		pWideFileName = pFileName;
		DeleteFileExImpl(pWideFileName, INVALID_FILE_ATTRIBUTES);
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _stdcall FileSearchDeleteCallback(CStrBuilder<MAX_WIDE_PATH>& pPath, DWORD nAttributes, LPVOID pParam)
{
	if (nAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		if (pPath.Len() < MAX_PATH)
		{
			RemoveDirectoryEx(pPath);
		}
		else
		{
			FoxWString<MAX_PATH * 2> pWidePath;
			pWidePath = pPath.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
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
			pWidePath = pPath.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
			DeleteFileExImpl(pWidePath, nAttributes);
		}
	}
}

void _fastcall DeleteDirectory(ParamBlkEx& parm)
{
	FileSearch* pFileSearch = 0;
try
{
	FoxString pDirectory(parm(1));
	FoxString pSearch(pDirectory);
	pSearch.AddBsWildcard();
	pFileSearch = new FileSearch(true, pSearch, 0, 0, 0, ADIREX_FULLPATH);
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
		pWideDirectory = pDirectory.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
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

void _fastcall GetOpenFileNameLib(ParamBlkEx& parm)
{
try
{
	FoxString pFilter(parm,2,2);
	FoxString pFileName(parm,3);
	FoxString pInitialDir(parm,4);
	FoxString pTitle(parm,5);
	FoxArray pArray(parm,7);
	FoxString pCallback(parm,8);
	FoxString pFiles(MAX_OPENFILENAME_BUFFER);
	FoxString pFileBuffer;

	OPENFILENAME sOpenFilename = {0};
	OpenfileCallback sCallbackParam;

	if (pFileName.Len() > MAX_OPENFILENAME_BUFFER)
		throw E_INVALIDPARAMS;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
	{
		SaveCustomError("GetOpenFileName", "Callback function length is greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}

	if (parm.PCount() >= 1 && parm(1)->ev_long)
		sOpenFilename.Flags = parm(1)->ev_long & ~(OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE | OFN_ALLOWMULTISELECT);
	else
		sOpenFilename.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_NODEREFERENCELINKS |
			OFN_FILEMUSTEXIST |	OFN_DONTADDTORECENT;

	if (parm.PCount() >= 6)
		sOpenFilename.FlagsEx = parm(6)->ev_long;

	sOpenFilename.lStructSize = sizeof(OPENFILENAME);
	sOpenFilename.Flags |= OFN_ENABLEHOOK;
	sOpenFilename.lpfnHook = &GetFileNameCallback;

	sOpenFilename.hwndOwner = WTopHwnd();

	if (pFileName.Len())
		pFiles = pFileName;
	else
		pFiles[0] = '\0';

	sOpenFilename.lpstrFile = pFiles;
	sOpenFilename.nMaxFile = pFiles.Size();

	if (pFilter.Len())
		sOpenFilename.lpstrFilter = pFilter;
	else if (!(sOpenFilename.Flags & OFN_EXPLORER))
		sOpenFilename.lpstrFilter = "All\0*.*\0";

	sOpenFilename.lpstrInitialDir = pInitialDir;
	sOpenFilename.lpstrTitle = pTitle;

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
			int nFileCount;
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

void _fastcall GetSaveFileNameLib(ParamBlkEx& parm)
{
try
{
	FoxString pFilter(parm,2,2);
	FoxString pFileName(parm,3);
	FoxString pInitialDir(parm,4);
	FoxString pTitle(parm,5);
	FoxString pCallback(parm,7);
	FoxString pFiles(MAX_OPENFILENAME_BUFFER);

	OPENFILENAME sFile = {0};
	OpenfileCallback sCallbackParam;

	if (pFileName.Len() > MAX_OPENFILENAME_BUFFER)
		throw E_INVALIDPARAMS;

	if (pCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION)
	{
		SaveCustomError("GetSaveFileName", "Callback function length is greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}

	if (parm.PCount() >= 1 && parm(1)->ev_long)
		sFile.Flags = parm(1)->ev_long & ~(OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE);
	else
		sFile.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR;

	if (parm.PCount() >= 6)
		sFile.FlagsEx = parm(6)->ev_long;

	sFile.lStructSize = sizeof(OPENFILENAME);
	// set callback procedure 
	sFile.Flags |= OFN_ENABLEHOOK;
	sFile.lpfnHook = &GetFileNameCallback;

	sFile.hwndOwner = WTopHwnd();

	pFiles[0] = '\0';
	sFile.lpstrFile = pFiles;
	sFile.nMaxFile = MAX_OPENFILENAME_BUFFER;

	if (pFilter.Len())
		sFile.lpstrFilter = pFilter;
	else if (!(sFile.Flags & OFN_EXPLORER))
		sFile.lpstrFilter = "All\0*.*\0";

	if (pFileName.Len())
		pFiles = pFileName;
		// strcpy(sFile.lpstrFile,pFileName);
	if (pInitialDir.Len())
		sFile.lpstrInitialDir = pInitialDir;
	if (pTitle.Len())
		sFile.lpstrTitle = pTitle;

	if (pCallback.Len())
	{
		sFile.lCustData = (LPARAM)&sCallbackParam;
		sCallbackParam.pCallback.SetCallback(pCallback);
	}

	if (GetSaveFileName(&sFile)) {
		pFiles.StringLen();
		if (pFilter.Len() && sFile.nFilterIndex) {
			char *pSelectedFilter = pFilter;
			for (unsigned int xj = 1; xj < sFile.nFilterIndex * 2; xj++)
				pSelectedFilter += strlen(pSelectedFilter) + 1;

			FoxString pExtension(pSelectedFilter);
			pExtension.SubStr(pExtension.Rat('.'));
			if (!pExtension.ICompare(".*")) {
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
				if (lpCallback->vRetVal.Vartype() == 'I')
					return lpCallback->vRetVal.ev_long;
				else if (lpCallback->vRetVal.Vartype() == 'N')
					return static_cast<UINT_PTR>(lpCallback->vRetVal.ev_real);
				else if (lpCallback->vRetVal.Vartype() == 'L')
					return lpCallback->vRetVal.ev_length;
				else
				{
					lpCallback->vRetVal.Release();
					lpCallback->vRetVal = 0;
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
	FoxString pDrive("X:\\");
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
			pArray(nRow,2) = GetDriveType(pDrive);

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
	FoxArray pArray(parm(1), 1);
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
	FoxArray pArray(parm(1), 1);
	FoxString pVolume(parm(2));
	VolumeMountPointSearch pSearch;

	if (pSearch.FindFirst(pVolume))
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
	{
		HMODULE hDll;
		hDll = GetModuleHandle("kernel32.dll");
		if (hDll)
		{
			fpGetVolumePathNamesForVolumeName = (PGETVOLUMEPATHNAMESFORVOLUMENAME)GetProcAddress(hDll, "GetVolumePathNamesForVolumeNameA");
			if (!fpGetVolumePathNamesForVolumeName)
				throw E_NOENTRYPOINT;
		}
		else
		{
			SaveWin32Error("GetModuleHandle", GetLastError());
			throw E_APIERROR;
		}
	}

	FoxArray pArray(parm(1));
	FoxString pVolume(parm(2));
	FoxString pBuffer(2048);
	DWORD dwLen; 

	do 
	{
		if (!fpGetVolumePathNamesForVolumeName(pVolume, pBuffer, pBuffer.Size(), &dwLen))
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
	char *pPathPtr = pBuffer;
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
	FoxArray pArray(parm(1), 5);
	FoxString pRootPath(parm(2), 2);
	FoxString pVolumeName(MAX_PATH);
	FoxString pFileSystemName(MAX_PATH);
	DWORD dwVolumeSerialNumber, dwMaximumComponentLen, dwFileSystemFlags;

	SwitchErrorMode pErrorMode(SEM_FAILCRITICALERRORS);

	pRootPath.AddBs();
	if (!GetVolumeInformation(pRootPath, pVolumeName, pVolumeName.Size(), &dwVolumeSerialNumber, &dwMaximumComponentLen, 
							&dwFileSystemFlags, pFileSystemName, pFileSystemName.Size()))
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

void _fastcall GetWindowsDirectoryLib(ParamBlkEx& parm)
{
try
{
	FoxString pDir(MAX_PATH);

	pDir.Len(GetWindowsDirectory(pDir,MAX_PATH));
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

void _fastcall GetSystemDirectoryLib(ParamBlkEx& parm)
{
try
{
	FoxString pDir(MAX_PATH);

	pDir.Len(GetSystemDirectory(pDir,MAX_PATH));
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

void _fastcall ExpandEnvironmentStringsLib(ParamBlkEx& parm)
{
try
{
	FoxString pEnvString(parm(1));
	FoxString pEnvBuffer(MAX_ENVSTRING_BUFFER);

	pEnvBuffer.Len(ExpandEnvironmentStrings(pEnvString, pEnvBuffer, pEnvBuffer.Size()));
	if (!pEnvBuffer.Len())
	{
		SaveWin32Error("ExpandEnvironmentStrings", GetLastError());
		throw E_APIERROR;
	}
	else if (pEnvBuffer.Len() > MAX_ENVSTRING_BUFFER)
	{
		pEnvBuffer.Size(pEnvBuffer.Len());
		pEnvBuffer.Len(ExpandEnvironmentStrings(pEnvString,pEnvBuffer,pEnvBuffer.Size()));
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
	FoxString pFileName(parm(1));

	HANDLE hFile;
	DWORD dwAccess, dwShare, dwFlags;

	MapFileAccessFlags(parm.PCount() >= 2 ? parm(2)->ev_long : FILE_ATTRIBUTE_NORMAL, parm.PCount() >= 3 ? parm(3)->ev_long : 2, parm.PCount() >= 4 ? parm(4)->ev_long : 0, &dwAccess, &dwShare, &dwFlags);

	if (pFileName.Len() <= MAX_PATH)
		hFile = CreateFile(pFileName, dwAccess, dwShare, 0, CREATE_ALWAYS, dwFlags, 0);
	else
	{
		pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
		FoxWString<MAX_PATH*2> pWideFileName;
		pWideFileName = pFileName;
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
	FoxString pFileName(parm(1));
	HANDLE hFile;
	DWORD dwAccess, dwShare, dwFlags;

	// get a free entry in our file handle array
	if (pFileName.Len() <= MAX_PATH)
		pFileName.Fullpath();

	MapFileAccessFlags(parm.PCount() >= 2 ? parm(2)->ev_long : 0, parm.PCount() >= 3 ? parm(3)->ev_long : 2, parm.PCount() >= 4 ? parm(4)->ev_long : 0, &dwAccess, &dwShare, &dwFlags);

	if (pFileName.Len() <= MAX_PATH)
        hFile = CreateFile(pFileName,dwAccess,dwShare,0,OPEN_EXISTING,dwFlags,0);
	else
	{
		pFileName.PrependIfNotPresent(FILE_UNICODE_EXTENSION);
		FoxWString<MAX_PATH * 2> pWideFileName;
		pWideFileName = pFileName;
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
	BOOL bApiRet, bRet;
	HANDLE hFile = parm(1)->Ptr<HANDLE>();
	VFP2CTls& tls = VFP2CTls::Tls();

	if (hFile != INVALID_HANDLE_VALUE)
	{
		bApiRet = CloseHandle(hFile);
		if (!bApiRet)
			SaveWin32Error("CloseHandle", GetLastError());
		else
		{
			int nIndex = tls.FileHandles.Find(hFile);
			if (nIndex != -1)
				tls.FileHandles.Remove(nIndex);
		}
	}
	else
	{
		int nCount = tls.FileHandles.GetCount();
		for (int xj = 0; xj < nCount; xj++)
		{
			bRet = CloseHandle(tls.FileHandles[xj]);
			if (!bRet) 
			{
				bApiRet = FALSE;
				AddWin32Error("CloseHandle", GetLastError());
			}
		}
		tls.FileHandles.SetIndex(-1);
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
		SaveWin32Error("WriteFile",GetLastError());

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

	pOrigData = pData = pBuffer;

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
	FoxInt64 pNewFilePos;
	DWORD dwMove = parm.PCount() == 3 ? parm(3)->ev_long : FILE_BEGIN;

	nFilePos.QuadPart = static_cast<__int64>(parm(2)->ev_real);
	nFilePos.LowPart = SetFilePointer(hFile, nFilePos.LowPart, &nFilePos.HighPart, dwMove);
	if (nFilePos.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
	{
		SaveWin32Error("SetFilePointer", GetLastError());
		throw E_APIERROR;
	}

	pNewFilePos = nFilePos.QuadPart;
	pNewFilePos.Return();
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
	LARGE_INTEGER nCurr, nEof;
	DWORD dwLastError, nReset;

	nCurr.HighPart = 0;
	nCurr.LowPart = SetFilePointer(hFile, 0, &nCurr.HighPart, FILE_CURRENT);
	if (nCurr.LowPart == INVALID_SET_FILE_POINTER)
	{
		dwLastError = GetLastError();
		if (dwLastError != NO_ERROR)
		{
			SaveWin32Error("SetFilePointer", dwLastError);
			throw E_APIERROR;
		}
	}

	nEof.HighPart = 0;
	nEof.LowPart = SetFilePointer(hFile,0,&nEof.HighPart,FILE_END);
	if (nEof.LowPart == INVALID_SET_FILE_POINTER)
	{
		dwLastError = GetLastError();
		if (dwLastError != NO_ERROR)
		{
			SaveWin32Error("SetFilePointer", dwLastError);
			throw E_APIERROR;
		}
	}

	nReset = SetFilePointer(hFile,nCurr.LowPart,&nCurr.HighPart,FILE_BEGIN);
	if (nReset == INVALID_SET_FILE_POINTER)
	{
		dwLastError = GetLastError();
		if (dwLastError != NO_ERROR)
		{
			SaveWin32Error("SetFilePointer", dwLastError);
			throw E_APIERROR;
		}
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
	BOOL bApiRet;
	LARGE_INTEGER nSize;
	LARGE_INTEGER nCurrPos;
	DWORD dwRet, dwLastError;
	FoxInt64 pFilePos;

	nSize.QuadPart = static_cast<__int64>(parm(2)->ev_real);
    
	// save current file pointer
	nCurrPos.HighPart = 0;
	nCurrPos.LowPart = SetFilePointer(hFile, 0, &nCurrPos.HighPart, FILE_CURRENT);
	if (nCurrPos.LowPart == INVALID_SET_FILE_POINTER)
	{
		dwLastError = GetLastError();
		if (dwLastError != NO_ERROR)
		{
			SaveWin32Error("SetFilePointer", dwLastError);
			throw E_APIERROR;
		}
	}

	// set file pointer to specified size
	nSize.LowPart = SetFilePointer(hFile, nSize.LowPart, &nSize.HighPart, FILE_BEGIN);
	if (nSize.LowPart == INVALID_SET_FILE_POINTER)
	{
		dwLastError = GetLastError();
		if (dwLastError != NO_ERROR)
		{
			SaveWin32Error("SetFilePointer", dwLastError);
			throw E_APIERROR;
		}
	}

	// set file size
	bApiRet = SetEndOfFile(hFile);
	if (!bApiRet)
	{
		SaveWin32Error("SetEndOfFile", GetLastError());
		throw E_APIERROR;
	}

	// reset file pointer to saved position
	dwRet = SetFilePointer(hFile, nCurrPos.LowPart, &nCurrPos.HighPart, FILE_BEGIN);
	if (dwRet == INVALID_SET_FILE_POINTER)
	{
		dwLastError = GetLastError();
		if (dwLastError != NO_ERROR)
		{
			SaveWin32Error("SetFilePointer", dwLastError);
			throw E_APIERROR;
		}
	}

	pFilePos = nSize.QuadPart;
	pFilePos.Return();
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

	if (parm(2)->Vartype() == 'I')
		nOffset.QuadPart = static_cast<__int64>(parm(2)->ev_long);
	else if (parm(2)->Vartype() == 'N')
		nOffset.QuadPart = static_cast<__int64>(parm(2)->ev_real);
	else
		throw E_INVALIDPARAMS;

	if (parm(3)->Vartype() == 'I')
		nLen.QuadPart = static_cast<__int64>(parm(3)->ev_long);
	else if (parm(3)->Vartype() == 'N')
		nLen.QuadPart = static_cast<__int64>(parm(3)->ev_real);
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

	if (parm(2)->Vartype() == 'I')
		nOffset.QuadPart = static_cast<__int64>(parm(2)->ev_long);
	else if (parm(2)->Vartype() == 'N')
		nOffset.QuadPart = static_cast<__int64>(parm(2)->ev_real);
	else
		throw E_INVALIDPARAMS;

	if (parm(3)->Vartype() == 'I')
		nLen.QuadPart = static_cast<__int64>(parm(3)->ev_long);
	else if (parm(3)->Vartype() == 'N')
		nLen.QuadPart = static_cast<__int64>(parm(3)->ev_real);
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

	if (parm(2)->Vartype() == 'I')
		nOffset.QuadPart = static_cast<__int64>(parm(2)->ev_long);
	else if (parm(2)->Vartype() == 'N')
		nOffset.QuadPart = static_cast<__int64>(parm(2)->ev_real);
	else
		throw E_INVALIDPARAMS;

	if (parm(3)->Vartype() == 'I')
		nLen.QuadPart = static_cast<__int64>(parm(3)->ev_long);
	else if (parm(3)->Vartype() == 'N')
		nLen.QuadPart = static_cast<__int64>(parm(3)->ev_real);
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

	if (parm(2)->Vartype() == 'I')
		nOffset.QuadPart = static_cast<__int64>(parm(2)->ev_long);
	else if (parm(2)->Vartype() == 'N')
		nOffset.QuadPart = static_cast<__int64>(parm(2)->ev_real);
	else
		throw E_INVALIDPARAMS;

	if (parm(3)->Vartype() == 'I')
		nLen.QuadPart = static_cast<__int64>(parm(3)->ev_long);
	else if (parm(3)->Vartype() == 'N')
		nLen.QuadPart = static_cast<__int64>(parm(3)->ev_real);
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
	size_t nHandleCnt = tls.FileHandles.GetCount();

	if (nHandleCnt == 0)
	{
		Return(0);
		return;
	}

	pArray.Dimension(nHandleCnt, 1);

	for (size_t xj = 0; xj < nHandleCnt; xj++)
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