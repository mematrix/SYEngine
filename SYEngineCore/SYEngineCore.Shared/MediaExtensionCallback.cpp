#include "MediaExtensionActivate.h"

static GUID GetGuidFromString(LPCWSTR str)
{
	GUID ret = GUID_NULL;
	CLSIDFromString(str, &ret);
	return ret;
}

void CALLBACK DefaultMediaExtensionActivatedEventCallback(LPCWSTR dllfile, LPCWSTR activatableClassId, HMODULE hmod, IUnknown* punk)
{
#ifdef _DEBUG
	WCHAR log[512] = {};
	StringCchPrintfW(log, _countof(log), L"MediaExtensionActivated: %s -> %s\n", dllfile, activatableClassId);
	OutputDebugStringW(log);
#endif

	if (wcsicmp(dllfile, L"CoreMFSource.dll") == 0) {
		typedef IMFAttributes* (WINAPI* Callback)();
		auto GetCoreMFsGlobalSettings = (Callback)GetProcAddress(hmod, "GetCoreMFsGlobalSettings");
		if (GetCoreMFsGlobalSettings) {
			//CoreDisable10bitH264Video
			GetCoreMFsGlobalSettings()->SetUINT32(GetGuidFromString(L"{9165F81A-C1F8-4818-980E-E7C0A6565553}"), TRUE);
			//CoreUseWin10FLACDecoder
			GetCoreMFsGlobalSettings()->SetUINT32(GetGuidFromString(L"{1342C27D-A1A7-490C-91E5-4E389B7213F2}"), TRUE);
		}
	}else if (wcsicmp(dllfile, L"MultipartStreamMatroska.dll") == 0) {
		if (wcscmp(activatableClassId, L"MultipartStreamMatroska.UrlHandler") == 0) {
			ComPtr<IMFAttributes> attrs;
			punk->QueryInterface(IID_PPV_ARGS(&attrs));
			if (attrs) {
				//TODO...
			}
		}
	}
}