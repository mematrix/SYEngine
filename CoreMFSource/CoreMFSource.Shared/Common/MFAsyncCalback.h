#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MF_ASYNC_CALLBACK_H
#define __MF_ASYNC_CALLBACK_H

#include "MFCommon.h"

namespace WMF{

template<class T>
class AsyncCallback : public IMFAsyncCallback
{
public:
	typedef HRESULT (T::*InvokeCallback)(IMFAsyncResult*);

public:
	AsyncCallback() throw() : _parent(nullptr), _cb(nullptr) {}
	AsyncCallback(T* parent,InvokeCallback callback) throw() : _parent(_parent), _cb(callback) {}

public: //IUnknown
	STDMETHODIMP_(ULONG) AddRef() { return _parent->AddRef(); }
	STDMETHODIMP_(ULONG) Release() { return _parent->Release(); }
	STDMETHODIMP QueryInterface(REFIID iid,void** ppv)
	{
		if (ppv == nullptr)
			return E_POINTER;

		if (iid == IID_IUnknown)
		{
			*ppv = static_cast<IUnknown*>(this);
		}else if (iid == IID_IMFAsyncCallback)
		{
			*ppv = static_cast<IMFAsyncCallback*>(this);
		}else{
			*ppv = nullptr;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

public: //IMFAsyncCallback
	STDMETHODIMP GetParameters(DWORD*,DWORD*) { return E_NOTIMPL; }
	STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult)
	{
		if (_parent != nullptr && _cb != nullptr)
			return (_parent->*_cb)(pAsyncResult);

		return S_OK;
	}

public:
	void SetCallback(T* parent,InvokeCallback callback)
	{
		_parent = parent;
		_cb = callback;
	}

protected:
	T* _parent;
	InvokeCallback _cb;
};

} //namespace WMF.

#endif //__MF_ASYNC_CALLBACK_H