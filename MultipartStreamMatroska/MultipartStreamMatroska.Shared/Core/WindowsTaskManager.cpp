#include "WindowsTaskManager.h"
#include <new>

using namespace Downloader::Core;
using namespace Downloader::Windows;

bool WindowsTaskManager::OnStartTask(int index)
{
	Item* file = InternalGetItem(index);
	if (file == NULL || file->Url == NULL)
		return false;
	if (file->Source != NULL)
		return true;
	
	WCHAR szFilePath[MAX_PATH];
	MultiByteToWideChar(CP_UTF8, 0, file->Url, -1, szFilePath, MAX_PATH);
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	HANDLE hFile = CreateFileW(szFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
#else
	HANDLE hFile = CreateFile2(szFilePath, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, NULL);
#endif
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	Win32DataSource* src = new(std::nothrow) Win32DataSource(hFile);
	if (src == NULL) {
		CloseHandle(hFile);
		return false;
	}
	file->Source = static_cast<DataSource*>(src);
	return true;
}

bool WindowsTaskManager::OnStopTask(int index)
{
	Item* file = InternalGetItem(index);
	if (file->Source == NULL)
		return true;
	delete file->Source;
	file->Source = NULL;
	return true;
}

void WindowsTaskManager::DestroyTasks() throw()
{
	for (unsigned i = 0; i < GetTaskCapacity(); i++) {
		if (InternalGetItem(i)->Source != NULL)
			StopTask(i);
	}
}

CommonResult WindowsTaskManager::Win32DataSource::InitCheck()
{
	if (_hFile == INVALID_HANDLE_VALUE ||
		_hFile == NULL)
		return CommonResult::kNonInit;
	return CommonResult::kSuccess;
}

CommonResult WindowsTaskManager::Win32DataSource::ReadBytes(void* buf, unsigned size, unsigned* read_size)
{
	if (buf == NULL ||
		size == 0)
		return CommonResult::kInvalidInput;
	DWORD rs = 0;
	ReadFile(_hFile, buf, size, &rs, NULL);
	if (read_size)
		*read_size = rs;
	return CommonResult::kSuccess;
}

CommonResult WindowsTaskManager::Win32DataSource::SetPosition(int64_t offset)
{
	LARGE_INTEGER pos;
	pos.QuadPart = offset;
	return SetFilePointerEx(_hFile, pos, &pos, FILE_BEGIN) ? CommonResult::kSuccess : CommonResult::kError;
}

CommonResult WindowsTaskManager::Win32DataSource::GetLength(int64_t* size)
{
	if (size == NULL)
		return CommonResult::kInvalidInput;
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	LARGE_INTEGER s;
	GetFileSizeEx(_hFile, &s);
	*size = s.QuadPart;
#else
	FILE_STANDARD_INFO info = {};
	GetFileInformationByHandleEx(_hFile, FileStandardInfo, &info, sizeof(info));
	*size = info.EndOfFile.QuadPart;
#endif
	return CommonResult::kSuccess;
}