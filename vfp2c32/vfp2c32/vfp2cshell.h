#ifndef _VFP2CSHELL_H__
#define _VFP2CSHELL_H__

#include <shlobj.h>
#include <shobjidl.h>

class CPropVariant : public PROPVARIANT {
public:
	CPropVariant() { PropVariantInit(this); }
	~CPropVariant() { PropVariantClear(this); }
};

template<class T>
class CDispatchBase : public IDispatch
{
public:
	CDispatchBase() : m_RefCount(1) {};

	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
	{
		*ppvObject = NULL;
		HRESULT hr;

		// IUnknown
		if (::IsEqualIID(riid, __uuidof(IUnknown)))
		{
			*ppvObject = this;
			hr = S_OK;
		}
		// IDispatch
		else if (::IsEqualIID(riid, __uuidof(IDispatch)))
		{
			*ppvObject = static_cast<IDispatch*>(this);
			hr = S_OK;
		}
		else
			hr = E_NOINTERFACE;

		return hr;
	};

	STDMETHOD_(ULONG, AddRef)()
	{
		return ++m_RefCount;
	};

	STDMETHOD_(ULONG, Release)()
	{
		m_RefCount--;
		if (m_RefCount == 0)
		{
			delete static_cast<T*>(this);
			return 0;
		}
		else
			return m_RefCount;
	};

	// IDispatch methods
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
	{
		*pctinfo = 0;
		return S_OK;
	}
	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{
		return E_NOTIMPL;
	};

	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
	{
		return static_cast<T*>(this)->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
	}

	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
		DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
		return static_cast<T*>(this)->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
	}

protected:
	long m_RefCount;	// reference count
};

class CPropertyDescription : public CDispatchBase<CPropertyDescription>
{
public:
	CPropertyDescription(IPropertyDescription* pDesc);

	// IDispatch methods
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
		DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

private:
	ComPtr<IPropertyDescription*> m_Desc;
	static const DISPID DISPID_GetCanonicalName = 1;
	static const DISPID DISPID_GetPropertyType = 2;
	static const DISPID DISPID_GetDisplayName = 3;
};

class CPropertyDescriptionList : public CDispatchBase<CPropertyDescriptionList>
{
public:
	CPropertyDescriptionList(IPropertyDescriptionList* pList);

	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
		DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);
private:
	ComPtr<IPropertyDescriptionList*> m_List;
	static const DISPID DISPID_GetAt = 1;
	static const DISPID DISPID_GetCount = 2;
};

class CPropertyDescriptionListEnumerator : public IEnumVARIANT
{
public:
	CPropertyDescriptionListEnumerator(IPropertyDescriptionList* pList);
	CPropertyDescriptionListEnumerator(CPropertyDescriptionListEnumerator& pEnum);

	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(Next)(ULONG celt, VARIANT* rgVar, ULONG* pCeltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IEnumVARIANT** ppEnum);

private:
	ComPtr<IPropertyDescriptionList*> m_List;
	int m_Index;
	long m_RefCount;	// reference count
};

class CPropertyStore : public CDispatchBase<CPropertyStore>
{
public:
	CPropertyStore(IPropertyStore* pStore);

	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
		DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

private:
	ComPtr<IPropertyStore*> m_Store;
	static const DISPID DISPID_GetValue = 1;
	static const DISPID DISPID_SetValue = 2;
	static const DISPID DISPID_Commit = 3;
};

class CShellItem : public CDispatchBase<CShellItem>
{
public:
	CShellItem(IShellItem2* pShellItem);

	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid);
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
		DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

private:
	ComPtr<IShellItem2*> m_ShellItem;
	static const DISPID DISPID_GetProperty = 1;
	static const DISPID DISPID_GetPropertyDescriptionList = 2;
	static const DISPID DISPID_GetPropertyStore = 3;
};

// typedef's for runtime dynamic linking to shell32.dll
typedef BOOL(_stdcall* PGETSPECIALFOLDER)(HWND, LPSTR, int, BOOL); // SHGetSpecialFolderPathA
typedef HRESULT(_stdcall* PSHILCREATEFROMPATH)(LPCWSTR, LPITEMIDLIST*, DWORD*); // SHILCreateFromPath
typedef HRESULT(_stdcall* PSHCREATEITEMFROMPARSINGNAME)(PCWSTR, IBindCtx* pbc, REFIID, void**); // SHCreateItemFromParsingName
typedef LPITEMIDLIST(_stdcall* PSHILCREATEFROMPATHEX)(LPCWSTR); // undocumented func #162
#define SHILCREATEFROMPATHEXID	162

// shell api wrappers
void _fastcall SHSpecialFolder(ParamBlkEx& parm);
void _fastcall SHMoveFiles(ParamBlkEx& parm);
void _fastcall SHCopyFiles(ParamBlkEx& parm);
void _fastcall SHDeleteFiles(ParamBlkEx& parm);
void _fastcall SHRenameFiles(ParamBlkEx& parm);
void _fastcall SHBrowseFolder(ParamBlkEx& parm);
void _fastcall SHGetShellItem(ParamBlkEx& parm);

int _stdcall SHBrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

#endif // _VFP2CSHELL_H__