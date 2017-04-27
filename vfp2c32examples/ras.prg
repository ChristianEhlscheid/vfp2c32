&& RAS functions 
&& prerequisites: InitVFP2C32 must have been called with the VFP2C_INIT_RAS flag set and
&& VFP2C_INIT_MARSHAL flag for the RASDIALPARAMS structure

#INCLUDE "vfp2c.h"
#INCLUDE "rasapi32.h"

CD (FULLPATH(JUSTPATH(SYS(16))))
SET LIBRARY TO vfp2c32.fll ADDITIVE
INITVFP2C32(VFP2C_INIT_RAS + VFP2C_INIT_ASYNC + VFP2C_INIT_MARSHAL)

&& enumerate active RAS connections
LOCAL lnCount, xj
lnCount = ARasConnections('laCons')
IF lnCount > 0
	? "Currently active dialup connections"
	FOR xj = 1 TO lnCount
		? "Name:", laCons[xj,1]
		? "Device:", laCons[xj,2]
		? "Devicetype:", laCons[xj,3]
		? "IP Address:", laCons[xj,4]
		? "HRASCONN handle", laCons[xj,5]
	ENDFOR
ENDIF

&& enumerate phonebook entries
lnCount = ARasPhonebookEntries('laBook')
IF lnCount > 0
	? "Phonebook entries"
	FOR xj = 1 TO lnCount
		? "Entryname:", laBook[xj,1]
		? "Entrytype:", IIF(laBook[xj,2]=REN_User,'In users phonebook','In global phonebook')
	ENDFOR
ENDIF

&& calling a phonebook entry
*!*	PUBLIC loRasConn
*!* LOCAL lnFlags 
*!* lnFlags = 0 && see RasDialEx flags constants in rasapi32.h
*!*	loRasConn = CREATEOBJECT('RasConnection')
*!*	IF loRasConn.Dial('phonebookentryname','phonebookfile',lnFlags)
*!*		? "Rasconnection established"
*!*	ENDIF
*!* loRasConn.HangUp()

&& calling a custom connection
&& fill out the RASDIALPARAMS structure and pass this instead of the phonebook entry name
*!*	loParams = CREATEOBJECT('RASDIALPARAMS')
*!*	loParams.szPhoneNumber = '111 2453'
*!*	loParams.szCallbackNumber = '*'
*!*	loParams.szUserName = 'username'
*!*	loParams.szPassword = '******'
*!*	loParams.szDomain = 'domainname'
*!*	loRasConn.Dial(loParams)

LOCAL loDlg, lnFlags
lnFlags = 0
&& lnFlags can be one or more of (add them with +) the following values
&&RASPBDFLAG_PositionDlg !!! NOT SUPPORTED !!! the dialog is always centered on the current active VFP window
&&RASPBDFLAG_ForceCloseOnDial
&&RASPBDFLAG_NoUser          
&&RASPBDFLAG_UpdateDefaults 
&&have a look at the RasPhonebookDlg function description in MSDN for their meanings
loDlg = CREATEOBJECT('RasPhonebook')
IF loDlg.ShowDialog('','',.T.,lnFlags)
	? loDlg.Entryname, "dialed!"
ENDIF

&& monitor RAS connnections
PUBLIC loRasMonitor
loRasMonitor = CREATEOBJECT('RasMonitor')
&& 
loRasMonitor.MonitorConnection(-1, ; && either -1 for all connections or a valid HRASCONN handle
			RASCN_Connection + RASCN_Disconnection)


DEFINE CLASS RasConnection AS Custom

	PROTECTED hConn
	PROTECTED cPublicName
	hConn = 0
	cPublicName = ''
	
	FUNCTION Destroy
		&& release the public "shadow" reference
		IF !EMPTY(THIS.cPublicName)
			ReleasePublicShadowObjReference(THIS.cPublicName)
		ENDIF
	ENDFUNC
	
	FUNCTION Dial(lcEntry, lcPhoneBookFile, lnFlags, lbAsync)
		lcEntry = IIF(VARTYPE(lcEntry)='O',lcEntry.Address,lcEntry)
		lnFlags = IIF(VARTYPE(lnFlags)='N',lnFlags,0)

		LOCAL lcCallback
		IF lbAsync
			&& we need a PUBLIC variable for callbacks from another thread
			IF !EMPTY(THIS.cPublicName)
				&& create a unique name for the variable
				THIS.cPublicName = THIS.Class + SYS(2015)
				&& this function creates a reference to the passed object in a public variable without incrementing the object reference count
				CreatePublicShadowObjReference(THIS.cPublicName,THIS)
			ENDIF
			lcCallback = THIS.cPublicName + '.DialCallback'
		ELSE
			lcCallback = ''
		ENDIF
		
		THIS.hConn = RasDialEx(lcEntry, lcPhonebookFile, lcCallback, lnFlags)
		RETURN THIS.hConn != 0
	ENDFUNC
	
	FUNCTION HangUp()
		IF THIS.hConn != 0
			RasHangUpEx(THIS.hConn)
			THIS.hConn = 0
		ENDIF
	ENDFUNC

	&& callback function that gets called in asyncronous mode
	&& it's up to you to implement it in a reasable manner
	&& see: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/rras/rras/rasdialfunc2.asp for behaviour on WinNT/2K/XP
	&& see: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/rras/rras/rasdialfunc1.asp for behaviour on WinMe/Win9X
	FUNCTION DialCallback(nSubEntry, hRasConn, nEvent, nConnstate, nError, nExtendedError)
	
		DO CASE
			CASE nConnstate = RASCS_OpenPort
			CASE nConnstate = RASCS_PortOpened
			CASE nConnstate = RASCS_ConnectDevice
			CASE nConnstate = RASCS_DeviceConnected
			CASE nConnstate = RASCS_AllDevicesConnected
			CASE nConnstate = RASCS_Authenticate
			CASE nConnstate = RASCS_AuthNotify
			CASE nConnstate = RASCS_AuthRetry
			CASE nConnstate = RASCS_AuthCallback
			CASE nConnstate = RASCS_AuthChangePassword
			CASE nConnstate = RASCS_AuthProject
			CASE nConnstate = RASCS_AuthLinkSpeed
			CASE nConnstate = RASCS_AuthAck
			CASE nConnstate = RASCS_ReAuthenticate
			CASE nConnstate = RASCS_Authenticated
			CASE nConnstate = RASCS_PrepareForCallback
			CASE nConnstate = RASCS_WaitForModemReset
			CASE nConnstate = RASCS_WaitForCallback
			CASE nConnstate = RASCS_Projected
			CASE nConnstate = RASCS_StartAuthentication
			CASE nConnstate = RASCS_CallbackComplete
			CASE nConnstate = RASCS_LogonNetwork
			CASE nConnstate = RASCS_SubEntryConnected
			CASE nConnstate = RASCS_SubEntryDisconnected
			CASE nConnstate = RASCS_Interactive
			CASE nConnstate = RASCS_RetryAuthentication
			CASE nConnstate = RASCS_CallbackSetByCaller
			CASE nConnstate = RASCS_PasswordExpired
			CASE nConnstate = RASCS_InvokeEapUI
			CASE nConnstate = RASCS_Connected
				THIS.hConn = hRasConn
			CASE nConnstate = RASCS_Disconnected
				THIS.hConn = 0
		ENDCASE
		
		RETURN 1 && RETURN 0 to stop to receive notifications
		
	ENDFUNC
	
ENDDEFINE

DEFINE CLASS RasPhoneBook AS Custom
	
	Entryname = ''
	
	FUNCTION ShowDialog(lcInitialSelectedEntry,lcPhonebookFile, lbCallback, lnFlags)
		LOCAL lcCallbackFunc
		lcCallbackFunc = IIF(lbCallback,'THIS.DialogCallback',.NULL.)
		lnFlags = IIF(PCOUNT()<4,0,lnFlags) && default to 0 when not passed
		RETURN RasPhonebookDlgEx(lcInitialSelectedEntry,lcPhonebookFile,lcCallbackFunc,lnFlags)
		&& returns .T when a phonebookentry was dialed, otherwise .F.
	ENDFUNC

	FUNCTION DialogCallback(lnEvent, lcText, lnData)
		DO CASE
			CASE lnEvent = RASPBDEVENT_AddEntry
				? "RasEntry added: ", lcText
			CASE lnEvent = RASPBDEVENT_EditEntry
				? "RasEntry edited: ", lcText
			CASE lnEvent = RASPBDEVENT_RemoveEntry
				? "RasEntry removed: ", lcText
			CASE lnEvent = RASPBDEVENT_DialEntry
				THIS.Entryname = lcText
				? "RasEntry dialed: ", lcText
			CASE lnEvent = RASPBDEVENT_EditGlobals
				? "RasEntry (global) edited: ", lcText
			CASE lnEvent = RASPBDEVENT_NoUser
				? "RasEntry NoUser event"
			CASE lnEvent = RASPBDEVENT_NoUserEdit
				? "RasEntry NoUserEdit event"			
		ENDCASE
	ENDFUNC
	
ENDDEFINE

&& example implementation of a Ras Connection Monitor class
DEFINE CLASS RasMonitor AS Custom

	PROTECTED cPublicName
	PROTECTED nSlot
	PROTECTED nConn
	
	cPublicName = ''
	nSlot = -1
	nConn = -1
	
	FUNCTION Init
		&& we need a PUBLIC variable for callbacks from another thread
		&& create a unique name for the variable
		THIS.cPublicName = THIS.Class + SYS(2015)
		&& this function creates a reference to the passed object in a public variable without incrementing the object reference count
		CreatePublicShadowObjReference(THIS.cPublicName,THIS)
	ENDFUNC
	
	FUNCTION Destroy
		IF THIS.nSlot >= 0
			AbortRasConnectionNotificationEx(THIS.nSlot)
		ENDIF
		&& release the public "shadow" reference
		ReleasePublicShadowObjReference(THIS.cPublicName)
	ENDFUNC
	
	FUNCTION MonitorConnection(lnConn, lnReasons)
		ASSERT THIS.nSlot != -1 MESSAGE 'Abort active RAS connection monitoring first!'
		THIS.nConn = lnConn
		THIS.nSlot = RasConnectionNotificationEx(lnConn,lnReasons,THIS.cPublicName+".OnConnectionChange")
	ENDFUNC
	
	FUNCTION AbortMonitoring()
		ASSERT THIS.nSlot >= 0 MESSAGE 'Start RAS connection monitoring first!'
		RETURN AbortRasConnectionNotificationEx(THIS.nSlot)
	ENDFUNC

	FUNCTION OnConnectionChange(lnConn, lnError)
		IF lnError = 0

			&& call ARasConnections to see if a new connection was established or one was disconnected
			LOCAL lnConnCount, laRasConns[1,5]
			lnConnCount = ARasConnections('laRasConns')
			IF ASCAN(laRasConns,lnConn,1,0,5,8) > 0
				? "RAS connection", lnConn, "connected."
			ELSE
				? "RAS connection", lnConn, "disconnected."
			ENDIF
			
			&& when monitoring a single active RAS connection, the notifications end automatically after 
			&& it's disconnected
			IF THIS.nConn != -1
				THIS.nSlot = -1
				THIS.nConn = -1
			ENDIF
		ELSE
			? "An API error occured", lnError
		ENDIF
	ENDFUNC

ENDDEFINE

&& RASDIALPARAMS struct, for RasDialEx
DEFINE CLASS RASDIALPARAMS AS Exception

	Address = 0
	SizeOf = 1060
	Name = "tagRASDIALPARAMSA"
	&& structure fields
	dwSize = .F.
	szEntryName = .F.
	szPhoneNumber = .F.
	szCallbackNumber = .F.
	szUserName = .F.
	szPassword = .F.
	szDomain = .F.
	dwSubEntry = .F.
	dwCallbackId = .F.

	PROCEDURE Init()
		THIS.Address = AllocMem(THIS.SizeOf)
		IF THIS.Address = 0
			ERROR(43)
			RETURN .F.
		ENDIF
		THIS.dwSize = THIS.SizeOf
	ENDPROC

	PROCEDURE Destroy()
		FreeMem(THIS.Address)
	ENDPROC

	PROCEDURE dwSize_Access()
		RETURN ReadUInt(THIS.Address)
	ENDPROC

	PROCEDURE dwSize_Assign(lnNewVal)
		WriteUInt(THIS.Address,lnNewVal)
	ENDPROC

	PROCEDURE szEntryName_Access()
		RETURN ReadCharArray(THIS.Address+4,257)
	ENDPROC

	PROCEDURE szEntryName_Assign(lnNewVal)
		WriteCharArray(THIS.Address+4,lnNewVal,257)
	ENDPROC

	PROCEDURE szPhoneNumber_Access()
		RETURN ReadCharArray(THIS.Address+261,129)
	ENDPROC

	PROCEDURE szPhoneNumber_Assign(lnNewVal)
		WriteCharArray(THIS.Address+261,lnNewVal,129)
	ENDPROC

	PROCEDURE szCallbackNumber_Access()
		RETURN ReadCharArray(THIS.Address+390,129)
	ENDPROC

	PROCEDURE szCallbackNumber_Assign(lnNewVal)
		WriteCharArray(THIS.Address+390,lnNewVal,129)
	ENDPROC

	PROCEDURE szUserName_Access()
		RETURN ReadCharArray(THIS.Address+519,257)
	ENDPROC

	PROCEDURE szUserName_Assign(lnNewVal)
		WriteCharArray(THIS.Address+519,lnNewVal,257)
	ENDPROC

	PROCEDURE szPassword_Access()
		RETURN ReadCharArray(THIS.Address+776,257)
	ENDPROC

	PROCEDURE szPassword_Assign(lnNewVal)
		WriteCharArray(THIS.Address+776,lnNewVal,257)
	ENDPROC

	PROCEDURE szDomain_Access()
		RETURN ReadCharArray(THIS.Address+1033,16)
	ENDPROC

	PROCEDURE szDomain_Assign(lnNewVal)
		WriteCharArray(THIS.Address+1033,lnNewVal,16)
	ENDPROC

	PROCEDURE dwSubEntry_Access()
		RETURN ReadUInt(THIS.Address+1052)
	ENDPROC

	PROCEDURE dwSubEntry_Assign(lnNewVal)
		WriteUInt(THIS.Address+1052,lnNewVal)
	ENDPROC

	PROCEDURE dwCallbackId_Access()
		RETURN ReadUInt(THIS.Address+1056)
	ENDPROC

	PROCEDURE dwCallbackId_Assign(lnNewVal)
		WriteUInt(THIS.Address+1056,lnNewVal)
	ENDPROC

ENDDEFINE