#include "vfpservice.h"

DEFINE CLASS ExceptionBase AS Exception

	StackTrace = ''
	
	&& reads the lineno and procedure from the stack information at the passed relative level
	PROTECTED FUNCTION GetStackInfo(lnStackLevel)
		LOCAL laStack[1], lnCount, lnLevel, lcStackTrace, xj
		m.lnCount = ASTACKINFO(m.laStack)
		IF m.lnCount - m.lnStackLevel >= 1
			m.lnLevel = m.lnCount - m.lnStackLevel
		ELSE
			m.lnLevel = m.lnCount
		ENDIF
		
		THIS.Procedure = m.laStack[m.lnLevel, 4]
		THIS.LineNo = m.laStack[m.lnLevel, 5]		
		THIS.StackLevel = m.laStack[m.lnLevel, 1]
		
		m.lcStackTrace = ''
		FOR m.xj = 1 TO m.lnLevel
			m.lcStackTrace = m.lcStackTrace + ALLTRIM(STR(m.laStack[m.xj, 1])) + PADR(': Line ' + ALLTRIM(STR(m.laStack[m.xj, 5])), 14, ' ') + ;
							 PADR(ALLTRIM(m.laStack[m.xj, 6]), 100, ' ') + ' -> ' + m.laStack[m.xj, 3] + ' (' + m.laStack[m.xj, 2] + ')' + CHR(13)
		ENDFOR
		THIS.StackTrace = m.lcStackTrace
	ENDFUNC
	
ENDDEFINE

DEFINE CLASS Win32Exception AS ExceptionBase
	
	NativeErrorCode = 0
	Name = 'Win32Exception'
	
	FUNCTION Init(lnErrorNo, lcFunction, lcMessage)
		THIS.GetStackInfo(2)
		THIS.NativeErrorCode = m.lnErrorNo
		THIS.Details = IIF(VARTYPE(m.lcFunction) = 'C', m.lcFunction, '')
		THIS.Message = IIF(VARTYPE(m.lcMessage) = 'C', m.lcMessage, FormatMessageEx(m.lnErrorNo))
	ENDFUNC
	
ENDDEFINE

DEFINE CLASS CWaitableHandleBase As Custom
	
	PROTECTED hHandle
	hHandle = 0
	PROTECTED bOwner
	bOwner = .T.

	FUNCTION Destroy
		THIS.Close()
	ENDFUNC
	
	FUNCTION Close()
		IF THIS.hHandle != 0
			IF THIS.bOwner
				CloseHandle(THIS.hHandle)
			ENDIF
			THIS.hHandle = 0
		ENDIF
	ENDFUNC

	FUNCTION IsSignaled()
		LOCAL lnRet
		m.lnRet = WaitForSingleObject(THIS.hHandle, 0)
		DO CASE
			CASE m.lnRet = WAIT_OBJECT_0
				RETURN .T.
			CASE m.lnRet = WAIT_TIMEOUT
				RETURN .F.
			CASE m.lnRet = WAIT_FAILED
				THROW CREATEOBJECT('Win32Exception', GetLastError(), 'WaitForSingleObject')
		ENDCASE
	ENDFUNC

	FUNCTION Wait(lnTimeOut, laEvents)
		LOCAL lcHandles, lnHandleCount, lnRet, lnCount, xj
		EXTERNAL ARRAY laEvents
		m.lnHandleCount = 1
		m.lcHandles = Long2Str(THIS.hHandle)

		IF VARTYPE(m.lnTimeOut) != 'N'
			m.lnTimeOut = INFINITE
		ENDIF
		
		DO CASE
			CASE TYPE('m.laEvents', 1) = 'A'
				m.lnCount = ALEN(m.laEvents)
				FOR m.xj = 1 TO m.lnCount
					DO CASE
						CASE VARTYPE(m.laEvents[m.xj]) = 'N'					
							m.lcHandles = m.lcHandles + Long2Str(m.laEvents)
							m.lnHandleCount = m.lnHandleCount + 1
						CASE VARTYPE(m.laEvents[m.xj]) = 'O'
							m.lcHandles = m.lcHandles + Long2Str(m.laEvents.GetGandle())
							m.lnHandleCount = m.lnHandleCount + 1
					ENDCASE
				ENDFOR
				
			CASE VARTYPE(m.laEvents) = 'N'
				m.lnHandleCount = 2
				m.lcHandles = m.lcHandles + Long2Str(m.laEvents)

			CASE VARTYPE(m.laEvents) = 'O'
				m.lnHandleCount = 2
				m.lcHandles = m.lcHandles + Long2Str(m.laEvents.GetHandle())
			
		ENDCASE

		m.lnRet = WaitForMultipleObjects(m.lnHandleCount, m.lcHandles, FALSE, m.lnTimeOut)
		IF m.lnRet = WAIT_FAILED
			THROW CREATEOBJECT('Win32Exception', GetLastError(), 'WaitForMultipleObjects')
		ENDIF
		RETURN m.lnRet
	ENDFUNC

	FUNCTION Attach(hHandle, bOwner)
		THIS.Close()
		THIS.hHandle = m.hHandle
		IF PCOUNT() = 1
			THIS.bOwner = .T.
		ELSE
			THIS.bOwner = m.bOwner
		ENDIF
	ENDFUNC

	FUNCTION GetHandle()
		RETURN THIS.hHandle
	ENDFUNC

ENDDEFINE

DEFINE CLASS CEvent AS CWaitableHandleBase

	FUNCTION Init(bManualReset, bInitialState, cName)
		IF PCOUNT() > 0
			THIS.Create(m.bManualReset, m.bInitialState, m.cName)
		ENDIF
	ENDFUNC

	FUNCTION Create(bManualReset, bInitialState, cName)
		IF VARTYPE(cName) != 'C'
			m.cName = .NULL.
		ENDIF
		THIS.Close()
		THIS.hHandle = CreateEvent(0, IIF(m.bManualReset, TRUE, FALSE), IIF(m.bInitialState, TRUE, FALSE), m.cName)
		IF THIS.hHandle = 0
			THROW CREATEOBJECT('Win32Exception', GetLastError(), 'CreateEvent')
		ENDIF
		THIS.bOwner = .T.
	ENDFUNC

	FUNCTION Signal()
		IF SetEvent(THIS.hHandle) = 0
			THROW CREATEOBJECT('Win32Exception', GetLastError(), 'SetEvent')
		ENDIF
	ENDFUNC

ENDDEFINE

DEFINE CLASS CWaitableTimer AS CWaitableHandleBase

	FUNCTION Init(bManualReset, cName)
		IF PCOUNT() > 0
			THIS.Create(m.bManualReset, m.cName)
		ENDIF
	ENDFUNC

	FUNCTION Create(bManualReset, cName)
		IF VARTYPE(cName) != 'C'
			m.cName = .NULL.
		ENDIF
		THIS.Close()
		THIS.hHandle = CreateWaitableTimer(0, IIF(m.bManualReset, TRUE, FALSE), m.cName)
		IF THIS.hHandle = 0
			THROW CREATEOBJECT('Win32Exception', GetLastError(), 'CreateWaitableTimer')
		ENDIF
		THIS.bOwner = .T.
	ENDFUNC

	FUNCTION SetTimer(nInterval, tStartTime)
		LOCAL lnRet, loFileTime
		m.loFileTime = CREATEOBJECT('FILETIME')
		DO CASE
			CASE VARTYPE(m.tStartTime) = 'T'
				m.loFileTime.mDate = m.tStartTime
			CASE VARTYPE(m.tStartTime) = 'N'
				m.loFileTime.dwQuadPart = m.tStartTime * -10000
			OTHERWISE
				m.loFileTime.dwQuadPart = m.nInterval * -10000
		ENDCASE
		m.lnRet = SetWaitableTimer(THIS.hHandle, m.loFileTime.Address, m.nInterval, 0, 0, 0)
		IF m.lnRet = 0
			THROW CREATEOBJECT('Win32Exception', GetLastError(), 'SetWaitableTimer')
		ENDIF
	ENDFUNC

	FUNCTION CancelTimer
		IF CancelWaitableTimer(THIS.hHandle) = 0
			THROW CREATEOBJECT('Win32Exception', GetLastError(), 'CancelWaitableTimer')
		ENDIF
	ENDFUNC

ENDDEFINE


