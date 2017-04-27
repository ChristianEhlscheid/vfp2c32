#INCLUDE "vfpservice.h"

&& this service just logs all events into a file
#DEFINE SERVICE_LOGFILE 'ExampleService1.log'

DEFINE CLASS ExampleService1 AS ServiceBaseclass OLEPUBLIC

	ExitCode = 0

	FUNCTION OnStart(laArgs[] As String,  loServiceController AS Object) As Void
		THIS.LogMessage('Start')
	ENDFUNC
	
	FUNCTION OnStop() As Void
		THIS.LogMessage('Stop')
	ENDFUNC
	
	FUNCTION OnPause() As Void
		THIS.LogMessage('OnPause')
	ENDFUNC
	
	FUNCTION OnContinue() As Void
		THIS.LogMessage('OnContinue')
	ENDFUNC

	FUNCTION OnShutdown() As Void
		THIS.LogMessage('OnShutdown')
	ENDFUNC

	FUNCTION OnPreShutdown() As Void
		THIS.LogMessage('OnPreShutdown')
		RETURN 0
	ENDFUNC

	FUNCTION OnParamChange() As Void
		THIS.LogMessage('OnParamChange')
	ENDFUNC

	FUNCTION OnHardwareProfileChange(eventtype As Long) As Void
		THIS.LogMessage('OnHardwareProfileChange')
	ENDFUNC
		
	FUNCTION OnSessionChange(reason As Long, sessionid As Long) AS Void
		LOCAL lcReason
		DO CASE
			CASE m.reason = WTS_CONSOLE_CONNECT
				m.lcReason = 'Console session connected'
			CASE m.reason = WTS_CONSOLE_DISCONNECT
				m.lcReason = 'Console session disconnected'
			CASE m.reason = WTS_REMOTE_CONNECT
				m.lcReason = 'Remote session connected'
			CASE m.reason = WTS_REMOTE_DISCONNECT
				m.lcReason = 'Remote session disconnected'
			CASE m.reason = WTS_SESSION_LOGON
				m.lcReason = 'User logged on session'
			CASE m.reason = WTS_SESSION_LOGOFF
				m.lcReason = 'User logged off session '
			CASE m.reason = WTS_SESSION_LOCK
				m.lcReason = 'Session locked'
			CASE m.reason = WTS_SESSION_UNLOCK																		
				m.lcReason = 'Session unlocked'
			OTHERWISE
				m.lcReason = 'Unknown reason'
		ENDCASE
		m.lcReason = m.lcReason + ' ' + TRANSFORM(m.sessionid)
		THIS.LogMessage('OnSessionChange - ' + m.lcReason)
	ENDFUNC

	FUNCTION OnPowerEvent(eventtype As Long, powerevent As Long, eventdata AS Long) As Long
		TRY
			LOCAL lcEvent
			DO CASE
				CASE m.eventtype = PBT_APMPOWERSTATUSCHANGE
					m.lcEvent = 'Power status has changed'
				CASE m.eventtype = PBT_APMRESUMEAUTOMATIC
					m.lcEvent = 'Operation is resuming automatically from a low-power state'
				CASE m.eventtype = PBT_APMRESUMESUSPEND
					m.lcEvent = 'Operation is resuming from a low-power state'
				CASE m.eventtype = PBT_APMSUSPEND
					m.lcEvent = 'System is suspending operation'
				CASE m.eventtype = PBT_POWERSETTINGCHANGE
					m.lcEvent = 'Power setting change - PowerEvent: ' + ;
						ICASE(m.powerevent = PE_POWERSCHEME_PERSONALITY, 'PE_POWERSCHEME_PERSONALITY', ;
								m.powerevent = PE_ACDC_POWER_SOURCE, 'PE_ACDC_POWER_SOURCE', ;
								m.powerevent = PE_BATTERY_PERCENTAGE_REMAINING, 'PE_BATTERY_PERCENTAGE_REMAINING', ;
								m.powerevent = PE_IDLE_BACKGROUND_TASK, 'PE_IDLE_BACKGROUND_TASK', ;
								m.powerevent = PE_SYSTEM_AWAYMODE, 'PE_SYSTEM_AWAYMODE', ;
								m.powerevent = PE_MONITOR_POWER_ON, 'PE_MONITOR_POWER_ON', 'Unknown') + ;
						' Data: ' + TRANSFORM(m.eventdata)										
				CASE m.eventtype = PBT_APMBATTERYLOW
					m.lcEvent = 'Battery power is low'
				CASE m.eventtype = PBT_APMOEMEVENT
					m.lcEvent = 'OEM-defined event'
				CASE m.eventtype = PBT_APMQUERYSUSPEND																		
					m.lcEvent = 'Request for permission to suspend'
				CASE m.eventtype = PBT_APMQUERYSUSPENDFAILED																		
					m.lcEvent = 'Suspension request denied'
				CASE m.eventtype = PBT_APMRESUMECRITICAL																		
					m.lcEvent = 'Operation resuming after critical suspension'
				OTHERWISE
					m.lcEvent = 'Unknown event'
			ENDCASE
			THIS.LogMessage('OnPowerEvent - ' + m.lcEvent)	
		CATCH TO loExc
			THIS.LogMessage('OnPowerEvent - Error')	
			THIS.ThrowError(m.loExc)
		ENDTRY
		RETURN 0
	ENDFUNC

	FUNCTION OnTimeChange(newTime As Datetime, oldTime As Datetime) As Void
		THIS.LogMessage('OnTimeChange - NewTime: ' + TRANSFORM(m.newTime) + ' OldTime: ' + TRANSFORM(m.oldTime))
	ENDFUNC

	FUNCTION OnCustomCommand(customcommand As Long) As Void
		THIS.LogMessage('OnCustomCommand: ' + TRANSFORM(m.customcommand))
	ENDFUNC

	FUNCTION LogMessage(lcMessage As String) AS Void
		STRTOFILE(m.lcMessage  + CHR(13), SERVICE_LOGFILE, 1)
	ENDFUNC

ENDDEFINE