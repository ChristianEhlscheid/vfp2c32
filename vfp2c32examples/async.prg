&& Async functions 
&& prerequisites: InitVFP2C32 must have been called with the VFP2C_INIT_ASYNC flag set 

#INCLUDE "vfp2c.h"

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_ASYNC)

&& monitor a directory for changes
LOCAL lcDir
m.lcDir = GETDIR('', '', 'Select directory to watch for changes')
IF !EMPTY(m.lcDir)
	PUBLIC loFileMonitor
	loFileMonitor = CREATEOBJECT('FileSystemWatcher')
	&& 
	loFileMonitor.Watch(m.lcDir, .T., FILE_NOTIFY_CHANGE_FILE_NAME + FILE_NOTIFY_CHANGE_DIR_NAME + FILE_NOTIFY_CHANGE_ATTRIBUTES + FILE_NOTIFY_CHANGE_SIZE)
ENDIF
	
PUBLIC loRegMonitor
loRegMonitor = CREATEOBJECT('RegistryKeyWatcher')

TRY
	STORE 0 TO lnKey, lnNewKey
	&& one can only create direct subkey's of already open keys
	lnKey = OPENREGISTRYKEY(HKEY_LOCAL_MACHINE,'SOFTWARE')
	lnNewKey = CREATEREGISTRYKEY(lnKey,'YourFirmName') && create HKEY_LOCAL_MACHINE\SOFTWARE\YourFirmName

	loRegMonitor.Watch(HKEY_LOCAL_MACHINE, 'Software\YourFirmName', .T., REG_NOTIFY_CHANGE_NAME + REG_NOTIFY_CHANGE_LAST_SET)

	WRITEREGISTRYKEY(lnNewKey,'Hello') && write Hello to the standard value
	WRITEREGISTRYKEY(lnNewKey,25,'SomeNumber')
	WRITEREGISTRYKEY(lnNewKey,34.345,'SomeFractional')
	WRITEREGISTRYKEY(lnNewKey,DATE(),'InstallDate')
	WRITEREGISTRYKEY(lnNewKey,DATETIME(),'InstallTime')
	WRITEREGISTRYKEY(lnNewKey,'A Multi_SZ_Value'+CHR(0)+'Second Value','SomeValue','',REG_MULTI_SZ)
	WRITEREGISTRYKEY(lnNewKey,'somebinary_variable_here_aasdfasdfkdjfkajdkfjalkjdfjadksfjlkajsdfj','SomeValue2','',REG_BINARY)
	WRITEREGISTRYKEY(lnNewKey,DATETIME(),'InstallTime 2')

CATCH TO loExp
	AERROREX('laError')
	DISPLAY MEMORY LIKE laError
FINALLY 
	IF lnNewKey != 0
		CLOSEREGISTRYKEY(lnNewKey)
	ENDIF
	IF lnKey != 0
		CLOSEREGISTRYKEY(lnKey)
	ENDIF
ENDTRY

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