#include "MatroskaEbmlCore.h"
#include <assert.h>

using namespace MKV::EBML::Core;

int MKV::EBML::Core::ParseByteZeroBitsNum(unsigned char value)
{
	if (value == 0)
		return 8;

	static int magic[] = {128,64,32,16,8,4,2,1};

	for (int i = 0;i < 8;i++)
		if ((value & magic[i]) != 0)
			return i;

	return -1;
}

int MKV::EBML::Core::StripZeroBitInNum(unsigned char value,int num)
{
	if (value == 0 || num >= 8)
		return 0;

	static int magic[] = {127,63,31,15,7,3,1,0};
	return (value & magic[num]);
}

int MKV::EBML::Core::GetVIntLength(unsigned char entry)
{
	if (entry == 0)
		return 8;

	return ParseByteZeroBitsNum(entry) + 1;
}

unsigned MKV::EBML::Core::GetEbmlId(DataSource::IDataSource* dataSource)
{
	unsigned char b = 0;
	if (dataSource->Read(&b) == MKV_DATA_SOURCE_READ_ERROR)
		return INVALID_EBML_ID;

	int size = GetVIntLength(b);
#ifdef _DEBUG
	assert(size <= 4);
#endif

	if (size > 4)
		return INVALID_EBML_ID;

	unsigned result = b;
	if (size > 1)
	{
		unsigned char buf[3];
		if (dataSource->Read(buf,size - 1) == MKV_DATA_SOURCE_READ_ERROR)
			return INVALID_EBML_ID;

		for (int i = 1;i < size;i++)
		{
			result <<= 8;
			result |= buf[i - 1];
		}
	}

	return result;
}

long long MKV::EBML::Core::GetEbmlSize(DataSource::IDataSource* dataSource)
{
	unsigned char b = 0;
	if (dataSource->Read(&b) == MKV_DATA_SOURCE_READ_ERROR)
		return INVALID_EBML_SIZE;

	int size = GetVIntLength(b);
#ifdef _DEBUG
	assert(size <= 8);
#endif

	long long result = StripZeroBitInNum(b,size - 1);
	if (size > 1)
	{
		unsigned char buf[7];
		if (dataSource->Read(buf,size - 1) == MKV_DATA_SOURCE_READ_ERROR)
			return INVALID_EBML_SIZE;

		for (int i = 1;i < size;i++)
		{
			result <<= 8;
			result |= buf[i - 1];
		}
	}

	return result;
}

unsigned MKV::EBML::Core::GetEbmlBytes(unsigned long long num)
{
	int bytes = 1;
	while ((num + 1) >> bytes *7)
		bytes++;
	return bytes;
}

int MKV::EBML::Core::SearchEbmlClusterStartCodeOffset(unsigned char* buf,unsigned size)
{
	for (unsigned i = 0;i < (size - 4);i++)
		if (buf[i] == 0x1F &&
			buf[i + 1] == 0x43 &&
			buf[i + 2] == 0xB6 &&
			buf[i + 3] == 0x75)
			return (int)i;
	return -1;
}