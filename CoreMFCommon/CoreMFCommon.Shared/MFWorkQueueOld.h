#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MF_WORK_QUEUE_OLD_H
#define __MF_WORK_QUEUE_OLD_H

#include "MFCommon.h"

#if WINAPI_PARTITION_DESKTOP
namespace WMF{

template<class T>
class AutoWorkQueueOld : public IUnknown
{
public:
	AutoWorkQueueOld(T* parent,IMFAsyncCallback* callback) throw()
	{
		HRESULT hr = MFLockPlatform();
		if (SUCCEEDED(hr))
			hr = MFAllocateWorkQueue(&_dwWorkQueue);

		if (FAILED(hr))
			_dwWorkQueue = MFASYNC_CALLBACK_QUEUE_STANDARD;

		_invokeCallback.SetCallback(this,&AutoWorkQueueOld::OnInvoke);

		_parent = parent;
		_callback = callback;
	}

	virtual ~AutoWorkQueueOld() throw()
	{
		if (_dwWorkQueue != MFASYNC_CALLBACK_QUEUE_STANDARD)
		{
			MFUnlockWorkQueue(_dwWorkQueue);
			MFUnlockPlatform();
		}
	}

public: //IUnknown
	STDMETHODIMP_(ULONG) AddRef() { return _parent->AddRef(); }
	STDMETHODIMP_(ULONG) Release() { return _parent->Release(); }
	STDMETHODIMP QueryInterface(REFIID iid,void** ppv) { return E_NOINTERFACE; }

public:
	DWORD GetWorkQueue() const throw()
	{
		return _dwWorkQueue;
	}

	HRESULT PutWorkItem(IMFAsyncCallback* pCallback,IUnknown* pState) throw()
	{
		CritSec::AutoLock lock(_cs);

		ComPtr<IMFAsyncResult> pResult;
		HRESULT hr = MFCreateAsyncResult(nullptr,&_invokeCallback,pState,pResult.GetAddressOf());
		if (FAILED(hr))
			return hr;

		hr = _reqQueue.PushInterface(pResult.Get());
		if (FAILED(hr))
			return hr;

		return MFPutWorkItemEx(_dwWorkQueue,pResult.Get());
	}

private:
	HRESULT OnInvoke(IMFAsyncResult*)
	{
		while (1)
		{
			ComPtr<IMFAsyncResult> pResult;
			{
				CritSec::AutoLock lock(_cs);

				DWORD dwCount = 0;
				_reqQueue.GetCount(&dwCount);
				if (dwCount == 0)
					break;

				HRESULT hr = _reqQueue.PopInterface((IUnknown**)pResult.GetAddressOf());
				if (FAILED(hr))
					continue;
			}

			_callback->Invoke(pResult.Get());
		}

		return S_OK;
	}

private:
	T* _parent;
	IMFAsyncCallback* _callback;
	
	DWORD _dwWorkQueue;
	AsyncCallback<AutoWorkQueueOld<T>> _invokeCallback;
	ComQueue _reqQueue;
	CritSec _cs;
};

} //namespace WMF
#endif

#endif //__MF_WORK_QUEUE_OLD_H