#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __MF_SAMPLE_HELP_H
#define __MF_SAMPLE_HELP_H

#include "MFCommon.h"

namespace WMF{
namespace Misc{

HRESULT CreateSampleWithBuffer(IMFSample** ppSample,IMFMediaBuffer** ppBuffer,DWORD dwBufSize,DWORD dwAlignedSize = 0);
HRESULT CreateSampleAndCopyData(IMFSample** ppSample,PVOID pData,DWORD dwDataSize,MFTIME pts,MFTIME duration);

HRESULT CreateAudioMediaType(IMFMediaType** ppMediaType);
HRESULT CreateVideoMediaType(IMFMediaType** ppMediaType);

HRESULT IsAudioMediaType(IMFMediaType* pMediaType);
HRESULT IsVideoMediaType(IMFMediaType* pMediaType);

HRESULT IsAudioPCMSubType(IMFMediaType* pMediaType);
HRESULT IsVideoRGBSubType(IMFMediaType* pMediaType);
HRESULT IsVideoYUVSubType(IMFMediaType* pMediaType);

HRESULT GetMediaTypeFromPD(IMFPresentationDescriptor* ppd,DWORD dwStreamIndex,IMFMediaType** ppMediaType);

LONG64 SecondsToMFTime(double seconds);
double SecondsFromMFTime(LONG64 time);
double GetSecondsFromMFSample(IMFSample* pSample);

bool IsMFTExists(LPCWSTR clsid);
bool IsMFTExists(LPCSTR clsid);

}} //namespace WMF::Misc.

#endif //__MF_SAMPLE_HELP_H