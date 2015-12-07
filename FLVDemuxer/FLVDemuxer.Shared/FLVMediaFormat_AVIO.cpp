#include "FLVMediaFormat.h"

int FLVMediaFormat::Read(void* pb,int len)
{
	if (_av_io == nullptr)
		return 0;

	return _av_io->Read(pb,len);
}

bool FLVMediaFormat::Seek(long long pos)
{
	if (_av_io == nullptr)
		return false;

	return _av_io->Seek(pos,SEEK_SET);
}

bool FLVMediaFormat::SeekByCurrent(long long pos)
{
	if (_av_io == nullptr)
		return false;

	return _av_io->Seek(pos,SEEK_CUR);
}

long long FLVMediaFormat::GetSize()
{
	if (_av_io == nullptr)
		return 0;

	return _av_io->GetSize();
}