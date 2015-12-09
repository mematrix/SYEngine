#ifndef _MATROSKA_LIST_H
#define _MATROSKA_LIST_H

template<class T,unsigned Count = 2>
struct MatroskaList
{
	explicit MatroskaList(unsigned InitCount) throw() { InitList(InitCount); }
	MatroskaList() throw() { InitList(Count); }
	~MatroskaList() throw() { FreeList(); }

	bool AddItem(T* item) throw()
	{
		if (_internal.Alloc == nullptr)
			return false;

		if (_internal.TrueSize == _internal.AllocSize)
		{
			if (!AllocList(_count + 1))
				return false;
		}

		memcpy(_internal.Alloc + _count,item,_tsize);
		_internal.TrueSize += _tsize;

		_count++;
		return true;
	}

	void ClearItems() throw()
	{
		memset(_internal.Alloc,0,_internal.AllocSize);
		_internal.TrueSize = 0;

		_count = 0;
	}

	T* GetItem(unsigned index = 0) throw()
	{
		if (index >= _count)
			return nullptr;

		return _internal.Alloc + index;
	}

	inline unsigned GetCount() const throw() { return _count; }
	inline bool IsEmpty() const throw() { return _count == 0; }

private:
	struct ListInternal
	{
		T* Alloc;
		unsigned AllocSize;
		unsigned TrueSize;
	};
	ListInternal _internal;
	unsigned _count;
	unsigned _tsize;

private:
	void InitList(unsigned initCount) throw()
	{
		_count = 0;
		_tsize = sizeof(T);

		memset(&_internal,0,sizeof(_internal));
		AllocList(initCount);
	}

	bool AllocList(unsigned addCount) throw()
	{
		_internal.AllocSize += (addCount * _tsize + _tsize);
		if (_internal.Alloc == nullptr)
			_internal.Alloc = (T*)malloc(_internal.AllocSize),
			memset(_internal.Alloc,0,_internal.AllocSize);
		else
			_internal.Alloc = (T*)realloc(_internal.Alloc,_internal.AllocSize);

		return _internal.Alloc != nullptr;
	}

	void FreeList() throw()
	{
		if (_internal.Alloc != nullptr)
			free(_internal.Alloc);

		memset(&_internal,0,sizeof(_internal));
		_count = 0;
	}
};

#endif //_MATROSKA_LIST_H