#ifndef __MEMORY_STREAM_H
#define __MEMORY_STREAM_H

#include "MemoryBuffer.h"

class MemoryStream
{
	MemoryBuffer _buffer;
	unsigned _offset;
	unsigned _bound_size;

public:
	MemoryStream() throw() : _offset(0), _bound_size(0) {}

	unsigned Read(void* buf, unsigned size, bool non_move_offset = false);
	unsigned Write(const void* buf, unsigned size, bool non_move_offset = false);

	bool SetOffset(unsigned off);
	unsigned GetOffset() { return _offset; }

	bool SetLength(unsigned new_length);
	unsigned GetLength() { return _bound_size; }
	unsigned GetPendingLength() { return GetLength() - GetOffset(); }

	bool IsEof() { return _offset == _bound_size; }

	inline void* Pointer() { return _buffer.Get<char*>() + _offset; }
	inline MemoryBuffer* Buffer() throw() { return &_buffer; }

private:
	static void memcpy_8bytes(void* dst, const void* src, unsigned length);
};

#endif //__MEMORY_STREAM_H