#include "pch.h"
#include "HDCoreByteStreamHandler.h"

CoCreatableClass(HDCoreByteStreamHandler);
#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
ActivatableClass(HDCoreByteStreamHandler);
#endif

HRESULT HDCoreByteStreamHandler::BeginCreateObject(IMFByteStream *pByteStream,LPCWSTR pwszURL,DWORD dwFlags,IPropertyStore *pProps,IUnknown **ppIUnknownCancelCookie,IMFAsyncCallback *pCallback,IUnknown *punkState)
{
#ifdef _DEBUG
	OutputDebugStringA("HDCoreByteStreamHandler->CreateObject...\n");
#endif

	std::lock_guard<decltype(_mutex)> lock(_mutex);

	if (pByteStream == nullptr ||
		pCallback == nullptr)
		return E_POINTER;

	if ((dwFlags & MF_RESOLUTION_MEDIASOURCE) == 0)
		return E_INVALIDARG;

	if (ppIUnknownCancelCookie)
		*ppIUnknownCancelCookie = nullptr;

	if (_openResult)
		return E_UNEXPECTED;

	double bufferTime = 5.0;
	if (pProps) {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
		PROPVARIANT var;
		PropVariantInit(&var);
		PROPERTYKEY key;
		key.fmtid = MFNETSOURCE_BUFFERINGTIME;
		key.pid = 0;
		if (SUCCEEDED(pProps->GetValue(key,&var)))
		{
			if (var.vt = VT_I4 || var.vt == VT_UI4)
				bufferTime = (double)var.lVal / 1000.0;
			if (bufferTime <= 0.0)
				bufferTime = 5.0;
		}
		PropVariantClear(&var);
#endif
	}
	if (GlobalOptionGetDouble(kNetworkPacketBufferTime) > 0.9)
		bufferTime = GlobalOptionGetDouble(kNetworkPacketBufferTime);

	ComPtr<IMFByteStreamBuffering> pIsNetwork;
	HRESULT hrIsNetwork = pByteStream->QueryInterface(
		IID_PPV_ARGS(pIsNetwork.GetAddressOf()));

	auto source = HDMediaSource::CreateMediaSource(
		SUCCEEDED(hrIsNetwork) ? MF_SOURCE_STREAM_QUEUE_AUTO:
		MF_SOURCE_STREAM_QUEUE_FPS_AUTO);

	if (source == nullptr)
		return E_OUTOFMEMORY;

	if (SUCCEEDED(hrIsNetwork))
		source->SetNetworkPrerollTime(bufferTime);

	HRESULT hr = MFCreateAsyncResult(static_cast<IMFMediaSource*>(source.Get()),
		pCallback,punkState,
		_openResult.GetAddressOf());

	if (FAILED(hr))
		return hr;

	hr = source->OpenAsync(pByteStream,this);
	if (FAILED(hr))
		return hr;

	_mediaSource = source;
	return S_OK;
}

HRESULT HDCoreByteStreamHandler::EndCreateObject(IMFAsyncResult *pResult,MF_OBJECT_TYPE *pObjectType,IUnknown **ppObject)
{
	if (pResult == nullptr || pObjectType == nullptr || ppObject == nullptr)
		return E_POINTER;

	*pObjectType = MF_OBJECT_INVALID;
	*ppObject = nullptr;

	HRESULT hr = pResult->GetStatus();
	if (FAILED(hr))
		return hr;

	std::lock_guard<decltype(_mutex)> lock(_mutex);

	if (_mediaSource == nullptr)
		return E_UNEXPECTED;

	*pObjectType = MF_OBJECT_MEDIASOURCE;
	*ppObject = static_cast<IUnknown*>(
		static_cast<IMFMediaSource*>(_mediaSource.Detach()));

	_openResult.Reset();

	return S_OK;
}

HRESULT HDCoreByteStreamHandler::Invoke(IMFAsyncResult* pAsyncResult)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);

	if (_mediaSource == nullptr)
		return E_UNEXPECTED;

	_openResult->SetStatus(pAsyncResult->GetStatus());
	return MFInvokeCallback(_openResult.Get()); //Notify to System...
}