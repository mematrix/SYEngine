#include <Windows.h>
#include <Windows.ApplicationModel.h>
#include <windows.storage.h>
#include <windows.storage.streams.h>
#include <wrl.h>
#include <wrl\wrappers\corewrappers.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

HRESULT GetExeModulePath(LPWSTR lpstrPath,HMODULE hModule)
{
#if (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
	WCHAR szBuffer[MAX_PATH] = {};
	DWORD dwResult = GetModuleFileNameW(hModule,szBuffer,MAX_PATH);
	if (dwResult == 0)
		return HRESULT_FROM_WIN32(GetLastError());

	auto p = wcsrchr(szBuffer,L'\\');
	if (p)
		*p = NULL;

	wcscpy_s(lpstrPath,MAX_PATH,szBuffer);
	return S_OK;
#else
	ComPtr<ABI::Windows::ApplicationModel::IPackageStatics> pPkgMethods;
	HRESULT hr = RoGetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Package).Get(),
		IID_PPV_ARGS(pPkgMethods.GetAddressOf()));

	if (FAILED(hr))
		return hr;

	ComPtr<ABI::Windows::ApplicationModel::IPackage> pPackage;
	hr = pPkgMethods->get_Current(pPackage.GetAddressOf());
	if (FAILED(hr))
		return hr;

	ComPtr<ABI::Windows::Storage::IStorageFolder> pFolder;
	hr = pPackage->get_InstalledLocation(pFolder.GetAddressOf());
	if (FAILED(hr))
		return hr;

	ComPtr<ABI::Windows::Storage::IStorageItem> pFolderItem;
	hr = pFolder.As(&pFolderItem);
	if (FAILED(hr))
		return hr;

	HString hstrPath;
	hr = pFolderItem->get_Path(hstrPath.GetAddressOf());
	if (FAILED(hr))
		return hr;

	wcscpy_s(lpstrPath,MAX_PATH,hstrPath.GetRawBuffer(nullptr));
	return S_OK;
#endif
}