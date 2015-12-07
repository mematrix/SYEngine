#include "MemoryBuffer.h"
#include <stdio.h>
#include <malloc.h>
#include <memory.h>

#define MEMORY_ALLOC_ALIGN(x) ((((x) >> 2) << 2) + 8)

bool MemoryBuffer::Alloc(unsigned size, bool zero_mem) throw()
{
	if (size > _alloc_size)
	{
		Free();
		_ptr = malloc(MEMORY_ALLOC_ALIGN(size));
		if (_ptr)
			_alloc_size = size;
	}

	if (_ptr && zero_mem)
		memset(_ptr, 0, size);
	return _ptr != NULL;
}

bool MemoryBuffer::Realloc(unsigned new_size, bool zero_mem) throw()
{
	if (new_size <= _alloc_size)
		return Alloc(new_size, false);
	if (_ptr == NULL)
		return Alloc(new_size, true);

	_ptr = realloc(_ptr, new_size);
	if (_ptr && zero_mem)
		memset((char*)_ptr + _alloc_size, 0, new_size - _alloc_size);
	_alloc_size = new_size;
	return _ptr != NULL;
}

void MemoryBuffer::Free() throw()
{
	if (_ptr)
		free(_ptr);
	_ptr = NULL;
	_alloc_size = 0;
}