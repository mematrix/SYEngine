#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __AUTO_WIN_HANDLE_H
#define __AUTO_WIN_HANDLE_H

#ifndef _WINDOWS_
#include <Windows.h>
#endif

class AutoWinHandle final
{
	HANDLE _handle;

public:
	AutoWinHandle() throw() : _handle(INVALID_HANDLE_VALUE) {}
	AutoWinHandle(HANDLE hObject) throw() //Non-explicit.
	{
		_handle = hObject;
	}

	~AutoWinHandle() throw()
	{
		__try{
			if (_handle != INVALID_HANDLE_VALUE &&
				_handle != nullptr)
				CloseHandle(_handle);
		}__except(EXCEPTION_EXECUTE_HANDLER) {}
	}

public:
	HANDLE Get() const throw()
	{
		return _handle;
	}

public:
	void* operator new(std::size_t) = delete;
	void* operator new[](std::size_t) = delete;
};

#endif //__AUTO_WIN_HANDLE_H