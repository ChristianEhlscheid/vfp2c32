#ifndef _VFPHYPERSCAN_H__
#define _VFPHYPERSCAN_H__

#include "hs.h"

enum Hyperscan_ErrorState
{
	ERRORSTATE_NOERROR = 0x00,
	ERRORSTATE_WIN32 = 0x01,
	ERRORSTATE_VFP = 0x02,
	ERRORSTATE_HYPERSCAN = 0x04,
	ERRORSTATE_POSTCOMPLETION = 0x08
};

#define HYPERSCAN_FILESCANCONTEXT_SIZE		32
#define HYPERSCAN_LINENO_EXPR_ID			(~(0u))
#define HYPERSCAN_HORIZON_SMALL_BUFFERSIZE	(64ull * 1024ull)
#define HYPERSCAN_BLOCK_BUFFER_OVERFLOW		(-20)

#define HS_MODE_EX_LINENO					0x00000100
#define HS_MODE_EX_MATCH_ID					0x00000200
#define HS_MODE_EX_LITERAL					0x00000400
#define HS_MODE_EX_SKIP_OVERLAPPED_EXPR		0x00000800
#define HS_MODE_EX_STORE_SKIPPED_FILES		0x00001000

#define MAX_WIDE_PATH					32767

#ifndef LFILE_UNICODE_EXTENSION
#define LFILE_UNICODE_EXTENSION			L"\\\\?\\"
#endif

typedef ULONG(_stdcall* PRTLNTSTATUSTODOSERROR)(LONG status);
typedef BOOL(_stdcall* PCANCELIOEX)(HANDLE hFile, LPOVERLAPPED lpOverlapped);
typedef BOOL(_stdcall* PGETQUEUEDCOMPLETIONSTATUSEX)(HANDLE CompletionPort, LPOVERLAPPED_ENTRY lpCompletionPortEntries, ULONG ulCount, PULONG ulNumEntriesRemoved, DWORD dwMilliseconds, BOOL fAlertable);

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

typedef hs_error_t(HS_CDECL* phs_populate_platform)(hs_platform_info_t* platform);
typedef hs_error_t(HS_CDECL* phs_set_allocator)(hs_alloc_t alloc_func, hs_free_t free_func);
typedef hs_error_t(HS_CDECL* phs_compile)(const char* expression, unsigned int flags, unsigned int mode, const hs_platform_info_t* platform, hs_database_t** db, hs_compile_error_t** error);
typedef hs_error_t(HS_CDECL* phs_compile_multi)(const char* const* expressions, const unsigned int* flags, const unsigned int* ids, unsigned int elements, unsigned int mode, const hs_platform_info_t* platform,
	hs_database_t** db, hs_compile_error_t** error);
typedef hs_error_t(HS_CDECL* phs_compile_ext_multi)(const char* const* expressions, const unsigned int* flags, const unsigned int* ids, const hs_expr_ext_t* ext, unsigned int elements, unsigned int mode,
	const hs_platform_info_t* platform, hs_database_t** db, hs_compile_error_t** error);
typedef hs_error_t(HS_CDECL* phs_compile_lit)(const char* expression, unsigned flags, const size_t len, unsigned mode, const hs_platform_info_t* platform, hs_database_t** db, hs_compile_error_t** error);
typedef hs_error_t(HS_CDECL* phs_compile_lit_multi)(const char* const* expressions, const unsigned* flags, const unsigned* ids, const size_t* lens, unsigned elements, unsigned mode, const hs_platform_info_t* platform,
	hs_database_t** db, hs_compile_error_t** error);
typedef hs_error_t(HS_CDECL* phs_free_compile_error)(hs_compile_error_t* error);
typedef hs_error_t(HS_CDECL* phs_alloc_scratch)(const hs_database_t* db, hs_scratch_t** scratch);
typedef hs_error_t(HS_CDECL* phs_clone_scratch)(hs_scratch_t* scratch, hs_scratch_t** clone);
typedef hs_error_t(HS_CDECL* phs_free_scratch)(hs_scratch_t* scratch);
typedef hs_error_t(HS_CDECL* phs_scan)(const hs_database_t* db, const char* data, unsigned int length, unsigned int flags, hs_scratch_t* scratch, match_event_handler onEvent, void* context);
typedef hs_error_t(HS_CDECL* phs_free_database)(hs_database_t* db);
typedef hs_error_t(HS_CDECL* phs_open_stream)(const hs_database_t* db, unsigned int flags, hs_stream_t** stream);
typedef hs_error_t(HS_CDECL* phs_scan_stream)(hs_stream_t* id, const char* data, unsigned int length, unsigned int flags, hs_scratch_t* scratch, match_event_handler onEvent, void* ctxt);
typedef hs_error_t(HS_CDECL* phs_close_stream)(hs_stream_t* id, hs_scratch_t* scratch, match_event_handler onEvent, void* ctxt);
typedef hs_error_t(HS_CDECL* phs_reset_stream)(hs_stream_t* id, unsigned int flags, hs_scratch_t* scratch, match_event_handler onEvent, void* context);

#ifdef __cplusplus
extern "C" {
#endif

void _fastcall Hyperscan_platform(ParamBlkEx& parm);
void _fastcall Hyperscan_compile(ParamBlkEx& parm);
void _fastcall Hyperscan_free_database(ParamBlkEx& parm);
void _fastcall Hyperscan_alloc_scratch(ParamBlkEx& parm);
void _fastcall Hyperscan_free_scratch(ParamBlkEx& parm);
void _fastcall Hyperscan_scan(ParamBlkEx& parm);
void _fastcall Hyperscan_scan_cursor(ParamBlkEx& parm);
void _fastcall Hyperscan_scan_files(ParamBlkEx& parm);

#ifdef __cplusplus
}
#endif

#endif
