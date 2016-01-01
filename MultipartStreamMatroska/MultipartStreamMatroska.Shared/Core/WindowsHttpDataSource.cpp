#include "WindowsHttpDataSource.h"
#include <inttypes.h>

EXTERN_C HRESULT WINAPI ObtainUserAgentString(DWORD,LPSTR,DWORD*);

using namespace Downloader::Core;
using namespace Downloader::Windows;

WindowsHttpDataSource::WindowsHttpDataSource(
	const char* url,
	const char* ua,
	const char* ref_url,
	const char* cookie,
	int time_out_sec,
	unsigned max_buf_one_block_size_kb, unsigned max_buf_block_count) : _downloader(NULL) {
	_abort_event = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
	_process_abort_event = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
	_init_with_wait_headers = false;

	_content_len = 0;
	_mime_type[0] = 0;

	if (url)
		_cfgs.Url = strdup(url);
	if (ua)
		_cfgs.UserAgent = strdup(ua);
	if (ref_url)
		_cfgs.RefUrl = strdup(ref_url);
	if (cookie)
		_cfgs.Cookie = strdup(cookie);

	_cfgs.Timeout = time_out_sec;
	_cfgs.MaxBlockSizeKb = max_buf_one_block_size_kb;
	_cfgs.MaxBlockCount = max_buf_block_count;
}

CommonResult WindowsHttpDataSource::InitCheck()
{
	if (_cfgs.Url == NULL)
		return CommonResult::kInvalidInput;
	if (_downloader != NULL)
		return CommonResult::kSuccess;

	_downloader = new(std::nothrow) WindowsHttpDownloader();
	if (_downloader == NULL)
		return CommonResult::kError;
	if (!StartDownloadTask())
		return CommonResult::kNonInit;

	_init_with_wait_headers = true;
	if (!WaitForResponseHeaders()) {
		NotifyCompleteDownloadAbort();
		return CommonResult::kError;
	}
	_init_with_wait_headers = false;

	_download_eof = false;
	_cur_pos = 0;
	return CommonResult::kSuccess;
}

CommonResult WindowsHttpDataSource::ReadBytes(void* buf, unsigned size, unsigned* read_size)
{
	if (buf == NULL ||
		size == 0)
		return CommonResult::kInvalidInput;

	std::lock_guard<decltype(_mutex)> lock(_mutex);
	unsigned rs = ReadInBuffered(buf, size);
	if (rs == size) {
		if (read_size)
			*read_size = rs;
		_cur_pos += rs; //remember
		return CommonResult::kSuccess;
	}

	if (rs == 0 && _download_eof) {
		if (read_size)
			*read_size = 0;
		return CommonResult::kSuccess;
	}

	if (!_download_eof) {
		_read_more_from_network = true;
		rs += ReadInNetwork((char*)buf + rs, size - rs, _cur_pos + rs);
		_read_more_from_network = false;
	}
	if (read_size)
		*read_size = rs;

	_cur_pos += rs;
	return CommonResult::kSuccess;
}

CommonResult WindowsHttpDataSource::SetPosition(int64_t offset)
{
	//ReadBytes方法必须没有在运行
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_content_len != 0 && offset > _content_len)
		return CommonResult::kInvalidInput;
	if (offset == _cur_pos)
		return CommonResult::kSuccess;

	if (offset != 0 && offset > _cur_pos) {
		//尝试在已经缓冲的区域内seek（不执行ReconnectAtOffset，这样会重新下载）
		unsigned buf_size = _downloader->GetBufferedReadableSize();
		if (buf_size > 0) {
			if (offset < (buf_size + _cur_pos - 1024)) {
				unsigned skip_bytes = (unsigned)(offset - _cur_pos);
				if (_downloader->ForwardSeekInBufferedReadableSize(skip_bytes)) {
					_cur_pos = offset;
					return CommonResult::kSuccess;
				}
			}
		}
	}

	return ReconnectAtOffset(offset);
}

CommonResult WindowsHttpDataSource::GetLength(int64_t* size)
{
	if (size == NULL)
		return CommonResult::kInvalidInput;
	*size = _content_len;
	if (_content_len == 0)
		return CommonResult::kUnsupported;
	return CommonResult::kSuccess;
}

CommonResult WindowsHttpDataSource::ReconnectAtOffset(int64_t offset)
{
	DiscardDownloadTask();
	_downloader = new(std::nothrow) WindowsHttpDownloader();
	if (_downloader == NULL)
		return CommonResult::kError;

	char range[64] = {};
	sprintf(range, "bytes=%" PRIi64 "-", offset);
	_downloader->SetRequestHeader("Range", range);

	_download_eof = false;
	_cur_pos = offset;
	return StartDownloadTask() ? CommonResult::kSuccess : CommonResult::kError;
}

void WindowsHttpDataSource::DestroyObjects()
{
	//ReadBytes方法必须没有在运行
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_init_with_wait_headers) { //WaitForResponseHeaders
		ThrowDownloadAbort();
		WaitProcessDownloadAbort();
	}

	DiscardDownloadTask();
	CloseHandle(_abort_event);
	CloseHandle(_process_abort_event);
	_cfgs.Free();
}

bool WindowsHttpDataSource::CheckForAppendHeaders(const char* name)
{
	if (_cfgs.AppendHeaders == NULL)
		return false;
	if (strstr(_cfgs.AppendHeaders, name) == NULL)
		return false;
	return true;
}

void WindowsHttpDataSource::ConfigRequestHeaders()
{
	if (_downloader == NULL)
		return;

	if (!CheckForAppendHeaders("Accept"))
		_downloader->SetRequestHeader("Accept", "*/*");
	if (!CheckForAppendHeaders("Connection"))
		_downloader->SetRequestHeader("Connection", "Keep-Alive");
	if (!CheckForAppendHeaders("Cache-Control"))
		_downloader->SetRequestHeader("Cache-Control", "no-cache");
	if (!CheckForAppendHeaders("Pragma"))
		_downloader->SetRequestHeader("Pragma", "no-cache");
	if (!CheckForAppendHeaders("If-Modified-Since"))
		_downloader->SetRequestHeader("If-Modified-Since", "Tue, 01 Jan 1901 00:00:00 GMT");

	if (_cfgs.Cookie && !CheckForAppendHeaders("Cookie"))
		_downloader->SetRequestHeader("Cookie", _cfgs.Cookie);
	if (_cfgs.RefUrl && !CheckForAppendHeaders("Referer"))
		_downloader->SetRequestHeader("Referer", _cfgs.RefUrl);
}

bool WindowsHttpDataSource::StartDownloadTask(bool init)
{
#ifdef _DEBUG
	printf("StartDownloadTask -> %s\n", _cfgs.Url);
#endif
	char ua_sys[1024] = {};
	auto ua = _cfgs.UserAgent;
	if (ua == NULL || strlen(ua) <= 1) {
		DWORD sz = _countof(ua_sys);
		ObtainUserAgentString(0, ua_sys, &sz);
		if (ua_sys[0] != 0)
			ua = ua_sys;
	}
	if (CheckForAppendHeaders("User-Agent"))
		ua = NULL;

	if (init)
		if (!_downloader->Initialize(ua,
			_cfgs.Timeout,
			_cfgs.MaxBlockSizeKb,
			_cfgs.MaxBlockCount))
			return false;

	ConfigRequestHeaders();
	return _downloader->StartAsync(_cfgs.Url, _cfgs.AppendHeaders);
}

bool WindowsHttpDataSource::WaitForResponseHeaders()
{
	HANDLE events[] = {_abort_event,
		_downloader->GetAsyncEventHandle(WindowsHttpDownloader::AsyncEvents::kError),
		_downloader->GetAsyncEventHandle(WindowsHttpDownloader::AsyncEvents::kRecvComplete)};

	int index = WaitForMultipleObjectsEx(_countof(events), events, FALSE, INFINITE, FALSE);
	if ((index - WAIT_OBJECT_0) + 1 != _countof(events))
		return false;

	auto clen = _downloader->GetResponseHeader("Content-Length", false);
	auto type = _downloader->GetResponseHeader("Content-Type", false);
	if (_downloader->GetIsChunked()) {
		clen = NULL;
		_content_len = INT64_MAX;
	}
	if (clen != NULL)
		_content_len = _wtoi64(clen);
	if (type != NULL)
		if (wcslen(type) < 128)
			WideCharToMultiByte(CP_ACP, 0, type, -1, _mime_type, _countof(_mime_type), NULL, NULL);
#ifdef _DEBUG
	OutputDebugStringW(_downloader->GetResponseHeaders());
#endif
	return true;
}

void WindowsHttpDataSource::DiscardDownloadTask()
{
#ifdef _DEBUG
	printf("DiscardDownloadTask -> %s\n", _cfgs.Url);
#endif
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	if (_downloader == NULL)
		return;

	if (_read_more_from_network)
		ThrowDownloadAbort();

	_downloader->AbortAsync();
	_downloader->ResumeBuffering(true);
	_downloader->Release();
	_downloader = NULL;
}

unsigned WindowsHttpDataSource::ReadInBuffered(void* buf, unsigned size)
{
	unsigned rs = _downloader->GetBufferedReadableSize();
	if (rs == 0)
		return 0;
	rs = size > rs ? rs : size;
	return _downloader->ReadBufferedBytes(buf, rs);
}

unsigned WindowsHttpDataSource::ReadInNetwork(void* buf, unsigned size, int64_t cur_pos)
{
	unsigned rs = 0;
	char* pb = (char*)buf;
	bool skip_loop = false;
	while (1) {
		NetworkDownloadStatus status = WaitNetworkDownloadStatus();
		switch (status) {
		case NetworkDownloadStatus::EventAbort:
		case NetworkDownloadStatus::EventTimeout:
		case NetworkDownloadStatus::EventError:
			if (status == NetworkDownloadStatus::EventAbort)
				NotifyCompleteDownloadAbort();
			else
				_download_eof = true;
			skip_loop = true;
			break;
		case NetworkDownloadStatus::EventEof:
			_download_eof = true;
			rs += ReadInBuffered(pb + rs, size - rs);
			skip_loop = true;
			break;
		case NetworkDownloadStatus::EventDataAvailable:
			rs += ReadInBuffered(pb + rs, size - rs);
			if (rs == size)
				skip_loop = true;
			break;
		}

		if (skip_loop)
			break;
	}
	return rs;
}

WindowsHttpDataSource::NetworkDownloadStatus WindowsHttpDataSource::WaitNetworkDownloadStatus()
{
	HANDLE events[] = {_abort_event,
		_downloader->GetAsyncEventHandle(WindowsHttpDownloader::AsyncEvents::kError),
		_downloader->GetAsyncEventHandle(WindowsHttpDownloader::AsyncEvents::kEof),
		_downloader->GetAsyncEventHandle(WindowsHttpDownloader::AsyncEvents::kBuffered),
		_downloader->GetAsyncEventHandle(WindowsHttpDownloader::AsyncEvents::kDataAvailable)};

	DWORD index = WaitForMultipleObjectsEx(_countof(events), events,
		FALSE, _cfgs.Timeout * 2 * 1000, FALSE);

	if (index == WAIT_TIMEOUT || index == WAIT_FAILED)
		return NetworkDownloadStatus::EventTimeout;

	index -= WAIT_OBJECT_0;
	if (index == 0)
		return NetworkDownloadStatus::EventAbort;
	else if (index == 1)
		return NetworkDownloadStatus::EventError;
	else if (index == 2)
		return NetworkDownloadStatus::EventEof;
	else if (index == 3)
		return NetworkDownloadStatus::EventBuffered;
	else if (index == 4)
		return NetworkDownloadStatus::EventDataAvailable;

	return NetworkDownloadStatus::EventTimeout;
}