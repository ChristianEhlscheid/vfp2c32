&& Async functions 

#INCLUDE "vfp2c.h"

CD (FULLPATH(JUSTPATH(SYS(16))))
IF TYPE('_WIN64') = 'L' AND _WIN64
SET LIBRARY TO vfp2c64d.fll ADDITIVE
ELSE
SET LIBRARY TO vfp2c32d.fll ADDITIVE
ENDIF

&& monitor a directory for changes
LOCAL lcDir
&& m.lcDir = GETDIR('', '', 'Select directory to watch for changes')
m.lcDir = 'C:\temp'
*!*	m.lcDir = GETENV("TMP")
*!*	m.lcDir = 'C:\'
IF !EMPTY(m.lcDir)
	PUBLIC loFileMonitor
	loFileMonitor = CREATEOBJECT('FileSystemWatcherEx')
	&& 
	loFileMonitor.Watch(m.lcDir, .T., FILE_NOTIFY_CHANGE_FILE_NAME + FILE_NOTIFY_CHANGE_DIR_NAME)
ENDIF
	
*!*	PUBLIC loRegMonitor
*!*	loRegMonitor = CREATEOBJECT('RegistryKeyWatcher')

*!*	TRY
*!*		STORE 0 TO lnKey, lnNewKey
*!*		&& one can only create direct subkey's of already open keys
*!*		lnKey = OPENREGISTRYKEY(HKEY_LOCAL_MACHINE,'SOFTWARE')
*!*		lnNewKey = CREATEREGISTRYKEY(lnKey,'YourFirmName') && create HKEY_LOCAL_MACHINE\SOFTWARE\YourFirmName

*!*		loRegMonitor.Watch(HKEY_LOCAL_MACHINE, 'Software\YourFirmName', .T., REG_NOTIFY_CHANGE_NAME + REG_NOTIFY_CHANGE_LAST_SET)

*!*		WRITEREGISTRYKEY(lnNewKey,'Hello') && write Hello to the standard value
*!*		WRITEREGISTRYKEY(lnNewKey,25,'SomeNumber')
*!*		WRITEREGISTRYKEY(lnNewKey,34.345,'SomeFractional')
*!*		WRITEREGISTRYKEY(lnNewKey,DATE(),'InstallDate')
*!*		WRITEREGISTRYKEY(lnNewKey,DATETIME(),'InstallTime')
*!*		WRITEREGISTRYKEY(lnNewKey,'A Multi_SZ_Value'+CHR(0)+'Second Value','SomeValue','',REG_MULTI_SZ)
*!*		WRITEREGISTRYKEY(lnNewKey,'somebinary_variable_here_aasdfasdfkdjfkajdkfjalkjdfjadksfjlkajsdfj','SomeValue2','',REG_BINARY)
*!*		WRITEREGISTRYKEY(lnNewKey,DATETIME(),'InstallTime 2')

*!*	CATCH TO loExp
*!*		AERROREX('laError')
*!*		DISPLAY MEMORY LIKE laError
*!*	FINALLY 
*!*		IF lnNewKey != 0
*!*			CLOSEREGISTRYKEY(lnNewKey)
*!*		ENDIF
*!*		IF lnKey != 0
*!*			CLOSEREGISTRYKEY(lnKey)
*!*		ENDIF
*!*	ENDTRY

DEFINE CLASS FoxCallbackTest AS Custom

	nCalls = 0

	FUNCTION Test
		LPARAMETERS lbComCallback
		THIS.nCalls = 0		
		IF m.lbComCallback
			RETURN CallbackTest("CallbackFunc", THIS)
		ELSE
			RETURN CallbackTest("THIS.CallbackFunc")
		ENDIF
	ENDFUNC

	FUNCTION CallbackFunc
		LPARAMETERS xj, cString
		IF xj = 1
			? xj, "" + cString
		ENDIF
		THIS.nCalls = THIS.nCalls + 1 
	ENDFUNC

ENDDEFINE

DEFINE CLASS AsyncWatcher AS Custom

	PROTECTED hHandle
	PROTECTED cPublicName
	hHandle = 0
	cPublicName = ''

	FUNCTION Init
		THIS.cPublicName = THIS.Class + SYS(2015)
		CreatePublicShadowObjReference(THIS.cPublicName, THIS)
	ENDFUNC

	FUNCTION Destroy
		THIS.Stop()
		ReleasePublicShadowObjReference(THIS.cPublicName)
	ENDFUNC

	FUNCTION Stop

	ENDFUNC

	FUNCTION Watch
	
	ENDFUNC

ENDDEFINE


DEFINE CLASS FileSystemWatcher AS AsyncWatcher

	FUNCTION Stop
		IF THIS.hHandle != 0
			CancelFileChange(THIS.hHandle)
			THIS.hHandle = 0
		ENDIF
	ENDFUNC
	
	FUNCTION Watch
		LPARAMETERS cPath, bWatchSubtree, nFilter
		THIS.Stop()
		THIS.hHandle = FindFileChange(cPath, bWatchSubtree, nFilter, THIS.cPublicName + '.FolderChanged')
	ENDFUNC
	
	&& callback function that gets called in asyncronous mode
	&& it's up to you to implement it in a reasable manner
	FUNCTION FolderChanged
		LPARAMETERS hHandle, cPath, nError

		IF nError = 0
			? cPath + ' changed'
		ELSE
			? 'Function: ' + cPath + " failed. ErrorNo. ", nError
			THIS.hHandle = 0
		ENDIF		
	ENDFUNC
	
ENDDEFINE

DEFINE CLASS FileSystemWatcherEx AS Custom

	nCounter = 0

	FUNCTION Destroy
		THIS.Stop()
	ENDFUNC

	FUNCTION Stop
		LPARAMETERS cPath
		DO CASE
			CASE VARTYPE(m.cPath) = 'L'
				CancelFileChangeEx(0)
			CASE INLIST(VARTYPE(m.cPath), 'N', 'C')
				CancelFileChangeEx(m.cPath)
		ENDCASE
	ENDFUNC
	
	FUNCTION Watch
		LPARAMETERS cPath, bWatchSubtree, nFilter
		LOCAL lnHandle
		m.lnHandle = FindFileChangeEx(m.cPath, m.bWatchSubtree, m.nFilter, 'FolderChanged', THIS)
	ENDFUNC
	
	&& callback function that gets called in asyncronous mode
	&& it's up to you to implement it in a reasable manner
	FUNCTION FolderChanged
		LPARAMETERS hHandle, nReason, cPath, cPath2
		THIS.nCounter = THIS.nCounter + 1 
		DO CASE
			CASE m.nReason = 1
				? 'File added: ' + m.cPath
			CASE m.nReason = 2
				? 'File removed: ' + m.cPath
			CASE m.nReason = 3
				? 'File modified: ' + m.cPath
			CASE m.nReason = 4
				? 'File renamed from: ' + m.cPath + CHR(13) + CHR(10) + ' to: ' +  m.cPath2
			CASE m.nReason = 0
				? 'Error watching path: ' + m.cPath + " - ErrorNo: " + m.cPath2
				&& error is either GetLastError from ReadDirectoryChangesW or E_INSUFEMORY in an out of memory situation
		ENDCASE
		&& THIS.Stop(m.hHandle)
	ENDFUNC
	
ENDDEFINE

DEFINE CLASS RegistryKeyWatcher AS AsyncWatcher

	FUNCTION Stop
		IF THIS.hHandle != 0
			CancelRegistryChange(THIS.hHandle)
			THIS.hHandle = 0
		ENDIF
	ENDFUNC
	
	FUNCTION Watch
		LPARAMETERS hRoot, cKey, bWatchSubtree, nFilter
		THIS.Stop()
		THIS.hHandle = FindRegistryChange(m.hRoot, m.cKey, m.bWatchSubtree, m.nFilter, THIS.cPublicName + '.RegistryKeyChanged')
	ENDFUNC
	
	&& callback function that gets called in asyncronous mode
	&& it's up to you to implement it in a reasable manner
	FUNCTION RegistryKeyChanged
		LPARAMETERS hHandle, nError

		IF nError = 0
			? 'RegistryKey changed'
		ELSE
			? 'API Function failed - ErrorNo. ', nError
			THIS.hHandle = 0
		ENDIF		
	ENDFUNC
	
ENDDEFINE

DEFINE CLASS ObjectWatcher AS AsyncWatcher

	FUNCTION Stop
		IF THIS.hHandle != 0
			CancelWaitForObject(THIS.hHandle)
			THIS.hHandle = 0
		ENDIF
	ENDFUNC
	
	FUNCTION Watch
		LPARAMETERS hObject
		THIS.Stop()
		THIS.hHandle = AsyncWaitForObject(m.hObject, THIS.cPublicName + '.ObjectSignaled')
	ENDFUNC
	
	&& callback function that gets called in asyncronous mode
	&& it's up to you to implement it in a reasable manner
	FUNCTION ObjectSignaled
		LPARAMETERS nError
		IF nError = 0
			? 'Object signaled'
		ELSE
			? 'Function: WaitForMultipleObjects failed with error', nError
		ENDIF		
		THIS.hHandle = 0		
	ENDFUNC
	
ENDDEFINE