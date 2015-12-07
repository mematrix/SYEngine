#ifndef __DOWNLOAD_TASK_H
#define __DOWNLOAD_TASK_H

#include "DownloadCore.h"
#include <string.h>

namespace Downloader {
namespace Core {

	struct Task
	{
		char Url[2048];
		char RefUrl[2048];
		char Cookie[1024];
		char UserAgent[512];
		int Timeout;

		inline void SetUrl(const char* src) throw()
		{ strcpy(Url, src); }
		inline void SetOthers(const char* ref_u, const char* cook, const char* ua) throw()
		{
			if (ref_u)
				strcpy(RefUrl, ref_u);
			if (cook)
				strcpy(Cookie, cook);
			if (ua)
				strcpy(UserAgent, ua);
		}
	};

	typedef int TaskId; //based 0 index.
	struct TaskManager
	{
		TaskManager() {}
		virtual ~TaskManager() {}

		virtual CommonResult Initialize(LocalFile* delegated, void* userdata) = 0;
		virtual CommonResult Uninitialize() = 0;

		virtual CommonResult AddTask(Task* task, TaskId* id) = 0;
		virtual CommonResult RemoveTask(TaskId id) = 0; //±ÿ–Îœ»StopTask

		virtual unsigned GetTaskCapacity() { return INT32_MAX; }
		virtual unsigned GetTaskCount() = 0;
		virtual DataSource* GetDataSource(TaskId id) = 0;

		enum TaskState
		{
			kInvalid,
			kStopped,
			kStarted,
			kPaused
		};
		virtual TaskState GetTaskState(TaskId id) = 0;

		virtual CommonResult StartTask(TaskId id) = 0;
		virtual CommonResult StopTask(TaskId id) = 0;

		virtual CommonResult PauseTask(TaskId id) { return CommonResult::kNotImpl; }
		virtual CommonResult ResumeTask(TaskId id) { return CommonResult::kNotImpl; }

		bool IsStopped(TaskId id) throw()
		{ return GetTaskState(id) == TaskState::kStopped; }
		bool IsStarted(TaskId id) throw()
		{ return GetTaskState(id) == TaskState::kStarted; }
		bool IsPaused(TaskId id) throw()
		{ return GetTaskState(id) == TaskState::kPaused; }
	};
}}

#endif //__DOWNLOAD_TASK_H