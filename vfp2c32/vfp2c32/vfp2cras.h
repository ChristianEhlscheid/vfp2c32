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
	void SetCallback(char *pCallback);
	void CallbackTxt(DWORD dwEvent, LPTSTR pszText);
	void Callback(DWORD dwEvent, LPVOID pData);
	static void _stdcall RasPhonebookDlgCallbackFunc(DWORD dwCallbackId, DWORD dwEvent, LPTSTR pszText, LPVOID pData);

private:
	CStr m_Callback;
	CStr m_CallbackTxt;
	CStr m_Buffer;
};

class RasDialCallback
{
public:
	RasDialCallback() {};
	~RasDialCallback() {};
	void SetCallback(char *pCallback);
	DWORD Callback(DWORD dwSubEntry, HRASCONN hrasconn, UINT unMsg,
					RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError);
	static DWORD _stdcall RasDialCallbackFunc(DWORD dwCallbackId, DWORD dwSubEntry, HRASCONN hrasconn, UINT unMsg,
											RASCONNSTATE rascs, DWORD dwError, DWORD dwExtendedError);
private:
	CStr m_Callback;
	CStr m_Buffer;
};

class RasNotifyThread : public CThread
{
public:
	RasNotifyThread(CThreadManager &pPool) : CThread(pPool) { }
	~RasNotifyThread() { }

	virtual void SignalThreadAbort();
	virtual DWORD Run();

	bool Setup(HRASCONN hConn, DWORD dwFlags, char *pCallback);

private:
	CStr m_Callback;
	CStr m_Buffer;
	CEvent m_RasEvent;
	CEvent m_AbortEvent;
    HRASCONN m_Conn;
	DWORD m_Flags;
};

#ifdef __cplusplus
extern "C" {
#endif

void _stdcall SaveRas32Error(char *pFunction, DWORD nErrorNo);

bool _stdcall VFP2C_Init_Ras(VFP2CTls& tls);

void _fastcall ARasConnections(ParamBlk *parm);
void _fastcall ARasDevices(ParamBlk *parm);
void _fastcall ARasPhonebookEntries(ParamBlk *parm);
void _fastcall RasPhonebookDlgEx(ParamBlk *parm);
void _fastcall RasDialEx(ParamBlk *parm);
void _fastcall RasHangUpEx(ParamBlk *parm);
void _fastcall RasGetConnectStatusEx(ParamBlk *parm);
void _fastcall RasDialDlgEx(ParamBlk *parm);

void _fastcall RasConnectionNotificationEx(ParamBlk *parm);
void _fastcall AbortRasConnectionNotificationEx(ParamBlk *parm);

void _fastcall RasClearConnectionStatisticsEx(ParamBlk *parm);

#ifdef __cplusplus
}
#endif

#endif // _VFP2CWINDOWS_H__