#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __W32_CRIT_SEC_H
#define __W32_CRIT_SEC_H

#ifndef _WINDOWS_
#include <Windows.h>
#endif

class CritSec
{
	CRITICAL_SECTION _cs;

public:
	CritSec()
	{
		InitializeCriticalSectionEx(&_cs,0,CRITICAL_SECTION_NO_DEBUG_INFO);
	}

	~CritSec()
	{
		DeleteCriticalSection(&_cs);
	}

	CritSec(const CritSec&) = delete;
	CritSec& operator=(const CritSec&) = delete;

	CritSec(CritSec&&) = delete;
	CritSec& operator=(CritSec&&) = delete;

public:
	void Lock() throw()
	{
		EnterCriticalSection(&_cs);
	}

	void Unlock() throw()
	{
		LeaveCriticalSection(&_cs);
	}

public:
	CRITICAL_SECTION& RefObj()
	{
		return _cs;
	}

public:
	class AutoLock
	{
		CritSec* _cs;

	public:
		explicit AutoLock(CritSec* cs) : _cs(cs) { _cs->Lock(); }
		explicit AutoLock(CritSec& cs) : _cs(&cs) { _cs->Lock(); }
		~AutoLock() { _cs->Unlock(); }

	public:
		AutoLock(const AutoLock&) = delete;
		AutoLock& operator=(const AutoLock&) = delete;

		AutoLock(AutoLock&&) = delete;
		AutoLock& operator=(AutoLock&&) = delete;

	public:
		void* operator new(std::size_t) = delete;
		void* operator new[](std::size_t) = delete;
	};
};

#endif //__W32_CRIT_SEC_H