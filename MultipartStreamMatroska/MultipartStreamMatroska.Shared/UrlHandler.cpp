#include "UrlHandler.h"
#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#include <windows.storage.h>
#endif

#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
ActivatableClass(UrlHandler);
#endif
CoCreatableClass(UrlHandler);

static bool CheckFileAllow(LPCWSTR filePath)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	auto file = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
#else
	auto file = CreateFile2(filePath, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, NULL);
#endif
	if (file != INVALID_HANDLE_VALUE)
		CloseHandle(file);
	return file != INVALID_HANDLE_VALUE;
}

HRESULT UrlHandler::BeginCreateObject(
	LPCWSTR pwszURL,
	DWORD dwFlags, IPropertyStore *pProps,
	IUnknown **ppIUnknownCancelCookie,
	IMFAsyncCallback *pCallback, IUnknown *punkState) {

	if (pwszURL == NULL || pCallback == NULL)
		return E_INVALIDARG;
	if (ppIUnknownCancelCookie)
		*ppIUnknownCancelCookie = NULL;

	if (wcslen(pwszURL) < 10)
		return E_INVALIDARG;
	if (_wcsnicmp(pwszURL, L"plist://", 8) != 0)
		return E_INVALIDARG;

	auto file = pwszURL + 8;
	auto len = wcslen(file);
	wcscpy_s(_list_file, file);
	if (_list_file[1] == '_')
		_list_file[1] = L':';
	if (_list_file[len - 1] == L'/' ||
		_list_file[len - 1] == L'\\')
		_list_file[len - 1] = 0;

	if (wcsstr(_list_file, L"\\") == NULL &&
		wcsstr(_list_file, L"$") == NULL &&
		_list_file[2] == L'-') {
		for (int i = 0; i < _countof(_list_file); i++)
			if (_list_file[i] == L'-')
				_list_file[i] = L'\\';
	}

	BOOL delete_list_file = FALSE;
#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
	if (wcsnicmp(_list_file, L"WinRT-", 6) == 0) {
		WCHAR strRuntimePath[MAX_PATH] = {};
		ComPtr<ABI::Windows::Storage::IApplicationDataStatics> pAppDataStatics;
		RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), IID_PPV_ARGS(&pAppDataStatics));
		if (pAppDataStatics)
		{
			ComPtr<ABI::Windows::Storage::IApplicationData> pAppData;
			if (SUCCEEDED(pAppDataStatics->get_Current(&pAppData)))
			{
				ComPtr<ABI::Windows::Storage::IStorageFolder> folder;
				ComPtr<ABI::Windows::Storage::IStorageItem> info;
				if (wcsstr(_list_file, L"-TemporaryFolder_") != NULL)
					pAppData->get_TemporaryFolder(&folder);
				else if (wcsstr(_list_file, L"-RoamingFolder_") != NULL)
					pAppData->get_RoamingFolder(&folder);
				else if (wcsstr(_list_file, L"-LocalFolder_") != NULL)
					pAppData->get_LocalFolder(&folder);

				HSTRING hstr = NULL;
				if (folder)
					folder.As(&info);
				if (info)
					info->get_Path(&hstr);

				if (hstr) {
					HString str;
					str.Attach(hstr);
					wcscpy_s(strRuntimePath, str.GetRawBuffer(NULL));
					wcscat_s(strRuntimePath, L"\\");
				}
			}
		}
		if (strRuntimePath[0] != 0)
		{
			wcscat_s(strRuntimePath, wcsstr(_list_file, L"_") + 1);
			wcscpy(_list_file, strRuntimePath);
			delete_list_file = TRUE;
		}
	}
#endif

	if (!CheckFileAllow(_list_file))
		return E_ACCESSDENIED;

	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_result != nullptr)
		return E_FAIL;

	auto hr = MFCreateAsyncResult(NULL, pCallback, punkState, &_result);
	if (FAILED(hr))
		return hr;

	return ThreadStart((void*)delete_list_file) ? S_OK : E_UNEXPECTED;
}

HRESULT UrlHandler::EndCreateObject(
	IMFAsyncResult *pResult,
	MF_OBJECT_TYPE *pObjectType, IUnknown **ppObject) {

	if (pResult == NULL ||
		pObjectType == NULL ||
		ppObject == NULL)
		return E_INVALIDARG;

	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_result == nullptr || _stream == nullptr)
		return E_FAIL;
	if (FAILED(_result->GetStatus()))
		return _result->GetStatus();

	*pObjectType = MF_OBJECT_BYTESTREAM;
	*ppObject = (IUnknown*)_stream.Get();
	(*ppObject)->AddRef();

	_stream.Reset();
	_result.Reset();
	return S_OK;
}

void UrlHandler::ThreadInvoke(void* delete_list_file)
{
	AddRef();
	_result->SetStatus(S_OK);
	{
		std::lock_guard<decltype(_mutex)> lock(_mutex);
		auto s = new(std::nothrow) MultipartStream(_list_file);
		if (s == NULL) {
			_result->SetStatus(E_OUTOFMEMORY);
		}else{
			if (s->Open(this, delete_list_file != NULL ? true:false)) {
				_stream.Attach(static_cast<IMFByteStream*>(s));
			}else{
				_result->SetStatus(E_FAIL);
				s->Release();
			}
		}
	}
	MFInvokeCallback(_result.Get());
}