#include "MFComList.h"

using namespace WMF;
using namespace Microsoft::WRL;

ComList::ComList(BOOL bThreadSafety) throw()
{
	_hrInit = MFCreateCollection(_pCollection.GetAddressOf());

	if (bThreadSafety)
		_cs = std::make_unique<CritSec>();

	_safe_lock = bThreadSafety;
}

ComList::ComList(const ComList& r) throw()
{
	if (r._safe_lock)
		_cs = std::make_unique<CritSec>();

	r._pCollection.Get()->AddRef();
	_pCollection.Attach(r._pCollection.Get());

	_safe_lock = r._safe_lock;
}

ComList::ComList(ComList&& r) throw()
{
	if (r._safe_lock)
		_cs = std::make_unique<CritSec>();

	_pCollection.Attach(r._pCollection.Get());

	_safe_lock = r._safe_lock;
}

IUnknown* ComList::operator[](std::size_t index) throw()
{
	if (FAILED(_hrInit))
		return nullptr;

	DWORD dwCount = 0;
	GetCount(&dwCount);
	if (index > dwCount)
		return nullptr;

	IUnknown* pUnk = nullptr;
	if (FAILED(GetAt(index,&pUnk)))
		return nullptr;

	pUnk->Release();
	return pUnk;
}

//////////////// Nude Methods ////////////////
HRESULT ComList::InsertFront(IUnknown* pUnk) throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalInsertFront(pUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalInsertFront(pUnk);
}

HRESULT ComList::InsertBack(IUnknown* pUnk) throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalInsertBack(pUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalInsertBack(pUnk);
}

HRESULT ComList::InsertAt(DWORD dwIndex,IUnknown* pUnk) throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalInsertAt(dwIndex,pUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalInsertAt(dwIndex,pUnk);
}

HRESULT ComList::RemoveFront(IUnknown** ppUnk) throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalRemoveFront(ppUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalRemoveFront(ppUnk);
}

HRESULT ComList::RemoveBack(IUnknown** ppUnk) throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalRemoveBack(ppUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalRemoveBack(ppUnk);
}

HRESULT ComList::RemoveAt(DWORD dwIndex,IUnknown** ppUnk) throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalRemoveAt(dwIndex,ppUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalRemoveAt(dwIndex,ppUnk);
}

HRESULT ComList::GetFront(IUnknown** ppUnk) const throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalGetFront(ppUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalGetFront(ppUnk);
}

HRESULT ComList::GetBack(IUnknown** ppUnk) const throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalGetBack(ppUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalGetBack(ppUnk);
}

HRESULT ComList::GetAt(DWORD dwIndex,IUnknown** ppUnk) const throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalGetAt(dwIndex,ppUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalGetAt(dwIndex,ppUnk);
}

HRESULT ComList::GetCount(PDWORD pdwCount) const throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalGetCount(pdwCount);

	CritSec::AutoLock lock(_cs.get());
	return InternalGetCount(pdwCount);
}

HRESULT ComList::DeleteAt(DWORD dwIndex) throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalDeleteAt(dwIndex);

	CritSec::AutoLock lock(_cs.get());
	return InternalDeleteAt(dwIndex);
}

HRESULT ComList::Clear() throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalClear();

	CritSec::AutoLock lock(_cs.get());
	return InternalClear();
}
//////////////// Nude Methods ////////////////

//////////////// Internal Methods ////////////////
HRESULT ComList::InternalInsertFront(IUnknown* pUnk) throw()
{
	if (pUnk == nullptr)
		return E_INVALIDARG;

	return _pCollection->InsertElementAt(0,pUnk);
}

HRESULT ComList::InternalInsertBack(IUnknown* pUnk) throw()
{
	if (pUnk == nullptr)
		return E_INVALIDARG;

	return _pCollection->AddElement(pUnk);
}

HRESULT ComList::InternalInsertAt(DWORD dwIndex,IUnknown* pUnk) throw()
{
	if (pUnk == nullptr)
		return E_INVALIDARG;

	return _pCollection->InsertElementAt(dwIndex,pUnk);
}

HRESULT ComList::InternalRemoveFront(IUnknown** ppUnk) throw()
{
	if (ppUnk == nullptr)
		return E_POINTER;

	DWORD dwCount = 0;
	InternalGetCount(&dwCount);
	if (dwCount == 0)
		return E_FAIL;

	return _pCollection->RemoveElement(0,ppUnk);
}

HRESULT ComList::InternalRemoveBack(IUnknown** ppUnk) throw()
{
	if (ppUnk == nullptr)
		return E_POINTER;

	DWORD dwCount = 0;
	InternalGetCount(&dwCount);
	if (dwCount == 0)
		return E_FAIL;

	return _pCollection->RemoveElement(dwCount - 1,ppUnk);
}

HRESULT ComList::InternalRemoveAt(DWORD dwIndex,IUnknown** ppUnk) throw()
{
	if (ppUnk == nullptr)
		return E_POINTER;

	DWORD dwCount = 0;
	InternalGetCount(&dwCount);
	if (dwCount == 0 ||
		dwIndex > dwCount)
		return E_FAIL;

	return _pCollection->RemoveElement(dwIndex,ppUnk);
}

HRESULT ComList::InternalGetFront(IUnknown** ppUnk) const throw()
{
	if (ppUnk == nullptr)
		return E_POINTER;

	DWORD dwCount = 0;
	InternalGetCount(&dwCount);
	if (dwCount == 0)
		return E_FAIL;

	return _pCollection->GetElement(0,ppUnk);
}

HRESULT ComList::InternalGetBack(IUnknown** ppUnk) const throw()
{
	if (ppUnk == nullptr)
		return E_POINTER;

	DWORD dwCount = 0;
	InternalGetCount(&dwCount);
	if (dwCount == 0)
		return E_FAIL;

	return _pCollection->GetElement(dwCount - 1,ppUnk);
}

HRESULT ComList::InternalGetAt(DWORD dwIndex,IUnknown** ppUnk) const throw()
{
	if (ppUnk == nullptr)
		return E_POINTER;

	DWORD dwCount = 0;
	InternalGetCount(&dwCount);
	if (dwCount == 0 ||
		dwIndex > dwCount)
		return E_FAIL;

	return _pCollection->GetElement(dwIndex,ppUnk);
}

HRESULT ComList::InternalGetCount(PDWORD pdwCount) const throw()
{
	if (pdwCount == nullptr)
		return E_POINTER;

	return _pCollection->GetElementCount(pdwCount);
}

HRESULT ComList::InternalDeleteAt(DWORD dwIndex) throw()
{
	Microsoft::WRL::ComPtr<IUnknown> pUnk;
	return InternalRemoveAt(dwIndex,pUnk.GetAddressOf());
}

HRESULT ComList::InternalClear() throw()
{
	return _pCollection->RemoveAllElements();
}
//////////////// Internal Methods ////////////////