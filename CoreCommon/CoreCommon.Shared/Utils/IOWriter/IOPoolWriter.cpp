#include "IOPoolWriter.h"

unsigned IOPoolWriter::Write(void* pb,unsigned size)
{
	if (pb == nullptr || size == 0)
		return 0;

	if (!BufferStartup())
		return 0;

	unsigned result = 0;
	if (size + _offset > _buf_size)
	{
		result = _io->Write(_buffer,_offset);
		_offset = 0;
		result = result + _io->Write(pb,size);
	}else{
		if (size > 8)
			memcpy(_buffer + _offset,pb,size);
		else
			WriteSmall(_buffer + _offset,(unsigned char*)pb,size);
		_offset += size;
		result = size;
	}
	return result;
}

bool IOPoolWriter::Seek(long long offset,int whence)
{
	if (_offset > 0)
		_io->Write(_buffer,_offset);
	_offset = 0;

	return _io->Seek(offset,whence);
}

bool IOPoolWriter::BufferStartup()
{
	if (_buffer != nullptr)
		return true;

	_buffer = (decltype(_buffer))malloc(_buf_size / 4 * 4 + 8);
	if (_buffer == nullptr)
		return false;

	memset(_buffer,0,_buf_size);
	return true;
}

void IOPoolWriter::WriteSmall(unsigned char* dst,unsigned char* src,unsigned size)
{
	switch (size)
	{
	case 1:
		*dst = *src;
		break;
	case 2:
		*(unsigned short*)dst = *(unsigned short*)src;
		break;
	case 4:
		*(unsigned*)dst = *(unsigned*)src;
		break;
	case 8:
		*(unsigned long long*)dst = *(unsigned long long*)src;
		break;
	default:
		for (unsigned i = 0;i < size;i++)
			dst[i] = src[i];
	}
}