#include "TaskManagerBase.h"

#define CHECK_TASKID(id) \
	if ((id) > _MAX_DOWNLOAD_ITEM_COUNT) \
	return CommonResult::kUnsupported

using namespace Downloader;
using namespace Downloader::Core;

CommonResult TaskManagerBase::Initialize(LocalFile* delegated, void* userdata)
{
	_local_file = delegated;
	_user_data = userdata;
	return OnLoad() ? CommonResult::kSuccess : CommonResult::kError;
}

CommonResult TaskManagerBase::Uninitialize()
{
	OnUnload();
	if (_local_file)
		delete _local_file;
	_local_file = NULL;
	_user_data = NULL;
	return CommonResult::kSuccess;
}

CommonResult TaskManagerBase::AddTask(Task* task, TaskId* id)
{
	Item item = {};
	int index = FindAllowItemIndex();
	if (index == -1)
		return CommonResult::kUnsupported;
	if (!CopyTaskToItem(task, &item))
		return CommonResult::kInvalidInput;

	if (!OnNewTask(&item, index)) {
		FreeItem(&item);
		return CommonResult::kError;
	}
	*id = index;
	_items[index] = item;
	return CommonResult::kSuccess;
}

CommonResult TaskManagerBase::RemoveTask(TaskId id)
{
	CHECK_TASKID(id);
	if (_items[id].State != TaskManager::TaskState::kInvalid &&
		_items[id].State != TaskManager::TaskState::kStopped)
		return CommonResult::kError;
	if (!OnRemoveTask(id))
		return CommonResult::kError;
	FreeItem(&_items[id]);
	memset(&_items[id], 0, sizeof(Item));
	return CommonResult::kSuccess;
}

CommonResult TaskManagerBase::UpdateTask(TaskId id, Task* new_task_info)
{
	CHECK_TASKID(id);
	Item item = {};
	if (!CopyTaskToItem(new_task_info, &item))
		return CommonResult::kInvalidInput;
	if (!OnUpdateTask(id, &item))
		return CommonResult::kError;
	item.Source = _items[id].Source;
	item.Context = _items[id].Context;
	item.State = _items[id].State;
	FreeItem(&_items[id]);
	_items[id] = item;
	return CommonResult::kSuccess;
}

unsigned TaskManagerBase::GetTaskCount()
{
	unsigned count = 0;
	for (int i = 0; i < _MAX_DOWNLOAD_ITEM_COUNT; i++) {
		if (_items[i].Url != NULL)
			++count;
	}
	return count;
}

DataSource* TaskManagerBase::GetDataSource(TaskId id)
{
	if (id >= _MAX_DOWNLOAD_ITEM_COUNT)
		return NULL;
	return _items[id].Source;
}

TaskManager::TaskState TaskManagerBase::GetTaskState(TaskId id)
{
	if (id >= _MAX_DOWNLOAD_ITEM_COUNT)
		return TaskManager::TaskState::kInvalid;
	return _items[id].State;
}

CommonResult TaskManagerBase::StartTask(TaskId id)
{
	CHECK_TASKID(id);
	if (_items[id].State == TaskManager::TaskState::kStarted)
		return CommonResult::kSuccess;
	if (!OnStartTask(id))
		return CommonResult::kError;
	if (_items[id].Source == NULL)
		return CommonResult::kNonInit;
	if (_items[id].Source->InitCheck() != CommonResult::kSuccess)
		return CommonResult::kNonInit;
	_items[id].State = TaskManager::TaskState::kStarted;
	return CommonResult::kSuccess;
}

CommonResult TaskManagerBase::StopTask(TaskId id)
{
	CHECK_TASKID(id);
	if (_items[id].State == TaskManager::TaskState::kStopped)
		return CommonResult::kSuccess;
	if (!OnStopTask(id))
		return CommonResult::kError;
	_items[id].State = TaskManager::TaskState::kStopped;
	return CommonResult::kSuccess;
}

CommonResult TaskManagerBase::PauseTask(TaskId id)
{
	CHECK_TASKID(id);
	if (_items[id].State == TaskManager::TaskState::kPaused)
		return CommonResult::kSuccess;
	if (!OnPauseTask(id))
		return CommonResult::kError;
	_items[id].State = TaskManager::TaskState::kPaused;
	return CommonResult::kSuccess;
}

CommonResult TaskManagerBase::ResumeTask(TaskId id)
{
	CHECK_TASKID(id);
	if (_items[id].State == TaskManager::TaskState::kStarted)
		return CommonResult::kSuccess;
	if (!OnResumeTask(id))
		return CommonResult::kError;
	_items[id].State = TaskManager::TaskState::kStarted;
	return CommonResult::kSuccess;
}

int TaskManagerBase::FindAllowItemIndex()
{
	for (int i = 0; i < _MAX_DOWNLOAD_ITEM_COUNT; i++)
	{
		if (_items[i].Url == NULL)
			return i;
	}
	return -1;
}

bool TaskManagerBase::CopyTaskToItem(Task* task, Item* item)
{
	if (task == NULL || item == NULL)
		return false;
	if (task->Url[0] == 0)
		return false;

	item->State = TaskManager::TaskState::kStopped;
	item->Url = strdup(task->Url);
	if (item->Url == NULL)
		return false;
	if (task->RefUrl[0] != 0)
		item->RefUrl = strdup(task->RefUrl);
	if (task->Cookie[0] != 0)
		item->Cookie = strdup(task->Cookie);
	if (task->UserAgent[0] != 0)
		item->UserAgent = strdup(task->UserAgent);
	item->Timeout = task->Timeout;
	return true;
}

void TaskManagerBase::FreeItem(Item* item)
{
	if (item->Url)
		free(item->Url);
	if (item->RefUrl)
		free(item->RefUrl);
	if (item->Cookie)
		free(item->Cookie);
	if (item->UserAgent)
		free(item->UserAgent);
	item->Url = item->RefUrl = item->Cookie = item->UserAgent = NULL;
}

void TaskManagerBase::FreeAllItems()
{
	for (int i = 0; i < _MAX_DOWNLOAD_ITEM_COUNT; i++) {
		if (_items[i].Url != NULL)
			FreeItem(&_items[i]);
	}
}