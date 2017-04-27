#INCLUDE vfp2c.h

CD (FULLPATH(JUSTPATH(SYS(16))))

SET LIBRARY TO vfp2c32.fll ADDITIVE
SET PROCEDURE TO winapi_structs.prg ADDITIVE
INITVFP2C32(VFP2C_INIT_MARSHAL) && we only need marshaling support 

#DEFINE ABM_NEW				0
#DEFINE ABM_REMOVE			1
#DEFINE ABM_QUERYPOS		2
#DEFINE ABM_SETPOS			3
#DEFINE ABM_GETSTATE		4
#DEFINE ABM_GETTASKBARPOS	5
#DEFINE ABM_ACTIVATE		6
#DEFINE ABM_GETAUTOHIDEBAR	7
#DEFINE ABM_SETAUTOHIDEBAR	8
#DEFINE ABM_WINDOWPOSCHANGED	9
#DEFINE ABM_SETSTATE      	10

#DEFINE ABN_STATECHANGE		0
#DEFINE ABN_POSCHANGED		1
#DEFINE ABN_FULLSCREENAPP	2
#DEFINE ABN_WINDOWARRANGE	3

#DEFINE ABS_AUTOHIDE		1
#DEFINE ABS_ALWAYSONTOP		2

#DEFINE ABE_LEFT	0
#DEFINE ABE_TOP		1
#DEFINE ABE_RIGHT	2
#DEFINE ABE_BOTTOM	3

*!*	&& TEST
*!*	? "Autohide is ", IIF(GetAutoHideState(),'ON','OFF')
*!*	? "Setting Autohide ON", SetAutoHideState(.T.)
*!*	? "Autohide is ", IIF(GetAutoHideState(),'ON','OFF')
*!*	INKEY(5,'H')
*!*	? "Setting Autohide OFF", SetAutoHideState(.F.)
*!*	? "Autohide is ", IIF(GetAutoHideState(),'ON','OFF')
*!*	&& END TEST

&& TEST 2
? "Autohide is ", IIF(GetAutoHideState(),'ON','OFF')
? "Setting Autohide ON", SetAutoHideState(.T.)
? "Autohide is ", IIF(GetAutoHideState(),'ON','OFF')
INKEY(5,'H')
? "Setting Autohide OFF", SetAutoHideState(.F.)
? "Autohide is ", IIF(GetAutoHideState(),'ON','OFF')
&& END TEST 2


FUNCTION SetAutoHideState
	LPARAMETERS lbAutohide

	DECLARE INTEGER SHAppBarMessage IN shell32.dll INTEGER dwMessage, INTEGER pData
	LOCAL loAppBar, lnResult
	m.loAppBar = CREATEOBJECT('APPBARDATA')
	m.lnResult = SHAppBarMessage(ABM_GETSTATE, m.loAppBar.Address)
	&& set the lParam member of the APPBARDATA structure to the new state
	IF lbAutoHide
		&& BITOR(... will add the ABS_AUTOHIDE bit if not set
		m.loAppBar.lParam = BITOR(m.lnResult,ABS_AUTOHIDE)
	ELSE
		&& BITAND(... will remove the ABS_AUTOHIDE bit if set
		m.loAppBar.lParam = BITAND(m.lnResult,BITNOT(ABS_AUTOHIDE))
	ENDIF
	m.lnResult = SHAppBarMessage(ABM_SETSTATE, m.loAppBar.Address)
ENDFUNC

FUNCTION GetAutoHideState
	DECLARE INTEGER SHAppBarMessage IN shell32.dll INTEGER dwMessage, INTEGER pData
	LOCAL loAppBar, lnResult
	m.loAppBar = CREATEOBJECT('APPBARDATA')
	m.lnResult = SHAppBarMessage(ABM_GETSTATE, m.loAppBar.Address)
	RETURN BITAND(m.lnResult,ABS_AUTOHIDE) > 0
ENDFUNC

DEFINE CLASS APPBARDATA AS Exception

	Address = 0
	SizeOf = 36
	Name = "APPBARDATA"
	&& structure fields
	cbSize = .F.
	hWnd = .F.
	uCallbackMessage = .F.
	uEdge = .F.
	rc = .NULL.
	lParam = .F.

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.SizeOf)
		IF THIS.Address = 0
			ERROR(43)
			RETURN .F.
		ENDIF
		THIS.rc = CREATEOBJECT('RECT',THIS.Address+16)
		THIS.cbSize = THIS.SizeOf
	ENDPROC

	PROCEDURE Destroy()
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE Address_Assign(lnAddress)
		DO CASE
			CASE THIS.Address = 0
				THIS.Address = lnAddress
			CASE THIS.Address = lnAddress
			OTHERWISE
				THIS.Address = lnAddress
				THIS.rc.Address = lnAddress+16
		ENDCASE
	ENDPROC

	PROCEDURE cbSize_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE cbSize_Assign(lnNewVal)
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE hWnd_Access()
		RETURN ReadInt(THIS.Address+4)
	ENDPROC

	PROCEDURE hWnd_Assign(lnNewVal)
		WriteInt(THIS.Address+4,lnNewVal)
	ENDPROC

	PROCEDURE uCallbackMessage_Access()
		RETURN ReadUInt(THIS.Address+8)
	ENDPROC

	PROCEDURE uCallbackMessage_Assign(lnNewVal)
		WriteUInt(THIS.Address+8,lnNewVal)
	ENDPROC

	PROCEDURE uEdge_Access()
		RETURN ReadUInt(THIS.Address+12)
	ENDPROC

	PROCEDURE uEdge_Assign(lnNewVal)
		WriteUInt(THIS.Address+12,lnNewVal)
	ENDPROC

	PROCEDURE lParam_Access()
		RETURN ReadInt(THIS.Address+32)
	ENDPROC

	PROCEDURE lParam_Assign(lnNewVal)
		WriteInt(THIS.Address+32,lnNewVal)
	ENDPROC

ENDDEFINE
