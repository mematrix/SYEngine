#ifndef _MATROSKA_DATA_SOURCE_CRT_H
#define _MATROSKA_DATA_SOURCE_CRT_H

#include "MatroskaDataSource.h"
#include <memory>
#include <stdio.h>

namespace MKV {
namespace DataSource {

class CRTDataSource : public IDataSource
{
	FILE* _file;
	long long _size;

public:
	explicit CRTDataSource(const char* filePath) throw()
	{
		_file = fopen(filePath,"r+");
		if (_file)
		{
			fseek(_file,0,SEEK_END);
			fpos_t pos = 0;
			fgetpos(_file,&pos);
			fseek(_file,0,SEEK_SET);

			_size = pos;
		}
	}

	virtual ~CRTDataSource() throw()
	{
		if (_file)
			fclose(_file);
	}

public:
	int Read(void* buffer,unsigned size)
	{
		if (_file == nullptr)
			return MKV_DATA_SOURCE_READ_ERROR;

		auto r = fread(buffer,size,1,_file);
		if (r == 0)
			return MKV_DATA_SOURCE_READ_ERROR;

		return (int)r;
	}

	int Seek(long long offset,int whence)
	{
		if (_file == nullptr)
			return MKV_DATA_SOURCE_SEEK_ERROR;
		if (whence != SEEK_SET)
			return MKV_DATA_SOURCE_SEEK_ERROR;

		fpos_t pos = offset;
		if (fsetpos(_file,&pos) != 0)
			return MKV_DATA_SOURCE_SEEK_ERROR;

		return 1;
	}

	long long Tell()
	{
		if (_file == nullptr)
			return 0;

		fpos_t pos = 0;
		fgetpos(_file,&pos);

		return pos;
	}

	long long Size()
	{
		if (_file == nullptr)
			return 0;

		return _size;
	}
};

std::shared_ptr<CRTDataSource> CreateCRTMKVDataSource(const char* filename)
{
	auto p = std::make_shared<CRTDataSource>(filename);
	return p;
}

}} //MKV::DataSource.

#endif //_MATROSKA_DATA_SOURCE_CRT_H