#include "HDMediaStream.h"

HRESULT HDMediaStream::BeginGetEvent(IMFAsyncCallback *pCallback,IUnknown *punkState)
{
	SourceAutoLock lock(_pMediaSource.Get());

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	return _pEventQueue->BeginGetEvent(pCallback,punkState);
}

HRESULT HDMediaStream::EndGetEvent(IMFAsyncResult *pResult,IMFMediaEvent **ppEvent)
{
	SourceAutoLock lock(_pMediaSource.Get());

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	return _pEventQueue->EndGetEvent(pResult,ppEvent);
}

HRESULT HDMediaStream::GetEvent(DWORD dwFlags,IMFMediaEvent **ppEvent)
{
	ComPtr<IMFMediaEventQueue> pEventQueue;

	{
		SourceAutoLock lock(_pMediaSource.Get());

		HRESULT hr = CheckShutdown();
		if (FAILED(hr))
			return hr;

		pEventQueue = _pEventQueue;
	}

	return pEventQueue->GetEvent(dwFlags,ppEvent);
}

HRESULT HDMediaStream::QueueEvent(MediaEventType met,REFGUID guidExtendedType,HRESULT hrStatus,const PROPVARIANT *pvValue)
{
	SourceAutoLock lock(_pMediaSource.Get());

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	return _pEventQueue->QueueEventParamVar(met,guidExtendedType,hrStatus,pvValue);
}