#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __AUTO_COM_MEM_H
#define __AUTO_COM_MEM_H

#ifndef _WINDOWS_
#include <Windows.h>
#endif

template<class T>
class AutoComMem final
{
	PVOID _p;

public:
	AutoComMem() throw() : _p(nullptr) {}

	explicit AutoComMem(unsigned size) throw()
	{
		_p = CoTaskMemAlloc(size);
		if (_p)
			memset(_p,0,size);
	}

	~AutoComMem() throw()
	{
		if (_p)
			CoTaskMemFree(_p);
	}

public:
	T* operator()(unsigned size) throw()
	{
		if (_p != nullptr)
			CoTaskMemFree(_p);

		_p = CoTaskMemAlloc(size);
		if (_p)
			memset(_p,0,size);

		return Get();
	}

public:
	void* operator new(std::size_t) = delete;
	void* operator new[](std::size_t) = delete;

public:
	T* Get()
	{
		return (T*)_p;
	}

	void Free()
	{
		if (_p)
			CoTaskMemFree(_p);

		_p = nullptr;
	}
};

#endif //__AUTO_COM_MEM_H