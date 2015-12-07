#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MF_COM_QUEUE_H
#define __MF_COM_QUEUE_H

#include <memory>

#include "CritSec.h"
#include "MFCommon.h"

namespace WMF{

class ComQueue
{
public:
	explicit ComQueue(BOOL bThreadSafety = FALSE) throw();
	explicit ComQueue(const ComQueue& r) throw();
	explicit ComQueue(ComQueue&& r) throw();

	IUnknown* operator[](std::size_t index) throw();

	ComQueue& operator=(const ComQueue&) = delete;
	ComQueue& operator=(ComQueue&&) = delete;

public:
	HRESULT PushInterface(IUnknown* pUnk) throw();
	HRESULT PushInterfaceWithNoAddRef(IUnknown* pUnk) throw();
	HRESULT PopInterface(IUnknown** ppUnk) throw();
	HRESULT RemoveByIndex(DWORD dwIndex) throw();
	HRESULT GetFirstInterface(IUnknown** ppUnk) const throw();
	HRESULT GetLastInterface(IUnknown** ppUnk) const throw();
	HRESULT GetInterface(DWORD dwIndex,IUnknown** ppUnk) const throw();
	HRESULT GetCount(PDWORD pdwCount) const throw();
	HRESULT Clear() throw();

protected:
	HRESULT InternalPushInterface(IUnknown* pUnk) throw();
	HRESULT InternalPushInterfaceWithNoAddRef(IUnknown* pUnk) throw();
	HRESULT InternalPopInterface(IUnknown** ppUnk) throw();
	HRESULT InternalRemoveByIndex(DWORD dwIndex) throw();
	HRESULT InternalGetFirstInterface(IUnknown** ppUnk) const throw();
	HRESULT InternalGetLastInterface(IUnknown** ppUnk) const throw();
	HRESULT InternalGetInterface(DWORD dwIndex,IUnknown** ppUnk) const throw();
	HRESULT InternalGetCount(PDWORD pdwCount) const throw();
	HRESULT InternalClear() throw();

private:
	HRESULT _hrInit;
	BOOL _safe_lock;

	Microsoft::WRL::ComPtr<IMFCollection> _pCollection;
	std::unique_ptr<CritSec> _cs;
};

} //namespace WMF.

#endif //__MF_COM_QUEUE_H