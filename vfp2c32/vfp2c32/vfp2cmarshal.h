#ifndef _VFP2CMARSHAL_H__
#define _VFP2CMARSHAL_H__

// additional heap defines
const int HEAP_INITIAL_SIZE	= 1024 * 1024;	// 1 MB

#define SAVEHEAPEXCEPTION()	Win32HeapExceptionHandler(GetExceptionCode())

// debugging macros and types
#ifdef _DEBUG

#define ADDDEBUGALLOC(pPointer,nSize) AddDebugAlloc((void*)pPointer,nSize)
#define REMOVEDEBUGALLOC(pPointer) RemoveDebugAlloc((void*)pPointer)
#define REPLACEDEBUGALLOC(pOrig,pNew,nSize) ReplaceDebugAlloc((void*)pOrig,(void*)pNew,nSize)

#else

#define ADDDEBUGALLOC(pPointer,nSize)
#define REMOVEDEBUGALLOC(pPointer)
#define REPLACEDEBUGALLOC(pOrig,pNew,nSize)

#endif

typedef enum MarshalType
{
	CTYPE_SHORT = 0,
	CTYPE_USHORT,
	CTYPE_INT, 
	CTYPE_UINT,
	CTYPE_FLOAT,
	CTYPE_DOUBLE,
	CTYPE_BOOL,
	CTYPE_CSTRING,
	CTYPE_WSTRING,
	CTYPE_CHARARRAY,
	CTYPE_WCHARARRAY,
	CTYPE_INT64,
	CTYPE_UINT64
};

// typedefs for dynamic linking to heap functions
typedef BOOL (_stdcall *PHEAPSETINFO)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T); // HeapSetInformation

#ifdef __cplusplus
extern "C" {
#endif

// function forward definitions
int _stdcall VFP2C_Init_Marshal();
void _stdcall VFP2C_Destroy_Marshal(VFP2CTls& tls);

int _stdcall Win32HeapExceptionHandler(int nExceptionCode);

#ifdef _DEBUG
void _stdcall AddDebugAlloc(void* pPointer, int nSize);
void _stdcall RemoveDebugAlloc(void* pPointer);
void _stdcall ReplaceDebugAlloc(void* pOrig, void *pNew, int nSize);
void _stdcall FreeDebugAlloc();
void _fastcall TrackMem(ParamBlk *parm);
void _fastcall AMemLeaks(ParamBlk *parm);
#endif

void _fastcall AllocMem(ParamBlk *parm);
void _fastcall AllocMemTo(ParamBlk *parm);
void _fastcall ReAllocMem(ParamBlk *parm);
void _fastcall FreeMem(ParamBlk *parm);
void _fastcall FreePMem(ParamBlk *parm);
void _fastcall FreeRefArray(ParamBlk *parm);
void _fastcall SizeOfMem(ParamBlk *parm);
void _fastcall ValidateMem(ParamBlk *parm);
void _fastcall CompactMem(ParamBlk *parm);
void _fastcall AMemBlocks(ParamBlk *parm);

void _fastcall AllocHGlobal(ParamBlk *parm);
void _fastcall FreeHGlobal(ParamBlk *parm);
void _fastcall ReAllocHGlobal(ParamBlk *parm);
void _fastcall LockHGlobal(ParamBlk *parm);
void _fastcall UnlockHGlobal(ParamBlk *parm);

void _fastcall WriteInt8(ParamBlk *parm);
void _fastcall WritePInt8(ParamBlk *parm);
void _fastcall WriteUInt8(ParamBlk *parm);
void _fastcall WritePUInt8(ParamBlk *parm);
void _fastcall WriteShort(ParamBlk *parm);
void _fastcall WritePShort(ParamBlk *parm);
void _fastcall WriteUShort(ParamBlk *parm);
void _fastcall WritePUShort(ParamBlk *parm);
void _fastcall WriteInt(ParamBlk *parm);
void _fastcall WritePInt(ParamBlk *parm);
void _fastcall WriteUInt(ParamBlk *parm);
void _fastcall WritePUInt(ParamBlk *parm);
void _fastcall WriteInt64(ParamBlk *parm);
void _fastcall WritePInt64(ParamBlk *parm);
void _fastcall WriteUInt64(ParamBlk *parm);
void _fastcall WritePUInt64(ParamBlk *parm);
void _fastcall WriteFloat(ParamBlk *parm);
void _fastcall WritePFloat(ParamBlk *parm);
void _fastcall WriteDouble(ParamBlk *parm);
void _fastcall WritePDouble(ParamBlk *parm);
void _fastcall WriteLogical(ParamBlk *parm);
void _fastcall WritePLogical(ParamBlk *parm);
void _fastcall WritePointer(ParamBlk *parm);
void _fastcall WritePPointer(ParamBlk *parm);
void _fastcall WriteChar(ParamBlk *parm);
void _fastcall WritePChar(ParamBlk *parm);
void _fastcall WriteWChar(ParamBlk *parm);
void _fastcall WritePWChar(ParamBlk *parm);
void _fastcall WriteCString(ParamBlk *parm);
void _fastcall WritePCString(ParamBlk *parm);
void _fastcall WriteGPCString(ParamBlk *parm);
void _fastcall WriteCharArray(ParamBlk *parm);
void _fastcall WriteWString(ParamBlk *parm);
void _fastcall WritePWString(ParamBlk *parm);
void _fastcall WriteWCharArray(ParamBlk *parm);
void _fastcall WriteBytes(ParamBlk *parm);

void _fastcall ReadInt8(ParamBlk *parm);
void _fastcall ReadPInt8(ParamBlk *parm);
void _fastcall ReadUInt8(ParamBlk *parm);
void _fastcall ReadPUInt8(ParamBlk *parm);
void _fastcall ReadShort(ParamBlk *parm);
void _fastcall ReadPShort(ParamBlk *parm);
void _fastcall ReadUShort(ParamBlk *parm);
void _fastcall ReadPUShort(ParamBlk *parm);
void _fastcall ReadInt(ParamBlk *parm);
void _fastcall ReadPInt(ParamBlk *parm);
void _fastcall ReadUInt(ParamBlk *parm);
void _fastcall ReadPUInt(ParamBlk *parm);
void _fastcall ReadInt64(ParamBlk *parm);
void _fastcall ReadPInt64(ParamBlk *parm);
void _fastcall ReadUInt64(ParamBlk *parm);
void _fastcall ReadPUInt64(ParamBlk *parm);
void _fastcall ReadFloat(ParamBlk *parm);
void _fastcall ReadPFloat(ParamBlk *parm);
void _fastcall ReadDouble(ParamBlk *parm);
void _fastcall ReadPDouble(ParamBlk *parm);
void _fastcall ReadLogical(ParamBlk *parm);
void _fastcall ReadPLogical(ParamBlk *parm);
void _fastcall ReadPointer(ParamBlk *parm);
void _fastcall ReadPPointer(ParamBlk *parm);
void _fastcall ReadChar(ParamBlk *parm);
void _fastcall ReadPChar(ParamBlk *parm);
void _fastcall ReadCString(ParamBlk *parm);
void _fastcall ReadPCString(ParamBlk *parm);
void _fastcall ReadCharArray(ParamBlk *parm);
void _fastcall ReadWString(ParamBlk *parm);
void _fastcall ReadPWString(ParamBlk *parm);
void _fastcall ReadWCharArray(ParamBlk *parm);
void _fastcall ReadBytes(ParamBlk *parm);

void _fastcall MarshalFoxArray2CArray(ParamBlk *parm);
void _fastcall MarshalCArray2FoxArray(ParamBlk *parm);

void _fastcall MarshalCursor2CArray(ParamBlk *parm);
void _fastcall MarshalCArray2Cursor(ParamBlk *parm);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif // _VFP2CMARSHAL_H__