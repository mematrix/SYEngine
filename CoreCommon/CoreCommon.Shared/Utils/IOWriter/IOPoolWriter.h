#ifndef __IO_POOL_WRITER_H
#define __IO_POOL_WRITER_H

#include <stdlib.h>
#include <memory.h>
#include "MediaAVIO.h"

class IOPoolWriter : public IAVMediaIO
{
public:
	IOPoolWriter(IAVMediaIO* pDelegateIO,unsigned BufferSize) throw() : _buffer(nullptr), _offset(0)
	{ _io = pDelegateIO; _buf_size = BufferSize; }
	virtual ~IOPoolWriter() throw()
	{ if (_buffer) free(_buffer); }

public:
	unsigned Read(void* pb,unsigned size) { return 0; }
	unsigned Write(void* pb,unsigned size);
	bool Seek(long long offset,int whence);
	long long Tell() { return _io->Tell() + _offset; }
	long long GetSize() { return _io->GetSize() + _offset; }
	bool GetPlatformSpec(void** pp,int* spec_code) { return _io->GetPlatformSpec(pp,spec_code); }

private:
	bool BufferStartup();
	void WriteSmall(unsigned char* dst,unsigned char* src,unsigned size);

private:
	IAVMediaIO* _io;
	unsigned _buf_size;

	unsigned char* _buffer;
	unsigned _offset;
};

#endif //__IO_POOL_WRITER_H