#ifndef _MATROSKA_DATA_SOURCE_H
#define _MATROSKA_DATA_SOURCE_H

#define MKV_DATA_SOURCE_READ_ERROR -1
#define MKV_DATA_SOURCE_SEEK_ERROR 0

#include <stdio.h>

namespace MKV {
namespace DataSource {

struct IDataSource
{
	virtual int Read(void* buffer,unsigned size = 1) = 0;
	virtual int Seek(long long offset,int whence = SEEK_SET) = 0;

	virtual long long Tell() = 0;
	virtual long long Size() = 0;
protected:
	virtual ~IDataSource() {}
};

inline long long SizeToAbsolutePosition(long long size,IDataSource* dataSource)
{
	return dataSource->Tell() + size;
}
inline bool IsCurrentPosition(long long pos,IDataSource* dataSource)
{
	return dataSource->Tell() >= pos;
}
inline void ResetInSkipOffsetError(long long endPos,IDataSource* dataSource)
{
	if (dataSource->Tell() > endPos)
		dataSource->Seek(endPos);
}

}} //MKV::DataSource.

#endif //_MATROSKA_DATA_SOURCE_H