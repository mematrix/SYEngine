#ifndef __MEMORY_BUFFER_H
#define __MEMORY_BUFFER_H

#include <stdio.h>

class MemoryBuffer
{
	void* _ptr;
	unsigned _alloc_size;

public:
	explicit MemoryBuffer(unsigned size) throw() : _ptr(NULL), _alloc_size(0)
	{ Alloc(size, true); }
	MemoryBuffer() throw() : _ptr(NULL), _alloc_size(0) {}
	~MemoryBuffer() throw() { Free(); }

	bool Alloc(unsigned size, bool zero_mem = false) throw();
	bool Realloc(unsigned new_size, bool zero_mem = false) throw();
	void Free() throw();

	template<typename T>
	inline T Get() const throw() { return (T)_ptr; }
	inline unsigned Length() const throw() { return _alloc_size; }
};

#endif //__MEMORY_BUFFER_H