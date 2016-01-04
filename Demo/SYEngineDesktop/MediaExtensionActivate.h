#pragma once

#include "pch.h"
#include <mutex>

typedef void (CALLBACK* MediaExtensionActivatedEventCallback)
(LPCWSTR dllfile, LPCWSTR activatableClassId, HMODULE hmod, IUnknown* punk);

class MediaExtensionActivate sealed : public IMFActivate
{
public:
	MediaExtensionActivate(REFCLSID clsid, REFIID iid, LPCWSTR activatableClassId, LPCWSTR dllfile);
	~MediaExtensionActivate() {}

	void SetCallbackObjectActivated(MediaExtensionActivatedEventCallback callback) throw()
	{ _callback = callback; }

public:
	STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_ref_count); }
	STDMETHODIMP_(ULONG) Release()
	{ ULONG rc = InterlockedDecrement(&_ref_count); if (rc == 0) delete this; return rc; }
	STDMETHODIMP QueryInterface(REFIID iid,void** ppv)
	{ if (iid != IID_IUnknown && iid != IID_IMFAttributes && iid != IID_IMFActivate) return E_NOINTERFACE; 
	  if (ppv == NULL) return E_POINTER; *ppv = this; AddRef(); return S_OK;}

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

	STDMETHODIMP ActivateObject(REFIID riid, void **ppv);
	STDMETHODIMP ShutdownObject() { return S_OK; }
	STDMETHODIMP DetachObject();

private:
	ULONG _ref_count;
	ComPtr<IMFAttributes> _attrs;

	struct ObjectInformation
	{
		WCHAR activatableClassId[MAX_PATH];
		WCHAR file[MAX_PATH];
		CLSID clsid;
		IID iid;
	};
	ObjectInformation _obj_info;

	ComPtr<IUnknown> _instance;
	HMODULE _module;
	MediaExtensionActivatedEventCallback _callback;

	std::recursive_mutex _mutex;
};