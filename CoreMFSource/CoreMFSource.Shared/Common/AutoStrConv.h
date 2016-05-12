#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __AUTO_STR_CONV_H
#define __AUTO_STR_CONV_H

#ifndef _WINDOWS_
#include <Windows.h>
#endif

class AutoStrConv final
{
	PVOID _pb;
	UINT _cp;

public:
	AutoStrConv() throw() : _pb(nullptr), _cp(CP_ACP) {}

	explicit AutoStrConv(const char* pAnsiStr,unsigned cp = CP_ACP) throw() : _pb(nullptr), _cp(cp)
	{
		if (pAnsiStr == nullptr)
			return;

		int len = MultiByteToWideChar(_cp,0,pAnsiStr,-1,nullptr,0);
		if (len > 0)
		{
			_pb = _aligned_malloc((len + 1) * 2,4);
			MultiByteToWideChar(_cp,0,pAnsiStr,-1,(LPWSTR)_pb,len + 1);
		}
	}

	explicit AutoStrConv(const wchar_t* pUniStr,unsigned cp = CP_ACP) throw() : _pb(nullptr), _cp(cp)
	{
		if (pUniStr == nullptr)
			return;

		int len = WideCharToMultiByte(_cp,0,pUniStr,-1,nullptr,0,nullptr,nullptr);
		if (len > 0)
		{
			_pb = _aligned_malloc(len + 1,4);
			WideCharToMultiByte(_cp,0,pUniStr,-1,(LPSTR)_pb,len + 1,nullptr,nullptr);
		}
	}

	~AutoStrConv()
	{
		if (_pb)
			_aligned_free(_pb);
	}

public:
	const wchar_t* operator()(const char* pAnsiStr,unsigned cp = CP_ACP) throw()
	{
		if (_pb)
			_aligned_free(_pb);
		_pb = nullptr;

		if (pAnsiStr == nullptr)
			return nullptr;

		int len = MultiByteToWideChar(cp,0,pAnsiStr,-1,nullptr,0);
		if (len > 0)
		{
			_pb = _aligned_malloc((len + 1) * 2,4);
			MultiByteToWideChar(cp,0,pAnsiStr,-1,(LPWSTR)_pb,len + 1);
		}

		return GetResultW();
	}

	const char* operator()(const wchar_t* pUniStr,unsigned cp = CP_ACP) throw()
	{
		if (_pb)
			_aligned_free(_pb);
		_pb = nullptr;

		if (pUniStr == nullptr)
			return nullptr;

		int len = WideCharToMultiByte(cp,0,pUniStr,-1,nullptr,0,nullptr,nullptr);
		if (len > 0)
		{
			_pb = _aligned_malloc(len + 1,4);
			WideCharToMultiByte(cp,0,pUniStr,-1,(LPSTR)_pb,len + 1,nullptr,nullptr);
		}

		return GetResultA();
	}

public:
	const char* GetResultA() const throw()
	{
		return (const char*)_pb;
	}

	const wchar_t* GetResultW() const throw()
	{
		return (const wchar_t*)_pb;
	}
};

#endif //__AUTO_STR_CONV_H