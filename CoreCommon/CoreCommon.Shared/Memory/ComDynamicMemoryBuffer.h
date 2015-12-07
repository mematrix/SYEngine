#ifdef _WIN32
#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __COM_DYNAMINC_MEMORY_BUFFER_H
#define __COM_DYNAMINC_MEMORY_BUFFER_H

#ifndef _WINDOWS_
#include <Windows.h>
#include <objbase.h>
#endif

#include "DynamicMemoryBuffer.h"

class ComDynamicMemoryBuffer : public IDynamicMemoryBuffer
{
public:
	ComDynamicMemoryBuffer() throw() : 
		_pMemory(nullptr), _dwMemorySize(0), _dwMaxMemorySize(0), _dwTotalSize(0), _dwDataSize(0) {}
	virtual ~ComDynamicMemoryBuffer() { Free(); }

	//copy
	explicit ComDynamicMemoryBuffer(const ComDynamicMemoryBuffer& r) throw() :
		_pMemory(nullptr), _dwMemorySize(0), _dwMaxMemorySize(0), _dwTotalSize(0), _dwDataSize(0) {
		if (!Alloc(r._dwTotalSize))
			return;

		Append(r._pMemory,r._dwTotalSize);
	}

	ComDynamicMemoryBuffer& operator=(const ComDynamicMemoryBuffer& r) throw()
	{
		if (this != &r)
		{
			Free();

			if (Alloc(r._dwTotalSize))
				Append(r._pMemory,r._dwTotalSize);
		}

		return *this;
	}

public:
	bool Alloc(unsigned init_size);
	void Free();

	void* GetStartPointer();
	void* GetEndPointer();

	unsigned GetTotalSize();
	unsigned GetDataSize();

	void SetMaxSize(unsigned size);

	bool Append(void* data,unsigned size);
	void Clear();

private:
	bool InternalFlushMemory(unsigned new_size);

private:
	PVOID _pMemory;
	DWORD _dwMemorySize;
	DWORD _dwMaxMemorySize;

	DWORD _dwTotalSize,_dwDataSize;
};

#endif //__COM_DYNAMINC_MEMORY_BUFFER_H
#endif