#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MF_COM_LIST_H
#define __MF_COM_LIST_H

#include <memory>

#include "CritSec.h"
#include "MFCommon.h"

namespace WMF{

class ComList
{
public:
	explicit ComList(BOOL bThreadSafety = FALSE) throw();
	explicit ComList(const ComList& r) throw();
	explicit ComList(ComList&& r) throw();

	IUnknown* operator[](std::size_t index) throw();

	ComList& operator=(const ComList&) = delete;
	ComList& operator=(ComList&&) = delete;

public:
	HRESULT InsertFront(IUnknown* pUnk) throw();
	HRESULT InsertBack(IUnknown* pUnk) throw();
	HRESULT InsertAt(DWORD dwIndex,IUnknown* pUnk) throw();

	HRESULT RemoveFront(IUnknown** ppUnk) throw();
	HRESULT RemoveBack(IUnknown** ppUnk) throw();
	HRESULT RemoveAt(DWORD dwIndex,IUnknown** ppUnk) throw();

	HRESULT GetFront(IUnknown** ppUnk) const throw();
	HRESULT GetBack(IUnknown** ppUnk) const throw();
	HRESULT GetAt(DWORD dwIndex,IUnknown** ppUnk) const throw();

	HRESULT GetCount(PDWORD pdwCount) const throw();
	HRESULT DeleteAt(DWORD dwIndex) throw();
	HRESULT Clear() throw();

protected:
	HRESULT InternalInsertFront(IUnknown* pUnk) throw();
	HRESULT InternalInsertBack(IUnknown* pUnk) throw();
	HRESULT InternalInsertAt(DWORD dwIndex,IUnknown* pUnk) throw();

	HRESULT InternalRemoveFront(IUnknown** ppUnk) throw();
	HRESULT InternalRemoveBack(IUnknown** ppUnk) throw();
	HRESULT InternalRemoveAt(DWORD dwIndex,IUnknown** ppUnk) throw();

	HRESULT InternalGetFront(IUnknown** ppUnk) const throw();
	HRESULT InternalGetBack(IUnknown** ppUnk) const throw();
	HRESULT InternalGetAt(DWORD dwIndex,IUnknown** ppUnk) const throw();

	HRESULT InternalGetCount(PDWORD pdwCount) const throw();
	HRESULT InternalDeleteAt(DWORD dwIndex) throw();
	HRESULT InternalClear() throw();

private:
	HRESULT _hrInit;
	BOOL _safe_lock;

	Microsoft::WRL::ComPtr<IMFCollection> _pCollection;
	std::unique_ptr<CritSec> _cs;
};

} //namespace WMF.

#endif //__MF_COM_LIST_H