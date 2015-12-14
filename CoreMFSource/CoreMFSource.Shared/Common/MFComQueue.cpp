#include "MFComQueue.h"

using namespace WMF;
using namespace Microsoft::WRL;

ComQueue::ComQueue(BOOL bThreadSafety) throw()
{
	_hrInit = MFCreateCollection(_pCollection.GetAddressOf());

	if (bThreadSafety)
		_cs = std::make_unique<CritSec>();

	_safe_lock = bThreadSafety;
}

ComQueue::ComQueue(const ComQueue& r) throw()
{
	if (r._safe_lock)
		_cs = std::make_unique<CritSec>();

	r._pCollection.Get()->AddRef();
	_pCollection.Attach(r._pCollection.Get());

	_safe_lock = r._safe_lock;
}

ComQueue::ComQueue(ComQueue&& r) throw()
{
	if (r._safe_lock)
		_cs = std::make_unique<CritSec>();

	_pCollection.Attach(r._pCollection.Get());

	_safe_lock = r._safe_lock;
}

IUnknown* ComQueue::operator[](std::size_t index) throw()
{
	if (FAILED(_hrInit))
		return nullptr;

	DWORD dwCount = 0;
	GetCount(&dwCount);
	if (index > dwCount)
		return nullptr;

	IUnknown* pUnk = nullptr;
	if (FAILED(GetInterface(index,&pUnk)))
		return nullptr;

	pUnk->Release();
	return pUnk;
}

//////////////// Nude Methods ////////////////
HRESULT ComQueue::PushInterface(IUnknown* pUnk) throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalPushInterface(pUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalPushInterface(pUnk);
}

HRESULT ComQueue::PushInterfaceWithNoAddRef(IUnknown* pUnk) throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalPushInterfaceWithNoAddRef(pUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalPushInterfaceWithNoAddRef(pUnk);
}

HRESULT ComQueue::PopInterface(IUnknown** ppUnk) throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalPopInterface(ppUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalPopInterface(ppUnk);
}

HRESULT ComQueue::RemoveByIndex(DWORD dwIndex) throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalRemoveByIndex(dwIndex);

	CritSec::AutoLock lock(_cs.get());
	return InternalRemoveByIndex(dwIndex);
}

HRESULT ComQueue::GetFirstInterface(IUnknown** ppUnk) const throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalGetFirstInterface(ppUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalGetFirstInterface(ppUnk);
}

HRESULT ComQueue::GetLastInterface(IUnknown** ppUnk) const throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalGetLastInterface(ppUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalGetLastInterface(ppUnk);
}

HRESULT ComQueue::GetInterface(DWORD dwIndex,IUnknown** ppUnk) const throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalGetInterface(dwIndex,ppUnk);

	CritSec::AutoLock lock(_cs.get());
	return InternalGetInterface(dwIndex,ppUnk);
}

HRESULT ComQueue::GetCount(PDWORD pdwCount) const throw()
{
	if (FAILED(_hrInit))
		return _hrInit;

	if (!_safe_lock)
		return InternalGetCount(pdwCount);

	CritSec::AutoLock lock(_cs.get());
	return InternalGetCount(pdwCount);
}

HRESULT ComQueue::Clear() throw()
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
HRESULT ComQueue::InternalPushInterface(IUnknown* pUnk) throw()
{
	if (pUnk == nullptr)
		return E_INVALIDARG;

	return _pCollection->AddElement(pUnk);
}

HRESULT ComQueue::InternalPushInterfaceWithNoAddRef(IUnknown* pUnk) throw()
{
	if (pUnk == nullptr)
		return E_INVALIDARG;

	HRESULT hr = _pCollection->AddElement(pUnk);
	if (FAILED(hr))
		return hr;

	pUnk->Release();
	return hr;
}

HRESULT ComQueue::InternalPopInterface(IUnknown** ppUnk) throw()
{
	if (ppUnk == nullptr)
		return E_POINTER;

	DWORD dwCount = 0;
	_pCollection->GetElementCount(&dwCount);
	if (dwCount == 0)
		return E_FAIL;

	return _pCollection->RemoveElement(0,ppUnk);
}

HRESULT ComQueue::InternalRemoveByIndex(DWORD dwIndex) throw()
{
	Microsoft::WRL::ComPtr<IUnknown> pUnk;

	DWORD dwCount = 0;
	_pCollection->GetElementCount(&dwCount);
	if (dwCount == 0)
		return E_FAIL;

	return _pCollection->RemoveElement(dwIndex,pUnk.GetAddressOf());
}

HRESULT ComQueue::InternalGetFirstInterface(IUnknown** ppUnk) const throw()
{
	return InternalGetInterface(0,ppUnk);
}

HRESULT ComQueue::InternalGetLastInterface(IUnknown** ppUnk) const throw()
{
	if (ppUnk == nullptr)
		return E_POINTER;

	DWORD dwCount = 0;
	_pCollection->GetElementCount(&dwCount);
	if (dwCount == 0)
		return E_FAIL;

	return _pCollection->GetElement(dwCount - 1,ppUnk);
}

HRESULT ComQueue::InternalGetInterface(DWORD dwIndex,IUnknown** ppUnk) const throw()
{
	if (ppUnk == nullptr)
		return E_POINTER;

	DWORD dwCount = 0;
	_pCollection->GetElementCount(&dwCount);
	if (dwCount == 0)
		return E_FAIL;

	return _pCollection->GetElement(dwIndex,ppUnk);
}

HRESULT ComQueue::InternalGetCount(PDWORD pdwCount) const throw()
{
	if (pdwCount == nullptr)
		return E_POINTER;

	return _pCollection->GetElementCount(pdwCount);
}

HRESULT ComQueue::InternalClear() throw()
{
	return _pCollection->RemoveAllElements();
}
//////////////// Internal Methods ////////////////