#ifndef _VFP2CSERVICES_H__
#define _VFP2CSERVICES_H__

#include "vfp2chelpers.h"

// EnumDependentServices
const int SERVICE_ENUM_BUFFER		= 8192;
const int SERVICE_MAX_ENUM_BUFFER	= 32768;
const int SERVICE_INFINITE_TIMEOUT	= -1;
const int SERVICE_DEFAULT_TIMEOUT	= -2;

class ServiceManager
{
public:
	ServiceManager() : m_Handle(NULL) {}
	ServiceManager(const char *pMachine, const char *pDatabase, DWORD dwAccess = 0);
	~ServiceManager();

	void Open(const char *pMachine, const char *pDatabase, DWORD dwAccess = 0);

	operator SC_HANDLE() { return m_Handle; }
	operator LPSC_HANDLE() { return &m_Handle; }

private:
	SC_HANDLE m_Handle;
};

class Service
{
public:
	Service() : m_Handle(NULL), m_Owner(true) {}
	Service(SC_HANDLE hService) { m_Handle = hService; m_Owner = false; }
	Service(SC_HANDLE hSCM, const char* pServiceName, DWORD dwAccess);
	~Service();

	void Open(SC_HANDLE hSCM, const char* pServiceName, DWORD dwAccess = 0);
	int Stop(bool bStopDependencies, int nTimeout, SC_HANDLE hSCM);
	int Start(DWORD nNumberOfArgs, LPCSTR *pArgs, int nTimeout);
	int Pause(int nTimeout);
	int Continue(int nTimeout);
	int Control(DWORD nControlCode);
	void QueryStatus(LPSERVICE_STATUS pStatus);
	void QueryConfig(CBuffer &pBuffer);
	void StopDependantServices(SC_HANDLE hSCM);
	Service& Attach(ValueEx& pVal);
	SC_HANDLE Detach() { m_Owner = false; return m_Handle; }

	operator SC_HANDLE() { return m_Handle; }
	operator LPSC_HANDLE() { return &m_Handle; }
	bool operator!() { return m_Handle == NULL; }
	operator bool() { return m_Handle != NULL; }
	int WaitForServiceStatus(DWORD dwState, int nTimeout);

private:
	SC_HANDLE m_Handle;
	bool m_Owner;
	SERVICE_STATUS m_Status;
};

#ifdef __cplusplus
extern "C" {
#endif

void _fastcall OpenServiceLib(ParamBlkEx& parm);
void _fastcall CloseServiceHandleLib(ParamBlkEx& parm);
void _fastcall StartServiceLib(ParamBlkEx& parm);
void _fastcall StopServiceLib(ParamBlkEx& parm);
void _fastcall PauseService(ParamBlkEx& parm);
void _fastcall ContinueService(ParamBlkEx& parm);
void _fastcall ControlServiceLib(ParamBlkEx& parm);
void _fastcall AServices(ParamBlkEx& parm);
void _fastcall AServiceStatus(ParamBlkEx& parm);
void _fastcall AServiceConfig(ParamBlkEx& parm);
void _fastcall ADependentServices(ParamBlkEx& parm);
void _fastcall WaitForServiceStatus(ParamBlkEx& parm);
void _fastcall CreateServiceLib(ParamBlkEx& parm);
void _fastcall DeleteServiceLib(ParamBlkEx& parm);

#ifdef __cplusplus
}
#endif

#endif _VFP2CSERVICES_H__