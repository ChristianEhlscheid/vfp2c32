&& enumeration functions
#include vfp2c.h

&& prerequisites for all functions in this file:
&& InitVFP2C32 must have been called with the VFP2C_INIT_ENUM flag set
CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE
InitVFP2C32( VFP2C_INIT_ENUM)

&& enumerate running processes into an array
&& on WinNT process name, parent process id & priority class can be empty (0) 
&& if the current user account doesn't has suffiecient rights to query process information
LOCAL lnCount
lnCount = AProcesses("laProcs")
IF lnCount > 0
? "Process name", laProcs[1,1]
? "Process ID", laProcs[1,2]
? "Parent process ID", laProcs[1,3]
? "Number of threads in process", laProcs[1,4] && on WinNT this is always 0 (not available)
? "Priority class", laProcs[1,5]
ENDIF

&& enumerate threads of a process
LOCAL laThreads[1]
lnCount = APROCESSTHREADS("laThreads",_VFP.ProcessId)
IF lnCount > 0
? "Thread ID", laThreads[1,1]		
? "Owner Process ID", laThreads[1,2]
? "Thread base priority", laThreads[1,3]
ENDIF
&& enumerate threads of all processes
&& lnCount = APROCESSTHREADS("laThreads",0)

&& enumerate all DLL's (modules) loaded by a process
LOCAL laMods[1]
lnCount = AProcessModules("laMods",_VFP.ProcessId)
IF lnCount > 0
? "Dll name", laMods[1,1]
? "Dll path", laMods[1,2]
? "Dll handle (HMODULE)", laMods[1,3]
? "Dll size in bytes", laMods[1,4]
? "Dll base address", laMods[1,5]
ENDIF

&& enumerate all heaps owned by a process
LOCAL laHeaps[1]
lnCount = AProcessHeaps("laHeaps",_VFP.ProcessId)
IF lnCount > 0
? "Heap ID", laHeaps[1,1]
? "Heap flags", laHeaps[1,2]
ENDIF

LOCAL laWinst[1]
&& enumerate windowstations
lnCount = AWindowStations("laWinst")
IF lnCount > 0
?"Windowstation name", laWinst[1]
ENDIF

LOCAL laDesks[1]
&& enumerate desktops
lnCount = ADesktops("laDesks")
IF lnCount > 0
?"Desktop name", laDesks[1]
ENDIF

LOCAL laWinds[1]
&& enumerate toplevel windows into array
lnCount = AWindows("laWinds",AWINDOWS_TOPLEVEL)
IF lnCount > 0
? "HWND", laWinds[1]
ENDIF
&& enumerate child windows of a window
lnCount = AWINDOWS("laWinds",AWINDOWS_CHILD,_SCREEN.HWnd)
&& enumerate all windows of a thread
lnCount = AWINDOWS("laWinds",AWINDOWS_THREAD,_VFP.ThreadId)
&& enumerate all windows of a desktop
&& lnCount = AWINDOWS("laWinds,AWINDOWS_DESKTOP,desktophandle)

&& the same as above but instead of putting the info into an array callback a function
lnCount = AWINDOWS("WindowEnumCallback",AWINDOWS_TOPLEVEL+AWINDOWS_CALLBACK)
&& .....

&& enumerate toplevel windows with additional information on each window
lnCount = AWindowsEx("laWinds","WCTSEHPDIROVNMU",AWINDOWS_TOPLEVEL)
&& set intellisense help for valid flags and their meaning for the second parameter

LOCAL laProps[1]
&& enumerate custom properties of a window
lnCount = AWindowProps("laProps",_SCREEN.hWnd)
IF lnCount > 0
?"Property name", laProps[1,1]
?"Property data", laProps[1,2]
ENDIF

LOCAL laRes[1]
&& enumerate all resolutions the graphic card supports
lnCount = AResolutions("laRes")
IF lnCount > 0
?"Width in pixels", laRes[1,1]
?"Height in pixels", laRes[1,2]
?"Color depth", laRes[1,3]
?"Display frequency", laRes[1,4]
ENDIF

lnCount = ADISPLAYDEVICES('laDevices')
IF lnCount > 0
	FOR xj = 1 TO lnCount
		? 'DeviceString', laDevices[xj,1]
		? 'DeviceName', laDevices[xj,2]
		? 'DeviceID', laDevices[xj,3]
		? 'DeviceKey', laDevices[xj,4]
		? 'StateFlags', laDevices[xj,5]
	ENDFOR	
ENDIF

FUNCTION WindowEnumCallback(tnHwnd)
	&& do something here :)
	RETURN .T. && return .F. to stop enumeration
ENDFUNC