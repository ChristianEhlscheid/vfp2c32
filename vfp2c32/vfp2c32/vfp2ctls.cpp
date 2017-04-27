#include <atlbase.h>

#include "vfp2c32.h"
#include "vfp2ctls.h"

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
#ifdef _DEBUG
	DbgInfo = 0;
	TrackAlloc = FALSE;
#endif
	WinsockInited = FALSE;
	DefaultWinsockTimeout = 5000;
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