#include "MediaExtensionInstaller.h"
#include "AutoLib.h"

static bool CreateMediaExtensionActivate(REFCLSID clsid, REFIID iid, LPCWSTR activatableClassId, LPCWSTR dllfile, IMFActivate** pa)
{
	auto thunk = new(std::nothrow) MediaExtensionActivate(clsid, iid, activatableClassId, dllfile);
	if (thunk == nullptr)
		return false;
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

	typedef HRESULT (WINAPI* Callback)(LPCWSTR,IMFActivate*);
	AutoLibrary platform("mfplat.dll");
	if (FAILED(platform.GetProcAddr<Callback>("MFRegisterLocalSchemeHandler")
		(schemeType, thunk.Get())))
		return false;

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

	typedef HRESULT (WINAPI* Callback)(LPCWSTR,LPCWSTR,IMFActivate*);
	AutoLibrary platform("mfplat.dll");
	if (FAILED(platform.GetProcAddr<Callback>("MFRegisterLocalByteStreamHandler")
		(fileExt, mimeType, thunk.Get())))
		return false;

	_objs->AddElement(thunk.Get());
	return true;
}