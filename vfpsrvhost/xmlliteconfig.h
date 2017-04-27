#ifndef _XMLLITECONFIG_H_
#define _XMLLITECONFIG_H_

#include <shellapi.h>
#include <oleauto.h>
#include <xmllite.h>
#pragma comment(lib, "xmllite.lib")

#define _ATL_ALL_WARNINGS 
#include <atlbase.h> 
#include <atlstr.h>

#pragma once

class XmlLiteConfig
{
public:
	XmlLiteConfig() : m_hr(S_OK), m_Function(0) { }
	~XmlLiteConfig() { }

	HRESULT GetLastError() { return m_hr; }
	LPTSTR GetLastFunction() { return m_Function; }
	virtual bool Save(LPCTSTR configfile) { return false; }
	virtual bool Load(LPCTSTR configfile) { return false; }



protected:
	HRESULT ConvertValue(LPCWSTR Value, bool &var)
	{
		var = (_wcsicmp(Value, L"true") == 0);
		return S_OK;
	}

	HRESULT ConvertValue(LPCWSTR Value, int &var)
	{
		HRESULT hr = VarIntFromStr(Value, LOCALE_INVARIANT, 0, (long*)&var);
		if (FAILED(hr))
			m_Function = _T("VarIntFromStr");
		return hr;
	}

	HRESULT ConvertValue(LPCWSTR Value, long &var)
	{
		HRESULT hr = VarI4FromStr(Value, LOCALE_INVARIANT, 0, &var);
		if (FAILED(hr))
			m_Function = _T("VarI4FromStr");
		return hr;
	}

	HRESULT ConvertValue(LPCWSTR Value, unsigned long &var)
	{
		HRESULT hr = VarUI4FromStr(Value, LOCALE_INVARIANT, 0, &var);
		if (FAILED(hr))
			m_Function = _T("VarUI4FromStr");
		return hr;
	}

	HRESULT ConvertValue(LPCWSTR Value, double &var)
	{
		HRESULT hr = VarR8FromStr(Value, LOCALE_INVARIANT, 0, &var);
		if (FAILED(hr))
			m_Function = _T("VarR8FromStr");
		return hr;
	}

	HRESULT ConvertValue(LPCWSTR Value, CString &var)
	{
		var = Value;
		return S_OK;
	}

	HRESULT WriteAttribute(IXmlWriter *xmlWriter, LPCWSTR pAttributeName, int value)
	{
		HRESULT hr;
		CComBSTR string;
		hr = VarBstrFromInt(value, LOCALE_INVARIANT, 0, &string);
		if (FAILED(hr))
		{
			m_Function = _T("VarBstrFromInt");
			return hr;
		}
		return xmlWriter->WriteAttributeString(NULL, pAttributeName, NULL, string);
	}

	HRESULT WriteAttribute(IXmlWriter *xmlWriter, LPCWSTR pAttributeName, long value)
	{
		HRESULT hr;
		CComBSTR string;
		hr = VarBstrFromInt(value, LOCALE_INVARIANT, 0, &string);
		if (FAILED(hr))
		{
			m_Function = _T("VarBstrFromInt");
			return hr;
		}
		return xmlWriter->WriteAttributeString(NULL, pAttributeName, NULL, string);
	}

	HRESULT WriteAttribute(IXmlWriter *xmlWriter, LPCWSTR pAttributeName, unsigned long value)
	{
		HRESULT hr;
		CComBSTR string;
		hr = VarBstrFromUI4(value, LOCALE_INVARIANT, 0, &string);
		if (FAILED(hr))
		{
			m_Function = _T("VarBstrFromUI4");
			return hr;
		}
		return xmlWriter->WriteAttributeString(NULL, pAttributeName, NULL, string);
	}

	HRESULT WriteAttribute(IXmlWriter *xmlWriter, LPCWSTR pAttributeName, double value)
	{
		HRESULT hr;
		CComBSTR string;
		hr = VarBstrFromR8(value, LOCALE_INVARIANT, 0, &string);
		if (FAILED(hr))
		{
			m_Function = _T("VarBstrFromR8");
			return hr;
		}
		return xmlWriter->WriteAttributeString(NULL, pAttributeName, NULL, string);
	}

	HRESULT WriteAttribute(IXmlWriter *xmlWriter, LPCWSTR pAttributeName, CString &value)
	{
		HRESULT hr = xmlWriter->WriteAttributeString(NULL, pAttributeName, NULL, value);
		if (FAILED(hr))
			m_Function = _T("XmlWriter::WriteAttributeString");
		return hr;
	}

	HRESULT WriteAttribute(IXmlWriter *xmlWriter, LPCWSTR pAttributeName, BSTR value)
	{
		HRESULT hr = xmlWriter->WriteAttributeString(NULL, pAttributeName, NULL, value);
		if (FAILED(hr))
			m_Function = _T("XmlWriter::WriteAttributeString");
		return hr;
	}

	HRESULT WriteAttribute(IXmlWriter *xmlWriter, LPCWSTR pAttributeName, LPCSTR value)
	{
		CComBSTR string(value);
		HRESULT hr = xmlWriter->WriteAttributeString(NULL, pAttributeName, NULL, string);
		if (FAILED(hr))
			m_Function = _T("XmlWriter::WriteAttributeString");
		return hr;
	} 

	HRESULT m_hr;
	LPTSTR m_Function;
	CString m_configfile;
};

// defines for reading XML
#define BEGIN_LOAD_CONFIG() bool Load(LPCTSTR configfile) { \
		CComPtr<IStream> fileStream; \
		CComPtr<IXmlReader> xmlReader; \
		LPCWSTR pszNodeName = 0, pszValue = 0, pszAttribName = 0, pszAttribValue = 0; \
		if (FAILED(m_hr = ::SHCreateStreamOnFile(configfile, STGM_READ | STGM_SHARE_DENY_NONE, &fileStream))) \
		{ \
			m_Function = _T("SHCreateStreamOnFile"); \
			return m_hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND); \
		} \
		if (FAILED(m_hr = ::CreateXmlReader(__uuidof(IXmlReader), reinterpret_cast<LPVOID*>(&xmlReader), 0))) \
		{ \
			m_Function = _T("CreateXmlReader"); \
			return false; \
		} \
		if (FAILED(m_hr = xmlReader->SetInput(fileStream))) \
		{ \
			m_Function = _T("XmlReader::SetInput"); \
			return false; \
		}

#define END_LOAD_CONFIG() return true; }

#define BEGIN_ELEMENT_LOOP() XmlNodeType NodeType = XmlNodeType_None; \
		while(xmlReader->Read(&NodeType) == S_OK) { \
		switch(NodeType) {

#define END_ELEMENT_LOOP() } }

#define BEGIN_ON_START_ELEMENT() case XmlNodeType_Element: \
		if (FAILED(m_hr = xmlReader->GetLocalName(&pszNodeName, NULL))) \
		{ \
			m_Function = _T("XmlReader::GetLocalName"); \
			return false; \
		}

#define END_ON_START_ELEMENT() break;

#define BEGIN_ON_END_ELEMENT() case XmlNodeType_EndElement: \
		if (FAILED(m_hr = xmlReader->GetLocalName(&pszNodeName, NULL))) \
		{ \
			m_Function = _T("XmlReader::GetLocalName"); \
			return false; \
		}

#define END_ON_END_ELEMENT() break;

#define BEGIN_ELEMENT_NAME(_name) if(wcscmp(L ## _name, pszNodeName) == 0) {

#define END_ELEMENT_NAME() break; }

#define BEGIN_ATTRIBUTE_MAP() m_hr = xmlReader->MoveToFirstAttribute(); \
		while(m_hr == S_OK) \
		{ \
			if (FAILED(m_hr = xmlReader->GetLocalName(&pszAttribName, NULL))) \
			{ \
				m_Function = _T("XmlReader::GetLocalName"); \
				return false; \
			} \
			if (FAILED(m_hr = xmlReader->GetValue(&pszAttribValue, NULL))) \
			{ \
				m_Function = _T("XmlReader::GetValue"); \
				return false; \
			} \
		}

#define END_ATTRIBUTE_MAP() m_hr = xmlReader->MoveToNextAttribute(); }

#define READ_ATTRIBUTE(_attrib, _var) if( wcscmp(L ## _attrib, pszAttribName) == 0) \
		{ \
			if (FAILED(m_hr = ConvertValue(pszAttribValue, _var))) \
				return false; \
		}

#define READ_ELEMENT_VALUE(_var) if (FAILED(m_hr = xmlReader->Read(&NodeType))) \
		{ \
			m_Function = _T("XmlReader::Read"); \
			return false; \
		} \
		if (NodeType == XmlNodeType_Text) \
		{ \
			if (FAILED(m_hr = xmlReader->GetValue(&pszValue, NULL))) \
			{ \
				m_Function = _T("XmlReader::GetValue"); \
				return false; \
			} \
			if (FAILED(m_hr = ConvertValue(pszValue, _var))) \
				return false; \
		}

// defines for writing XML
#define BEGIN_SAVE_CONFIG(_rootelement) bool Save(LPCTSTR configfile) { \
		CComPtr<IXmlWriter> xmlWriter; \
		CComPtr<IStream> fileStream; \
		if (FAILED(m_hr = ::CreateXmlWriter(__uuidof(IXmlWriter), reinterpret_cast<LPVOID*>(&xmlWriter), 0))) \
		{ \
			m_Function = _T("CreateXmlWriter"); \
			return false; \
		} \
		if (FAILED(m_hr = ::SHCreateStreamOnFile(configfile, STGM_WRITE | STGM_CREATE, &fileStream))) \
		{ \
			m_Function = _T("SHCreateStreamOnFile"); \
			return false; \
		} \
		if (FAILED(m_hr = xmlWriter->SetOutput(fileStream))) \
		{ \
			m_Function = _T("XmlWriter::SetOutput"); \
			return false; \
		} \
		if (FAILED(m_hr = xmlWriter->SetProperty(XmlWriterProperty_Indent, TRUE))) \
		{ \
			m_Function = _T("XmlWriter::SetProperty"); \
			return false; \
		} \
		if (FAILED(m_hr = xmlWriter->WriteStartDocument(XmlStandalone_Omit))) \
		{ \
			m_Function = _T("XmlWriter::WriteStartDocument"); \
			return false; \
		} \
		if (FAILED(m_hr = xmlWriter->WriteStartElement(NULL, L ##_rootelement, NULL))) \
		{ \
			m_Function = _T("XmlWriter::WriteStartElement"); \
			return false; \
		}

#define END_SAVE_CONFIG() if (FAILED(m_hr = xmlWriter->WriteEndElement())) \
		{ \
			m_Function = _T("XmlWriter::WriteEndElement"); \
			return false; \
		} \
		return true; }

#define WRITE_START_ELEMENT(_name) if (FAILED(m_hr = xmlWriter->WriteStartElement(NULL, L ## _name, NULL))) \
		{ \
			m_Function = _T("XmlWriter::WriteStartElement"); \
			return false; \
		}

#define WRITE_END_ELEMENT() if (FAILED(m_hr = xmlWriter->WriteEndElement())) \
		{ \
			m_Function = _T("XmlWriter::WriteEndElement"); \
			return false; \
		}

#define WRITE_ATTRIBUTE(_name, _value) if (FAILED(m_hr = WriteAttribute(xmlWriter, L ## _name, _value))) \
		{ \
			m_Function = _T("XmlWriter::WriteAttribute"); \
			return false; \
		}

#endif // _XMLLITECONFIG_H_