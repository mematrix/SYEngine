#include "HDMediaSource.h"

HRESULT HDMediaSource::BeginGetEvent(IMFAsyncCallback *pCallback,IUnknown *punkState)
{
	CritSec::AutoLock lock(_cs);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	return _pEventQueue->BeginGetEvent(pCallback,punkState);
}

HRESULT HDMediaSource::EndGetEvent(IMFAsyncResult *pResult,IMFMediaEvent **ppEvent)
{
	CritSec::AutoLock lock(_cs);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	return _pEventQueue->EndGetEvent(pResult,ppEvent);
}

HRESULT HDMediaSource::GetEvent(DWORD dwFlags,IMFMediaEvent **ppEvent)
{
	ComPtr<IMFMediaEventQueue> pEventQueue;

	{
		CritSec::AutoLock lock(_cs);

		HRESULT hr = CheckShutdown();
		if (FAILED(hr))
			return hr;

		pEventQueue = _pEventQueue;
	}

	return pEventQueue->GetEvent(dwFlags,ppEvent);
}

HRESULT HDMediaSource::QueueEvent(MediaEventType met,REFGUID guidExtendedType,HRESULT hrStatus,const PROPVARIANT *pvValue)
{
	CritSec::AutoLock lock(_cs);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	return _pEventQueue->QueueEventParamVar(met,guidExtendedType,hrStatus,pvValue);
}