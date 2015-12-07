#include "CrtDynamicMemoryBuffer.h"

bool CrtDynamicMemoryBuffer::Alloc(unsigned init_size)
{
	if (init_size == 0)
		return false;
	if (_ptr != nullptr)
		return false;
	
	_ptr_size = ((init_size >> 2) << 2) + 8;
	_ptr = malloc(_ptr_size);
	if (_ptr == nullptr)
		return false;

	memset(_ptr,0,_ptr_size);

	_total_size = init_size;
	_data_size = 0;
	return true;
}

void CrtDynamicMemoryBuffer::Free()
{
	if (_ptr)
		free(_ptr);

	_ptr = nullptr;
	_ptr_size = 0;

	_total_size = _data_size = 0;
}

void* CrtDynamicMemoryBuffer::GetStartPointer()
{
	return _ptr;
}

void* CrtDynamicMemoryBuffer::GetEndPointer()
{
	return (unsigned char*)_ptr + _data_size;
}

unsigned CrtDynamicMemoryBuffer::GetTotalSize()
{
	return _total_size;
}

unsigned CrtDynamicMemoryBuffer::GetDataSize()
{
	return _data_size;
}

void CrtDynamicMemoryBuffer::SetMaxSize(unsigned size)
{
	_max_size = size;
}

bool CrtDynamicMemoryBuffer::Append(void* data,unsigned size)
{
	unsigned new_size = _data_size + size;
	if (new_size > _total_size)
	{
		if (_max_size != 0)
		{
			if (new_size > _max_size)
				return false;
		}

		if (!InternalFlushMemory(new_size))
			return false;
	}

	memcpy(GetEndPointer(),data,size);
	_data_size += size;

	return true;
}

void CrtDynamicMemoryBuffer::Clear()
{
	if (_ptr)
		memset(_ptr,0,_ptr_size);

	_data_size = 0;
}

bool CrtDynamicMemoryBuffer::InternalFlushMemory(unsigned new_size)
{
	unsigned size = ((new_size >> 2) << 2) + 8;
	void* temp = realloc(_ptr,size);
	if (temp == nullptr)
		return false;

	memset((unsigned char*)temp + _ptr_size,0,size - _ptr_size);

	_ptr = temp;
	_ptr_size = size;

	_total_size = new_size;

	return true;
}