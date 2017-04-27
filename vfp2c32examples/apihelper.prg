&& some DLL helper functions
DECLARE INTEGER LoadLibraryA IN kernel32.dll AS LoadLibrary STRING lcModule
DECLARE INTEGER GetModuleHandleA IN kernel32.dll AS GetModuleHandle STRING lcModule
DECLARE INTEGER GetProcAddress IN kernel32.dll INTEGER hModule, STRING lcFunction
DECLARE INTEGER FreeLibrary IN kernel32.dll INTEGER hModule

SET PROCEDURE TO apihelper ADDITIVE

&& determine if the function passed in 'lcFunction'
&& is exported by the DLL passed in 'lcModule'
&& Returs:
&& -1 if the DLL can't be found
&& 0 if the function is not exported
&& > 0 if the function is exported
FUNCTION IsFuncExported(lcModule,lcFunction)
	LOCAL lnMHandle, lnFuncAddr, lbLoaded

	lnMHandle = GetModuleHandle(lcModule)
	IF lnMHandle = 0
		lnMHandle = LoadLibrary(lcModule)
		IF lnMHandle = 0
			RETURN -1
		ENDIF
		lbLoaded = .T.
	ENDIF
	
	lnFuncAddr = GetProcAddress(lnMHandle,lcFunction)
	
	IF lbLoaded
		FreeLibrary(lnMHandle)
	ENDIF
	
	RETURN lnFuncAddr
ENDFUNC

&& is a given DLL actually loaded into the process
&& return .T. or .F. respectivly
FUNCTION IsModuleLoaded(lcModule)
	RETURN GetModuleHandle(lcModule) != 0
ENDFUNC

&& are there 2 entry points for the function:
&& one ANSI version and one UNICODE version ?
&& returns:
&& -1 if the DLL can't be found
&& 0 if the function is not Ansi & Unicode
&& 1 if it is Ansi & Unicode
FUNCTION IsFuncAnsiUnicode(lcModule,lcFunction)
	LOCAL lnMHandle, lnAnsiAddr, lnUnicodeAddr, lbLoaded

	lnMHandle = GetModuleHandle(lcModule)
	IF lnMHandle = 0
		lnMHandle = LoadLibrary(lcModule)
		IF lnMHandle = 0
			RETURN -1
		ENDIF
		lbLoaded = .T.
	ENDIF
	
	lnAnsiAddr = GetProcAddress(lnMHandle,lcFunction+"A")
	lnUnicodeAddr = GetProcAddress(lnMHandle,lcFunction+"W")
	
	IF lbLoaded
		FreeLibrary(lnMHandle)
	ENDIF
	
	IF lnAnsiAddr > 0 AND lnUnicodeAddr > 0
		RETURN 1
	ELSE
		RETURN 0
	ENDIF

ENDFUNC


