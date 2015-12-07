#include "MemoryStream.h"
#include <stdio.h>
#include <malloc.h>
#include <memory.h>

unsigned MemoryStream::Read(void* buf, unsigned size, bool non_move_offset)
{
	if (buf == NULL || size == 0)
		return 0;
	if (_bound_size == 0)
		return 0;

	unsigned allow_size = _bound_size - _offset;
	if (allow_size >= size)
		allow_size = size;
	if (allow_size == 0)
		return 0;

	unsigned char* ptr = _buffer.Get<unsigned char*>() + _offset;
	if (allow_size > 8)
		memcpy(buf, ptr, allow_size);
	else
		memcpy_8bytes(buf, ptr, allow_size);
	if (!non_move_offset)
		_offset += allow_size;
	return allow_size;
}

unsigned MemoryStream::Write(const void* buf, unsigned size, bool non_move_offset)
{
	if (buf == NULL || size == 0)
		return 0;

	unsigned allow_size = size;
	bool alloc_ret = false;
	if (_bound_size == 0)
		alloc_ret = _buffer.Alloc(allow_size, true);
	else
		alloc_ret = _buffer.Realloc(_offset + allow_size, true);
	if (!alloc_ret)
		return 0;

	unsigned char* ptr = _buffer.Get<unsigned char*>() + _offset;
	unsigned off = _offset + allow_size;
	if (allow_size > 8)
		memcpy(ptr, buf, allow_size);
	else
		memcpy_8bytes(ptr, buf, allow_size);
	if (!non_move_offset)
		_offset = off;

	if (off > _bound_size)
		_bound_size = off;
	return allow_size;
}

bool MemoryStream::SetOffset(unsigned off)
{
	if (off > _bound_size) {
		if (!SetLength(off))
			return false;
	}
	_offset = off;
	return true;
}

bool MemoryStream::SetLength(unsigned new_length)
{
	if (new_length <= _bound_size) {
		SetOffset(new_length);
		_bound_size = new_length;
		return true;
	}
	if (!_buffer.Realloc(new_length, true))
		return false;
	_bound_size = new_length;
	return true;
}

void MemoryStream::memcpy_8bytes(void* dst, const void* src, unsigned length)
{
	unsigned char* d = (unsigned char*)dst;
	const unsigned char* s = (const unsigned char*)src;
	if (length == 1) {
		*d = *s;
		return;
	}
	for (unsigned i = 0; i < length; i++)
		d[i] = s[i];
}