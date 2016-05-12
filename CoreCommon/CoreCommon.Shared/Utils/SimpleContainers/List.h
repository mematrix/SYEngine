#ifndef __AV_UTILS_LIST_H
#define __AV_UTILS_LIST_H

#include "Buffer.h"
#include <cstddef>

//帮助类，支持泛型结构体线性保存。

namespace AVUtils {

template<typename T,unsigned InitCount = 2>
class List
{
protected:
	Buffer _buffer; //List的内部线性存储区
	unsigned _count; //当前List项目总数
	unsigned _tsize; //模板T结构的大小

public:
	explicit List(unsigned userInitCount) throw() { InitList(userInitCount); }
	List() throw() { InitList(InitCount); }
	virtual ~List() throw() { FreeList(); }

	explicit List(const List<T>& from) throw() { CopyFrom(from); }

public:
	bool AddItem(T* item) throw()
	{
		if (_buffer.Get<void>() == nullptr)
			return false;

		if (_buffer.IsFullUsed())
		{
			if (!AllocList(_count + 1))
				return false;
		}

		memcpy(_buffer.Get<T>() + _count,item,_tsize);
		_buffer.AddUsedSize(_tsize);

		_count++;
		return true;
	}

	void EraseItems(unsigned eraseCount) throw()
	{
		if (eraseCount >= _count)
		{
			ClearItems();
			return;
		}

		_count -= eraseCount;

		_buffer.ResetUsedSize();
		_buffer.AddUsedSize(_count * _tsize);
	}

	void ClearItems() throw()
	{
		if (_buffer.Get<void>())
		{
			_buffer.ZeroAll();
			_buffer.ResetUsedSize();
		}

		_count = 0;
	}

	T* GetItem(unsigned index) throw()
	{
		if (index >= _count)
			return nullptr;

		return _buffer.Get<T>() + index;
	}

	inline unsigned GetCount() const throw() { return _count; }
	inline bool IsEmpty() const throw() { return _count > 0; }

	inline const Buffer* InternalBuffer() const throw() { return &_buffer; }
	inline T* operator[](std::size_t pos) throw() { return GetItem(pos); }

public:
	bool CopyFrom(const List<T>& other) throw()
	{
		_count = 0;
		_tsize = sizeof(T);
		_buffer.Free();

		if (!AllocList(other.GetCount()))
			return false;

		memcpy(_buffer.Get<T>(),other.InternalBuffer()->Get(),other.GetCount() * _tsize);
		_buffer.AddUsedSize(other.GetCount() * _tsize);
		_count = other.GetCount();

		return true;
	}

protected:
	void InitList(unsigned initCount) throw()
	{
		_count = 0;
		_tsize = sizeof(T);

		AllocList(initCount);
	}

	bool AllocList(unsigned addCount) throw()
	{
		unsigned size = _buffer.Size() + (addCount * _tsize) + _tsize;
		if (_buffer.Get<void>() == nullptr)
			_buffer.Alloc(size,true);
		else
			_buffer.Realloc(size);

		return _buffer.Get<void>() != nullptr;
	}

	void FreeList() throw()
	{
		_buffer.Free();
		_count = 0;
	}
};

} //AVUtils

#endif //__AV_UTILS_LIST_H