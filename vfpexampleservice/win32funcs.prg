FUNCTION GetLastError()
	DECLARE INTEGER GetLastError IN WIN32API
	RETURN GetLastError()
ENDFUNC

FUNCTION CloseHandle(hObject)
	DECLARE INTEGER CloseHandle IN WIN32API INTEGER hObject
	RETURN CloseHandle(m.hObject)
ENDFUNC

FUNCTION WaitForSingleObject(hHandle, dwMilliseconds)
	DECLARE INTEGER WaitForSingleObject IN WIN32API INTEGER hHandle, INTEGER dwMilliseconds
	RETURN WaitForSingleObject(m.hHandle, m.dwMilliseconds) 
ENDFUNC

FUNCTION WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds)
	DECLARE INTEGER WaitForMultipleObjects IN WIN32API INTEGER nCount, STRING lpHandles, INTEGER bWaitAll, INTEGER dwMilliseconds
	RETURN WaitForMultipleObjects(m.nCount, m.lpHandles, m.bWaitAll, m.dwMilliseconds)
ENDFUNC

FUNCTION CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName)
	DECLARE INTEGER CreateEvent IN WIN32API INTEGER lpEventAttributes, INTEGER bManualReset, INTEGER bInitialState, STRING lpName
	RETURN CreateEvent(m.lpEventAttributes, m.bManualReset, m.bInitialState, m.lpName)
ENDFUNC

FUNCTION SetEvent(hEvent)
	DECLARE INTEGER SetEvent IN WIN32API INTEGER hEvent
	RETURN SetEvent(m.hEvent)
ENDFUNC

FUNCTION CreateWaitableTimer(lpTimerAttributes, bManualReset, lpTimerName)
	DECLARE INTEGER CreateWaitableTimer IN WIN32API INTEGER lpTimerAttributes, INTEGER bManualReset, STRING lpTimerName
	RETURN CreateWaitableTimer(m.lpTimerAttributes, m.bManualReset, m.lpTimerName)
ENDFUNC

FUNCTION SetWaitableTimer(hTimer, pDueTime, lPeriod, pfnCompletionRoutine, lpArgToCompletionRoutine, fResume)
	DECLARE INTEGER SetWaitableTimer IN WIN32API INTEGER hTimer, INTEGER pDueTime, INTEGER lPeriod, INTEGER pfnCompletionRoutine, ;
				INTEGER lpArgToCompletionRoutine, INTEGER fResume
	RETURN SetWaitableTimer(m.hTimer, m.pDueTime, m.lPeriod, m.pfnCompletionRoutine, m.lpArgToCompletionRoutine, m.fResume)
ENDFUNC

FUNCTION CancelWaitableTimer(hTimer)
	DECLARE INTEGER CancelWaitableTimer IN WIN32API INTEGER hTimer
	RETURN CancelWaitableTimer(m.hTimer)
ENDFUNC

FUNCTION DebugOutput(lpOutputString)
	OutputDebugString(m.lpOutputString + CHR(10))
ENDFUNC

FUNCTION OutputDebugString(lpOutputString)
	DECLARE OutputDebugString IN WIN32API STRING lpOutputString
	OutputDebugString(m.lpOutputString)
ENDFUNC
