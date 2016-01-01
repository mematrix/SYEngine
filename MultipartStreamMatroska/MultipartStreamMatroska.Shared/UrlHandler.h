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
	public RuntimeClass<RuntimeClassFlags<RuntimeClassType::ClassicCom>,IMFSchemeHandler,IMFAttributes>,
	protected ThreadImpl {
#else
class DECLSPEC_UUID(_URL_HANDLER_GUID) UrlHandler WrlSealed : 
	public RuntimeClass<RuntimeClassFlags<RuntimeClassType::WinRtClassicComMix>,
	ABI::Windows::Media::IMediaExtension,IMFSchemeHandler,IMFAttributes>,
	protected ThreadImpl {
	InspectableClass(_URL_HANDLER_NAME,BaseTrust)
#endif

public:
	UrlHandler() { MFCreateAttributes(&_attrs, 0); }
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

public: //IMFAttributes
	STDMETHODIMP GetItem(REFGUID guidKey, PROPVARIANT *pValue)
	{ return _attrs->GetItem(guidKey, pValue); }
	STDMETHODIMP GetItemType(REFGUID guidKey, MF_ATTRIBUTE_TYPE *pType)
	{ return _attrs->GetItemType(guidKey, pType); }
	STDMETHODIMP CompareItem(REFGUID guidKey, REFPROPVARIANT Value, BOOL *pbResult)
	{ return _attrs->CompareItem(guidKey, Value, pbResult); }
	STDMETHODIMP Compare(IMFAttributes *pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL *pbResult)
	{ return _attrs->Compare(pTheirs, MatchType, pbResult); }
	STDMETHODIMP GetUINT32(REFGUID guidKey, UINT32 *punValue)
	{ return _attrs->GetUINT32(guidKey, punValue); }
	STDMETHODIMP GetUINT64(REFGUID guidKey, UINT64 *punValue)
	{ return _attrs->GetUINT64(guidKey, punValue); }
	STDMETHODIMP GetDouble(REFGUID guidKey, double *pfValue)
	{ return _attrs->GetDouble(guidKey, pfValue); }
	STDMETHODIMP GetGUID(REFGUID guidKey, GUID *pguidValue)
	{ return _attrs->GetGUID(guidKey, pguidValue); }
	STDMETHODIMP GetStringLength(REFGUID guidKey, UINT32 *pcchLength)
	{ return _attrs->GetStringLength(guidKey, pcchLength); }
	STDMETHODIMP GetString(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32 *pcchLength)
	{ return _attrs->GetString(guidKey, pwszValue, cchBufSize, pcchLength); }
	STDMETHODIMP GetAllocatedString(REFGUID guidKey, LPWSTR *ppwszValue, UINT32 *pcchLength)
	{ return _attrs->GetAllocatedString(guidKey, ppwszValue, pcchLength); }
	STDMETHODIMP GetBlobSize(REFGUID guidKey, UINT32 *pcbBlobSize)
	{ return _attrs->GetBlobSize(guidKey, pcbBlobSize); }
	STDMETHODIMP GetBlob(REFGUID guidKey, UINT8 *pBuf, UINT32 cbBufSize, UINT32 *pcbBlobSize)
	{ return _attrs->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize); }
	STDMETHODIMP GetAllocatedBlob(REFGUID guidKey, UINT8 **ppBuf, UINT32 *pcbSize)
	{ return _attrs->GetAllocatedBlob(guidKey, ppBuf, pcbSize); }
	STDMETHODIMP GetUnknown(REFGUID guidKey, REFIID riid, LPVOID *ppv)
	{ return _attrs->GetUnknown(guidKey, riid, ppv); }
	STDMETHODIMP SetItem(REFGUID guidKey, REFPROPVARIANT Value)
	{ return _attrs->SetItem(guidKey, Value); }
	STDMETHODIMP DeleteItem(REFGUID guidKey)
	{ return _attrs->DeleteItem(guidKey); }
	STDMETHODIMP DeleteAllItems()
	{ return _attrs->DeleteAllItems(); }
	STDMETHODIMP SetUINT32(REFGUID guidKey, UINT32 unValue)
	{ return _attrs->SetUINT32(guidKey, unValue); }
	STDMETHODIMP SetUINT64(REFGUID guidKey, UINT64 unValue)
	{ return _attrs->SetUINT64(guidKey, unValue); }
	STDMETHODIMP SetDouble(REFGUID guidKey, double fValue)
	{ return _attrs->SetDouble(guidKey, fValue); }
	STDMETHODIMP SetGUID(REFGUID guidKey, REFGUID guidValue)
	{ return _attrs->SetGUID(guidKey, guidValue); }
	STDMETHODIMP SetString(REFGUID guidKey, LPCWSTR wszValue)
	{ return _attrs->SetString(guidKey, wszValue); }
	STDMETHODIMP SetBlob(REFGUID guidKey, const UINT8 *pBuf, UINT32 cbBufSize)
	{ return _attrs->SetBlob(guidKey, pBuf, cbBufSize); }
	STDMETHODIMP SetUnknown(REFGUID guidKey, IUnknown *pUnknown)
	{ return _attrs->SetUnknown(guidKey, pUnknown); }
	STDMETHODIMP LockStore()
	{ return _attrs->LockStore(); }
	STDMETHODIMP UnlockStore()
	{ return _attrs->UnlockStore(); }
	STDMETHODIMP GetCount(UINT32 *pcItems)
	{ return _attrs->GetCount(pcItems); }
	STDMETHODIMP GetItemByIndex(UINT32 unIndex, GUID *pguidKey, PROPVARIANT *pValue)
	{ return _attrs->GetItemByIndex(unIndex, pguidKey, pValue); }
	STDMETHODIMP CopyAllItems(IMFAttributes *pDest)
	{ return _attrs->CopyAllItems(pDest); }

protected:
	virtual void ThreadInvoke(void* delete_list_file);
	virtual void ThreadEnded() { Release(); }

private:
	std::recursive_mutex _mutex;

	WCHAR _list_file[MAX_PATH];
	ComPtr<IUnknown> _stream;
	ComPtr<IMFAsyncResult> _result;

	ComPtr<IMFAttributes> _attrs;
};