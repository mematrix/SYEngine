#ifndef _MATROSKA_EBML_H
#define _MATROSKA_EBML_H

#include "MatroskaSpec.h"
#include "MatroskaMisc.h"
#include "MatroskaEbmlCore.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

#define INVALID_EBML_UINT32 0xFFFFFFFFUL
#define INVALID_EBML_UINT64 0xFFFFFFFFFFFFFFFFULL

#define INVALID_EBML_FLOAT -1.0f
#define INVALID_EBML_DOUBLE -1.0

namespace MKV {
namespace EBML {

struct EbmlHead
{
	void Parse(MKV::DataSource::IDataSource* dataSource) throw()
	{
		_id = MKV::EBML::Core::GetEbmlId(dataSource);
		if (_id != INVALID_EBML_ID)
			_size = MKV::EBML::Core::GetEbmlSize(dataSource);
		else
			_size = INVALID_EBML_SIZE;

		_dataSource = dataSource;
	}

	void ParseCanIfEnd(MKV::DataSource::IDataSource* dataSource) throw()
	{
		Parse(dataSource);

		if (_size != INVALID_EBML_SIZE)
			_io_end_pos = dataSource->Tell() + _size;
	}

	inline bool IsValid() const throw()
	{
		if (_id != INVALID_EBML_ID &&
			_size != INVALID_EBML_SIZE)
			return true;

		return false;
	}

	inline unsigned Id() const throw() { return _id; }
	inline long long Size() const throw() { return _size; }

	inline void ResetId() throw() { _id = INVALID_EBML_ID; }

	inline bool MatchId(unsigned id) const throw() { return _id == id; }
	inline bool MatchSize(long long size) const throw() { return _size == size; }

	inline bool Match(unsigned id,long long size) const throw()
	{
		return (MatchId(id) && MatchSize(size));
	}

	bool MatchIdList(const unsigned* id_list,unsigned count) throw()
	{
		for (unsigned i = 0;i < count;i++)
			if (_id == id_list[i])
				return true;

		return false;
	}

	bool IsLevel1Elements() throw()
	{
		if (_id == MKV_ID_L1_SEEKHEAD ||
			_id == MKV_ID_L1_INFO ||
			_id == MKV_ID_L1_TRACKS ||
			_id == MKV_ID_L1_CHAPTERS ||
			_id == MKV_ID_L1_CLUSTER ||
			_id == MKV_ID_L1_CUES ||
			_id == MKV_ID_L1_ATTACHMENTS ||
			_id == MKV_ID_L1_TAGS)
			return true;

		return false;
	}

	inline bool IsCluster() const throw() { return _id == MKV_ID_L1_CLUSTER; }

	bool Read(void* p,unsigned size)
	{
		int result = _dataSource->Read(p,size);

		if (result == MKV_DATA_SOURCE_READ_ERROR ||
			result != size)
			return false;

		return true;
	}
	bool Read(void* p)
	{
		return Read(p,(unsigned)_size);
	}
	unsigned char Read()
	{
		unsigned char temp = 0;
		Read(&temp,1);

		return temp;
	}

	inline void Skip() const throw()
	{
		if (_size > 0)
			_dataSource->Seek(_dataSource->Tell() + _size); 
	}
	void Skip(unsigned size) const throw()
	{
		if (size > _size)
			return;
		_dataSource->Seek(_dataSource->Tell() - size);
	}

	bool SkipIfVoid() throw()
	{
		if (_id == EBML_ID_GLOBAL_VOID ||
			_id == EBML_ID_GLOBAL_CRC32) {
			Skip();
			return true;
		}

		return false;
	}

	bool IsEnd() throw()
	{
		if (_dataSource == nullptr)
			return false;
		if (_io_end_pos == 0)
			return false;

		if (_dataSource->Tell() >= _io_end_pos)
			return true;

		return false;
	}
	inline long long GetEndPos() const throw() { return _io_end_pos; }

	inline MKV::DataSource::IDataSource* GetDataSource() const throw() { return _dataSource; }
	inline void SetDataSource(MKV::DataSource::IDataSource* dataSource) throw() { _dataSource = dataSource; }

	inline void SetSimpleCore(unsigned id,long long size) throw()
	{
		_id = id;
		_size = size;

		_io_end_pos = 0;
	}

	EbmlHead() : _id(INVALID_EBML_ID), _size(INVALID_EBML_SIZE), _io_end_pos(0), _dataSource(nullptr) {}
	EbmlHead(MKV::DataSource::IDataSource* dataSource) { Parse(dataSource); }

private:
	unsigned _id; //EBML元素ID
	long long _size; //EBML元素大小
	long long _io_end_pos; //在DataSource里面的结束位置（绝对偏移）

	MKV::DataSource::IDataSource* _dataSource;
};

//to Skip Void and CRC32.
bool FastParseEbmlHead(EbmlHead& head,MKV::DataSource::IDataSource* dataSource,bool isEnd = false);
bool FastParseEbmlHeadIfSize(EbmlHead& head,MKV::DataSource::IDataSource* dataSource,bool isEnd = false);
bool FastParseEbmlHeadIfSizeWithNoErr(EbmlHead& head,MKV::DataSource::IDataSource* dataSource,bool isEnd = false);
bool FastParseEbmlHeadIfSizeWithIfSpec(EbmlHead& head,MKV::DataSource::IDataSource* dataSource,bool isEnd = false,unsigned preserve_id = INVALID_EBML_ID);

#define FAST_PARSE_WITH_IO(head) head,head.GetDataSource()
#define FAST_PARSE_WITH_IO_P(head) *head,head->GetDataSource()

enum EbmlDataType
{
	UnknownEbmlType = 0,
	UIntAs1Byte,
	UIntAs2Byte,
	UIntAs3Byte,
	UIntAs4Byte, //32 bits
	UIntAs5Byte,
	UIntAs6Byte,
	UIntAs7Byte,
	UIntAs8Byte, //64 bits
	FloatAs4Byte,
	DoubleAs8Byte
};

unsigned ReadUInt(EbmlHead& head,EbmlDataType type = UIntAs4Byte);
unsigned long long ReadUInt64(EbmlHead& head);
unsigned long long ReadUInt64On567(EbmlHead& head);
float ReadFloat(EbmlHead& head);
double ReadDouble(EbmlHead& head);

unsigned ReadUIntFromPointer(unsigned char* p,unsigned long long* value);
unsigned ReadSIntFromPointer(unsigned char* p,long long* value);

struct EBML_VALUE_DATA
{
	EbmlDataType type;
	union
	{
		unsigned Ui32;
		unsigned long long Ui64;
	};
	float Float;
	double Double;

	bool UIntToBool()
	{
		return Ui64 != 0;
	}
};
bool EbmlReadValue(EbmlHead& head,EBML_VALUE_DATA& value,bool bFloat = false);
bool EbmlReadString(EbmlHead& head,char* copyTo,unsigned copySize = 0);
bool EbmlReadBinary(EbmlHead& head,void* bin,unsigned size = 0);

bool EbmlReadUIntValueSearch(EbmlHead& head,EBML_VALUE_DATA& value,const unsigned* id_list,unsigned count);

#define EbmlReadStringSafe(head,copyTo) \
	EBML::EbmlReadString(head,copyTo,MKV_ARRAY_COUNT(copyTo))
#define EbmlReadBinarySafe(head,bin) \
	EBML::EbmlReadBinary(head,bin,MKV_ARRAY_COUNT(bin))

}} //MKV::EBML.

#endif //_MATROSKA_EBML_H