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
} MarshalType;

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
void _fastcall TrackMem(ParamBlkEx& parm);
void _fastcall AMemLeaks(ParamBlkEx& parm);
#endif

void _fastcall AllocMem(ParamBlkEx& parm);
void _fastcall AllocMemTo(ParamBlkEx& parm);
void _fastcall ReAllocMem(ParamBlkEx& parm);
void _fastcall FreeMem(ParamBlkEx& parm);
void _fastcall FreePMem(ParamBlkEx& parm);
void _fastcall FreeRefArray(ParamBlkEx& parm);
void _fastcall SizeOfMem(ParamBlkEx& parm);
void _fastcall ValidateMem(ParamBlkEx& parm);
void _fastcall CompactMem(ParamBlkEx& parm);
void _fastcall AMemBlocks(ParamBlkEx& parm);

void _fastcall AllocHGlobal(ParamBlkEx& parm);
void _fastcall FreeHGlobal(ParamBlkEx& parm);
void _fastcall ReAllocHGlobal(ParamBlkEx& parm);
void _fastcall LockHGlobal(ParamBlkEx& parm);
void _fastcall UnlockHGlobal(ParamBlkEx& parm);

void _fastcall WriteInt8(ParamBlkEx& parm);
void _fastcall WritePInt8(ParamBlkEx& parm);
void _fastcall WriteUInt8(ParamBlkEx& parm);
void _fastcall WritePUInt8(ParamBlkEx& parm);
void _fastcall WriteShort(ParamBlkEx& parm);
void _fastcall WritePShort(ParamBlkEx& parm);
void _fastcall WriteUShort(ParamBlkEx& parm);
void _fastcall WritePUShort(ParamBlkEx& parm);
void _fastcall WriteInt(ParamBlkEx& parm);
void _fastcall WritePInt(ParamBlkEx& parm);
void _fastcall WriteUInt(ParamBlkEx& parm);
void _fastcall WritePUInt(ParamBlkEx& parm);
void _fastcall WriteInt64(ParamBlkEx& parm);
void _fastcall WritePInt64(ParamBlkEx& parm);
void _fastcall WriteUInt64(ParamBlkEx& parm);
void _fastcall WritePUInt64(ParamBlkEx& parm);
void _fastcall WriteFloat(ParamBlkEx& parm);
void _fastcall WritePFloat(ParamBlkEx& parm);
void _fastcall WriteDouble(ParamBlkEx& parm);
void _fastcall WritePDouble(ParamBlkEx& parm);
void _fastcall WriteLogical(ParamBlkEx& parm);
void _fastcall WritePLogical(ParamBlkEx& parm);
void _fastcall WritePointer(ParamBlkEx& parm);
void _fastcall WritePPointer(ParamBlkEx& parm);
void _fastcall WriteChar(ParamBlkEx& parm);
void _fastcall WritePChar(ParamBlkEx& parm);
void _fastcall WriteWChar(ParamBlkEx& parm);
void _fastcall WritePWChar(ParamBlkEx& parm);
void _fastcall WriteCString(ParamBlkEx& parm);
void _fastcall WritePCString(ParamBlkEx& parm);
void _fastcall WriteGPCString(ParamBlkEx& parm);
void _fastcall WriteCharArray(ParamBlkEx& parm);
void _fastcall WriteWString(ParamBlkEx& parm);
void _fastcall WritePWString(ParamBlkEx& parm);
void _fastcall WriteWCharArray(ParamBlkEx& parm);
void _fastcall WriteBytes(ParamBlkEx& parm);

void _fastcall ReadInt8(ParamBlkEx& parm);
void _fastcall ReadPInt8(ParamBlkEx& parm);
void _fastcall ReadUInt8(ParamBlkEx& parm);
void _fastcall ReadPUInt8(ParamBlkEx& parm);
void _fastcall ReadShort(ParamBlkEx& parm);
void _fastcall ReadPShort(ParamBlkEx& parm);
void _fastcall ReadUShort(ParamBlkEx& parm);
void _fastcall ReadPUShort(ParamBlkEx& parm);
void _fastcall ReadInt(ParamBlkEx& parm);
void _fastcall ReadPInt(ParamBlkEx& parm);
void _fastcall ReadUInt(ParamBlkEx& parm);
void _fastcall ReadPUInt(ParamBlkEx& parm);
void _fastcall ReadInt64(ParamBlkEx& parm);
void _fastcall ReadPInt64(ParamBlkEx& parm);
void _fastcall ReadUInt64(ParamBlkEx& parm);
void _fastcall ReadPUInt64(ParamBlkEx& parm);
void _fastcall ReadFloat(ParamBlkEx& parm);
void _fastcall ReadPFloat(ParamBlkEx& parm);
void _fastcall ReadDouble(ParamBlkEx& parm);
void _fastcall ReadPDouble(ParamBlkEx& parm);
void _fastcall ReadLogical(ParamBlkEx& parm);
void _fastcall ReadPLogical(ParamBlkEx& parm);
void _fastcall ReadPointer(ParamBlkEx& parm);
void _fastcall ReadPPointer(ParamBlkEx& parm);
void _fastcall ReadChar(ParamBlkEx& parm);
void _fastcall ReadPChar(ParamBlkEx& parm);
void _fastcall ReadCString(ParamBlkEx& parm);
void _fastcall ReadPCString(ParamBlkEx& parm);
void _fastcall ReadCharArray(ParamBlkEx& parm);
void _fastcall ReadWString(ParamBlkEx& parm);
void _fastcall ReadPWString(ParamBlkEx& parm);
void _fastcall ReadWCharArray(ParamBlkEx& parm);
void _fastcall ReadBytes(ParamBlkEx& parm);

void _fastcall MarshalFoxArray2CArray(ParamBlkEx& parm);
void _fastcall MarshalCArray2FoxArray(ParamBlkEx& parm);

void _fastcall MarshalCursor2CArray(ParamBlkEx& parm);
void _fastcall MarshalCArray2Cursor(ParamBlkEx& parm);

#ifdef __cplusplus
}
#endif // end of extern "C"

#endif // _VFP2CMARSHAL_H__