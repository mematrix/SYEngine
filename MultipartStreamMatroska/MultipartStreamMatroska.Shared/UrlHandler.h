/* ---------------------------------------------------------------
 -
 - 实现IMFSchemeHandler，用于piepline根据uri创建IMFByteStream对象。
 - 创建于：2015-09-18
 - 作者：ShanYe
 --------------------------------------------------------------- */

#pragma once

#include "pch.h"
#include "ThreadImpl.h"
#include "MultipartStream.h"
#include <windows.media.h> //for WinRT-IMediaExtension

#define _URL_HANDLER_GUID "2A0DFC9E-009C-4266-ADFF-CA37D7F8E450"
#define _URL_HANDLER_NAME L"MultipartStreamMatroska.UrlHandler"

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
class DECLSPEC_UUID(_URL_HANDLER_GUID) UrlHandler : 
	public RuntimeClass<RuntimeClassFlags<RuntimeClassType::ClassicCom>,IMFSchemeHandler>,
	protected ThreadImpl {
#else
class DECLSPEC_UUID(_URL_HANDLER_GUID) UrlHandler WrlSealed : 
	public RuntimeClass<RuntimeClassFlags<RuntimeClassType::WinRtClassicComMix>,
	ABI::Windows::Media::IMediaExtension,IMFSchemeHandler>,
	protected ThreadImpl {
	InspectableClass(_URL_HANDLER_NAME,BaseTrust)
#endif

public:
	UrlHandler() {}
	virtual ~UrlHandler() {}

	STDMETHODIMP BeginCreateObject(
		LPCWSTR pwszURL,
		DWORD dwFlags,
		IPropertyStore *pProps,
		IUnknown **ppIUnknownCancelCookie,
		IMFAsyncCallback *pCallback,
		IUnknown *punkState);

	STDMETHODIMP EndCreateObject(
		 IMFAsyncResult *pResult,
		 MF_OBJECT_TYPE *pObjectType,
		 IUnknown **ppObject);

	STDMETHODIMP CancelObjectCreation(IUnknown*) { return E_NOTIMPL; }

#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
	//Windows Runtime...
	IFACEMETHOD (SetProperties) (ABI::Windows::Foundation::Collections::IPropertySet*)
	{ return S_OK; }
#endif

protected:
	virtual void ThreadInvoke(void*);
	virtual void ThreadEnded() { Release(); }

private:
	std::recursive_mutex _mutex;

	WCHAR _list_file[MAX_PATH];
	ComPtr<IUnknown> _stream;
	ComPtr<IMFAsyncResult> _result;
};