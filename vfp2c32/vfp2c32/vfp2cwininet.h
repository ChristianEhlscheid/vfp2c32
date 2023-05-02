#ifndef _VFP2CWININET_H__
#define _VFP2CWININET_H__

#include "wininet.h"
#include "vfp2chelpers.h"

class FTPDownloadThread : public CThread
{
public:
	FTPDownloadThread(CThreadManager &pPool) : CThread(pPool) { }
	~FTPDownloadThread() {}

	virtual void StartThread();
	virtual void Abort(int nAbortFlag);
	virtual bool IsShutdown();
};

bool _stdcall VFP2C_Init_WinInet();
void _stdcall VFP2C_Destroy_WinInet();

/* WinInet FTP/HTTP wrappers & related functions */
void _fastcall InitWinInet(ParamBlkEx& parm);
void _fastcall SetWinInetOptions(ParamBlkEx& parm);
void _fastcall FTPConnect(ParamBlkEx& parm);
void _fastcall FTPDisconnect(ParamBlkEx& parm);
void _fastcall FTPGetFileLib(ParamBlkEx& parm);
void _fastcall FTPPutFileLib(ParamBlkEx& parm);
void _fastcall FTPGetDirectory(ParamBlkEx& parm);
void _fastcall FTPSetDirectory(ParamBlkEx& parm);
void _fastcall AFTPFiles(ParamBlkEx& parm);
void _fastcall HTTPGetFile(ParamBlkEx& parm);

#endif // _VFP2CWININET_H__