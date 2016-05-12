#include "MediaMemory.h"
#include <memory.h>
#include <stdlib.h>

#define ALLOC_ALIGNED(x) ((((x) >> 2) << 2) + 8) //4bytes.

void* AllocMediaMemory(unsigned size)
{
	return malloc(ALLOC_ALIGNED(size));
}

void* ReallocMediaMemory(void* p,unsigned size)
{
	return realloc(p,ALLOC_ALIGNED(size));
}

void FreeMediaMemory(void* p)
{
	if (p)
		free(p);
}

bool InitMediaBuffer(AVMediaBuffer* buffer,unsigned size)
{
	buffer->buf = (unsigned char*)AllocMediaMemory(size);
	if (buffer->buf == nullptr)
		return false;

	buffer->size = size;
	return true;
}

bool AdjustMediaBufferSize(AVMediaBuffer* buffer,unsigned new_size)
{
	if (buffer == nullptr)
		return false;
	if (buffer->buf == nullptr)
		return false;

	if (new_size == 0)
		return true;

	if (new_size <= buffer->size)
	{
		buffer->size = new_size;
		return true;
	}

	buffer->buf = (unsigned char*)ReallocMediaMemory(buffer->buf,new_size);
	if (buffer->buf == nullptr)
		return false;

	buffer->size = new_size;

	return true;
}

bool FreeMediaBuffer(AVMediaBuffer* buffer)
{
	if (buffer == nullptr)
		return false;
	if (buffer->buf == nullptr)
		return false;

	FreeMediaMemory(buffer->buf);
	return true;
}

void CopyMediaMemory(AVMediaBuffer* buffer,void* copy_to,unsigned copy_to_size)
{
	if (buffer == nullptr)
		return;
	if (buffer->buf == nullptr)
		return;

	if (copy_to == nullptr ||
		copy_to_size == 0)
		return;

	memcpy(copy_to,buffer->buf,
		copy_to_size > buffer->size ? buffer->size:copy_to_size);
}