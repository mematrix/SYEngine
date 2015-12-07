/*
 - FLV Stream Parser (Kernel) -

 - Author: K.F Yang
 - Date: 2014-11-26
*/

#include "FLVParser.h"
#include <memory.h>
#include <string.h>

unsigned VerifyFlvStreamHeader(FLVParser::IFLVParserIO* pIo,unsigned& av_flag)
{
	unsigned char head[FLV_HEAD_LENGTH] = {};
	auto r = pIo->Read(head,FLV_HEAD_LENGTH);
	if (r != FLV_HEAD_LENGTH)
		return 0;

	if (head[0] != 'F' || head[1] != 'L' || head[2] != 'V' || head[3] != 1)
		return 0;

	if (head[4] == 0)
		return 0;

	av_flag = head[4];
	
	unsigned len = *(unsigned*)&head[FLV_HEAD_INFO_LENGTH];
	return FLV_SWAP32(len);
}

bool ReadFlvStreamTag(FLVParser::IFLVParserIO* pIo,unsigned& tag_type,unsigned& data_size,unsigned& timestamp)
{
	FLVParser::FLV_BODY_TAG body = {};
	auto r = pIo->Read(&body,sizeof(body));
	if (r != sizeof(body))
		return false;

	tag_type = (body.tag.type & 0x1F);

	unsigned size_temp = *(unsigned*)&body.tag.type;
	size_temp = (FLV_SWAP32(size_temp) & 0x00FFFFFF);

	data_size = size_temp;

	unsigned char* p = (unsigned char*)&timestamp;

#ifndef _MSC_VER
	p[0] = body.tag.timestamp[3];
	p[1] = body.tag.timestamp[0];
	p[2] = body.tag.timestamp[1];
	p[3] = body.tag.timestamp[2];
	timestamp = FLV_SWAP32(timestamp);
#else
	p[0] = body.tag.timestamp[2];
	p[1] = body.tag.timestamp[1];
	p[2] = body.tag.timestamp[0];
	p[3] = body.tag.timestamp[3];
#endif

	return true;
}

int AMFParseGetString(unsigned char* pb,char* pstr,int len)
{
	int size = FLV_SWAP16(*(unsigned short*)pb);
	if (len >= size && pstr)
		memcpy(pstr,pb + 2,size);

	return size;
}

unsigned AMFParseFindOnMetadata(unsigned char* pb,unsigned len)
{
	static const char onMetaData[] = FLV_META_OBJECT_SEPC;
	for (unsigned i = 0;i < (len - 15);i++)
	{
		if (memcmp(pb + i,onMetaData,10) == 0)
			return i + 10;
	}

	return 0;
}

double AMFParseHexToDouble(unsigned char* pb)
{
	double temp;
	unsigned char* p = (unsigned char*)&temp;

	for (unsigned i = 0;i < 8;i++)
	{
		*p = pb[7 - i];
		p++;
	}

	return temp;
}

unsigned AMFParseMakeKeyFrameIndex(unsigned char* pb,unsigned len,FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX** kf_index)
{
	unsigned result = 0;

	unsigned char* p = pb;
	char name[128];

	FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX* index = nullptr;

	while (1)
	{
		memset(name,0,128);
		unsigned str_len = AMFParseGetString(p,name,128);
		if (str_len == 0)
			break;

		p = p + 2 + str_len;
		if (*p != FLV_AMF_DATA_TYPE_ARRAY)
			break;
		p++;
		
		unsigned count = FLV_SWAP32(*(unsigned*)p);
		if (count == 0)
			break;
		p += 4;
		
		if (!strcmp(name,FLV_META_KEY_FRAMES_TIMESTAMP) || !strcmp(name,FLV_META_KEY_FRAMES_BYTEOFFSET))
		{
			result = count;

			if (index == nullptr)
				index = (FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX*)malloc(
					sizeof(FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX) * (count + 1));

			if (index == nullptr)
				return 0xFFFFFFFF;

			bool bpos = (strcmp(name,FLV_META_KEY_FRAMES_TIMESTAMP) == 0 ? false:true);

			for (unsigned i = 0;i < count;i++)
			{
				if (*p != FLV_AMF_DATA_TYPE_NUMBER)
					return 0;

				p++;
				double val = AMFParseHexToDouble(p);
				p += 8;

				if (bpos)
					(index + i)->pos = (long long)val;
				else
					(index + i)->time = val;
			}
		}
	}

	*kf_index = index;
	return result;
}

unsigned AMFParseObjectAndInit(unsigned char* pb,unsigned len,char* key,FLVParser::FLV_STREAM_GLOBAL_INFO* global_info,FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX** kf_index,unsigned* kf_count)
{
	auto amf_type = *pb;
	unsigned offset = 1; //amf_type

	char val_str[512];
	double val_num;
	int val_bool;

	if (key != nullptr)
	{
		if (strlen(key) == 0)
			return 0;
	}

	switch (amf_type)
	{
	case FLV_AMF_DATA_TYPE_NUMBER:
		val_num = AMFParseHexToDouble(pb + offset);
		offset += 8;
		break;
	case FLV_AMF_DATA_TYPE_BOOL:
		val_bool = *(pb + offset);
		offset += 1;
		break;
	case FLV_AMF_DATA_TYPE_STRING:
		memset(val_str,0,512);
		offset = offset +
			AMFParseGetString(pb + offset,val_str,512) + 2;
		break;
	case FLV_AMF_DATA_TYPE_OBJECT:
		if (key && !strcmp(key,FLV_META_KEY_FRAMES))
		{
			if (kf_index && kf_count)
				*kf_count = AMFParseMakeKeyFrameIndex(pb + offset,len - offset,kf_index);
		}

		while (1)
		{
			memset(val_str,0,512);
			unsigned value_size = AMFParseGetString(pb + offset,val_str,512);
			if (value_size == 0)
				break;

			offset = offset + 2 + value_size;
			offset = offset + AMFParseObjectAndInit(pb + offset,
				len - offset,val_str,global_info,kf_index,kf_count);

			if (offset >= (len - 4))
				break;
		}
		
		if (offset <= (len - 3))
		{
			if (pb[offset + 0] == 0 &&
				pb[offset + 1] == 0 &&
				pb[offset + 2] == 9)
				offset += 3;
		}

		break;
	case FLV_AMF_DATA_TYPE_ARRAY_MIXED:
		offset += 4;
		while (1)
		{
			memset(val_str,0,512);
			unsigned value_size = AMFParseGetString(pb + offset,val_str,512);
			if (value_size == 0)
				break;

			offset = offset + 2 + value_size;
			offset = offset + AMFParseObjectAndInit(pb + offset,
				len - offset,val_str,global_info,kf_index,kf_count);

			if (offset >= (len - 4))
				break;
		}

		if (offset <= (len - 3))
		{
			if (pb[offset + 0] == 0 &&
				pb[offset + 1] == 0 &&
				pb[offset + 2] == 9)
				offset += 3;
		}

		break;
	case FLV_AMF_DATA_TYPE_ARRAY:
		unsigned count;
		count = FLV_SWAP32(*(unsigned*)(pb + offset));
		offset += 4;

		for (unsigned i = 0;i < count;i++)
			offset = offset + AMFParseObjectAndInit(pb + offset,len - offset,nullptr,global_info,kf_index,kf_count);

		break;
	case FLV_AMF_DATA_TYPE_DATE:
		offset = offset + 8 + 2; //double + utc
		break;
	case FLV_AMF_DATA_TYPE_LONG_STRING:
		unsigned len;
		len = FLV_SWAP32(*(unsigned*)(pb + offset));
		offset = offset + 4 + len;
		break;
	}

	if (key == nullptr)
		return offset;

	if (amf_type == FLV_AMF_DATA_TYPE_NUMBER || amf_type == FLV_AMF_DATA_TYPE_BOOL)
	{
		if (strcmp(key,FLV_META_AUDIO_SAMPLE_RATE_NUMBER) == 0)
			global_info->audio_info.srate = (int)val_num;
		else if (strcmp(key,FLV_META_AUDIO_SAMPLE_SIZE_NUMBER) == 0)
			global_info->audio_info.bits = (int)val_num;
		else if (strcmp(key,FLV_META_AUDIO_CH_TYPE_BOOL) == 0)
			global_info->audio_info.nch = val_bool + 1;
		else if (strcmp(key,FLV_META_AUDIO_DATA_RATE_NUMBER) == 0)
			global_info->audio_info.bitrate = (int)val_num;

		else if (strcmp(key,FLV_META_VIDEO_FRAME_WIDTH_NUMBER) == 0)
			global_info->video_info.width = (int)val_num;
		else if (strcmp(key,FLV_META_VIDEO_FRAME_HEIGHT_NUMBER) == 0)
			global_info->video_info.height = (int)val_num;
		else if (strcmp(key,FLV_META_VIDEO_FRAME_RATE_NUMBER) == 0)
			global_info->video_info.fps = val_num;
		else if (strcmp(key,FLV_META_VIDEO_DATA_RATE_NUMBER) == 0)
			global_info->video_info.bitrate = (int)val_num;

		else if (strcmp(key,FLV_META_FILE_DURATION_NUMBER) == 0)
			global_info->duration = val_num;

		else if (strcmp(key,FLV_META_AUDIO_CODEC_ID_NUMBER) == 0)
			global_info->audio_info.raw_codec_id = (int)val_num;
		else if (strcmp(key,FLV_META_VIDEO_CODEC_ID_NUMBER) == 0)
			global_info->video_info.raw_codec_id = (int)val_num;
	}

	return offset;
}