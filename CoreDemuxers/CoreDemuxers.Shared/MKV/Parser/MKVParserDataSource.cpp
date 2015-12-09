#include "MKVParser.h"

using namespace MKVParser;

int MKVFileParser::Read(void* buffer,unsigned size)
{
	if (size >= INT_MAX)
		return MKV_DATA_SOURCE_READ_ERROR;

	unsigned result = _io->Read(buffer,size);
	if (result == 0)
		return MKV_DATA_SOURCE_READ_ERROR;

	return (int)result;
}

int MKVFileParser::Seek(long long offset,int whence)
{
	return _io->Seek(offset,whence) ? 1:MKV_DATA_SOURCE_SEEK_ERROR;
}

long long MKVFileParser::Tell()
{
	return _io->Tell();
}

long long MKVFileParser::Size()
{
	return _io->GetSize();
}