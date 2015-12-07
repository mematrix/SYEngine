#ifndef __CRT_DYNAMINC_MEMORY_BUFFER_H
#define __CRT_DYNAMINC_MEMORY_BUFFER_H

#include <malloc.h>
#include <memory.h>
#include "DynamicMemoryBuffer.h"

class CrtDynamicMemoryBuffer : public IDynamicMemoryBuffer
{
public:
	CrtDynamicMemoryBuffer() throw() : _ptr(nullptr), _ptr_size(0), _total_size(0), _data_size(0), _max_size(0) {}
	virtual ~CrtDynamicMemoryBuffer() { Free(); }

public:
	//copy
	explicit CrtDynamicMemoryBuffer(const CrtDynamicMemoryBuffer& r) throw() : 
		_ptr(nullptr), _total_size(0), _data_size(0), _max_size(0) {
		if (!Alloc(r._total_size))
			return;

		Append(r._ptr,r._total_size);
	}

	//move
	CrtDynamicMemoryBuffer(CrtDynamicMemoryBuffer&& r) throw()
	{
		_ptr = r._ptr;
		_ptr_size = r._ptr_size;

		_max_size = r._max_size;

		_data_size = r._data_size;
		_total_size = r._total_size;
	}

	CrtDynamicMemoryBuffer& operator=(const CrtDynamicMemoryBuffer& r) throw()
	{
		if (this != &r)
		{
			Free();

			if (Alloc(r._total_size))
				Append(r._ptr,r._total_size);
		}

		return *this;
	}

	CrtDynamicMemoryBuffer& operator=(CrtDynamicMemoryBuffer&& r) throw()
	{
		if (this != &r)
		{
			_ptr = r._ptr;
			_ptr_size = r._ptr_size;

			_max_size = r._max_size;

			_data_size = r._data_size;
			_total_size = r._total_size;
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
	void* _ptr;
	unsigned _ptr_size;
	unsigned _max_size;

	unsigned _total_size,_data_size;
};

#endif //__CRT_DYNAMINC_MEMORY_BUFFER_H