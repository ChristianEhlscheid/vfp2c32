#include <windows.h>
#include <stdio.h>

#include "pro_ext.h"
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2cenum.h"
#include "vfp2ccppapi.h"
#include "vfp2chelpers.h"
#include "vfpmacros.h"

void _fastcall AWindowStations(ParamBlk *parm)
{
try
{
	EnumParameter sEnum;
	// initialize array
	sEnum.pArray.Dimension(p1,1);

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
	pEnum->pArray = pEnum->pName = lpszWinSta;
	return TRUE;
} 

void _fastcall ADesktops(ParamBlk *parm)
{
try
{
	EnumParameter sEnum;
	HWINSTA hWinSta;

	sEnum.pArray.Dimension(p1,1);
	hWinSta = PCount() == 1 ? GetProcessWindowStation() : reinterpret_cast<HWINSTA>(p2.ev_long);
	
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
	pEnum->pArray = pEnum->pName = (LPSTR)lpszDesktop;
	return TRUE;  
}

void _fastcall AWindows(ParamBlk *parm)
{
try
{
	WindowEnumParam sEnum;
	FoxString pArrayOrCallback(p1);
	DWORD nEnumFlag = p2.ev_long;
	DWORD nLastError = ERROR_SUCCESS;

	// are parameters valid?
	if (PCount() == 2 && !(nEnumFlag & WINDOW_ENUM_TOPLEVEL))
		throw E_INVALIDPARAMS;
	else if (!(nEnumFlag & (WINDOW_ENUM_TOPLEVEL|WINDOW_ENUM_CHILD|WINDOW_ENUM_THREAD|WINDOW_ENUM_DESKTOP)))
		throw E_INVALIDPARAMS;

	if (nEnumFlag & WINDOW_ENUM_CALLBACK && p1.ev_length > VFP2C_MAX_CALLBACKFUNCTION)
		throw E_INVALIDPARAMS;
	
	if (nEnumFlag & WINDOW_ENUM_CALLBACK)
	{
		sEnum.pCallback = pArrayOrCallback;
		sEnum.pCallback += "(%U)";
		sEnum.pBuffer.Size(VFP2C_MAX_CALLBACKBUFFER);
	}
	else
	{
		sEnum.pArray.Dimension((char*)pArrayOrCallback, 1);
		sEnum.pArray.AutoGrow(16);
	}

	if (nEnumFlag & WINDOW_ENUM_TOPLEVEL)
	{
		if (!EnumWindows(nEnumFlag & WINDOW_ENUM_CALLBACK ? WindowEnumCallbackCall : WindowEnumCallback, reinterpret_cast<LPARAM>(&sEnum)))
			nLastError = GetLastError();
	}
	else if (nEnumFlag & WINDOW_ENUM_CHILD)
	{
		if (!EnumChildWindows(reinterpret_cast<HWND>(p3.ev_long), nEnumFlag & WINDOW_ENUM_CALLBACK ? WindowEnumCallbackCall : WindowEnumCallback, reinterpret_cast<LPARAM>(&sEnum)))
			nLastError = GetLastError();
	}
	else if (nEnumFlag & WINDOW_ENUM_THREAD)
	{
		if (!EnumThreadWindows(p3.ev_long, nEnumFlag & WINDOW_ENUM_CALLBACK ? WindowEnumCallbackCall : WindowEnumCallback, reinterpret_cast<LPARAM>(&sEnum)))
			nLastError = GetLastError();
	}
	else if (nEnumFlag & WINDOW_ENUM_DESKTOP)
	{
		if (!EnumDesktopWindows(reinterpret_cast<HDESK>(p3.ev_long), nEnumFlag & WINDOW_ENUM_CALLBACK ? WindowEnumCallbackCall : WindowEnumCallback, reinterpret_cast<LPARAM>(&sEnum)))
			nLastError = GetLastError();
	}

	if (nLastError != ERROR_SUCCESS)
	{
		SaveWin32Error("EnumWindowsX", nLastError);
		throw E_APIERROR;
	}

	if (nEnumFlag & WINDOW_ENUM_CALLBACK)
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
	pEnum->pArray = nHwnd;
	return TRUE;		
}

BOOL _stdcall WindowEnumCallbackCall(HWND nHwnd, LPARAM nParam) throw(int)
{
	WindowEnumParam *pEnum = reinterpret_cast<WindowEnumParam*>(nParam);
	FoxValue vRetVal;
	pEnum->pBuffer.Format(pEnum->pCallback, nHwnd);
	vRetVal.Evaluate(pEnum->pBuffer);
	if (vRetVal.Vartype() == 'L')
        return vRetVal->ev_length;
	else
		return 0;
}

void _fastcall AWindowsEx(ParamBlk *parm)
{
try
{
	FoxString pFlags(p2);
	WindowEnumParamEx sEnum;
	
	// are parameters valid?
	if (PCount() == 3 && p3.ev_long != WINDOW_ENUM_TOPLEVEL)
		throw E_INVALIDPARAMS;
	else if (p3.ev_long < WINDOW_ENUM_TOPLEVEL || p3.ev_long > WINDOW_ENUM_DESKTOP)
		throw E_INVALIDPARAMS;

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
		throw E_INVALIDPARAMS;

	sEnum.pArray.Dimension(p1,1,nMaxDim);
	sEnum.pArray.AutoGrow(16);

	DWORD nLastError = ERROR_SUCCESS;
	switch(p3.ev_long)
	{
		case WINDOW_ENUM_TOPLEVEL:
			if (!EnumWindows(WindowEnumCallbackEx, reinterpret_cast<LPARAM>(&sEnum)))
				nLastError = GetLastError();
			break;

		case WINDOW_ENUM_CHILD:
            if (!EnumChildWindows(reinterpret_cast<HWND>(p4.ev_long), WindowEnumCallbackEx, reinterpret_cast<LPARAM>(&sEnum)))
				nLastError = GetLastError();
			break;

		case WINDOW_ENUM_THREAD:
            if (!EnumThreadWindows(p4.ev_long, WindowEnumCallbackEx, reinterpret_cast<LPARAM>(&sEnum)))
				nLastError = GetLastError();
			break;

		case WINDOW_ENUM_DESKTOP:
			if (!EnumDesktopWindows(reinterpret_cast<HDESK>(p4.ev_long), WindowEnumCallbackEx, reinterpret_cast<LPARAM>(&sEnum)))
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
		pEnum->pArray(nRow,pEnum->aFlags[3]) = GetWindowLong(nHwnd,GWL_STYLE);

	if (pEnum->aFlags[4])
		pEnum->pArray(nRow,pEnum->aFlags[4]) = GetWindowLong(nHwnd,GWL_EXSTYLE);

	if (pEnum->aFlags[5])
		pEnum->pArray(nRow,pEnum->aFlags[5]) = GetWindowLong(nHwnd,GWL_HINSTANCE);

	if (pEnum->aFlags[6])
		pEnum->pArray(nRow,pEnum->aFlags[6]) = GetParent(nHwnd);

	if (pEnum->aFlags[7])
		pEnum->pArray(nRow,pEnum->aFlags[7]) = GetWindowLong(nHwnd,GWL_USERDATA);

	if (pEnum->aFlags[8])
		pEnum->pArray(nRow,pEnum->aFlags[8]) = GetWindowLong(nHwnd,GWL_ID);
	
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

void _fastcall AWindowProps(ParamBlk *parm)
{
try
{
	EnumParameter sEnum;
	int nApiRet;

	sEnum.pArray.Dimension(p1,1,2);
	
	nApiRet = EnumPropsEx(reinterpret_cast<HWND>(p2.ev_long), (PROPENUMPROCEX)WindowPropEnumCallback, reinterpret_cast<LPARAM>(&sEnum));
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

BOOL _stdcall WindowPropEnumCallback(HWND nHwnd, LPCSTR pPropName, HANDLE hData, DWORD nParam)
{
	EnumParameter *pEnum = reinterpret_cast<EnumParameter*>(nParam);
	if (IsBadReadPtr(pPropName,1))
		return TRUE;
	unsigned int nRow = pEnum->pArray.Grow();
	pEnum->pArray(nRow,1) = pEnum->pName = (LPSTR)pPropName;
	pEnum->pArray(nRow,2) = hData;
	return TRUE;		
}

void _fastcall AProcesses(ParamBlk *parm)
{
try
{
 	ApiHandle hProcSnap;
 	FoxString pExeName(MAX_PATH+1);
	FoxArray pArray(p1,1,5);
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

void _fastcall AProcessThreads(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1,1,3);
	ApiHandle hThreadSnap;
	THREADENTRY32 pThreads; 
   	DWORD nProcId = p2.ev_long;
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

void _fastcall AProcessModules(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1,1,5);
	FoxString pBuffer(MAX_PATH+1);
	ApiHandle hModuleSnap; 
	MODULEENTRY32 pModules;
  	DWORD nProcId = p2.ev_long;
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

void _fastcall AProcessHeaps(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1,1,2);
	ApiHandle hHeapSnap;
  	DWORD nProcId = p2.ev_long;
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

void _fastcall AHeapBlocks(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1,1,4);
	HEAPENTRY32 pHeapEntry;
	DWORD nLastError;

	pHeapEntry.dwSize = sizeof(HEAPENTRY32);

	if (!Heap32First(&pHeapEntry, p2.ev_long, p3.ev_long))
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

void _fastcall ReadProcessMemoryEx(ParamBlk *parm)
{
try
{
	FoxString vRetVal(p3.ev_long);
	SIZE_T dwRead;

	if (Toolhelp32ReadProcessMemory(p1.ev_long, reinterpret_cast<LPCVOID>(p2.ev_long), vRetVal, static_cast<SIZE_T>(p3.ev_long), &dwRead))
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

void _fastcall AResourceTypes(ParamBlk *parm)
{
try
{
	RESOURCEENUMPARAM sEnum;

	sEnum.pArray.Dimension(p1,1);
	sEnum.pBuffer.Size(RESOURCE_ENUM_TYPELEN);

	if (!EnumResourceTypes(reinterpret_cast<HMODULE>(p2.ev_long),(ENUMRESTYPEPROC)ResourceTypesEnumCallback, reinterpret_cast<LONG>(&sEnum)))
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

BOOL _stdcall ResourceTypesEnumCallback(HANDLE hModule, LPSTR lpszType, LONG nParam)  
{
	LPRESOURCEENUMPARAM pEnum = reinterpret_cast<LPRESOURCEENUMPARAM>(nParam);
	pEnum->pArray.Grow();
	if ((ULONG)lpszType & 0xFFFF0000) 
		pEnum->pArray = pEnum->pBuffer = lpszType;
	else
		pEnum->pArray = reinterpret_cast<unsigned short>(lpszType);
	return TRUE;		
}

void _fastcall AResourceNames(ParamBlk *parm)
{
try
{
	RESOURCEENUMPARAM sEnum;
	FoxString pType(parm,3);
	char *pResourceType;

	sEnum.pArray.Dimension(p1,1);
	sEnum.pBuffer.Size(RESOURCE_ENUM_NAMELEN);

	if (Vartype(p3) == 'C')
		pResourceType = pType;
	else if (Vartype(p3) == 'I')
        pResourceType = MAKEINTRESOURCE(p3.ev_long); 
	else if (Vartype(p3) == 'N')
		pResourceType = MAKEINTRESOURCE(static_cast<int>(p3.ev_real));
	else
		throw E_INVALIDPARAMS;

	if (!EnumResourceNames(reinterpret_cast<HMODULE>(p2.ev_long), pResourceType, (ENUMRESNAMEPROC)ResourceNamesEnumCallback, reinterpret_cast<LONG_PTR>(&sEnum)))
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
	if ((ULONG)lpszName & 0xFFFF0000)
		pEnum->pArray = pEnum->pBuffer = lpszName;
	else
		pEnum->pArray = reinterpret_cast<int>(lpszName);
	return TRUE;
}

void _fastcall AResourceLanguages(ParamBlk *parm)
{
try
{
	RESOURCEENUMPARAM sEnum;
	FoxString pType(parm,3);
	FoxString pName(parm,4);
	char *pResourceType;
	char *pResourceName;

	sEnum.pArray.Dimension(p1,1);
	
	if (Vartype(p3) == 'C')
		pResourceType = pType;
	else if (Vartype(p3) == 'I')
		pResourceType = reinterpret_cast<char*>(p3.ev_long);
	else if (Vartype(p3) == 'N')
		pResourceType = reinterpret_cast<char*>(static_cast<int>(p3.ev_real));
	else
		throw E_INVALIDPARAMS;

	if (Vartype(p4) == 'C')
        pResourceName = pName;
	else if (Vartype(p4) == 'I')
		pResourceName = reinterpret_cast<char*>(p4.ev_long);
	else if (Vartype(p4) == 'N')
		pResourceName = reinterpret_cast<char*>(static_cast<int>(p4.ev_real));
	else
		throw E_INVALIDPARAMS;

	if (!EnumResourceLanguages(reinterpret_cast<HMODULE>(p2.ev_long), pResourceType, pResourceName, (ENUMRESLANGPROC)ResourceLangEnumCallback, reinterpret_cast<LONG>(&sEnum)))
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

BOOL _stdcall ResourceLangEnumCallback(HANDLE hModule, LPCSTR lpszType, LPCSTR lpszName, WORD wIDLanguage, LONG nParam)
{
	LPRESOURCEENUMPARAM pEnum = reinterpret_cast<LPRESOURCEENUMPARAM>(nParam);
	pEnum->pArray.Grow();
	pEnum->pArray = wIDLanguage;
	return TRUE;
}

void _fastcall AResolutions(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1,1,4);
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

void _fastcall ADisplayDevices(ParamBlk *parm)
{
try
{
	FoxArray pArray(p1,1,5);
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