#include "MediaExtensionActivate.h"

double MediaSourceNetworkIOBufTime;
unsigned MediaSourceNetworkIOBufSize;

LPSTR CALLBACK DefaultPlaylistSegmentUpdateCallback(LPCSTR uniqueId, int nCurrentIndex, int nTotalCount, LPCSTR strCurrentUrl);

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
		typedef void* (WINAPI* Callback)();
		auto GetCoreMFsGlobalSettings = (Callback)GetProcAddress(hmod, "GetCoreMFsGlobalSettings");
		auto GetCoreMFsGlobalCS = (Callback)GetProcAddress(hmod, "GetCoreMFsGlobalCS");
		if (GetCoreMFsGlobalSettings != NULL && GetCoreMFsGlobalCS != NULL) {
			EnterCriticalSection((LPCRITICAL_SECTION)GetCoreMFsGlobalCS());
			auto attrs = (IMFAttributes*)GetCoreMFsGlobalSettings();
			attrs->AddRef();

			//CoreDisable10bitH264Video
			attrs->SetUINT32(GetGuidFromString(L"{9165F81A-C1F8-4818-980E-E7C0A6565553}"), TRUE);
			//CoreUseWin10FLACDecoder
			attrs->SetUINT32(GetGuidFromString(L"{1342C27D-A1A7-490C-91E5-4E389B7213F2}"), TRUE);

			if (MediaSourceNetworkIOBufSize > 0)
				attrs->SetUINT32(GetGuidFromString(L"{BF18CECE-34A6-4601-BA51-EC485D137A06}"),
				MediaSourceNetworkIOBufSize);
			if (MediaSourceNetworkIOBufTime > 1.0)
				attrs->SetDouble(GetGuidFromString(L"{BF18CECE-34A6-4601-BA51-EC485D137A05}"),
				MediaSourceNetworkIOBufTime);

			attrs->Release();
			LeaveCriticalSection((LPCRITICAL_SECTION)GetCoreMFsGlobalCS());
		}
	}else if (wcsicmp(dllfile, L"MultipartStreamMatroska.dll") == 0) {
		if (wcscmp(activatableClassId, L"MultipartStreamMatroska.UrlHandler") == 0) {
			ComPtr<IMFAttributes> attrs;
			punk->QueryInterface(IID_PPV_ARGS(&attrs));
			if (attrs) {
				UINT64 ptr = (UINT64)(&DefaultPlaylistSegmentUpdateCallback);
				attrs->SetUINT64(GetGuidFromString(L"{402A3476-507D-42A7-AC34-E69E199C1A9D}"), ptr);
			}
		}
	}
}