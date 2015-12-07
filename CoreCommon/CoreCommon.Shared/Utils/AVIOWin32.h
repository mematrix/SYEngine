#ifdef _MSC_VER

#pragma once

#include <memory>
#include <Windows.h>
#include "MediaAVIO.h"

class Win32AVIO : public IAVMediaIO
{
	HANDLE _file;

public:
	explicit Win32AVIO(HANDLE hFile) throw() : _file(hFile) {}
	virtual ~Win32AVIO()
	{ if (_file != INVALID_HANDLE_VALUE) CloseHandle(_file); }

public:
	unsigned Read(void* pb,unsigned size)
	{
		DWORD dwReadSize = 0;
		ReadFile(_file,pb,size,&dwReadSize,nullptr);
		return dwReadSize;
	}

	unsigned Write(void* pb,unsigned size)
	{
		DWORD dwWriteSize = 0;
		WriteFile(_file,pb,size,&dwWriteSize,nullptr);
		return dwWriteSize;
	}

	bool Seek(long long offset,int whence)
	{
		LARGE_INTEGER pos;
		pos.QuadPart = offset;
		return SetFilePointerEx(_file,pos,&pos,(DWORD)whence) ? true:false;
	}

	long long Tell()
	{
		LARGE_INTEGER cur,pos;
		cur.QuadPart = pos.QuadPart = 0;
		SetFilePointerEx(_file,cur,&pos,FILE_CURRENT);
		return pos.QuadPart;
	}

	long long GetSize()
	{
		LARGE_INTEGER size;
		size.QuadPart = 0;
		GetFileSizeEx(_file,&size);
		return size.QuadPart;
	}

	bool GetPlatformSpec(void** pp,int* spec_code)
	{
		if (pp == nullptr)
			return false;
		*pp = _file;
		return true;
	}
};

inline std::shared_ptr<Win32AVIO> CreateWin32AVIO(HANDLE hFile)
{
	return std::make_shared<Win32AVIO>(hFile);
}

#endif