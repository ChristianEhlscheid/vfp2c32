#include <windows.h>
#include <atlbase.h>

#if !defined(_WIN64)
#include "pro_ext.h"
#else
#include "pro_ext64.h"
#endif
#include "vfp2c32.h"
#include "vfp2cutil.h"
#include "vfp2cservices.h"
#include "vfp2ccppapi.h"
#include "vfp2chelpers.h"

ServiceManager::ServiceManager(const char *pMachine, const char *pDatabase, DWORD dwAccess)
{
	Open(pMachine,pDatabase,dwAccess);
}

ServiceManager::~ServiceManager()
{
	if (m_Handle)
		CloseServiceHandle(m_Handle);
}

void ServiceManager::Open(const char *pMachine, const char *pDatabase, DWORD dwAccess)
{
	m_Handle = OpenSCManager(pMachine,pDatabase,dwAccess);
	if (m_Handle == NULL)
	{
		SaveWin32Error("OpenSCManager", GetLastError());
		throw E_APIERROR;
	}
}

Service::Service(SC_HANDLE hSCM, const char* pServiceName, DWORD dwAccess)
{
	Open(hSCM, pServiceName, dwAccess);
}

Service::~Service()
{
	if (m_Handle && m_Owner)
		CloseServiceHandle(m_Handle);
}

void Service::Open(SC_HANDLE hSCM, const char* pServiceName, DWORD dwAccess)
{
	if (m_Handle && m_Owner)
		CloseServiceHandle(m_Handle);

	m_Handle = OpenService(hSCM,pServiceName,dwAccess);
	if (m_Handle == NULL)
	{
		SaveWin32Error("OpenService", GetLastError());
		throw E_APIERROR;
	}
	m_Owner = true;
}

int Service::Start(DWORD nNumberOfArgs, LPCSTR *pArgs, int nTimeout)
{
	// Check if the service is already running
	QueryStatus(&m_Status);

	if (m_Status.dwCurrentState == SERVICE_RUNNING)
		return 1;

	if (m_Status.dwCurrentState != SERVICE_START_PENDING)
	{
		if (!StartService(m_Handle, nNumberOfArgs, pArgs))
		{
			SaveWin32Error("StartService", GetLastError());
			throw E_APIERROR;
		}

		QueryStatus(&m_Status);
	}

	// Check the status until the service is no longer start pending. 
    // Save the tick count and initial checkpoint.
	DWORD dwOldCheckPoint, dwStartTime, dwTimeout;
	bool bCustomTimeout = false;

	dwOldCheckPoint = m_Status.dwCheckPoint;
	dwStartTime = GetTickCount();

	if (nTimeout == 0)
		return 0;
	else if (nTimeout == SERVICE_DEFAULT_TIMEOUT)
		dwTimeout = m_Status.dwWaitHint;
	else
	{
		dwTimeout = nTimeout * 1000;
		bCustomTimeout = true;
	}

	while (m_Status.dwCurrentState != SERVICE_RUNNING) 
	{ 
		// Check the status again. 
		QueryStatus(&m_Status);

		if(m_Status.dwCheckPoint > dwOldCheckPoint)
		{
			// The service is making progress.
			if (!bCustomTimeout)
			{
				dwStartTime = GetTickCount();
				dwOldCheckPoint = m_Status.dwCheckPoint;
				dwTimeout = m_Status.dwWaitHint;
			}
		}
		else if(GetTickCount() - dwStartTime > dwTimeout)
			break;

		Sleep(333);
	} 

	return m_Status.dwCurrentState == SERVICE_RUNNING ? 1 : 0;
}

int Service::Stop(bool bStopDependencies, int nTimeout, SC_HANDLE hSCM)
{
	// Make sure the service is not already stopped
	QueryStatus(&m_Status);

	if (m_Status.dwCurrentState == SERVICE_STOPPED)
		return 1;

	if (m_Status.dwCurrentState == SERVICE_STOP_PENDING)
		return WaitForServiceStatus(SERVICE_STOPPED,nTimeout);

	if (bStopDependencies)
		StopDependantServices(hSCM);

	// Send a stop code to the main service
	if (!ControlService(m_Handle,SERVICE_CONTROL_STOP,&m_Status))
	{
		SaveWin32Error("ControlService", GetLastError());
		throw E_APIERROR;
	}

	return WaitForServiceStatus(SERVICE_STOPPED,nTimeout);
}

int Service::Pause(int nTimeout)
{
	// make sure the service is not already paused
	QueryStatus(&m_Status);

	if (m_Status.dwCurrentState == SERVICE_PAUSED)
		return 1;

	if (m_Status.dwCurrentState == SERVICE_PAUSE_PENDING)
		return WaitForServiceStatus(SERVICE_PAUSED, nTimeout);

	// Send pause command to the service
	if (!ControlService(m_Handle,SERVICE_CONTROL_PAUSE,&m_Status))
	{
		SaveWin32Error("ControlService", GetLastError());
		throw E_APIERROR;
	}

	return WaitForServiceStatus(SERVICE_PAUSED, nTimeout);
}

int Service::Continue(int nTimeout)
{
	// make sure the service is not already paused
	QueryStatus(&m_Status);

	if (m_Status.dwCurrentState == SERVICE_RUNNING)
		return 1;

	// If continue is pending, just wait for it
	if (m_Status.dwCurrentState == SERVICE_CONTINUE_PENDING)
		return WaitForServiceStatus(SERVICE_RUNNING, nTimeout);

	// Send pause command to the service
	if (!ControlService(m_Handle, SERVICE_CONTROL_CONTINUE, &m_Status))
	{
		SaveWin32Error("ControlService", GetLastError());
		throw E_APIERROR;
	}

	return WaitForServiceStatus(SERVICE_RUNNING,nTimeout);
}

int Service::Control(DWORD nControlCode)
{
	// Send a stop code to the main service
	if (!ControlService(m_Handle, nControlCode, &m_Status))
	{
		SaveWin32Error("ControlService", GetLastError());
		throw E_APIERROR;
	}
	return 1;
}

void Service::QueryStatus(LPSERVICE_STATUS pStatus)
{
	if (!QueryServiceStatus(m_Handle, pStatus))
	{
		SaveWin32Error("QueryServiceStatus", GetLastError());
		throw E_APIERROR;
	}
}

void Service::QueryConfig(CBuffer &pBuffer)
{
	DWORD nBytesNeeded = 0;
	pBuffer.Size(8192);
	if(!QueryServiceConfig(m_Handle, reinterpret_cast<LPQUERY_SERVICE_CONFIG>(pBuffer.Address()), pBuffer.Size(), &nBytesNeeded))
	{
		SaveWin32Error("QueryServiceConfig", GetLastError());
		throw E_APIERROR;
	}
}

int Service::WaitForServiceStatus(DWORD dwState, int nTimeout)
{
	DWORD nStartTime = GetTickCount();
	DWORD dwTimeout;

	if (nTimeout == SERVICE_DEFAULT_TIMEOUT)
		dwTimeout = m_Status.dwWaitHint;
	else if (nTimeout == SERVICE_INFINITE_TIMEOUT)
		dwTimeout = INFINITE;
	else
		dwTimeout = nTimeout * 1000;

	QueryStatus(&m_Status);
	if (dwTimeout == 0)
		return m_Status.dwCurrentState == dwState ? 1 : 0;

	while (m_Status.dwCurrentState != dwState) 
	{
		QueryStatus(&m_Status);

		if (m_Status.dwCurrentState == dwState)
			break;
		if (dwTimeout != INFINITE && GetTickCount() - nStartTime > dwTimeout)
			break;

		Sleep(250);
	}
	return m_Status.dwCurrentState == dwState ? 1 : 0;
}

void Service::StopDependantServices(SC_HANDLE hSCM)
{
	Service hDepService;
	CBuffer pBuffer;
	DWORD nApiRet, nBytesNeeded, nCount;
    LPENUM_SERVICE_STATUS pDepStatus;

	// this will always fail, called to determine the required buffersize
	EnumDependentServices(m_Handle, SERVICE_ACTIVE, 0, 0, &nBytesNeeded, &nCount);
	nApiRet = GetLastError();
	if (nApiRet != ERROR_MORE_DATA)
	{
		SaveWin32Error("EnumDependentServices", nApiRet);
		throw E_APIERROR;
	}
	
	pBuffer.Size(nBytesNeeded);
	pDepStatus = reinterpret_cast<LPENUM_SERVICE_STATUS>(pBuffer.Address());

	if (!EnumDependentServices(m_Handle,SERVICE_ACTIVE, pDepStatus, nBytesNeeded, &nBytesNeeded, &nCount))
	{
		SaveWin32Error("EnumDependentServices", GetLastError());
		throw E_APIERROR;
	}
	
	for (unsigned int xj = 0; xj < nCount; xj++) 
	{
		hDepService.Open(hSCM,pDepStatus->lpServiceName, SERVICE_STOP|SERVICE_QUERY_STATUS|SERVICE_ENUMERATE_DEPENDENTS);
		hDepService.Stop(true,INFINITE,hSCM);
		pDepStatus++;
	}
}

Service& Service::Attach(ValueEx &pVal)
{
	if (m_Handle && m_Owner)
	{
		CloseServiceHandle(m_Handle);
		m_Handle = NULL;
	}

	if (pVal.Vartype() == 'I' || pVal.Vartype() == 'N')
		m_Handle = pVal.DynamicPtr<SC_HANDLE>();

	m_Owner = false;
	return *this;
}

void _fastcall OpenServiceLib(ParamBlkEx& parm)
{
try
{
	DWORD dwAccess = parm.PCount() >= 2 && parm(2)->ev_long ? parm(2)->ev_long : SERVICE_ALL_ACCESS;
	FoxString pServiceName(parm, 1);
	FoxString pMachine(parm, 3, NullIfEmpty);
	FoxString pDatabase(parm, 4, NullIfEmpty);

	ServiceManager hSCM;
	Service hService;

	hSCM.Open(pMachine,pDatabase);
	hService.Open(hSCM,pServiceName,dwAccess);

	Return(hService.Detach());
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall CloseServiceHandleLib(ParamBlkEx& parm)
{
try
{
	if (!CloseServiceHandle(parm(1)->Ptr<SC_HANDLE>()))
	{
		SaveWin32Error("CloseServiceHandle", GetLastError());
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall StartServiceLib(ParamBlkEx& parm)
{
try
{
	if (parm.PCount() >= 2 && parm(2)->Vartype() != 'R' && parm(2)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	if (parm.PCount() >= 3 && parm(3)->Vartype() != 'I' && parm(3)->Vartype() != 'N' && parm(3)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	FoxString pService(parm, 1);
	FoxArray pArgs(parm, 2);
	FoxString pMachine(parm, 4, NullIfEmpty);
	FoxString pDatabase(parm, 5, NullIfEmpty);
	FoxCStringArray pArguments;
	ServiceManager hSCM;
	Service hService;

	if (parm(1)->Vartype() == 'I' || parm(1)->Vartype() == 'N')
	{
		if (parm.PCount() >= 4)
			throw E_INVALIDPARAMS;
		hService.Attach(parm(1));
	}
	else if (parm(1)->Vartype() == 'C')
	{
		hSCM.Open(pMachine,pDatabase);
		hService.Open(hSCM,pService, SERVICE_START|SERVICE_QUERY_STATUS);
	}
	else
		throw E_INVALIDPARAMS;

	pArguments = pArgs;

	if (parm.PCount() < 3 || parm(3)->Vartype() == '0')
		parm(3)->ev_long = SERVICE_DEFAULT_TIMEOUT;

	Return (hService.Start(pArguments.ARows(), pArguments, parm(3)->ev_long));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall StopServiceLib(ParamBlkEx& parm)
{
try
{
	if (parm.PCount() >= 2 && parm(2)->Vartype() != 'I' && parm(2)->Vartype() != 'N' && parm(2)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	if (parm.PCount() < 2 || parm(2)->Vartype() == '0')
		parm(2)->ev_long = SERVICE_DEFAULT_TIMEOUT;

	bool bStopDependencies = parm.PCount() >= 3 && parm(3)->ev_length;

	FoxString pServiceName(parm, 1);
	FoxString pMachine(parm, 4, NullIfEmpty);
	FoxString pDatabase(parm, 5, NullIfEmpty);

	ServiceManager hSCM;
	Service hService;

	if (parm(1)->Vartype() == 'C' || bStopDependencies)
        hSCM.Open(pMachine,pDatabase);

	if (parm(1)->Vartype() == 'I' || parm(1)->Vartype() == 'N')
	{
		hService.Attach(parm(1));
	}
	else if (parm(1)->Vartype() == 'C')
	{
		hService.Open(hSCM,pServiceName,bStopDependencies ? (SERVICE_STOP|SERVICE_QUERY_STATUS|SERVICE_ENUMERATE_DEPENDENTS) : (SERVICE_STOP|SERVICE_QUERY_STATUS));
	}
	else
		throw E_INVALIDPARAMS;

	Return(hService.Stop(bStopDependencies,parm(2)->ev_long,hSCM));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall PauseService(ParamBlkEx& parm)
{
try
{
	if (parm.PCount() >= 2 && parm(2)->Vartype() != 'I' && parm(2)->Vartype() != 'N' && parm(2)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	if (parm.PCount() < 2 || parm(2)->Vartype() == '0')
		parm(2)->ev_long = SERVICE_DEFAULT_TIMEOUT;

	FoxString pServiceName(parm, 1);
	FoxString pMachine(parm, 3, NullIfEmpty);
	FoxString pDatabase(parm, 4, NullIfEmpty);
	ServiceManager hSCM;
	Service hService;

	if (parm(1)->Vartype() == 'I' || parm(1)->Vartype() == 'N')
	{
		hService.Attach(parm(1));
	}
	else if (parm(1)->Vartype() == 'C')
	{
		hSCM.Open(pMachine,pDatabase);
		hService.Open(hSCM,pServiceName,SERVICE_PAUSE_CONTINUE|SERVICE_QUERY_STATUS);
	}
	else
		throw E_INVALIDPARAMS;

	Return(hService.Pause(parm(2)->ev_long));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);	
}
}

void _fastcall ContinueService(ParamBlkEx& parm)
{
try
{
	if (parm.PCount() >= 2 && parm(2)->Vartype() != 'I' && parm(2)->Vartype() != 'N' && parm(2)->Vartype() != '0')
		throw E_INVALIDPARAMS;

	if (parm.PCount() < 2 || parm(2)->Vartype() == '0')
		parm(2)->ev_long = SERVICE_DEFAULT_TIMEOUT;

	FoxString pServiceName(parm, 1);
	FoxString pMachine(parm, 3, NullIfEmpty);
	FoxString pDatabase(parm, 4, NullIfEmpty);
	ServiceManager hSCM;
	Service hService;

	if (parm(1)->Vartype() == 'I' || parm(1)->Vartype() == 'N')
	{
		hService.Attach(parm(1));
	}
	else if (parm(1)->Vartype() == 'C')
	{
		hSCM.Open(pMachine,pDatabase);
		hService.Open(hSCM,pServiceName,SERVICE_PAUSE_CONTINUE|SERVICE_QUERY_STATUS);
	}
	else
		throw E_INVALIDPARAMS;

	Return (hService.Continue(parm(2)->ev_long));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ControlServiceLib(ParamBlkEx& parm)
{
try
{
	FoxString pServiceName(parm, 1);
	FoxString pMachine(parm, 3, NullIfEmpty);
	FoxString pDatabase(parm, 4, NullIfEmpty);
	ServiceManager hSCM;
	Service hService;

	if (parm(1)->Vartype() == 'I' || parm(1)->Vartype() == 'N')
	{
		hService.Attach(parm(1));
	}
	else if (parm(1)->Vartype() == 'C')
	{
		hSCM.Open(pMachine,pDatabase);
		hService.Open(hSCM,pServiceName,SERVICE_USER_DEFINED_CONTROL);
	}
	else
		throw E_INVALIDPARAMS;

	hService.Control(parm(2)->ev_long);
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AServices(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),1,10);
	FoxString pMachine(parm, 2, NullIfEmpty);
	FoxString pDatabase(parm, 3, NullIfEmpty);
	FoxString pStringBuffer(MAX_PATH+1);
	ServiceManager hSCM;
	CBuffer pBuffer;

	DWORD dwState, dwType, dwBytes = SERVICE_ENUM_BUFFER, hResume = 0;
	DWORD nServices, nLastError;
	unsigned int nRow;
	LPENUM_SERVICE_STATUS_PROCESS pServiceStatusEx;
	
	dwState = parm.PCount() >= 4 && parm(4)->ev_long ? parm(4)->ev_long : SERVICE_STATE_ALL;
	dwType = parm.PCount() >= 5 && parm(5)->ev_long ? parm(5)->ev_long : SERVICE_WIN32;

	hSCM.Open(pMachine,pDatabase,SC_MANAGER_ENUMERATE_SERVICE);

	pBuffer.Size(dwBytes);
	pArray.AutoGrow(32);

	while (1)
	{
		if (!EnumServicesStatusEx(hSCM,SC_ENUM_PROCESS_INFO, dwType, dwState, pBuffer, dwBytes, &dwBytes, &nServices, &hResume,0))
			nLastError = GetLastError();
		else
			nLastError = ERROR_SUCCESS;

		if (nLastError != ERROR_SUCCESS && nLastError != ERROR_MORE_DATA)
		{
			SaveWin32Error("EnumServicesStatusEx", nLastError);
			throw E_APIERROR;
		}

		pServiceStatusEx = reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESS>(pBuffer.Address());
		while (nServices--)
		{
			nRow = pArray.Grow();
			pArray(nRow,1) = pStringBuffer = pServiceStatusEx->lpServiceName;
			pArray(nRow,2) = pStringBuffer = pServiceStatusEx->lpDisplayName;
			pArray(nRow,3) = pServiceStatusEx->ServiceStatusProcess.dwServiceType;
			pArray(nRow,4) = pServiceStatusEx->ServiceStatusProcess.dwCurrentState;
			pArray(nRow,5) = pServiceStatusEx->ServiceStatusProcess.dwWin32ExitCode;
			pArray(nRow,6) = pServiceStatusEx->ServiceStatusProcess.dwServiceSpecificExitCode;
			pArray(nRow,7) = pServiceStatusEx->ServiceStatusProcess.dwCheckPoint;
			pArray(nRow,8) = pServiceStatusEx->ServiceStatusProcess.dwControlsAccepted;
			pArray(nRow,9) = pServiceStatusEx->ServiceStatusProcess.dwServiceFlags;
			pArray(nRow,10) = pServiceStatusEx->ServiceStatusProcess.dwProcessId;
			pServiceStatusEx++;
		}
		
		if (nLastError == ERROR_MORE_DATA)
		{
			if (dwBytes < SERVICE_ENUM_BUFFER)
				continue;

			dwBytes = max(dwBytes, SERVICE_MAX_ENUM_BUFFER);
			pBuffer.Size(dwBytes);
		}
		else
			break;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AServiceStatus(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),7);
	FoxString pServiceName(parm, 2);
	FoxString pMachine(parm, 3, NullIfEmpty);
	FoxString pDatabase(parm, 4, NullIfEmpty);
	ServiceManager hSCM;
	Service hService;
	SERVICE_STATUS sStatus;

	if (parm(2)->Vartype() == 'I' || parm(2)->Vartype() == 'N')
	{
		if (parm.PCount() > 2)
			throw E_INVALIDPARAMS;
		hService.Attach(parm(2));
	}
	else if (parm(2)->Vartype() == 'C')
	{
		hSCM.Open(pMachine, pDatabase);
		hService.Open(hSCM, pServiceName, SERVICE_START|SERVICE_QUERY_STATUS);
	}
	else
		throw E_INVALIDPARAMS;

	hService.QueryStatus(&sStatus);

	pArray(1) = sStatus.dwCheckPoint;
	pArray(2) = sStatus.dwControlsAccepted;
	pArray(3) = sStatus.dwCurrentState;
	pArray(4) = sStatus.dwServiceSpecificExitCode;
	pArray(5) = sStatus.dwServiceType;
	pArray(6) = sStatus.dwWaitHint;
	pArray(7) = sStatus.dwWin32ExitCode;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall AServiceConfig(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),9);
	FoxString pServiceName(parm, 2);
	FoxString pMachine(parm, 3, NullIfEmpty);
	FoxString pDatabase(parm, 4, NullIfEmpty);
	FoxString pStringBuffer(4096);
	ServiceManager hSCM;
	Service hService;
	CBuffer pBuffer;
	LPQUERY_SERVICE_CONFIG pServiceConfig;

	if (parm(2)->Vartype() == 'I' || parm(2)->Vartype() == 'N')
	{
		if (parm.PCount() > 2)
			throw E_INVALIDPARAMS;
		hService.Attach(parm(2));
	}
	else if (parm(2)->Vartype() == 'C')
	{
		hSCM.Open(pMachine,pDatabase);
		hService.Open(hSCM,pServiceName,SERVICE_START|SERVICE_QUERY_STATUS);
	}
	else
		throw E_INVALIDPARAMS;

	hService.QueryConfig(pBuffer);

	pServiceConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIG>(pBuffer.Address());

	pArray(1) = pServiceConfig->dwServiceType;
	pArray(2) = pServiceConfig->dwStartType;
	pArray(3) = pServiceConfig->dwErrorControl;
	pArray(4) = pStringBuffer = pServiceConfig->lpBinaryPathName;
	pArray(5) = pStringBuffer = pServiceConfig->lpLoadOrderGroup;
	pArray(6) = pServiceConfig->dwTagId;

	pArray(7) = pStringBuffer.CopyDblString(pServiceConfig->lpDependencies);
	pArray(8) = pStringBuffer = pServiceConfig->lpServiceStartName;
	pArray(9) = pStringBuffer = pServiceConfig->lpDisplayName;
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall ADependentServices(ParamBlkEx& parm)
{
try
{
	FoxArray pArray(parm(1),1,8);
	FoxString pServiceName(parm, 2);
	FoxString pMachine(parm, 3, NullIfEmpty);
	FoxString pDatabase(parm, 4, NullIfEmpty);
	FoxString pStringBuffer(MAX_PATH+1);

	ServiceManager hSCM;
	Service hService;

	CBuffer pBuffer;
	LPENUM_SERVICE_STATUS pServiceStatus;
	DWORD dwBytesNeeded = 0, dwServiceCount = 0, nLastError;
	unsigned int nRow;

	if (parm(2)->Vartype() == 'I' || parm(2)->Vartype() == 'N')
	{
		if (parm.PCount() > 2)
			throw E_INVALIDPARAMS;
		hService.Attach(parm(2));
	}
	else if (parm(2)->Vartype() == 'C')
	{
		hSCM.Open(pMachine,pDatabase);
		hService.Open(hSCM,pServiceName,SERVICE_START|SERVICE_QUERY_STATUS);
	}
	else
		throw E_INVALIDPARAMS;

	if (!EnumDependentServices(hService, SERVICE_STATE_ALL, 0, 0, &dwBytesNeeded, &dwServiceCount))
	{
		nLastError = GetLastError();
		if (nLastError != ERROR_MORE_DATA)
		{
			SaveWin32Error("EnumDependentServices", nLastError);
			throw E_APIERROR;
		}
	}
	else if (dwServiceCount == 0)
	{
		Return(0);
		return;
	}

	pBuffer.Size(dwBytesNeeded);
	pServiceStatus = reinterpret_cast<LPENUM_SERVICE_STATUS>(pBuffer.Address());

	if (!EnumDependentServices(hService, SERVICE_STATE_ALL, pServiceStatus, pBuffer.Size(), &dwBytesNeeded, &dwServiceCount))
	{
		SaveWin32Error("EnumDependentServices", nLastError);
		throw E_APIERROR;
	}
    
	pArray.Dimension(dwServiceCount, 8);
	while (dwServiceCount--)
	{
		nRow = pArray.Grow();
		pArray(nRow,1) = pStringBuffer = pServiceStatus->lpServiceName;
		pArray(nRow,2) = pStringBuffer = pServiceStatus->lpDisplayName;
		pArray(nRow,3) = pServiceStatus->ServiceStatus.dwServiceType;
		pArray(nRow,4) = pServiceStatus->ServiceStatus.dwCurrentState;
		pArray(nRow,5) = pServiceStatus->ServiceStatus.dwWin32ExitCode;
		pArray(nRow,6) = pServiceStatus->ServiceStatus.dwServiceSpecificExitCode;
		pArray(nRow,7) = pServiceStatus->ServiceStatus.dwCheckPoint;
		pArray(nRow,8) = pServiceStatus->ServiceStatus.dwControlsAccepted;
		pServiceStatus++;
	}

	pArray.ReturnRows();
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall WaitForServiceStatus(ParamBlkEx& parm)
{
try
{
	FoxString pServiceName(parm, 1);
	FoxString pMachine(parm, 4, NullIfEmpty);
	FoxString pDatabase(parm, 5, NullIfEmpty);

	ServiceManager hSCM;
	Service hService;
	int nTimeout;

	if (parm(1)->Vartype() == 'I' || parm(1)->Vartype() == 'N')
	{
		hService.Attach(parm(1));
	}
	else if (parm(1)->Vartype() == 'C')
	{
		hSCM.Open(pMachine, pDatabase);
		hService.Open(hSCM, pServiceName, SERVICE_QUERY_STATUS);
	}
	else
		throw E_INVALIDPARAMS;

	nTimeout = parm.PCount() < 3 ? SERVICE_INFINITE_TIMEOUT : parm(3)->ev_long;
	
	Return(hService.WaitForServiceStatus(parm(2)->ev_long, nTimeout));
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall CreateServiceLib(ParamBlkEx& parm)
{
try
{
	FoxString pServiceName(parm(1));
	FoxString pDisplayName(parm(2));
	FoxString pExecutable(parm(3));
	DWORD dwServiceType = parm.PCount() >= 4 && parm(4)->Vartype() == 'N' ? (DWORD)parm(4)->ev_real : SERVICE_WIN32_OWN_PROCESS;
	DWORD dwStartType = parm.PCount() >= 5 && parm(5)->Vartype() == 'N' ? (DWORD)parm(5)->ev_real : SERVICE_AUTO_START;
	DWORD dwErrorControl = parm.PCount() >= 6 && parm(6)->Vartype() == 'N' ? (DWORD)parm(6)->ev_real : SERVICE_ERROR_NORMAL;
	FoxString pLoadOrderGroup(parm, 7);
	FoxString pDependencies(parm, 8);
	FoxString pServiceAccount(parm, 9);
	FoxString pPassword(parm, 10);
	FoxString pMachine(parm, 11, NullIfEmpty);
	FoxString pDatabase(parm, 12, NullIfEmpty);
	SC_HANDLE hService;

	ServiceManager hSCM;
	hSCM.Open(pMachine, pDatabase, SC_MANAGER_CREATE_SERVICE);

	hService = CreateService(hSCM, pServiceName, pDisplayName, SERVICE_ALL_ACCESS, 
					dwServiceType, dwStartType, dwErrorControl, pExecutable, 
					pLoadOrderGroup, 0, pDependencies, pServiceAccount, pPassword);

	if (hService == 0)
	{
		SaveWin32Error("CreateService", GetLastError());
		throw E_APIERROR;
	}

	if (CloseServiceHandle(hService) == 0)
	{
		SaveWin32Error("CloseServiceHandle", GetLastError());
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}

void _fastcall DeleteServiceLib(ParamBlkEx& parm)
{
try
{
	FoxString pServiceName(parm, 1);
	FoxString pMachine(parm, 2, NullIfEmpty);
	FoxString pDatabase(parm, 3, NullIfEmpty);

	ServiceManager hSCM;
	Service hService;

	if (parm(1)->Vartype() == 'I' || parm(1)->Vartype() == 'N')
	{
		hService.Attach(parm(1));
	}
	else if (parm(1)->Vartype() == 'C')
	{
		hSCM.Open(pMachine, pDatabase);
		hService.Open(hSCM, pServiceName, DELETE);
	}
	else
		throw E_INVALIDPARAMS;

	if (DeleteService(hService) == 0)
	{
		SaveWin32Error("DeleteService", GetLastError());
		throw E_APIERROR;
	}
}
catch(int nErrorNo)
{
	RaiseError(nErrorNo);
}
}