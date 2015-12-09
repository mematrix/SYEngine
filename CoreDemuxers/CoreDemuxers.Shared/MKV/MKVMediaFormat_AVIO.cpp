#include "MKVMediaFormat.h"

unsigned MKVMediaFormat::Read(void* buf,unsigned size)
{
	if (_av_io == nullptr)
		return 0;

	return _av_io->Read(buf,size);
}

bool MKVMediaFormat::Seek(long long offset,int whence)
{
	if (_av_io == nullptr)
		return false;

	return _av_io->Seek(offset,whence);
}

long long MKVMediaFormat::Tell()
{
	if (_av_io == nullptr)
		return -1;

	return _av_io->Tell();
}

long long MKVMediaFormat::GetSize()
{
	if (_av_io == nullptr)
		return 0;

	return _av_io->GetSize();
}