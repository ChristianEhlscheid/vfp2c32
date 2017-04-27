#ifndef _VFPSRVHOST_H__
#define _VFPSRVHOST_H__

#pragma once

#include <windows.h>
#include <dbt.h>
#include <stdlib.h>
#include <wchar.h>

#define _ATL_ALL_WARNINGS 
#include <atlbase.h> 
#include <atlsafe.h>
#include <atlstr.h>
#include <atlcoll.h>
#include <atlcomtime.h>

typedef HPOWERNOTIFY (_stdcall *PREGISTERPOWERSETTINGNOTIFICATION)(HANDLE, LPCGUID, DWORD);
typedef BOOL (_stdcall *PUNREGISTERPOWERSETTINGNOTIFICATION)(HPOWERNOTIFY);

#define DEFAULT_LCID MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)

class CControlToDispEntry
{
public:
	CControlToDispEntry() : Name(0), ControlCode(0), DispId(DISPID_UNKNOWN) { }
	CControlToDispEntry(LPCTSTR pName, DWORD ncontrolcode, DISPID ndispid) : Name(pName), ControlCode(ncontrolcode), DispId(ndispid) { }

	LPCTSTR Name;
	DWORD ControlCode;
	DISPID DispId;
};

class CService : public IDispatch
{
public:
	CService();
	friend class SrvHostConfig;
	friend int SrvHostProcessCommandLine(int argc, _TCHAR* argv[]);

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IDispatch methods
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo);
	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, 
		DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

	LPTSTR Name() { return (LPTSTR)ServiceName.GetString(); }

	static void _stdcall Main(DWORD dwArgc, LPTSTR *lpszArgv);
	static DWORD _stdcall HandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
	static void WaitForThreadShutdown();

private:

	// scriptable methods
	HRESULT Stop();
	HRESULT RequestAdditionalTime(VARIANT *Milliseconds);

	// internal methods
	void Install();
	void Uninstall();
	void Initialize();
	void RegPowerSettingNotifications();
	void Shutdown();
	void MessageLoop();
	bool GetDispId(DWORD dwControlCode, DISPID* DispId = 0);
	DWORD SetStatus();
	HRESULT GetServiceObject(IDispatch **pObject);
	static LPTSTR ControlCodeToString(DWORD dwControl);
	static LPTSTR EventTypeToString(DWORD dwControl, DWORD dwEvent);

	void Start(DWORD dwArgc, LPTSTR *lpszArgv);
	DWORD OnStop();
	DWORD OnPause();
	DWORD OnContinue();
	DWORD OnShutdown();
	DWORD OnPreShutdown();
	DWORD OnParamChange();
	DWORD OnDeviceEvent(DWORD dwEventType, LPVOID lpEventData);
	DWORD OnHardwareProfileChange(DWORD dwEventType);
	DWORD OnPowerEvent(DWORD dwEventType, LPVOID lpEventData);
	DWORD OnSessionChange(DWORD dwEventType, LPVOID lpEventData);
	DWORD OnTimeChange(LPVOID lpEventData);
	DWORD OnTriggerEvent();
	DWORD OnCustomCommand(DWORD dwCommand);
	DWORD OnControl(DWORD dwControl);

	CString DisplayName;
	CString ServiceName;
	CString StartType;
	CString ServiceAccount;
	CString ComClass;
	CString Dependencies;
	CString PowerNotifications;
	DWORD StopWaitHint;
	DWORD PauseWaitHint;
	DWORD ContinueWaitHint;
	DWORD ServiceId;
	SERVICE_STATUS Status;
	SERVICE_STATUS_HANDLE StatusHandle;
	DWORD ControlsAccepted;
	IDispatch* ServiceObject;
	IGlobalInterfaceTable* GIT;
	DWORD ServiceObjectCookie;
	HANDLE StopEvent;
	CAtlArray<CControlToDispEntry> DispatchMap;
	CAtlArray<HPOWERNOTIFY> PowerNotifyHandles;
	bool StopEventSignaled;
	static volatile long ShutdownFlag;

	// custom control codes for service object
	static const DWORD SERVICE_CONTROL_START	= -1;
	static const DWORD SERVICE_CONTROL_CUSTOM	= -2;
	static const DWORD SERVICE_EXITCODE			= -3;

	// dispid's for the scriptable part of this object
	static const DISPID DISPID_Stop				= 1;
	static const DISPID DISPID_RequestAdditionalTime = 2;

	static const LONG PE_POWERSCHEME_PERSONALITY		= 1;
	static const LONG PE_ACDC_POWER_SOURCE				= 2;
	static const LONG PE_BATTERY_PERCENTAGE_REMAINING	= 3;
	static const LONG PE_IDLE_BACKGROUND_TASK			= 4;
	static const LONG PE_SYSTEM_AWAYMODE				= 5;
	static const LONG PE_MONITOR_POWER_ON				= 6;

	static const LONG PD_MIN_POWER_SAVINGS				= 1;
	static const LONG PD_MAX_POWER_SAVINGS				= 2;
	static const LONG PD_TYPICAL_POWER_SAVINGS			= 3;

	static PREGISTERPOWERSETTINGNOTIFICATION pRegisterPowerSettingNotification;
	static PUNREGISTERPOWERSETTINGNOTIFICATION pUnregisterPowerSettingNotification;
};

#endif _VFPSRVHOST_H__

