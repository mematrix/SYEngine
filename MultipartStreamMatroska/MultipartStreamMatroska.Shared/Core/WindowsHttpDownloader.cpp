#include "WindowsHttpDownloader.h"

inline static bool AnsiToUnicode(const char* ansi, LPWSTR uni, unsigned max_u_len = MAX_PATH)
{ return MultiByteToWideChar(CP_ACP, 0, ansi, -1, uni, max_u_len) > 0; }

bool WindowsHttpDownloader::Initialize(const char* user_agent, int timeout_sec, unsigned buf_block_size_kb, unsigned buf_block_count)
{
	if (!WinHttpCheckPlatform())
		return false;
	if (_http.session != NULL)
		return false;

	WCHAR ua[MAX_PATH] = {};
	if (user_agent)
		AnsiToUnicode(user_agent, ua);

	_http.session = WinHttpOpen(ua[0] != 0 ? ua : NULL,
		WINHTTP_ACCESS_TYPE_NO_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (_http.session == NULL)
		return false;

	int timeout = timeout_sec * 1000;
	if (timeout > 100)
		WinHttpSetTimeouts(_http.session, timeout, timeout, timeout, timeout);

	for (int i = 0; i < _countof(_events); i++) {
		_events[i] = CreateEventExW(NULL, NULL,
			(i != EventBuf && i != EventDataAvailable) ? CREATE_EVENT_MANUAL_RESET : 0,
			EVENT_ALL_ACCESS);
	}
	return _buffered.Initialize(buf_block_size_kb, buf_block_count);
}

bool WindowsHttpDownloader::SetRequestHeader(const char* name, const char* value)
{
	KVHeader h = {};
	h.SetName(name);
	h.SetValue(value);
	_user_heads.push(h);
	return true;
}

bool WindowsHttpDownloader::StartAsync(const char* url, const char* append_headers)
{
	int len = strlen(url);
	if (len < 8 || len > 2047)
		return false;

	_urls[0] = 0;
	AnsiToUnicode(url, _urls, _countof(_urls));
	RtlZeroMemory(&_url, sizeof(_url));
	_url.dwStructSize = sizeof(_url);
	_url.dwUrlPathLength =
		_url.dwHostNameLength =
		_url.dwExtraInfoLength =
		_url.dwUserNameLength =
		_url.dwPasswordLength =
		_url.dwSchemeLength = -1;
	if (!WinHttpCrackUrl(_urls, 0, 0, &_url))
		return false;
	return ThreadStart(append_headers ? strdup(append_headers) : NULL);
}

const wchar_t* WindowsHttpDownloader::GetResponseHeader(const char* name, bool text_match_case)
{
	if (strlen(name) > MAX_PATH)
		return NULL;
	if (_recv_headers_zero == NULL)
		return NULL;

	WCHAR n[MAX_PATH];
	AnsiToUnicode(name, n);

	wchar_t* result = NULL;
	unsigned len = wcslen(n);
	unsigned char* start = _recv_headers_zero;
	unsigned char* end = _recv_headers_zero + _recv_headers_zero_size + 1;
	while (1)
	{
		auto ret = wcsncmp((wchar_t*)start, n, len);
		if (!text_match_case)
			ret = wcsnicmp((wchar_t*)start, n, len);
		if (ret != 0)
		{
			start += (wcslen((wchar_t*)start) * 2 + 2);
			if (start >= end)
				break;
			continue;
		}

		result = wcsstr((wchar_t*)start, L":");
		if (result != NULL) {
			result++;
			for (unsigned i = 0; i < wcslen(result); i++) {
				if (*result == L' ') {
					result++;
					--i;
				}else{
					break;
				}
			}
		}
		break;
	}
	return result;
}

bool WindowsHttpDownloader::GetIsChunked() throw()
{
	auto c = GetResponseHeader("Transfer-Encoding");
	if (c == NULL)
		return false;
	wchar_t temp[128] = {};
	wcscpy_s(temp, c);
	wcslwr(temp);
	return wcsstr(temp, L"chunked") != NULL;
}

bool WindowsHttpDownloader::GetIsGZip() throw()
{
	auto c = GetResponseHeader("Content-Encoding");
	if (c == NULL)
		return false;
	wchar_t temp[256] = {};
	wcscpy_s(temp, c);
	wcslwr(temp);
	return wcsstr(temp, L"gzip") != NULL;
}

unsigned WindowsHttpDownloader::GetBufferedReadableSize() throw()
{
	std::lock_guard<decltype(_buf_lock)> lock(_buf_lock);
	return _buffered.GetReadableSize();
}

unsigned WindowsHttpDownloader::ReadBufferedBytes(void* buf, unsigned size) throw()
{
	std::lock_guard<decltype(_buf_lock)> lock(_buf_lock);
	unsigned result = _buffered.ReadBytes(buf, size);
	if (_buffered.IsBlockWriteable())
		SetEvent(_event_download);
	return result;
}

bool WindowsHttpDownloader::ForwardSeekInBufferedReadableSize(unsigned skip_bytes) throw()
{
	std::lock_guard<decltype(_buf_lock)> lock(_buf_lock);
	if (skip_bytes >= _buffered.GetReadableSize())
		return false;
	return _buffered.ForwardSeekInReadableRange(skip_bytes);
}

void WindowsHttpDownloader::PauseBuffering()
{
	std::lock_guard<decltype(_buf_lock)> lock(_buf_lock);
	ResetEvent(_event_download);
}

void WindowsHttpDownloader::ResumeBuffering(bool force)
{
	std::lock_guard<decltype(_buf_lock)> lock(_buf_lock);
	if (!force) {
		if (_buffered.IsBlockWriteable())
			SetEvent(_event_download);
	}else{
		_buffered.Reinitialize();
		SetEvent(_event_download);
	}
}

void WindowsHttpDownloader::InternalConnect(char* append_headers)
{
	WCHAR host_name[1024] = {};
	if (wcslen(_url.lpszHostName) > 1024) {
		SetEvent(_events[EventErr]);
		return;
	}
	wcscpy(host_name, _url.lpszHostName);
	if (wcsstr(host_name, L"/") != NULL)
		*wcsstr(host_name, L"/") = 0;
	if (wcsstr(host_name, L":") != NULL)
		*wcsstr(host_name, L":") = 0;

	_http.connect = WinHttpConnect(_http.session, host_name, _url.nPort, 0);
	if (_http.connect == NULL) {
		SetEvent(_events[EventErr]);
		return;
	}
	SetEvent(_events[EventConnect]);
	if (_abort_download)
		return;

	_http.request = WinHttpOpenRequest(_http.connect, L"GET",
		_url.dwUrlPathLength > 0 ? _url.lpszUrlPath : NULL,
		NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
	if (_http.connect == NULL) {
		SetEvent(_events[EventErr]);
		return;
	}
	if (_abort_download)
		return;

	DWORD temp = WINHTTP_DISABLE_AUTHENTICATION;
	WinHttpSetOption(_http.request, WINHTTP_OPTION_DISABLE_FEATURE, &temp, sizeof(temp));

	int hcount = _user_heads.size();
	for (int i = 0; i < hcount; i++) {
		WCHAR header[MAX_PATH], hvalue[1024];
		header[0] = hvalue[0] = 0;
		auto h = _user_heads.front();
		_user_heads.pop();
		AnsiToUnicode(h.name, header);
		AnsiToUnicode(h.value, hvalue, _countof(hvalue));
		h.Free();
		auto str = (LPWSTR)malloc(4096);
		RtlZeroMemory(str, 4096);
		wcscpy(str, header);
		wcscat(str, L": ");
		wcscat(str, hvalue);
		WinHttpAddRequestHeaders(_http.request, str, -1, WINHTTP_ADDREQ_FLAG_ADD);
		free(str);
	}

	LPWSTR add_headers = NULL;
	if (append_headers) {
		int add_head_len = MultiByteToWideChar(CP_ACP, 0, append_headers, -1, NULL, 0);
		if (add_head_len > 1) {
			add_headers = (LPWSTR)calloc(2, add_head_len);
			if (add_headers)
				MultiByteToWideChar(CP_ACP, 0, append_headers, -1, add_headers, add_head_len + 1);
		}
	}
	if (!WinHttpSendRequest(_http.request,
		add_headers, -1,
		WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
		if (add_headers)
			free(add_headers);
		_http.Close();
		SetEvent(_events[EventErr]);
		return;
	}
	if (add_headers)
		free(add_headers);
	SetEvent(_events[EventSendRequest]);
	if (_abort_download)
		return;

	if (!WinHttpReceiveResponse(_http.request, NULL)) {
		_http.Close();
		SetEvent(_events[EventErr]);
		return;
	}

	//status code...
	temp = MAX_PATH;
	WCHAR scode[MAX_PATH] = {};
	WinHttpQueryHeaders(_http.request, WINHTTP_QUERY_STATUS_CODE,
		WINHTTP_HEADER_NAME_BY_INDEX, &scode, &temp, WINHTTP_NO_HEADER_INDEX);
	_http.status_code = wcstol(scode, NULL, 10);

	//headers...
	WinHttpQueryHeaders(_http.request, WINHTTP_QUERY_RAW_HEADERS_CRLF,
		WINHTTP_HEADER_NAME_BY_INDEX, NULL, (LPDWORD)&_recv_headers_zero_size, WINHTTP_NO_HEADER_INDEX);
	_recv_headers = (wchar_t*)calloc(_recv_headers_zero_size, 2);
	if (_recv_headers)
		WinHttpQueryHeaders(_http.request, WINHTTP_QUERY_RAW_HEADERS_CRLF,
		WINHTTP_HEADER_NAME_BY_INDEX, _recv_headers, (LPDWORD)&_recv_headers_zero_size, WINHTTP_NO_HEADER_INDEX);

	WinHttpQueryHeaders(_http.request, WINHTTP_QUERY_RAW_HEADERS,
		WINHTTP_HEADER_NAME_BY_INDEX, NULL, (LPDWORD)&_recv_headers_zero_size, WINHTTP_NO_HEADER_INDEX);
	_recv_headers_zero = (unsigned char*)calloc(_recv_headers_zero_size, 2);
	if (_recv_headers_zero)
		WinHttpQueryHeaders(_http.request, WINHTTP_QUERY_RAW_HEADERS,
		WINHTTP_HEADER_NAME_BY_INDEX, _recv_headers_zero, (LPDWORD)&_recv_headers_zero_size, WINHTTP_NO_HEADER_INDEX);

	SetEvent(_events[EventReceiveResponse]);
	if (_abort_download)
		return;

	if (_http.status_code >= 400)
		SetEvent(_events[EventStatusCode400]);
	else
		InternalDownload();
}

void WindowsHttpDownloader::InternalDownload()
{
	while (1)
	{
		if (_abort_download)
			break;
		WaitForSingleObjectEx(_event_download, INFINITE, FALSE);
		if (_abort_download)
			break;

		DWORD download_size = 0;
		if (!WinHttpQueryDataAvailable(_http.request, &download_size))
			break;
		if (download_size == 0)
			break;

		if (!_read_buf.Alloc(download_size))
			break;
		if (!WinHttpReadData(_http.request, _read_buf.Get<void*>(), download_size, &download_size))
			break;
		if (download_size == 0)
			break;
		
		std::lock_guard<decltype(_buf_lock)> lock(_buf_lock);
		_buffered.WriteBytes(_read_buf.Get<void*>(), download_size);
		if (!_buffered.IsBlockWriteable()) {
			ResetEvent(_event_download);
			SetEvent(_events[EventBuf]);
		}
		SetEvent(_events[EventDataAvailable]);
	}
	SetEvent(_events[EventEof]);
}

void WindowsHttpDownloader::InternalClose()
{
	if (_recv_headers)
		free(_recv_headers);
	if (_recv_headers_zero)
		free(_recv_headers_zero);

	int count = _user_heads.size();
	for (int i = 0; i < count; i++) {
		_user_heads.front().Free();
		_user_heads.pop();
	}

	CloseHandle(_event_download);
	for (int i = 0; i < _countof(_events); i++)
		CloseHandle(_events[i]);

	_http.Close();
}