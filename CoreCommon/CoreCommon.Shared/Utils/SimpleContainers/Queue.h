#ifndef __AV_UTIL_QUEUE_H
#define __AV_UTIL_QUEUE_H

#include "List.h"

namespace AVUtils {

template<typename T>
class Queue : public List<T,5>
{
public:
	Queue() : _index(0) {}

	inline bool Enqueue(T* item) throw() { return AddItem(item); }
	bool Dequeue(T** item) throw()
	{
		if (item == nullptr)
			return false;
		if (_index >= _count)
			return false;

		*item = GetItem(_index);
		_index++;
		return true;
	}

	T* Peek() throw()
	{
		if (_index >= _count)
			return nullptr;
		if (_count == 0)
			return nullptr;

		return GetItem(0);
	}
	T* Last() throw()
	{
		if (_index >= _count)
			return nullptr;
		if (_count == 0)
			return nullptr;

		return GetItem(_count - 1);
	}

	void Clear() throw()
	{
		_index = 0;
		ClearItems();
	}

	inline unsigned GetCount() const throw() { return _count - _index; }
	inline bool IsEmpty() const throw() { return (_count - _index) > 0; }

private:
	unsigned _index;
};

} //AVUtils

#endif //__AV_UTIL_QUEUE_H