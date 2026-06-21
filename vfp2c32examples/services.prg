&& service functions

#INCLUDE vfp2c.h

LOCAL lnCount, lnRet, laStatus[1], lnStatus, lnServiceHandle

CD (FULLPATH(JUSTPATH(SYS(16))))
IF TYPE('_WIN64') = 'L' AND _WIN64
SET LIBRARY TO vfp2c64.fll ADDITIVE
ELSE
SET LIBRARY TO vfp2c32.fll ADDITIVE
ENDIF

LOCAL laServiceStatus[7]
laServiceStatus[1] = 'STOPPED'
laServiceStatus[2] = 'START PENDING'
laServiceStatus[3] = 'STOP PENDING'
laServiceStatus[4] = 'RUNNING'
laServiceStatus[5] = 'CONTINUE PENDING'
laServiceStatus[6] = 'PAUSE PENDING'
laServiceStatus[7] = 'PAUSED'

*!*	lnServiceHandle = OpenServiceEx('Dhcp')
*!*	IF AServiceStatus('laStatus',lnServiceHandle)
*!*		? "Service is in : " + laServiceStatus[laStatus[3]] + " state"
*!*	ELSE
*!*		AERROREX('laError')
*!*		DISPLAY MEMORY LIKE laError
*!*	ENDIF

*!*	CloseServiceHandleEx(lnServiceHandle)

lnCount = ASERVICES("laServs")
IF lnCount > 0
?"Service name", laServs[1,1]
?"Display name", laServs[1,2]
?"Servicetype", laServs[1,3]
?"current state", laServs[1,4]
?"Win32 exitcode", laServs[1,5]
?"Service specific exitcode", laServs[1,6]
?"Checkpoint", laServs[1,7]
?"ControlsAccepted", laServs[1,8]
&& the next two values are not available on Windows NT and will always be 0
?"Service flags", laServs[1,9]
?"Process ID", laServs[1,10]
&& for more info to the above service data have a look at
&& http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dllproc/base/service_status_process_str.asp
ENDIF

&& enumerate services of another maching
&& lnCount = ASERVICES("laServs","yourServer")
&& enumerate all running services of local machine

&& enumerate only running services
lnCount = ASERVICES("laServs","","",SERVICE_ACTIVE)
&& enumerate only non running services
lnCount = ASERVICES("laServs","","",SERVICE_INACTIVE)


&& starting a service & wait up to default seconds specified by the service for complete initialization
*!*	lnRet = STARTSERVICEEX('MSSQLSERVER')

&& starting a service and passing arguments to it
*!*	LOCAL laArgs[2]
*!*	laArgs[1] = 'service parameter 1'
*!*	laArgs[2] = 'service parameter 2'
*!*	lnRet = STARTSERVICEEX('MSSQLSERVER',@laArgs)

&& starting a service with no parameters & wait up to 5 seconds for complete initialization
lnRet = STARTSERVICEEX("MSSQL$SQLEXPRESS",NULL,5)
DO CASE
	CASE lnRet = 1
		? "Service started & running"
	CASE lnRet = 0
		? "Service started & still initializing"
ENDCASE

&& by using a timeout of 0 you can start a service and don't care about initialization state
*!*	lnRet = STARTSERVICEEX("MSSQLSERVER",NULL,0)

&& stopping a service, basic
*!*	lnRet = STOPSERVICEEX("MSSQLSERVER")
*!*	DO CASE
*!*		CASE lnRet = 1
*!*			? "Service completly stopped"
*!*		CASE lnRet = 0
*!*			? "Service accepted stop request but is still shutting down"
*!*	ENDCASE

&& stop a service which has dependant services running .. (shuts down dependant services first)
*!*	lnRet = STOPSERVICEEX('MSSQLSERVER',NULL,.T.)

&& stopping the service on another machine
*!*	lnRet = STOPSERVICEEX('MSSQLSERVER',NULL,.F.,'yourServer')

DEFINE CLASS oService AS Custom
	
	cServiceName = "MSSQLSERVER"
	cServer = ""
	cDatabase = ""
	nHandle = 0
	
	FUNCTION Init
		THIS.nHandle = OpenServiceEx(THIS.cServiceName,0,THIS.cServer,THIS.cDatabase)
	ENDFUNC

	FUNCTION Destroy
		IF THIS.nHandle != 0
			CloseServiceHandleEx(THIS.nHandle)	
		ENDIF
	ENDFUNC
	
	FUNCTION Start(laArgs)
		EXTERNAL ARRAY laArgs
		IF PCOUNT() = 1
			RETURN STARTSERVICEEX(THIS.nHandle,@laArgs)		
		ELSE
			RETURN STARTSERVICEEX(THIS.nHandle)
		ENDIF
	ENDFUNC

	FUNCTION Stop(nTimeout)
		IF PCOUNT() == 1
			RETURN STOPSERVICEEX(THIS.nHandle,nTimeout)
		ELSE
			RETURN STOPSERVICEEX(THIS.nHandle)
		ENDIF
	ENDFUNC

	FUNCTION AStatus(laStatus)	
		EXTERNAL ARRAY laStatus
		RETURN AServiceStatus('laStatus',THIS.nHandle)
	ENDFUNC

ENDDEFINE