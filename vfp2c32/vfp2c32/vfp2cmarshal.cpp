#include <windows.h>
#include <stdio.h> // sprintf, memcpy and other common C library routines

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2cstring.h"
#include "vfp2cmarshal.h"
#include "vfp2ccppapi.h"
#include "vfp2ctls.h"

int _stdcall VFP2C_Init_Marshal()
{
	// create default Heap
	if (VFP2CTls::Heap == 0)
	{
		if (!(VFP2CTls::Heap = HeapCreate(0, HEAP_INITIAL_SIZE, 0)))
		{
			AddWin32Error("HeapCreate", GetLastError());
			return E_APIERROR;
		}
		// we can use GetModuleHandle instead of LoadLibrary since kernel32.dll is loaded already by VFP
		HMODULE hDll = GetModuleHandle("kernel32.dll");
		if (hDll)
		{
			PHEAPSETINFO fpHeapSetInformation = (PHEAPSETINFO)GetProcAddress(hDll,"HeapSetInformation");
			// if HeapSetInformation is supported (only on WinXP), call it to make our heap a low-fragmentation heap.
			if (fpHeapSetInformation)
			{
				ULONG nLFHFlag = 2;
				fpHeapSetInformation(VFP2CTls::Heap, HeapCompatibilityInformation, &nLFHFlag, sizeof(ULONG));
			}
		}
	}

	return 0;
}

void _stdcall VFP2C_Destroy_Marshal(VFP2CTls& tls)
{
#ifdef _DEBUG
	FreeDebugAlloc();
#endif
}

int _stdcall Win32HeapExceptionHandler(int nExceptionCode)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	LPVFP2CERROR pError = tls.ErrorInfo;
	tls.ErrorCount = 0;
	pError->nErrorType = VFP2C_ERRORTYPE_WIN32;
	pError->nErrorNo = nExceptionCode;
	strcpy(pError->aErrorFunction,"HeapAlloc/HeapReAlloc");

	if (nExceptionCode == STATUS_NO_MEMORY)
		strcpy(pError->aErrorMessage,"The allocation attempt failed because of a lack of available memory or heap corruption.");
	else if (nExceptionCode == STATUS_ACCESS_VIOLATION)
		strcpy(pError->aErrorMessage,"The allocation attempt failed because of heap corruption or improper function parameters.");
	else
		strcpy(pError->aErrorMessage,"Unknown exception code.");

	return EXCEPTION_EXECUTE_HANDLER;
}


#ifdef _DEBUG

void _stdcall AddDebugAlloc(void* pPointer, int nSize)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	FoxValue vProgInfo;
	LPDBGALLOCINFO pDbg;	
	char *pProgInfo = 0;

	if (pPointer && tls.TrackAlloc)
	{
		pDbg = (LPDBGALLOCINFO)malloc(sizeof(DBGALLOCINFO));
		if (!pDbg)
			return;

		_Evaluate(vProgInfo, "ALLTRIM(STR(LINENO())) + ':' + PROGRAM() + CHR(0)");
		if (vProgInfo.Vartype() == 'C')
		{
			pProgInfo = _strdup(vProgInfo.HandleToPtr());
			vProgInfo.Release();
		}
		else
		{
			pProgInfo = 0;
		}

		pDbg->pPointer = pPointer;
		pDbg->pProgInfo = pProgInfo;
		pDbg->nSize = nSize;
		pDbg->next = tls.DbgInfo;
		tls.DbgInfo = pDbg;
	}
}

void _stdcall RemoveDebugAlloc(void* pPointer)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	if (pPointer && tls.TrackAlloc)
	{
		LPDBGALLOCINFO pDbg = tls.DbgInfo, pDbgPrev = 0;
		while (pDbg && pDbg->pPointer != pPointer)
		{
			pDbgPrev = pDbg;
			pDbg = pDbg->next;
		}

		if (pDbg)
		{
			if (pDbgPrev)
				pDbgPrev->next = pDbg->next;
			else
				tls.DbgInfo = pDbg->next;

			if (pDbg->pProgInfo)
				free(pDbg->pProgInfo);
			free(pDbg);
		}
	}
}

void _stdcall ReplaceDebugAlloc(void* pOrig, void* pNew, int nSize)
{
   	VFP2CTls& tls = VFP2CTls::Tls();
	if (!pNew || !tls.TrackAlloc)
		return;

	LPDBGALLOCINFO pDbg = tls.DbgInfo;
	FoxValue vProgInfo;
	char* pProgInfo = 0;

	while (pDbg && pDbg->pPointer != pOrig)
		pDbg = pDbg->next;

    if (pDbg)
	{
		_Evaluate(vProgInfo, "ALLTRIM(STR(LINENO())) + ':' + PROGRAM() + CHR(0)");
		if (vProgInfo.Vartype() == 'C')
		{
			pProgInfo = _strdup(vProgInfo.HandleToPtr());
			vProgInfo.Release();
		}
		else
		{
			pProgInfo = 0;
		}
			

		if (pDbg->pProgInfo)
			free(pDbg->pProgInfo);

		pDbg->pPointer = pNew;
		pDbg->pProgInfo = pProgInfo;
		pDbg->nSize = nSize;
	}
}

void _stdcall FreeDebugAlloc()
{
	VFP2CTls& tls = VFP2CTls::Tls();
	LPDBGALLOCINFO pDbg = tls.DbgInfo, pDbgEx;
	while (pDbg)
	{
		pDbgEx = pDbg->next;
		if (pDbg->pProgInfo)
			free(pDbg->pProgInfo);
		free(pDbg);
		pDbg = pDbgEx;
	}
	tls.DbgInfo = 0;
}

void _fastcall AMemLeaks(ParamBlkEx& parm)
{
try
{
	VFP2CTls& tls = VFP2CTls::Tls();
	LPDBGALLOCINFO pDbg = tls.DbgInfo;
	if (!pDbg)
	{
		Return(0);
		return;
	}

	FoxArray aMemLeaks(parm(1));
	FoxString vMemInfo(VFP2C_ERROR_MESSAGE_LEN);
	int nRows = 0;

	aMemLeaks.Dimension(1,4);

	while (pDbg)
	{
		nRows++;
		aMemLeaks.Dimension(nRows, 4);

		aMemLeaks(nRows, 1) = pDbg->pPointer;
		aMemLeaks(nRows, 2) = pDbg->nSize;
		aMemLeaks(nRows, 3) = vMemInfo = pDbg->pProgInfo;

		vMemInfo.Len(min(pDbg->nSize,VFP2C_ERROR_MESSAGE_LEN));
		memcpy(vMemInfo.Ptr(), pDbg->pPointer, vMemInfo.Len());
		aMemLeaks(nRows, 4) = vMemInfo;

		pDbg = pDbg->next;
	}

	aMemLeaks.ReturnRows();
	return;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}	
}

void _fastcall TrackMem(ParamBlkEx& parm)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	tls.TrackAlloc = static_cast<BOOL>(parm(1)->ev_length);
	if (parm.PCount() == 2 && parm(2)->ev_length)
		FreeDebugAlloc();
}

#endif // DEBUG

// FLL memory allocation functions using FLL's standard heap
void _fastcall AllocMem(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	void *pAlloc = 0;
	__try
	{
		pAlloc = HeapAlloc(VFP2CTls::Heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, parm(1)->ev_long);
	}
	__except(SAVEHEAPEXCEPTION()) { }

	if (pAlloc)
	{
		ADDDEBUGALLOC(pAlloc, parm(1)->ev_long);
		Return(pAlloc);
	}
	else
		RaiseError(E_APIERROR);
}

void _fastcall AllocMemTo(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	void** pPointer = parm(1)->Ptr<void**>();
	void *pAlloc = 0;

	if (pPointer)
	{
		__try
		{
			pAlloc = HeapAlloc(VFP2CTls::Heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, parm(2)->ev_long);
		}
		__except(SAVEHEAPEXCEPTION()) { }
	
		if (pAlloc)
		{
			*pPointer = pAlloc;
			ADDDEBUGALLOC(pAlloc,parm(2)->ev_long);
			Return(pAlloc);
		}
		else
			RaiseError(E_APIERROR);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReAllocMem(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	void *pPointer = parm(1)->Ptr();
	void *pAlloc = 0;
	__try
	{
		if (pPointer)
		{
			pAlloc = HeapReAlloc(VFP2CTls::Heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, pPointer, parm(2)->ev_long);
			REPLACEDEBUGALLOC(pPointer, pAlloc, parm(2)->ev_long);
		}
		else
		{
			pAlloc = HeapAlloc(VFP2CTls::Heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, parm(2)->ev_long);
			ADDDEBUGALLOC(pAlloc, parm(2)->ev_long);
		}
    }
	__except(SAVEHEAPEXCEPTION()) { }

	if (pAlloc)
		Return(pAlloc);
	else
		RaiseError(E_APIERROR);
}

void _fastcall FreeMem(ParamBlkEx& parm)
{
	void* pPointer = parm(1)->Ptr<void*>();
	if (pPointer)
	{
		if (HeapFree(VFP2CTls::Heap,0, pPointer))
		{
			REMOVEDEBUGALLOC(pPointer);
		}
		else
		{
			SaveWin32Error("HeapFree", GetLastError());
			RaiseError(E_APIERROR);
		}
	}
}

void _fastcall FreePMem(ParamBlkEx& parm)
{
	void* pAlloc;
	if (parm(1)->ev_long)
	{
		if ((pAlloc = *parm(1)->Ptr<void**>()))
		{
			if (HeapFree(VFP2CTls::Heap, 0, pAlloc))
			{
				REMOVEDEBUGALLOC(pAlloc);
			}
			else
			{
				SaveWin32Error("HeapFree", GetLastError());
				RaiseError(E_APIERROR);
			}
		}
	}
}

void _fastcall FreeRefArray(ParamBlkEx& parm)
{
	void **pAddress;
	int nStartElement, nElements;
	BOOL bApiRet = TRUE;

	if (parm(2)->ev_long < 1 || parm(2)->ev_long > parm(3)->ev_long)
		RaiseError(E_INVALIDPARAMS);

	pAddress = parm(1)->Ptr<void**>();
	nStartElement = --parm(2)->ev_long;
	nElements = parm(3)->ev_long;
	pAddress += nStartElement;
	nElements -= nStartElement;
	
	while(nElements--)
	{
		if (*pAddress)
		{
			if (!HeapFree(VFP2CTls::Heap, 0, *pAddress))
			{
				AddWin32Error("HeapFree", GetLastError());
				bApiRet = FALSE;
			}
		}
		pAddress++;
	}
	Return(bApiRet == TRUE);
}

void _fastcall SizeOfMem(ParamBlkEx& parm)
{
	if (parm(1)->ev_long)
		Return((int)HeapSize(VFP2CTls::Heap, 0, parm(1)->Ptr<void**>()));
	else
		Return(0);
}

void _fastcall ValidateMem(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	Return(HeapValidate(VFP2CTls::Heap, 0, parm(1)->Ptr<void**>()) > 0);
}

void _fastcall CompactMem(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	SIZE_T ret = HeapCompact(VFP2CTls::Heap, 0);
	Return(ret);
}

// wrappers around GlobalAlloc, GlobalFree etc .. for movable memory objects ..
void _fastcall AllocHGlobal(ParamBlkEx& parm)
{
	HGLOBAL hMem;
	UINT nFlags = parm.PCount() == 2 ? parm(2)->ev_long : GMEM_MOVEABLE | GMEM_ZEROINIT;
	hMem = GlobalAlloc(nFlags, parm(1)->ev_long);
	if (hMem)
	{
		ADDDEBUGALLOC(hMem, parm(1)->ev_long);
		Return(hMem);
	}
	else
	{
		SaveWin32Error("GlobalAlloc", GetLastError());
		RaiseError(E_APIERROR);
	}
}

void _fastcall FreeHGlobal(ParamBlkEx& parm)
{
	HGLOBAL hHandle = parm(1)->Ptr<HGLOBAL>();
	if (hHandle)
	{
		if (GlobalFree(hHandle) == 0)
		{
			REMOVEDEBUGALLOC(hHandle);
		}
		else
		{
			SaveWin32Error("GlobalFree", GetLastError());
			RaiseError(E_APIERROR);
		}
	}
}

void _fastcall ReAllocHGlobal(ParamBlkEx& parm)
{
	HGLOBAL hHandle = parm(1)->Ptr<HGLOBAL>();
	UINT nFlags = parm.PCount() == 2 ? GMEM_ZEROINIT : parm(3)->ev_long;
	HGLOBAL hMem;
	hMem = GlobalReAlloc(hHandle, parm(2)->ev_long, nFlags);
	if (hMem)
	{
		REPLACEDEBUGALLOC(parm(1)->Ptr<void*>(), hMem, parm(2)->ev_long);
		Return(hMem);
	}
	else
	{
		SaveWin32Error("GlobalReAlloc", GetLastError());
		RaiseError(E_APIERROR);
	}
}

void _fastcall LockHGlobal(ParamBlkEx& parm)
{
	HGLOBAL hHandle = parm(1)->Ptr<HGLOBAL>();
	LPVOID pMem;
	pMem = GlobalLock(hHandle);
	if (pMem)
		Return(pMem);
	else
	{
		SaveWin32Error("GlobalLock", GetLastError());
		RaiseError(E_APIERROR);
	}
}

void _fastcall UnlockHGlobal(ParamBlkEx& parm)
{
	HGLOBAL hHandle = parm(1)->Ptr<HGLOBAL>();
	BOOL bRet;
	DWORD nLastError;
	
	bRet = GlobalUnlock(hHandle);
	if (!bRet)
	{
		nLastError = GetLastError();
		if (nLastError == NO_ERROR)
			Return(1);
		else
		{
			SaveWin32Error("GlobalUnlock", nLastError);
			RaiseError(E_APIERROR);
		}
	}
	else
		Return(2);
}

void _fastcall AMemBlocks(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1) ,1,3);
	PROCESS_HEAP_ENTRY pEntry;
	DWORD nLastError;
	HANDLE hHeap = VFP2CTls::Heap;

	pEntry.lpData = NULL;

	if (!HeapWalk(hHeap, &pEntry))
	{
		nLastError = GetLastError();
		if (nLastError == ERROR_NO_MORE_ITEMS)
			Return(0);
		else
		{
			SaveWin32Error("HeapWalk", nLastError);
			throw E_APIERROR;
		}
		return;
	}

	pArray.AutoGrow(16);
	unsigned int nRow;
	do 
	{
		nRow = pArray.Grow();
		pArray(nRow,1) = pEntry.lpData;
		pArray(nRow,2) = pEntry.cbData;
		pArray(nRow,3) = pEntry.cbOverhead;
	} while (HeapWalk(hHeap, &pEntry));
	
	nLastError = GetLastError();
    if (nLastError != ERROR_NO_MORE_ITEMS)
	{
		SaveWin32Error("HeapWalk", nLastError);
		throw E_APIERROR;
	}
	else
		pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall WriteInt8(ParamBlkEx& parm)
{
	__int8 *pPointer = parm(1)->Ptr<__int8*>();
	if (pPointer)
		*pPointer = static_cast<__int8>(parm(2)->ev_long);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePInt8(ParamBlkEx& parm)
{
	__int8** pPointer = parm(1)->Ptr<__int8**>();
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<__int8>(parm(2)->ev_long);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteUInt8(ParamBlkEx& parm)
{
	unsigned __int8 *pPointer = parm(1)->Ptr<unsigned __int8*>();
	if (pPointer)
		*pPointer = static_cast<unsigned __int8>(parm(2)->ev_long);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePUInt8(ParamBlkEx& parm)
{
	unsigned __int8 **pPointer = parm(1)->Ptr<unsigned __int8**>();
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<unsigned __int8>(parm(2)->ev_long);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteShort(ParamBlkEx& parm)
{
	short* pPointer = parm(1)->Ptr<short*>();
	if (pPointer)
		*pPointer = static_cast<short>(parm(2)->ev_long);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePShort(ParamBlkEx& parm)
{
	short **pPointer = parm(1)->Ptr<short**>();
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<short>(parm(2)->ev_long);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteUShort(ParamBlkEx& parm)
{
	unsigned short *pPointer = parm(1)->Ptr<unsigned short*>();
	if (pPointer)
		*pPointer = static_cast<unsigned short>(parm(2)->ev_long);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePUShort(ParamBlkEx& parm)
{
	unsigned short **pPointer = parm(1)->Ptr<unsigned short**>(); 
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<unsigned short>(parm(2)->ev_long);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteInt(ParamBlkEx& parm)
{
	int *pPointer = parm(1)->Ptr<int*>();
	if (pPointer)
		*pPointer = parm(2)->ev_long;
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePInt(ParamBlkEx& parm)
{
	int **pPointer = parm(1)->Ptr<int**>();
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = parm(2)->ev_long;
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteUInt(ParamBlkEx& parm)
{
	unsigned int *pPointer = parm(1)->Ptr<unsigned int*>();
	if (pPointer)
		*pPointer = static_cast<unsigned int>(parm(2)->ev_real);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePUInt(ParamBlkEx& parm)
{
	unsigned int **pPointer = parm(1)->Ptr<unsigned int**>();
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<unsigned int>(parm(2)->ev_real);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteInt64(ParamBlkEx& parm)
{
try
{
	__int64 *pPointer = parm(1)->Ptr<__int64*>();
	if (pPointer)
		*pPointer = Value2Int64(parm(2));
	else
		throw E_INVALIDPARAMS;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall WritePInt64(ParamBlkEx& parm)
{
try
{
	__int64 **pPointer = parm(1)->Ptr<__int64**>();
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = Value2Int64(parm(2));
	}
	else
		throw E_INVALIDPARAMS;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall WriteUInt64(ParamBlkEx& parm)
{
try
{
	unsigned __int64 *pPointer = parm(1)->Ptr<unsigned __int64*>();
	if (pPointer)
		*pPointer = Value2UInt64(parm(2));
	else
		throw E_INVALIDPARAMS;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall WritePUInt64(ParamBlkEx& parm)
{
try
{
	unsigned __int64 **pPointer = parm(1)->Ptr<unsigned __int64**>();
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = Value2UInt64(parm(2));
	}
	else
		throw E_INVALIDPARAMS;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall WriteFloat(ParamBlkEx& parm)
{
	float *pPointer = parm(1)->Ptr<float*>();
	if (pPointer)
		*pPointer = static_cast<float>(parm(2)->ev_real);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePFloat(ParamBlkEx& parm)
{
	float **pPointer = parm(1)->Ptr<float**>();
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<float>(parm(2)->ev_real);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteDouble(ParamBlkEx& parm)
{
	double *pPointer = parm(1)->Ptr<double*>();
	if (pPointer)
		*pPointer = parm(2)->ev_real;
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePDouble(ParamBlkEx& parm)
{
	double **pPointer = parm(1)->Ptr<double**>();
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = parm(2)->ev_real;
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteLogical(ParamBlkEx& parm)
{
	unsigned int *pPointer = parm(1)->Ptr<unsigned int*>();
	if (pPointer)
        *pPointer = parm(2)->ev_length;
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePLogical(ParamBlkEx& parm)
{
	unsigned int **pPointer = parm(1)->Ptr<unsigned int**>();
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = parm(2)->ev_length;
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePointer(ParamBlkEx& parm)
{
	void **pPointer = parm(1)->Ptr<void**>();
	void* nValue = parm(2)->Ptr();
	if (pPointer)
		*pPointer = nValue;
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePPointer(ParamBlkEx& parm)
{
	void ***pPointer = parm(1)->Ptr<void***>();
	void* nValue = parm(2)->Ptr();
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = nValue;
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteChar(ParamBlkEx& parm)
{
	char* pChar = parm(1)->Ptr<char*>();
	if (pChar)
		*pChar = *parm(2)->HandleToPtr();
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePChar(ParamBlkEx& parm)
{
	char **pChar = parm(1)->Ptr<char**>();
	if (pChar)
	{
		if ((*pChar))
            **pChar = *parm(2)->HandleToPtr();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteWChar(ParamBlkEx& parm)
{
	wchar_t *pString = parm(1)->Ptr<wchar_t*>();
	unsigned int nCodePage = parm.PCount() == 2 ? VFP2CTls::Tls().ConvCP : parm(3)->ev_long;
	if (pString)
	{
		if (parm(2)->ev_length)
			MultiByteToWideChar(nCodePage, 0, parm(2)->HandleToPtr(), 1, pString, 1);
		else
			*pString = L'\0';
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePWChar(ParamBlkEx& parm)
{
	wchar_t** pString = parm(1)->Ptr<wchar_t**>();
	unsigned int nCodePage = parm.PCount() == 2 ? VFP2CTls::Tls().ConvCP : parm(3)->ev_long;
	if (pString)
	{
		if ((*pString))
		{
			if (parm(2)->ev_length)
				MultiByteToWideChar(nCodePage, 0, parm(2)->HandleToPtr(), 1, *pString, 1);
			else
				**pString = L'\0';
		}
	}
}

void _fastcall WriteCString(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	char* pPointer = parm(1)->Ptr<char*>();
	char *pNewAddress = 0;

	__try
	{
		if (pPointer)
		{
			pNewAddress = (char*)HeapReAlloc(VFP2CTls::Heap, HEAP_GENERATE_EXCEPTIONS, pPointer, parm(2)->ev_length+1);
			REPLACEDEBUGALLOC(pPointer, pNewAddress, parm(2)->ev_length+1);
		}
		else
		{
			pNewAddress = (char*)HeapAlloc(VFP2CTls::Heap, HEAP_GENERATE_EXCEPTIONS, parm(2)->ev_length+1);
			ADDDEBUGALLOC(pNewAddress, parm(2)->ev_length+1);
		}
	}
	__except(SAVEHEAPEXCEPTION()) { }

	if (pNewAddress)
	{
		memcpy(pNewAddress, parm(2)->HandleToPtr(), parm(2)->ev_length);
		pNewAddress[parm(2)->ev_length] = '\0';
		Return((void*)pNewAddress);
	}
	else
		RaiseError(E_APIERROR);
}

void _fastcall WriteGPCString(ParamBlkEx& parm)
{
	char *pNewAddress = 0;
	HGLOBAL *pOldAddress = parm(1)->Ptr<HGLOBAL*>();
	SIZE_T dwLen = parm(2)->ev_length + 1;

	if (parm(2)->Vartype() == 'C' && pOldAddress)
	{
		if ((*pOldAddress))
		{
			pNewAddress = (char*)GlobalReAlloc(*pOldAddress, dwLen, GMEM_FIXED);
			if (pNewAddress == 0)
			{
				SaveWin32Error("GlobalReAlloc", GetLastError());
				RaiseError(E_APIERROR);
			}
			REPLACEDEBUGALLOC(pOldAddress, pNewAddress, dwLen);
		}
		else
		{
			pNewAddress = (char*)GlobalAlloc(GMEM_FIXED, dwLen);
			if (pNewAddress == 0)
			{
				SaveWin32Error("GlobalAlloc", GetLastError());
				RaiseError(E_APIERROR);
			}
			ADDDEBUGALLOC(pNewAddress, dwLen);
		}

		*pOldAddress = pNewAddress;
		memcpy(pNewAddress, parm(2)->HandleToPtr(),parm(2)->ev_length);
		pNewAddress[parm(2)->ev_length] = '\0';		
		Return((void*)pNewAddress);
	}
	else if (parm(2)->Vartype() == '0' && pOldAddress)
	{
		if ((*pOldAddress))
		{
			if (GlobalFree(*pOldAddress) == 0)
			{
				REMOVEDEBUGALLOC(*pOldAddress);
				*pOldAddress = 0;
				Return(0);
			}
			else
			{
				SaveWin32Error("GlobalFree", GetLastError());
				RaiseError(E_APIERROR);
			}
		}
		else
			Return(0);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePCString(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	char *pNewAddress = 0;
	char **pOldAddress = parm(1)->Ptr<char**>();

	if (parm(2)->Vartype() == 'C' && pOldAddress)
	{
		__try
		{
			if ((*pOldAddress))
			{
				pNewAddress = (char*)HeapReAlloc(VFP2CTls::Heap, HEAP_GENERATE_EXCEPTIONS, (*pOldAddress), parm(2)->ev_length+1);
				REPLACEDEBUGALLOC(*pOldAddress, pNewAddress, parm(2)->ev_length);
			}
			else
			{
				pNewAddress = (char*)HeapAlloc(VFP2CTls::Heap, HEAP_GENERATE_EXCEPTIONS, parm(2)->ev_length+1);
				ADDDEBUGALLOC(pNewAddress,parm(2)->ev_length);
			}
		}
		__except(SAVEHEAPEXCEPTION()) { }

		if (pNewAddress)
		{
			*pOldAddress = pNewAddress;
			memcpy(pNewAddress,parm(2)->HandleToPtr(),parm(2)->ev_length);
			pNewAddress[parm(2)->ev_length] = '\0';		
			Return((void*)pNewAddress);
		}
		else
			RaiseError(E_APIERROR);
	}
	else if (parm(2)->Vartype() == '0' && pOldAddress)
	{
		if ((*pOldAddress))
		{
			if (HeapFree(VFP2CTls::Heap,0,*pOldAddress))
			{
				REMOVEDEBUGALLOC(*pOldAddress);
				*pOldAddress = 0;
				Return(0);
			}
			else
			{
				SaveWin32Error("HeapFree", GetLastError());
				RaiseError(E_APIERROR);
			}
		}
		else
			Return(0);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteCharArray(ParamBlkEx& parm)
{
	char* pPointer = parm(1)->Ptr<char*>();
	if (pPointer)
	{
		if (parm.PCount() == 2 || (long)parm(2)->ev_length < parm(3)->ev_long)
		{
			memcpy(pPointer, parm(2)->HandleToPtr(),parm(2)->ev_length);
			pPointer[parm(2)->ev_length] = '\0';
		}
		else
		{
			memcpy(pPointer, parm(2)->HandleToPtr(), parm(3)->ev_long);
			pPointer[parm(3)->ev_long-1] = '\0';
		}
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteWString(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	wchar_t *pString = parm(1)->Ptr<wchar_t*>();
	unsigned int nCodePage = parm.PCount() == 2 ? VFP2CTls::Tls().ConvCP : parm(3)->ev_long;
	wchar_t *pDest = 0;
	int nStringLen, nBytesNeeded, nBytesWritten;	
	nStringLen = parm(2)->ev_length;
	nBytesNeeded = nStringLen * sizeof(wchar_t) + sizeof(wchar_t);

	__try
	{
		if (pString)
		{
			pDest = (wchar_t*)HeapReAlloc(VFP2CTls::Heap, HEAP_GENERATE_EXCEPTIONS, pString, nBytesNeeded);
			REPLACEDEBUGALLOC(pString, pDest, nBytesNeeded);
		}
		else
		{
			pDest = (wchar_t*)HeapAlloc(VFP2CTls::Heap, HEAP_GENERATE_EXCEPTIONS, nBytesNeeded);
			ADDDEBUGALLOC(pDest, nBytesNeeded);
		}
	}
	__except(SAVEHEAPEXCEPTION()) { }

	if (pDest)
	{
		if (nStringLen)
		{
			nBytesWritten = MultiByteToWideChar(nCodePage, 0, parm(2)->HandleToPtr(), nStringLen, pDest, nBytesNeeded);
			if (nBytesWritten)
				pDest[nBytesWritten] = L'\0';
			else
				RaiseWin32Error("MultiByteToWideChar", GetLastError());
		}
		else
			*pDest = L'\0';

		Return((void*)pDest);
	}
	else
		RaiseError(E_APIERROR);
}

void _fastcall WritePWString(ParamBlkEx& parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	wchar_t **pOld = parm(1)->Ptr<wchar_t**>();
	wchar_t *pDest = 0;
	unsigned int nCodePage = parm.PCount() == 2 ? VFP2CTls::Tls().ConvCP : parm(3)->ev_long;
	int nStringLen, nBytesNeeded, nBytesWritten;

	if (parm(2)->Vartype() == 'C' && pOld)
	{
		nStringLen = parm(2)->ev_length;
		nBytesNeeded = nStringLen * sizeof(wchar_t) + sizeof(wchar_t);

		__try
		{
			if ((*pOld))
			{
				pDest = (wchar_t*)HeapReAlloc(VFP2CTls::Heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,*pOld,nBytesNeeded);
				REPLACEDEBUGALLOC(*pOld,pDest,nBytesNeeded);
			}
			else
			{
				pDest = (wchar_t*)HeapAlloc(VFP2CTls::Heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS,nBytesNeeded);
				ADDDEBUGALLOC(pDest,nBytesNeeded);
			}
		}
		__except(SAVEHEAPEXCEPTION()) { }

		if (pDest)
		{
			nBytesWritten = MultiByteToWideChar(nCodePage, 0, parm(2)->HandleToPtr(), nStringLen, pDest, nBytesNeeded);
			if (nBytesWritten)
			{
				pDest[nBytesWritten] = L'\0';
				*pOld = pDest;
			}
			else
				RaiseWin32Error("MultiByteToWideChar", GetLastError());
		}
		else
			RaiseError(E_APIERROR);
	}
	else if (parm(2)->Vartype() == '0' && pOld)
	{
		if ((*pOld))
		{
			if (HeapFree(VFP2CTls::Heap, 0, *pOld))
			{
				REMOVEDEBUGALLOC(*pOld);
				*pOld = 0;
				Return(0);
			}
			else
			{
				SaveWin32Error("HeapFree", GetLastError());
				RaiseError(E_APIERROR);
			}
		}
		else
			Return(0);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteWCharArray(ParamBlkEx& parm)
{
	wchar_t *pString = parm(1)->Ptr<wchar_t*>();
	unsigned int nCodePage = parm.PCount() == 3 ? VFP2CTls::Tls().ConvCP : parm(4)->ev_long;
	int nBytesWritten, nArrayWidth, nStringLen;
	nArrayWidth = parm(3)->ev_long - 1; // -1 for null terminator
	nStringLen = parm(2)->ev_length;

	if (pString)
	{
		if (nStringLen)
		{
			nBytesWritten = MultiByteToWideChar(nCodePage, 0, parm(2)->HandleToPtr(), min(nStringLen,nArrayWidth), pString, nArrayWidth);
			if (nBytesWritten)
				pString[nBytesWritten] = L'\0';
			else
				RaiseWin32Error("MultiByteToWideChar", GetLastError());
		}
		else
			*pString = L'\0';
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteBytes(ParamBlkEx& parm)
{
	void* pPointer = parm(1)->Ptr<void*>();
	if (pPointer)
		memcpy(pPointer, parm(2)->HandleToPtr(), parm.PCount() == 3 ? min(parm(2)->ev_length, (UINT)parm(3)->ev_long) : parm(2)->ev_length);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadInt8(ParamBlkEx& parm)
{
	__int8 *pPointer = parm(1)->Ptr<__int8*>();
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPInt8(ParamBlkEx& parm)
{
	__int8 **pPointer = parm(1)->Ptr<__int8**>();
	if (pPointer)
	{
		if ((*pPointer))
            Return(**pPointer);
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadUInt8(ParamBlkEx& parm)
{
	unsigned __int8 *pPointer = parm(1)->Ptr<unsigned __int8*>();
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPUInt8(ParamBlkEx& parm)
{
	unsigned __int8 **pPointer = parm(1)->Ptr<unsigned __int8**>();
	if (pPointer)
	{
		if ((*pPointer))
            Return(**pPointer);
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadShort(ParamBlkEx& parm)
{
	short *pPointer = parm(1)->Ptr<short*>();
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPShort(ParamBlkEx& parm)
{
	short **pPointer = parm(1)->Ptr<short**>();
	if (pPointer)
	{
		if ((*pPointer))
            Return(**pPointer);
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadUShort(ParamBlkEx& parm)
{
	unsigned short *pPointer = parm(1)->Ptr<unsigned short*>();
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPUShort(ParamBlkEx& parm)
{
	unsigned short **pPointer = parm(1)->Ptr<unsigned short**>();
	if (pPointer)
	{
		if ((*pPointer))
            Return(**pPointer);
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadInt(ParamBlkEx& parm)
{
	int *pPointer = parm(1)->Ptr<int*>();
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPInt(ParamBlkEx& parm)
{
	int **pPointer = parm(1)->Ptr<int**>();
	if (pPointer)
	{
		if ((*pPointer))
            Return(**pPointer);
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadUInt(ParamBlkEx& parm)
{
	unsigned int *pPointer = parm(1)->Ptr<unsigned int*>();
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPUInt(ParamBlkEx& parm)
{
	unsigned int** pPointer = parm(1)->Ptr<unsigned int**>();
	if (pPointer)
	{
		if ((*pPointer))
            Return(**pPointer);
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadInt64(ParamBlkEx& parm)
{
	__int64 *pPointer = parm(1)->Ptr<__int64*>();
	if (pPointer)
	{
		if (parm.PCount() == 1 || parm(2)->ev_long == 1)
			ReturnInt64AsBinary(*pPointer);
		else if (parm(2)->ev_long == 2)
			ReturnInt64AsString(*pPointer);
		else
			ReturnInt64AsDouble(*pPointer);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPInt64(ParamBlkEx& parm)
{
	__int64 **pPointer = parm(1)->Ptr<__int64**>();
	if (pPointer)
	{
		if ((*pPointer))
		{
			if (parm.PCount() == 1 || parm(2)->ev_long == 1)
				ReturnInt64AsBinary(**pPointer);
			else if (parm(2)->ev_long == 2)
				ReturnInt64AsString(**pPointer);
			else
				ReturnInt64AsDouble(**pPointer);
		}
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadUInt64(ParamBlkEx& parm)
{
	unsigned __int64 *pPointer = parm(1)->Ptr<unsigned __int64*>();
	if (pPointer)
	{
		if (parm.PCount() == 1 || parm(2)->ev_long == 1)
			ReturnInt64AsBinary(*pPointer);
		else if (parm(2)->ev_long == 2)
			ReturnInt64AsString(*pPointer);
		else
			ReturnInt64AsDouble(*pPointer);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPUInt64(ParamBlkEx& parm)
{
	unsigned __int64 **pPointer = parm(1)->Ptr<unsigned __int64**>();
	if (pPointer)
	{
		if ((*pPointer))
		{
			if (parm.PCount() == 1 || parm(2)->ev_long == 1)
				ReturnInt64AsBinary(**pPointer);
			else if (parm(2)->ev_long == 2)
				ReturnInt64AsString(**pPointer);
			else
				ReturnInt64AsDouble(**pPointer);
		}
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadFloat(ParamBlkEx& parm)
{
	float *pPointer = parm(1)->Ptr<float*>();
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPFloat(ParamBlkEx& parm)
{
	float **pPointer = parm(1)->Ptr<float**>();
	if (pPointer)
	{
		if ((*pPointer))
            Return(**pPointer);
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadDouble(ParamBlkEx& parm)
{
	double *pPointer = parm(1)->Ptr<double*>();
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPDouble(ParamBlkEx& parm)
{
	double **pPointer = parm(1)->Ptr<double**>();
	if (pPointer)
	{
		if ((*pPointer))
            Return(**pPointer);
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadLogical(ParamBlkEx& parm)
{
	BOOL *pPointer = parm(1)->Ptr<BOOL*>();
	if (pPointer)
		_RetLogical(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPLogical(ParamBlkEx& parm)
{
	BOOL **pPointer = parm(1)->Ptr<BOOL**>();
	if (pPointer)
	{
		if ((*pPointer))
            _RetLogical(**pPointer);
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPointer(ParamBlkEx& parm)
{
	void **pPointer = parm(1)->Ptr<void**>();
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPPointer(ParamBlkEx& parm)
{
	void ***pPointer = parm(1)->Ptr<void***>();
	if (pPointer)
	{
		if ((*pPointer))
			Return(**pPointer);
		else
			ReturnNull();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadChar(ParamBlkEx& parm)
{
	ValueEx cChar;
	cChar.SetString(1);
	char *pPointer = parm(1)->Ptr<char*>();
	char *pChar;
	if (pPointer)
	{
		if (cChar.AllocHandle(1))
		{
			pChar = cChar.HandleToPtr();
			*pChar = *pPointer;
			Return(cChar);
		}
		else
			RaiseError(E_INSUFMEMORY);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPChar(ParamBlkEx& parm)
{
	ValueEx cChar;
	cChar.SetString(1);
	char **pPointer = parm(1)->Ptr<char**>();
	char *pChar;

	if (pPointer)
	{
		if ((*pPointer))
		{
			if (cChar.AllocHandle(1))
			{
				pChar = cChar.HandleToPtr();
				*pChar = **pPointer;
				Return(cChar);
			}
			else
				RaiseError(E_INSUFMEMORY);
		}
		else if (parm.PCount() == 1)
			ReturnNull();
		else
			parm(2)->Return();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadCString(ParamBlkEx& parm)
{
	char *pPointer = parm(1)->Ptr<char*>();
	if (pPointer)
		Return(pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPCString(ParamBlkEx& parm)
{
	char **pPointer = parm(1)->Ptr<char**>();
	if (pPointer)
	{
		if ((*pPointer))
            Return(*pPointer);
		else if (parm.PCount() == 1)
		{
			char aNothing[1] = {'\0'};
			Return(aNothing);
		}
		else
			parm(2)->Return();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadCharArray(ParamBlkEx& parm)
{
	ValueEx vBuffer;
	vBuffer.SetString();
	const char *pPointer = parm(1)->Ptr<const char*>();

	if (pPointer)
	{
		if (vBuffer.AllocHandle(parm(2)->ev_long))
		{
			vBuffer.ev_length = strncpyex(vBuffer.HandleToPtr(), pPointer, parm(2)->ev_long);
			Return(vBuffer);
		}
		else
			RaiseError(E_INSUFMEMORY);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadWString(ParamBlkEx& parm)
{
	ValueEx vBuffer;
	vBuffer.SetString();
	int nStringLen, nBufferLen;
	wchar_t* pString = parm(1)->Ptr<wchar_t*>();
	unsigned int nCodePage = parm.PCount() == 1 ? VFP2CTls::Tls().ConvCP : parm(2)->ev_long;

	if (pString)
	{
		nStringLen = lstrlenW(pString);
		if (nStringLen)
		{
			nBufferLen = nStringLen * sizeof(wchar_t) + sizeof(wchar_t);
			if (vBuffer.AllocHandle(nBufferLen))
			{
				nBufferLen = WideCharToMultiByte(nCodePage, 0, pString, nStringLen, vBuffer.HandleToPtr(), nBufferLen, 0, 0);
				if (nBufferLen)
				{
					vBuffer.ev_length = (unsigned int)nBufferLen;
					Return(vBuffer);
					return;
				}
				else
					RaiseWin32Error("WideCharToMultiByte", GetLastError());
			}
			else
				RaiseError(E_INSUFMEMORY);
		}
		vBuffer.ev_length = 0;
		Return(vBuffer);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPWString(ParamBlkEx& parm)
{
	ValueEx vBuffer;
	vBuffer.SetString();
	wchar_t **pString = parm(1)->Ptr<wchar_t**>();
	unsigned int nCodePage = parm.PCount() > 1 && parm(2)->ev_long ? parm(2)->ev_long : VFP2CTls::Tls().ConvCP;
	int nStringLen, nBufferLen;

	if (pString)
	{
		if ((*pString))
		{
			nStringLen = lstrlenW(*pString);
			if (nStringLen)
			{
				nBufferLen = nStringLen * sizeof(wchar_t);
				if (vBuffer.AllocHandle(nBufferLen))
				{
					nBufferLen = WideCharToMultiByte(nCodePage, 0, *pString, nStringLen, vBuffer.HandleToPtr(), nBufferLen, 0, 0);
					if (nBufferLen)
						vBuffer.ev_length = nBufferLen;
					else
						RaiseWin32Error("WideCharToMultiByte", GetLastError());
				}
				else
					RaiseError(E_INSUFMEMORY);
			}
			Return(vBuffer);
		}
		else if (parm.PCount() < 3)
			Return(vBuffer);
		else
			parm(3)->Return();
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadWCharArray(ParamBlkEx& parm)
{
	ValueEx vBuffer;
	vBuffer.SetString();
	wchar_t *pString = parm(1)->Ptr<wchar_t*>();
	unsigned int nCodePage = parm.PCount() == 2 ? VFP2CTls::Tls().ConvCP : parm(3)->ev_long;
	int nBufferLen, nStringLen;

	if (pString)
	{
		nStringLen = wstrnlen(pString, parm(2)->ev_long);
		if (nStringLen)
		{
			nBufferLen = nStringLen * sizeof(wchar_t);
			if (vBuffer.AllocHandle(nBufferLen))
			{
				nBufferLen = WideCharToMultiByte(nCodePage, 0, pString, nStringLen, vBuffer.HandleToPtr(), nBufferLen, 0, 0);
				if (nBufferLen)
					vBuffer.ev_length = nBufferLen;
				else
					RaiseWin32Error("WideCharToMultiByte", GetLastError());
			}
			else
				RaiseError(E_INSUFMEMORY);
		}
		Return(vBuffer);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}



void _fastcall ReadBytes(ParamBlkEx& parm)
{
	ValueEx vBuffer;
	vBuffer.SetString(parm(2)->ev_long);
	void *pPointer = parm(1)->Ptr<void*>();
	if (pPointer)
	{
		if (vBuffer.AllocHandle(parm(2)->ev_long))
		{
			memcpy(vBuffer.HandleToPtr(), pPointer, parm(2)->ev_long);
			Return(vBuffer);
			return;
		}
		else
			RaiseError(E_INSUFMEMORY);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

#define BEGIN_ARRAYLOOP() \
	while(++vfpArray.CurrentDim() <= vfpArray.ADims()) \
	{ \
		vfpArray.CurrentRow() = 0; \
		while(++vfpArray.CurrentRow() <= vfpArray.ARows()) \
		{ 

#define END_ARRAYLOOP() \
		} \
	}

void _fastcall MarshalFoxArray2CArray(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		throw nErrorNo;

	FoxArray vfpArray(parm(2));
	MarshalType Type = static_cast<MarshalType>(parm(3)->ev_long);
	FoxValue pValue;
	HANDLE hHeap = VFP2CTls::Heap;

	switch(Type)
	{
		case CTYPE_SHORT:
			{
				short *CArray = parm(1)->Ptr<short*>();
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'N')
						*CArray = static_cast<short>(pValue->ev_long);
					else if (pValue.Vartype() != '0')
						throw E_INVALIDPARAMS;
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_USHORT:
			{
				unsigned short *CArray = parm(1)->Ptr<unsigned short*>();
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'N')
						*CArray = static_cast<unsigned short>(pValue->ev_long);
					else if (pValue.Vartype() != '0')
						throw E_INVALIDPARAMS;
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_INT:
			{
				int *CArray = parm(1)->Ptr<int*>();
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'N')
						*CArray = pValue->ev_long;
					else if (pValue.Vartype() != '0')
						throw E_INVALIDPARAMS;
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_UINT:
			{
				unsigned int *CArray = parm(1)->Ptr<unsigned int*>();
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'N')
						*CArray = static_cast<unsigned int>(pValue->ev_long);
					else if (pValue.Vartype() != '0')
						throw E_INVALIDPARAMS;
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_FLOAT:
			{
				float *CArray = parm(1)->Ptr<float*>();
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'N')
						*CArray = static_cast<float>(pValue->ev_long);
					else if (pValue.Vartype() != '0')
						throw E_INVALIDPARAMS;
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_DOUBLE:
			{
				double *CArray = parm(1)->Ptr<double*>();
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'N')
						*CArray = pValue->ev_real;
					else if (pValue.Vartype() != '0')
						throw E_INVALIDPARAMS;
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_BOOL:
			{
				BOOL *CArray = parm(1)->Ptr<BOOL*>();
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'L')
						*CArray = pValue->ev_length;
					else if (pValue.Vartype() != '0')
						throw E_INVALIDPARAMS;
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_CSTRING:
			{
				char **CArray = parm(1)->Ptr<char**>();
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'C')
					{
						if (*CArray)
							*CArray = (char*)HeapReAlloc(hHeap, 0, *CArray, pValue->ev_length + sizeof(char));
						else
							*CArray = (char*)HeapAlloc(hHeap, 0, pValue->ev_length + sizeof(char));
			
						if (*CArray)
						{
							memcpy(*CArray, pValue.HandleToPtr(), pValue->ev_length);
							(*CArray)[pValue->ev_length] = '\0';
						}
						else
							throw E_INSUFMEMORY;
					}
					else if (pValue.Vartype() == '0')
					{
						if (*CArray)
						{
							HeapFree(hHeap, 0, *CArray);
							*CArray = 0;
						}
					}
					else 
						throw E_INVALIDPARAMS;

					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_WSTRING:
			{
				wchar_t **CArray = parm(1)->Ptr<wchar_t**>();
				int nCharsWritten;
				unsigned int nCodePage = parm.PCount() == 3 ? VFP2CTls::Tls().ConvCP : parm(4)->ev_long;
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'C')
					{
						if (*CArray)
							*CArray = (wchar_t*)HeapReAlloc(hHeap, 0, *CArray, pValue->ev_length * sizeof(wchar_t) + sizeof(wchar_t));
						else
							*CArray = (wchar_t*)HeapAlloc(hHeap, 0, pValue->ev_length * sizeof(wchar_t) + sizeof(wchar_t));

						if (*CArray)
						{
							nCharsWritten = MultiByteToWideChar(nCodePage, 0, pValue.HandleToPtr(), pValue->ev_length, *CArray, pValue->ev_length);
							if (nCharsWritten)
							{
								(*CArray)[nCharsWritten] = L'\0';
							}
							else
							{
								SaveWin32Error("MultiByteToWideChar", GetLastError());
								throw E_APIERROR;
							}
						}
						else
							throw E_INSUFMEMORY;
					}
					else if (pValue.Vartype() == '0')
					{
						if (*CArray)
						{
							HeapFree(hHeap, 0, *CArray);
							*CArray = 0;
						}
					}
					else
						throw E_INVALIDPARAMS;

					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_CHARARRAY:
			{
				if (parm.PCount() < 4)
					throw E_INVALIDPARAMS;

				char *CArray = parm(1)->Ptr<char*>();
				unsigned int nCharCount, nLength = parm(4)->ev_long;
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
                	if (pValue.Vartype() == 'C')
					{
						nCharCount = min(pValue->ev_length, nLength);
						if (nCharCount)
							memcpy(CArray, pValue.HandleToPtr(), nCharCount);
						if (nCharCount < nLength)
							CArray[nCharCount] = '\0';
					}
					else if (pValue.Vartype() == '0')
						memset(CArray, 0, nLength);
					else
						throw E_INVALIDPARAMS;

					CArray += nLength;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_WCHARARRAY:
			{
				if (parm.PCount() < 4)
					throw E_INVALIDPARAMS;

				wchar_t *CArray = parm(1)->Ptr<wchar_t*>();
				unsigned int nCodePage, nCharsWritten, nLength = parm(4)->ev_long;
				nCodePage = parm.PCount() == 4 ? VFP2CTls::Tls().ConvCP : parm(5)->ev_long;
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'C')
					{
						nCharsWritten = MultiByteToWideChar(nCodePage, 0, pValue.HandleToPtr(), min(pValue->ev_length, nLength), CArray, nLength);
						if (nCharsWritten)
						{
							if (nCharsWritten < nLength)
								CArray[nCharsWritten] = L'\0';
						}
						else
						{
							SaveWin32Error("MultiByteToWideChar", GetLastError());
							throw E_APIERROR;
						}
					}
					else if (pValue.Vartype() == '0')
						memset(CArray, 0, nLength * 2);
					else
						throw E_INVALIDPARAMS;

					CArray += nLength;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_INT64:
			{
				__int64 *CArray = parm(1)->Ptr<__int64*>();
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'Y')
						*CArray = pValue->ev_currency.QuadPart;
					else if (pValue.Vartype() == 'C' && pValue->ev_length == 8)
						*CArray = *reinterpret_cast<__int64*>(pValue.HandleToPtr());
					if (pValue.Vartype() == 'N')
						*CArray = static_cast<__int64>(pValue->ev_real);
					else if (pValue.Vartype() != '0')
						throw E_INVALIDPARAMS;
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_UINT64:
			{
				unsigned __int64 *CArray = parm(1)->Ptr<unsigned __int64*>();
				BEGIN_ARRAYLOOP()
					pValue = vfpArray();
					if (pValue.Vartype() == 'Y')
						*CArray = static_cast<unsigned __int64>(pValue->ev_currency.QuadPart);
					else if (pValue.Vartype() == 'C' && pValue->ev_length == 8)
						*CArray = *reinterpret_cast<unsigned __int64*>(pValue.HandleToPtr());
					if (pValue.Vartype() == 'N')
						*CArray = static_cast<unsigned __int64>(pValue->ev_real);
					else if (pValue.Vartype() != '0')
						throw E_INVALIDPARAMS;
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		default:
			throw E_INVALIDPARAMS;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall MarshalCArray2FoxArray(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		throw nErrorNo;

	FoxArray vfpArray(parm(2));
	MarshalType Type = static_cast<MarshalType>(parm(3)->ev_long);

	switch(Type)
	{
		case CTYPE_SHORT:
			{
				short *CArray = parm(1)->Ptr<short*>();
				FoxShort pValue;
				BEGIN_ARRAYLOOP()
					vfpArray() = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_USHORT:
			{
				unsigned short *CArray = parm(1)->Ptr<unsigned short*>();
				FoxUShort pValue;
				BEGIN_ARRAYLOOP()
					vfpArray() = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_INT:
			{
				int *CArray = parm(1)->Ptr<int*>();
				FoxInt pValue;
				BEGIN_ARRAYLOOP()
					vfpArray() = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_UINT:
			{
				unsigned int *CArray = parm(1)->Ptr<unsigned int*>();
				FoxUInt pValue;
				BEGIN_ARRAYLOOP()
					vfpArray() = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_FLOAT:
			{
				float *CArray = parm(1)->Ptr<float*>();
				FoxFloat pValue;
				BEGIN_ARRAYLOOP()
					vfpArray() = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_DOUBLE:
			{
				double *CArray = parm(1)->Ptr<double*>();
				FoxDouble pValue;
				BEGIN_ARRAYLOOP()
					vfpArray() = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_BOOL:
			{
				BOOL *CArray = parm(1)->Ptr<BOOL*>();
				FoxLogical pValue;
				BEGIN_ARRAYLOOP()
					vfpArray() = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_CSTRING:
			{
				char **CArray = parm(1)->Ptr<char**>();
				// unsigned int nStringLen;
				FoxString pValue(256);
				FoxValue pNull;

				BEGIN_ARRAYLOOP()
					if (*CArray)
					{
						/*
						nStringLen = lstrlen(*CArray);
						if (nStringLen > pValue.Size())
							pValue.Size(nStringLen);
						if (nStringLen)
							memcpy(pValue, *CArray, nStringLen);
						pValue.Len(nStringLen);
						*/
						vfpArray() = pValue = *CArray;
					}
					else
						vfpArray() = pNull;
					
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_WSTRING:
			{
				wchar_t **CArray = parm(1)->Ptr<wchar_t**>();
				unsigned int nByteCount, nWCharCount, nCharsWritten;
				unsigned int nCodePage = parm.PCount() == 3 ? VFP2CTls::Tls().ConvCP : parm(4)->ev_long;;
				FoxString pValue(512);
				FoxValue pNull;

				BEGIN_ARRAYLOOP()
					if (*CArray)
					{
						nWCharCount = lstrlenW(*CArray);
						nByteCount = nWCharCount * sizeof(wchar_t);
						if (nByteCount > pValue.Size())
							pValue.Size(nByteCount);
						if (nByteCount)
						{
							nCharsWritten = WideCharToMultiByte(nCodePage, 0, *CArray, nWCharCount, pValue, pValue.Size(), 0, 0);
							if (nCharsWritten)
								pValue.Len(nCharsWritten);
							else
							{
								SaveWin32Error("WideCharToMultiByte", GetLastError());
								throw E_APIERROR;
							}
						}
						else
							pValue.Len(0);

						vfpArray() = pValue;
					}
					else
						vfpArray() = pNull;
				
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_CHARARRAY:
			{
				if (parm.PCount() != 4)
					throw E_INVALIDPARAMS;
				
				char *CArray = parm(1)->Ptr<char*>();
				unsigned int nLen = parm(4)->ev_long;
				FoxString pValue(nLen);
				BEGIN_ARRAYLOOP()
					vfpArray() = pValue.StrnCpy(CArray, nLen);
					CArray += nLen;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_WCHARARRAY:
			{
				if (parm.PCount() < 4)
					throw E_INVALIDPARAMS;

				wchar_t *CArray = parm(1)->Ptr<wchar_t*>();
				int nCharCount, nLen = parm(4)->ev_long;
				unsigned int nCodePage = parm.PCount() == 4 ? VFP2CTls::Tls().ConvCP : parm(5)->ev_long;
				FoxString pValue(nLen);
				
				BEGIN_ARRAYLOOP()
					nCharCount = WideCharToMultiByte(nCodePage, 0, CArray, nLen, pValue, pValue.Size(), 0, 0);
					if (nCharCount)
						pValue.Len(nCharCount);
					else
					{
						SaveWin32Error("WideCharToMultiByte", GetLastError());
						throw E_APIERROR;
					}
					vfpArray() = pValue;
					CArray += nLen;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_INT64:
			{
				__int64 *CArray = parm(1)->Ptr<__int64*>();
				FoxCurrency pValue;
				BEGIN_ARRAYLOOP()
					vfpArray() = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_UINT64:
			{
				unsigned __int64 *CArray = parm(1)->Ptr<unsigned __int64*>();
				FoxCurrency pValue;
				BEGIN_ARRAYLOOP()
					vfpArray() = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		default:
			throw E_INVALIDPARAMS;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

#undef BEGIN_ARRAYLOOP
#undef END_ARRAYLOOP

#define BEGIN_CURSORLOOP() \
	for (int nRecNo = 0; nRecNo < nRecCount; nRecNo++) \
	{

#define END_CURSORLOOP() \
		pCursor.Skip(); \
	}

#define BEGIN_FIELDLOOP() \
	for (unsigned int nFieldNo = 0; nFieldNo < nFieldCount; nFieldNo++) \
	{

#define END_FIELDLOOP() \
	}

void _fastcall MarshalCursor2CArray(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		throw nErrorNo;

	FoxValue pValue;
	FoxString pCursorAndFields(parm(2));
	MarshalType Type = static_cast<MarshalType>(parm(3)->ev_long);
	FoxCursor pCursor;
	VFP2CTls& tls = VFP2CTls::Tls();

	CStringView pCursorName = pCursorAndFields.GetWordNum(1, '.');
	CStringView pFieldNames = pCursorAndFields.GetWordNum(2, '.');
	// pFieldNames += GetWordNumN(CursorName, pCursorAndFields, '.', 1, VFP_MAX_CURSOR_NAME) + 1;
	pCursor.Attach(pCursorName, pFieldNames);
	pCursor.GoTop();
	int nRecCount = pCursor.RecCount();
	unsigned int nFieldCount = pCursor.FCount();
	
	switch(Type)
	{
		case CTYPE_SHORT:
			{
				short *CArray = parm(1)->Ptr<short*>();
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						if (pValue.Vartype() == 'N')
							CArray[nRecNo + (nRecCount * nFieldNo)] = static_cast<short>(pValue->ev_real);
						else if (pValue.Vartype() != '0')
							throw E_INVALIDPARAMS;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_USHORT:
			{
				unsigned short *CArray = parm(1)->Ptr<unsigned short*>();
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						if (pValue.Vartype() == 'N')
							CArray[nRecNo + (nRecCount * nFieldNo)] = static_cast<unsigned short>(pValue->ev_real);
						else if (pValue.Vartype() != '0')
							throw E_INVALIDPARAMS;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_INT:
			{
				int *CArray = parm(1)->Ptr<int*>();
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						if (pValue.Vartype() == 'N')
							CArray[nRecNo + (nRecCount * nFieldNo)] = static_cast<int>(pValue->ev_real);
						else if (pValue.Vartype() != '0')
							throw E_INVALIDPARAMS;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_UINT:
			{
				unsigned int *CArray = parm(1)->Ptr<unsigned int*>();
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						if (pValue.Vartype() == 'N')
							CArray[nRecNo + (nRecCount * nFieldNo)] = static_cast<unsigned int>(pValue->ev_real);
						else if (pValue.Vartype() != '0')
							throw E_INVALIDPARAMS;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_FLOAT:
			{
				float *CArray = parm(1)->Ptr<float*>();
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						if (pValue.Vartype() == 'N')
							CArray[nRecNo + (nRecCount * nFieldNo)] = static_cast<float>(pValue->ev_real);
						else if (pValue.Vartype() != '0')
							throw E_INVALIDPARAMS;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_DOUBLE:
			{
				double *CArray = parm(1)->Ptr<double*>();
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						if (pValue.Vartype() == 'N')
							CArray[nRecNo + (nRecCount * nFieldNo)] = pValue->ev_real;
						else if (pValue.Vartype() != '0')
							throw E_INVALIDPARAMS;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_BOOL:
			{
				BOOL *CArray = parm(1)->Ptr<BOOL*>();
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()										
						pValue = pCursor(nFieldNo);
						if (pValue.Vartype() == 'L')
							CArray[nRecNo + (nRecCount * nFieldNo)] = pValue->ev_length;
						else if (pValue.Vartype() != '0')
							throw E_INVALIDPARAMS;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_CSTRING:
			{
				char **CArray = parm(1)->Ptr<char**>();
				char **pString;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						pString = &CArray[nRecNo + (nRecCount * nFieldNo)];
						if (pValue.Vartype() == 'C')
						{
							if (*pString)
								*pString = (char*)HeapReAlloc(VFP2CTls::Heap, 0, *pString, pValue->ev_length + sizeof(char));
							else
								*pString = (char*)HeapAlloc(VFP2CTls::Heap, 0, pValue->ev_length + sizeof(char));
				
							if (*pString)
							{
								memcpy(*pString, pValue.HandleToPtr(), pValue->ev_length);
								(*pString)[pValue->ev_length] = '\0';
							}
							else
								throw E_INSUFMEMORY;
						}
						else if (pValue.Vartype() == '0')
						{
							if (*pString)
							{
								HeapFree(VFP2CTls::Heap, 0, *CArray);
								*pString = 0;
							}
						}
						else 
							throw E_INVALIDPARAMS;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_WSTRING:
			{
				wchar_t **CArray = parm(1)->Ptr<wchar_t**>();
				wchar_t **pString;
				int nCharsWritten;
				unsigned int nCodePage = parm.PCount() == 3 ? VFP2CTls::Tls().ConvCP : parm(4)->ev_long;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						pString = &CArray[nRecNo + (nRecCount * nFieldNo)];
						if (pValue.Vartype() == 'C')
						{
							if (*pString)
								*pString = (wchar_t*)HeapReAlloc(VFP2CTls::Heap, 0, *pString, pValue->ev_length * sizeof(wchar_t) + sizeof(wchar_t));
							else
								*pString = (wchar_t*)HeapAlloc(VFP2CTls::Heap, 0, pValue->ev_length * sizeof(wchar_t) + sizeof(wchar_t));

							if (*pString)
							{
								nCharsWritten = MultiByteToWideChar(nCodePage, 0, pValue.HandleToPtr(), pValue->ev_length, *pString, pValue->ev_length);
								if (nCharsWritten)
								{
									(*pString)[nCharsWritten] = L'\0';
								}
								else
								{
									SaveWin32Error("MultiByteToWideChar", GetLastError());
									throw E_APIERROR;
								}
							}
							else
								throw E_INSUFMEMORY;
						}
						else if (pValue.Vartype() == '0')
						{
							if (*pString)
							{
								HeapFree(VFP2CTls::Heap, 0, *pString);
								*pString = 0;
							}
						}
						else
							throw E_INVALIDPARAMS;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_CHARARRAY:
			{
				char *CArray = parm(1)->Ptr<char*>();
				char *pString;
				unsigned int nCharCount, nDimensionSize, nLength = parm(4)->ev_long;
				nDimensionSize = nRecCount * nLength;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						pString = CArray + (nFieldNo * nDimensionSize);
                		if (pValue.Vartype() == 'C')
						{
							nCharCount = min(pValue->ev_length, nLength);
							if (nCharCount)
								memcpy(pString, pValue.HandleToPtr(), nCharCount);
							if (nCharCount < nLength)
								pString[nCharCount] = '\0';
						}
						else if (pValue.Vartype() == '0')
							memset(CArray, 0, nLength);
						else
							throw E_INVALIDPARAMS;

						CArray += nLength;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_WCHARARRAY:
			{
				if (parm.PCount() < 4)
					throw E_INVALIDPARAMS;

				wchar_t *CArray = parm(1)->Ptr<wchar_t*>();
				wchar_t *pString;
				unsigned int nByteLen, nCharsWritten, nDimensionSize, nLen = parm(4)->ev_long;
				unsigned int nCodePage = parm.PCount() == 4 ? tls.ConvCP : parm(5)->ev_long;
				nByteLen = nLen * sizeof(wchar_t);
				nDimensionSize = nRecCount * nByteLen;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						pString = CArray + (nFieldNo * nDimensionSize);
						if (pValue.Vartype() == 'C')
						{
							nCharsWritten = MultiByteToWideChar(nCodePage, 0, pValue.HandleToPtr(), min(pValue->ev_length, nLen), pString, nLen);
							if (nCharsWritten)
							{
								if (nCharsWritten < nLen)
									pString[nCharsWritten] = L'\0';
							}
							else
							{
								SaveWin32Error("MultiByteToWideChar", GetLastError());
								throw E_APIERROR;
							}
						}
						else if (pValue.Vartype() == '0')
							memset(pString, 0, nByteLen);
						else
							throw E_INVALIDPARAMS;

						CArray += nLen;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_INT64:
			{
				__int64 *CArray = parm(1)->Ptr<__int64*>();
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						if (pValue.Vartype() == 'Y')
							CArray[nRecNo + (nRecCount * nFieldNo)] = pValue->ev_currency.QuadPart;
						else if (pValue.Vartype() == 'C' && pValue->ev_length == 8)
							CArray[nRecNo + (nRecCount * nFieldNo)] = *reinterpret_cast<__int64*>(pValue.HandleToPtr());
						if (pValue.Vartype() == 'N')
							CArray[nRecNo + (nRecCount * nFieldNo)] = static_cast<__int64>(pValue->ev_real);
						else if (pValue.Vartype() != '0')
							throw E_INVALIDPARAMS;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_UINT64:
			{
				unsigned __int64 *CArray = parm(1)->Ptr<unsigned __int64*>();
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pValue = pCursor(nFieldNo);
						if (pValue.Vartype() == 'Y')
							CArray[nRecNo + (nRecCount * nFieldNo)] = static_cast<unsigned __int64>(pValue->ev_currency.QuadPart);
						else if (pValue.Vartype() == 'C' && pValue->ev_length == 8)
							CArray[nRecNo + (nRecCount * nFieldNo)] = *reinterpret_cast<unsigned __int64*>(pValue.HandleToPtr());
						if (pValue.Vartype() == 'N')
							CArray[nRecNo + (nRecCount * nFieldNo)] = static_cast<unsigned __int64>(pValue->ev_real);
						else if (pValue.Vartype() != '0')
							throw E_INVALIDPARAMS;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		default:
			throw E_INVALIDPARAMS;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

#undef BEGIN_CURSORLOOP
#undef END_CURSORLOOP
#undef BEGIN_FIELDLOOP
#undef BEGIN_FIELDLOOP

#define BEGIN_CURSORLOOP() \
	for(unsigned int nRow = 0; nRow < nRowCount; nRow++) \
	{ \
		if (pCursor.Eof()) \
			pCursor.AppendBlank(); 

#define END_CURSORLOOP() \
		pCursor.Skip(); \
	}

#define BEGIN_FIELDLOOP() \
	for(unsigned int nFieldNo = 0; nFieldNo < nFieldCount; nFieldNo++) \
	{

#define END_FIELDLOOP() \
	}

void _fastcall MarshalCArray2Cursor(ParamBlkEx& parm)
{
try
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		throw nErrorNo;

	FoxString pCursorAndFields(parm(2));
	MarshalType Type = static_cast<MarshalType>(parm(3)->ev_long);
	FoxCursor pCursor;
	VFP2CTls& tls = VFP2CTls::Tls();

	unsigned int nFieldCount;
	unsigned int nRowCount = parm(4)->ev_long;
	char CursorName[VFP_MAX_CURSOR_NAME];
	char *pFieldNames = pCursorAndFields;
	pFieldNames += GetWordNumN(CursorName, pCursorAndFields, '.', 1, VFP_MAX_CURSOR_NAME) + 1;
	pCursor.Attach(CursorName, pFieldNames);
	pCursor.GoTop();
	nFieldCount = pCursor.FCount();

	switch(Type)
	{
		case CTYPE_SHORT:
			{
				short *CArray = parm(1)->Ptr<short*>();
				FoxShort pValue;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pCursor(nFieldNo) = pValue = CArray[nRow + (nRowCount * nFieldNo)];
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_USHORT:
			{
				unsigned short *CArray = parm(1)->Ptr<unsigned short*>();
				FoxUShort pValue;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pCursor(nFieldNo) = pValue = CArray[nRow + (nRowCount * nFieldNo)];
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_INT:
			{
				int *CArray = parm(1)->Ptr<int*>();
				FoxInt pValue;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pCursor(nFieldNo) = pValue = CArray[nRow + (nRowCount * nFieldNo)];
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_UINT:
			{
				unsigned int *CArray = parm(1)->Ptr<unsigned int*>();
				FoxUInt pValue;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pCursor(nFieldNo) = pValue = CArray[nRow + (nRowCount * nFieldNo)];
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_FLOAT:
			{
				float *CArray = parm(1)->Ptr<float*>();
				FoxFloat pValue;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pCursor(nFieldNo) = pValue = CArray[nRow + (nRowCount * nFieldNo)];
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_DOUBLE:
			{
				double *CArray = parm(1)->Ptr<double*>();
				FoxDouble pValue;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pCursor(nFieldNo) = pValue = CArray[nRow + (nRowCount * nFieldNo)];
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_BOOL:
			{
				BOOL *CArray = parm(1)->Ptr<BOOL*>();
				FoxLogical pValue;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pCursor(nFieldNo) = pValue = CArray[nRow + (nRowCount * nFieldNo)];
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_CSTRING:
			{
				char **CArray = parm(1)->Ptr<char**>();
				char *pString;
				unsigned int nStringLen;
				FoxString pValue(256);
				FoxValue pNull;

				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pString = CArray[nRow + (nRowCount * nFieldNo)];
						if (pString)
						{
							nStringLen = lstrlen(pString);
							if (nStringLen > pValue.Size())
								pValue.Size(nStringLen);
							if (nStringLen)
								memcpy(pValue.Ptr<void*>(), pString, nStringLen);
							pCursor(nFieldNo) = pValue.Len(nStringLen);
						}
						else
							pCursor(nFieldNo) = pNull;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_WSTRING:
			{
				wchar_t **CArray = parm(1)->Ptr<wchar_t**>();
				wchar_t *pString;
				unsigned int nByteCount, nWCharCount, nCharsWritten;
				UINT nCodePage = parm.PCount() == 4 ? tls.ConvCP : parm(5)->ev_long;;
				FoxString pValue(512);
				FoxValue pNull;

				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pString = CArray[nRow + (nRowCount * nFieldNo)];
						if (pString)
						{
							nWCharCount = lstrlenW(pString);
							nByteCount = nWCharCount * sizeof(wchar_t);
							if (nByteCount > pValue.Size())
								pValue.Size(nByteCount);
							if (nByteCount)
							{
								nCharsWritten = WideCharToMultiByte(nCodePage, 0, pString, nWCharCount, pValue, pValue.Size(), 0, 0);
								if (nCharsWritten)
									pValue.Len(nCharsWritten);
								else
								{
									SaveWin32Error("WideCharToMultiByte", GetLastError());
									throw E_APIERROR;
								}
							}
							else
								pValue.Len(0);
							pCursor(nFieldNo) = pValue;
						}
						else
							pCursor(nFieldNo) = pNull;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_CHARARRAY:
			{
				if (parm.PCount() != 5)
					throw E_INVALIDPARAMS;
				
				char *CArray = parm(1)->Ptr<char*>();
				char *pString;
				unsigned int nLen = parm(5)->ev_long;
				unsigned int nDimensionSize = nLen * nRowCount;
				FoxString pValue(nLen);
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pString = CArray + (nFieldNo * nDimensionSize);
						pCursor(nFieldNo) = pValue.StrnCpy(pString, nLen);
						CArray += nLen;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_WCHARARRAY:
			{
				if (parm.PCount() < 5)
					throw E_INVALIDPARAMS;

				wchar_t *CArray = parm(1)->Ptr<wchar_t*>();
				wchar_t *pString;
				int nCharCount, nByteLen;
				unsigned int nLen = parm(5)->ev_long;
				nByteLen = nLen * sizeof(wchar_t);
				unsigned int nDimensionSize = nByteLen * nRowCount;
				UINT nCodePage = parm.PCount() == 5 ? tls.ConvCP : parm(6)->ev_long;
				FoxString pValue(nByteLen);
				
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pString = CArray + (nFieldNo * nDimensionSize);
						nCharCount = WideCharToMultiByte(nCodePage,0, pString, -1, pValue, pValue.Size(), 0, 0);
						if (nCharCount)
							pValue.Len(nCharCount);
						else
						{
							SaveWin32Error("WideCharToMultiByte", GetLastError());
							throw E_APIERROR;
						}
						pCursor(nFieldNo) = pValue;
						CArray += nLen;
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_INT64:
			{
				__int64 *CArray = parm(1)->Ptr<__int64*>();
				FoxCurrency pValue;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pCursor(nFieldNo) = pValue = CArray[nRow + (nRowCount * nFieldNo)];
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		case CTYPE_UINT64:
			{
				unsigned __int64 *CArray = parm(1)->Ptr<unsigned __int64*>();
				FoxCurrency pValue;
				BEGIN_CURSORLOOP()
					BEGIN_FIELDLOOP()
						pCursor(nFieldNo) = pValue = CArray[nRow + (nRowCount * nFieldNo)];
					END_FIELDLOOP()
				END_CURSORLOOP()
			}
			break;

		default:
			throw E_INVALIDPARAMS;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

#undef BEGIN_CURSORLOOP
#undef END_CURSORLOOP
#undef BEGIN_FIELDLOOP
#undef BEGIN_FIELDLOOP