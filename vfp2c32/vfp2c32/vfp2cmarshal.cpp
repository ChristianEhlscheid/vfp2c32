#include <windows.h>
#include <stdio.h> // sprintf, memcpy and other common C library routines

#include "pro_ext.h" // general FoxPro library header
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2cmarshal.h"
#include "vfp2ccppapi.h"
#include "vfp2ctls.h"
#include "vfpmacros.h"

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
			pProgInfo = strdup(vProgInfo.HandleToPtr());

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
			pProgInfo = strdup(vProgInfo.HandleToPtr());

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

void _fastcall AMemLeaks(ParamBlk *parm)
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

	FoxArray aMemLeaks(vp1);
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
		memcpy(vMemInfo, pDbg->pPointer, vMemInfo.Len());
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

void _fastcall TrackMem(ParamBlk *parm)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	tls.TrackAlloc = static_cast<BOOL>(vp1.ev_length);
	if (PCount() == 2 && vp2.ev_length)
		FreeDebugAlloc();
}

#endif // DEBUG

// FLL memory allocation functions using FLL's standard heap
void _fastcall AllocMem(ParamBlk *parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	void *pAlloc = 0;
	__try
	{
		pAlloc = HeapAlloc(VFP2CTls::Heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, vp1.ev_long);
	}
	__except(SAVEHEAPEXCEPTION()) { }

	if (pAlloc)
	{
		ADDDEBUGALLOC(pAlloc, vp1.ev_long);
		Return(pAlloc);
	}
	else
		RaiseError(E_APIERROR);
}

void _fastcall AllocMemTo(ParamBlk *parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	void **pPointer = reinterpret_cast<void**>(vp1.ev_long);
	void *pAlloc = 0;

	if (pPointer)
	{
		__try
		{
			pAlloc = HeapAlloc(VFP2CTls::Heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, vp2.ev_long);
		}
		__except(SAVEHEAPEXCEPTION()) { }
	
		if (pAlloc)
		{
			*pPointer = pAlloc;
			ADDDEBUGALLOC(pAlloc,vp2.ev_long);
			Return(pAlloc);
		}
		else
			RaiseError(E_APIERROR);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReAllocMem(ParamBlk *parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	void *pPointer = reinterpret_cast<void*>(vp1.ev_long);
	void *pAlloc = 0;
	__try
	{
		if (pPointer)
		{
			pAlloc = HeapReAlloc(VFP2CTls::Heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, pPointer, vp2.ev_long);
			REPLACEDEBUGALLOC(pPointer, pAlloc, vp2.ev_long);
		}
		else
		{
			pAlloc = HeapAlloc(VFP2CTls::Heap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, vp2.ev_long);
			ADDDEBUGALLOC(pAlloc, vp2.ev_long);
		}
    }
	__except(SAVEHEAPEXCEPTION()) { }

	if (pAlloc)
		Return(pAlloc);
	else
		RaiseError(E_APIERROR);
}

void _fastcall FreeMem(ParamBlk *parm)
{
	void *pPointer = reinterpret_cast<void*>(vp1.ev_long);
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

void _fastcall FreePMem(ParamBlk *parm)
{
	void* pAlloc;
	if (vp1.ev_long)
	{
		if ((pAlloc = *reinterpret_cast<void**>(vp1.ev_long)))
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

void _fastcall FreeRefArray(ParamBlk *parm)
{
	void **pAddress;
	int nStartElement, nElements;
	BOOL bApiRet = TRUE;

	if (vp2.ev_long < 1 || vp2.ev_long > vp3.ev_long)
		RaiseError(E_INVALIDPARAMS);

	pAddress = reinterpret_cast<void**>(vp1.ev_long);
	nStartElement = --vp2.ev_long;
	nElements = vp3.ev_long;
	pAddress += nStartElement;
	nElements -= nStartElement;
	
	ResetWin32Errors();

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

void _fastcall SizeOfMem(ParamBlk *parm)
{
	if (vp1.ev_long)
		Return((int)HeapSize(VFP2CTls::Heap, 0, reinterpret_cast<void*>(vp1.ev_long)));
	else
		Return(0);
}

void _fastcall ValidateMem(ParamBlk *parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	Return(HeapValidate(VFP2CTls::Heap, 0, reinterpret_cast<void*>(vp1.ev_long)) > 0);
}

void _fastcall CompactMem(ParamBlk *parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	SIZE_T ret = HeapCompact(VFP2CTls::Heap, 0);
	Return(ret);
}

// wrappers around GlobalAlloc, GlobalFree etc .. for movable memory objects ..
void _fastcall AllocHGlobal(ParamBlk *parm)
{
	HGLOBAL hMem;
	UINT nFlags = PCount() == 2 ? vp2.ev_long : GMEM_MOVEABLE | GMEM_ZEROINIT;
	hMem = GlobalAlloc(nFlags, vp1.ev_long);
	if (hMem)
	{
		ADDDEBUGALLOC(hMem, vp1.ev_long);
		Return(hMem);
	}
	else
	{
		SaveWin32Error("GlobalAlloc", GetLastError());
		RaiseError(E_APIERROR);
	}
}

void _fastcall FreeHGlobal(ParamBlk *parm)
{
	HGLOBAL hHandle = reinterpret_cast<HGLOBAL>(vp1.ev_long);
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

void _fastcall ReAllocHGlobal(ParamBlk *parm)
{
	HGLOBAL hHandle = reinterpret_cast<HGLOBAL>(vp1.ev_long);
	UINT nFlags = PCount() == 2 ? GMEM_ZEROINIT : vp3.ev_long;
	HGLOBAL hMem;
	hMem = GlobalReAlloc(hHandle, vp2.ev_long, nFlags);
	if (hMem)
	{
		REPLACEDEBUGALLOC(vp1.ev_long, hMem, vp2.ev_long);
		Return(hMem);
	}
	else
	{
		SaveWin32Error("GlobalReAlloc", GetLastError());
		RaiseError(E_APIERROR);
	}
}

void _fastcall LockHGlobal(ParamBlk *parm)
{
	HGLOBAL hHandle = reinterpret_cast<HGLOBAL>(vp1.ev_long);
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

void _fastcall UnlockHGlobal(ParamBlk *parm)
{
	HGLOBAL hHandle = reinterpret_cast<HGLOBAL>(vp1.ev_long);
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

void _fastcall AMemBlocks(ParamBlk *parm)
{
try
{
	FoxArray pArray(vp1,1,3);
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

void _fastcall WriteInt8(ParamBlk *parm)
{
	__int8 *pPointer = reinterpret_cast<__int8*>(vp1.ev_long);
	if (pPointer)
		*pPointer = static_cast<__int8>(vp2.ev_long);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePInt8(ParamBlk *parm)
{
	__int8 **pPointer = reinterpret_cast<__int8**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<__int8>(vp2.ev_long);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteUInt8(ParamBlk *parm)
{
	unsigned __int8 *pPointer = reinterpret_cast<unsigned __int8*>(vp1.ev_long);
	if (pPointer)
		*pPointer = static_cast<unsigned __int8>(vp2.ev_long);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePUInt8(ParamBlk *parm)
{
	unsigned __int8 **pPointer = reinterpret_cast<unsigned __int8**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<unsigned __int8>(vp2.ev_long);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteShort(ParamBlk *parm)
{
	short *pPointer = reinterpret_cast<short*>(vp1.ev_long);
	if (pPointer)
		*pPointer = static_cast<short>(vp2.ev_long);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePShort(ParamBlk *parm)
{
	short **pPointer = reinterpret_cast<short**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<short>(vp2.ev_long);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteUShort(ParamBlk *parm)
{
	unsigned short *pPointer = reinterpret_cast<unsigned short*>(vp1.ev_long);
	if (pPointer)
		*pPointer = static_cast<unsigned short>(vp2.ev_long);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePUShort(ParamBlk *parm)
{
	unsigned short **pPointer = reinterpret_cast<unsigned short**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<unsigned short>(vp2.ev_long);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteInt(ParamBlk *parm)
{
	int *pPointer = reinterpret_cast<int*>(vp1.ev_long);
	if (pPointer)
		*pPointer = vp2.ev_long;
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePInt(ParamBlk *parm)
{
	int **pPointer = reinterpret_cast<int**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = vp2.ev_long;
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteUInt(ParamBlk *parm)
{
	unsigned int *pPointer = reinterpret_cast<unsigned int*>(vp1.ev_long);
	if (pPointer)
		*pPointer = static_cast<unsigned int>(vp2.ev_real);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePUInt(ParamBlk *parm)
{
	unsigned int **pPointer = reinterpret_cast<unsigned int**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<unsigned int>(vp2.ev_real);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteInt64(ParamBlk *parm)
{
try
{
	__int64 *pPointer = reinterpret_cast<__int64*>(vp1.ev_long);
	if (pPointer)
		*pPointer = Value2Int64(vp2);
	else
		throw E_INVALIDPARAMS;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall WritePInt64(ParamBlk *parm)
{
try
{
	__int64 **pPointer = reinterpret_cast<__int64**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = Value2Int64(vp2);
	}
	else
		throw E_INVALIDPARAMS;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall WriteUInt64(ParamBlk *parm)
{
try
{
	unsigned __int64 *pPointer = reinterpret_cast<unsigned __int64*>(vp1.ev_long);
	if (pPointer)
		*pPointer = Value2UInt64(vp2);
	else
		throw E_INVALIDPARAMS;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall WritePUInt64(ParamBlk *parm)
{
try
{
	unsigned __int64 **pPointer = reinterpret_cast<unsigned __int64**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = Value2UInt64(vp2);
	}
	else
		throw E_INVALIDPARAMS;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall WriteFloat(ParamBlk *parm)
{
	float *pPointer = reinterpret_cast<float*>(vp1.ev_long);
	if (pPointer)
		*pPointer = static_cast<float>(vp2.ev_real);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePFloat(ParamBlk *parm)
{
	float **pPointer = reinterpret_cast<float**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = static_cast<float>(vp2.ev_real);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteDouble(ParamBlk *parm)
{
	double *pPointer = reinterpret_cast<double*>(vp1.ev_long);
	if (pPointer)
		*pPointer = vp2.ev_real;
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePDouble(ParamBlk *parm)
{
	double **pPointer = reinterpret_cast<double**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = vp2.ev_real;
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteLogical(ParamBlk *parm)
{
	unsigned int *pPointer = reinterpret_cast<unsigned int*>(vp1.ev_long);
	if (pPointer)
        *pPointer = vp2.ev_length;
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePLogical(ParamBlk *parm)
{
	unsigned int **pPointer = reinterpret_cast<unsigned int**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = vp2.ev_length;
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePointer(ParamBlk *parm)
{
	void **pPointer = reinterpret_cast<void**>(vp1.ev_long);
	unsigned int nPointer = static_cast<unsigned int>(vp2.ev_real);
	if (pPointer)
		*pPointer = reinterpret_cast<void*>(nPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePPointer(ParamBlk *parm)
{
	void ***pPointer = reinterpret_cast<void***>(vp1.ev_long);
	unsigned int nPointer = static_cast<unsigned int>(vp2.ev_real);
	if (pPointer)
	{
		if ((*pPointer))
            **pPointer = reinterpret_cast<void*>(nPointer);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteChar(ParamBlk *parm)
{
	char *pChar = reinterpret_cast<char*>(vp1.ev_long);
	if (pChar)
		*pChar = *HandleToPtr(vp2);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePChar(ParamBlk *parm)
{
	char **pChar = reinterpret_cast<char**>(vp1.ev_long);
	if (pChar)
	{
		if ((*pChar))
            **pChar = *HandleToPtr(vp2);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteWChar(ParamBlk *parm)
{
	wchar_t *pString = reinterpret_cast<wchar_t*>(vp1.ev_long);
	unsigned int nCodePage = PCount() == 2 ? VFP2CTls::Tls().ConvCP : vp3.ev_long;
	if (pString)
	{
		if (vp2.ev_length)
			MultiByteToWideChar(nCodePage, 0, HandleToPtr(vp2), 1, pString, 1);
		else
			*pString = L'\0';
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WritePWChar(ParamBlk *parm)
{
	wchar_t **pString = reinterpret_cast<wchar_t**>(vp1.ev_long);
	unsigned int nCodePage = PCount() == 2 ? VFP2CTls::Tls().ConvCP : vp3.ev_long;
	if (pString)
	{
		if ((*pString))
		{
			if (vp2.ev_length)
				MultiByteToWideChar(nCodePage, 0, HandleToPtr(vp2), 1, *pString, 1);
			else
				**pString = L'\0';
		}
	}
}

void _fastcall WriteCString(ParamBlk *parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	char *pPointer = reinterpret_cast<char*>(vp1.ev_long);
	char *pNewAddress = 0;

	__try
	{
		if (pPointer)
		{
			pNewAddress = (char*)HeapReAlloc(VFP2CTls::Heap, HEAP_GENERATE_EXCEPTIONS, pPointer, vp2.ev_length+1);
			REPLACEDEBUGALLOC(pPointer, pNewAddress, vp2.ev_length+1);
		}
		else
		{
			pNewAddress = (char*)HeapAlloc(VFP2CTls::Heap, HEAP_GENERATE_EXCEPTIONS, vp2.ev_length+1);
			ADDDEBUGALLOC(pNewAddress, vp2.ev_length+1);
		}
	}
	__except(SAVEHEAPEXCEPTION()) { }

	if (pNewAddress)
	{
		memcpy(pNewAddress, HandleToPtr(vp2), vp2.ev_length);
		pNewAddress[vp2.ev_length] = '\0';
		Return((void*)pNewAddress);
	}
	else
		RaiseError(E_APIERROR);
}

void _fastcall WriteGPCString(ParamBlk *parm)
{
	char *pNewAddress = 0;
	HGLOBAL *pOldAddress = reinterpret_cast<HGLOBAL*>(vp1.ev_long);
	SIZE_T dwLen = vp2.ev_length + 1;

	if (Vartype(vp2) == 'C' && pOldAddress)
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
		memcpy(pNewAddress, HandleToPtr(vp2),vp2.ev_length);
		pNewAddress[vp2.ev_length] = '\0';		
		Return((void*)pNewAddress);
	}
	else if (Vartype(vp2) == '0' && pOldAddress)
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

void _fastcall WritePCString(ParamBlk *parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	char *pNewAddress = 0;
	char **pOldAddress = reinterpret_cast<char**>(vp1.ev_long);

	if (Vartype(vp2) == 'C' && pOldAddress)
	{
		__try
		{
			if ((*pOldAddress))
			{
				pNewAddress = (char*)HeapReAlloc(VFP2CTls::Heap, HEAP_GENERATE_EXCEPTIONS, (*pOldAddress), vp2.ev_length+1);
				REPLACEDEBUGALLOC(*pOldAddress, pNewAddress, vp2.ev_length);
			}
			else
			{
				pNewAddress = (char*)HeapAlloc(VFP2CTls::Heap, HEAP_GENERATE_EXCEPTIONS, vp2.ev_length+1);
				ADDDEBUGALLOC(pNewAddress,vp2.ev_length);
			}
		}
		__except(SAVEHEAPEXCEPTION()) { }

		if (pNewAddress)
		{
			*pOldAddress = pNewAddress;
			memcpy(pNewAddress,HandleToPtr(vp2),vp2.ev_length);
			pNewAddress[vp2.ev_length] = '\0';		
			Return((void*)pNewAddress);
		}
		else
			RaiseError(E_APIERROR);
	}
	else if (Vartype(vp2) == '0' && pOldAddress)
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

void _fastcall WriteCharArray(ParamBlk *parm)
{
	char *pPointer = reinterpret_cast<char*>(vp1.ev_long);
	if (pPointer)
	{
		if (PCount() == 2 || (long)vp2.ev_length < vp3.ev_long)
		{
			memcpy(pPointer, HandleToPtr(vp2),vp2.ev_length);
			pPointer[vp2.ev_length] = '\0';
		}
		else
		{
			memcpy(pPointer, HandleToPtr(vp2), vp3.ev_long);
			pPointer[vp3.ev_long-1] = '\0';
		}
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall WriteWString(ParamBlk *parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	wchar_t *pString = reinterpret_cast<wchar_t*>(vp1.ev_long);
	unsigned int nCodePage = PCount() == 2 ? VFP2CTls::Tls().ConvCP : vp3.ev_long;
	wchar_t *pDest = 0;
	int nStringLen, nBytesNeeded, nBytesWritten;	
	nStringLen = vp2.ev_length;
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
			nBytesWritten = MultiByteToWideChar(nCodePage, 0, HandleToPtr(vp2), nStringLen, pDest, nBytesNeeded);
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

void _fastcall WritePWString(ParamBlk *parm)
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		RaiseError(nErrorNo);

	wchar_t **pOld = reinterpret_cast<wchar_t**>(vp1.ev_long);
	wchar_t *pDest = 0;
	unsigned int nCodePage = PCount() == 2 ? VFP2CTls::Tls().ConvCP : vp3.ev_long;
	int nStringLen, nBytesNeeded, nBytesWritten;

	if (Vartype(vp2) == 'C' && pOld)
	{
		nStringLen = vp2.ev_length;
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
			nBytesWritten = MultiByteToWideChar(nCodePage, 0, HandleToPtr(vp2), nStringLen, pDest, nBytesNeeded);
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
	else if (Vartype(vp2) == '0' && pOld)
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

void _fastcall WriteWCharArray(ParamBlk *parm)
{
	wchar_t *pString = reinterpret_cast<wchar_t*>(vp1.ev_long);
	unsigned int nCodePage = PCount() == 3 ? VFP2CTls::Tls().ConvCP : vp4.ev_long;
	int nBytesWritten, nArrayWidth, nStringLen;
	nArrayWidth = vp3.ev_long - 1; // -1 for null terminator
	nStringLen = vp2.ev_length;

	if (pString)
	{
		if (nStringLen)
		{
			nBytesWritten = MultiByteToWideChar(nCodePage, 0, HandleToPtr(vp2), min(nStringLen,nArrayWidth), pString, nArrayWidth);
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

void _fastcall WriteBytes(ParamBlk *parm)
{
	void *pPointer = reinterpret_cast<void*>(vp1.ev_long);
	if (pPointer)
		memcpy(pPointer, HandleToPtr(vp2), PCount() == 3 ? min(vp2.ev_length, (UINT)vp3.ev_long) : vp2.ev_length);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadInt8(ParamBlk *parm)
{
	__int8 *pPointer = reinterpret_cast<__int8*>(vp1.ev_long);
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPInt8(ParamBlk *parm)
{
	__int8 **pPointer = reinterpret_cast<__int8**>(vp1.ev_long);
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

void _fastcall ReadUInt8(ParamBlk *parm)
{
	unsigned __int8 *pPointer = reinterpret_cast<unsigned __int8*>(vp1.ev_long);
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPUInt8(ParamBlk *parm)
{
	unsigned __int8 **pPointer = reinterpret_cast<unsigned __int8**>(vp1.ev_long);
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

void _fastcall ReadShort(ParamBlk *parm)
{
	short *pPointer = reinterpret_cast<short*>(vp1.ev_long);
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPShort(ParamBlk *parm)
{
	short **pPointer = reinterpret_cast<short**>(vp1.ev_long);
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

void _fastcall ReadUShort(ParamBlk *parm)
{
	unsigned short *pPointer = reinterpret_cast<unsigned short*>(vp1.ev_long);
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPUShort(ParamBlk *parm)
{
	unsigned short **pPointer = reinterpret_cast<unsigned short**>(vp1.ev_long);
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

void _fastcall ReadInt(ParamBlk *parm)
{
	int *pPointer = reinterpret_cast<int*>(vp1.ev_long);
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPInt(ParamBlk *parm)
{
	int **pPointer = reinterpret_cast<int**>(vp1.ev_long);
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

void _fastcall ReadUInt(ParamBlk *parm)
{
	unsigned int *pPointer = reinterpret_cast<unsigned int*>(vp1.ev_long);
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPUInt(ParamBlk *parm)
{
	unsigned int **pPointer = reinterpret_cast<unsigned int**>(vp1.ev_long);
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

void _fastcall ReadInt64(ParamBlk *parm)
{
	__int64 *pPointer = reinterpret_cast<__int64*>(vp1.ev_long);
	if (pPointer)
	{
		if (PCount() == 1 || vp2.ev_long == 1)
			ReturnInt64AsBinary(*pPointer);
		else if (vp2.ev_long == 2)
			ReturnInt64AsString(*pPointer);
		else
			ReturnInt64AsDouble(*pPointer);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPInt64(ParamBlk *parm)
{
	__int64 **pPointer = reinterpret_cast<__int64**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
		{
			if (PCount() == 1 || vp2.ev_long == 1)
				ReturnInt64AsBinary(**pPointer);
			else if (vp2.ev_long == 2)
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

void _fastcall ReadUInt64(ParamBlk *parm)
{
	unsigned __int64 *pPointer = reinterpret_cast<unsigned __int64*>(vp1.ev_long);
	if (pPointer)
	{
		if (PCount() == 1 || vp2.ev_long == 1)
			ReturnInt64AsBinary(*pPointer);
		else if (vp2.ev_long == 2)
			ReturnInt64AsString(*pPointer);
		else
			ReturnInt64AsDouble(*pPointer);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPUInt64(ParamBlk *parm)
{
	unsigned __int64 **pPointer = reinterpret_cast<unsigned __int64**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
		{
			if (PCount() == 1 || vp2.ev_long == 1)
				ReturnInt64AsBinary(**pPointer);
			else if (vp2.ev_long == 2)
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

void _fastcall ReadFloat(ParamBlk *parm)
{
	float *pPointer = reinterpret_cast<float*>(vp1.ev_long);
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPFloat(ParamBlk *parm)
{
	float **pPointer = reinterpret_cast<float**>(vp1.ev_long);
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

void _fastcall ReadDouble(ParamBlk *parm)
{
	double *pPointer = reinterpret_cast<double*>(vp1.ev_long);
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPDouble(ParamBlk *parm)
{
	double **pPointer = reinterpret_cast<double**>(vp1.ev_long);
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

void _fastcall ReadLogical(ParamBlk *parm)
{
	BOOL *pPointer = reinterpret_cast<BOOL*>(vp1.ev_long);
	if (pPointer)
		_RetLogical(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPLogical(ParamBlk *parm)
{
	BOOL **pPointer = reinterpret_cast<BOOL**>(vp1.ev_long);
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

void _fastcall ReadPointer(ParamBlk *parm)
{
	void **pPointer = reinterpret_cast<void**>(vp1.ev_long);
	if (pPointer)
		Return(*pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPPointer(ParamBlk *parm)
{
	void ***pPointer = reinterpret_cast<void***>(vp1.ev_long);
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

void _fastcall ReadChar(ParamBlk *parm)
{
	StringValue cChar(1);
	char *pPointer = reinterpret_cast<char*>(vp1.ev_long);
	char *pChar;
	if (pPointer)
	{
		if (AllocHandleEx(cChar,1))
		{
			pChar = HandleToPtr(cChar);
			*pChar = *pPointer;
			Return(cChar);
		}
		else
			RaiseError(E_INSUFMEMORY);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPChar(ParamBlk *parm)
{
	StringValue cChar(1);
	char **pPointer = reinterpret_cast<char**>(vp1.ev_long);
	char *pChar;

	if (pPointer)
	{
		if ((*pPointer))
		{
			if (AllocHandleEx(cChar,1))
			{
				pChar = HandleToPtr(cChar);
				*pChar = **pPointer;
				Return(cChar);
			}
			else
				RaiseError(E_INSUFMEMORY);
		}
		else if (PCount() == 1)
			ReturnNull();
		else
			Return(vp2);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadCString(ParamBlk *parm)
{
	char *pPointer = reinterpret_cast<char*>(vp1.ev_long);
	if (pPointer)
		Return(pPointer);
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadPCString(ParamBlk *parm)
{
	char **pPointer = reinterpret_cast<char**>(vp1.ev_long);
	if (pPointer)
	{
		if ((*pPointer))
            Return(*pPointer);
		else if (PCount() == 1)
		{
			char aNothing[1] = {'\0'};
			Return(aNothing);
		}
		else
			Return(vp2);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadCharArray(ParamBlk *parm)
{
	StringValue vBuffer;
	const char *pPointer = reinterpret_cast<const char*>(vp1.ev_long);

	if (pPointer)
	{
		if (AllocHandleEx(vBuffer, vp2.ev_long))
		{
			vBuffer.ev_length = strncpyex(HandleToPtr(vBuffer), pPointer, vp2.ev_long);
			Return(vBuffer);
		}
		else
			RaiseError(E_INSUFMEMORY);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}



void _fastcall ReadWString(ParamBlk *parm)
{
	StringValue vBuffer;
	int nStringLen, nBufferLen;
	wchar_t* pString = reinterpret_cast<wchar_t*>(vp1.ev_long);
	unsigned int nCodePage = PCount() == 1 ? VFP2CTls::Tls().ConvCP : vp2.ev_long;

	if (pString)
	{
		nStringLen = lstrlenW(pString);
		if (nStringLen)
		{
			nBufferLen = nStringLen * sizeof(wchar_t) + sizeof(wchar_t);
			if (AllocHandleEx(vBuffer, nBufferLen))
			{
				nBufferLen = WideCharToMultiByte(nCodePage, 0, pString, nStringLen, HandleToPtr(vBuffer), nBufferLen, 0, 0);
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

void _fastcall ReadPWString(ParamBlk *parm)
{
	StringValue vBuffer;
	wchar_t **pString = reinterpret_cast<wchar_t**>(vp1.ev_long);
	unsigned int nCodePage = PCount() > 1 && vp2.ev_long ? vp2.ev_long : VFP2CTls::Tls().ConvCP;
	int nStringLen, nBufferLen;

	if (pString)
	{
		if ((*pString))
		{
			nStringLen = lstrlenW(*pString);
			if (nStringLen)
			{
				nBufferLen = nStringLen * sizeof(wchar_t);
				if (AllocHandleEx(vBuffer, nBufferLen))
				{
					nBufferLen = WideCharToMultiByte(nCodePage, 0, *pString, nStringLen, HandleToPtr(vBuffer), nBufferLen, 0, 0);
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
		else if (PCount() < 3)
			Return(vBuffer);
		else
			Return(vp3);
	}
	else
		RaiseError(E_INVALIDPARAMS);
}

void _fastcall ReadWCharArray(ParamBlk *parm)
{
	StringValue vBuffer;
	wchar_t *pString = reinterpret_cast<wchar_t*>(vp1.ev_long);
	unsigned int nCodePage = PCount() == 2 ? VFP2CTls::Tls().ConvCP : vp3.ev_long;
	int nBufferLen, nStringLen;

	if (pString)
	{
		nStringLen = wstrnlen(pString, vp2.ev_long);
		if (nStringLen)
		{
			nBufferLen = nStringLen * sizeof(wchar_t);
			if (AllocHandleEx(vBuffer, nBufferLen))
			{
				nBufferLen = WideCharToMultiByte(nCodePage, 0, pString, nStringLen, HandleToPtr(vBuffer), nBufferLen, 0, 0);
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



void _fastcall ReadBytes(ParamBlk *parm)
{
	StringValue vBuffer(vp2.ev_long);
	void *pPointer = reinterpret_cast<void*>(vp1.ev_long);
	if (pPointer)
	{
		if (AllocHandleEx(vBuffer,vp2.ev_long))
		{
			memcpy(HandleToPtr(vBuffer), pPointer, vp2.ev_long);
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

void _fastcall MarshalFoxArray2CArray(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		throw nErrorNo;

	FoxArray vfpArray(rp2);
	MarshalType Type = static_cast<MarshalType>(vp3.ev_long);
	FoxValue pValue;
	HANDLE hHeap = VFP2CTls::Heap;

	switch(Type)
	{
		case CTYPE_SHORT:
			{
				short *CArray = reinterpret_cast<short*>(vp1.ev_long);
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				unsigned short *CArray = reinterpret_cast<unsigned short*>(vp1.ev_long);
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				int *CArray = reinterpret_cast<int*>(vp1.ev_long);
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				unsigned int *CArray = reinterpret_cast<unsigned int*>(vp1.ev_long);
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				float *CArray = reinterpret_cast<float*>(vp1.ev_long);
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				double *CArray = reinterpret_cast<double*>(vp1.ev_long);
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				BOOL *CArray = reinterpret_cast<BOOL*>(vp1.ev_long);
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				char **CArray = reinterpret_cast<char**>(vp1.ev_long);
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				wchar_t **CArray = reinterpret_cast<wchar_t**>(vp1.ev_long);
				int nCharsWritten;
				unsigned int nCodePage = PCount() == 3 ? VFP2CTls::Tls().ConvCP : vp4.ev_long;
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				if (PCount() < 4)
					throw E_INVALIDPARAMS;

				char *CArray = reinterpret_cast<char*>(vp1.ev_long);
				unsigned int nCharCount, nLength = vp4.ev_long;
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				if (PCount() < 4)
					throw E_INVALIDPARAMS;

				wchar_t *CArray = reinterpret_cast<wchar_t*>(vp1.ev_long);
				unsigned int nCodePage, nCharsWritten, nLength = vp4.ev_long;
				nCodePage = PCount() == 4 ? VFP2CTls::Tls().ConvCP : vp5.ev_long;
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				__int64 *CArray = reinterpret_cast<__int64*>(vp1.ev_long);
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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
				unsigned __int64 *CArray = reinterpret_cast<unsigned __int64*>(vp1.ev_long);
				BEGIN_ARRAYLOOP()
					pValue = vfpArray;
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

void _fastcall MarshalCArray2FoxArray(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		throw nErrorNo;

	FoxArray vfpArray(rp2);
	MarshalType Type = static_cast<MarshalType>(vp3.ev_long);

	switch(Type)
	{
		case CTYPE_SHORT:
			{
				short *CArray = reinterpret_cast<short*>(vp1.ev_long);
				FoxShort pValue;
				BEGIN_ARRAYLOOP()
					vfpArray = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_USHORT:
			{
				unsigned short *CArray = reinterpret_cast<unsigned short*>(vp1.ev_long);
				FoxUShort pValue;
				BEGIN_ARRAYLOOP()
					vfpArray = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_INT:
			{
				int *CArray = reinterpret_cast<int*>(vp1.ev_long);
				FoxInt pValue;
				BEGIN_ARRAYLOOP()
					vfpArray = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_UINT:
			{
				unsigned int *CArray = reinterpret_cast<unsigned int*>(vp1.ev_long);
				FoxUInt pValue;
				BEGIN_ARRAYLOOP()
					vfpArray = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_FLOAT:
			{
				float *CArray = reinterpret_cast<float*>(vp1.ev_long);
				FoxFloat pValue;
				BEGIN_ARRAYLOOP()
					vfpArray = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_DOUBLE:
			{
				double *CArray = reinterpret_cast<double*>(vp1.ev_long);
				FoxDouble pValue;
				BEGIN_ARRAYLOOP()
					vfpArray = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_BOOL:
			{
				BOOL *CArray = reinterpret_cast<BOOL*>(vp1.ev_long);
				FoxLogical pValue;
				BEGIN_ARRAYLOOP()
					vfpArray = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_CSTRING:
			{
				char **CArray = reinterpret_cast<char**>(vp1.ev_long);
				unsigned int nStringLen;
				FoxString pValue(256);
				FoxValue pNull;

				BEGIN_ARRAYLOOP()
					if (*CArray)
					{
						nStringLen = lstrlen(*CArray);
						if (nStringLen > pValue.Size())
							pValue.Size(nStringLen);
						if (nStringLen)
							memcpy(pValue, *CArray, nStringLen);
						pValue.Len(nStringLen);
						vfpArray = pValue;
					}
					else
						vfpArray = pNull;
					
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_WSTRING:
			{
				wchar_t **CArray = reinterpret_cast<wchar_t**>(vp1.ev_long);
				unsigned int nByteCount, nWCharCount, nCharsWritten;
				unsigned int nCodePage = PCount() == 3 ? VFP2CTls::Tls().ConvCP : vp4.ev_long;;
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

						vfpArray = pValue;
					}
					else
						vfpArray = pNull;
				
					CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_CHARARRAY:
			{
				if (PCount() != 4)
					throw E_INVALIDPARAMS;
				
				char *CArray = reinterpret_cast<char*>(vp1.ev_long);
				unsigned int nLen = vp4.ev_long;
				FoxString pValue(nLen);
				BEGIN_ARRAYLOOP()
					vfpArray = pValue.StrnCpy(CArray, nLen);
					CArray += nLen;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_WCHARARRAY:
			{
				if (PCount() < 4)
					throw E_INVALIDPARAMS;

				wchar_t *CArray = reinterpret_cast<wchar_t*>(vp1.ev_long);
				int nCharCount, nLen = vp4.ev_long;
				unsigned int nCodePage = PCount() == 4 ? VFP2CTls::Tls().ConvCP : vp5.ev_long;
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
					vfpArray = pValue;
					CArray += nLen;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_INT64:
			{
				__int64 *CArray = reinterpret_cast<__int64*>(vp1.ev_long);
				FoxCurrency pValue;
				BEGIN_ARRAYLOOP()
					vfpArray = pValue = *CArray++;
				END_ARRAYLOOP()
			}
			break;

		case CTYPE_UINT64:
			{
				unsigned __int64 *CArray = reinterpret_cast<unsigned __int64*>(vp1.ev_long);
				FoxCurrency pValue;
				BEGIN_ARRAYLOOP()
					vfpArray = pValue = *CArray++;
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

void _fastcall MarshalCursor2CArray(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		throw nErrorNo;

	FoxValue pValue;
	FoxString pCursorAndFields(vp2);
	MarshalType Type = static_cast<MarshalType>(vp3.ev_long);
	FoxCursor pCursor;
	VFP2CTls& tls = VFP2CTls::Tls();
		
	char CursorName[VFP_MAX_CURSOR_NAME];
	char *pFieldNames = pCursorAndFields;
	pFieldNames += GetWordNumN(CursorName, pCursorAndFields, '.', 1, VFP_MAX_CURSOR_NAME) + 1;
	pCursor.Attach(CursorName, pFieldNames);
	pCursor.GoTop();
	int nRecCount = pCursor.RecCount();
	unsigned int nFieldCount = pCursor.FCount();
	
	switch(Type)
	{
		case CTYPE_SHORT:
			{
				short *CArray = reinterpret_cast<short*>(vp1.ev_long);
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
				unsigned short *CArray = reinterpret_cast<unsigned short*>(vp1.ev_long);
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
				int *CArray = reinterpret_cast<int*>(vp1.ev_long);
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
				unsigned int *CArray = reinterpret_cast<unsigned int*>(vp1.ev_long);
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
				float *CArray = reinterpret_cast<float*>(vp1.ev_long);
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
				double *CArray = reinterpret_cast<double*>(vp1.ev_long);
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
				BOOL *CArray = reinterpret_cast<BOOL*>(vp1.ev_long);
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
				char **CArray = reinterpret_cast<char**>(vp1.ev_long);
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
				wchar_t **CArray = reinterpret_cast<wchar_t**>(vp1.ev_long);
				wchar_t **pString;
				int nCharsWritten;
				unsigned int nCodePage = PCount() == 3 ? VFP2CTls::Tls().ConvCP : vp4.ev_long;
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
				char *CArray = reinterpret_cast<char*>(vp1.ev_long);
				char *pString;
				unsigned int nCharCount, nDimensionSize, nLength = vp4.ev_long;
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
				if (PCount() < 4)
					throw E_INVALIDPARAMS;

				wchar_t *CArray = reinterpret_cast<wchar_t*>(vp1.ev_long);
				wchar_t *pString;
				unsigned int nByteLen, nCharsWritten, nDimensionSize, nLen = vp4.ev_long;
				unsigned int nCodePage = PCount() == 4 ? tls.ConvCP : vp5.ev_long;
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
				__int64 *CArray = reinterpret_cast<__int64*>(vp1.ev_long);
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
				unsigned __int64 *CArray = reinterpret_cast<unsigned __int64*>(vp1.ev_long);
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

void _fastcall MarshalCArray2Cursor(ParamBlk *parm)
{
try
{
	int nErrorNo = VFP2C_Init_Marshal();
	if (nErrorNo)
		throw nErrorNo;

	FoxString pCursorAndFields(vp2);
	MarshalType Type = static_cast<MarshalType>(vp3.ev_long);
	FoxCursor pCursor;
	VFP2CTls& tls = VFP2CTls::Tls();

	unsigned int nFieldCount;
	unsigned int nRowCount = vp4.ev_long;
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
				short *CArray = reinterpret_cast<short*>(vp1.ev_long);
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
				unsigned short *CArray = reinterpret_cast<unsigned short*>(vp1.ev_long);
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
				int *CArray = reinterpret_cast<int*>(vp1.ev_long);
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
				unsigned int *CArray = reinterpret_cast<unsigned int*>(vp1.ev_long);
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
				float *CArray = reinterpret_cast<float*>(vp1.ev_long);
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
				double *CArray = reinterpret_cast<double*>(vp1.ev_long);
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
				BOOL *CArray = reinterpret_cast<BOOL*>(vp1.ev_long);
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
				char **CArray = reinterpret_cast<char**>(vp1.ev_long);
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
								memcpy(pValue, pString, nStringLen);
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
				wchar_t **CArray = reinterpret_cast<wchar_t**>(vp1.ev_long);
				wchar_t *pString;
				unsigned int nByteCount, nWCharCount, nCharsWritten;
				UINT nCodePage = PCount() == 4 ? tls.ConvCP : vp5.ev_long;;
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
				if (PCount() != 5)
					throw E_INVALIDPARAMS;
				
				char *CArray = reinterpret_cast<char*>(vp1.ev_long);
				char *pString;
				unsigned int nLen = vp5.ev_long;
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
				if (PCount() < 5)
					throw E_INVALIDPARAMS;

				wchar_t *CArray = reinterpret_cast<wchar_t*>(vp1.ev_long);
				wchar_t *pString;
				int nCharCount, nByteLen;
				unsigned int nLen = vp5.ev_long;
				nByteLen = nLen * sizeof(wchar_t);
				unsigned int nDimensionSize = nByteLen * nRowCount;
				UINT nCodePage = PCount() == 5 ? tls.ConvCP : vp6.ev_long;
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
				__int64 *CArray = reinterpret_cast<__int64*>(vp1.ev_long);
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
				unsigned __int64 *CArray = reinterpret_cast<unsigned __int64*>(vp1.ev_long);
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