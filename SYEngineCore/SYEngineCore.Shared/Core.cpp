#include "Core.h"
#include <map>
#include <string>

#define _STM_HANDLER_CLSID L"{1A0DFC9E-009C-4266-ADFF-CA37D7F8E450}"
#define _URL_HANDLER_CLSID L"{2A0DFC9E-009C-4266-ADFF-CA37D7F8E450}"

#define _STM_HANDLER_NAME L"CoreMFSource.HDCoreByteStreamHandler"
#define _URL_HANDLER_NAME L"MultipartStreamMatroska.UrlHandler"

#define _STM_HANDLER_FILE L"CoreMFSource.dll"
#define _URL_HANDLER_FILE L"MultipartStreamMatroska.dll"

using namespace SYEngineCore;

bool Core::Initialize()
{
	if (Installer != nullptr)
		return false;

	Installer = new(std::nothrow) MediaExtensionInstaller();
	if (Installer == nullptr)
		return false;

	if (!Installer->InstallSchemeHandler(_URL_HANDLER_CLSID,
		_URL_HANDLER_NAME,
		_URL_HANDLER_FILE,
		L"plist:"))
		return false;

	struct ByteStreamHandlerPair
	{
		LPCWSTR FileExtension;
		LPCWSTR MimeType;
	};
	static ByteStreamHandlerPair Handlers[] = {
		{NULL, L"video/force-network-stream"},
		{L".mkv", L"video/x-matroska"},
		{L".flv", L"video/x-flv"},
		{L".f4v", NULL}
	};
	for (auto h : Handlers)
		Installer->InstallByteStreamHandler(_STM_HANDLER_CLSID,
		_STM_HANDLER_NAME,
		_STM_HANDLER_FILE,
		h.FileExtension,
		h.MimeType);

	return true;
}

void Core::Uninitialize()
{
	if (Installer)
		delete Installer;
	Installer = nullptr;
}

///////////////// Playlist Events /////////////////

wchar_t* AnsiToUnicode(const char* str)
{
	int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	if (len == 0)
		return NULL;
	auto result = (wchar_t*)calloc(2, len);
	if (result)
		MultiByteToWideChar(CP_ACP, 0, str, -1, result, len + 1);
	return result;
}

char* UnicodeToAnsi(const wchar_t* str)
{
	int len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	if (len == 0)
		return NULL;
	auto result = (char*)calloc(2, len);
	if (result)
		WideCharToMultiByte(CP_ACP, 0, str, -1, result, len + 1, NULL, NULL);
	return result;
}

private ref class PlaylistNetworkUpdateInfo sealed : IPlaylistNetworkUpdateInfo
{
public:
	PlaylistNetworkUpdateInfo(Platform::String^ url)
	{ _url = _wcsdup(url->Data()); _timeout = 0; }
	virtual ~PlaylistNetworkUpdateInfo() { _headers.clear(); free(_url); }

	virtual property Platform::String^ Url
	{
		Platform::String^ get()
		{ return ref new Platform::String(_url); }
		void set(Platform::String^ value)
		{ if (value->Length() > 0) free(_url); _url = _wcsdup(value->Data()); }
	}

	virtual property int Timeout
	{
		int get() { return _timeout; }
		void set(int value) { _timeout = value; }
	}

	virtual bool SetRequestHeader(Platform::String^ name, Platform::String^ value)
	{
		if (name == nullptr || name->Length() == 0)
			return false;

		auto exists = _headers.find(name->Data());
		if (exists != _headers.end()) {
			_headers.erase(exists);
			if (value == nullptr || value->Length() == 0)
				return true;
		}
		_headers.insert(std::make_pair(name->Data(), value->Data()));
		return true;
	}

	virtual Platform::String^ GetRequestHeader(Platform::String^ name)
	{
		if (name == nullptr || name->Length() == 0)
			return nullptr;

		auto result = _headers.find(name->Data());
		if (result == _headers.end())
			return nullptr;
		return ref new Platform::String(result->second.data());
	}

internal:
	wchar_t* GetAllRequestHeaders()
	{
		if (_headers.empty())
			return NULL;

		int len = 0;
		for (auto i = _headers.begin(); i != _headers.end(); ++i) {
			len += i->first.length();
			len += i->second.length();
			len += 10;
		}
		auto result = (wchar_t*)calloc(2, len);
		if (result == NULL)
			return NULL;

		auto str = result;
		for (auto i = _headers.begin(); i != _headers.end(); ++i) {
			wcscat(str, i->first.data());
			str += i->first.length();
			wcscat(str, L": ");
			str += 2;
			wcscat(str, i->second.data());
			str += i->second.length();
			wcscat(str, L"\r");
			str += 1;
		}
		result[wcslen(result) - 1] = 0;
		return result;
	}

private:
	wchar_t* _url;
	int _timeout;
	std::map<std::wstring,std::wstring> _headers;
};

LPSTR CALLBACK Core::DefaultPlaylistSegmentUrlUpdateCallback(LPCSTR uniqueId, LPCSTR opType, int nCurrentIndex, int nTotalCount, LPCSTR strCurrentUrl)
{
	auto w_uniqueId = AnsiToUnicode(uniqueId);
	auto w_opType = AnsiToUnicode(opType);
	auto w_strCurrentUrl = AnsiToUnicode(strCurrentUrl);

	auto uid = ref new Platform::String(w_uniqueId);
	auto typ = ref new Platform::String(w_opType);
	auto url = ref new Platform::String(w_strCurrentUrl);
	free(w_uniqueId);
	free(w_opType);
	free(w_strCurrentUrl);

	Platform::String^ result = nullptr;
	try {
		result = PlaylistSegmentUrlUpdateEvent(uid, typ, nCurrentIndex, nTotalCount, url);
	}catch(...) {
		return NULL;
	}
	if (result == nullptr)
		return NULL;
	if (result->Length() == 0)
		return NULL;

	int len = WideCharToMultiByte(CP_ACP, 0, result->Data(), -1, NULL, 0, NULL, NULL);
	auto str = (char*)CoTaskMemAlloc(len * 2);
	WideCharToMultiByte(CP_ACP, 0, result->Data(), -1, str, len + 1, NULL, NULL);
	return str;
}

BOOL CALLBACK Core::DefaultPlaylistSegmentDetailUpdateCallback(LPCSTR uniqueId, LPCSTR opType, int nCurrentIndex, int nTotalCount, LPCSTR strCurrentUrl, void* values)
{
	auto w_uniqueId = AnsiToUnicode(uniqueId);
	auto w_opType = AnsiToUnicode(opType);
	auto w_strCurrentUrl = AnsiToUnicode(strCurrentUrl);

	auto uid = ref new Platform::String(w_uniqueId);
	auto typ = ref new Platform::String(w_opType);
	auto url = ref new Platform::String(w_strCurrentUrl);
	free(w_uniqueId);
	free(w_opType);
	free(w_strCurrentUrl);

	struct UpdateItemDetailValues
	{
		LPSTR pszUrl;
		LPSTR pszRequestHeaders;
		int timeout;
	};
	auto v = (UpdateItemDetailValues*)values;
	auto info = ref new PlaylistNetworkUpdateInfo(url);

	try {
		if (!PlaylistSegmentDetailUpdateEvent(uid, typ, nCurrentIndex, nTotalCount, info))
			return FALSE;
	}catch(...) {
		return FALSE;
	}

	if (info->Url->Length() == 0 ||
		wcsstr(info->Url->Data(), L":") == NULL)
		return FALSE;
	if (wcscmp(info->Url->Data(), url->Data()) == 0)
		return FALSE;

	auto result = info->GetAllRequestHeaders();
	if (result != NULL) {
		int len = WideCharToMultiByte(CP_ACP, 0, result, -1, NULL, 0, NULL, NULL);
		v->pszRequestHeaders = (LPSTR)CoTaskMemAlloc(len * 2);
		WideCharToMultiByte(CP_ACP, 0, result, -1, v->pszRequestHeaders, len + 1, NULL, NULL);
		free(result);
	}

	v->timeout = info->Timeout;
	int len = WideCharToMultiByte(CP_ACP, 0, info->Url->Data(), -1, NULL, NULL, NULL, NULL);
	v->pszUrl = (LPSTR)CoTaskMemAlloc(len * 2);
	WideCharToMultiByte(CP_ACP, 0, info->Url->Data(), -1, v->pszUrl, len + 1, NULL, NULL);
	return TRUE;
}