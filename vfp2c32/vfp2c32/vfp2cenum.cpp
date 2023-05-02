#include "vfp2c32.h"
#include "vfp2cenum.h"

void _fastcall AWindowStations(ParamBlkEx& parm)
{
try
{
	EnumParameter sEnum;
	// initialize array
	sEnum.pArray.Dimension(parm(1),1);

	if (!EnumWindowStations(WindowStationEnumCallback,reinterpret_cast<LPARAM>(&sEnum)))
	{
		SaveWin32Error("EnumWindowStations", GetLastError());
		throw E_APIERROR;
	}

	sEnum.pArray.ReturnRows();
}
catch (int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

BOOL _stdcall WindowStationEnumCallback(LPSTR lpszWinSta, LPARAM nParam)
{
	EnumParameter *pEnum = reinterpret_cast<EnumParameter*>(nParam);
	pEnum->pArray.Grow();
	pEnum->pArray() = pEnum->pName = lpszWinSta;
	return TRUE;
} 

void _fastcall ADesktops(ParamBlkEx& parm)
{
try
{
	EnumParameter sEnum;
	HWINSTA hWinSta;

	sEnum.pArray.Dimension(parm(1),1);
	hWinSta = parm.PCount() == 1 ? GetProcessWindowStation() : parm(2)->Ptr<HWINSTA>();
	
	if (!EnumDesktops(hWinSta, (DESKTOPENUMPROC)DesktopEnumCallback, reinterpret_cast<LPARAM>(&sEnum)))
	{
		SaveWin32Error("EnumDesktops", GetLastError());
		throw E_APIERROR;
	}

	sEnum.pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

BOOL _stdcall DesktopEnumCallback(LPCSTR lpszDesktop, LPARAM nParam)
{
	EnumParameter *pEnum = reinterpret_cast<EnumParameter*>(nParam);
	pEnum->pArray.Grow();
	pEnum->pArray() = pEnum->pName = (LPSTR)lpszDesktop;
	return TRUE;  
}

void _fastcall AWindows(ParamBlkEx& parm)
{
try
{
	WindowEnumParam sEnum;
	FoxString pArrayOrCallback(parm(1));
	DWORD nEnumFlag = parm(2)->ev_long;
	DWORD nLastError = ERROR_SUCCESS;
	bool bCallback = (nEnumFlag & WINDOW_ENUM_CALLBACK) > 0;

	// are parameters valid?
	if (parm.PCount() == 2 && !(nEnumFlag & WINDOW_ENUM_TOPLEVEL))
	{
		SaveCustomError("AWindows", "Passed parameter 'nType' requires parameter 'nParamType'.");
		throw E_INVALIDPARAMS;
	}
	else if (!(nEnumFlag & (WINDOW_ENUM_TOPLEVEL | WINDOW_ENUM_CHILD | WINDOW_ENUM_THREAD | WINDOW_ENUM_DESKTOP)))
	{
		SaveCustomError("AWindows", "Invalid parameter nType '%U'", nEnumFlag);
		throw E_INVALIDPARAMS;
	}
	
	if (bCallback && (pArrayOrCallback.Len() > VFP2C_MAX_CALLBACKFUNCTION || pArrayOrCallback.Len() == 0))
	{
		SaveCustomError("AWindows", "Callback function length is zero or greater than maximum length of 1024.");
		throw E_INVALIDPARAMS;
	}
	
	if (bCallback)
	{
		sEnum.pCallback.SetCallback(pArrayOrCallback);
	}
	else
	{
		sEnum.pArray.Dimension(pArrayOrCallback, 1);
		sEnum.pArray.AutoGrow(16);
	}

	if (nEnumFlag & WINDOW_ENUM_TOPLEVEL)
	{
		if (!EnumWindows(bCallback ? WindowEnumCallbackCall : WindowEnumCallback, reinterpret_cast<LPARAM>(&sEnum)))
			nLastError = GetLastError();
	}
	else if (nEnumFlag & WINDOW_ENUM_CHILD)
	{
		if (!EnumChildWindows(parm(3)->Ptr<HWND>(), bCallback ? WindowEnumCallbackCall : WindowEnumCallback, reinterpret_cast<LPARAM>(&sEnum)))
			nLastError = GetLastError();
	}
	else if (nEnumFlag & WINDOW_ENUM_THREAD)
	{
		if (!EnumThreadWindows(parm(3)->ev_long, bCallback ? WindowEnumCallbackCall : WindowEnumCallback, reinterpret_cast<LPARAM>(&sEnum)))
			nLastError = GetLastError();
	}
	else if (nEnumFlag & WINDOW_ENUM_DESKTOP)
	{
		if (!EnumDesktopWindows(parm(3)->Ptr<HDESK>(), bCallback ? WindowEnumCallbackCall : WindowEnumCallback, reinterpret_cast<LPARAM>(&sEnum)))
			nLastError = GetLastError();
	}

	if (nLastError != ERROR_SUCCESS)
	{
		SaveWin32Error("EnumWindowsX", nLastError);
		throw E_APIERROR;
	}

	if (bCallback)
		Return(1);
	else
		sEnum.pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

BOOL _stdcall WindowEnumCallback(HWND nHwnd, LPARAM nParam)
{
	WindowEnumParam *pEnum = reinterpret_cast<WindowEnumParam*>(nParam);
	pEnum->pArray.Grow();
	pEnum->pArray() = nHwnd;
	return TRUE;		
}

#pragma warning(disable : 4290)
BOOL _stdcall WindowEnumCallbackCall(HWND nHwnd, LPARAM nParam) throw(int)
{
	WindowEnumParam *pEnum = reinterpret_cast<WindowEnumParam*>(nParam);
	ValueEx vRetVal;
	if (pEnum->pCallback.Evaluate(vRetVal, nHwnd) == 0)
	{
		if (vRetVal.Vartype() == 'L')
			return vRetVal.ev_length;
		else
			vRetVal.Release();
	}
	return 0;
}
#pragma warning(default : 4290)

void _fastcall AWindowsEx(ParamBlkEx& parm)
{
try
{
	FoxString pFlags(parm(2));
	WindowEnumParamEx sEnum;
	
	// are parameters valid?
	if (parm.PCount() == 3 && parm(3)->ev_long != WINDOW_ENUM_TOPLEVEL)
	{
		SaveCustomError("AWindows", "Passed parameter 'nType' requires parameter 'nParamType'.");
		throw E_INVALIDPARAMS;

	}
	else if (parm(3)->ev_long < WINDOW_ENUM_TOPLEVEL || parm(3)->ev_long > WINDOW_ENUM_DESKTOP)
	{
		SaveCustomError("AWindowsEx", "Invalid parameter nType '%U'", parm(3)->ev_long);
		throw E_INVALIDPARAMS;
	}

	sEnum.aFlags[0] = pFlags.At('W',1,WINDOW_ENUM_FLAGS); // HWND
	sEnum.aFlags[1] = pFlags.At('C',1,WINDOW_ENUM_FLAGS); // WindowClass
	sEnum.aFlags[2] = pFlags.At('T',1,WINDOW_ENUM_FLAGS); // WindowText
	sEnum.aFlags[3] = pFlags.At('S',1,WINDOW_ENUM_FLAGS); // Style
	sEnum.aFlags[4] = pFlags.At('E',1,WINDOW_ENUM_FLAGS); // ExStyle
	sEnum.aFlags[5] = pFlags.At('H',1,WINDOW_ENUM_FLAGS); // HInstance
	sEnum.aFlags[6] = pFlags.At('P',1,WINDOW_ENUM_FLAGS); // ParentHwnd
	sEnum.aFlags[7] = pFlags.At('D',1,WINDOW_ENUM_FLAGS); // UserData
	sEnum.aFlags[8] = pFlags.At('I',1,WINDOW_ENUM_FLAGS); // ID
	sEnum.aFlags[9] = pFlags.At('R',1,WINDOW_ENUM_FLAGS); // ThreadID
	sEnum.aFlags[10] = pFlags.At('O',1,WINDOW_ENUM_FLAGS); // ProcessID
	sEnum.aFlags[11] = pFlags.At('V',1,WINDOW_ENUM_FLAGS); // Visible
	sEnum.aFlags[12] = pFlags.At('N',1,WINDOW_ENUM_FLAGS); // Iconic
	sEnum.aFlags[13] = pFlags.At('M',1,WINDOW_ENUM_FLAGS); // Maximized
	sEnum.aFlags[14] = pFlags.At('U',1,WINDOW_ENUM_FLAGS); // Unicode

	short nMaxDim = 0;
	for (unsigned int nFlag = 0; nFlag < WINDOW_ENUM_FLAGS; nFlag++)
		nMaxDim = max(nMaxDim,sEnum.aFlags[nFlag]);

	if (nMaxDim == 0)
	{
		CStringView pFlagsView = pFlags;
		SaveCustomError("AWindowsEx", "Invalid parameter cFlags '%V'", &pFlagsView);
		throw E_INVALIDPARAMS;
	}

	sEnum.pArray.Dimension(parm(1),1,nMaxDim);
	sEnum.pArray.AutoGrow(32);

	DWORD nLastError = ERROR_SUCCESS;
	switch(parm(3)->ev_long)
	{
		case WINDOW_ENUM_TOPLEVEL:
			if (!EnumWindows(WindowEnumCallbackEx, reinterpret_cast<LPARAM>(&sEnum)))
				nLastError = GetLastError();
			break;

		case WINDOW_ENUM_CHILD:
            if (!EnumChildWindows(parm(4)->Ptr<HWND>(), WindowEnumCallbackEx, reinterpret_cast<LPARAM>(&sEnum)))
				nLastError = GetLastError();
			break;

		case WINDOW_ENUM_THREAD:
            if (!EnumThreadWindows(parm(4)->ev_long, WindowEnumCallbackEx, reinterpret_cast<LPARAM>(&sEnum)))
				nLastError = GetLastError();
			break;

		case WINDOW_ENUM_DESKTOP:
			if (!EnumDesktopWindows(parm(4)->Ptr<HDESK>(), WindowEnumCallbackEx, reinterpret_cast<LPARAM>(&sEnum)))
				nLastError = GetLastError();
	}

	if (nLastError != ERROR_SUCCESS)
	{
		SaveWin32Error("EnumXWindows", nLastError);
		throw E_APIERROR;
	}

	sEnum.pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

BOOL _stdcall WindowEnumCallbackEx(HWND nHwnd, LPARAM nParam)
{
	DWORD nProcessID, nThreadID;
	WindowEnumParamEx *pEnum = reinterpret_cast<WindowEnumParamEx*>(nParam);

	unsigned int nRow = pEnum->pArray.Grow();

	if (pEnum->aFlags[0])
		pEnum->pArray(nRow,pEnum->aFlags[0]) = nHwnd;
	
	if(pEnum->aFlags[1])
		pEnum->pArray(nRow,pEnum->aFlags[1]) = pEnum->pBuffer.Len(GetClassName(nHwnd,pEnum->pBuffer,WINDOW_ENUM_CLASSLEN));
		
	if (pEnum->aFlags[2])
		pEnum->pArray(nRow,pEnum->aFlags[2]) = pEnum->pBuffer.Len(GetWindowText(nHwnd,pEnum->pBuffer,WINDOW_ENUM_TEXTLEN));

	if (pEnum->aFlags[3])
		pEnum->pArray(nRow,pEnum->aFlags[3]) = GetWindowLongPtr(nHwnd,GWL_STYLE);

	if (pEnum->aFlags[4])
		pEnum->pArray(nRow,pEnum->aFlags[4]) = GetWindowLongPtr(nHwnd,GWL_EXSTYLE);

	if (pEnum->aFlags[5])
		pEnum->pArray(nRow,pEnum->aFlags[5]) = GetWindowLongPtr(nHwnd,GWLP_HINSTANCE);

	if (pEnum->aFlags[6])
		pEnum->pArray(nRow,pEnum->aFlags[6]) = GetParent(nHwnd);

	if (pEnum->aFlags[7])
		pEnum->pArray(nRow,pEnum->aFlags[7]) = GetWindowLongPtr(nHwnd,GWLP_USERDATA);

	if (pEnum->aFlags[8])
		pEnum->pArray(nRow,pEnum->aFlags[8]) = GetWindowLongPtr(nHwnd,GWLP_ID);
	
	if (pEnum->aFlags[9] || pEnum->aFlags[10])
	{
		nThreadID = GetWindowThreadProcessId(nHwnd,&nProcessID);
		if (pEnum->aFlags[9])
			pEnum->pArray(nRow,pEnum->aFlags[9]) = nThreadID;
		if (pEnum->aFlags[10])
			pEnum->pArray(nRow,pEnum->aFlags[10]) = nProcessID;
	}

	if (pEnum->aFlags[11])
		pEnum->pArray(nRow,pEnum->aFlags[11]) = IsWindowVisible(nHwnd) > 0;

	if (pEnum->aFlags[12])
		pEnum->pArray(nRow,pEnum->aFlags[12]) = IsIconic(nHwnd) > 0;

	if (pEnum->aFlags[13])
		pEnum->pArray(nRow,pEnum->aFlags[13]) = IsZoomed(nHwnd) > 0; 

	if (pEnum->aFlags[14])
		pEnum->pArray(nRow,pEnum->aFlags[14]) = IsWindowUnicode(nHwnd) > 0;

	return TRUE;
}

void _fastcall AWindowProps(ParamBlkEx& parm)
{
try
{
	EnumParameter sEnum;
	int nApiRet;

	sEnum.pArray.Dimension(parm(1),1,2);
	
	nApiRet = EnumPropsEx(parm(2)->Ptr<HWND>(), (PROPENUMPROCEX)WindowPropEnumCallback, reinterpret_cast<LPARAM>(&sEnum));
	if (nApiRet != -1)
		sEnum.pArray.ReturnRows();
	else
		Return(0);
}
catch (int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

BOOL _stdcall WindowPropEnumCallback(HWND nHwnd, LPCSTR pPropName, HANDLE hData, ULONG_PTR nParam)
{
	EnumParameter *pEnum = reinterpret_cast<EnumParameter*>(nParam);
	if (IsBadReadPtr(pPropName,1))
		return TRUE;
	unsigned int nRow = pEnum->pArray.Grow();
	pEnum->pArray(nRow,1) = pEnum->pName = (LPSTR)pPropName;
	pEnum->pArray(nRow,2) = hData;
	return TRUE;		
}

void _fastcall AMonitors(ParamBlkEx& parm)
{
	try
	{
		MonitorEnumParam pEnum;
		pEnum.pArray.Dimension(parm(1), 1, 10);
		BOOL ret = EnumDisplayMonitors(0, 0, MonitorEnumCallback, (LPARAM)&pEnum);
		if (ret == FALSE)
		{
			SaveWin32Error("EnumDisplayMonitors", GetLastError());
			throw E_APIERROR;
		}
		else if (pEnum.bError)
		{
			throw E_APIERROR;
		}
		Return(pEnum.nCount);
	}
	catch (int nErrorNo)
	{
		RaiseError(nErrorNo);
	}
}

BOOL _stdcall MonitorEnumCallback(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	MonitorEnumParam *pEnum = reinterpret_cast<MonitorEnumParam*>(dwData);
	MONITORINFO lpmi;
	lpmi.cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfo(hMonitor, &lpmi) == FALSE)
	{
		SaveWin32Error("GetMonitorInfo", GetLastError());
		pEnum->bError = true;
		return FALSE;
	}
	pEnum->nCount++;
	unsigned int nRow = pEnum->pArray.Grow();
	pEnum->pArray(nRow, 1) = hMonitor;
	pEnum->pArray(nRow, 2) = lprcMonitor->left;
	pEnum->pArray(nRow, 3) = lprcMonitor->top;
	pEnum->pArray(nRow, 4) = lprcMonitor->right;
	pEnum->pArray(nRow, 5) = lprcMonitor->bottom;
	pEnum->pArray(nRow, 6) = lpmi.rcWork.left;
	pEnum->pArray(nRow, 7) = lpmi.rcWork.bottom;
	pEnum->pArray(nRow, 8) = lpmi.rcWork.right;
	pEnum->pArray(nRow, 9) = lpmi.rcWork.bottom;
	pEnum->pArray(nRow, 10) = (lpmi.dwFlags & MONITORINFOF_PRIMARY) > 0;
	return TRUE;
}

void _fastcall AProcesses(ParamBlkEx& parm)
{
try
{
 	ApiHandle hProcSnap;
 	FoxString pExeName(MAX_PATH);
	FoxArray pArray(parm(1),1,5);
	PROCESSENTRY32 pProcs;
	DWORD nLastError;
	
	hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	if(!hProcSnap)
	{
		SaveWin32Error("CreateToolhelp32Snapshot", GetLastError());
		throw E_APIERROR;
	}
	
	pProcs.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hProcSnap,&pProcs))
	{
		nLastError = GetLastError();
		if (nLastError == ERROR_NO_MORE_FILES)
		{
			Return(0);
			return;
		}
		else
		{
			SaveWin32Error("Process32First", nLastError);
			throw E_APIERROR;
		}
	}

	pArray.AutoGrow(64);
	unsigned int nRow;
	do
	{
		nRow = pArray.Grow();		
		pArray(nRow,1) = pExeName = pProcs.szExeFile;
		pArray(nRow,2) = pProcs.th32ProcessID;
		pArray(nRow,3) = pProcs.th32ParentProcessID;
		pArray(nRow,4) = pProcs.cntThreads;
		pArray(nRow,5) = pProcs.pcPriClassBase;
	} while(Process32Next(hProcSnap, &pProcs));

	nLastError = GetLastError();
	if (nLastError != ERROR_NO_MORE_FILES)
	{
		SaveWin32Error("Process32Next", nLastError);
		throw E_APIERROR;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AProcessThreads(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),1,3);
	ApiHandle hThreadSnap;
	THREADENTRY32 pThreads; 
   	DWORD nProcId = parm(2)->ev_long;
	DWORD nLastError;

	// Take a snapshot of all running threads  
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0); 
	if(!hThreadSnap)
	{
        SaveWin32Error("CreateToolhelp32SnapShot", GetLastError());
		throw E_APIERROR;
	}
     
	pThreads.dwSize = sizeof(THREADENTRY32); 
	if(!Thread32First(hThreadSnap,&pThreads)) 
	{
		nLastError = GetLastError();
		if (nLastError == ERROR_NO_MORE_FILES)
		{
			Return(0);
			return;
		}
		else
		{
			SaveWin32Error("Thread32First", nLastError);
			throw E_APIERROR;
		}
	}

	pArray.AutoGrow(4);
	unsigned int nRow;
	do 
	{ 
		if(nProcId == 0 || nProcId == pThreads.th32OwnerProcessID)
		{
			nRow = pArray.Grow();
			pArray(nRow,1) = pThreads.th32ThreadID;
			pArray(nRow,2) = pThreads.th32OwnerProcessID;
			pArray(nRow,3) = pThreads.tpBasePri;
		}
	} while(Thread32Next(hThreadSnap,&pThreads)); 

	nLastError = GetLastError();
	if (nLastError != ERROR_NO_MORE_FILES)
	{
		SaveWin32Error("Thread32Next", nLastError);
		throw E_APIERROR;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AProcessModules(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),1,5);
	FoxString pBuffer(MAX_PATH);
	ApiHandle hModuleSnap; 
	MODULEENTRY32 pModules;
  	DWORD nProcId = parm(2)->ev_long;
	DWORD nLastError;
	
	pModules.dwSize = sizeof(MODULEENTRY32);
	while (true)
	{
		// Take a snapshot of all running threads
		hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,nProcId); 
		if(!hModuleSnap)
		{
			nLastError = GetLastError();
			if (nLastError == ERROR_BAD_LENGTH)
				continue;
			SaveWin32Error("CreateToolhelp32SnapShot", nLastError);
			throw E_APIERROR;
		}
		else
			break;
	}

	if(!Module32First(hModuleSnap,&pModules))
	{
		nLastError = GetLastError();
		if (nLastError == ERROR_NO_MORE_FILES)
		{
			Return(0);
			return;
		}
		else
		{
			SaveWin32Error("Module32First", nLastError);
			throw E_APIERROR;
		}
	}

	pArray.AutoGrow(32);
	unsigned int nRow;
	do 
	{ 
		nRow = pArray.Grow();
		pArray(nRow,1) = pBuffer = pModules.szModule;
		pArray(nRow,2) = pBuffer = pModules.szExePath;
		pArray(nRow,3) = pModules.hModule;
		pArray(nRow,4) = pModules.modBaseSize;
		pArray(nRow,5) = pModules.modBaseAddr;
	} while(Module32Next(hModuleSnap, &pModules)); 

	nLastError = GetLastError();
	if (nLastError != ERROR_NO_MORE_FILES)
	{
		SaveWin32Error("Module32Next", nLastError);
		throw E_APIERROR;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AProcessHeaps(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),1,2);
	ApiHandle hHeapSnap;
  	DWORD nProcId = parm(2)->ev_long;
	DWORD nLastError;
	HEAPLIST32 pHeaps; 

	pHeaps.dwSize = sizeof(HEAPLIST32);

	// Take a snapshot of all running threads
	hHeapSnap = CreateToolhelp32Snapshot(TH32CS_SNAPHEAPLIST,nProcId); 
	if(!hHeapSnap)
	{
		SaveWin32Error("CreateToolhelp32SnapShot", GetLastError());
		throw E_APIERROR;
	}

	if(!Heap32ListFirst(hHeapSnap,&pHeaps))
	{
		nLastError = GetLastError();
		if (nLastError == ERROR_NO_MORE_FILES)
		{
			Return(0);
			return;
		}
		else
		{
			SaveWin32Error("HeapList32First", nLastError);
			throw E_APIERROR;
		}
	}

	pArray.AutoGrow(4);
	unsigned int nRow;
	do 
	{ 
		nRow = pArray.Grow();
		pArray(nRow,1) = pHeaps.th32HeapID;
		pArray(nRow,2) = pHeaps.dwFlags;
	} while(Heap32ListNext(hHeapSnap,&pHeaps)); 

	nLastError = GetLastError();
	if (nLastError != ERROR_NO_MORE_FILES)
	{
		SaveWin32Error("Heap32ListNext", nLastError);
		throw E_APIERROR;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AHeapBlocks(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),1,4);
	HEAPENTRY32 pHeapEntry;
	DWORD nLastError;

	pHeapEntry.dwSize = sizeof(HEAPENTRY32);

	if (!Heap32First(&pHeapEntry, parm(2)->ev_long, parm(3)->ev_long))
	{
		nLastError = GetLastError();
		if (nLastError == ERROR_NO_MORE_FILES)
		{
			Return(0);
			return;
		}
		else
		{
            SaveWin32Error("Heap32First", nLastError);
			throw E_APIERROR;
		}
	}

	pArray.AutoGrow(32);
	unsigned int nRow;
	do
	{
		nRow = pArray.Grow();
		pArray(nRow,1) = pHeapEntry.hHandle;
		pArray(nRow,2) = pHeapEntry.dwAddress;
		pArray(nRow,3) = pHeapEntry.dwBlockSize;
		pArray(nRow,4) = pHeapEntry.dwFlags;
	} while(Heap32Next(&pHeapEntry));

	nLastError = GetLastError();
	if (nLastError != ERROR_NO_MORE_FILES)
	{
		SaveWin32Error("Heap32Next", nLastError);
		throw E_APIERROR;
	}
	
	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ReadProcessMemoryEx(ParamBlkEx& parm)
{
try
{
	FoxString vRetVal(parm(3)->ev_long);
	SIZE_T dwRead;

	if (Toolhelp32ReadProcessMemory(parm(1)->ev_long, parm(2)->Ptr<LPCVOID>(), vRetVal.Ptr<LPVOID>(), static_cast<SIZE_T>(parm(3)->ev_long), &dwRead))
		vRetVal.Len(dwRead);
	else
		vRetVal.Len(0);
	
	vRetVal.Return();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
} 

void _fastcall AResourceTypes(ParamBlkEx& parm)
{
try
{
	RESOURCEENUMPARAM sEnum;

	sEnum.pArray.Dimension(parm(1),1);
	sEnum.pBuffer.Size(RESOURCE_ENUM_TYPELEN);

	if (!EnumResourceTypes(parm(2)->Ptr<HMODULE>(), (ENUMRESTYPEPROC)ResourceTypesEnumCallback, reinterpret_cast<LONG_PTR>(&sEnum)))
	{
		SaveWin32Error("EnumResourceTypes", GetLastError());
		throw E_APIERROR;
	}

	sEnum.pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

BOOL _stdcall ResourceTypesEnumCallback(HANDLE hModule, LPSTR lpszType, LONG_PTR nParam)
{
	LPRESOURCEENUMPARAM pEnum = reinterpret_cast<LPRESOURCEENUMPARAM>(nParam);
	pEnum->pArray.Grow();
	if ((ULONG_PTR)lpszType & 0xFFFF0000)
	{
		pEnum->pArray() = pEnum->pBuffer = lpszType;
	}
	else
	{
#pragma warning(disable:4302)
		pEnum->pArray() = reinterpret_cast<unsigned short>(lpszType);
#pragma warning(default:4302)
	}
		
	return TRUE;		
}

void _fastcall AResourceNames(ParamBlkEx& parm)
{
try
{
	RESOURCEENUMPARAM sEnum;
	FoxString pType(parm,3);
	char *pResourceType;

	sEnum.pArray.Dimension(parm(1),1);
	sEnum.pBuffer.Size(RESOURCE_ENUM_NAMELEN);

	if (parm(3)->Vartype() == 'C')
		pResourceType = pType;
	else if (parm(3)->Vartype() == 'I')
        pResourceType = MAKEINTRESOURCE(parm(3)->ev_long); 
	else if (parm(3)->Vartype() == 'N')
		pResourceType = MAKEINTRESOURCE(static_cast<int>(parm(3)->ev_real));
	else
	{
		SaveCustomError("AResourceNames", "Invalid type '%s' for parameter 3.", parm(3)->Vartype());
		throw E_INVALIDPARAMS;
	}

	if (!EnumResourceNames(parm(2)->Ptr<HMODULE>(), pResourceType, (ENUMRESNAMEPROC)ResourceNamesEnumCallback, reinterpret_cast<LONG_PTR>(&sEnum)))
	{
		SaveWin32Error("EnumResourceNames", GetLastError());
		throw E_APIERROR;
	}

	sEnum.pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

BOOL _stdcall ResourceNamesEnumCallback(HANDLE hModule, LPCSTR lpszType, LPSTR lpszName, LONG_PTR nParam)
{
	LPRESOURCEENUMPARAM pEnum = reinterpret_cast<LPRESOURCEENUMPARAM>(nParam);
	pEnum->pArray.Grow();
	if ((ULONG_PTR)lpszName & 0xFFFF0000)
		pEnum->pArray() = pEnum->pBuffer = lpszName;
	else
		pEnum->pArray() = reinterpret_cast<UINT_PTR>(lpszName);
	return TRUE;
}

void _fastcall AResourceLanguages(ParamBlkEx& parm)
{
try
{
	RESOURCEENUMPARAM sEnum;
	FoxString pType(parm,3);
	FoxString pName(parm,4);
	char *pResourceType;
	char *pResourceName;

	sEnum.pArray.Dimension(parm(1),1);
	
	if (parm(3)->Vartype() == 'C')
		pResourceType = pType;
	else if (parm(3)->Vartype() == 'I' || parm(3)->Vartype() == 'N')
		pResourceType = parm(3)->DynamicPtr<char*>();
	else
	{
		SaveCustomError("AResourceLanguages", "Invalid type '%s' for parameter 3.", parm(3)->Vartype());
		throw E_INVALIDPARAMS;
	}

	if (parm(4)->Vartype() == 'C')
        pResourceName = pName;
	else if (parm(4)->Vartype() == 'I' || parm(4)->Vartype() == 'N')
		pResourceName = parm(1)->DynamicPtr<char*>();
	else
	{
		SaveCustomError("AResourceLanguages", "Invalid type '%s' for parameter 4.", parm(4)->Vartype());
		throw E_INVALIDPARAMS;
	}

	if (!EnumResourceLanguages(parm(2)->Ptr<HMODULE>(), pResourceType, pResourceName, (ENUMRESLANGPROC)ResourceLangEnumCallback, reinterpret_cast<LONG_PTR>(&sEnum)))
	{
		SaveWin32Error("EnumResourceLanguages", GetLastError());
		throw E_APIERROR;
	}

	sEnum.pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

BOOL _stdcall ResourceLangEnumCallback(HANDLE hModule, LPCSTR lpszType, LPCSTR lpszName, WORD wIDLanguage, LONG_PTR nParam)
{
	LPRESOURCEENUMPARAM pEnum = reinterpret_cast<LPRESOURCEENUMPARAM>(nParam);
	pEnum->pArray.Grow();
	pEnum->pArray() = wIDLanguage;
	return TRUE;
}

void _fastcall AResolutions(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),1,4);
	FoxString pDevice(parm,2);
	DWORD nLastError = ERROR_SUCCESS;
	DEVMODE sDevMode;

	sDevMode.dmDriverExtra = 0;
	sDevMode.dmSize = sizeof(DEVMODE);

	DWORD dwRes = 0;
	unsigned int nRow;
	pArray.AutoGrow(32);
	while (EnumDisplaySettings(pDevice, dwRes++, &sDevMode))
	{
		nRow = pArray.Grow();
		pArray(nRow,1) = sDevMode.dmPelsWidth;
		pArray(nRow,2) = sDevMode.dmPelsHeight;
		pArray(nRow,3) = sDevMode.dmBitsPerPel;
		pArray(nRow,4) = sDevMode.dmDisplayFrequency;
	}

	if (COs::GreaterOrEqual(Windows2000))
		nLastError = GetLastError();

	if (nLastError != ERROR_SUCCESS)
	{
		SaveWin32Error("EnumDisplaySettings", nLastError);
		throw E_APIERROR;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ADisplayDevices(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),1,5);
	FoxString pDevice(parm,2);
	FoxString pBuffer(DISPLAYDEVICE_ENUM_LEN);
	DISPLAY_DEVICE sDevice;	

	sDevice.cb = sizeof(DISPLAY_DEVICE);
	DWORD dwDev = 0;
	unsigned int nRow;
	while (EnumDisplayDevices(pDevice, dwDev++, &sDevice, 0))
	{
		nRow = pArray.Grow();
		pArray(nRow,1) = pBuffer = sDevice.DeviceString;
		pArray(nRow,2) = pBuffer = sDevice.DeviceName;
		pArray(nRow,3) = pBuffer = sDevice.DeviceID;
		pArray(nRow,4) = pBuffer = sDevice.DeviceKey;
		pArray(nRow,5) = sDevice.StateFlags;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}