#ifndef __DOWNLOAD_WINDOWS_HTTP_TASK_MANAGER_H
#define __DOWNLOAD_WINDOWS_HTTP_TASK_MANAGER_H

#include "WindowsHttpDataSource.h"
#include "downloader\TaskManagerBase.h"

namespace Downloader {
namespace Windows {

	class WindowsHttpTaskManager : public TaskManagerBase
	{
	public:
		WindowsHttpTaskManager(unsigned block_size_kb = 64, unsigned block_count = 80)
		{ _block_size_kb = block_size_kb; _block_count = block_count; }

	protected:
		virtual ~WindowsHttpTaskManager() {}

		virtual bool OnStartTask(int index);
		virtual bool OnStopTask(int index);

		virtual bool OnPauseTask(int index);
		virtual bool OnResumeTask(int index);

		virtual bool OnUpdateTask(int index, Item* new_task_info);

		virtual void OnUnload() { DestroyTasks(); }

	private:
		void DestroyTasks() throw();

		unsigned _block_size_kb;
		unsigned _block_count;
	};
}}

#endif //__DOWNLOAD_WINDOWS_HTTP_TASK_MANAGER_H