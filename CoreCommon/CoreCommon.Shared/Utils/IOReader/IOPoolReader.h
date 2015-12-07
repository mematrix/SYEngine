#ifndef __IO_POOL_READER_H
#define __IO_POOL_READER_H

#include <malloc.h>
#include <memory.h>
#include "MediaAVIO.h"

struct IOBuffer
{
	unsigned char* p;
	unsigned pos;
	unsigned size;

	void Reset() throw()
	{
		pos = size = 0;
	}

	void AddPosition(unsigned toAdd) throw()
	{
		pos += toAdd;
	}
	void SubPosition(unsigned toSub) throw()
	{
		pos -= toSub;
	}
	
	unsigned char* GetOffset() throw()
	{
		return p + pos;
	}
	unsigned GetAvailableSize() throw()
	{
		return size - pos;
	}

	bool IsFull() throw()
	{
		return pos == size;
	}
	bool IsEmpty() throw()
	{
		return size == 0;
	}

	bool CopyTo(void* copyTo,unsigned copySize) throw()
	{
		switch (copySize)
		{
		case 1:
			*(unsigned char*)copyTo = *(p + pos);
			break;
		case 2:
			*(unsigned short*)copyTo = *(unsigned short*)(p + pos);
			break;
		case 4:
			*(unsigned*)copyTo = *(unsigned*)(p + pos);
			break;
		case 3:
		case 5:
		case 6:
		case 7:
			for (unsigned i = 0;i < copySize;i++)
				*((unsigned char*)copyTo + i) = *(p + pos + i);
			break;
		case 8:
			*(unsigned long long*)copyTo = *(unsigned long long*)(p + pos);
			break;
		default:
			memcpy(copyTo,p + pos,copySize);
		}
		AddPosition(copySize);
		return true;
	}
};

struct IOContext
{
	long long pos, prev_pos;
	long long size;
	unsigned buffer_size;
	bool io_error;

	inline void OnPositionChanged(char* proc) const throw()
	{
#ifdef _DEBUG
		//printf("IOContext OnPositionChanged(%s):%I64d\n",proc,pos);
#endif
	}

	inline long long Position() const throw()
	{
		return pos;
	}

	void ResetPos() throw()
	{
#ifdef _DEBUG
		prev_pos = 0;
#endif

		pos = 0;
		OnPositionChanged("Reset");
	}
	void SetPos(long long value) throw()
	{
#ifdef _DEBUG
		prev_pos = pos;
#endif

		pos = value;
		OnPositionChanged("SetPos");
	}

	inline void AddPos(long long toAdd) throw()
	{
#ifdef _DEBUG
		prev_pos = pos;
#endif

		pos += toAdd;
		OnPositionChanged("AddPos");
	}
	inline void SubPos(long long toSub) throw()
	{
#ifdef _DEBUG
		prev_pos = pos;
#endif

		pos -= toSub;
		OnPositionChanged("SubPos");
	}
};

class IOPoolReader : public IAVMediaIO
{
public:
	IOPoolReader(IAVMediaIO* pDelegateIO,unsigned BufferSize) throw();
	virtual ~IOPoolReader() throw() { FreeIoBuffer(); }

public:
	unsigned Read(void* pb,unsigned size);
	unsigned Write(void* pb,unsigned size) { return 0; }
	bool Seek(long long offset,int whence);
	long long Tell() { return _ioContext.pos; }
	long long GetSize() { return _ioContext.size; }
	bool IsError() { return _ioContext.io_error; }
	bool GetPlatformSpec(void** pp,int* spec_code) { return _io->GetPlatformSpec(pp,spec_code); }

	void EmptyInternalBuffer() { _ioBuffer.Reset(); }

private:
	bool AllocIoBuffer();
	void FreeIoBuffer();

	bool FlushIoBuffer();

	unsigned InternalRead0(void* pb,unsigned size);
	unsigned InternalRead1(void* pb,unsigned size,unsigned append_size);

private:
	IOContext _ioContext;
	IOBuffer _ioBuffer;

	IAVMediaIO* _io;
};

#endif