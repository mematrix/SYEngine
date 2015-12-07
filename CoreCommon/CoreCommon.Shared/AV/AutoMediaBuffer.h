#ifndef __AUTO_MEDIA_BUFFER_H
#define __AUTO_MEDIA_BUFFER_H

#include "MediaMemory.h"
#include <memory.h>

class CAutoMediaBuffer final
{
	AVMediaBuffer _buffer;

public:
	explicit CAutoMediaBuffer(unsigned size) throw()
	{
		memset(&_buffer,0,sizeof(_buffer));
		InitMediaBuffer(&_buffer,size);
	}

	explicit CAutoMediaBuffer(AVMediaBuffer& other) throw()
	{
		_buffer = other;
	}

	explicit CAutoMediaBuffer(const AVMediaBuffer* other) throw()
	{
		if (other == nullptr)
			return;

		memset(&_buffer,0,sizeof(_buffer));
		InitMediaBuffer(&_buffer,other->size);

		memcpy(_buffer.buf,other->buf,other->size);
	}

	~CAutoMediaBuffer() throw()
	{
		if (_buffer.buf)
			FreeMediaBuffer(&_buffer);
	}

public:
	AVMediaBuffer* Get() throw()
	{
		return &_buffer;
	}

	const AVMediaBuffer* Get() const throw()
	{
		return &_buffer;
	}
};

#endif //__AUTO_MEDIA_MEMORY_H