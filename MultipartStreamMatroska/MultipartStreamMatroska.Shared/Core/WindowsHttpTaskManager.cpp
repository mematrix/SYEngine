#include "WindowsHttpTaskManager.h"

using namespace Downloader::Core;
using namespace Downloader::Windows;

bool WindowsHttpTaskManager::OnStartTask(int index)
{
	Item* file = InternalGetItem(index);
	if (file == NULL || file->Url == NULL)
		return false;
	if (file->Source != NULL)
		return true;

	WindowsHttpDataSource* src = new(std::nothrow) WindowsHttpDataSource(
		file->Url,
		file->UserAgent,
		file->RefUrl,
		file->Cookie,
		file->Timeout,
		_block_size_kb, _block_count);
	if (src == NULL)
		return false;
	file->Source = static_cast<DataSource*>(src);
	return true;
}

bool WindowsHttpTaskManager::OnStopTask(int index)
{
	Item* file = InternalGetItem(index);
	if (file->Source == NULL)
		return true;
	delete file->Source;
	file->Source = NULL;
	return true;
}

bool WindowsHttpTaskManager::OnPauseTask(int index)
{
	Item* file = InternalGetItem(index);
	if (file->Source == NULL)
		return false;
	file->Source->StopBuffering();
	return true;
}

bool WindowsHttpTaskManager::OnResumeTask(int index)
{
	Item* file = InternalGetItem(index);
	if (file->Source == NULL)
		return false;
	file->Source->EnableBuffering();
	return true;
}

bool WindowsHttpTaskManager::OnUpdateTask(int index, Item* new_task_info)
{
	Item* file = InternalGetItem(index);
	if (file->Source == NULL)
		return true;
	auto src = static_cast<WindowsHttpDataSource*>(file->Source);
	src->UpdateUrl(new_task_info->Url);
	return true;
}

void WindowsHttpTaskManager::DestroyTasks() throw()
{
	for (unsigned i = 0; i < GetTaskCapacity(); i++) {
		if (InternalGetItem(i)->Source != NULL)
			StopTask(i);
	}
}