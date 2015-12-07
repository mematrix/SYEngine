#ifndef _MATROSKA_DATA_SOURCE_W32_H
#define _MATROSKA_DATA_SOURCE_W32_H

#ifdef _MSC_VER
#include <Windows.h>
#include <memory>

#include "MatroskaDataSource.h"

namespace MKV {
namespace DataSource {

class Win32DataSource : public IDataSource
{
	HANDLE _hFile;

public:
	explicit Win32DataSource(LPCWSTR lpszFile) throw()
	{
		_hFile = CreateFileW(lpszFile,GENERIC_READ,FILE_SHARE_READ,
			NULL,OPEN_EXISTING,0,NULL);
	}
	
	virtual ~Win32DataSource() throw()
	{
		if (_hFile != INVALID_HANDLE_VALUE)
			CloseHandle(_hFile);
	}

public:
	int Read(void* buffer,unsigned size)
	{
		if (_hFile == INVALID_HANDLE_VALUE)
			return MKV_DATA_SOURCE_READ_ERROR;

		DWORD dwResult = 0;
		if (!ReadFile(_hFile,buffer,size,&dwResult,NULL))
			return MKV_DATA_SOURCE_READ_ERROR;

		if (dwResult == 0)
			return MKV_DATA_SOURCE_READ_ERROR;

		return (int)dwResult;
	}

	int Seek(long long offset,int whence)
	{
		if (_hFile == INVALID_HANDLE_VALUE)
			return MKV_DATA_SOURCE_SEEK_ERROR;

		LARGE_INTEGER li;
		li.QuadPart = offset;
		if (!SetFilePointerEx(_hFile,li,&li,(DWORD)whence))
			return MKV_DATA_SOURCE_SEEK_ERROR;

		return 1;
	}

	long long Tell()
	{
		LARGE_INTEGER li,liPos;
		li.QuadPart = liPos.QuadPart = 0;

		SetFilePointerEx(_hFile,li,&liPos,FILE_CURRENT);
		return liPos.QuadPart;
	}

	long long Size()
	{
		LARGE_INTEGER li;
		li.QuadPart = 0;

		GetFileSizeEx(_hFile,&li);
		return li.QuadPart;
	}
};

//CreateWin32MKVDataSource (for Debug...)
std::shared_ptr<Win32DataSource> CreateWin32MKVDataSource(const wchar_t* filename)
{
	auto p = std::make_shared<Win32DataSource>(filename);
	return p;
}

}} //MKV::DataSource.

#endif //_MSC_VER

#endif //_MATROSKA_DATA_SOURCE_W32_H