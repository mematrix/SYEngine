#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MF_SAMPLE_HELP2_H
#define __MF_SAMPLE_HELP2_H

#include "MFCommon.h"

namespace WMF{
namespace MediaType{

void SetMajorAndSubType(IMFMediaType* pMediaType,const GUID& majorType,const GUID& subType);

enum MFMediaType
{
	MediaType_MajorType,
	MediaType_SubType
};

GUID GetMediaTypeGUID(IMFMediaType* pMediaType,MFMediaType guidType);

struct MFVideoBasicInfo
{
	unsigned width;
	unsigned height;
	MFRatio fps;
	MFRatio ar;
	MFVideoInterlaceMode interlace_mode;
	unsigned sample_size;
};
HRESULT GetBasicVideoInfo(IMFMediaType* pMediaType,MFVideoBasicInfo* info);

struct MFAudioBasicInfo
{
	unsigned channelCount;
	unsigned sampleRate;
	unsigned sampleSize;
	unsigned chLayout;
	unsigned blockAlign;
};
HRESULT GetBasicAudioInfo(IMFMediaType* pMediaType,MFAudioBasicInfo* info);

HRESULT GetBlobDataAlloc(IMFMediaType* pMediaType,const GUID& guidKey,BYTE** ppBuffer,PDWORD pdwSize);

}} //namespace WMF::MediaType.

#endif