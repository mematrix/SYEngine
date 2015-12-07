#ifdef _WIN32

#include "ComDynamicMemoryBuffer.h"

bool ComDynamicMemoryBuffer::Alloc(unsigned init_size)
{
	if (init_size == 0)
		return false;
	if (_pMemory != nullptr)
		return false;

	_dwMemorySize = init_size / 4 * 4 + 8;
	_pMemory = CoTaskMemAlloc(_dwMemorySize);
	if (_pMemory == nullptr)
		return false;

	RtlZeroMemory(_pMemory,_dwMemorySize);

	_dwTotalSize = init_size;
	_dwDataSize = 0;
	return true;
}

void ComDynamicMemoryBuffer::Free()
{
	if (_pMemory)
		CoTaskMemFree(_pMemory);

	_pMemory = nullptr;
	_dwMemorySize = 0;

	_dwTotalSize = _dwDataSize = 0;
}

void* ComDynamicMemoryBuffer::GetStartPointer()
{
	return _pMemory;
}

void* ComDynamicMemoryBuffer::GetEndPointer()
{
	return (unsigned char*)_pMemory + _dwDataSize;
}

unsigned ComDynamicMemoryBuffer::GetTotalSize()
{
	return _dwTotalSize;
}

unsigned ComDynamicMemoryBuffer::GetDataSize()
{
	return _dwDataSize;
}

void ComDynamicMemoryBuffer::SetMaxSize(unsigned size)
{
	_dwMaxMemorySize = size;
}

bool ComDynamicMemoryBuffer::Append(void* data,unsigned size)
{
	unsigned new_size = _dwDataSize + size;
	if (new_size > _dwTotalSize)
	{
		if (_dwMaxMemorySize != 0)
		{
			if (new_size > _dwMaxMemorySize)
				return false;
		}

		if (!InternalFlushMemory(new_size))
			return false;
	}

	RtlCopyMemory(GetEndPointer(),data,size);
	_dwDataSize += size;

	return true;
}

void ComDynamicMemoryBuffer::Clear()
{
	if (_pMemory)
		RtlZeroMemory(_pMemory,_dwMemorySize);

	_dwDataSize = 0;
}

bool ComDynamicMemoryBuffer::InternalFlushMemory(unsigned new_size)
{
	unsigned size = new_size / 4 * 4 + 8;
	void* temp = CoTaskMemRealloc(_pMemory,size);
	if (temp == nullptr)
		return false;

	RtlZeroMemory((unsigned char*)temp + _dwMemorySize,size - _dwMemorySize);

	_pMemory = temp;
	_dwMemorySize = size;

	_dwTotalSize = new_size;

	return true;
}

#endif