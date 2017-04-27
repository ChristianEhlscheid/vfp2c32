// vfpsrvhost.cpp : Defines the entry point for the console application.
//

#include "VfpSrvHost.h"
#include "SrvHostConfig.h"
#include "Exceptions.h"

// Global variables
CString gPath;	// path to this executable
CString gExeFile;	// filename of this executable
SrvHostConfig gConfig;	// configuration object
long volatile CService::ShutdownFlag = 0;

int _tmain(int argc, _TCHAR* argv[])
{
	for (int xj = 0; xj < argc; xj++)
	{
		AtlTrace(_T("VFPSrvHost - _tmain argv[%d]: %s\n"), xj, argv[xj]);
	}

	try
	{
		DWORD nLen = GetModuleFileName(NULL, gPath.GetBuffer(MAX_PATH), MAX_PATH);
		if (nLen == 0)
			throw FileLogException(_T("GetModuleFileName"), GetLastError());
		gPath.ReleaseBufferSetLength(nLen);

		// strip filename from fullpath
		int pos = gPath.ReverseFind(_T('\\'));
		gExeFile = gPath.Right(gPath.GetLength() - pos - 1);
		gPath.Truncate(pos);

		if (!SetCurrentDirectory(gPath))
			throw FileLogException(_T("SetCurrentDirectory"), GetLastError());

		// config file = exefilename with .xml extension
		CString configFile = gExeFile.Left(gExeFile.ReverseFind('.') + 1) + _T("xml");
		if (!gConfig.Load(configFile))
			throw FileLogException(gConfig.GetLastFunction(), gConfig.GetLastError());

		// If command-line parameters are passed we are called manually.
		// otherwise, the service is probably being started by the SCM.
		if(argc > 1)
			return SrvHostProcessCommandLine(argc, argv);

		int ServiceCount = gConfig.Services.GetCount();
		SERVICE_TABLE_ENTRY* DispatchTable = new SERVICE_TABLE_ENTRY[ServiceCount + 1];
		if (!DispatchTable)
			throw FileLogException(_T("_tmain"), ERROR_OUTOFMEMORY);

		for (size_t xj = 0; xj < gConfig.Services.GetCount(); xj++)
		{
			DispatchTable[xj].lpServiceName = gConfig.Services[xj]->Name();
			DispatchTable[xj].lpServiceProc = CService::Main;
		}
		DispatchTable[ServiceCount].lpServiceName = 0;
		DispatchTable[ServiceCount].lpServiceProc = 0;

		AtlTrace(_T("VFPSrvHost - _tmain StartServiceCtrlDispatcher\n"));
		// This call returns when the service has stopped. 
		// The process should simply terminate when the call returns.
		if (!StartServiceCtrlDispatcher(DispatchTable)) 
			throw FileLogException(_T("StartServiceCtrlDispatcher"), GetLastError());

		delete[] DispatchTable;
		AtlTrace(_T("VFPSrvHost - _tmain StartServiceCtrlDispatcher returned\n"));
	}
	catch(BaseException &pExc)
	{
		AtlTrace(_T("VFPSrvHost - _tmain - Error logged to 'vfpsrvhost.log'\n"));
		pExc.Log();
	}
	catch(CAtlException pExc)
	{
		AtlTrace(_T("VFPSrvHost - _tmain - CAtlException: 0x%x\n"), pExc.m_hr);
	}
	
	CService::WaitForThreadShutdown();

	gPath.ReleaseBuffer();
	gExeFile.ReleaseBuffer();
	gConfig.Release();

	AtlTrace(_T("VFPSrvHost - _tmain exiting\n"));
	return 0;
}

int SrvHostProcessCommandLine(int argc, _TCHAR* argv[])
{
	CService *Service = 0;
	CString pExcMessage;
	int Action;
	const int ACTION_INSTALL = 1;
	const int ACTION_UNINSTALL = 2;

	try
	{
		if (_tcsicmp(argv[1], _T("install")) == 0)
			Action = ACTION_INSTALL;
		else if (_tcsicmp(argv[1], _T("uninstall")) == 0)
			Action = ACTION_UNINSTALL;
		else
		{
			pExcMessage.Format(_T("Unrecognized command line parameter '%s'!"), argv[1]);
			throw StdoutException(pExcMessage);
		}

		if (argc > 2)
		{
			for (size_t xj = 0; xj < gConfig.Services.GetCount(); xj++)
			{
				if (gConfig.Services[xj]->ServiceName.CompareNoCase(argv[2]) == 0)
				{
					Service = gConfig.Services[xj];
					break;
				}
			}

			if (Service == 0)
			{
				pExcMessage.Format(_T("Service '%s' not found in configuration file!"), argv[2]);
				throw StdoutException(pExcMessage);
			}
		}

		// no specific service passed - do requested action for all services
		if (Service == 0)
		{
			for (size_t xj = 0; xj < gConfig.Services.GetCount(); xj++)
			{
				if (Action == ACTION_INSTALL)
					gConfig.Services[xj]->Install();
				else
					gConfig.Services[xj]->Uninstall();
			}
		}
		else
		{
			if (Action == ACTION_INSTALL)
				Service->Install();
			else
				Service->Uninstall();
		}
	}
	catch(StdoutException &pExc)
	{
		pExc.Log();
	}
	return 0;
}

PREGISTERPOWERSETTINGNOTIFICATION CService::pRegisterPowerSettingNotification = 0;
PUNREGISTERPOWERSETTINGNOTIFICATION CService::pUnregisterPowerSettingNotification = 0;

CService::CService() : ServiceId(0), StatusHandle(0),  StopWaitHint(20000), PauseWaitHint(20000), ContinueWaitHint(5000), 
					ServiceObject(0), ServiceObjectCookie(0), GIT(0), StopEvent(0), StopEventSignaled(false)
{
	RtlZeroMemory(&Status, sizeof(SERVICE_STATUS));
	Status.dwWaitHint = 10000;
}

STDMETHODIMP CService::QueryInterface(REFIID riid, void **ppvObject)
{
	return E_NOINTERFACE;
	*ppvObject = NULL;
	HRESULT hr;

	// IUnknown
	if (::IsEqualIID(riid, __uuidof(IUnknown)))
	{
		*ppvObject = this;
		hr = S_OK;
	}
	// IDispatch
	else if (::IsEqualIID(riid, __uuidof(IDispatch)))
	{
		*ppvObject = reinterpret_cast<IDispatch*>(this);
		hr = S_OK;
	}
	else
		hr = E_NOINTERFACE;
	return hr;
}

STDMETHODIMP_(ULONG) CService::AddRef()
{
	return 1;
}

STDMETHODIMP_(ULONG) CService::Release()
{
	return 1;
}

STDMETHODIMP CService::GetTypeInfoCount(UINT* pctinfo)
{
	*pctinfo = 0; 
	return S_OK;
}

STDMETHODIMP CService::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP CService::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{
	HRESULT hr;
	if (wcscmp(L"stop", *rgszNames) == 0)
	{
		hr = S_OK;
		*rgdispid = DISPID_Stop;
	}
	if (wcscmp(L"requestadditionaltime", *rgszNames) == 0)
	{
		hr = S_OK;
		*rgdispid = DISPID_RequestAdditionalTime;
	}
	else
	{
		hr = DISP_E_UNKNOWNNAME;
		*rgdispid = DISPID_UNKNOWN;
	}
	return hr;
}

STDMETHODIMP CService::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, 
										DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	HRESULT hr;
	switch(dispidMember)
	{
		case DISPID_Stop:
			if (pdispparams->cArgs == 0)
			{
				StopEventSignaled = true;
				SetEvent(StopEvent);
				hr = S_OK;
			}
			else
				hr = DISP_E_BADPARAMCOUNT;
			break;

		case DISPID_RequestAdditionalTime:
			if (pdispparams->cArgs == 1)
				hr = RequestAdditionalTime(&pdispparams->rgvarg[0]);
			else
				hr = DISP_E_BADPARAMCOUNT;
			break;
	}
	return hr;
}

HRESULT CService::Stop()
{
	Status.dwCurrentState = SERVICE_STOP_PENDING;
	Status.dwWaitHint = StopWaitHint;
	Status.dwCheckPoint = 0;
	SetStatus();

	HRESULT hr = S_OK;
	DISPID dispid;
	if (GetDispId(SERVICE_CONTROL_STOP, &dispid))
	{
		HRESULT hrex;
		CComPtr<IDispatch> pObject;
		CComVariant result;
		DISPPARAMS parms = {0};
		hrex = GetServiceObject(&pObject);
		if (SUCCEEDED(hrex))
		{
			hrex = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, 0, 0, 0);
			if (GetDispId(SERVICE_EXITCODE, &dispid))
			{
				hrex = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_PROPERTYGET, 0, &result, 0, 0);
				if (SUCCEEDED(hr) && result.vt == VT_I4)
					Status.dwServiceSpecificExitCode = result.lVal;
			}
		}
	}
	return hr;
}

HRESULT CService::RequestAdditionalTime(VARIANT *Milliseconds)
{
	HRESULT hr;
	hr = VariantChangeType(Milliseconds, Milliseconds, 0, VT_I4);
	if (SUCCEEDED(hr))
	{
		Status.dwCheckPoint++;
		Status.dwWaitHint = Milliseconds->lVal;
		DWORD lastError = SetStatus();
		if (lastError)
			hr = HRESULT_FROM_WIN32(lastError);
	}
	return hr;
}

void CService::Initialize()
{
	AtlTrace(_T("VFPSrvHost - CService::Initialize()\n"));
	// Register the handler function for the service
	StatusHandle = RegisterServiceCtrlHandlerEx(ServiceName, HandlerEx, (LPVOID)ServiceId);
	if(!StatusHandle)
		throw FileLogException(_T("RegisterServiceCtrlHandlerEx"), GetLastError());

	// set service state to start pending
	Status.dwCurrentState = SERVICE_START_PENDING;
	Status.dwServiceType = gConfig.Services.GetCount() > 1 ? SERVICE_WIN32_SHARE_PROCESS : SERVICE_WIN32_OWN_PROCESS;
	DWORD ret = SetStatus();
	if (ret)
		throw FileLogException(_T("SetServiceStatus"), GetLastError());

	StopEvent = CreateEvent(0, TRUE, FALSE, 0);
	if (!StopEvent)
		throw FileLogException(_T("CreateEvent"), GetLastError());

	HRESULT hr;
	CLSID clsid;

	hr = CLSIDFromProgID(ComClass, &clsid);
	if (FAILED(hr))
		throw FileLogException(_T("CLSIDFromProgID"), hr);

	hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, IID_IGlobalInterfaceTable, reinterpret_cast<void **>(&GIT));
	if (FAILED(hr))
		throw FileLogException(_T("CoCreateInstance"), hr);

	hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IDispatch, reinterpret_cast<void**>(&ServiceObject));
	if (FAILED(hr))
		throw FileLogException(_T("CoCreateInstance"), hr);

	hr = GIT->RegisterInterfaceInGlobal(ServiceObject, IID_IDispatch, &ServiceObjectCookie);
	if (FAILED(hr))
		throw FileLogException(_T("IGlobalInterfaceTable::RegisterInterfaceInGlobal"), hr);

	struct _servicefuncs
	{
		LPTSTR Name;
		DWORD ControlCode;
		DWORD Mask;
	};

	static struct _servicefuncs servicefuncs[] = 
	{
		{L"OnStart", SERVICE_CONTROL_START, 0},
		{L"OnStop", SERVICE_CONTROL_STOP, SERVICE_ACCEPT_STOP},
		{L"OnPause", SERVICE_CONTROL_PAUSE, SERVICE_ACCEPT_PAUSE_CONTINUE},
		{L"OnContinue", SERVICE_CONTROL_CONTINUE, SERVICE_ACCEPT_PAUSE_CONTINUE},
		{L"OnInterrogate", SERVICE_CONTROL_INTERROGATE, 0},
		{L"OnShutdown", SERVICE_CONTROL_SHUTDOWN, SERVICE_ACCEPT_SHUTDOWN},
		{L"OnParamChange", SERVICE_CONTROL_PARAMCHANGE, SERVICE_ACCEPT_PARAMCHANGE},
		{L"OnDeviceEvent", SERVICE_CONTROL_DEVICEEVENT, 0},
		{L"OnHardwareProfileChange", SERVICE_CONTROL_HARDWAREPROFILECHANGE, SERVICE_ACCEPT_HARDWAREPROFILECHANGE},
		{L"OnPowerEvent", SERVICE_CONTROL_POWEREVENT, SERVICE_ACCEPT_POWEREVENT},
		{L"OnSessionChange", SERVICE_CONTROL_SESSIONCHANGE, SERVICE_ACCEPT_SESSIONCHANGE},
		{L"OnPreShutdown", SERVICE_CONTROL_PRESHUTDOWN, SERVICE_ACCEPT_PRESHUTDOWN},
		{L"OnTimeChange", SERVICE_CONTROL_TIMECHANGE, SERVICE_ACCEPT_TIMECHANGE},
		{L"OnCustomCommand", SERVICE_CONTROL_CUSTOM, 0},
		{L"ExitCode", SERVICE_EXITCODE, 0},
	};
	
	DISPID dispid;
	int entry = 0;
	int count = sizeof(servicefuncs) / sizeof(servicefuncs[0]);
	DispatchMap.SetCount(count);
	ControlsAccepted = 0;

	for (int xj = 0; xj < count; xj++)
	{
		dispid = DISPID_UNKNOWN;
		hr = ServiceObject->GetIDsOfNames(IID_NULL, &servicefuncs[xj].Name, 1, DEFAULT_LCID, &dispid);
		if (hr != S_OK && hr != DISP_E_UNKNOWNNAME)
			throw FileLogException(_T("IDispatch::GetIDsOfNames"), hr);
		
		if (dispid != DISPID_UNKNOWN)
		{
			ControlsAccepted |= servicefuncs[xj].Mask;
			CControlToDispEntry dispentry(servicefuncs[xj].Name, servicefuncs[xj].ControlCode, dispid);
			DispatchMap[entry] = dispentry;
			entry++;
		}
	}
	DispatchMap.SetCount(entry);

	OSVERSIONINFO osinfo;
	osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osinfo) == 0)
		throw FileLogException(_T("GetVersionEx"), GetLastError());

	// SERVICE_ACCEPT_PRESHUTDOWN is not supported on Windows XP/2000 or Windows Server 2003
	if (osinfo.dwMajorVersion < 6)
		ControlsAccepted &= ~SERVICE_ACCEPT_PRESHUTDOWN;

	if ((ControlsAccepted & SERVICE_ACCEPT_PRESHUTDOWN) && (ControlsAccepted & SERVICE_ACCEPT_SHUTDOWN))
		ControlsAccepted &= ~SERVICE_ACCEPT_SHUTDOWN;

	// SERVICE_ACCEPT_SESSIONCHANGE is not supported on Windows 2000 or earlier
	if (osinfo.dwMajorVersion < 5 || (osinfo.dwMajorVersion == 5 && osinfo.dwMinorVersion < 1))
		ControlsAccepted &= ~SERVICE_ACCEPT_SESSIONCHANGE;

	// SERVICE_ACCEPT_TIMECHANGE is only supported on Windows 7/Windows Server 2008 R2 or later
	if (osinfo.dwMajorVersion < 6 || (osinfo.dwMajorVersion == 6 && osinfo.dwMinorVersion < 1))
		ControlsAccepted &= ~SERVICE_ACCEPT_TIMECHANGE;
	
	if (!GetDispId(SERVICE_CONTROL_START))
		throw FileLogException(_T("Service::OnStart"), E_NOTIMPL);
	if (!GetDispId(SERVICE_CONTROL_STOP))
		throw FileLogException(_T("Service::OnStop"), E_NOTIMPL);
}

void CService::RegPowerSettingNotifications()
{
	AtlTrace(_T("VFPSrvHost - CService::RegPowerSettingNotifications()\n"));
	if (PowerNotifications.GetLength() == 0)
		return;

	if (!GetDispId(SERVICE_CONTROL_POWEREVENT))
		throw FileLogException(_T("PowerNotifications specified in configuration file, but OnPowerEvent not implemented!"));

	if (pRegisterPowerSettingNotification == 0 || pUnregisterPowerSettingNotification == 0)
	{
		HMODULE hUser32 = GetModuleHandle(_T("user32.dll"));
		if (!hUser32)
			throw FileLogException(_T("GetModuleHandle"), GetLastError());

		pRegisterPowerSettingNotification = (PREGISTERPOWERSETTINGNOTIFICATION)GetProcAddress(hUser32, "RegisterPowerSettingNotification");
		pUnregisterPowerSettingNotification = (PUNREGISTERPOWERSETTINGNOTIFICATION)GetProcAddress(hUser32, "UnregisterPowerSettingNotification");
		if (pRegisterPowerSettingNotification == 0 || pUnregisterPowerSettingNotification == 0)
			return;
	}

	CString pSetting;
	int start = 0;
	GUID pGuid;

	while(true)
	{
		pSetting = PowerNotifications.Tokenize(_T(";"), start);
		if (start == -1)
			break;

		pSetting.Trim(_T(" "));

		if (pSetting == _T("GUID_POWERSCHEME_PERSONALITY"))
			pGuid = GUID_POWERSCHEME_PERSONALITY;
		else if (pSetting == _T("GUID_ACDC_POWER_SOURCE"))
			pGuid = GUID_ACDC_POWER_SOURCE;
		else if (pSetting == _T("GUID_BATTERY_PERCENTAGE_REMAINING"))
			pGuid = GUID_BATTERY_PERCENTAGE_REMAINING;
		else if (pSetting == _T("GUID_IDLE_BACKGROUND_TASK"))
			pGuid = GUID_IDLE_BACKGROUND_TASK;
		else if (pSetting == _T("GUID_SYSTEM_AWAYMODE"))
			pGuid = GUID_SYSTEM_AWAYMODE;
		else if (pSetting == _T("GUID_MONITOR_POWER_ON"))
			pGuid = GUID_MONITOR_POWER_ON;
		else
			throw FileLogException(_T("Unknown PowerNotification option in config"));

		HPOWERNOTIFY handle = pRegisterPowerSettingNotification(StatusHandle, &pGuid, DEVICE_NOTIFY_SERVICE_HANDLE);
		if (handle)
			PowerNotifyHandles.Add(handle);
		else
			throw FileLogException(_T("RegisterPowerSettingNotification"), GetLastError());
	}
}

void CService::Shutdown()
{
	AtlTrace(_T("VFPSrvHost - CService::Shutdown()\n"));
	if (StopEventSignaled)
		Stop();

	if (ServiceObject)
	{
		AtlTrace(_T("VFPSrvHost - CService::Shutdown() - releasing ServiceObject\n"));
		ServiceObject->Release();
		ServiceObject = 0;
	}
	
	if (GIT)
	{
		if (ServiceObjectCookie)
		{
			AtlTrace(_T("VFPSrvHost - CService::Shutdown() - releasing ServiceObjectCookie\n"));
			GIT->RevokeInterfaceFromGlobal(ServiceObjectCookie);
			ServiceObjectCookie = 0;
		}
		GIT->Release();
		GIT = 0;
	}

	AtlTrace(_T("VFPSrvHost - CService::Shutdown() - UnregisterPowerSettingNotification\n"));
	for (size_t xj = 0; xj < PowerNotifyHandles.GetCount(); xj++)
		pUnregisterPowerSettingNotification(PowerNotifyHandles[xj]);
	PowerNotifyHandles.SetCount(0);

	if (StopEvent)
	{
		AtlTrace(_T("VFPSrvHost - CService::Shutdown() - releasing StopEvent\n"));
		CloseHandle(StopEvent);
		StopEvent = 0;
	}

	if (StatusHandle)
	{
		AtlTrace(_T("VFPSrvHost - CService::Shutdown() - set service status to SERVICE_STOPPED\n"));
		Status.dwCurrentState = SERVICE_STOPPED;
		if (!SetServiceStatus(StatusHandle, &Status))
		{
			AtlTrace(_T("VFPSrvHost - CService::Release - SetServiceStatus failed: %d\n"), GetLastError());
		}
		StatusHandle = 0;
	}
}

bool CService::GetDispId(DWORD dwControlCode, DISPID* DispId)
{
	bool retval = false;
	for (size_t xj = 0; xj < DispatchMap.GetCount(); xj++)
	{
		if (DispatchMap[xj].ControlCode == dwControlCode)
		{
			if (DispatchMap[xj].DispId != DISPID_UNKNOWN)
				retval = true;
			if (DispId)
				*DispId = DispatchMap[xj].DispId;
			return retval;
		}
	}
	if (DispId)
		*DispId = DISPID_UNKNOWN;
	return false;
}

HRESULT CService::GetServiceObject(IDispatch** pDisp)
{
	return GIT->GetInterfaceFromGlobal(ServiceObjectCookie, IID_IDispatch, (void**)pDisp);
}

void CService::Start(DWORD dwArgc, LPTSTR *lpszArgv)
{
	AtlTrace(_T("VFPSrvHost - CService::Start()\n"));
	HRESULT hr;
	try
	{
		CComSafeArray<BSTR> pArray;
		hr = pArray.Create(dwArgc, 1);
		if (FAILED(hr))
			throw FileLogException(_T("CComSafeArray.Create"), hr);

		for (unsigned int xj = 0; xj < dwArgc; xj++)
		{
			hr = pArray.SetAt(xj+1, lpszArgv[xj]);
			if (FAILED(hr))
				throw FileLogException(_T("CComSafeArray.SetAt"), hr);
		}

		VARIANTARG pArgs[2];
		pArgs[1].vt = VT_ARRAY | VT_BSTR;
		pArgs[1].parray = pArray;
		pArgs[0].vt = VT_DISPATCH;
		pArgs[0].pdispVal = reinterpret_cast<IDispatch*>(this);

		DISPPARAMS sParams = {0};
		sParams.cArgs = 2;
		sParams.rgvarg = pArgs;
		
		DISPID dispid;
		GetDispId(SERVICE_CONTROL_START, &dispid);
		hr = ServiceObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &sParams, 0, 0, 0);
		if (FAILED(hr))
			throw FileLogException(_T("Service::Start"), hr);

		Status.dwCurrentState = SERVICE_RUNNING;
		Status.dwControlsAccepted = ControlsAccepted;
		SetStatus();
	}
	catch(FileLogException &pExc)
	{
		throw pExc;
	}
}

DWORD CService::OnStop()
{
	Status.dwCurrentState = SERVICE_STOP_PENDING;
	Status.dwWaitHint = StopWaitHint;
	Status.dwCheckPoint = 0;
	SetStatus();

	DISPID dispid;
	if (GetDispId(SERVICE_CONTROL_STOP, &dispid))
	{
		HRESULT hr;
		CComPtr<IDispatch> pObject;
		CComVariant result;
		DISPPARAMS parms = {0};
		hr = GetServiceObject(&pObject);
		if (SUCCEEDED(hr))
		{
			hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, 0, 0, 0);
			if (GetDispId(SERVICE_EXITCODE, &dispid))
			{
				hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_PROPERTYGET, &parms, &result, 0, 0);
				if (SUCCEEDED(hr) && result.vt == VT_I4)
					Status.dwServiceSpecificExitCode = result.lVal;
			}
		}
	}

	SetEvent(StopEvent);
	return NO_ERROR;
}

DWORD CService::OnPause()
{
	Status.dwCurrentState = SERVICE_PAUSE_PENDING;
	Status.dwWaitHint = PauseWaitHint;
	Status.dwCheckPoint = 0;
	SetStatus();

	DISPID dispid;
	if (GetDispId(SERVICE_CONTROL_PAUSE, &dispid))
	{
		HRESULT hr;
		CComPtr<IDispatch> pObject;
		DISPPARAMS parms = {0};
		hr = GetServiceObject(&pObject);
		if (SUCCEEDED(hr))
			hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, 0, 0, 0);
		if (FAILED(hr))
			throw FileLogException(_T("CService::OnPause"), hr);
	}

	Status.dwCurrentState = SERVICE_PAUSED;
	Status.dwControlsAccepted = ControlsAccepted;
	SetStatus();
	return NO_ERROR;
}

DWORD CService::OnContinue()
{
	Status.dwCurrentState = SERVICE_CONTINUE_PENDING;
	Status.dwWaitHint = ContinueWaitHint;
	Status.dwCheckPoint = 0;
	SetStatus();

	DISPID dispid;
	if (GetDispId(SERVICE_CONTROL_CONTINUE, &dispid))
	{
		HRESULT hr;
		CComPtr<IDispatch> pObject;
		DISPPARAMS parms = {0};
		hr = GetServiceObject(&pObject);
		if (SUCCEEDED(hr))
			hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, 0, 0, 0);
		if (FAILED(hr))
			throw FileLogException(_T("CService::OnContinue"), hr);
	}

	Status.dwCurrentState = SERVICE_RUNNING;
	Status.dwControlsAccepted = ControlsAccepted;
	SetStatus();

	return NO_ERROR;
}

DWORD CService::OnShutdown()
{
	return OnControl(SERVICE_CONTROL_SHUTDOWN);
}

DWORD CService::OnPreShutdown()
{
	return OnControl(SERVICE_CONTROL_PRESHUTDOWN);
}

DWORD CService::OnParamChange()
{
	return OnControl(SERVICE_CONTROL_PARAMCHANGE);
}


DWORD CService::OnDeviceEvent(DWORD dwEventType, LPVOID lpEventData)
{
	DISPID dispid;
	if (GetDispId(SERVICE_CONTROL_DEVICEEVENT, &dispid))
	{
		HRESULT hr;
		CComPtr<IDispatch> pObject;
		CComVariant result;
		DISPPARAMS parms = {0};
		VARIANTARG args[2];
		args[1].vt = VT_I4;
		args[1].lVal = (LONG)lpEventData;
		args[0].vt = VT_I4;
		args[0].lVal = dwEventType;
		parms.cArgs = 2;
		parms.rgvarg = args;

		/*
		switch(dwEventType)
		{
			case DBT_CUSTOMEVENT:
			{
				PDEV_BROADCAST_HDR lpBroadcast = (PDEV_BROADCAST_HDR)lpEventData;
				memcpy(&ServiceParams->DeviceBroadcast, lpBroadcast, lpBroadcast->dbch_size);
				break;
			}

			case DBT_DEVICEQUERYREMOVE:
			case DBT_DEVICEQUERYREMOVEFAILED:
			case DBT_DEVICEREMOVECOMPLETE:
			case DBT_DEVICEREMOVEPENDING:
			case DBT_DEVICETYPESPECIFIC:
			{
				PDEV_BROADCAST_HDR lpBroadcast = (PDEV_BROADCAST_HDR)lpEventData;
				DWORD dwSize;
				switch(lpBroadcast->dbch_devicetype)
				{
					case DBT_DEVTYP_OEM:
						dwSize = sizeof(DEV_BROADCAST_OEM);
						break;
					case DBT_DEVTYP_VOLUME:
						dwSize = sizeof(DEV_BROADCAST_VOLUME);
						break;
					case DBT_DEVTYP_PORT:
						dwSize = sizeof(DEV_BROADCAST_PORT);
						break;
					case DBT_DEVTYP_DEVICEINTERFACE:
						dwSize = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
						break;
					case DBT_DEVTYP_HANDLE:
						dwSize = sizeof(DEV_BROADCAST_HANDLE);
						break;
				}
				memcpy(&ServiceParams->DeviceBroadcast, lpBroadcast, dwSize);
				break;
			}
		}
		*/

		hr = GetServiceObject(&pObject);
		if (SUCCEEDED(hr))
		{
			hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, &result, 0, 0);
			if (SUCCEEDED(hr) && result.vt == VT_I4)
				return result.lVal;
		}
		if (FAILED(hr))
			throw FileLogException(_T("CService::OnDeviceEvent"), hr);
		return NO_ERROR;
	}
	return NO_ERROR;
}

DWORD CService::OnHardwareProfileChange(DWORD dwEventType)
{
	DISPID dispid;
	if (GetDispId(SERVICE_CONTROL_HARDWAREPROFILECHANGE, &dispid))
	{
		HRESULT hr;
		CComPtr<IDispatch> pObject;
		CComVariant result;
		DISPPARAMS parms = {0};
		VARIANTARG args[1];
		args[0].vt = VT_I4;
		args[0].lVal = dwEventType;
		parms.cArgs = 1;
		parms.rgvarg = args;
		hr = GetServiceObject(&pObject);
		if (SUCCEEDED(hr))
		{
			hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, &result, 0, 0);
			if (SUCCEEDED(hr) && result.vt == VT_I4)
				return result.lVal;
		}
		if (FAILED(hr))
			throw FileLogException(_T("CService::OnHardwareProfileChange"), hr);
		return NO_ERROR;
	}
	return NO_ERROR;
}

DWORD CService::OnPowerEvent(DWORD dwEventType, LPVOID lpEventData)
{
	DISPID dispid;
	if (GetDispId(SERVICE_CONTROL_POWEREVENT, &dispid))
	{
		HRESULT hr;
		CComPtr<IDispatch> pObject;
		CComVariant result;
		DISPPARAMS parms = {0};
		VARIANTARG args[3];
		args[2].vt = VT_I4;
		args[2].lVal = dwEventType;

		if (dwEventType == PBT_POWERSETTINGCHANGE)
		{
			PPOWERBROADCAST_SETTING Pbs = (PPOWERBROADCAST_SETTING)lpEventData;
		
			args[1].vt = VT_I4;
			args[0].vt = VT_I4;

			if (InlineIsEqualGUID(Pbs->PowerSetting, GUID_POWERSCHEME_PERSONALITY))
			{
				args[1].lVal = PE_POWERSCHEME_PERSONALITY;

				LPGUID guid = (LPGUID)Pbs->Data;
				if (InlineIsEqualGUID(Pbs->PowerSetting, GUID_MIN_POWER_SAVINGS))
					args[0].lVal = PD_MIN_POWER_SAVINGS;
				else if (InlineIsEqualGUID(Pbs->PowerSetting, GUID_MAX_POWER_SAVINGS))
					args[0].lVal = PD_MAX_POWER_SAVINGS;
				else if (InlineIsEqualGUID(Pbs->PowerSetting, GUID_TYPICAL_POWER_SAVINGS))
					args[0].lVal = PD_TYPICAL_POWER_SAVINGS;
				else
					args[0].lVal = 0;
			}
			else if (InlineIsEqualGUID(Pbs->PowerSetting, GUID_ACDC_POWER_SOURCE))
			{
				args[1].lVal = PE_ACDC_POWER_SOURCE;
				args[0].lVal = *(LONG*)Pbs->Data;
			}
			else if (InlineIsEqualGUID(Pbs->PowerSetting, GUID_BATTERY_PERCENTAGE_REMAINING))
			{
				args[1].lVal = PE_BATTERY_PERCENTAGE_REMAINING;
				args[0].lVal = *(LONG*)Pbs->Data;
			}
			else if (InlineIsEqualGUID(Pbs->PowerSetting, GUID_IDLE_BACKGROUND_TASK))
			{
				args[1].lVal = PE_IDLE_BACKGROUND_TASK;
				args[0].lVal = 0;
			}
			else if (InlineIsEqualGUID(Pbs->PowerSetting, GUID_SYSTEM_AWAYMODE))
			{
				args[1].lVal = PE_SYSTEM_AWAYMODE;
				args[0].lVal = *(LONG*)Pbs->Data;
			}
			else if (InlineIsEqualGUID(Pbs->PowerSetting, GUID_MONITOR_POWER_ON))
			{
				args[1].lVal = PE_MONITOR_POWER_ON;
				args[0].lVal = *(LONG*)Pbs->Data;
			}
			else
			{
				args[1].lVal = 0;
				args[0].lVal = 0;
			}
		}
		else
		{
			AtlTrace(_T("CService::OnPowerEvent - empty\n"));
			args[1].vt = VT_EMPTY;
			args[0].vt = VT_EMPTY;
		}

		parms.cArgs = 3;
		parms.rgvarg = args;
		hr = GetServiceObject(&pObject);
		if (SUCCEEDED(hr))
		{
			hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, &result, 0, 0);
			if (SUCCEEDED(hr) && result.vt == VT_I4)
				return result.lVal;
			if (FAILED(hr))
				throw FileLogException(_T("CService::OnPowerEvent"), hr);
		}
		return NO_ERROR;
	}
	return NO_ERROR;
}

DWORD CService::OnSessionChange(DWORD dwEventType, LPVOID lpEventData)
{
	DISPID dispid;
	if (GetDispId(SERVICE_CONTROL_SESSIONCHANGE, &dispid))
	{
		HRESULT hr;
		PWTSSESSION_NOTIFICATION lpNotification = (PWTSSESSION_NOTIFICATION)lpEventData;
		CComPtr<IDispatch> pObject;
		DISPPARAMS parms = {0};
		VARIANTARG args[2];
		args[1].vt = VT_I4;
		args[1].lVal = dwEventType;
		args[0].vt = VT_I4;
		args[0].lVal = lpNotification->dwSessionId;
		parms.cArgs = 2;
		parms.rgvarg = args;
		hr = GetServiceObject(&pObject);
		if (SUCCEEDED(hr))
			hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, 0, 0, 0);
		if (FAILED(hr))
			throw FileLogException(_T("CService::OnSessionChange"), hr);
		return NO_ERROR;
	}
	return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD CService::OnTimeChange(LPVOID lpEventData)
{
	DISPID dispid;
	if (GetDispId(SERVICE_CONTROL_TIMECHANGE, &dispid))
	{
		HRESULT hr;
		PSERVICE_TIMECHANGE_INFO pTimeInfo = (PSERVICE_TIMECHANGE_INFO)lpEventData;

		if (pTimeInfo->liNewTime.QuadPart == pTimeInfo->liOldTime.QuadPart)
		{
			AtlTrace(_T("VFPSrvHost - CService::OnTimeChange - new and old time equal\n"));
			return NO_ERROR;
		}

		COleDateTime oleNewTime;
		COleDateTime oleOldTime;
		ULARGE_INTEGER newTime;
		ULARGE_INTEGER oldTime;
		FILETIME newFileTime;
		FILETIME oldFileTime;

		newTime.QuadPart = pTimeInfo->liNewTime.QuadPart;
		newFileTime.dwLowDateTime = newTime.LowPart;
		newFileTime.dwHighDateTime = newTime.HighPart;
		oleNewTime = newFileTime;

		oldTime.QuadPart = pTimeInfo->liOldTime.QuadPart;
		oldFileTime.dwLowDateTime = oldTime.LowPart;
		oldFileTime.dwHighDateTime = oldTime.HighPart;
		oleOldTime = oldFileTime;

		CComPtr<IDispatch> pObject;
		DISPPARAMS parms = {0};
		VARIANTARG args[2];
		args[1].vt = VT_DATE;
		args[1].date = oleNewTime;
		args[0].vt = VT_DATE;
		args[0].date = oleOldTime;
		parms.cArgs = 2;
		parms.rgvarg = args;
		hr = GetServiceObject(&pObject);
		if (SUCCEEDED(hr))
			hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, 0, 0, 0);
		if (FAILED(hr))
			throw FileLogException(_T("CService::OnTimeChange"), hr);
		return NO_ERROR;
	}
	return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD CService::OnTriggerEvent()
{
	DISPID dispid;
	if (GetDispId(SERVICE_CONTROL_TRIGGEREVENT, &dispid))
	{
		if (Status.dwCurrentState == SERVICE_STOP_PENDING)
			return ERROR_SHUTDOWN_IN_PROGRESS;

		HRESULT hr;
		CComPtr<IDispatch> pObject;
		DISPPARAMS parms = {0};
		hr = GetServiceObject(&pObject);
		if (SUCCEEDED(hr))
			hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, 0, 0, 0);
		if (FAILED(hr))
			throw FileLogException(_T("CService::TriggerEvent"), hr);
		return NO_ERROR;
	}
	return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD CService::OnCustomCommand(DWORD dwCommand)
{
	DISPID dispid;
	if (GetDispId(SERVICE_CONTROL_CUSTOM, &dispid))
	{
		HRESULT hr;
		CComPtr<IDispatch> pObject;
		DISPPARAMS parms = {0};
		VARIANTARG args[1];
		args[0].vt = VT_I4;
		args[0].lVal = dwCommand;
		parms.cArgs = 1;
		parms.rgvarg = args;
		hr = GetServiceObject(&pObject);
		if (SUCCEEDED(hr))
			hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, 0, 0, 0);
		if (FAILED(hr))
			throw FileLogException(_T("CService::OnCustomCommand"), hr);
	}
	return NO_ERROR;
}

DWORD CService::OnControl(DWORD dwControl)
{
	DISPID dispid;
	if (GetDispId(dwControl, &dispid))
	{
		HRESULT hr;
		CComPtr<IDispatch> pObject;
		DISPPARAMS parms = {0};
		hr = GetServiceObject(&pObject);
		if (SUCCEEDED(hr))
			hr = pObject->Invoke(dispid, IID_NULL, DEFAULT_LCID, DISPATCH_METHOD, &parms, 0, 0, 0);
		if (FAILED(hr))
			throw FileLogException(_T("CService::OnControl"), hr);
		return NO_ERROR;
	}
	return ERROR_CALL_NOT_IMPLEMENTED;
}

void CService::MessageLoop()
{
	MSG msg;
	bool messageloop = true;
	while(messageloop)
	{
		DWORD ret = MsgWaitForMultipleObjects(1, &StopEvent, FALSE, INFINITE, QS_ALLINPUT);
		switch(ret)
		{
			// process has terminated, exit the loop
			case WAIT_OBJECT_0:
				AtlTrace(_T("VFPSrvHost - CService::MessageLoop StopEvent signaled\n"));
				messageloop = false;
				break;

			// we received a message from the process, retrieve it (PeekMessage) and pass it to the window procecure SrvWindowProc (DispatchMessage)
			case WAIT_OBJECT_0 + 1:
				// AtlTrace(_T("CService::MessageLoop Message received\n"));
				while (PeekMessage(&msg,0,0,0,PM_REMOVE))
					DispatchMessage(&msg);
				break;

			case WAIT_FAILED:
				throw FileLogException(_T("MsgWaitForMultipleObjects"), GetLastError());
		}
	}
}

DWORD CService::SetStatus()
{
	if (SetServiceStatus(StatusHandle, &Status))
		return 0;
	else
	{
		DWORD lastError = GetLastError();
		AtlTrace(_T("VFPSrvHost - CService::SetStatus failed: %d"), lastError);
		return lastError;
	}
}

void _stdcall CService::Main(DWORD dwArgc, LPTSTR *lpszArgv)
{
	InterlockedIncrement(&ShutdownFlag);
	CService* Service = 0;
	bool ComInitialized = false;
	try
	{
		HRESULT hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
		if (FAILED(hr))
			throw FileLogException(_T("CoInitializeEx"), hr);
		ComInitialized = true;
		AtlTrace(_T("VFPSrvHost - CService::Main - Service: %s\n"), lpszArgv[0]);

		for (size_t xj = 0; xj < gConfig.Services.GetCount(); xj++)
		{
			if (gConfig.Services[xj]->ServiceName.Compare(lpszArgv[0]) == 0)
			{
				Service = gConfig.Services[xj];
				break;
			}
		}

		if (!Service)
			throw FileLogException(_T("Service not found in configuration file!\n"));

		Service->Initialize();
		Service->RegPowerSettingNotifications();
		Service->Start(dwArgc, lpszArgv);
		Service->MessageLoop();
	}
	catch(BaseException &pExc)
	{
		AtlTrace(_T("VFPSrvHost - CService::Main Error logged to 'vfpsrvhost.log'\n"));
		pExc.Log();
		if (Service)
			Service->Status.dwWin32ExitCode = pExc.LastError();
	}
	catch(CAtlException pExc)
	{
		AtlTrace(_T("VFPSrvHost - CService::Main CAtlException HRESULT: 0x%x\n"), pExc.m_hr);
		if (Service)
			Service->Status.dwWin32ExitCode = pExc.m_hr;
	}
	if (Service)
		Service->Shutdown();
	if (ComInitialized)
		CoUninitialize();
	InterlockedDecrement(&ShutdownFlag);
}

DWORD _stdcall CService::HandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	HRESULT hr;
	CService *Service = gConfig.Services[(int)lpContext];
	DWORD result = NO_ERROR;
	AtlTrace(_T("VFPSrvHost - CService::HandlerEx - Service: %s\tControl: %s\tEventType: %s\n"), Service->ServiceName, ControlCodeToString(dwControl), EventTypeToString(dwControl,dwEventType));

	if (Service->StopEventSignaled)
		return result;

	try
	{
		hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
		if (FAILED(hr))
			throw FileLogException(_T("CoInitializeEx"), hr);

		switch(dwControl)
		{
			case SERVICE_CONTROL_STOP:
				result = Service->OnStop();
				break;

			case SERVICE_CONTROL_PAUSE:
				result = Service->OnPause();
				break;

			case SERVICE_CONTROL_CONTINUE:
				result = Service->OnContinue();
				break;

			case SERVICE_CONTROL_INTERROGATE:
				Service->SetStatus();
				break;

			case SERVICE_CONTROL_SHUTDOWN:
				result = Service->OnShutdown();
				break;

			case SERVICE_CONTROL_PARAMCHANGE:
				result = Service->OnParamChange();
				break;

			case SERVICE_CONTROL_DEVICEEVENT:
				result = Service->OnDeviceEvent(dwEventType, lpEventData);
				break;

			case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
				result = Service->OnHardwareProfileChange(dwEventType);
				break;

			case SERVICE_CONTROL_POWEREVENT:
				result = Service->OnPowerEvent(dwEventType, lpEventData);
				break;

			case SERVICE_CONTROL_SESSIONCHANGE:
				result = Service->OnSessionChange(dwEventType, lpEventData);
				break;

			case SERVICE_CONTROL_PRESHUTDOWN:
				result = Service->OnPreShutdown();
				break;

			case SERVICE_CONTROL_TIMECHANGE:
				result = Service->OnTimeChange(lpEventData);
				break;

			case SERVICE_CONTROL_TRIGGEREVENT:
				result = Service->OnTriggerEvent();
				break;

			default:
				if (dwControl >= 128 && dwControl <= 255)
					result = Service->OnCustomCommand(dwControl);
		}
	}
	catch(BaseException &pExc)
	{
		pExc.Log();
		Service->StopEventSignaled = true;
		SetEvent(Service->StopEvent);
	}

	CoUninitialize();
	return result;
}

void CService::WaitForThreadShutdown()
{
	while (ShutdownFlag > 0) {
		AtlTrace(_T("VFPSrvHost - _tmain - waiting for thread shutdown:\n"));
		Sleep(50);
	}
}

LPTSTR CService::ControlCodeToString(DWORD dwControl)
{
	switch(dwControl)
	{
		case SERVICE_CONTROL_STOP:
			return _T("SERVICE_CONTROL_STOP");
		case SERVICE_CONTROL_PAUSE:
			return _T("SERVICE_CONTROL_PAUSE");
		case SERVICE_CONTROL_CONTINUE:
			return _T("SERVICE_CONTROL_CONTINUE");
		case SERVICE_CONTROL_INTERROGATE:
			return _T("SERVICE_CONTROL_INTERROGATE");
		case SERVICE_CONTROL_SHUTDOWN:
			return _T("SERVICE_CONTROL_SHUTDOWN");
		case SERVICE_CONTROL_PARAMCHANGE:
			return _T("SERVICE_CONTROL_PARAMCHANGE");
		case SERVICE_CONTROL_DEVICEEVENT:
			return _T("SERVICE_CONTROL_DEVICEEVENT");
		case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
			return _T("SERVICE_CONTROL_HARDWAREPROFILECHANGE");
		case SERVICE_CONTROL_POWEREVENT:
			return _T("SERVICE_CONTROL_POWEREVENT");
		case SERVICE_CONTROL_SESSIONCHANGE:
			return _T("SERVICE_CONTROL_SESSIONCHANGE");
		case SERVICE_CONTROL_PRESHUTDOWN:
			return _T("SERVICE_CONTROL_PRESHUTDOWN");
		case SERVICE_CONTROL_TIMECHANGE:
			return _T("SERVICE_CONTROL_TIMECHANGE");
		case SERVICE_CONTROL_TRIGGEREVENT:
			return _T("SERVICE_CONTROL_TRIGGEREVENT");
	}
	return _T("SERVICE_CONTROL_CUSTOM");
}

LPTSTR CService::EventTypeToString(DWORD dwControl, DWORD dwEvent)
{
	switch(dwControl)
	{
		case SERVICE_CONTROL_DEVICEEVENT:
			switch(dwEvent)
			{
				case DBT_DEVICEARRIVAL:
					return _T("DBT_DEVICEARRIVAL");
				case DBT_DEVICEREMOVECOMPLETE:
					return _T("DBT_DEVICEREMOVECOMPLETE");
				case DBT_DEVICEQUERYREMOVE:
					return _T("DBT_DEVICEQUERYREMOVE");
				case DBT_DEVICEQUERYREMOVEFAILED:
					return _T("DBT_DEVICEQUERYREMOVEFAILED");
				case DBT_DEVICEREMOVEPENDING:
					return _T("DBT_DEVICEREMOVEPENDING");
				case DBT_CUSTOMEVENT:
					return _T("DBT_CUSTOMEVENT");
			}
			return _T("Unknown Event");

		case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
			switch (dwEvent)
			{
				case DBT_CONFIGCHANGED:
					return _T("DBT_CONFIGCHANGED");
				case DBT_QUERYCHANGECONFIG:
					return _T("DBT_QUERYCHANGECONFIG");
				case DBT_CONFIGCHANGECANCELED:
					return _T("DBT_CONFIGCHANGECANCELED");
			}
			return _T("Unknown Event");

		case SERVICE_CONTROL_POWEREVENT:
			switch(dwEvent)
			{
				case PBT_APMPOWERSTATUSCHANGE:
					return _T("PBT_APMPOWERSTATUSCHANGE");
				case PBT_APMRESUMEAUTOMATIC:
					return _T("PBT_APMRESUMEAUTOMATIC");
				case PBT_APMRESUMESUSPEND:
					return _T("PBT_APMRESUMESUSPEND");
				case PBT_APMSUSPEND:
					return _T("PBT_APMSUSPEND");
				case PBT_POWERSETTINGCHANGE:
					return _T("PBT_POWERSETTINGCHANGE");
				case PBT_APMBATTERYLOW:
					return _T("PBT_APMBATTERYLOW");
				case PBT_APMOEMEVENT:
					return _T("PBT_APMOEMEVENT");
				case PBT_APMQUERYSUSPEND:
					return _T("PBT_APMQUERYSUSPEND");
				case PBT_APMQUERYSUSPENDFAILED:
					return _T("PBT_APMQUERYSUSPENDFAILED");
				case PBT_APMRESUMECRITICAL:
					return _T("PBT_APMRESUMECRITICAL");
			}
			return _T("Unknown Event");

		case SERVICE_CONTROL_SESSIONCHANGE:
			switch(dwEvent)
			{
				case WTS_CONSOLE_CONNECT:
					return _T("WTS_CONSOLE_CONNECT");
				case WTS_CONSOLE_DISCONNECT:
					return _T("WTS_CONSOLE_DISCONNECT");
				case WTS_REMOTE_CONNECT:
					return _T("WTS_REMOTE_CONNECT");
				case WTS_REMOTE_DISCONNECT:
					return _T("WTS_REMOTE_DISCONNECT");
				case WTS_SESSION_LOGON:
					return _T("WTS_SESSION_LOGON");
				case WTS_SESSION_LOGOFF:
					return _T("WTS_SESSION_LOGOFF");
				case WTS_SESSION_LOCK:
					return _T("WTS_SESSION_LOCK");
				case WTS_SESSION_UNLOCK:
					return _T("WTS_SESSION_UNLOCK");
				case WTS_SESSION_REMOTE_CONTROL:
					return _T("WTS_SESSION_REMOTE_CONTROL");
				case 0xa: // WTS_SESSION_CREATE
					return _T("WTS_SESSION_CREATE");
				case 0xb: // WTS_SESSION_TERMINATE:
					return _T("WTS_SESSION_TERMINATE");
			}
			return _T("Unknown Event");
	}
	return _T("-");
}

void CService::Install()
{
	SC_HANDLE hSCManager = 0;
	SC_HANDLE hService = 0;

	try
	{
		TCHAR szModule[MAX_PATH];
		if (!GetModuleFileName(NULL, szModule, MAX_PATH))
			throw StdoutException(_T("GetModuleFileName"), GetLastError());

		// Get a handle to the SCM database. 
		hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS); 
		if (hSCManager == 0)
			throw StdoutException(_T("OpenSCManager"), GetLastError());

		DWORD dwServiceType = gConfig.Services.GetCount() > 1 ? SERVICE_WIN32_SHARE_PROCESS : SERVICE_WIN32_OWN_PROCESS;
		DWORD dwStartType = StartType.CompareNoCase(_T("Autostart")) == 0 ? SERVICE_AUTO_START : SERVICE_DEMAND_START;
		LPCTSTR lpServiceAccount = ServiceAccount.GetLength() ? (LPCTSTR)ServiceAccount : NULL;
		LPCTSTR lpDependencies = NULL;
		DWORD len = Dependencies.GetLength();
		if (len)
		{
			// replace ; with character 0 and double nullterminate
			Dependencies.GetBuffer(len+1);
			Dependencies.Replace(_T(';'), _T('\0'));
			// gSrvConfig.Dependencies.[len] = _T('\0');
			Dependencies.ReleaseBufferSetLength(len+1);
			lpDependencies = (LPCTSTR)Dependencies;
		}

		// Create the service
		hService = CreateService(hSCManager, ServiceName,DisplayName, SERVICE_ALL_ACCESS, 
			dwServiceType, dwStartType, SERVICE_ERROR_NORMAL, szModule, NULL, NULL, lpDependencies, lpServiceAccount, NULL); 
	 
		if (hService == 0) 
			throw StdoutException(_T("CreateService"), GetLastError());

		wcout << _T("Service '") << DisplayName.GetString() << _T("' installed successfully.") << endl; 
	}
	catch(StdoutException &pExc)
	{
		pExc.Log();
	}

	if (hService)
		CloseServiceHandle(hService); 
	if (hSCManager)
		CloseServiceHandle(hSCManager);
}

void CService::Uninstall()
{
	SC_HANDLE hSCManager = 0;
	SC_HANDLE hService = 0;

	try
	{
		// Get a handle to the SCM database. 
		hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS); 
		if (hSCManager == 0) 
			throw StdoutException(_T("OpenSCManager"), GetLastError());

		hService = OpenService(hSCManager, ServiceName, DELETE);
		if (hService == 0)
			throw StdoutException(_T("OpenService"), GetLastError());

		// Create the service
		if (!DeleteService(hService))
			throw StdoutException(_T("DeleteService"), GetLastError());

		wcout << _T("Service '") << DisplayName.GetString() << _T("' uninstalled successfully.") << endl; 
	}
	catch(StdoutException &pExc)
	{
		pExc.Log();
	}

	if (hService)
		CloseServiceHandle(hService); 
	if (hSCManager)
		CloseServiceHandle(hSCManager);
}