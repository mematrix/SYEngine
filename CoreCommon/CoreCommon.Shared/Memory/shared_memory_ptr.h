#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __SHARED_MEMORY_PTR_H
#define __SHARED_MEMORY_PTR_H

#include <cstdlib>
#include <memory>

#ifndef _MSC_VER
#define _InterlockedIncrement(x) __sync_fetch_and_add(x,1)
#define _InterlockedDecrement(x) __sync_sub_and_fetch(x,1)
#endif

template<class T>
class shared_memory_ptr
{
	T* m_ptr;
	unsigned m_size;
	long* m_p_ref_count;

public:
	explicit shared_memory_ptr(unsigned size) throw()
	{
		m_ptr = (T*)malloc(size);
		m_size = size;

		memset(m_ptr,0,size);

		m_p_ref_count = (long*)malloc(sizeof(long));
		*m_p_ref_count = 1;
	}

	shared_memory_ptr(const shared_memory_ptr& src) throw()
	{
		m_ptr = src.m_ptr;
		m_size = src.m_size;
		m_p_ref_count = src.m_p_ref_count;

		AddRefCount();
	}

	~shared_memory_ptr() throw()
	{
		if (ReleaseRefCount() == 0)
		{
			free(m_p_ref_count);

			if (m_ptr)
				free(m_ptr);
		}
	}

public:
	shared_memory_ptr& operator=(const shared_memory_ptr& rhs) throw()
	{
		if (this != &rhs)
		{
			m_ptr = rhs.m_ptr;
			m_size = rhs.m_size;
			m_p_ref_count = rhs.m_p_ref_count;

			AddRefCount();
		}

		return *this;
	}

	void* operator new(std::size_t) = delete;
	void* operator new[](std::size_t) = delete;

public:
	T* ptr() const throw()
	{
		return m_ptr;
	}

	unsigned size() const throw()
	{
		return m_size;
	}

	long use_count() const throw()
	{
		return *m_p_ref_count;
	}

public:
	bool realloc(size_t new_size) throw()
	{
		if (m_ptr == nullptr)
			return false;
		if (new_size <= m_size)
			return false;

		m_ptr = (T*)::realloc(m_ptr,new_size);
		if (m_ptr == nullptr)
			return false;

		memset(((unsigned char*)m_ptr) + m_size,0,new_size - m_size);
		return true;
	}

private: //Thread Safety.
	long AddRefCount()
	{
		return _InterlockedIncrement(m_p_ref_count);
	}
    long ReleaseRefCount()
	{
		return _InterlockedDecrement(m_p_ref_count);
	}
};

#endif //__SHARED_MEMORY_PTR_H