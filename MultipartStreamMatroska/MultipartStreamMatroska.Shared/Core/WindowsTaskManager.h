#ifndef __DOWNLOAD_WINDOWS_TASK_MANAGER_H
#define __DOWNLOAD_WINDOWS_TASK_MANAGER_H

#include "downloader\TaskManagerBase.h"
#include <Windows.h>

namespace Downloader {
namespace Windows {

	class WindowsTaskManager : public TaskManagerBase
	{
	public:
		WindowsTaskManager() {}

	protected:
		virtual ~WindowsTaskManager() { DestroyTasks(); }

		virtual bool OnStartTask(int index);
		virtual bool OnStopTask(int index);
		
	private:
		void DestroyTasks() throw();

		class Win32DataSource : public Core::DataSource
		{
			HANDLE _hFile;
		public:
			explicit Win32DataSource(HANDLE hFile) throw()
			{ _hFile = hFile; }
			virtual ~Win32DataSource() throw()
			{ if (_hFile != INVALID_HANDLE_VALUE) CloseHandle(_hFile); }

			virtual Core::CommonResult InitCheck();
			virtual Core::CommonResult ReadBytes(void* buf, unsigned size, unsigned* read_size);
			virtual Core::CommonResult SetPosition(int64_t offset);
			virtual Core::CommonResult GetLength(int64_t* size);

			virtual unsigned GetFlags() { return Core::DataSource::Flags::kFromLocalHost; }
			virtual const char* GetMarker() { return "Win32"; }
		};
	};
}}

#endif //__DOWNLOAD_WINDOWS_TASK_MANAGER_H