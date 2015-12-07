#ifndef __DOWNLOAD_WINDOWS_LOCAL_FILE_H
#define __DOWNLOAD_WINDOWS_LOCAL_FILE_H

#include "downloader\DownloadCore.h"
#include <Windows.h>

namespace Downloader {
namespace Windows {

	class WindowsLocalFile : public Core::LocalFile
	{
	public:
		explicit WindowsLocalFile(LPCWSTR folder);
		explicit WindowsLocalFile(LPCSTR folder);

		virtual Core::CommonResult Create(
			Core::LocalFile::OpenMode open_mode,
			Core::LocalFile::ReadMode read_mode,
			const char* name, Core::LocalFile::LocalStream** stream);

		virtual Core::CommonResult Delete(const char* name);
		virtual bool IsExists(const char* name);

	protected:
		virtual ~WindowsLocalFile();

		class WindowsLocalStream : public Core::LocalFile::LocalStream
		{
			HANDLE _hFile;
		public:
			explicit WindowsLocalStream(HANDLE hFile);

			virtual void* GetPlatformData() { return _hFile; }
			virtual int Read(void* buf, int size);
			virtual int Write(const void* buf, int size);
			virtual bool Seek(int64_t offset);
			virtual void Flush() { FlushFileBuffers(_hFile); }
			virtual int64_t Tell();
			virtual int64_t GetSize();

		protected:
			virtual ~WindowsLocalStream();
		};

	private:
		LPWSTR _local_path;
	};
}}

#endif //__DOWNLOAD_WINDOWS_LOCAL_FILE_H