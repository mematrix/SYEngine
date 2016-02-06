#ifndef __MEDIA_AV_FORMAT_H
#define __MEDIA_AV_FORMAT_H

#include "AVDescription.h"
#include "MediaAVIO.h"
#include "MediaAVError.h"
#include "MediaAVStream.h"
#include "MediaAVChapter.h"
#include "MediaAVMetadata.h"
#include "MediaAVResource.h"
#include "MediaMemory.h"
#include "MediaPacket.h"

#pragma region Format Flags
#define MEDIA_FORMAT_CAN_SEEK_BACKWARD 1 //��ǰSeek
#define MEDIA_FORMAT_CAN_SEEK_FORWARD 2 //���Seek
#define MEDIA_FORMAT_CAN_RESET 4 //��λSeek
#define MEDIA_FORMAT_CAN_FLUSH 8 //�������ջ��棨������

#define MEDIA_FORMAT_CAN_SEEK (MEDIA_FORMAT_CAN_SEEK_BACKWARD|MEDIA_FORMAT_CAN_SEEK_FORWARD)
#define MEDIA_FORMAT_CAN_SEEK_ALL (MEDIA_FORMAT_CAN_SEEK|MEDIA_FORMAT_CAN_RESET)

#define MEDIA_FORMAT_READER_SKIP_AV_OUTSIDE 1
#define MEDIA_FORMAT_READER_SKIP_ALL_SUBTITLE 2
//H264��HEVC�ض��������NALU��ʽ��������ܣ�
#define MEDIA_FORMAT_READER_H264_FORCE_AVC1 0x4000
//MKV�ض�����ASS&SSA����Ļ�ڲ��л�ΪSRT��ʽ
#define MEDIA_FORMAT_READER_MATROSKA_ASS2SRT 0x8000
//MKV�ض���������MKV��SeekHead
#define MEDIA_FORMAT_READER_MATROSKA_DNT_PARSE_SEEKHEAD 0x10000
#pragma endregion

#define AV_MEDIA_INTERFACE_ID_CASE_EX 0x100000 //IAVMediaFormatEx*
#define AV_MEDIA_INTERFACE_ID_CAST_CHAPTER 0x100001 //IAVMediaChapters*
#define AV_MEDIA_INTERFACE_ID_CAST_METADATA 0x100002 //IAVMediaMetadata*
#define AV_MEDIA_INTERFACE_ID_CAST_RESOURCE 0x100003 //IAVMediaResources*

enum AVSeekDirection
{
	SeekDirection_Auto = 0,
	SeekDirection_Next,
	SeekDirection_Back
};

class IAVMediaFormat
{
public:
	virtual AV_MEDIA_ERR Open(IAVMediaIO* io) = 0; //Make Streams.
	virtual AV_MEDIA_ERR Close() = 0; //Destroy Streams.

	virtual void Dispose() = 0; //Delete this.

	virtual double GetDuration() = 0; //Seconds.
	virtual double GetStartTime() { return 0.0; } //Seconds
	virtual unsigned GetBitRate() { return 0; }

	virtual int GetStreamCount() = 0;
	virtual IAVMediaStream* GetStream(int index) = 0; //index�±�0

	virtual unsigned GetFormatFlags() { return MEDIA_FORMAT_CAN_SEEK_ALL; }
	virtual char* GetFormatName() { return nullptr; }
	virtual char* GetMimeType() { return nullptr; }

	virtual void SetReadFlags(unsigned flags) {}
	virtual void SetIoCacheSize(unsigned size) {}

	virtual AV_MEDIA_ERR Flush() { return AV_ERR_OK; }
	virtual AV_MEDIA_ERR Reset() = 0;

	virtual AV_MEDIA_ERR Seek(double seconds,bool key_frame,AVSeekDirection seek_direction) = 0;
	virtual AV_MEDIA_ERR ReadPacket(AVMediaPacket* packet) = 0;

	virtual AV_MEDIA_ERR OnNotifySeek() { return AV_ERR_OK; }

	virtual AV_MEDIA_ERR GetPrivateData(void*)
	{
		return AV_COMMON_ERR_NOTIMPL;
	}

	virtual void* StaticCastToInterface(unsigned id)
	{
		return nullptr;
	}

	virtual ~IAVMediaFormat() {}
};

struct AVMediaUTCDate
{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int seconds;
};

struct AVMediaAppName
{
	const char* main_app_name;
	const char* lib_app_name;
	const char* others;
};

class IAVMediaFormatEx : public IAVMediaFormat
{
public:
	virtual int GetKeyFramesCount() { return -1; }
	virtual unsigned CopyKeyFrames(double* copyTo,int copyCount = -1) { return false; } //����������count

	virtual bool GetCreatedDate(AVMediaUTCDate* date) { return false; }
	virtual bool GetAppName(AVMediaAppName* appName) { return false; }

	virtual ~IAVMediaFormatEx() {}
};

#endif //__MEDIA_AV_FORMAT_H