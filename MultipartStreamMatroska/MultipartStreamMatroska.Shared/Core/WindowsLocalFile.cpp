#include "WindowsLocalFile.h"
#include <new>

using namespace Downloader::Core;
using namespace Downloader::Windows;

inline static bool AnsiToUnicode(const char* ansi, LPWSTR uni, unsigned max_u_len = MAX_PATH)
{ return MultiByteToWideChar(CP_ACP, 0, ansi, -1, uni, max_u_len) > 0; }

WindowsLocalFile::WindowsLocalFile(LPCWSTR folder) : _local_path(NULL)
{
	if (folder != NULL) {
		_local_path = _wcsdup(folder);
		CreateDirectoryW(folder, NULL);
	}
}

WindowsLocalFile::WindowsLocalFile(LPCSTR folder) : _local_path(NULL)
{
	if (folder != NULL) {
		WCHAR temp[MAX_PATH] = {};
		AnsiToUnicode(folder, temp);
		_local_path = _wcsdup(temp);
		CreateDirectoryW(temp, NULL);
	}
}

WindowsLocalFile::~WindowsLocalFile()
{
	if (_local_path)
		free(_local_path);
	_local_path = NULL;
}

CommonResult WindowsLocalFile::Create(OpenMode open_mode, ReadMode read_mode, const char* name, LocalStream** stream)
{
	if (name == NULL ||
		stream == NULL)
		return CommonResult::kInvalidInput;

	WCHAR file_path[MAX_PATH] = {};
	if (!AnsiToUnicode(name, file_path))
		return CommonResult::kError;

	WCHAR path[MAX_PATH * 2];
	if (_local_path) {
		wcscpy_s(path, _local_path);
		wcscat_s(path, L"\\");
	}
	wcscat_s(path, file_path);

	DWORD openflags = (open_mode == OpenMode::CreateNew ? CREATE_ALWAYS : OPEN_EXISTING);
	DWORD rwflags = (read_mode == ReadMode::ReadOnly ? GENERIC_READ : GENERIC_READ|GENERIC_WRITE);
	DWORD shflags = (read_mode == ReadMode::ReadOnly ? FILE_SHARE_READ : 0);
	if (_local_path == NULL)
		openflags = OPEN_EXISTING;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	auto hFile = CreateFileW(path, rwflags, shflags, NULL, openflags, 0, NULL);
#else
	auto hFile = CreateFile2(path, rwflags, shflags, openflags, NULL);
#endif
	if (hFile == INVALID_HANDLE_VALUE)
		return CommonResult::kError;

	auto stm = new(std::nothrow) WindowsLocalStream(hFile);
	if (stm == NULL) {
		CloseHandle(hFile);
		return CommonResult::kError;
	}

	*stream = stm;
	return CommonResult::kSuccess;
}

CommonResult WindowsLocalFile::Delete(const char* name)
{
	if (name == NULL)
		return CommonResult::kInvalidInput;
	if (_local_path == NULL)
		return CommonResult::kSuccess;

	WCHAR file_path[MAX_PATH] = {};
	if (!AnsiToUnicode(name, file_path))
		return CommonResult::kError;

	WCHAR path[MAX_PATH * 2];
	wcscpy_s(path, _local_path);
	wcscat_s(path, L"\\");
	wcscat_s(path, file_path);
	return DeleteFileW(path) ? CommonResult::kSuccess : CommonResult::kError;
}

bool WindowsLocalFile::IsExists(const char* name)
{
	if (name == NULL)
		return false;

	WCHAR file_path[MAX_PATH] = {};
	if (!AnsiToUnicode(name, file_path))
		return false;

	WCHAR path[MAX_PATH * 2];
	if (_local_path) {
		wcscpy_s(path, _local_path);
		wcscat_s(path, L"\\");
	}
	wcscat_s(path, file_path);

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	auto file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
#else
	auto file = CreateFile2(path, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, NULL);
#endif
	if (file != INVALID_HANDLE_VALUE)
		CloseHandle(file);
	return file != INVALID_HANDLE_VALUE;
}

WindowsLocalFile::WindowsLocalStream::WindowsLocalStream(HANDLE hFile) : _hFile(hFile)
{
}

WindowsLocalFile::WindowsLocalStream::~WindowsLocalStream()
{
	if (_hFile != INVALID_HANDLE_VALUE) {
		FlushFileBuffers(_hFile);
		CloseHandle(_hFile);
	}
	_hFile = INVALID_HANDLE_VALUE;
}

int WindowsLocalFile::WindowsLocalStream::Read(void* buf, int size)
{
	if (buf == NULL || size == 0)
		return 0;
	DWORD len = 0;
	ReadFile(_hFile, buf, size, &len, NULL);
	return len;
}

int WindowsLocalFile::WindowsLocalStream::Write(const void* buf, int size)
{
	if (buf == NULL || size == 0)
		return 0;
	DWORD len = 0;
	WriteFile(_hFile, buf, size, &len, NULL);
	return len;
}

bool WindowsLocalFile::WindowsLocalStream::Seek(int64_t offset)
{
	LARGE_INTEGER li;
	li.QuadPart = offset;
	return SetFilePointerEx(_hFile, li, &li, FILE_BEGIN) ? true : false;
}

int64_t WindowsLocalFile::WindowsLocalStream::Tell()
{
	LARGE_INTEGER li0, li1;
	li0.QuadPart = li1.QuadPart = 0;
	SetFilePointerEx(_hFile, li0, &li1, FILE_CURRENT);
	return li1.QuadPart;
}

int64_t WindowsLocalFile::WindowsLocalStream::GetSize()
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	LARGE_INTEGER li;
	li.QuadPart = 0;
	GetFileSizeEx(_hFile, &li);
	return li.QuadPart;
#else
	FILE_STANDARD_INFO info = {};
	GetFileInformationByHandleEx(_hFile, FileStandardInfo, &info, sizeof(info));
	return info.EndOfFile.QuadPart;
#endif
}