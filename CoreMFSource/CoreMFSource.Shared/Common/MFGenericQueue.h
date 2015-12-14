#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MF_GENERIC_QUEUE_H
#define __MF_GENERIC_QUEUE_H

#include <memory>

#include "MFCommon.h"
#include "MFComQueue.h"

namespace WMF{

template<class T,BOOL THREAD_SAFE = TRUE>
class ComPtrQueue
{
public:
	ComPtrQueue() throw() : _queue(THREAD_SAFE) {}

public:
	HRESULT Push(T* pItem) throw()
	{
		return _queue.PushInterface(reinterpret_cast<IUnknown*>(pItem));
	}

	HRESULT Pop(T** ppItem) throw()
	{
		return _queue.PopInterface(reinterpret_cast<IUnknown**>(ppItem));
	}

	HRESULT Clear() throw()
	{
		return _queue.Clear();
	}

	HRESULT DeleteFromIndex(DWORD dwIndex) throw()
	{
		return _queue.RemoveByIndex(dwIndex);
	}

	HRESULT GetFromIndex(DWORD dwIndex,T** ppItem) throw()
	{
		return _queue.GetInterface(dwIndex,reinterpret_cast<IUnknown**>(ppItem));
	}

	DWORD GetCount() const throw()
	{
		DWORD dwCount = 0;
		_queue.GetCount(&dwCount);

		return dwCount;
	}

	bool IsEmpty() const throw()
	{
		return GetCount() == 0;
	}

public:
	T* operator[](std::size_t index) throw()
	{
		//Non-AddRef.
		return (T*)_queue[index];
	}

private:
	ComQueue _queue;
};

} //namespace WMF.

#endif //__MF_GENERIC_QUEUE_H