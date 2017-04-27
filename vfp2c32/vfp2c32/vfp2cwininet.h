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
void _fastcall InitWinInet(ParamBlk *parm);
void _fastcall SetWinInetOptions(ParamBlk *parm);
void _fastcall FTPConnect(ParamBlk *parm);
void _fastcall FTPDisconnect(ParamBlk *parm);
void _fastcall FTPGetFileLib(ParamBlk *parm);
void _fastcall FTPPutFileLib(ParamBlk *parm);
void _fastcall FTPGetDirectory(ParamBlk *parm);
void _fastcall FTPSetDirectory(ParamBlk *parm);
void _fastcall AFTPFiles(ParamBlk *parm);
void _fastcall HTTPGetFile(ParamBlk *parm);

#endif // _VFP2CWININET_H__