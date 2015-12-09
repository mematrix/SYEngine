#include "MatroskaEbml.h"

using namespace MKV::EBML;

bool MKV::EBML::FastParseEbmlHead(EbmlHead& head,MKV::DataSource::IDataSource* dataSource,bool isEnd)
{
	if (isEnd)
		head.ParseCanIfEnd(dataSource);
	else
		head.Parse(dataSource);

	if (!head.IsValid())
		return false;

	if (head.SkipIfVoid())
		return FastParseEbmlHead(head,dataSource);

	return true;
}

bool MKV::EBML::FastParseEbmlHeadIfSize(EbmlHead& head,MKV::DataSource::IDataSource* dataSource,bool isEnd)
{
	if (!FastParseEbmlHead(head,dataSource,isEnd))
		return false;
	if (head.Size() == 0)
		return false;

	return true;
}

bool MKV::EBML::FastParseEbmlHeadIfSizeWithNoErr(EbmlHead& head,MKV::DataSource::IDataSource* dataSource,bool isEnd)
{
	if (!FastParseEbmlHead(head,dataSource,isEnd))
		return false;
	if (head.Size() == 0)
		head.ResetId();

	return true;
}

bool MKV::EBML::FastParseEbmlHeadIfSizeWithIfSpec(EbmlHead& head,MKV::DataSource::IDataSource* dataSource,bool isEnd,unsigned preserve_id)
{
	if (!FastParseEbmlHead(head,dataSource,isEnd))
		return false;

	if (head.Size() == 0)
	{
		if (preserve_id == INVALID_EBML_ID)
			head.ResetId();
		else if (preserve_id != head.Id())
			head.ResetId();
	}

	return true;
}

unsigned MKV::EBML::ReadUInt(EbmlHead& head,EbmlDataType type)
{
	if (type == UIntAs1Byte)
		return head.Read();

	if (type == UIntAs2Byte)
	{
		unsigned short value = 0;
		head.Read(&value,2);
		return MKV_SWAP16(value);
	}else if (type == UIntAs4Byte)
	{
		unsigned value = 0;
		head.Read(&value,4);
		return MKV_SWAP32(value);
	}else if (type == UIntAs3Byte)
	{
		unsigned char fuck[3] = {0,0,0};
		head.Read(fuck,3);

		unsigned value;
		unsigned char* p = (unsigned char*)&value;
		p[1] = fuck[2];
		p[2] = fuck[1];
		p[3] = fuck[0];

		return (value >> 8);
	}
	else if (type == UIntAs8Byte)
	{
		return (unsigned)ReadUInt64(head);
	}

	return INVALID_EBML_UINT32;
}

unsigned long long MKV::EBML::ReadUInt64(EbmlHead& head)
{
	unsigned long long result = INVALID_EBML_UINT64;
	unsigned char buf[8];
	if (head.Read(buf,sizeof(result)))
	{
		unsigned char* p = (unsigned char*)&result;
		for (unsigned i = 0;i < 8;i++)
			p[i] = buf[7 - i];

		/*
		p[0] = buf[7];
		p[1] = buf[6];
		p[2] = buf[5];
		p[3] = buf[4];
		p[4] = buf[3];
		p[5] = buf[2];
		p[6] = buf[1];
		p[7] = buf[0];
		*/
	}

	return result;
}

unsigned long long MKV::EBML::ReadUInt64On567(EbmlHead& head)
{
	unsigned long long result = INVALID_EBML_UINT64;
	if (head.Size() > 7)
		return result;

	unsigned char buf[7];
	if (head.Read(buf,(unsigned)head.Size()))
	{
		result = 0;
		unsigned size = (unsigned)head.Size();

		unsigned char* p = (unsigned char*)&result;
		for (unsigned i = 0;i < size;i++)
			p[i] = buf[(size - 1) - i];
	}

	return result;
}

float MKV::EBML::ReadFloat(EbmlHead& head)
{
	float result = INVALID_EBML_FLOAT;
	unsigned char buf[4];
	if (head.Read(buf,sizeof(result)))
	{
		unsigned char* p = (unsigned char*)&result;

		p[0] = buf[3];
		p[1] = buf[2];
		p[2] = buf[1];
		p[3] = buf[0];
	}

	return result;
}

double MKV::EBML::ReadDouble(EbmlHead& head)
{
	double result = INVALID_EBML_DOUBLE;
	unsigned char buf[8];
	if (head.Read(buf,sizeof(result)))
	{
		unsigned char* p = (unsigned char*)&result;
		for (unsigned i = 0;i < 8;i++)
			p[i] = buf[7 - i];
	}

	return result;
}

bool MKV::EBML::EbmlReadValue(EbmlHead& head,EBML_VALUE_DATA& value,bool bFloat)
{
	value.type = UnknownEbmlType;
	value.Ui64 = 0;

	switch (head.Size())
	{
	case 1:
	case 2:
	case 3:
		value.type = (decltype(value.type))head.Size();
		value.Ui32 = ReadUInt(head,value.type);
		break;
	case 4:
		if (!bFloat)
		{
			value.type = UIntAs4Byte;
			value.Ui32 = ReadUInt(head);
		}else{
			value.type = FloatAs4Byte;
			value.Float = ReadFloat(head);
			value.Double = (double)value.Float;
		}
		break;
	case 5:
	case 6:
	case 7:
		value.type = (decltype(value.type))head.Size();
		value.Ui64 = ReadUInt64On567(head);
		break;
	case 8:
		if (!bFloat)
		{
			value.type = UIntAs8Byte;
			value.Ui64 = ReadUInt64(head);
		}else{
			value.type = DoubleAs8Byte;
			value.Double = ReadDouble(head);
			value.Float = (float)value.Double;
		}
		break;
	default:
		return false;
	}

	return true;
}

bool MKV::EBML::EbmlReadString(EbmlHead& head,char* copyTo,unsigned copySize)
{
	if (copyTo == nullptr)
		return false;

	memset(copyTo,0,copySize);
	
	auto str = (char*)malloc(MKV_ALLOC_ALIGNED((unsigned)head.Size()));
	if (str == nullptr)
		return false;

	memset(str,0,(unsigned)head.Size() + 1);
	if (head.Read(str))
	{
		if (copySize == 0)
			strcpy(copyTo,str);
		else
			//strcpy_s(copyTo,copySize - 1,str);
			strncpy(copyTo,str,copySize - 1);
	}else{
		free(str);
		return false;
	}

	free(str);
	return true;
}

bool MKV::EBML::EbmlReadBinary(EbmlHead& head,void* bin,unsigned size)
{
	if (bin == nullptr)
		return false;
	if (head.Size() >= 0x80000000) //too 2GB.
		return false;
	if (size > head.Size())
		return false;

	if (size == 0)
	{
		if (!head.Read(bin,(unsigned)head.Size()))
			return false;
	}else{
		if (!head.Read(bin,size))
			return false;

		if (head.Size() > size)
			head.Skip((unsigned)head.Size() - size);
	}

	return true;
}

bool MKV::EBML::EbmlReadUIntValueSearch(EbmlHead& head,EBML_VALUE_DATA& value,const unsigned* id_list,unsigned count)
{
	for (unsigned i = 0;i < count;i++)
	{
		if (id_list[i] == head.Id())
			return EbmlReadValue(head,value);
	}

	return false;
}

unsigned MKV::EBML::ReadUIntFromPointer(unsigned char* p,unsigned long long* value)
{
	if (*p == 0)
		return 0;

	auto entry = p[0];
	int size = EBML::Core::GetVIntLength(entry);
	long long num = EBML::Core::StripZeroBitInNum(entry,size - 1);

	unsigned result = 1;
	for (int i = 1;i < size;i++)
	{
		result++;
		entry = p[i];

		num <<= 8;
		num |= entry;
	}

	*value = num;
	return result;
}

unsigned MKV::EBML::ReadSIntFromPointer(unsigned char* p,long long* value)
{
	unsigned long long num = 0;
	unsigned r = ReadUIntFromPointer(p,&num);
	if (r == 0)
		return 0;

	*value = num - ((1LL << (7 * r - 1)) - 1);
	return r;
}