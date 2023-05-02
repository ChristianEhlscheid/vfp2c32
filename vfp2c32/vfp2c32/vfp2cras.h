#ifndef _VFP2CRAS_H__
#define _VFP2CRAS_H__

#include <ras.h>
#include <raserror.h>
#include <rasdlg.h>

#include "vfp2chelpers.h"
#include "vfp2casync.h"

typedef DWORD (_stdcall *PRASGETERRORSTRING)(UINT, LPTSTR, DWORD); // RasGetErrorString
typedef DWORD (_stdcall *PRASENUMCONNECTIONS)(LPRASCONN, LPDWORD, LPDWORD); // RasEnumConnections
typedef DWORD (_stdcall *PRASGETPROJECTIONINFO)(HRASCONN, RASPROJECTION, LPVOID, LPDWORD); // RasGetProjectionInfo
typedef DWORD (_stdcall *PRASENUMDEVICES)(LPRASDEVINFO, LPDWORD, LPDWORD); // RasEnumDevices
typedef DWORD (_stdcall *PRASENUMENTRIES)(LPCSTR, LPCSTR, LPRASENTRYNAME, LPDWORD, LPDWORD); // RasEnumEntries
typedef BOOL (_stdcall *PRASPHONEBOOKDLG)(LPSTR, LPSTR, LPRASPBDLG); // RasPhonebookDlg
typedef DWORD (_stdcall *PRASDIAL)(LPRASDIALEXTENSIONS, LPCSTR, LPRASDIALPARAMS, DWORD,
								   LPVOID lpvNotifier, LPHRASCONN); // RasDial
typedef DWORD (_stdcall *PRASHANGUP)(HRASCONN); // RasHangUp
typedef DWORD (_stdcall *PRASGETENTRYDIALPARAMS)(LPCSTR, LPRASDIALPARAMS, LPBOOL); // RasGetEntryDialParams
typedef DWORD (_stdcall *PRASGETEAPUSERIDENTITY)(LPCSTR pszPhonebook, LPCSTR pszEntry, DWORD dwFlags,
                                                 HWND hwnd, RASEAPUSERIDENTITY** ppRasEapUserIdentity); // RasGetEapUserIdentity
typedef void (_stdcall *PRASFREEEAPUSERIDENTITY)(RASEAPUSERIDENTITY*); // RasFreeEapUserIdentity
typedef DWORD (_stdcall *PRASGETCONNECTSTATUS)(HRASCONN, LPRASCONNSTATUS); // RasGetConnectStatus
typedef DWORD (_stdcall *PRASCONNECTIONNOTIFICATION)(HRASCONN, HANDLE, DWORD); // RasConnectionNotification
typedef BOOL (_stdcall *PRASDIALDLG)(LPSTR, LPSTR, LPSTR, LPRASDIALDLG); //RasDialDlg
typedef DWORD (_stdcall *PRASCLEARCONNECTIONSTATISTICS)(HRASCONN); // RasClearConnectionStatistics

class RasPhonebookDlgCallback
{
public:
	RasPhonebookDlgCallback() {};
	~RasPhonebookDlgCallback() {};
	void SetCallback(CStringView pCallback);
	void CallbackTxt(DWORD dwEvent, LPTSTR pszText);
	void Callback(DWORD dwEvent, LPVOID pData);
	static void _stdcall RasPhonebookDlgCallbackFunc(ULONG_PTR dwCallbackId, DWORD dwEvent, LPTSTR pszText, LPVOID pData);

private:
	CFoxCallback m_Callback;
};

class RasDialCallback
{
public:
	RasDialCallback() {};
	~RasDialCallback() {};
	void SetCallback(CStringView pCallback);
	DWORD Callback(DWORD dwSubEntry, HRASCONN hrasconn, UINT unMsg,
					RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError);
	static DWORD _stdcall RasDialCallbackFunc(ULONG_PTR dwCallbackId, DWORD dwSubEntry, HRASCONN hrasconn, UINT unMsg,
											RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError);
private:
	CFoxCallback m_Callback;
};

class RasNotifyThread : public CThread
{
public:
	RasNotifyThread(CThreadManager &pPool) : CThread(pPool), m_Conn(0), m_Flags(0) { }
	~RasNotifyThread() { }

	virtual void SignalThreadAbort();
	virtual DWORD Run();
	virtual void Release();

	bool Setup(HRASCONN hConn, DWORD dwFlags, FoxString &pCallback);

private:
	CEvent m_RasEvent;
	CEvent m_AbortEvent;
    HRASCONN m_Conn;
	DWORD m_Flags;
	CFoxCallback m_Callback;
};

#ifdef __cplusplus
extern "C" {
#endif

void _stdcall SaveRas32Error(char *pFunction, DWORD nErrorNo);

bool _stdcall VFP2C_Init_Ras(VFP2CTls& tls);

void _fastcall ARasConnections(ParamBlkEx& parm);
void _fastcall ARasDevices(ParamBlkEx& parm);
void _fastcall ARasPhonebookEntries(ParamBlkEx& parm);
void _fastcall RasPhonebookDlgEx(ParamBlkEx& parm);
void _fastcall RasDialEx(ParamBlkEx& parm);
void _fastcall RasHangUpEx(ParamBlkEx& parm);
void _fastcall RasGetConnectStatusEx(ParamBlkEx& parm);
void _fastcall RasDialDlgEx(ParamBlkEx& parm);

void _fastcall RasConnectionNotificationEx(ParamBlkEx& parm);
void _fastcall AbortRasConnectionNotificationEx(ParamBlkEx& parm);

void _fastcall RasClearConnectionStatisticsEx(ParamBlkEx& parm);

#ifdef __cplusplus
}
#endif

#endif // _VFP2CWINDOWS_H__