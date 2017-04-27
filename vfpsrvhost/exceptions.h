#ifndef _EXCEPTIONS_H__
#define _EXCEPTIONS_H__

#include <windows.h>
#include <iostream> 
#include <atlstr.h>
using namespace std;

class BaseException
{
public:
	BaseException(LPCTSTR pMessage) : m_Function(0), m_LastError(0), m_Message(pMessage) { }
	BaseException(LPCTSTR pFunction, DWORD nLastError) : m_Function(pFunction), m_LastError(nLastError), m_Message(0) { }
	DWORD LastError() { return m_LastError; }
	virtual void Log() = 0;

protected:
	LPCTSTR m_Message;
	LPCTSTR m_Function;
	DWORD m_LastError;
};
/*
class EventLogException : public BaseException
{
public:
	EventLogException(LPTSTR pFunction, DWORD nLastError) : BaseException(pFunction, nLastError) { }
	virtual void Log();
};
*/

class FileLogException : public BaseException
{
public:
	FileLogException(LPCTSTR pMessage) : BaseException(pMessage) { }
	FileLogException(LPCTSTR pFunction, DWORD nLastError) : BaseException(pFunction, nLastError) { }
	virtual void Log();
};

class StdoutException : public BaseException
{
public:
	StdoutException(LPCTSTR pMessage) : BaseException(pMessage) { }
	StdoutException(LPCTSTR pFunction, DWORD nLastError) : BaseException(pFunction, nLastError) { }
	virtual void Log();
};

#endif // _EXCEPTIONS_H__