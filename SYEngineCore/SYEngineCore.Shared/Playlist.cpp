#include "Playlist.h"

using namespace SYEngineCore;
using namespace Windows::Foundation;

wchar_t* AnsiToUnicode(const char* str);
char* UnicodeToAnsi(const wchar_t* str);

bool Playlist::Append(Platform::String^ url, int sizeInBytes, float durationInSeconds)
{
	auto str = url->Data();
	if (_type == PlaylistTypes::LocalFile) {
		auto hf = CreateFile2(str, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, NULL);
		if (hf == INVALID_HANDLE_VALUE)
			return false;
		CloseHandle(hf);
	}

	int ansi_size = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	if (ansi_size == 0)
		return false;
	auto p = (char*)calloc(2, ansi_size);
	if (p == NULL)
		return false;
	WideCharToMultiByte(CP_ACP, 0, str, -1, p, ansi_size * 2, NULL, NULL);

	Item item = {p, sizeInBytes, durationInSeconds};
	_list.push_back(std::move(item));
	return true;
}

void Playlist::Clear()
{
	if (_debugFile)
		free(_debugFile);
	_debugFile = NULL;

	if (_list.size() > 0)
	{
		for (auto i = _list.begin(); i != _list.end(); ++i)
			free(i->Url);
		_list.clear();
	}
}

void Playlist::SetDebugFile(Platform::String^ fileName)
{
	if (_debugFile)
		free(_debugFile);
	_debugFile = NULL;

	if (fileName != nullptr && fileName->Length() > 0) {
		DeleteFileW(fileName->Data());
		_debugFile = _wcsdup(fileName->Data());
	}
}

IAsyncOperation<Platform::String^>^ Playlist::SaveAndGetFileUriAsync()
{
	return concurrency::create_async([&]{
		WCHAR uri[MAX_PATH * 2] = {};
		SaveFile(uri);
		return ref new Platform::String(uri);
	});
}

bool Playlist::SaveFile(LPWSTR uri)
{
	char* str = NULL;
	if (_type == PlaylistTypes::LocalFile)
		str = SerializeForLocalFile();
	else if (_type == PlaylistTypes::NetworkHttp)
		str = SerializeForNetworkHttp();
	if (str == NULL)
		return false;

	if (strlen(str) < 8)
		return false;
	str[strlen(str) - 1] = 0; //skip \r\n
	str[strlen(str) - 1] = 0;

	auto temp_folder = Windows::Storage::ApplicationData::Current->TemporaryFolder->Path->Data();
	WCHAR temp_file[MAX_PATH] = {};
	StringCchPrintfW(temp_file, _countof(temp_file),
		L"%s\\MultipartStreamMatroska_%d.txt",
		temp_folder, (int)GetTickCount64());

	DeleteFileW(temp_file);
	auto file = CreateFile2(temp_file, GENERIC_WRITE, 0, CREATE_ALWAYS, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		free(str);
		return false;
	}
	WriteFile(file, str, (DWORD)strlen(str), NULL, NULL);
	FlushFileBuffers(file);
	CloseHandle(file);

	free(str);
	wcscpy(uri, temp_file);
	return true;
}

char* Playlist::SerializeForLocalFile()
{
	auto p = (char*)calloc(2, 64 + _list.size() * MAX_PATH);
	if (p == NULL)
		return NULL;

	strcpy(p, "LOCAL\r\n");
	for (auto i = _list.begin(); i != _list.end(); ++i) {
		strcat(p, i->Url);
		strcat(p, "\r\n");
	}
	return p;
}

char* Playlist::SerializeForNetworkHttp()
{
	int len = 0;
	for (auto i = _list.begin(); i != _list.end(); ++i)
		len += strlen(i->Url) + 1;

	auto p = (char*)calloc(2, len + 4096);
	if (p == NULL)
		return NULL;

	auto cookie = UnicodeToAnsi(_cfgs.HttpCookie ? _cfgs.HttpCookie->Data() : NULL);
	auto referer = UnicodeToAnsi(_cfgs.HttpReferer ? _cfgs.HttpReferer->Data() : NULL);
	auto userAgent = UnicodeToAnsi(_cfgs.HttpUserAgent ? _cfgs.HttpUserAgent->Data() : NULL);

	auto uniqueId = UnicodeToAnsi(_cfgs.UniqueId ? _cfgs.UniqueId->Data() : NULL);
	auto debugFile = UnicodeToAnsi(_debugFile);

	double duration = _cfgs.ExplicitTotalDurationSeconds;
	if (duration < 0.1) {
		for (auto i = _list.begin(); i != _list.end(); ++i)
			duration += (double)i->DurationInSeconds;
	}

	sprintf(p, "%d\r\n%s\r\n%d\r\n%s\r\n%d|%d\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n",
		(int)(duration * 1000.0),
		(_cfgs.DetectDurationForParts ? "FULL" : "NO"),
		(_cfgs.FetchNextPartThresholdSeconds > 1 ? _cfgs.FetchNextPartThresholdSeconds : 30),
		(_cfgs.DownloadRetryOnFail ? "Reconnect" : "NO"),
		(_cfgs.BufferBlockSizeKB > 8 ? _cfgs.BufferBlockSizeKB : 64),
		(_cfgs.BufferBlockCount > 2 ? _cfgs.BufferBlockCount : 80),
		"NULL",
		cookie ? cookie:"",
		referer ? referer:"",
		userAgent ? userAgent:"",
		uniqueId ? uniqueId:"",
		debugFile ? debugFile:"");

	for (auto i = _list.begin(); i != _list.end(); ++i) {
		if (i->SizeInBytes > 0 || i->DurationInSeconds > 0.1f) {
			char info[64] = {};
			sprintf(info, "%d|%d\r\n", i->SizeInBytes, (int)(i->DurationInSeconds * 1000.0f));
			strcat(p, info);
		}else{
			strcat(p, "0\r\n");
		}
		strcat(p, i->Url);
		strcat(p, "\r\n");
	}

	if (cookie)
		free(cookie);
	if (referer)
		free(referer);
	if (userAgent)
		free(userAgent);
	if (uniqueId)
		free(uniqueId);
	if (debugFile)
		free(debugFile);
	return p;
}