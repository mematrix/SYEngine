#ifndef __DOWNLOAD_CORE_H
#define __DOWNLOAD_CORE_H

#include <cstdio>
#include <cstdint>

namespace Downloader {
namespace Core {

	enum CommonResult
	{
		kSuccess = 0,
		kError,
		kNonInit,
		kNotImpl,
		kUnsupported,
		kInvalidInput,
	};

	struct DataSource
	{
		DataSource() {}
		virtual ~DataSource() {}

		enum Flags
		{
			kFromLocalHost = 1,
			kFromRemoteLink = 2,
			kNonSeekable = 4
		};

		virtual CommonResult InitCheck() = 0;
		virtual CommonResult ReadBytes(void* buf, unsigned size, unsigned* read_size) = 0;
		virtual CommonResult SetPosition(int64_t offset) = 0;
		virtual CommonResult GetLength(int64_t* size) = 0;

		virtual unsigned GetFlags() { return 0; }
		virtual const char* GetMarker() { return NULL; }
		virtual const char* GetMimeType() { return NULL; }

		virtual CommonResult ReconnectAtOffset(int64_t offset) { return kUnsupported; }
		virtual CommonResult Disconnect() { return kUnsupported; }

		virtual CommonResult EnableBuffering() { return kNotImpl; }
		virtual CommonResult StopBuffering() { return kNotImpl; }
	};

	struct LocalFile
	{
		LocalFile() {}
		virtual ~LocalFile() {}

		struct LocalStream
		{
			LocalStream() {}
			virtual ~LocalStream() {}

			virtual void* GetPlatformData() { return NULL; }

			virtual int Read(void* buf, int size) = 0;
			virtual int Write(const void* buf, int size) = 0;
			virtual void Flush() {}

			virtual bool Seek(int64_t offset) = 0;
			virtual int64_t Tell() = 0;
			virtual int64_t GetSize() = 0;
		};

		enum OpenMode
		{
			CreateNew,
			OpenExists
		};
		enum ReadMode
		{
			ReadOnly,
			ReadWrite
		};
		virtual CommonResult Create(OpenMode open_mode, ReadMode read_mode, const char* name, LocalStream** stream) = 0;
		virtual CommonResult Delete(const char* name) = 0;
		virtual bool IsExists(const char* name) = 0;

		virtual CommonResult New(const char* name, LocalFile** new_file)
		{ return CommonResult::kUnsupported; }
	};
}}

#endif //__DOWNLOAD_CORE_H