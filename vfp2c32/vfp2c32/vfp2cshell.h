#ifndef _VFP2CSHELL_H__
#define _VFP2CSHELL_H__

#include <shlobj.h>
#include <shobjidl.h>

class CPropVariant : public PROPVARIANT {
public:
	CPropVariant() { PropVariantInit(this); }
	~CPropVariant() { PropVariantClear(this); }
};

template<class Derived>
class CDispatchBase : public IDispatch
{
private:
	CDispatchBase() : m_RefCount(1) {};
	friend Derived;

public:
	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
	{
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
		{
			*ppvObject = NULL;
			hr = E_NOINTERFACE;
		}
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
			delete static_cast<Derived*>(this);
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
		return static_cast<Derived*>(this)->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
	}

	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
		DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
		return static_cast<Derived*>(this)->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
	}

protected:
	long m_RefCount;	// reference count
};

class CPropertyDescription : public CDispatchBase<CPropertyDescription>
{
public:
	CPropertyDescription(IPropertyDescription* pDesc);
	virtual ~CPropertyDescription() {};

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
	virtual ~CPropertyDescriptionList() {};

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
	virtual ~CPropertyDescriptionListEnumerator() { };

	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(Next)(ULONG celt, VARIANT* rgVar, ULONG* pCeltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IEnumVARIANT** ppEnum);

	static HRESULT Create(IPropertyDescriptionList* pList, IUnknown** pUnk);
	static HRESULT Create(CPropertyDescriptionListEnumerator& pPropList, IUnknown** pUnk);
private:
	ComPtr<IPropertyDescriptionList*> m_List;
	UINT m_Index;
	long m_RefCount;	// reference count
};

class CPropertyStore : public CDispatchBase<CPropertyStore>
{
public:
	CPropertyStore(IPropertyStore* pStore);
	virtual ~CPropertyStore() {};

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
	virtual ~CShellItem() {};

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
typedef BOOL(_stdcall *PGETSPECIALFOLDER)(HWND, LPSTR, int, BOOL); // SHGetSpecialFolderPathA
typedef HRESULT(_stdcall *PSHILCREATEFROMPATH)(LPCWSTR, LPITEMIDLIST*, DWORD*); // SHILCreateFromPath
typedef HRESULT(_stdcall *PSHCREATEITEMFROMPARSINGNAME)(PCWSTR, IBindCtx* pbc, REFIID, void**); // SHCreateItemFromParsingName
typedef LPITEMIDLIST(_stdcall *PSHILCREATEFROMPATHEX)(LPCWSTR); // undocumented func #162
#define SHILCREATEFROMPATHEXID	162

// typedef's for runtime dynamic linking to propsys.dll
typedef HRESULT(_stdcall *PPSGETPROPERTYKEYFROMNAME)(PCWSTR, PROPERTYKEY*); // PSGetPropertyKeyFromName
typedef HRESULT(_stdcall *PPROPVARIANTTOVARIANT)(PROPVARIANT*, VARIANT*); // PropVariantToVariant
typedef HRESULT(_stdcall* PVARIANTTOPROPVARIANT)(VARIANT*, PROPVARIANT*); // VariantToPropVariant

#define FOF_SOURCE_CURSOR		0x010000
#define FOF_SOURCE_ARRAY		0x020000
#define FOF_DEST_CURSOR			0x040000
#define FOF_DEST_ARRAY			0x080000
#define FOF_MAPPING_ARRAY		0x100000

typedef struct _SHNAMEMPAPPINGS
{
	UINT              uNumberOfMappings;  // Number of mappings in the array.
	LPSHNAMEMAPPINGW  lpSHNameMapping;    // Pointer to the array of mappings.
} *LPSHNAMEMAPPINGS;

// shell api wrappers
void _fastcall SHSpecialFolder(ParamBlkEx& parm);
void _fastcall SHMoveFiles(ParamBlkEx& parm);
void _fastcall SHCopyFiles(ParamBlkEx& parm);
void _fastcall SHDeleteFiles(ParamBlkEx& parm);
void _fastcall SHRenameFiles(ParamBlkEx& parm);
void _fastcall SHBrowseFolder(ParamBlkEx& parm);
void _fastcall SHGetShellItem(ParamBlkEx& parm);

void _fastcall SHCreateFileListCursor(FoxString& pSource, FoxString& pFileList);
void _fastcall SHCreateFileListArray(FoxParameterEx& pSource, FoxString& pFileList);
void _fastcall SHSaveFileMapping(SHFILEOPSTRUCT& sFileOp, FoxString& pParm, bool bMapToArray);
int _stdcall SHBrowseCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

#endif // _VFP2CSHELL_H__