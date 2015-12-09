#ifndef __ISOM_STRUCT_TABLE_H_
#define __ISOM_STRUCT_TABLE_H_

#include "GlobalPrehead.h"

namespace Isom {

template<class T, unsigned InitAllocCount = 1>
class StructTable
{
	struct Core
	{
		T* Pointer;
		unsigned AllocCount;
		unsigned UsedCount;
	};
	Core _core;
	unsigned _tsize;

public:
	explicit StructTable(unsigned count) throw() { InitTable(count); }
	StructTable() throw() { InitTable(InitAllocCount); }
	virtual ~StructTable() throw() { FreeTable(); }

public:
	bool Add(const T* item)
	{
		if (_core.Pointer == nullptr)
			return false;

		if (_core.UsedCount == _core.AllocCount)
			if (!AllocTable(_core.UsedCount))
				return false;

		memcpy(GetPointerUseOffset(), item, _tsize);
		_core.UsedCount++;
		return true;
	}

	T* New()
	{
		if (!ExpandSizeAndAddCount(1))
			return nullptr;

		T* result = Get(GetCount() - 1);
		memset(result, 0, _tsize);
		return result;
	}

	void Clear()
	{
		_core.UsedCount = 0;
		memset(_core.Pointer, 0, _core.AllocCount * _tsize);
	}

	T* Get(unsigned index = 0)
	{
		if (index >= _core.UsedCount)
			return nullptr;
		return _core.Pointer + index;
	}

	inline T* operator[](std::size_t pos) throw() { return Get(pos); }

	inline void AddCount(unsigned addCount)
	{ if (_core.UsedCount + addCount <= _core.AllocCount) _core.UsedCount += addCount; }
	inline void SetCount(unsigned count) { if (count <= _core.AllocCount) _core.UsedCount = count; }
	inline unsigned GetCount() throw() { return _core.UsedCount; }
	inline bool IsEmpty() throw() { return _core.UsedCount == 0; }

	inline bool ExpandSize(unsigned addCount) { return AllocTable(addCount); }
	inline bool ExpandSizeAndAddCount(unsigned addCount)
	{ if (!AllocTable(addCount)) return false; SetCount(GetCount() + addCount); return true; }

	inline T* GetPointer() { return _core.Pointer; }
	inline T* GetPointerUseOffset() { return _core.Pointer + _core.UsedCount; }
	
private:
	void InitTable(unsigned initCount) throw()
	{
		_tsize = sizeof(T);
		memset(&_core, 0, sizeof(_core));
		AllocTable(initCount);
	}

	bool AllocTable(unsigned addCount) throw()
	{
		_core.AllocCount += (addCount + 1);
		if (_core.Pointer == nullptr)
			_core.Pointer = (T*)malloc(_core.AllocCount * _tsize),
			memset(_core.Pointer, 0, _core.AllocCount * _tsize);
		else
			_core.Pointer = (T*)realloc(_core.Pointer, _core.AllocCount * _tsize);
		return _core.Pointer != nullptr;
	}

	void FreeTable() throw()
	{
		if (_core.Pointer)
			free(_core.Pointer);
		memset(&_core, 0, sizeof(_core));
	}
};

template<typename T, unsigned InitCount = 1>
class WriteableStructTable
{
	StructTable<T, InitCount>* _internal;

public:
	explicit WriteableStructTable(StructTable<T>* from) throw() : _internal(from) {}

	inline T* New() { return _internal->New(); }
	inline bool Add(T* item) { return _internal->Add(item); }
	inline void Clear() { _internal->Clear(); }

	inline T* Get(unsigned index = 0) { return _internal->Get(index); }

	inline unsigned GetCount() throw()
	{ return _internal->GetCount(); }
	inline bool IsEmpty() throw()
	{ return _internal->IsEmpty(); }

	inline T* GetPointer()
	{ return _internal->GetPointer(); }
};

template<typename T, unsigned InitCount = 1>
class ReadonlyStructTable
{
	StructTable<T, InitCount>* _internal;

public:
	explicit ReadonlyStructTable(StructTable<T>* from) throw() : _internal(from) {}

	inline T* Get(unsigned index = 0) { return _internal->Get(index); }

	inline unsigned GetCount() throw()
	{ return _internal->GetCount(); }
	inline bool IsEmpty() throw()
	{ return _internal->IsEmpty(); }

	inline T* GetPointer()
	{ return _internal->GetPointer(); }
};

}

#endif //__ISOM_STRUCT_TABLE_H_