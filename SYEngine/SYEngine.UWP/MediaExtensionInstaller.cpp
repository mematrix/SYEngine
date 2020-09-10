#include "pch.h"
#include "MediaExtensionInstaller.h"
#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#include "AutoLib.h"
#endif

void CALLBACK DefaultMediaExtensionActivatedEventCallback(LPCWSTR dllfile, LPCWSTR activatableClassId, HMODULE hmod, IUnknown* punk);

static bool CreateMediaExtensionActivate(REFCLSID clsid, REFIID iid, LPCWSTR activatableClassId, LPCWSTR dllfile, IMFActivate** pa)
{
	auto thunk = new(std::nothrow) MediaExtensionActivate(clsid, iid, activatableClassId, dllfile);
	if (thunk == nullptr)
		return false;

#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
	thunk->SetCallbackObjectActivated(DefaultMediaExtensionActivatedEventCallback);
#endif
	*pa = thunk;
	return true;
}

bool MediaExtensionInstaller::InstallSchemeHandler(LPCWSTR clsid, LPCWSTR activatableClassId, LPCWSTR dllFile, LPCWSTR schemeType)
{
	CLSID cid = GUID_NULL;
	if (clsid)
		CLSIDFromString(clsid, &cid);

	ComPtr<IMFActivate> thunk;
	if (!CreateMediaExtensionActivate(cid, IID_IMFSchemeHandler, activatableClassId, dllFile, &thunk))
		return false;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	if (FAILED(MFRegisterLocalSchemeHandler(schemeType, thunk.Get())))
		return false;
#else
	typedef HRESULT (WINAPI* Callback)(LPCWSTR,IMFActivate*);
	AutoLibrary platform("mfplat.dll");
	if (FAILED(platform.GetProcAddr<Callback>("MFRegisterLocalSchemeHandler")
		(schemeType, thunk.Get())))
		return false;
#endif

	_objs->AddElement(thunk.Get());
	return true;
}

bool MediaExtensionInstaller::InstallByteStreamHandler(LPCWSTR clsid, LPCWSTR activatableClassId, LPCWSTR dllFile, LPCWSTR fileExt, LPCWSTR mimeType)
{
	CLSID cid = GUID_NULL;
	if (clsid)
		CLSIDFromString(clsid, &cid);

	ComPtr<IMFActivate> thunk;
	if (!CreateMediaExtensionActivate(cid, IID_IMFByteStreamHandler, activatableClassId, dllFile, &thunk))
		return false;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	if (FAILED(MFRegisterLocalByteStreamHandler(fileExt, mimeType, thunk.Get())))
		return false;
#else
	typedef HRESULT (WINAPI* Callback)(LPCWSTR,LPCWSTR,IMFActivate*);
	AutoLibrary platform("mfplat.dll");
	if (FAILED(platform.GetProcAddr<Callback>("MFRegisterLocalByteStreamHandler")
		(fileExt, mimeType, thunk.Get())))
		return false;
#endif

	_objs->AddElement(thunk.Get());
	return true;
}