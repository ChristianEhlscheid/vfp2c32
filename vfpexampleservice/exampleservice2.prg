#INCLUDE "vfp2c.h"
#INCLUDE "vfpservice.h"

&& this Service spawns an object in a new thread using CreateThreadObject
&& the ExampleWorkerThread2 creates a waitable timer that is signaled every 60 seconds
&& the service supports pause and continue commands

DEFINE CLASS ExampleService2 AS ServiceBaseclass OLEPUBLIC

	&& 	custom properties
	oWorkerThread = .NULL.
	oServiceController = .NULL.
	
	FUNCTION  OnStart(laArgs[] As String, loServiceController AS Object) As Void
		TRY
			SET LIBRARY TO vfp2c32t.fll ADDITIVE
			IF INITVFP2C32(VFP2C_INIT_MARSHAL) = .F.
				LOCAL laError[1]
				AERROREX('laError')
				THROW CREATEOBJECT('Win32Exception', m.laError[1], m.laError[2], m.laError[3])
			ENDIF

#IFDEF _DEBUG
		DebugOutput('ExampleService2::OnStart')
#ENDIF
			THIS.oServiceController = m.loServiceController
			THIS.oWorkerThread = CreateThreadObject('VFPExampleService.ExampleWorker2', THIS)
			THIS.oWorkerThread.ThreadFunc()
		CATCH TO loExc
			THIS.ThrowError(m.loExc)
		ENDTRY
	ENDFUNC

	FUNCTION OnStop() AS Void
		TRY
#IFDEF _DEBUG
		DebugOutput('ExampleService2::OnStop')
#ENDIF	
			THIS.oWorkerThread.AbortCall()
			THIS.oWorkerThread = .NULL.
			SET LIBRARY TO			
		CATCH TO loExc
#IFDEF _DEBUG
		DebugOutput('ExampleService2::OnStop - Error')
#ENDIF		
			THIS.oWorkerThread = .NULL.
			THIS.ThrowError(m.loExc)
		ENDTRY
	ENDFUNC

	FUNCTION OnPause() AS Void
		TRY
#IFDEF _DEBUG
		DebugOutput('ExampleService2::OnPause')
#ENDIF	
			THIS.oWorkerThread.AbortCall()
		CATCH TO loExc
			THIS.ThrowError(m.loExc)
		ENDTRY
	ENDFUNC

	FUNCTION OnContinue() AS Void
		TRY
#IFDEF _DEBUG
		DebugOutput('ExampleService2::OnContinue')
#ENDIF		
			THIS.oWorkerThread.ThreadFunc()
		CATCH TO loExc
			THIS.ThrowError(m.loExc)
		ENDTRY
	ENDFUNC

	&& called when an error occurs in the ExampleWorkerThread2 object
	FUNCTION OnError(callid AS Long, callcontext AS Variant, errornumber AS Long, errorsource AS String, errordescription AS String) AS VOID
#IFDEF _DEBUG
		DebugOutput('ExampleService2::OnError')
#ENDIF			
		&& we just stop the service
		THIS.oServiceController.Stop()
	ENDFUNC

ENDDEFINE

DEFINE CLASS ExampleWorker2 AS ServiceBaseclass OLEPUBLIC

	DataSession = 2
	CallInfo = .NULL.
	
	FUNCTION ThreadFunc() As Void
		TRY
			SET LIBRARY TO vfp2c32t.fll ADDITIVE
#IFDEF _DEBUG
		DebugOutput('ExampleWorker2::ThreadFunc: ' + TRANSFORM(DATETIME()))
#ENDIF

			&& start a timer with an interval of 1 minute - the timer is first signaled at the next full minute
			LOCAL loTimer, ldStartTime 
			m.loTimer = CREATEOBJECT('CWaitableTimer', .F.)
			m.ldStartTime = DATETIME()
			m.ldStartTime = m.ldStartTime + (60 - SEC(m.ldStartTime)) && get next full minute
			m.loTimer.SetTimer(60 * 1000, m.ldStartTime)

			DO WHILE .T.
				IF m.loTimer.Wait(INFINITE, THIS.CallInfo.AbortEvent) = WAIT_OBJECT_0
					THIS.OnTimer()
				ELSE
					EXIT
				ENDIF
			ENDDO

		CATCH TO loExc
			THIS.ThrowError(m.loExc)
		ENDTRY

#IFDEF _DEBUG
		DebugOutput('ExampleWorker2::ThreadFunc: Exiting')
#ENDIF

	ENDFUNC

	FUNCTION OnTimer() AS Void
#IFDEF _DEBUG
		DebugOutput('ExampleWorker2::OnTimer: ' + TRANSFORM(DATETIME()))
#ENDIF	
	ENDFUNC

ENDDEFINE
