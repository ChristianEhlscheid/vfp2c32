#include <atlbase.h>

#include "vfp2c32.h"
#include "vfp2ctls.h"
#include "vfp2ccallback.h"

const SIZE_T JITHEAP_MAXIMUM_SIZE = 1024 * 1024 * 8; // 8MB should suffice for dynamic code

#if defined(_WIN64)
PRUNTIME_FUNCTION VFP2C64FunctionTableCallback(DWORD64 ControlPc, PVOID Context)
{
	VFP2CTls& tls = VFP2CTls::Tls();
	DWORD VirtualAddress = (ControlPc - tls.JitBaseAddress);
	int nCount = tls.RuntimeFunctions.GetCount();
	PRUNTIME_FUNCTION rf;
	for (int xj = 0; xj < nCount; xj++)
	{
		rf = &tls.RuntimeFunctions[xj];
		if (VirtualAddress >= rf->BeginAddress && VirtualAddress <= rf->EndAddress)
			return rf;
	}
	return 0;
}
#endif

#ifdef _THREADSAFE
DWORD VFP2CTls::TlsIndex = TLS_OUT_OF_INDEXES;
LONG VFP2CTls::ThreadCount = 0;
#endif

HANDLE VFP2CTls::Heap = 0;
HMODULE VFP2CTls::RasApi32 = 0;
HMODULE VFP2CTls::RasDlg = 0;
HMODULE VFP2CTls::IpHlpApi = 0;
HMODULE VFP2CTls::Icmp = 0;
HMODULE VFP2CTls::NetApi32 = 0;

VFP2CTls::VFP2CTls()
{
	ErrorCount = -1; 
	ZeroMemory(ErrorInfo, sizeof(ErrorInfo));
	InitStatus = 0;
	ConvCP = CP_ACP; 
	SqlUnicodeConversion = TRUE;
#ifdef _DEBUG
	DbgInfo = 0;
	TrackAlloc = FALSE;
#endif
	WinsockInited = FALSE;
	DefaultWinsockTimeout = 5000;
	JitHeap = 0;
#if defined(_WIN64)
	JitBaseAddress = 0;
#endif
}

VFP2CTls::~VFP2CTls()
{
#if defined(_WIN64)
	if (JitBaseAddress != 0)
	{
		PRUNTIME_FUNCTION nTableIdentifier = (PRUNTIME_FUNCTION)(JitBaseAddress | 0x3);
		RtlDeleteFunctionTable(nTableIdentifier);
	}
#endif

	if (JitHeap != 0)
		HeapDestroy(JitHeap);
}

void VFP2CTls::OnLoad()
{
#ifdef _THREADSAFE
	LONG Count = InterlockedIncrement(&ThreadCount);
	if (Count == 1) // first thread that loads VFP2C32
	{
		TlsIndex = TlsAlloc();
		if (TlsIndex == TLS_OUT_OF_INDEXES)
		{
			SaveWin32Error("TlsAlloc", GetLastError());
			return;
		}
	}

	VFP2CTls *tls = new VFP2CTls();
	if (!tls)
	{
		SaveWin32Error("VFP2CTls::OnLoad", E_OUTOFMEMORY);
		return;
	}

	if (!TlsSetValue(TlsIndex, tls))
	{
		SaveWin32Error("TlsSetValue", GetLastError());
		return;
	}
#else
	// this call will initialize the static version for nonthreadsafe usage
	VFP2CTls& tls = VFP2CTls::Tls();
#endif
}

void VFP2CTls::OnUnload()
{
#ifdef _THREADSAFE
	if (TlsIndex != TLS_OUT_OF_INDEXES)
	{
		VFP2CTls* tls = (VFP2CTls*)TlsGetValue(TlsIndex);
		if (tls)
			delete tls;
	}

	LONG Count = InterlockedDecrement(&ThreadCount);
	if (Count == 0) // last thread that unloads VFP2C32
	{
		if (TlsIndex != TLS_OUT_OF_INDEXES)
			TlsFree(TlsIndex);
		FreeGlobalResources();
	}
#else
	FreeGlobalResources();
#endif
	return;
}

void VFP2CTls::FreeGlobalResources()
{
	if (RasApi32)
		FreeLibrary(RasApi32);
	if (RasDlg)
		FreeLibrary(RasDlg);
	if (IpHlpApi)
		FreeLibrary(IpHlpApi);
	if (Icmp)
		FreeLibrary(Icmp);
	if (NetApi32)
		FreeLibrary(NetApi32);
	if (Heap)
		HeapDestroy(Heap);
}

VFP2CTls& _stdcall VFP2CTls::Tls()
{
#ifdef _THREADSAFE
	VFP2CTls* tls = (VFP2CTls*)TlsGetValue(TlsIndex);
	return *tls;
#else
	static VFP2CTls tls;
	return tls;
#endif
}

HANDLE VFP2CTls::GetJitHeap()
{
	if (JitHeap == 0)
	{
#ifndef HEAP_CREATE_ENABLE_EXECUTE
#define HEAP_CREATE_ENABLE_EXECUTE      0x00040000  
#endif

#if !defined(_WIN64)
		DWORD nHeapOptions = HEAP_CREATE_ENABLE_EXECUTE | HEAP_NO_SERIALIZE;
		DWORD nHeapSize = 0;
#else
		DWORD nHeapOptions = HEAP_CREATE_ENABLE_EXECUTE | HEAP_NO_SERIALIZE;
		DWORD nHeapSize = JITHEAP_MAXIMUM_SIZE;
#endif
		if ((JitHeap = HeapCreate(nHeapOptions, 1024 * 16, nHeapSize)) == 0)
		{
			SaveWin32Error("HeapCreate", GetLastError());
			throw E_APIERROR;
		}

#if defined(_WIN64)
		JitBaseAddress = (DWORD64)HeapAlloc(JitHeap, 0, 8);
		if (JitBaseAddress == 0)
		{
			SaveWin32Error("HeapAlloc", GetLastError());
			throw E_APIERROR;
		}
		DWORD64 nTableIdentifier = JitBaseAddress | 0x3;
		HeapFree(JitHeap, 0, (LPVOID)JitBaseAddress);
		if (RtlInstallFunctionTableCallback(nTableIdentifier, JitBaseAddress, nHeapSize,
			&VFP2C64FunctionTableCallback, 0, 0) == 0)
		{
			SaveWin32Error("RtlInstallFunctionTableCallback", GetLastError());
			throw E_APIERROR;
		}
#endif
	}
	return JitHeap;
}

