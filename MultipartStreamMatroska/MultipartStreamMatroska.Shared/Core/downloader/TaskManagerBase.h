#ifndef __DOWNLOAD_TASK_MANAGER_BASE_H
#define __DOWNLOAD_TASK_MANAGER_BASE_H

#include "DownloadTask.h"
#include <malloc.h>
#include <memory.h>
#include <string.h>

#define _MAX_DOWNLOAD_ITEM_COUNT 128

namespace Downloader {

	class TaskManagerBase : public Core::TaskManager
	{
	public:
		virtual Core::CommonResult Initialize(Core::LocalFile* delegated, void* userdata);
		virtual Core::CommonResult Uninitialize();

		virtual Core::CommonResult AddTask(Core::Task* task, Core::TaskId* id);
		virtual Core::CommonResult RemoveTask(Core::TaskId id);

		virtual unsigned GetTaskCapacity() { return _MAX_DOWNLOAD_ITEM_COUNT; }
		virtual unsigned GetTaskCount();
		virtual Core::DataSource* GetDataSource(Core::TaskId id);

		virtual Core::TaskManager::TaskState GetTaskState(Core::TaskId id);
		virtual Core::CommonResult StartTask(Core::TaskId id);
		virtual Core::CommonResult StopTask(Core::TaskId id);
		virtual Core::CommonResult PauseTask(Core::TaskId id);
		virtual Core::CommonResult ResumeTask(Core::TaskId id);

	protected:
		TaskManagerBase() : _local_file(NULL), _user_data(NULL)
		{ memset(_items, 0, sizeof(Item) * _MAX_DOWNLOAD_ITEM_COUNT); }
		virtual ~TaskManagerBase() { FreeAllItems(); }

		struct Item
		{
			char* Url;
			char* RefUrl;
			char* Cookie;
			char* UserAgent;
			int Timeout;
			Core::TaskManager::TaskState State;
			Core::DataSource* Source;
			void* Context;
		};

		inline Core::LocalFile* InternalGetLocalFile() throw()
		{ return _local_file; }
		inline void* InternalGetUserdata() throw()
		{ return _user_data; }
		inline Item* InternalGetItem(int index) throw()
		{ if (index >= _MAX_DOWNLOAD_ITEM_COUNT) return NULL; return &_items[index]; }

		virtual bool OnLoad() { return true; }
		virtual void OnUnload() {}

		virtual bool OnNewTask(Item* task, int index) { return true; } //还没有添加到list
		virtual bool OnRemoveTask(int index) { return true; }

		virtual bool OnStartTask(int index) { return false; }
		virtual bool OnStopTask(int index) { return false; }
		virtual bool OnPauseTask(int index) { return true; }
		virtual bool OnResumeTask(int index) { return true; }

	private:
		int FindAllowItemIndex();
		bool CopyTaskToItem(Core::Task* task, Item* item);

		void FreeItem(Item* item);
		void FreeAllItems();

	private:
		Core::LocalFile* _local_file;
		void* _user_data;
		Item _items[_MAX_DOWNLOAD_ITEM_COUNT];
	};
}

#endif //__DOWNLOAD_TASK_MANAGER_BASE_H