#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __WIN32_EVENT_H
#define __WIN32_EVENT_H

#ifndef _WINDOWS_
#include <Windows.h>
#endif

class Win32Event final
{
	HANDLE _hEvent;
	BOOL _bNoClose;

public:
	explicit Win32Event(HANDLE hEvent) throw() : _hEvent(hEvent), _bNoClose(TRUE) {}
	explicit Win32Event(DWORD dwFlags = 0) throw() : _bNoClose(FALSE)
	{
		_hEvent = CreateEventExW(nullptr,nullptr,dwFlags,EVENT_ALL_ACCESS);
	}

	Win32Event(const Win32Event& r) throw()
	{
		_bNoClose  = FALSE;
		DuplicateHandle(GetCurrentProcess(),r._hEvent,GetCurrentProcess(),&_hEvent,
			0,FALSE,DUPLICATE_SAME_ACCESS);
	}

	Win32Event(Win32Event&& r) throw()
	{
		_bNoClose = r._bNoClose;
		_hEvent = r._hEvent;
	}

	~Win32Event()
	{
		if (!_bNoClose)
		{
			if (_hEvent)
				CloseHandle(_hEvent);
		}
	}

public:

	bool operator==(const Win32Event& r) throw()
	{
		return _hEvent == r._hEvent;
	}
	bool operator!=(const Win32Event& r) throw()
	{
		return _hEvent != r._hEvent;
	}

public:
	void Set()
	{
		SetEvent(_hEvent);
	}

	void Reset()
	{
		ResetEvent(_hEvent);
	}

	bool Wait(DWORD dwTimeout = INFINITE)
	{
		return WaitForSingleObjectEx(_hEvent,dwTimeout,FALSE) == WAIT_OBJECT_0;
	}

	bool Check()
	{
		return Wait(0);
	}
};

#endif