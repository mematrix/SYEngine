#ifndef __DYNAMINC_MEMORY_BUFFER_H
#define __DYNAMINC_MEMORY_BUFFER_H

class IDynamicMemoryBuffer
{
public:
	virtual bool Alloc(unsigned init_size) = 0;
	virtual void Free() = 0;

	virtual void* GetStartPointer() = 0;
	virtual void* GetEndPointer() = 0;

	virtual unsigned GetTotalSize() = 0;
	virtual unsigned GetDataSize() = 0;

	virtual void SetMaxSize(unsigned size) = 0;

	virtual bool Append(void* data,unsigned size) = 0;
	virtual void Clear() = 0;
};

#endif //__DYNAMINC_MEMORY_BUFFER_H