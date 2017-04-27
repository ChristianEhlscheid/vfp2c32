#INCLUDE vfp2c.h

&& CreateCallbackFunc
&& prerequisites: InitVFP2C32 must have been called with the VFP2C_INIT_CALLBACK flag set

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_CALLBACK)

LOCAL loCallback
DECLARE INTEGER EnumWindows IN user32.dll INTEGER, INTEGER
loCallBack = CREATEOBJECT('WNDENUMPROC')
EnumWindows(loCallback.Address,0)

DEFINE CLASS WNDENUMPROC AS Exception

	Address = 0

	FUNCTION Init
		&& the C prototype for the callback function is
		&& BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam)
		&& BOOL is clear ..
		&& CALLBACK is a preprocessor definition for "WINAPI" which in turn is a definition for "_stdcall"
		&& which specifies the calling convention of the funtion.
		&& You can just ignore this since the callback functions created by CreateCallbackFunc always use
		&& the _stdcall calling convention by design. (I've never seen a C Callback definition using another 
		&& calling convention)
		THIS.Address = CreateCallbackFunc('EnumWindowsCallback','BOOL','LONG, LONG',THIS)
	ENDFUNC
	
	FUNCTION Destroy
		IF THIS.Address != 0
			DestroyCallbackFunc(THIS.Address)
		ENDIF
	ENDFUNC
	
	FUNCTION EnumWindowsCallback(hHwnd,lParam)
		?hHwnd,lParam
		RETURN .T.
	ENDFUNC
	
ENDDEFINE



