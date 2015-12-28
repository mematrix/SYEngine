#ifndef __DOWNLOAD_WINDOWS_HTTP_DATASOURCE_H
#define __DOWNLOAD_WINDOWS_HTTP_DATASOURCE_H

#include "downloader\DownloadCore.h"
#include "WindowsHttpDownloader.h"
#include <mutex>

namespace Downloader {
namespace Windows {

	class WindowsHttpDataSource : public Core::DataSource
	{
	public:
		explicit WindowsHttpDataSource(const char* url,
			const char* ua = NULL,
			const char* ref_url = NULL,
			const char* cookie = NULL,
			int time_out_sec = 30,
			unsigned max_buf_one_block_size_kb = 64, unsigned max_buf_block_count = 80);
		virtual ~WindowsHttpDataSource() { DestroyObjects(); }

		virtual Core::CommonResult InitCheck();
		virtual Core::CommonResult ReadBytes(void* buf, unsigned size, unsigned* read_size);
		virtual Core::CommonResult SetPosition(int64_t offset);
		virtual Core::CommonResult GetLength(int64_t* size);

		virtual unsigned GetFlags() { return Core::DataSource::Flags::kFromRemoteLink; }
		virtual const char* GetMarker() { return "winhttp"; }
		virtual const char* GetMimeType() { return _mime_type; }

		virtual Core::CommonResult ReconnectAtOffset(int64_t offset);
		virtual Core::CommonResult Disconnect()
		{ DiscardDownloadTask(); return Core::CommonResult::kSuccess; }

		virtual Core::CommonResult EnableBuffering()
		{ if (_downloader) _downloader->ResumeBuffering(); return Core::CommonResult::kSuccess; }
		virtual Core::CommonResult StopBuffering()
		{ if (_downloader) _downloader->PauseBuffering(); return Core::CommonResult::kSuccess; }

		void UpdateUrl(const char* url)
		{ if (_cfgs.Url) free(_cfgs.Url); _cfgs.Url = strdup(url); }
		void UpdateRefUrl(const char* url)
		{ if (_cfgs.RefUrl) free(_cfgs.RefUrl); _cfgs.RefUrl = strdup(url); }
		void UpdateCookie(const char* cookie)
		{ if (_cfgs.Cookie) free(_cfgs.Cookie); _cfgs.Cookie = strdup(cookie); }
		void UpdateAppendHeaders(const char* headers)
		{ if (_cfgs.AppendHeaders) free(_cfgs.AppendHeaders); _cfgs.AppendHeaders = strdup(headers); }

	private:
		void DestroyObjects();

		bool CheckForAppendHeaders(const char* name);
		void ConfigRequestHeaders();
		bool StartDownloadTask(bool init = true);

		bool WaitForResponseHeaders();
		void DiscardDownloadTask();

		unsigned ReadInBuffered(void* buf, unsigned size);
		unsigned ReadInNetwork(void* buf, unsigned size, int64_t cur_pos);
		enum NetworkDownloadStatus
		{
			EventAbort = 0,
			EventTimeout,
			EventError,
			EventEof = 100,
			EventBuffered,
			EventDataAvailable
		};
		NetworkDownloadStatus WaitNetworkDownloadStatus();

		inline void ThrowDownloadAbort() { SetEvent(_abort_event); }
		inline void WaitProcessDownloadAbort() { WaitForSingleObjectEx(_process_abort_event, 60 * 1000, FALSE); }
		inline void NotifyCompleteDownloadAbort() { SetEvent(_process_abort_event); }

		struct InitConfigs
		{
			char* Url;
			char* RefUrl;
			char* Cookie;
			char* UserAgent;
			char* AppendHeaders;
			int Timeout;
			unsigned MaxBlockSizeKb, MaxBlockCount;
			InitConfigs() { Init(); }

			inline void Init() throw()
			{ Url = RefUrl = Cookie = UserAgent = AppendHeaders = NULL;
			  Timeout = 0; MaxBlockSizeKb = MaxBlockCount = 0; }

			inline void Free() throw()
			{
				if (Url)
					free(Url);
				if (RefUrl)
					free(RefUrl);
				if (Cookie)
					free(Cookie);
				if (UserAgent)
					free(UserAgent);
				if (AppendHeaders)
					free(AppendHeaders);
				Init();
			}
		};
		InitConfigs _cfgs;

		WindowsHttpDownloader* _downloader;
		int64_t _content_len;
		char _mime_type[128];

		int64_t _cur_pos;
		bool _download_eof;

		HANDLE _abort_event, _process_abort_event;
		bool _init_with_wait_headers, _read_more_from_network;

		std::recursive_mutex _mutex;
	};
}}

#endif //__DOWNLOAD_WINDOWS_HTTP_DATASOURCE_H