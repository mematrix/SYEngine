#ifndef __AV_UTILS_BUFFER_H
#define __AV_UTILS_BUFFER_H

#include <memory.h>
#include <stdlib.h>

//帮助类，处理内存的自动申请释放功能。
namespace AVUtils {

class Buffer
{
	struct BufferCore
	{
		void* Pointer;
		unsigned InternalSize;
		unsigned TrueSize;
		unsigned UsedSize; //相对TrueSize
	};
	BufferCore _core;

public:
	Buffer() throw() { memset(&_core,0,sizeof(_core)); }
	explicit Buffer(unsigned size) throw() { memset(&_core,0,sizeof(_core)); Alloc(size,true); }
	virtual ~Buffer() throw() { Free(); }

public:
	bool Alloc(unsigned init_size,bool zero_mem = false) throw();
	bool AddAlloc(unsigned add_size) throw() { return Alloc(Size() + add_size); }
	bool Realloc(unsigned new_size) throw();
	void Free() throw();

	inline void ResetSize() throw() { _core.TrueSize = 0; }
	inline unsigned Size() const throw() { return _core.TrueSize; }
	inline unsigned InternalSize() const throw() { return _core.InternalSize; }

	inline unsigned UsedSize() const throw() { return _core.UsedSize; }
	inline void ResetUsedSize() throw() { _core.UsedSize = 0; }
	inline void AddUsedSize(unsigned toAdd) throw() { _core.UsedSize += toAdd; }
	inline void DecUsedSize(unsigned toDec) throw() { _core.UsedSize -= toDec; }
	inline bool IsFullUsed() const throw() { return _core.UsedSize >= _core.TrueSize; }

	inline void ZeroAll() throw() { memset(_core.Pointer,0,_core.InternalSize); }

	template<typename T>
	inline T* Get() const throw() { return (T*)_core.Pointer; }
};

} //AVUtils

#endif //__AV_UTILS_BUFFER_H