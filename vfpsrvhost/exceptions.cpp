#include "Exceptions.h"
#include "SrvHostConfig.h"

extern SrvHostConfig gConfig;
extern CString gExeFile;

/*
void EventLogException::Log()
{
    HANDLE hEventSource;
    LPCTSTR lpszStrings[3];
	CString message, message2;
    hEventSource = RegisterEventSource(NULL, gConfig.Services.[0]->ServiceName);
    if(hEventSource)
    {
		message.Format(_T("%s failed with 0x%x"), m_Function, m_LastError);
		LPTSTR buffer = message2.GetBuffer(1024);
		DWORD len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, m_LastError, 0, buffer, 1024, 0);		
		message2.ReleaseBufferSetLength(len);
        lpszStrings[0] = gConfig.Services[0].ServiceName;
        lpszStrings[1] = message;
		lpszStrings[2] = message2;
        ReportEvent(hEventSource, EVENTLOG_ERROR_TYPE, 0, 1, 0, 3, 0, lpszStrings, 0);
        DeregisterEventSource(hEventSource);
    }
}
*/

void FileLogException::Log()
{
	CString message, message2;
	CString logfile;
	DWORD bytesWritten;

	if (gExeFile.IsEmpty())
		logfile = _T("vfpsrvhost.log");
	else 
		logfile = gExeFile.Left(gExeFile.ReverseFind('.') + 1) + _T("log");

	HANDLE hFile = CreateFile(logfile, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(hFile)
    {
		if (m_Message)
		{
			WriteFile(hFile, m_Message, _tcslen(m_Message) * sizeof(TCHAR), &bytesWritten, 0);
		}
		else
		{
			LPTSTR buffer = message.GetBuffer(1024);
			DWORD len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, m_LastError, 0, buffer, 1024, 0);		 
			message.ReleaseBufferSetLength(len);
			message2.Format(_T("%s failed with 0x%x\n%s\n"), m_Function, m_LastError, (LPCTSTR)message);
			SetFilePointer(hFile, 0, 0, FILE_END);
			WriteFile(hFile, (LPCTSTR)message2, message2.GetLength() * sizeof(TCHAR), &bytesWritten, 0);
		}

		CloseHandle(hFile);
    }
}

void StdoutException::Log()
{
	if (m_Message)
		wcout << m_Message << endl;
	else
	{
		TCHAR szMessage[1024];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, m_LastError, 0, szMessage, 1024, 0);
		wcout << m_Function << _T(" failed with 0x") << hex << m_LastError << endl << szMessage << endl;
	}
}