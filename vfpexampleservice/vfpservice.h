#IFNDEF _VFPSERVICE_H__
#DEFINE _VFPSERVICE_H__

#DEFINE _DEBUG

&& possible values for "eventtype" parameter in Service::OnPowerEvent
#DEFINE PBT_APMPOWERSTATUSCHANGE	0xA	 && Power status has changed.
#DEFINE PBT_APMRESUMEAUTOMATIC		0x12 && Operation is resuming automatically from a low-power state. This message is sent every time the system resumes.
#DEFINE PBT_APMRESUMESUSPEND		0x7  && Operation is resuming from a low-power state. This message is sent after PBT_APMRESUMEAUTOMATIC if the resume is triggered by user input, such as pressing a key.
#DEFINE PBT_APMSUSPEND				0x4  && System is suspending operation.
#DEFINE PBT_POWERSETTINGCHANGE		0x8013 && A power setting change event has been received.
&& Windows Server 2003, Windows XP, and Windows 2000:  The following event identifiers are also supported.
#DEFINE PBT_APMBATTERYLOW			0x9  && Battery power is low. In Windows Server 2008 and Windows Vista, use PBT_APMPOWERSTATUSCHANGE instead.
#DEFINE PBT_APMOEMEVENT				0xB  && OEM-defined event occurred. In Windows Server 2008 and Windows Vista, this event is not available because these operating systems support only ACPI; APM BIOS events are not supported.
#DEFINE PBT_APMQUERYSUSPEND			0x0  && Request for permission to suspend. In Windows Server 2008 and Windows Vista, use the SetThreadExecutionState function instead.
#DEFINE PBT_APMQUERYSUSPENDFAILED	0x2	 && Suspension request denied. In Windows Server 2008 and Windows Vista, use SetThreadExecutionState instead.
#DEFINE PBT_APMRESUMECRITICAL		0x6  && Operation resuming after critical suspension. In Windows Server 2008 and Windows Vista, use PBT_APMRESUMEAUTOMATIC instead.

&& possible values for "powerevent" parameter in Service::OnPowerEvent
#DEFINE PE_POWERSCHEME_PERSONALITY		1
#DEFINE PE_ACDC_POWER_SOURCE			2
#DEFINE PE_BATTERY_PERCENTAGE_REMAINING	3
#DEFINE PE_IDLE_BACKGROUND_TASK			4
#DEFINE PE_SYSTEM_AWAYMODE				5
#DEFINE PE_MONITOR_POWER_ON				6

&& possible values for "eventdata" parameter in Service::OnPowerEvent
#DEFINE PD_MIN_POWER_SAVINGS		1
#DEFINE PD_MAX_POWER_SAVINGS		2
#DEFINE PD_TYPICAL_POWER_SAVINGS	3

&& valid reason values for Service::OnSessionChange
#DEFINE WTS_CONSOLE_CONNECT			0x1 && A session was connected to the console terminal.
#DEFINE WTS_CONSOLE_DISCONNECT		0x2 && A session was disconnected from the console terminal.
#DEFINE WTS_REMOTE_CONNECT			0x3 && A session was connected to the remote terminal.
#DEFINE WTS_REMOTE_DISCONNECT		0x4 && A session was disconnected from the remote terminal.
#DEFINE WTS_SESSION_LOGON			0x5 && A user has logged on to the session.
#DEFINE WTS_SESSION_LOGOFF			0x6 && A user has logged off the session.
#DEFINE WTS_SESSION_LOCK			0x7 && A session has been locked.
#DEFINE WTS_SESSION_UNLOCK			0x8	&& A session has been unlocked.
#DEFINE WTS_SESSION_REMOTE_CONTROL	0x9	&& A session has changed its remote controlled status. To determine the status, call GetSystemMetrics and check the SM_REMOTECONTROL metric.

&& some common defines for winapi development
#DEFINE FALSE	0
#DEFINE TRUE	1
#DEFINE INVALID_HANDLE_VALUE -1

&& defines for OpenFileMapping and MapViewOfFile
#DEFINE FILE_MAP_COPY	 		0x0001
#DEFINE FILE_MAP_WRITE			0x0002
#DEFINE FILE_MAP_READ			0x0004
#DEFINE FILE_MAP_ALL_ACCESS		0x000F001F
#DEFINE FILE_MAP_EXECUTE		0x0020

&& defines for MsgWaitForMultipleObjects
#DEFINE QS_KEY				0x0001
#DEFINE QS_MOUSEMOVE		0x0002
#DEFINE QS_MOUSEBUTTON		0x0004
#DEFINE QS_POSTMESSAGE		0x0008
#DEFINE QS_TIMER			0x0010
#DEFINE QS_PAINT			0x0020
#DEFINE QS_SENDMESSAGE		0x0040
#DEFINE QS_HOTKEY			0x0080
#DEFINE QS_ALLPOSTMESSAGE	0x0100
#DEFINE QS_RAWINPUT			0x0400
#DEFINE QS_MOUSE			0x0006
#DEFINE QS_INPUT			0x0407
#DEFINE QS_ALLEVENTS		0x04BF
#DEFINE QS_ALLINPUT			0x04FF

#DEFINE WAIT_OBJECT_0		0
#DEFINE WAIT_FAILED			0xFFFFFFFF
#DEFINE WAIT_TIMEOUT		258
#DEFINE INFINITE			0xFFFFFFFF

#ENDIF && _VFPSERVICE_H__