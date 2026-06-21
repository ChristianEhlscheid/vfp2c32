/* VFP2C specific includes */
#include "vfp2c32.h"  /* VFP2C32 specific types & defines */
#include "vfp2chyperscan.h"
#include <cmath>

BOOL g_HyperScanInitialized = FALSE;
static PCANCELIOEX fpCancelIoEx = 0;
static PGETQUEUEDCOMPLETIONSTATUSEX fpGetQueuedCompletionStatusEx = 0;
static PCREATETHREADPOOL fpCreateThreadpool = 0;
static PCLOSETHREADPOOL fpCloseThreadpool = 0;
static PSETTHREADPOOLTHREADMAXIMUM fpSetThreadpoolThreadMaximum = 0;
static PSETTHREADPOOLTHREADMINIMUM fpSetThreadpoolThreadMinimum = 0;

static PCREATETHREADPOOLCLEANUPGROUP fpCreateThreadpoolCleanupGroup = 0;
static PCLOSETHREADPOOLCLEANUPGROUP fpCloseThreadpoolCleanupGroup = 0;
static PCLOSETHREADPOOLCLEANUPGROUPMEMBERS fpCloseThreadpoolCleanupGroupMembers = 0;

// static PCREATETHREADPOOLIO fpCreateThreadpoolIo = 0;
// static PSTARTTHREADPOOLIO fpStartThreadpoolIo = 0;
// static PCANCELTHREADPOOLIO fpCancelThreadpoolIo = 0;
// static PCLOSETHREADPOOLIO fpCloseThreadpoolIo = 0;
// static PWAITFORTHREADPOOLIOCALLBACKS fpWaitForThreadpoolIoCallbacks = 0;

static PCREATETHREADPOOLWORK fpCreateThreadpoolWork = 0;
static PCLOSETHREADPOOLWORK fpCloseThreadpoolWork = 0;
static PSUBMITTHREADPOOLWORK fpSubmitThreadpoolWork = 0;
// static PTRYSUBMITTHREADPOOLCALLBACK fpTrySubmitThreadpoolCallback = 0;
// static PWAITFORTHREADPOOLWORKCALLBACKS fpWaitForThreadpoolWorkCallbacks = 0;

static phs_populate_platform fphs_populate_platform = 0;
static phs_compile fphs_compile = 0;
static phs_compile_multi fphs_compile_multi = 0;
static phs_compile_ext_multi fphs_compile_ext_multi = 0;
static phs_compile_lit fphs_compile_lit = 0;
static phs_compile_lit_multi fphs_compile_lit_multi = 0;
static phs_free_compile_error fphs_free_compile_error = 0;
static phs_alloc_scratch fphs_alloc_scratch = 0;
static phs_clone_scratch fphs_clone_scratch = 0;
static phs_free_scratch fphs_free_scratch = 0;
static phs_scan fphs_scan = 0;
static phs_free_database fphs_free_database = 0;
static phs_open_stream fphs_open_stream = 0;
static phs_scan_stream fphs_scan_stream = 0;
static phs_close_stream fphs_close_stream = 0;
static phs_reset_stream fphs_reset_stream = 0;
static phs_set_allocator fphs_set_allocator = 0;

class HS_GlobalContext
{
public:
	HS_GlobalContext() : HsDb(0), HsStream(0), HsScratch(0), HsCallback(0), HsMode(0), BufferSize(0), MatchContext_Idx(-1), MatchId_Idx(-1), 
		MatchFrom_Idx(-1), MatchTo_Idx(-1), MatchExpr_Idx(-1), MatchLineNo_Idx(-1), m_Buffer(0), m_FreeScratch(false) { }

	~HS_GlobalContext()
	{
		if (HsStream)
			fphs_close_stream(HsStream, 0, 0, 0);
		if (HsScratch)
			fphs_free_scratch(HsScratch);
		if (m_Buffer)
			_aligned_free(m_Buffer);
	}

	FoxCursor Cursor;
	FoxMemo MatchMemo;
	FoxInt64 MatchValue;
	HANDLE ResultCompletionPort;
	hs_database* HsDb;
	hs_stream* HsStream;
	hs_scratch* HsScratch;
	match_event_handler HsCallback;
	unsigned int HsMode;
	unsigned int BufferSize;
	int MatchContext_Idx,
		MatchId_Idx,
		MatchFrom_Idx,
		MatchTo_Idx,
		MatchExpr_Idx,
		MatchLineNo_Idx;

	char* GetBuffer(unsigned int nPos)
	{
		assert(HsDb);
		return (HsMode & HS_MODE_STREAM) ? m_Buffer + (size_t)(BufferSize * nPos) : 0;
	}

	hs_error_t SetupResources(hs_scratch* pScratch, unsigned int nCount, unsigned int nMode, unsigned int nBufferSize)
	{
		assert(HsDb);
		hs_error_t res = HS_SUCCESS;
		if (nMode & HS_MODE_STREAM)
		{
			if (nBufferSize == 0)
				BufferSize = 2 * ((nMode & HS_MODE_SOM_HORIZON_SMALL) ? MAX_USHORT : (256 * 1024));
			else
				BufferSize = 2 * nBufferSize;

			res = fphs_open_stream(HsDb, 0, &HsStream);
			if (res != HS_SUCCESS)
				return res;

			// NOLINTNEXTLINE(bugprone-implicit-widening-of-multiplication-result)
			m_Buffer = (char*)_aligned_malloc(nCount * BufferSize, 4096);
			if (m_Buffer == 0)
				throw E_OUTOFMEMORY;
		}
		else
		{
			// default to max 16MB BufferSize in HS_MODE_BLOCK if
			BufferSize = nBufferSize ? nBufferSize : (16 * 1024 * 1024);
		}

		if (pScratch)
			HsScratch = pScratch;
		else
		{
			res = fphs_alloc_scratch(HsDb, &HsScratch);
			if (res != HS_SUCCESS)
				return res;
			m_FreeScratch = true;
		}

		return res;
	}

private:
	char* m_Buffer;
	bool m_FreeScratch;
};

class __declspec(align(64)) HS_ScanContext
{
public:
	HS_ScanContext()
	{
		MatchContextValue = 0;
		HsStream = 0;
		HsScratch = 0;
		HsMode = 0;
		Buffer = 0;
		BufferSize = 0;
		LastError = 0;
		LineNoCounter = 1;
		LastLineStart = 0;
		LastExprPosition = 0;
		MatchContextValue_Idx = -1;
		MatchId_Idx = -1;
		MatchFrom_Idx = -1;
		MatchTo_Idx = -1;
		MatchLineNo_Idx = -1;
		MatchExpr_Idx = -1;
	}

	~HS_ScanContext()
	{
		if (HsScratch)
			fphs_free_scratch(HsScratch);
		if (HsStream)
			fphs_close_stream(HsStream, 0, 0, 0);
	}

	void Reset()
	{
		LineNoCounter = 1;
		LastLineStart = 0;
		LastExprPosition = 0;
	}

	static int __cdecl scan_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context);
	static int __cdecl scan_handler_lineno(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context);

	FoxString MatchExpr;
	FoxInt64 MatchValue;
	FoxCursor Cursor;
	FoxMemo MatchMemo;
	Value* MatchContextValue;
	hs_stream* HsStream;
	hs_scratch_t* HsScratch;
	unsigned int HsMode;
	unsigned int BufferSize;
	char* Buffer;
	unsigned long long LineNoCounter;
	unsigned long long LastLineStart;
	unsigned long long LastExprPosition;
	int LastError;
	int MatchContextValue_Idx;
	int MatchId_Idx;
	int MatchFrom_Idx;
	int MatchTo_Idx;
	int MatchLineNo_Idx;
	int MatchExpr_Idx;
};

class __declspec(align(64)) HS_FileScanContext
{
public:
	HS_FileScanContext()
	{
		GlobalContext = 0;
		HsResult = HS_SUCCESS;
		WorkItem = 0;
		FileHandle = INVALID_HANDLE_VALUE;
		FileSize.QuadPart = 0;
		BlockBufferSize = 0;
		Buffer = 0;
		ErrorState = ERRORSTATE_NOERROR;
		LastError = 0;
		LastErrorFunction = 0;
	}

	~HS_FileScanContext()
	{
		if (FileHandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(FileHandle);
		}
		if (GlobalContext && (GlobalContext->HsMode & HS_MODE_BLOCK))
		{
			if (Buffer)
				_aligned_free(Buffer);
		}
	}

	void Reset()
	{
		LastBuffer = 0;
		LastLineStart = 0;
		LineNoCounter = 1;
		LastExprPosition = 0;
		BufferPosition = 0;
		LastBufferPosition = 0;
		ErrorState = ERRORSTATE_NOERROR;
	}

	void CancelIo()
	{
		HANDLE lpHandle = FileHandle;
		if (lpHandle != INVALID_HANDLE_VALUE)
		{
			if (!fpCancelIoEx(lpHandle, 0))
			{
				DWORD nLastError = GetLastError();
				if (nLastError != ERROR_NOT_FOUND)
					AddWin32Error("CancelIoEx", nLastError);
			}
		}
	}

	static int __cdecl scan_block_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context);
	static int __cdecl scan_block_lineno_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context);
	static int __cdecl scan_stream_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context);
	static int __cdecl scan_stream_lineno_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context);

	static void _stdcall scan_worker(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK workitem);
	static bool _stdcall filter_item_postcompletion(HS_FileScanContext* lpContext, DWORD compare);

	HS_GlobalContext* GlobalContext;
	hs_error_t HsResult;
	PTP_WORK WorkItem;
	FoxString FileName;
	FoxValue ContextValue;
	HANDLE FileHandle;
	LARGE_INTEGER FileSize;
	union
	{
		DWORD ReadCount;
		DWORD BlockBufferSize;
	};
	DWORD BytesRead;
	char* Buffer;
	char* LastBuffer;
	unsigned long long LastLineStart;
	unsigned long long LineNoCounter;
	unsigned long long LastExprPosition;
	unsigned long long BufferPosition;
	unsigned long long LastBufferPosition;
	Hyperscan_ErrorState ErrorState;
	DWORD LastError;
	const char* LastErrorFunction;
private:
	CWideStrBuilder<MAX_WIDE_PATH> WideFileName;
};

static void __cdecl hyperscan_free(void* ptr)
{
	_aligned_free(ptr);
}

static void* __cdecl hyperscan_alloc(size_t size)
{
	return _aligned_malloc(size, 64);
}

static int _stdcall VFP2C_Init_HyperScan()
{
	if (VFP2CTls::HyperScanInitialized)
		return 0;
	
#ifdef _THREADSAFE
	CriticalSectionGuard guard = VFP2CTls::Guard();

	if (VFP2CTls::HyperScanInitialized)
		return 0;
#endif // _THREADSAFE

	HMODULE hDll = GetModuleHandle("kernel32.dll");
	if (hDll)
	{
		fpCancelIoEx = (PCANCELIOEX)GetProcAddress(hDll, "CancelIoEx");
		fpGetQueuedCompletionStatusEx = (PGETQUEUEDCOMPLETIONSTATUSEX)GetProcAddress(hDll, "GetQueuedCompletionStatusEx");
		fpCreateThreadpool = (PCREATETHREADPOOL)GetProcAddress(hDll, "CreateThreadpool");
		fpCloseThreadpool = (PCLOSETHREADPOOL)GetProcAddress(hDll, "CloseThreadpool");
		fpSetThreadpoolThreadMaximum = (PSETTHREADPOOLTHREADMAXIMUM)GetProcAddress(hDll, "SetThreadpoolThreadMaximum");
		fpSetThreadpoolThreadMinimum = (PSETTHREADPOOLTHREADMINIMUM)GetProcAddress(hDll, "SetThreadpoolThreadMinimum");

		fpCreateThreadpoolCleanupGroup = (PCREATETHREADPOOLCLEANUPGROUP)GetProcAddress(hDll, "CreateThreadpoolCleanupGroup");
		fpCloseThreadpoolCleanupGroup = (PCLOSETHREADPOOLCLEANUPGROUP)GetProcAddress(hDll, "CloseThreadpoolCleanupGroup");
		fpCloseThreadpoolCleanupGroupMembers = (PCLOSETHREADPOOLCLEANUPGROUPMEMBERS)GetProcAddress(hDll, "CloseThreadpoolCleanupGroupMembers");

		// fpCreateThreadpoolIo = (PCREATETHREADPOOLIO)GetProcAddress(hDll, "CreateThreadpoolIo");
		// fpStartThreadpoolIo = (PSTARTTHREADPOOLIO)GetProcAddress(hDll, "StartThreadpoolIo");
		// fpCancelThreadpoolIo = (PCANCELTHREADPOOLIO)GetProcAddress(hDll, "CancelThreadpoolIo");
		// fpCloseThreadpoolIo = (PCLOSETHREADPOOLIO)GetProcAddress(hDll, "CloseThreadpoolIo");
		// fpWaitForThreadpoolIoCallbacks = (PWAITFORTHREADPOOLIOCALLBACKS)GetProcAddress(hDll, "WaitForThreadpoolIoCallbacks");

		fpCreateThreadpoolWork = (PCREATETHREADPOOLWORK)GetProcAddress(hDll, "CreateThreadpoolWork");
		fpCloseThreadpoolWork = (PCLOSETHREADPOOLWORK)GetProcAddress(hDll, "CloseThreadpoolWork");
		fpSubmitThreadpoolWork = (PSUBMITTHREADPOOLWORK)GetProcAddress(hDll, "SubmitThreadpoolWork");
		// fpTrySubmitThreadpoolCallback = (PTRYSUBMITTHREADPOOLCALLBACK)GetProcAddress(hDll, "TrySubmitThreadpoolCallback");
		// fpWaitForThreadpoolWorkCallbacks = (PWAITFORTHREADPOOLWORKCALLBACKS)GetProcAddress(hDll, "WaitForThreadpoolWorkCallbacks");
	}
	else
	{
		SaveWin32Error("GetModuleHandle", GetLastError());
		return E_APIERROR;
	}
	
	if (VFP2CTls::HsDll == 0)
		VFP2CTls::HsDll = LoadLibrary("hs.dll");

	HMODULE hHs = VFP2CTls::HsDll;
	if (hHs)
	{
		fphs_populate_platform = (phs_populate_platform)GetProcAddress(hHs, "hs_populate_platform");
		fphs_compile = (phs_compile)GetProcAddress(hHs, "hs_compile");
		fphs_compile_multi = (phs_compile_multi)GetProcAddress(hHs, "hs_compile_multi");
		fphs_compile_ext_multi = (phs_compile_ext_multi)GetProcAddress(hHs, "hs_compile_ext_multi");
		fphs_compile_lit = (phs_compile_lit)GetProcAddress(hHs, "hs_compile_lit");
		fphs_compile_lit_multi = (phs_compile_lit_multi)GetProcAddress(hHs, "hs_compile_lit_multi");
		fphs_free_compile_error = (phs_free_compile_error)GetProcAddress(hHs, "hs_free_compile_error");
		fphs_alloc_scratch  = (phs_alloc_scratch)GetProcAddress(hHs, "hs_alloc_scratch");
		fphs_clone_scratch = (phs_clone_scratch)GetProcAddress(hHs, "hs_clone_scratch");
		fphs_free_scratch = (phs_free_scratch)GetProcAddress(hHs, "hs_free_scratch");
		fphs_scan = (phs_scan)GetProcAddress(hHs, "hs_scan");
		fphs_free_database = (phs_free_database)GetProcAddress(hHs, "hs_free_database");
		fphs_open_stream = (phs_open_stream)GetProcAddress(hHs, "hs_open_stream");
		fphs_scan_stream = (phs_scan_stream)GetProcAddress(hHs, "hs_scan_stream");
		fphs_close_stream = (phs_close_stream)GetProcAddress(hHs, "hs_close_stream");
		fphs_reset_stream = (phs_reset_stream)GetProcAddress(hHs, "hs_reset_stream");
		fphs_set_allocator = (phs_set_allocator)GetProcAddress(hHs, "hs_set_allocator");
		if (fphs_set_allocator)
		{
			hs_error_t ret = fphs_set_allocator(hyperscan_alloc, hyperscan_free);
			if (ret != HS_SUCCESS)
			{
				SaveCustomErrorEx("hs_set_allocator", "Function failed.", ret);
				return E_APIERROR;
			}
		}
	}
	else
	{
		SaveWin32Error("LoadLibrary", GetLastError());
		return E_APIERROR;
	}

	if (!fpCancelIoEx || !fpGetQueuedCompletionStatusEx || 
		!fpCreateThreadpool || !fpCloseThreadpool || !fpSetThreadpoolThreadMaximum || !fpSetThreadpoolThreadMinimum || 
		!fpCreateThreadpoolCleanupGroup || 	!fpCloseThreadpoolCleanupGroup || !fpCloseThreadpoolCleanupGroupMembers || 
		!fpCreateThreadpoolWork || !fpCloseThreadpoolWork || !fpSubmitThreadpoolWork || 
		!fphs_populate_platform || !fphs_compile || !fphs_compile_multi || !fphs_compile_ext_multi || !fphs_compile_lit || !fphs_compile_lit_multi ||
		!fphs_free_compile_error || !fphs_alloc_scratch || !fphs_clone_scratch || !fphs_free_scratch || !fphs_scan || !fphs_free_database ||
		!fphs_open_stream || !fphs_scan_stream || !fphs_close_stream || !fphs_reset_stream || !fphs_set_allocator)
	{
		SaveWin32Error("GetProcAddress", ERROR_PROC_NOT_FOUND);
		return E_APIERROR;
	}
	else
	{
		VFP2CTls::HyperScanInitialized = true;
	}

	return 0;
}

void _fastcall Hyperscan_platform(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_HyperScan();
	if (nErrorNo)
		return RaiseError(nErrorNo);

	try
	{

		hs_platform_info_t pPlatformInfo;
		hs_error_t result = fphs_populate_platform(&pPlatformInfo);
		if (result == HS_SUCCESS)
		{
			FoxObject pPlatform;
			pPlatform.EmptyObject();
			pPlatform("tune") << pPlatformInfo.tune;
			pPlatform("cpu_features") << (int)pPlatformInfo.cpu_features;
			LocatorEx& p1 = parm(1);
			p1 = pPlatform;
		}
		Return(result);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

void _fastcall Hyperscan_compile(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_HyperScan();
	if (nErrorNo)
		return RaiseError(nErrorNo);

	CArray<ValueEx> pExpressionValues;
	try
	{
		FoxString pExpr(parm, 1, 1);
		FoxArray pExpressions(parm, 1, FoxArray::BindByReference);
		FoxArray pExprFlags(parm, 3, FoxArray::BindByReference);
		FoxArray pExprExtension(parm, 5, FoxArray::BindByName);
		unsigned int nFlags = parm.PCount() >= 3 && parm(3)->Numeric() ? parm(3)->NumericCast<unsigned int>() : 0;
		unsigned int nMode = parm.PCount() >= 4 ? parm(4)->ev_long : (HS_MODE_STREAM | HS_MODE_SOM_HORIZON_SMALL);
		unsigned int nExtendedMode = (nMode & (HS_MODE_EX_LINENO | HS_MODE_EX_LITERAL | HS_MODE_EX_MATCH_ID | HS_MODE_EX_SKIP_OVERLAPPED_EXPR | HS_MODE_EX_STORE_SKIPPED_FILES));
		unsigned int nExpressionCount = (nExtendedMode & HS_MODE_EX_LINENO) ? 1 : 0;
		unsigned int nExpressionOffset = nExpressionCount;
		bool bMultiExpression = (pExpressions || nExpressionCount || pExprExtension);
		hs_error_t hs_Result;
		hs_database* hs_Db = 0;
		hs_compile_error* hs_Error = 0;
		hs_platform_info_t hs_Platform;
		hs_platform_info_t* phs_Platform = 0;
		nMode &= ~(HS_MODE_EX_LINENO | HS_MODE_EX_LITERAL | HS_MODE_EX_MATCH_ID | HS_MODE_EX_SKIP_OVERLAPPED_EXPR | HS_MODE_EX_STORE_SKIPPED_FILES);
		nFlags |= HS_FLAG_SOM_LEFTMOST;

		if (parm(1)->Vartype() != 'C' && parm(1)->Vartype() != 'R')
		{
			SaveCustomErrorEx("Hyperscan_compile_multi", "Parameter caExpression has an invalid type.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}

		if (parm(1)->Vartype() != 'R' && parm(3)->Vartype() == 'R')
		{
			SaveCustomErrorEx("Hyperscan_compile_multi", "Parameter aFlags has an invalid type, pass only a numeric value when passing a single regular expression.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}

		if (parm.PCount() >= 5 && parm(5)->Vartype() != 'O' && parm(5)->Vartype() != '0')
		{
			SaveCustomErrorEx("Hyperscan_compile_multi", "Parameter aExtendedFlags has an invalid type.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}

		if (parm.PCount() == 6)
		{
			if (parm(6)->Vartype() == 'O')
			{
				FoxObject pPlatform(parm(6));
				FoxValue pValue;
				pValue << pPlatform("tune");
				hs_Platform.tune = pValue.Numeric() ? pValue.NumericCast<unsigned int>() : 0;
				pValue << pPlatform("cpu_features");
				hs_Platform.cpu_features = pValue.Numeric() ? pValue.NumericCast<unsigned long long>() : 0;
				hs_Platform.reserved1 = 0;
				hs_Platform.reserved2 = 0;
				phs_Platform = &hs_Platform;
			}
			else
			{
				SaveCustomErrorEx("Hyperscan_compile", "Parameter oPlatform is not an object.", E_VFP_INVALIDPARAMS);
				throw E_APIERROR;
			}
		}

		// array of regular expression passed or a single expression + HS_FLAG_EX_LINENO or expression extension is used -> compile a multi statement
		if (bMultiExpression)
		{
			ValueEx pValue;
			CArray<const char*> pExpressionArray;
			CArray<size_t> pExpressionLenArray;
			CArray<unsigned int> pFlagArray;
			CArray<unsigned int> pIDArray;
			CArray<hs_expr_ext> pExprExtArray;
			unsigned int xj;
			nExpressionCount += pExpressions ? pExpressions.ARows() : 1;

			pExpressionArray.Reserve(nExpressionCount, false, true);
			pExpressionValues.Reserve(nExpressionCount, true, true);
			if (nExtendedMode & HS_MODE_EX_LITERAL)
				pExpressionLenArray.Reserve(nExpressionCount, false, true);

			pFlagArray.Reserve(nExpressionCount, false, true);
			pIDArray.Reserve(nExpressionCount, false, true);

			if (pExprExtension)
				pExprExtArray.Reserve(nExpressionCount, true, true);

			if (nExtendedMode & HS_MODE_EX_LINENO)
			{
				pExpressionArray[0] = "$";
				if (nExtendedMode & HS_MODE_EX_LITERAL)
					pExpressionLenArray[0] = 1;
				pFlagArray[0] = HS_FLAG_MULTILINE;
				pIDArray[0] = HYPERSCAN_LINENO_EXPR_ID;
			}

			if (pExpressions)
			{
				for (xj = 0; xj < pExpressions.ARows(); xj++)
				{
					pExpressionValues[xj] = pExpressions(xj + 1, 1);
					if (pExpressionValues[xj].Vartype() != 'C')
					{
						SaveCustomErrorEx("Hyperscan_compile_multi", "Array of regular expressions contained a non string value.", E_VFP_INVALIDPARAMS);
						throw E_APIERROR;
					}

					if (!pExpressionValues[xj].NullTerminate())
						throw E_INSUFMEMORY;

					pExpressionValues[xj].LockHandle();
					pExpressionArray[xj + nExpressionOffset] = pExpressionValues[xj].HandleToPtr();

					if (nExtendedMode & HS_MODE_EX_LITERAL)
						pExpressionLenArray[xj + nExpressionOffset] = pExpressionValues[xj].Len();
				}

				if (pExpressions.ADims() >= 2)
				{
					for (xj = 0; xj < pExpressions.ARows(); xj++)
					{
						pValue = pExpressions(xj + 1, 2);
						pIDArray[xj + nExpressionOffset] = pValue.NumericCast<unsigned int>();
						pValue.Release();
					}
				}
				else
				{
					for (xj = 0; xj < pExpressions.ARows(); xj++)
					{
						pIDArray[xj + nExpressionOffset] = xj + 1;
					}
				}

				for (xj = 0; xj < pExpressions.ARows(); xj++)
				{
					if (pExprFlags)
					{
						pValue = pExprFlags(xj + 1);
						pFlagArray[xj + nExpressionOffset] = (pValue.NumericCast<unsigned int>() | HS_FLAG_SOM_LEFTMOST);
						pValue.Release();
					}
					else
						pFlagArray[xj + nExpressionOffset] = nFlags;
				}
			}
			else
			{
				pExpressionArray[nExpressionOffset] = pExpr.Ptr<char*>();
				pIDArray[nExpressionOffset] = 0;
				pFlagArray[nExpressionOffset] = nFlags;
			}

			if (pExprExtension)
			{
				FoxObject pObj;
				pValue.SetNull();

				for (xj = 0; xj < nExpressionCount; xj++)
				{
					pObj = pExprExtension.EvaluateElement(xj + 1);

					if (!pObj)
						continue;

					pValue << pObj("min_offset");
					if (pValue.Numeric())
					{
						pExprExtArray[xj + nExpressionOffset].flags |= HS_EXT_FLAG_MIN_OFFSET;
						pExprExtArray[xj + nExpressionOffset].min_offset = pValue.NumericCast<ULONGLONG>();
					}

					pValue << pObj("max_offset");
					if (pValue.Numeric())
					{
						pExprExtArray[xj + nExpressionOffset].flags |= HS_EXT_FLAG_MAX_OFFSET;
						pExprExtArray[xj + nExpressionOffset].max_offset = pValue.NumericCast<ULONGLONG>();
					}

					pValue << pObj("min_length");
					if (pValue.Numeric())
					{
						pExprExtArray[xj + nExpressionOffset].flags |= HS_EXT_FLAG_MIN_LENGTH;
						pExprExtArray[xj + nExpressionOffset].min_length = pValue.NumericCast<ULONGLONG>();
					}

					pValue << pObj("edit_distance");
					if (pValue.Numeric())
					{
						pExprExtArray[xj + nExpressionOffset].flags |= HS_EXT_FLAG_EDIT_DISTANCE;
						pExprExtArray[xj + nExpressionOffset].edit_distance = pValue.NumericCast<unsigned int>();
					}

					pValue << pObj("hamming_distance");
					if (pValue.Numeric())
					{
						pExprExtArray[xj + nExpressionOffset].flags |= HS_EXT_FLAG_HAMMING_DISTANCE;
						pExprExtArray[xj + nExpressionOffset].hamming_distance = pValue.NumericCast<unsigned int>();
					}
				}
			}

			if ((nExtendedMode & HS_MODE_EX_LITERAL) == 0)
			{
				if (!pExprExtension)
					hs_Result = fphs_compile_multi(&pExpressionArray[0], &pFlagArray[0], &pIDArray[0], nExpressionCount, nMode, phs_Platform, &hs_Db, &hs_Error);
				else
					hs_Result = fphs_compile_ext_multi(&pExpressionArray[0], &pFlagArray[0], &pIDArray[0], &pExprExtArray[0], nExpressionCount, nMode, phs_Platform, &hs_Db, &hs_Error);
			}
			else
				hs_Result = fphs_compile_lit_multi(&pExpressionArray[0], &pFlagArray[0], &pIDArray[0], &pExpressionLenArray[0], nExpressionCount, nMode, phs_Platform, &hs_Db, &hs_Error);

		}
		else
		{
			if ((nExtendedMode & HS_MODE_EX_LITERAL) == 0)
				hs_Result = fphs_compile(pExpr.Ptr<char*>(), nFlags, nMode, phs_Platform, &hs_Db, &hs_Error);
			else
				hs_Result = fphs_compile_lit(pExpr.Ptr<char*>(), nFlags, pExpr.Len(), nMode, phs_Platform, &hs_Db, &hs_Error);
		}

		if (hs_Result == HS_SUCCESS)
		{
			LocatorEx& pDBRef = parm(2);
			pDBRef = hs_Db;
		}
		else
		{
			if (bMultiExpression)
				SaveCustomErrorEx((nExtendedMode & HS_MODE_EX_LITERAL) == 0 ? (!pExprExtension ? "hs_compile_multi" : "hs_compile_ext_multi") : "fphs_compile_lit_multi", hs_Error->message, hs_Result);
			else
				SaveCustomErrorEx((nExtendedMode & HS_MODE_EX_LITERAL) == 0 ? "hs_compile" : "hs_compile_lit", hs_Error->message, hs_Result);

			fphs_free_compile_error(hs_Error);
		}

		Return(hs_Result);
	}
	catch (int nError)
	{
		RaiseError(nError);
	}

	for (unsigned int xj = 0; xj < pExpressionValues.GetCount(); xj++)
		pExpressionValues[xj].UnlockAndFree();
}

void _fastcall Hyperscan_free_database(ParamBlkEx& parm)
{
	hs_database* hs_Db = parm(1)->DynamicPtr<hs_database*>();
	hs_error_t hs_Result = 	fphs_free_database(hs_Db);
	Return(hs_Result);
}

void _fastcall Hyperscan_alloc_scratch(ParamBlkEx& parm)
{
	hs_database* hs_Db = parm(1)->DynamicPtr<hs_database*>();
	LocatorEx& pScratchRef = parm(2);
	ValueEx nPrevScratch;
	pScratchRef.Load(nPrevScratch);
	hs_scratch* hs_Scratch = nPrevScratch.DynamicPtr<hs_scratch*>();
	hs_error_t hs_Result = fphs_alloc_scratch(hs_Db, &hs_Scratch);
	if (hs_Result == HS_SUCCESS)
		pScratchRef = hs_Scratch;
	Return(hs_Result);
}

void _fastcall Hyperscan_free_scratch(ParamBlkEx& parm)
{
	hs_scratch* hs_Scratch = parm(1)->DynamicPtr<hs_scratch*>();
	hs_error_t hs_Result = fphs_free_scratch(hs_Scratch);
	Return(hs_Result);
}

int __cdecl HS_ScanContext::scan_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context)
{
	HS_ScanContext* lpContext = (HS_ScanContext*)context;
	unsigned long long adjustedfrom;

	if (from == HS_OFFSET_PAST_HORIZON)
	{
		if (lpContext->HsMode & HS_MODE_SOM_HORIZON_SMALL)
			adjustedfrom = to - HYPERSCAN_HORIZON_SMALL_BUFFERSIZE;
		else
			adjustedfrom = to - lpContext->BufferSize;
	}
	else
		adjustedfrom = from;

	if (lpContext->HsMode & HS_MODE_EX_SKIP_OVERLAPPED_EXPR)
	{
		if (adjustedfrom < lpContext->LastExprPosition)
			return 0;

		lpContext->LastExprPosition = to;
	}

	try
	{
		lpContext->Cursor.AppendBlank();

		if (lpContext->MatchContextValue_Idx >= 0)
			lpContext->Cursor(lpContext->MatchContextValue_Idx) = lpContext->MatchContextValue;

		if (lpContext->MatchId_Idx >= 0)
			lpContext->Cursor(lpContext->MatchId_Idx) = lpContext->MatchValue = id;

		lpContext->Cursor(lpContext->MatchFrom_Idx) = lpContext->MatchValue = (from != HS_OFFSET_PAST_HORIZON ? from : -1);
		lpContext->Cursor(lpContext->MatchTo_Idx) = lpContext->MatchValue = to;

		CStringView pMatch(AdvancePtr(lpContext->Buffer, adjustedfrom), to - adjustedfrom);
		lpContext->MatchMemo = pMatch;
		return 0;
	}
	catch (int nErrorNo)
	{
		lpContext->LastError = nErrorNo;
		return 1;
	}
}

int __cdecl HS_ScanContext::scan_handler_lineno(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context)
{
	HS_ScanContext* lpContext = (HS_ScanContext*)context;
	unsigned long long adjustedfrom;
	if (id == HYPERSCAN_LINENO_EXPR_ID)
	{
		lpContext->LineNoCounter++;
		lpContext->LastLineStart = to;
		return 0;
	}

	if (from == HS_OFFSET_PAST_HORIZON)
	{
		if (lpContext->HsMode & HS_MODE_SOM_HORIZON_SMALL)
			adjustedfrom = to - HYPERSCAN_HORIZON_SMALL_BUFFERSIZE;
		else
			adjustedfrom = to - lpContext->BufferSize;
	}
	else
		adjustedfrom = from;

	if (lpContext->HsMode & HS_MODE_EX_SKIP_OVERLAPPED_EXPR)
	{
		if (adjustedfrom < lpContext->LastExprPosition)
			return 0;
	
		lpContext->LastExprPosition = to;
	}
	
	try
	{
		lpContext->Cursor.AppendBlank();

		if (lpContext->MatchContextValue_Idx >= 0)
			lpContext->Cursor(lpContext->MatchContextValue_Idx) = *lpContext->MatchContextValue;

		if (lpContext->MatchId_Idx >= 0)
			lpContext->Cursor(lpContext->MatchId_Idx) = id;

		lpContext->Cursor(lpContext->MatchFrom_Idx) = lpContext->MatchValue = (from != HS_OFFSET_PAST_HORIZON ? from : -1);
		lpContext->Cursor(lpContext->MatchTo_Idx) = lpContext->MatchValue = to;

		CStringView pMatch(AdvancePtr(lpContext->Buffer, adjustedfrom), to - adjustedfrom);
		lpContext->MatchMemo = pMatch;

		if (from < lpContext->LastLineStart)
			lpContext->Cursor(lpContext->MatchLineNo_Idx) = lpContext->MatchValue = lpContext->LineNoCounter - pMatch.GetWordCount('\n', 0);
		else
			lpContext->Cursor(lpContext->MatchLineNo_Idx) = lpContext->MatchValue = lpContext->LineNoCounter;

		return 0;
	}
	catch (int nErrorNo)
	{
		lpContext->LastError = nErrorNo;
		return 1;
	}
}

static hs_error_t _stdcall hyperscan_scancontext(hs_database_t* hs_Db, hs_scratch_t* hs_Scratch, match_event_handler hs_Handler, HS_ScanContext* pContext,  bool bCloseStream)
{
	hs_error_t hs_Result;
	if (pContext->HsMode & HS_MODE_BLOCK)
	{
		hs_Result = fphs_scan(hs_Db, pContext->Buffer, pContext->BufferSize, 0, hs_Scratch, hs_Handler, (void*)pContext);
		if (hs_Result != HS_SUCCESS)
		{
			if (hs_Result != HS_SCAN_TERMINATED)
				SaveCustomErrorEx("hs_scan", "Function failed.", hs_Result);
			else
				throw pContext->LastError;
		}
	}
	else
	{
		if (pContext->HsStream == 0)
		{
			hs_Result = fphs_open_stream(hs_Db, 0, &pContext->HsStream);
			if (hs_Result != HS_SUCCESS)
			{
				SaveCustomErrorEx("hs_open_stream", "Function failed.", hs_Result);
				return hs_Result;
			}
		}

		hs_Result = fphs_scan_stream(pContext->HsStream, pContext->Buffer, pContext->BufferSize, 0, hs_Scratch, hs_Handler, (void*)pContext);
		if (hs_Result == HS_SUCCESS)
		{
			if (bCloseStream)
			{
				hs_Result = fphs_close_stream(pContext->HsStream, hs_Scratch, hs_Handler, (void*)pContext);
				pContext->HsStream = 0;
				if (hs_Result != HS_SUCCESS && hs_Result != HS_SCAN_TERMINATED)
					SaveCustomErrorEx("hs_close_stream", "Function failed.", hs_Result);
			}
			else
			{
				hs_Result = fphs_reset_stream(pContext->HsStream, 0, hs_Scratch, hs_Handler, (void*)pContext);
				if (hs_Result != HS_SUCCESS && hs_Result != HS_SCAN_TERMINATED)
					SaveCustomErrorEx("hs_reset_stream", "Function failed.", hs_Result);
			}
		}
		else if (hs_Result != HS_SCAN_TERMINATED)
			SaveCustomErrorEx("hs_scan_stream", "Function failed.", hs_Result);

		if (hs_Result == HS_SCAN_TERMINATED)
			throw pContext->LastError;
	}

	return hs_Result;
}


void _fastcall Hyperscan_scan(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_HyperScan();
	if (nErrorNo)
		return RaiseError(nErrorNo);

	hs_database* hs_Db = parm(1)->DynamicPtr<hs_database*>();
	hs_scratch* hs_Scratch = parm(2)->DynamicPtr<hs_scratch*>();
	FoxParameterEx& pScan = parm(4);
	FoxParameterEx& pResult = parm(5);
	hs_error_t hs_Result;
	match_event_handler hs_Handler;
	HS_ScanContext lpContext;
	lpContext.HsMode = parm(3)->ev_long;
	if (hs_Scratch == 0)
	{
		hs_Result = fphs_alloc_scratch(hs_Db, &lpContext.HsScratch);
		if (hs_Result != HS_SUCCESS)
		{
			SaveCustomError("hs_alloc_scratch", "Function failed.", hs_Result);
			return Return(hs_Result);
		}
		hs_Scratch = lpContext.HsScratch;
	}

	try
	{
		FoxString pParam;
		FoxString pContextName(parm, 6);
		CStringView pContextField;
		int nFieldNo = 0;
		CStrBuilder<256> pStrBuilder;

		if (pContextName.Len())
			lpContext.MatchContextValue = &parm(7).val;

		if (pResult->Vartype() == 'C')
		{
			FoxString pResultAlias(pResult, 0);

			if (pContextName.Len())
			{
				pContextField = pContextName;
				pStrBuilder.Append(pContextName.Alltrim());
				pStrBuilder.Append(',');
				lpContext.MatchContextValue_Idx = nFieldNo++;
			}

			if (lpContext.HsMode & HS_MODE_EX_MATCH_ID)
			{
				pStrBuilder.Append(__sv("match_id N(10,0) NOT NULL,"));
				lpContext.MatchId_Idx = nFieldNo++;
			}

			pStrBuilder.Append(__sv("match_from N(20,0) NOT NULL,match_to N(20,0) NOT NULL"));
			lpContext.MatchFrom_Idx = nFieldNo++;
			lpContext.MatchTo_Idx = nFieldNo++;

			if (lpContext.HsMode & HS_MODE_EX_LINENO)
			{
				pStrBuilder.Append(__sv(",match_lineno N(20,0) NOT NULL"));
				lpContext.MatchLineNo_Idx = nFieldNo++;
			}
			pStrBuilder.Append(__sv(",match_expr M NOT NULL"));
			lpContext.MatchExpr_Idx = nFieldNo;

			lpContext.Cursor.Create(pResultAlias, pStrBuilder);
			lpContext.MatchMemo.Attach(lpContext.Cursor(nFieldNo));
		}
		else if (pResult->Numeric())
		{
			SHORT nWorkArea = pResult->NumericCast<SHORT>();
			
			if (pContextName.Len())
			{
				pContextField = pContextName;
				pStrBuilder.Append(pContextField.Alltrim());
				pStrBuilder.Append(',');
				lpContext.MatchContextValue_Idx = nFieldNo++;
			}

			if (lpContext.HsMode & HS_MODE_EX_MATCH_ID)
			{
				pStrBuilder.Append(__sv("match_id,"));
				lpContext.MatchId_Idx = nFieldNo++;
			}
				
			pStrBuilder.Append(__sv("match_from,match_to"));
			lpContext.MatchFrom_Idx = nFieldNo++;
			lpContext.MatchTo_Idx = nFieldNo++;

			if (lpContext.HsMode & HS_MODE_EX_LINENO)
			{
				pStrBuilder.Append(__sv(",match_lineno"));
				lpContext.MatchLineNo_Idx = nFieldNo++;
			}
				
			pStrBuilder.Append(__sv(",match_expr"));
			lpContext.MatchExpr_Idx = nFieldNo;

			nErrorNo = lpContext.Cursor.Attach(nWorkArea, pStrBuilder);
			if (nErrorNo)
				throw nErrorNo;

			lpContext.MatchMemo.Attach(lpContext.Cursor(nFieldNo));
		}
		else
		{
			SaveCustomErrorEx("Hyperscan_scan", "Invalid parameter 'cnResultCursor' has to be a cursor name or a workarea number.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}

		hs_Handler = (lpContext.HsMode & HS_MODE_EX_LINENO) ? HS_ScanContext::scan_handler_lineno : HS_ScanContext::scan_handler;

		if (pScan->Vartype() == 'C')
		{
			pParam.Attach(pScan, 0);
			lpContext.BufferSize = pParam.Len();
			lpContext.Buffer = pParam.Ptr<char*>();
			hs_Result = hyperscan_scancontext(hs_Db, hs_Scratch, hs_Handler, &lpContext, true);
		}
		else if (pScan.IsMemoRef())
		{
			FoxMemo pMemo(pScan);
			lpContext.BufferSize = 0;
			lpContext.Buffer = pMemo.Read(lpContext.BufferSize);
			hs_Result = hyperscan_scancontext(hs_Db, hs_Scratch,  hs_Handler, &lpContext, true);
		}
		else if (pScan.IsVariableRef())
		{
			LocatorEx& varref = pScan;
			pParam = varref;
			lpContext.BufferSize = pParam.Len();
			lpContext.Buffer = pParam.Ptr<char*>();
			hs_Result = hyperscan_scancontext(hs_Db, hs_Scratch, hs_Handler, &lpContext, true);
		}
		else
		{
			throw E_INVALIDPARAMS;
		}

		Return(hs_Result);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

void _fastcall Hyperscan_scan_cursor(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_HyperScan();
	if (nErrorNo)
		return RaiseError(nErrorNo);

	hs_database* hs_Db = parm(1)->DynamicPtr<hs_database*>();
	hs_scratch* hs_Scratch = parm(2)->DynamicPtr<hs_scratch*>();
	FoxParameterEx& pSource = parm(4);
	FoxParameterEx& pResult = parm(6);
	hs_error_t hs_Result = HS_SUCCESS;
	match_event_handler hs_Handler = 0;
	HS_ScanContext lpContext;
	ValueEx pFieldValue;
	lpContext.HsMode = parm(3)->ev_long;
	if (hs_Scratch == 0)
	{
		hs_Result = fphs_alloc_scratch(hs_Db, &lpContext.HsScratch);
		if (hs_Result != HS_SUCCESS)
		{
			SaveCustomError("hs_alloc_scratch", "Function failed.", hs_Result);
			return Return(hs_Result);
		}
		hs_Scratch = lpContext.HsScratch;
	}

	try
	{
		FoxCursor pSourceCursor;
		FoxString pSourceAlias;
		FoxString pSourceField(parm(5));
		FoxString pContextName(parm, 7);
		FoxString pMemoBuffer;
		CStringView pContextField;
		int nFieldNo = 0, nErrorNo, nSourceIndex;
		CStrBuilder<256> pStrBuilder;
		pFieldValue.SetNull();

		if (pContextName.Len())
			lpContext.MatchContextValue = &parm(8).val;

		if (pResult->Vartype() == 'C')
		{
			FoxString pResultAlias(pResult, 0);

			if (pContextName.Len())
			{
				pContextField = pContextName;
				pStrBuilder.Append(pContextName.Alltrim());
				pStrBuilder.Append(',');
				lpContext.MatchContextValue_Idx = nFieldNo++;
			}

			if (lpContext.HsMode & HS_MODE_EX_MATCH_ID)
			{
				pStrBuilder.Append(__sv("match_id N(10,0) NOT NULL,"));
				lpContext.MatchId_Idx = nFieldNo++;
			}

			pStrBuilder.Append(__sv("match_from N(20,0) NOT NULL, match_to N(20,0) NOT NULL"));
			lpContext.MatchFrom_Idx = nFieldNo++;
			lpContext.MatchTo_Idx = nFieldNo++;

			if (lpContext.HsMode & HS_MODE_EX_LINENO)
			{
				pStrBuilder.Append(__sv(",match_lineno N(20,0) NOT NULL"));
				lpContext.MatchLineNo_Idx = nFieldNo++;
			}
			pStrBuilder.Append(__sv(",match_expr M NOT NULL"));
			lpContext.MatchExpr_Idx = nFieldNo;

			lpContext.Cursor.Create(pResultAlias, pStrBuilder);
			lpContext.MatchMemo.Attach(lpContext.Cursor(nFieldNo));
		}
		else if (pResult->Numeric())
		{
			SHORT nWorkArea = pResult->NumericCast<SHORT>();

			if (pContextName.Len())
			{
				pContextField = pContextName;
				pStrBuilder.Append(pContextName.Alltrim());
				pStrBuilder.Append(',');
				lpContext.MatchContextValue_Idx = nFieldNo++;
			}

			if (lpContext.HsMode & HS_MODE_EX_MATCH_ID)
			{
				pStrBuilder.Append(__sv("match_id,"));
				lpContext.MatchId_Idx = nFieldNo++;
			}

			pStrBuilder.Append(__sv("match_from,match_to"));
			lpContext.MatchFrom_Idx = nFieldNo++;
			lpContext.MatchTo_Idx = nFieldNo++;

			if (lpContext.HsMode & HS_MODE_EX_LINENO)
			{
				pStrBuilder.Append(__sv(",match_lineno"));
				lpContext.MatchLineNo_Idx = nFieldNo++;
			}

			pStrBuilder.Append(__sv(",match_expr"));
			lpContext.MatchExpr_Idx = nFieldNo;

			nErrorNo = lpContext.Cursor.Attach(nWorkArea, pStrBuilder);
			if (nErrorNo)
				throw nErrorNo;

			lpContext.MatchMemo.Attach(lpContext.Cursor(nFieldNo));
		}
		else
		{
			SaveCustomErrorEx("Hyperscan_scan", "Parameter 'cnResult' has to be a cursor name or a workarea number.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}
			
		if (pSource->Vartype() == 'C')
		{
			pSourceAlias.Attach(pSource, 0);
			nErrorNo = pSourceCursor.Attach(pSourceAlias, pSourceField);
			if (nErrorNo == E_ALIASNOTFOUND)
				throw nErrorNo;

			nSourceIndex = nErrorNo == E_FIELDNOTFOUND ? -1 : 0;
		}
		else if (pSource->Numeric())
		{
			nErrorNo = pSourceCursor.Attach(pSource->NumericCast<SHORT>(), pSourceField);
			if (nErrorNo == E_ALIASNOTFOUND)
				throw nErrorNo;

			nSourceIndex = nErrorNo == E_FIELDNOTFOUND ? -1 : 0;
		}
		else
		{
			SaveCustomErrorEx("Hyperscan_scan", "Parameter 'cnSource' has to be a cursor name or a workarea number.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}

		hs_Handler = (lpContext.HsMode & HS_MODE_EX_LINENO) ? HS_ScanContext::scan_handler_lineno : HS_ScanContext::scan_handler;
		
		AutoRecNoRestore pRestore(pSourceCursor.WorkArea());
		pSourceCursor.GoTop();
		do
		{
			if (nSourceIndex == 0)
				pFieldValue = pSourceCursor(0);
			else
			{
				nErrorNo = pFieldValue.Evaluate(pSourceField.Ptr<const char*>());
				if (nErrorNo)
					throw nErrorNo;
			}
			
			if (pFieldValue.Vartype() == 'C')
			{
				if (pFieldValue.Len())
				{
					pFieldValue.LockHandle();
					lpContext.BufferSize = pFieldValue.Len();
					lpContext.Buffer = pFieldValue.HandleToPtr();

					hs_Result = hyperscan_scancontext(hs_Db, hs_Scratch, hs_Handler, &lpContext, false);
					if (hs_Result != HS_SUCCESS)
						break;

					pFieldValue.UnlockAndFree();
				}
			}
			else if (pFieldValue.Vartype() == 'M')
			{
				if (pFieldValue.ev_long)
				{
					pMemoBuffer.ExtendBuffer(pFieldValue.ev_long);
					FoxMemo::Read(pFieldValue, pMemoBuffer, false);
					lpContext.Buffer = pMemoBuffer.Ptr<char*>();
					lpContext.BufferSize = pFieldValue.ev_long;

					hs_Result = hyperscan_scancontext(hs_Db, hs_Scratch, hs_Handler, &lpContext, false);
					if (hs_Result != HS_SUCCESS)
						break;

				}
			}
			else if (pFieldValue.Vartype() != '0')
			{
				CStringView pView = pSourceField;
				SaveCustomError("Hyperscan_scan_cursor", "The field '%V' has a non scannable datatype.", &pView);
				throw E_APIERROR;
			}

			lpContext.Reset();
			pSourceCursor.Skip();
		} while (!pSourceCursor.Eof());

		Return(hs_Result);
	}
	catch (int nErrorNo)
	{
		pFieldValue.UnlockAndFree();
		RaiseError(nErrorNo);
	}
}

int __cdecl HS_FileScanContext::scan_stream_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context)
{
	HS_FileScanContext* lpContext = (HS_FileScanContext*)context;
	HS_GlobalContext* lpGC = lpContext->GlobalContext;
	FoxCursor& pCursor = lpGC->Cursor;
	CStringView pMatch, pMatch2;
	unsigned long long adjustedfrom;

	if (from == HS_OFFSET_PAST_HORIZON)
	{
		if (lpGC->HsMode & HS_MODE_SOM_HORIZON_SMALL)
			adjustedfrom = to - HYPERSCAN_HORIZON_SMALL_BUFFERSIZE;
		else
			adjustedfrom = to - lpGC->BufferSize;
	}
	else
		adjustedfrom = from;

	if (lpGC->HsMode & HS_MODE_EX_SKIP_OVERLAPPED_EXPR)
	{
		if (adjustedfrom < lpContext->LastExprPosition)
			return 0;

		lpContext->LastExprPosition = to;
	}

	try
	{
		pCursor.AppendBlank();

		if (lpGC->MatchContext_Idx >= 0)
			pCursor(lpGC->MatchContext_Idx) = lpContext->ContextValue;

		if (lpGC->MatchId_Idx >= 0)
			pCursor(lpGC->MatchId_Idx) = id;

		pCursor(lpGC->MatchFrom_Idx) = lpGC->MatchValue = (from != HS_OFFSET_PAST_HORIZON ? from : -1);
		pCursor(lpGC->MatchTo_Idx) = lpGC->MatchValue = to;

		if (from == HS_OFFSET_PAST_HORIZON)
		{
			pMatch.Data = AdvancePtr(lpContext->LastBuffer, adjustedfrom);
			pMatch.Len = lpContext->BufferPosition - lpContext->LastBufferPosition;
			pMatch2.Data = lpContext->Buffer;
			pMatch2.Len = to - lpContext->BufferPosition;
			lpGC->MatchMemo.Alloc((int)(pMatch.Len + pMatch2.Len));
			lpGC->MatchMemo.Write(pMatch.Data, 0, pMatch.Len);
			lpGC->MatchMemo.Write(pMatch2.Data, (int)pMatch.Len, pMatch2.Len);
		}
		else if (from >= lpContext->BufferPosition)
		{
			pMatch.Data = lpContext->Buffer + (from - lpContext->BufferPosition);
			pMatch.Len = to - from;
			lpGC->MatchMemo = pMatch;
		}
		else
		{
			pMatch.Data = lpContext->LastBuffer + (from - lpContext->LastBufferPosition);
			pMatch.Len = lpContext->BufferPosition - from;
			pMatch2.Data = lpContext->Buffer;
			pMatch.Len = to - lpContext->BufferPosition;
			lpGC->MatchMemo.Alloc((int)(pMatch.Len + pMatch2.Len));
			lpGC->MatchMemo.Write(pMatch.Data, 0, pMatch.Len);
			lpGC->MatchMemo.Write(pMatch2.Data, (int)pMatch.Len, pMatch2.Len);
		}
	}
	catch (int nErrorNo)
	{
		lpContext->ErrorState = ERRORSTATE_VFP;
		lpContext->LastError = nErrorNo;
		return 1;
	}

	return 0;
}

int __cdecl HS_FileScanContext::scan_block_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context)
{
	HS_FileScanContext* lpContext = (HS_FileScanContext*)context;
	HS_GlobalContext* lpGC = lpContext->GlobalContext;
	FoxCursor& pCursor = lpGC->Cursor;
	if (lpGC->HsMode & HS_MODE_EX_SKIP_OVERLAPPED_EXPR)
	{
		if (from < lpContext->LastExprPosition)
			return 0;
		lpContext->LastExprPosition = to;
	}

	try
	{
		pCursor.AppendBlank();

		if (lpGC->MatchContext_Idx >= 0)
			pCursor(lpGC->MatchContext_Idx) = lpContext->ContextValue;

		if (lpGC->MatchId_Idx >= 0)
			pCursor(lpGC->MatchId_Idx) = id;

		pCursor(lpGC->MatchFrom_Idx) = lpGC->MatchValue = from;
		pCursor(lpGC->MatchTo_Idx) = lpGC->MatchValue = to;

		CStringView pMatch(AdvancePtr(lpContext->Buffer, from), to - from);
		lpGC->MatchMemo = pMatch;
	}
	catch (int nErrorNo)
	{
		lpContext->ErrorState = ERRORSTATE_VFP;
		lpContext->LastError = nErrorNo;
		return 1;
	}

	return 0;
}

int __cdecl HS_FileScanContext::scan_stream_lineno_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context)
{
	HS_FileScanContext* lpContext = (HS_FileScanContext*)context;
	HS_GlobalContext* lpGC = lpContext->GlobalContext;
	FoxCursor& pCursor = lpGC->Cursor;
	CStringView pMatch, pMatch2;
	unsigned long long adjustedfrom;

	if (id == HYPERSCAN_LINENO_EXPR_ID)
	{
		lpContext->LastLineStart = to;
		lpContext->LineNoCounter++;
		return 0;
	}

	if (from == HS_OFFSET_PAST_HORIZON)
	{
		if (lpGC->HsMode & HS_MODE_SOM_HORIZON_SMALL)
			adjustedfrom = to - HYPERSCAN_HORIZON_SMALL_BUFFERSIZE;
		else
			adjustedfrom = to - lpGC->BufferSize;
	}
	else
		adjustedfrom = from;

	if (lpGC->HsMode & HS_MODE_EX_SKIP_OVERLAPPED_EXPR)
	{
		if (adjustedfrom < lpContext->LastExprPosition)
			return 0;

		lpContext->LastExprPosition = to;
	}

	try
	{
		pCursor.AppendBlank();

		if (lpGC->MatchContext_Idx >= 0)
			pCursor(lpGC->MatchContext_Idx) = lpContext->ContextValue;

		if (lpGC->MatchId_Idx >= 0)
			pCursor(lpGC->MatchId_Idx) = id;

		pCursor(lpGC->MatchFrom_Idx) = lpGC->MatchValue = (from != HS_OFFSET_PAST_HORIZON ? from : -1);
		pCursor(lpGC->MatchTo_Idx) = lpGC->MatchValue = to;

		if (from == HS_OFFSET_PAST_HORIZON)
		{
			pMatch.Data = AdvancePtr(lpContext->LastBuffer, adjustedfrom);
			pMatch.Len = lpContext->BufferPosition - lpContext->LastBufferPosition;
			pMatch2.Data = lpContext->Buffer;
			pMatch2.Len = to - lpContext->BufferPosition;
			lpGC->MatchMemo.Alloc((int)(pMatch.Len + pMatch2.Len));
			lpGC->MatchMemo.Write(pMatch.Data, 0, pMatch.Len);
			lpGC->MatchMemo.Write(pMatch2.Data, (int)pMatch.Len, pMatch2.Len);
		}
		else if (from >= lpContext->BufferPosition)
		{
			pMatch.Data = lpContext->Buffer + (from - lpContext->BufferPosition);
			pMatch.Len = to - from;
			lpGC->MatchMemo = pMatch;
		}
		else
		{
			pMatch.Data = lpContext->LastBuffer + (from - lpContext->LastBufferPosition);
			pMatch.Len = lpContext->BufferPosition - from;
			pMatch2.Data = lpContext->Buffer;
			pMatch.Len = to - lpContext->BufferPosition;
			lpGC->MatchMemo.Alloc((int)(pMatch.Len + pMatch2.Len));
			lpGC->MatchMemo.Write(pMatch.Data, 0, pMatch.Len);
			lpGC->MatchMemo.Write(pMatch2.Data, (int)pMatch.Len, pMatch2.Len);
		}

		if (from < lpContext->LastLineStart)
			pCursor(lpGC->MatchLineNo_Idx) = lpGC->MatchValue = lpContext->LineNoCounter - (pMatch.GetWordCount('\n', 0) + pMatch2.GetWordCount('\n', 0));
		else
			pCursor(lpGC->MatchLineNo_Idx) = lpGC->MatchValue = lpContext->LineNoCounter;

	}
	catch (int nErrorNo)
	{
		lpContext->ErrorState = ERRORSTATE_VFP;
		lpContext->LastError = nErrorNo;
		return 1;
	}

	return 0;
}

int __cdecl HS_FileScanContext::scan_block_lineno_handler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, void* context)
{
	HS_FileScanContext* lpContext = (HS_FileScanContext*)context;
	HS_GlobalContext* lpGC = lpContext->GlobalContext;
	if (id == HYPERSCAN_LINENO_EXPR_ID)
	{
		lpContext->LineNoCounter++;
		lpContext->LastLineStart = to;
		return 0;
	}

	if ((lpGC->HsMode & HS_MODE_EX_SKIP_OVERLAPPED_EXPR))
	{
		if (from < lpContext->LastExprPosition)
			return 0;
		lpContext->LastExprPosition = to;
	}
	
	try
	{
		FoxCursor& pCursor = lpGC->Cursor;
		pCursor.AppendBlank();

		if (lpGC->MatchContext_Idx >= 0)
			pCursor(lpGC->MatchContext_Idx) = lpContext->ContextValue;

		if (lpGC->MatchId_Idx >= 0)
			pCursor(lpGC->MatchId_Idx) = id;

		pCursor(lpGC->MatchFrom_Idx) = lpGC->MatchValue = from;
		pCursor(lpGC->MatchTo_Idx) = lpGC->MatchValue = to;

		CStringView pMatch(AdvancePtr(lpContext->Buffer, from), to - from);
		lpGC->MatchMemo = pMatch;

		if (from < lpContext->LastLineStart)
			pCursor(lpGC->MatchLineNo_Idx) = lpGC->MatchValue = lpContext->LineNoCounter - pMatch.GetWordCount('\n', 0);
		else
			pCursor(lpGC->MatchLineNo_Idx) = lpGC->MatchValue = lpContext->LineNoCounter;

	}
	catch (int nErrorNo)
	{
		lpContext->ErrorState = ERRORSTATE_VFP;
		lpContext->LastError = nErrorNo;
		return 1;
	}

	return 0;
}

void _stdcall HS_FileScanContext::scan_worker(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK workitem)
{
	HS_FileScanContext* lpContext = (HS_FileScanContext*)context;
	DWORD dwBufferSize;

	if (lpContext->FileHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(lpContext->FileHandle);
		lpContext->FileHandle = INVALID_HANDLE_VALUE;
	}
	lpContext->Reset();
	lpContext->WideFileName = __wsv(LFILE_UNICODE_EXTENSION);
	lpContext->LastError = lpContext->WideFileName.AppendNoThrow(lpContext->FileName.StringView());
	if (lpContext->LastError)
	{
		if (lpContext->LastError == E_APIERROR)
		{
			lpContext->ErrorState = ERRORSTATE_WIN32;
			lpContext->LastErrorFunction = "MultiByteToWideChar";
			lpContext->LastError = GetLastError();
		}
		else
		{
			lpContext->ErrorState = ERRORSTATE_VFP;
		}
		goto ErrorOut;
	}

	lpContext->FileHandle = CreateFileW(lpContext->WideFileName, GENERIC_READ, FILE_SHARE_READ & FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);

	if (lpContext->FileHandle == INVALID_HANDLE_VALUE)
	{
		lpContext->ErrorState = ERRORSTATE_WIN32;
		lpContext->LastError = GetLastError();
		lpContext->LastErrorFunction = "CreateFile";
		goto ErrorOut;
	}

	if (lpContext->FileSize.QuadPart == 0)
	{
		if (!GetFileSizeEx(lpContext->FileHandle, &lpContext->FileSize))
		{
			lpContext->ErrorState = ERRORSTATE_WIN32;
			lpContext->LastError = GetLastError();
			lpContext->LastErrorFunction = "GetFileSizeEx";
			goto ErrorOut;
		}
	}

	if (lpContext->GlobalContext->HsMode & HS_MODE_BLOCK)
	{
		// Hyperscan_scan_cursor - contextfield in cursor - contextvalue in parameter
		if (lpContext->GlobalContext->BufferSize == 0 || lpContext->GlobalContext->BufferSize >= lpContext->FileSize.QuadPart)
		{
			if (lpContext->Buffer == 0 || lpContext->BlockBufferSize < lpContext->FileSize.QuadPart)
			{
				dwBufferSize = (64 * 1024);
				lpContext->BlockBufferSize = (lpContext->FileSize.QuadPart / dwBufferSize) * dwBufferSize + (lpContext->FileSize.QuadPart % dwBufferSize ? dwBufferSize : 0);
				if (lpContext->Buffer)
					_aligned_free(lpContext->Buffer);
					
				lpContext->Buffer = (char*)_aligned_malloc(lpContext->BlockBufferSize, 8);
				if (lpContext->Buffer == 0)
				{
					lpContext->ErrorState = ERRORSTATE_VFP;
					lpContext->LastError = E_INSUFMEMORY;
					goto ErrorOut;
				}
			}

			dwBufferSize = lpContext->FileSize.QuadPart;
			if (!ReadFile(lpContext->FileHandle, lpContext->Buffer, dwBufferSize, &lpContext->BytesRead, 0))
			{
				lpContext->ErrorState = ERRORSTATE_WIN32;
				lpContext->LastError = GetLastError();
				lpContext->LastErrorFunction = "ReadFile";
			}
		}
		else
		{
			lpContext->ErrorState = ERRORSTATE_HYPERSCAN;
			lpContext->HsResult = HYPERSCAN_BLOCK_BUFFER_OVERFLOW;
		}
	}
	else
	{
		// NOLINTNEXTLINE(bugprone-narrowing-conversions,bugprone-integer-division)
		lpContext->ReadCount = (int)ceil(((lpContext->FileSize.QuadPart / (lpContext->GlobalContext->BufferSize / 2)) + 0.5));

		if (lpContext->ReadCount <= 2)
			dwBufferSize = lpContext->FileSize.QuadPart;
		else
			dwBufferSize = lpContext->GlobalContext->BufferSize;

		if (!ReadFile(lpContext->FileHandle, lpContext->Buffer, dwBufferSize, &lpContext->BytesRead, 0))
		{
			lpContext->ErrorState = ERRORSTATE_WIN32;
			lpContext->LastError = GetLastError();
			lpContext->LastErrorFunction = "ReadFile";
		}
	}

ErrorOut:
	if (!PostQueuedCompletionStatus(lpContext->GlobalContext->ResultCompletionPort, 0, (ULONG_PTR)lpContext, 0))
	{
		if (lpContext->ErrorState == ERRORSTATE_NOERROR)
		{
			lpContext->ErrorState = (Hyperscan_ErrorState)(ERRORSTATE_WIN32 | ERRORSTATE_POSTCOMPLETION);
			lpContext->LastError = GetLastError();
			lpContext->LastErrorFunction = "PostQueuedCompletionStatus";
		}
		else
			lpContext->ErrorState = (Hyperscan_ErrorState)(lpContext->ErrorState | ERRORSTATE_POSTCOMPLETION);
	}
}

bool _stdcall HS_FileScanContext::filter_item_postcompletion(HS_FileScanContext* lpContext, DWORD compare)
{
	return (lpContext->ErrorState & ERRORSTATE_POSTCOMPLETION) > 0;
}

class HyperScan_FileScan
{
public:
	HyperScan_FileScan() : m_IoCompletionPort(0), m_Canceled(false), m_FileContextPool(0), m_ThreadPool(0), m_CleanupGroup(0)
	{
		m_MinThreads = 2;
		m_MaxThreads = 8;
	}

	~HyperScan_FileScan()
	{
		if (m_CleanupGroup)
		{
			if (m_Canceled == false)
				fpCloseThreadpoolCleanupGroupMembers(m_CleanupGroup, FALSE, 0);
			fpCloseThreadpoolCleanupGroup(m_CleanupGroup);
		}

		DestroyThreadpoolEnvironment(&m_WorkEnvironment);
		if (m_ThreadPool)
			fpCloseThreadpool(m_ThreadPool);

		if (m_IoCompletionPort)
			CloseHandle(m_IoCompletionPort);
		if (m_FileContextPool)
			delete m_FileContextPool;
	}

	hs_error_t Setup(hs_database* pDB, hs_scratch* pScratch, unsigned int nMode, unsigned int nBufferSize)
	{
		InitializeThreadpoolEnvironment(&m_WorkEnvironment);

		m_IoCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1);
		if (m_IoCompletionPort == 0)
		{
			SaveWin32Error("CreateIoCompletionPort", GetLastError());
			throw E_APIERROR;
		}
		
		m_ThreadPool = fpCreateThreadpool(0);
		if (m_ThreadPool == 0)
		{
			SaveWin32Error("CreateThreadpoolCleanupGroup", GetLastError());
			throw E_APIERROR;
		}

		if (!fpSetThreadpoolThreadMinimum(m_ThreadPool, m_MinThreads))
		{
			SaveWin32Error("SetThreadpoolThreadMinimum", GetLastError());
			throw E_APIERROR;
		}

		fpSetThreadpoolThreadMaximum(m_ThreadPool, m_MaxThreads);
		m_CleanupGroup = fpCreateThreadpoolCleanupGroup();
		if (m_CleanupGroup == 0)
		{
			SaveWin32Error("CreateThreadpoolCleanupGroup", GetLastError());
			throw E_APIERROR;
		}
		SetThreadpoolCallbackCleanupGroup(&m_WorkEnvironment, m_CleanupGroup, 0);
		SetThreadpoolCallbackPool(&m_WorkEnvironment, m_ThreadPool);

		m_GlobalContext.ResultCompletionPort = m_IoCompletionPort;
		m_GlobalContext.HsDb = pDB;
		m_GlobalContext.HsMode = nMode;
		hs_error_t hs_Result = m_GlobalContext.SetupResources(pScratch, HYPERSCAN_FILESCANCONTEXT_SIZE, nMode, nBufferSize);
		if (hs_Result != HS_SUCCESS)
		{
			SaveCustomError("hs_alloc_scratch", "Function failed.", hs_Result);
			return hs_Result;
		}
		m_FileContextPool = new(std::nothrow) CFastItemPool32<HS_FileScanContext>();
		if (m_FileContextPool == 0)
			throw E_INSUFMEMORY;

		if (nMode & HS_MODE_STREAM)
			m_GlobalContext.HsCallback = (nMode & HS_MODE_EX_LINENO) ? HS_FileScanContext::scan_stream_lineno_handler : HS_FileScanContext::scan_stream_handler;
		else
			m_GlobalContext.HsCallback = (nMode & HS_MODE_EX_LINENO) ? HS_FileScanContext::scan_block_lineno_handler : HS_FileScanContext::scan_block_handler ;
		
		for (unsigned int xj = 0; xj < m_FileContextPool->Size(); xj++)
		{
			HS_FileScanContext* lpContext = (*m_FileContextPool)[xj];
			lpContext->GlobalContext = &m_GlobalContext;
			lpContext->Buffer = m_GlobalContext.GetBuffer(xj);
			lpContext->WorkItem = fpCreateThreadpoolWork(HS_FileScanContext::scan_worker, (PVOID)lpContext, &m_WorkEnvironment);
			if (lpContext->WorkItem == 0)
			{
				SaveWin32Error("CreateThreadpoolWork", GetLastError());
				throw E_APIERROR;
			}
		}

		return HS_SUCCESS;
	}

	void SetupCursors(CStringView pSourceAlias, SHORT nSourceWorkArea, CStringView pResultAlias, SHORT nResultWorkArea, CStringView pContextField)
	{
		int nErrorNo, nFieldStatus, nRequiredFields, nFieldNo = 0;
		CStrBuilder<256> pFieldList;

		if (pResultAlias)
		{
			if (pContextField)
			{
				pFieldList = pContextField.Alltrim();
				pFieldList.Append(',');
				m_GlobalContext.MatchContext_Idx = nFieldNo++;
			}

			if (m_GlobalContext.HsMode & HS_MODE_EX_MATCH_ID)
			{
				pFieldList.Append(__sv("match_id N(10,0) NOT NULL,"));
				m_GlobalContext.MatchId_Idx = nFieldNo++;
			}

			pFieldList.Append(__sv("match_from N(20,0) NOT NULL, match_to N(20,0) NOT NULL"));
			m_GlobalContext.MatchFrom_Idx = nFieldNo++;
			m_GlobalContext.MatchTo_Idx = nFieldNo++;

			if (m_GlobalContext.HsMode & HS_MODE_EX_LINENO)
			{
				pFieldList.Append(__sv(",match_lineno N(20,0) NOT NULL"));
				m_GlobalContext.MatchLineNo_Idx = nFieldNo++;
			}

			pFieldList.Append(__sv(",match_expr M NOT NULL"));
			m_GlobalContext.MatchExpr_Idx = nFieldNo;

			m_GlobalContext.Cursor.Create(pResultAlias, pFieldList);
		}
		else
		{
			if (pContextField)
			{
				pFieldList = pContextField.Alltrim();
				pFieldList.Append(',');
				m_GlobalContext.MatchContext_Idx = nFieldNo++;
			}

			if (m_GlobalContext.HsMode & HS_MODE_EX_MATCH_ID)
			{
				pFieldList.Append(__sv("match_id"));
				m_GlobalContext.MatchId_Idx = nFieldNo++;
			}
				
			pFieldList.Append(__sv("match_from, match_to"));
			m_GlobalContext.MatchFrom_Idx = nFieldNo++;
			m_GlobalContext.MatchTo_Idx = nFieldNo++;

			if (m_GlobalContext.HsMode & HS_MODE_EX_LINENO)
			{
				pFieldList.Append(__sv(",match_lineno"));
				m_GlobalContext.MatchLineNo_Idx = nFieldNo++;
			}
				
			pFieldList.Append(__sv(",match_expr"));
			m_GlobalContext.MatchExpr_Idx = nFieldNo;

			nErrorNo = m_GlobalContext.Cursor.Attach(nResultWorkArea, pFieldList);
			if (nErrorNo)
				throw nErrorNo;
		}

		m_GlobalContext.MatchMemo.Attach(m_GlobalContext.Cursor(m_GlobalContext.MatchExpr_Idx));

		pFieldList.Len(0);
		if (pContextField)
		{
			pFieldList = pContextField.GetWordNum(1, ' ').Alltrim();
			pFieldList.Append(',');
			nRequiredFields = 3;
		}
		else
			nRequiredFields = 1;

		pFieldList.Append(__sv("filename,filesize,fileattribs"));
		if (pSourceAlias)
			nErrorNo = m_SourceCursor.Attach(pSourceAlias, pFieldList, nFieldStatus, nRequiredFields);
		else
			nErrorNo = m_SourceCursor.Attach(nSourceWorkArea, pFieldList, nFieldStatus, nRequiredFields);

		if (nErrorNo)
			throw nErrorNo;

		nFieldNo = 0;
		m_ContextField_Idx = pContextField ? nFieldNo++ : -1;
		m_FileName_Idx = nFieldNo++;
		m_FileSize_Idx = (nFieldStatus & (1 << (pContextField ? 2 : 1))) ? nFieldNo : -1;
		nFieldNo++;
		m_FileAttribs_Idx = (nFieldStatus & (1 << (pContextField ? 3 : 2))) ? nFieldNo : -1;
 	}

	hs_error_t Scan()
	{
		m_Canceled = false;
		ValueEx pMemoRef;
		ValueEx pFieldTest;
		ValueEx pFileSize;
		ValueEx pFileAttribs;
		HS_FileScanContext* pContext;
		hs_error_t result = HS_SUCCESS;
		DWORD nFileAttribs;
		LARGE_INTEGER nFileSize = { { 0, 0 } };
		char cFileNameFieldType;

		AutoRecNoRestore pRecRestore(m_SourceCursor.WorkArea(), true, true);
		m_SourceCursor.GoTop();

		pFieldTest.SetNull();
		do
		{
			pFieldTest = m_SourceCursor(m_FileName_Idx);
			cFileNameFieldType = pFieldTest.Vartype();
			pFieldTest.Release();
			if (cFileNameFieldType != '\0')
				break;

			m_SourceCursor.Skip();
		} while (!m_SourceCursor.Eof());

		while (!m_SourceCursor.Eof())
		{
			if (m_FileAttribs_Idx >= 0)
			{
				pFileAttribs = m_SourceCursor(m_FileAttribs_Idx);
				nFileAttribs = pFileAttribs.NumericCast<DWORD>();
				// filter out directory entries
				if (nFileAttribs & FILE_ATTRIBUTE_DIRECTORY)
				{
					m_SourceCursor.Skip();
					continue;
				}
			}
			if (m_FileSize_Idx >= 0)
			{
				pFileSize = m_SourceCursor(m_FileSize_Idx);
				nFileSize.QuadPart = pFileSize.NumericCast<LONGLONG>();
				// filter out empty files
				if (nFileSize.QuadPart == 0)
				{
					m_SourceCursor.Skip();
					continue;
				}
			}

		ProcessItem:
			pContext = m_FileContextPool->GetItem();
			if (pContext)
			{
				try
				{
					if (cFileNameFieldType == 'M')
					{
						pMemoRef = m_SourceCursor(m_FileName_Idx);
						if (pMemoRef.Vartype() == '\0')
						{
							m_SourceCursor.Skip();
							continue;
						}
						FoxMemo::Read(pMemoRef, pContext->FileName, true);
					}
					else
					{
						pContext->FileName = m_SourceCursor(m_FileName_Idx);
						if (pContext->FileName.Vartype() == '\0')
						{
							m_SourceCursor.Skip();
							continue;
						}
					}
					pContext->FileSize = nFileSize;

					if (m_ContextField_Idx >= 0)
						pContext->ContextValue = m_SourceCursor(m_ContextField_Idx);

					fpSubmitThreadpoolWork(pContext->WorkItem);
				}
				catch (int nErrorNo)
				{
					m_FileContextPool->ReleaseItem(pContext);
					throw nErrorNo;
				}
			}
			else
			{
				do
				{
					result = ProcessFile(false);
					if (result != HS_SUCCESS)
						return result;

				} while (m_FileContextPool->FreeCount() == 0);

				goto ProcessItem;
			}

			m_SourceCursor.Skip();
		}

		while (m_FileContextPool->FreeCount() != m_FileContextPool->Size())
		{
			result = ProcessFile(true);
			if (result != HS_SUCCESS)
				break;
		}

		return result;
	}

	void Cancel()
	{
		HS_FileScanContext* lpContext;
		if (m_IoCompletionPort && m_FileContextPool)
		{
			// cancel all pending IO operations 
			for (unsigned int xj = 0; xj < HYPERSCAN_FILESCANCONTEXT_SIZE; xj++)
			{
				lpContext = (*m_FileContextPool)[xj];
				lpContext->CancelIo();
			}

			// wait for all threadpool workers
			fpCloseThreadpoolCleanupGroupMembers(m_CleanupGroup, TRUE, 0);
		}

		m_Canceled = true; 
	}

private:

	void SaveSkippedFile(HS_FileScanContext* lpContext)
	{
		HS_GlobalContext* lpGC = lpContext->GlobalContext;
		if (lpGC->HsMode & HS_MODE_EX_STORE_SKIPPED_FILES)
		{
			FoxCursor& pCursor = lpGC->Cursor;
			pCursor.AppendBlank();

			if (lpGC->MatchContext_Idx >= 0)
				pCursor(lpGC->MatchContext_Idx) = lpContext->ContextValue;

			if (lpGC->MatchId_Idx >= 0)
				pCursor(lpGC->MatchId_Idx) = -1;

			pCursor(lpGC->MatchFrom_Idx) = lpGC->MatchValue = 0;
			pCursor(lpGC->MatchTo_Idx) = lpGC->MatchValue = 0;

			if (lpGC->MatchLineNo_Idx >= 0)
				pCursor(lpGC->MatchLineNo_Idx) = lpGC->MatchValue = 0;
		}
	}

	hs_error_t ProcessFile(bool bConsumeAll)
	{
		HS_FileScanContext* lpContext = 0;
		hs_error_t result = HS_SUCCESS;
		OVERLAPPED_ENTRY overlappedResults[HYPERSCAN_FILESCANCONTEXT_SIZE / 4];
		ULONG nEntries;

		if (fpGetQueuedCompletionStatusEx(m_IoCompletionPort, overlappedResults, countof(overlappedResults), &nEntries, 50, FALSE))
		{
			for (ULONG entry = 0; entry < nEntries; entry++)
			{
				lpContext = (HS_FileScanContext*)overlappedResults[entry].lpCompletionKey;
				m_FileContextPool->ReleaseItem(lpContext);
HandleError:
				switch (lpContext->ErrorState)
				{
					case 0:
						result = ScanFile(lpContext);
						if (result != HS_SUCCESS)
							goto HandleError;
						break;

					case ERRORSTATE_WIN32:
						if (lpContext->LastError != ERROR_ACCESS_DENIED && lpContext->LastError != ERROR_FILE_NOT_FOUND &&
							lpContext->LastError != ERROR_SHARING_VIOLATION)
						{
							SaveWin32Error(lpContext->LastErrorFunction, lpContext->LastError);
							throw E_APIERROR;
						}
						break;

					case ERRORSTATE_HYPERSCAN:
						// ignore 
						if (lpContext->HsResult != HYPERSCAN_BLOCK_BUFFER_OVERFLOW)
						{
							SaveCustomErrorEx(lpContext->LastErrorFunction, "Function failed.", lpContext->HsResult);
							return lpContext->HsResult;
						}
						else
						{
							SaveSkippedFile(lpContext);
						}
						break;

					case ERRORSTATE_VFP:
						throw (int)lpContext->LastError;

					case ERRORSTATE_POSTCOMPLETION:
						// can never happen here- noop
						break;
				}
			}
		}

		if ((bConsumeAll == true || lpContext == 0) && m_FileContextPool->FreeCount() != m_FileContextPool->Size())
		{
			while ((lpContext = m_FileContextPool->FindUsedItem(HS_FileScanContext::filter_item_postcompletion, 0)))
			{
				m_FileContextPool->ReleaseItem(lpContext);
				lpContext->ErrorState = (Hyperscan_ErrorState)(lpContext->ErrorState & ~ERRORSTATE_POSTCOMPLETION);

				switch (lpContext->ErrorState)
				{
					case ERRORSTATE_WIN32:
						if (lpContext->LastError != ERROR_ACCESS_DENIED && lpContext->LastError != ERROR_FILE_NOT_FOUND &&
							lpContext->LastError != ERROR_SHARING_VIOLATION)
						{
							SaveWin32Error(lpContext->LastErrorFunction, lpContext->LastError);
							throw E_APIERROR;
						}
						break;
					case ERRORSTATE_HYPERSCAN:
						if (lpContext->HsResult != HYPERSCAN_BLOCK_BUFFER_OVERFLOW)
						{
							SaveCustomErrorEx(lpContext->LastErrorFunction, "Function failed.", lpContext->HsResult);
							result = lpContext->HsResult;
						}
						else
						{
							SaveSkippedFile(lpContext);
						}
						break;
					case ERRORSTATE_VFP:
						throw (int)lpContext->LastError;

					case ERRORSTATE_POSTCOMPLETION:
					case ERRORSTATE_NOERROR:
						break;
				}
			}
		}

		return result;
	}

	hs_error_t ScanFile(HS_FileScanContext* lpContext)
	{
		HS_GlobalContext* lpGC = lpContext->GlobalContext;
		DWORD dwBufferSize, dwLength;
		char *buffer1, *buffer2;

		if (lpGC->HsMode & HS_MODE_BLOCK)
		{
			lpContext->HsResult = fphs_scan(lpGC->HsDb, lpContext->Buffer, lpContext->BytesRead, 0, lpGC->HsScratch, lpGC->HsCallback, (void*)lpContext);
			if (lpContext->HsResult != HS_SUCCESS)
			{
				if (lpContext->HsResult != HS_SCAN_TERMINATED)
				{
					lpContext->ErrorState = ERRORSTATE_HYPERSCAN;
					lpContext->LastErrorFunction = "hs_scan";
				}
			}
		}
		else
		{
			dwBufferSize = lpGC->BufferSize / 2;
			buffer1 = lpContext->Buffer;
			buffer2 = lpContext->Buffer + dwBufferSize;

			if (lpContext->ReadCount == 1)
				dwLength = lpContext->BytesRead;
			else
				dwLength = dwBufferSize;

			lpContext->HsResult = fphs_scan_stream(lpGC->HsStream, lpContext->Buffer, dwLength, 0, lpGC->HsScratch, lpGC->HsCallback, (void*)lpContext);
			if (lpContext->HsResult != HS_SUCCESS)
			{
				if (lpContext->HsResult != HS_SCAN_TERMINATED)
				{
					lpContext->ErrorState = ERRORSTATE_HYPERSCAN;
					lpContext->LastErrorFunction = "hs_scan_stream";
				}
				goto ErrorOut;
			}

			if (lpContext->ReadCount >= 2)
			{
				if (lpContext->ReadCount == 2)
					dwLength = lpContext->BytesRead - dwBufferSize;
				else
					dwLength = dwBufferSize;

				lpContext->LastBufferPosition = lpContext->BufferPosition;
				lpContext->BufferPosition += dwBufferSize;
				lpContext->LastBuffer = buffer1;
				lpContext->Buffer = buffer2;

				lpContext->HsResult = fphs_scan_stream(lpGC->HsStream, lpContext->Buffer, dwLength, 0, lpGC->HsScratch, lpGC->HsCallback, (void*)lpContext);
				if (lpContext->HsResult != HS_SUCCESS)
				{
					if (lpContext->HsResult != HS_SCAN_TERMINATED)
					{
						lpContext->ErrorState = ERRORSTATE_HYPERSCAN;
						lpContext->LastErrorFunction = "hs_scan_stream";
					}
					goto ErrorOut;
				}

				lpContext->LastBufferPosition = lpContext->BufferPosition;
				lpContext->BufferPosition += dwBufferSize;
				lpContext->LastBuffer = buffer2;
				lpContext->Buffer = buffer1;

				for (unsigned int xj = 3; xj <= lpContext->ReadCount; xj++)
				{
					if (xj == lpContext->ReadCount)
						dwBufferSize = lpContext->FileSize.QuadPart % dwBufferSize;

					if (!ReadFile(lpContext->FileHandle, lpContext->Buffer, dwBufferSize, &dwLength, 0))
					{
						lpContext->ErrorState = ERRORSTATE_WIN32;
						lpContext->LastError = GetLastError();
						lpContext->LastErrorFunction = "ReadFile";
						lpContext->HsResult = HS_SCAN_TERMINATED;
						goto ErrorOut;
					}

					lpContext->HsResult = fphs_scan_stream(lpGC->HsStream, lpContext->Buffer, dwLength, 0, lpGC->HsScratch, lpGC->HsCallback, (void*)lpContext);
					if (lpContext->HsResult != HS_SUCCESS)
					{
						if (lpContext->HsResult != HS_SCAN_TERMINATED)
						{
							lpContext->ErrorState = ERRORSTATE_HYPERSCAN;
							lpContext->LastErrorFunction = "hs_scan_stream";
						}
						goto ErrorOut;
					}

					if (xj != lpContext->ReadCount)
					{
						lpContext->LastBufferPosition = lpContext->BufferPosition;
						lpContext->BufferPosition += dwLength;
						lpContext->LastBuffer = lpContext->Buffer;
						lpContext->Buffer = (lpContext->Buffer == buffer1 ? buffer2 : buffer1);
					}
				}

			}

			lpContext->HsResult = fphs_reset_stream(lpGC->HsStream, 0, lpGC->HsScratch, lpGC->HsCallback, (void*)lpContext);
			if (lpContext->HsResult != HS_SUCCESS && lpContext->HsResult != HS_SCAN_TERMINATED)
			{
				lpContext->ErrorState = ERRORSTATE_HYPERSCAN;
				lpContext->LastErrorFunction = "hs_reset_stream";
			}

ErrorOut:
			lpContext->Buffer = buffer1;
		}

		return lpContext->HsResult;
	}

	HANDLE m_IoCompletionPort;
	bool m_Canceled;
	int m_ContextField_Idx,
		m_FileName_Idx,
		m_FileSize_Idx,
		m_FileAttribs_Idx;
	DWORD m_MinThreads;
	DWORD m_MaxThreads;
	FoxCursor m_SourceCursor;
	CFastItemPool32<HS_FileScanContext>* m_FileContextPool;
	HS_GlobalContext m_GlobalContext;
	TP_CALLBACK_ENVIRON m_WorkEnvironment;
	PTP_POOL m_ThreadPool;
	PTP_CLEANUP_GROUP m_CleanupGroup;
};

void _fastcall Hyperscan_scan_files(ParamBlkEx& parm)
{
	if (int nError = VFP2C_Init_HyperScan())
	{
		RaiseError(nError);
		return;
	}

	HyperScan_FileScan *hs_Filescanner = 0;
	hs_database* hs_Db = parm(1)->DynamicPtr<hs_database*>();
	hs_scratch* hs_Scratch = parm(2)->DynamicPtr<hs_scratch*>();
	unsigned int hs_Mode = parm(3)->ev_long;
	FoxParameterEx& pSource = parm(4);
	FoxParameterEx& pResult = parm(5);
	unsigned int hs_BufferSize = parm.PCount() >= 7 ? parm(7)->NumericCast<unsigned int>() : 0;
	hs_error_t hs_Result;

	try
	{
		FoxString pRegEx;
		FoxString pSourceAlias;
		FoxString pResultAlias;
		FoxString pContextField(parm, 6);
		SHORT nSourceWorkArea = 0, nResultWorkArea = 0;

		if (pSource->Vartype() == 'C')
			pSourceAlias.Attach(pSource, 0);
		else if (pSource->Numeric())
			nSourceWorkArea = pSource->NumericCast<SHORT>();
		else
		{
			SaveCustomErrorEx("Hyperscan_scan", "Parameter 'cnSource' has to be a cursor name or a workarea number.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}

		if (pResult->Vartype() == 'C')
			pResultAlias.Attach(pResult, 0);
		else if (pResult->Numeric())
			nResultWorkArea = pResult->NumericCast<SHORT>();
		else
		{
			SaveCustomErrorEx("Hyperscan_scan", "Parameter 'cnResult' has to be a cursor name or a workarea number.", E_VFP_INVALIDPARAMS);
			throw E_APIERROR;
		}

		hs_Filescanner = new(std::nothrow) HyperScan_FileScan();
		if (!hs_Filescanner)
			throw E_INSUFMEMORY;

		hs_Result = hs_Filescanner->Setup(hs_Db, hs_Scratch, hs_Mode, hs_BufferSize);
		if (hs_Result == HS_SUCCESS)
		{
			hs_Filescanner->SetupCursors(pSourceAlias, nSourceWorkArea, pResultAlias, nResultWorkArea , pContextField);
			hs_Result = hs_Filescanner->Scan();
		}

		Return(hs_Result);
	}
	catch (int nError)
	{
		if (hs_Filescanner)
			hs_Filescanner->Cancel();

		RaiseError(nError);
	}

	if (hs_Filescanner)
		delete hs_Filescanner;
}