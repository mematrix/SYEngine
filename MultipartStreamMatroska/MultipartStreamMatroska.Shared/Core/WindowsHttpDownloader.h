#ifndef __WINDOWS_HTTP_DOWNLOADER_H
#define __WINDOWS_HTTP_DOWNLOADER_H

#include <Windows.h>
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#include <winhttp.h>
#else
#include <combaseapi.h>
#include "winhttp_runtime.h" //crack...
#endif
#include <queue>
#include <mutex>

#include "MemoryStream.h"
#include "RingBlockBuffer.h"

#include "..\ThreadImpl.h"

//#pragma comment(lib, "winhttp.lib")

class WindowsHttpDownloader : public IUnknown, private ThreadImpl
{
	ULONG _ref_count;
public: //IUnknown
	STDMETHODIMP_(ULONG) AddRef()
	{ return InterlockedIncrement(&_ref_count); }
	STDMETHODIMP_(ULONG) Release()
	{ auto rc = InterlockedDecrement(&_ref_count); if (rc == 0) delete this; return rc; }
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv)
	{ return E_NOINTERFACE; }

public:
	bool Initialize(const char* user_agent, int timeout_sec = 0, unsigned buf_block_size_kb = 64, unsigned buf_block_count = 80);
	bool SetRequestHeader(const char* name, const char* value);
	bool StartAsync(const char* url, const char* append_headers = NULL);
	void AbortAsync()
	{ InterlockedExchange(&_abort_download, 1); }

	const wchar_t* GetResponseHeaders()
	{ return _recv_headers; }
	const wchar_t* GetResponseHeader(const char* name, bool text_match_case = true);

	int GetStatusCode() const throw() { return _http.status_code; }
	bool GetIsChunked() throw();
	bool GetIsGZip() throw();

	unsigned GetBufferedReadableSize() throw();
	unsigned ReadBufferedBytes(void* buf, unsigned size) throw();
	bool ForwardSeekInBufferedReadableSize(unsigned skip_bytes) throw();

	void PauseBuffering();
	void ResumeBuffering(bool force = false);

	enum AsyncEvents
	{
		kError = 0,
		kBuffered = 1,
		kEof = 2,
		kConnected = 3,
		kSendComplete = 4,
		kRecvComplete = 5,
		kDataAvailable = 6,
		kStatusCode400 = 7,
	};
	HANDLE GetAsyncEventHandle(AsyncEvents types) const throw()
	{ return _events[types]; }

	WindowsHttpDownloader() throw() :
		_ref_count(1),
		_abort_download(0),
		_recv_headers(NULL), _recv_headers_zero(NULL) {
		RtlZeroMemory(&_events, sizeof(HANDLE) * _countof(_events));
		_event_download = CreateEventExW(NULL, NULL,
			CREATE_EVENT_MANUAL_RESET|CREATE_EVENT_INITIAL_SET,
			EVENT_ALL_ACCESS);
	}

private:
	~WindowsHttpDownloader() throw() { InternalClose(); }

	void InternalClose();
	void InternalConnect(char* append_headers);
	void InternalDownload();

	virtual void ThreadInvoke(void* param) { AddRef(); InternalConnect((char*)param); if (param) free(param); }
	virtual void ThreadEnded() { Release(); }

private:
	MemoryBuffer _read_buf;
	RingBlockBuffer _buffered;

	std::recursive_mutex _buf_lock;
	volatile unsigned _abort_download;

	struct WHttp
	{
		int status_code;
		HINTERNET session;
		HINTERNET connect;
		HINTERNET request;
		WHttp() throw() { Init(); status_code = 200; }

		inline void Init() throw()
		{ session = connect = request = NULL; }
		inline void Close() throw()
		{
			if (request)
				WinHttpCloseHandle(request);
			if (connect)
				WinHttpCloseHandle(connect);
			if (session)
				WinHttpCloseHandle(session);
			Init();
		}
	};
	WHttp _http;

	enum EventTypes
	{
		EventErr = 0, //手动重置
		EventBuf = 1, //自动重置
		EventEof = 2, //手动重置
		EventConnect = 3, //手动重置
		EventSendRequest = 4, //手动重置
		EventReceiveResponse = 5, //手动重置
		EventDataAvailable = 6, //自动重置
		EventStatusCode400 = 7, //手动重置
		EventMaxCount
	};
	HANDLE _events[EventTypes::EventMaxCount];
	HANDLE _event_download;

	WCHAR _urls[2048];
	URL_COMPONENTS _url;
	struct KVHeader
	{
		char* name;
		char* value;

		inline void SetName(const char* n) throw()
		{ if (n) name = strdup(n); }
		inline void SetValue(const char* v) throw()
		{ if (v) value = strdup(v); }

		inline void Free() throw()
		{
			if (name)
				free(name);
			if (value)
				free(value);
			name = value = NULL;
		}
	};
	std::queue<KVHeader> _user_heads;

	wchar_t* _recv_headers;
	unsigned char* _recv_headers_zero;
	unsigned _recv_headers_zero_size;
};

#endif //__WINDOWS_HTTP_DOWNLOADER_H