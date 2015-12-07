#include "Buffer.h"

#define ALLOC_ALIGNED(x) ((((x) >> 2) << 2) + 8)

using namespace AVUtils;

bool Buffer::Alloc(unsigned init_size,bool zero_mem)
{
	_core.TrueSize = init_size;
	if (init_size > _core.InternalSize)
	{
		Free();

		_core.Pointer = malloc(ALLOC_ALIGNED(init_size));
		_core.InternalSize = _core.TrueSize = init_size;
	}

	if (_core.Pointer && zero_mem)
		memset(_core.Pointer,0,_core.TrueSize);

	return _core.Pointer != nullptr;
}

bool Buffer::Realloc(unsigned new_size)
{
	if (new_size < _core.InternalSize)
		return Alloc(new_size,true);
	if (_core.Pointer == nullptr)
		return Alloc(new_size,true);

	_core.Pointer = realloc(_core.Pointer,ALLOC_ALIGNED(new_size));
	_core.InternalSize = _core.TrueSize = new_size;

	return _core.Pointer != nullptr;
}

void Buffer::Free()
{
	if (_core.Pointer)
		free(_core.Pointer);

	memset(&_core,0,sizeof(_core));
}