#include "IOPoolReader.h"
#include <assert.h>

IOPoolReader::IOPoolReader(IAVMediaIO* pDelegateIO,unsigned BufferSize) throw()
{
	memset(&_ioBuffer,0,sizeof(_ioBuffer));

	_io = pDelegateIO;
	_ioContext.buffer_size = BufferSize;

	_ioContext.ResetPos();
	_ioContext.size = pDelegateIO->GetSize();
	_ioContext.io_error = false;
}

unsigned IOPoolReader::Read(void* pb,unsigned size)
{
	if (pb == nullptr)
		return 0;

	if (!AllocIoBuffer())
	{
		_ioContext.io_error = true;
		return 0;
	}

	unsigned result = 0;
	if (size > _ioContext.buffer_size)
		result =  InternalRead0(pb,size);
	else
		result = InternalRead1(pb,size,0);

#ifdef _DEBUG
	if (result == 0)
		printf("IOPoolReader Data Ended.\n");
#endif

	return result;
}

bool IOPoolReader::Seek(long long offset,int whence)
{
	if (offset == 0 && whence == SEEK_SET)
	{
		_ioBuffer.Reset();
		_ioContext.ResetPos();
		return _io->Seek(0,SEEK_SET);
	}

	if (_ioBuffer.GetAvailableSize() == 0)
	{
		bool r = _io->Seek(offset,whence);
		_ioContext.SetPos(_io->Tell());
		return r;
	}

	long long true_offset = 0;
	switch (whence)
	{
	case SEEK_SET:
		true_offset = offset;
		break;
	case SEEK_CUR:
		true_offset = _ioContext.Position() + offset;
		break;
	case SEEK_END:
		true_offset = _ioContext.size + offset;
		break;
	default:
		return false;
	}

	if (true_offset == _ioContext.Position())
		return true;

	long long cur_buf_end = _io->Tell();
	long long cur_buf_begin = cur_buf_end - _ioBuffer.size;
	
	//ÔÚ»º³åÇøÓòÄÚ
	if (true_offset >= cur_buf_begin &&
		true_offset <= cur_buf_end) {
		unsigned new_offset = (unsigned)(true_offset - cur_buf_begin);
		if (new_offset != _ioBuffer.pos)
		{
			if (new_offset > _ioBuffer.pos)
				_ioContext.AddPos(new_offset - _ioBuffer.pos);
			else
				_ioContext.SubPos(_ioBuffer.pos - new_offset);

			_ioBuffer.pos = new_offset;
		}
		return true;
	}

	_ioBuffer.Reset();
	bool r = _io->Seek(true_offset,SEEK_SET);
	_ioContext.SetPos(_io->Tell());
	return r;
}

bool IOPoolReader::AllocIoBuffer()
{
	if (_ioBuffer.p != nullptr)
		return true;

	_ioBuffer.p = (unsigned char*)
		malloc(_ioContext.buffer_size / 4 * 4 + 8);
	if (_ioBuffer.p == nullptr)
		return false;

	memset(_ioBuffer.p,0,_ioContext.buffer_size);
	_ioBuffer.Reset();

	return true;
}

void IOPoolReader::FreeIoBuffer()
{
	if (_ioBuffer.p)
		free(_ioBuffer.p);

	memset(&_ioBuffer,0,sizeof(_ioBuffer));
}

bool IOPoolReader::FlushIoBuffer()
{
	_ioBuffer.Reset();

	unsigned read_size = _io->Read(
		_ioBuffer.p,_ioContext.buffer_size);
	if (read_size == 0)
		return false;

	_ioBuffer.size = read_size;
	return true;
}

unsigned IOPoolReader::InternalRead0(void* pb,unsigned size)
{
	unsigned asize = _ioBuffer.GetAvailableSize();
	if (asize == 0)
	{
		unsigned rsize = _io->Read(pb,size);
		//_ioContext.pos += rsize;
		_ioContext.AddPos(rsize);

		return rsize;
	}
	else
	{
		auto p = (unsigned char*)pb;
		_ioBuffer.CopyTo(p,asize);

		unsigned rsize = _io->Read(p + asize,size - asize);
		rsize += asize;
		//_ioContext.pos += rsize;
		_ioContext.AddPos(rsize);

		return rsize;
	}
}

unsigned IOPoolReader::InternalRead1(void* pb,unsigned size,unsigned append_size)
{
	if (_ioBuffer.GetAvailableSize() == 0)
	{
		if (!FlushIoBuffer())
			return 0;
	}

	unsigned asize = _ioBuffer.GetAvailableSize();
	if (asize >= size)
	{
		_ioBuffer.CopyTo(pb,size);
		//_ioContext.pos += size;
		_ioContext.AddPos(size);

		return size + append_size;
	}

	_ioBuffer.CopyTo(pb,asize);
	//_ioContext.pos += asize;
	_ioContext.AddPos(asize);

	auto p = (unsigned char*)pb;
	return InternalRead1(p + asize,size - asize,append_size + asize);
}