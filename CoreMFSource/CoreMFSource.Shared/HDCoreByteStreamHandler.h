/*
 - AVMediaFormat Demuxer Media Foundation Wrapper -
   - Author: ShanYe (K.F Yang)

   - Module: HDCoreByteStreamHandler
   - Description: Process IO and Open Media Stream.
*/

#pragma once

#include "pch.h"
#include "HDMediaSource.h"
#include <mutex>

#define HDCORE_BYTE_STREAM_HANDLER_UUID "1A0DFC9E-009C-4266-ADFF-CA37D7F8E450"

#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
class DECLSPEC_UUID(HDCORE_BYTE_STREAM_HANDLER_UUID)
	HDCoreByteStreamHandler WrlSealed : 
	public Microsoft::WRL::RuntimeClass<
		Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::WinRtClassicComMix>,
		ABI::Windows::Media::IMediaExtension,
		IMFByteStreamHandler,
		IMFAsyncCallback> {
	InspectableClass(L"CoreMFSource.HDCoreByteStreamHandler",BaseTrust)
#else
class DECLSPEC_UUID(HDCORE_BYTE_STREAM_HANDLER_UUID)
	HDCoreByteStreamHandler WrlSealed : 
	public Microsoft::WRL::RuntimeClass<
		Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::ClassicCom>,
		IMFByteStreamHandler,
		IMFAsyncCallback> {
#endif

public:
	HDCoreByteStreamHandler() = default;

#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
public: //IMediaExtension (Windows Runtime)
	IFACEMETHOD (SetProperties) (ABI::Windows::Foundation::Collections::IPropertySet*)
	{
		return S_OK;
	}
#endif

public: //IMFAsyncCallback
	STDMETHODIMP GetParameters(DWORD*,DWORD*) { return E_NOTIMPL; }
	STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult);

public: //IMFByteStreamHandler
	STDMETHODIMP BeginCreateObject(
		/* [in] */ IMFByteStream *pByteStream,
		/* [in] */ LPCWSTR pwszURL,
		/* [in] */ DWORD dwFlags,
		/* [in] */ IPropertyStore *pProps,
		/* [out] */ IUnknown **ppIUnknownCancelCookie,
		/* [in] */ IMFAsyncCallback *pCallback,
		/* [in] */ IUnknown *punkState);

	STDMETHODIMP EndCreateObject(
		/* [in] */ IMFAsyncResult *pResult,
		/* [out] */ MF_OBJECT_TYPE *pObjectType,
		/* [out] */ IUnknown **ppObject);

	STDMETHODIMP CancelObjectCreation(IUnknown*) { return E_NOTIMPL; }
	STDMETHODIMP GetMaxNumberOfBytesRequiredForResolution(QWORD* pcb)
	{
		if (pcb == nullptr)
			return E_POINTER;
		*pcb = 1024;
		return S_OK;
	}

private:
	std::recursive_mutex _mutex;

	ComPtr<HDMediaSource> _mediaSource;
	ComPtr<IMFAsyncResult> _openResult;
};