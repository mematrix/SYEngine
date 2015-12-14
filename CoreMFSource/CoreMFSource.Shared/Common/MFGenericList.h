#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MF_GENERIC_LIST_H
#define __MF_GENERIC_LIST_H

#include <memory>

#include "MFCommon.h"
#include "MFComList.h"

namespace WMF{

template<class T,BOOL THREAD_SAFE = TRUE>
class ComPtrList
{
public:
	ComPtrList() throw() : _list(THREAD_SAFE) {}

public:
	HRESULT Add(T* pUnk) throw()
	{
		return InsertBack(pUnk);
	}

	HRESULT InsertFront(T* pUnk) throw()
	{
		return _list.InsertFront(reinterpret_cast<IUnknown*>(pUnk));
	}
	HRESULT InsertBack(T* pUnk) throw()
	{
		return _list.InsertBack(reinterpret_cast<IUnknown*>(pUnk));
	}
	HRESULT InsertAt(DWORD dwIndex,T* pUnk) throw()
	{
		return _list.InsertAt(dwIndex,reinterpret_cast<IUnknown*>(pUnk));
	}

	HRESULT RemoveFront(T** ppUnk) throw()
	{
		return _list.RemoveFront(reinterpret_cast<IUnknown**>(ppUnk));
	}
	HRESULT RemoveBack(T** ppUnk) throw()
	{
		return _list.RemoveBack(reinterpret_cast<IUnknown**>(ppUnk));
	}
	HRESULT RemoveAt(DWORD dwIndex,T** ppUnk) throw()
	{
		return _list.RemoveAt(dwIndex,reinterpret_cast<IUnknown**>(ppUnk));
	}

	HRESULT GetFront(T** ppUnk) const throw()
	{
		return _list.GetFront(reinterpret_cast<IUnknown**>(ppUnk));
	}
	HRESULT GetBack(T** ppUnk) const throw()
	{
		return _list.GetBack(reinterpret_cast<IUnknown**>(ppUnk));
	}
	HRESULT GetAt(DWORD dwIndex,T** ppUnk) const throw()
	{
		return _list.GetAt(dwIndex,reinterpret_cast<IUnknown**>(ppUnk));
	}

	HRESULT GetCount(PDWORD pdwCount) const throw()
	{
		return _list.GetCount(pdwCount);
	}
	HRESULT DeleteAt(DWORD dwIndex) throw()
	{
		return _list.DeleteAt(dwIndex);
	}
	HRESULT Clear() throw()
	{
		return _list.Clear();
	}

	unsigned Count() const throw()
	{
		DWORD dwCount = 0;
		GetCount(&dwCount);

		return dwCount;
	}

	bool IsEmpty() const throw()
	{
		DWORD dwCount = 0;
		GetCount(&dwCount);

		return dwCount == 0;
	}

public:
	T* operator[](std::size_t index) throw()
	{
		//Non-AddRef.
		return (T*)_list[index];
	}

private:
	ComList _list;
};

} //namespace WMF.

#endif //__MF_GENERIC_LIST_H