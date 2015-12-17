#include "MediaExtensionActivate.h"

MediaExtensionActivate::MediaExtensionActivate(REFCLSID clsid, REFIID iid, LPCWSTR activatableClassId, LPCWSTR dllfile)
{
	_ref_count = 1;
	MFCreateAttributes(&_attrs, 0);
	if (activatableClassId)
		wcscpy_s(_obj_info.activatableClassId, activatableClassId);
	wcscpy_s(_obj_info.file, dllfile);
	_obj_info.clsid = clsid;
	_obj_info.iid = iid;
	_module = NULL;
}

HRESULT MediaExtensionActivate::ActivateObject(REFIID riid, void **ppv)
{
	if (ppv == NULL)
		return E_POINTER;
	if (_obj_info.iid != riid)
		return E_INVALIDARG;

	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_instance) {
		*ppv = _instance.Get();
		_instance.Get()->AddRef();
		return S_OK;
	}

	auto module = _module;
	if (module == NULL) {
		module = LoadPackagedLibrary(_obj_info.file, 0);
		if (module == NULL)
			return HRESULT_FROM_WIN32(GetLastError());
	}

	auto GetActivationFactory = (PFNGETACTIVATIONFACTORY)GetProcAddress(module, "DllGetActivationFactory");
	if (GetActivationFactory == NULL) {
		FreeLibrary(module);
		return E_ABORT;
	}

	ComPtr<IActivationFactory> factroy;
	auto hr = GetActivationFactory(HStringReference(_obj_info.activatableClassId).Get(), &factroy);
	if (FAILED(hr)) {
		FreeLibrary(module);
		return hr;
	}

	ComPtr<IInspectable> punk;
	hr = factroy->ActivateInstance(&punk);
	if (FAILED(hr)) {
		FreeLibrary(module);
		return hr;
	}

	punk.As(&_instance);
	_module = module;
	*ppv = _instance.Get();
	_instance.Get()->AddRef();
	return S_OK;
}

HRESULT MediaExtensionActivate::DetachObject()
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_instance)
		_instance.Reset();
	return S_OK;
}