#INCLUDE vfp2c.h

SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_MARSHAL)

#DEFINE STARTF_USESHOWWINDOW		0x01
#DEFINE SW_HIDE							0
#DEFINE SW_SHOWNOACTIVATE			4
#DEFINE SW_SHOWMINNOACTIVE		7
#DEFINE SW_SHOWNA					8
#DEFINE LSFW_LOCK						1
#DEFINE LSFW_UNLOCK					2

DECLARE INTEGER CreateProcess IN kernel32.dll STRING cApplication, STRING cCommandLine, ;
INTEGER pProcessAtrributes, INTEGER pThreadAttributes, INTEGER bInheritHandles, ;
INTEGER dwCreationFlags, INTEGER lpEnvironment, STRING cCurrentDirectory, ;
INTEGER pStartupInfo, INTEGER pProcessInformation

DECLARE INTEGER CloseHandle IN kernel32.dll INTEGER
DECLARE INTEGER GetModuleFileName IN kernel32.dll INTEGER, STRING @, INTEGER

LOCAL loStartup, loProcessInfo, lcCommand, lnRet, lnHwnd

loStartup = CREATEOBJECT('STARTUPINFO')
loProcessInfo = CREATEOBJECT('PROCESS_INFORMATION')
loStartup.dwFlags = STARTF_USESHOWWINDOW
	loStartup.wShowWindow = SW_HIDE
lcCommand = "notepad.exe"
lnRet = CreateProcess(0, lcCommand,0,0,0,0,0,0,loStartup.Address,loProcessInfo.Address)

CloseHandle(loProcessInfo.hProcess)
CloseHandle(loProcessInfo.hThread)


FUNCTION GetExeFileName()

#DEFINE MAX_PATH 260
DECLARE INTEGER GetModuleFileName IN kernel32.dll INTEGER hModule, ;
STRING @lpFilename, INTEGER nSize

LOCAL lcPath, lnRet, lnSize
m.lnSize = MAX_PATH
m.lcPath = REPLICATE(CHR(0),m.lnSize)
m.lnRet = GetModuleFileName(0,@m.lcPath,m.lnSize)
IF lnRet > 0
 RETURN LEFT(m.lcPath,m.lnRet)
ELSE
 RETURN '' && handle error
ENDIF

ENDFUNC

DEFINE CLASS STARTUPINFO AS Exception

	Address = 0
	SizeOf = 68
	Name = "STARTUPINFO"
	&& structure fields
	cb = .F.
	&& lpReserved = .F.
	lpDesktop = .F.
	lpTitle = .F.
	dwX = .F.
	dwY = .F.
	dwXSize = .F.
	dwYSize = .F.
	dwXCountChars = .F.
	dwYCountChars = .F.
	dwFillAttribute = .F.
	dwFlags = .F.
	wShowWindow = .F.
	&& cbReserved2 = .F.
	&& lpReserved2 = .F.
	hStdInput = .F.
	hStdOutput = .F.
	hStdError = .F.

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.SizeOf)
		THIS.cb = THIS.SizeOf
	ENDPROC

	PROCEDURE Destroy()
		THIS.FreeMembers()
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE FreeMembers()
		FreePMem(THIS.Address+4)
		FreePMem(THIS.Address+8)
		FreePMem(THIS.Address+12)
		FreePMem(THIS.Address+52)
	ENDPROC

	PROCEDURE cb_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE cb_Assign(lnNewVal)
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE lpDesktop_Access()
		RETURN ReadPCString(THIS.Address+8)
	ENDPROC

	PROCEDURE lpDesktop_Assign(lnNewVal)
		WritePCString(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE lpTitle_Access()
		RETURN ReadPCString(THIS.Address+12)
	ENDPROC

	PROCEDURE lpTitle_Assign(lnNewVal)
		WritePCString(THIS.Address+12,lnNewVal)
	ENDPROC

	PROCEDURE dwX_Access()
		RETURN ReadUInt(THIS.Address+16)
	ENDPROC

	PROCEDURE dwX_Assign(lnNewVal)
		WriteUInt(THIS.Address+16,lnNewVal)
	ENDPROC

	PROCEDURE dwY_Access()
		RETURN ReadUInt(THIS.Address+20)
	ENDPROC

	PROCEDURE dwY_Assign(lnNewVal)
		WriteUInt(THIS.Address+20,lnNewVal)
	ENDPROC

	PROCEDURE dwXSize_Access()
		RETURN ReadUInt(THIS.Address+24)
	ENDPROC

	PROCEDURE dwXSize_Assign(lnNewVal)
		WriteUInt(THIS.Address+24,lnNewVal)
	ENDPROC

	PROCEDURE dwYSize_Access()
		RETURN ReadUInt(THIS.Address+28)
	ENDPROC

	PROCEDURE dwYSize_Assign(lnNewVal)
		WriteUInt(THIS.Address+28,lnNewVal)
	ENDPROC

	PROCEDURE dwXCountChars_Access()
		RETURN ReadUInt(THIS.Address+32)
	ENDPROC

	PROCEDURE dwXCountChars_Assign(lnNewVal)
		WriteUInt(THIS.Address+32,lnNewVal)
	ENDPROC

	PROCEDURE dwYCountChars_Access()
		RETURN ReadUInt(THIS.Address+36)
	ENDPROC

	PROCEDURE dwYCountChars_Assign(lnNewVal)
		WriteUInt(THIS.Address+36,lnNewVal)
	ENDPROC

	PROCEDURE dwFillAttribute_Access()
		RETURN ReadUInt(THIS.Address+40)
	ENDPROC

	PROCEDURE dwFillAttribute_Assign(lnNewVal)
		WriteUInt(THIS.Address+40,lnNewVal)
	ENDPROC

	PROCEDURE dwFlags_Access()
		RETURN ReadUInt(THIS.Address+44)
	ENDPROC

	PROCEDURE dwFlags_Assign(lnNewVal)
		WriteUInt(THIS.Address+44,lnNewVal)
	ENDPROC

	PROCEDURE wShowWindow_Access()
		RETURN ReadUShort(THIS.Address+48)
	ENDPROC

	PROCEDURE wShowWindow_Assign(lnNewVal)
		WriteUShort(THIS.Address+48,lnNewVal)
	ENDPROC

	PROCEDURE hStdInput_Access()
		RETURN ReadPointer(THIS.Address+56)
	ENDPROC

	PROCEDURE hStdInput_Assign(lnNewVal)
		WritePointer(THIS.Address+56,lnNewVal)
	ENDPROC

	PROCEDURE hStdOutput_Access()
		RETURN ReadPointer(THIS.Address+60)
	ENDPROC

	PROCEDURE hStdOutput_Assign(lnNewVal)
		WritePointer(THIS.Address+60,lnNewVal)
	ENDPROC

	PROCEDURE hStdError_Access()
		RETURN ReadPointer(THIS.Address+64)
	ENDPROC

	PROCEDURE hStdError_Assign(lnNewVal)
		WritePointer(THIS.Address+64,lnNewVal)
	ENDPROC

ENDDEFINE

DEFINE CLASS PROCESS_INFORMATION AS Exception

	Address = 0
	SizeOf = 16
	Name = "PROCESS_INFORMATION"
	&& structure fields
	hProcess = .F.
	hThread = .F.
	dwProcessId = .F.
	dwThreadId = .F.

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.SizeOf)
	ENDPROC

	PROCEDURE Destroy()
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE hProcess_Access()
		RETURN ReadPointer(THIS.Address)
	ENDPROC

	PROCEDURE hProcess_Assign(lnNewVal)
		WritePointer(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE hThread_Access()
		RETURN ReadPointer(THIS.Address+4)
	ENDPROC

	PROCEDURE hThread_Assign(lnNewVal)
		WritePointer(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE dwProcessId_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE dwProcessId_Assign(lnNewVal)
		WriteUInt(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE dwThreadId_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE dwThreadId_Assign(lnNewVal)
		WriteUInt(THIS.Address+12,lnNewVal)
	ENDPROC

ENDDEFINE
