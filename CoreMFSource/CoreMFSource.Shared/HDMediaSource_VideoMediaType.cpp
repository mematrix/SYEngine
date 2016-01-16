#include "HDMediaSource.h"
#include <H264\X264VideoDescription.h>
#include <MPEG2\MPEG2VideoDescription.h>
#include <MPEG4\MPEG4VideoDescription.h>

DEFINE_MEDIATYPE_GUID(MFVideoFormat_MP42,FCC('MP42'));
DEFINE_MEDIATYPE_GUID(MFVideoFormat_MPG4,FCC('MPG4'));

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define DS_MEDIASUBTYPE_MPEG1Packet L"{e436eb80-524f-11ce-9F53-0020af0ba770}"
#define DS_MEDIASUBTYPE_MPEG1Payload L"{e436eb81-524f-11ce-9F53-0020af0ba770}"
#define DS_MEDIASUBTYPE_MPEG2_VIDEO L"{e06d8026-db46-11cf-b4d1-00805f6cbbea}"
#define DS_MEDIASUBTYPE_MPEG1_VIDEO L"{e436eb86-524f-11ce-9f53-0020af0ba770}"

static inline bool IsWindows7()
{
	DWORD dwVersion = GetVersion();
	if (LOBYTE(LOWORD(dwVersion)) == 6 &&
		HIBYTE(LOWORD(dwVersion)) == 1)
		return true;
	return false;
}

static inline bool IsUseDShowFilter()
{
	if (!IsWindows7())
		return false;
	if (!LoadLibraryA("mfds.dll"))
		return false;
	return true;
}
#endif

#define H263_VID_WH_E(w,h,w2,h2) (w <= w2) && (h <= h2)
static unsigned GetH263ProfileLevelDefault(unsigned width,unsigned height)
{
	if (H263_VID_WH_E(width,height,176,144))
		return 1;
	else if (H263_VID_WH_E(width,height,352,288))
		return 2;
	else if (H263_VID_WH_E(width,height,352,756))
		return 4;
	else if (H263_VID_WH_E(width,height,720,576))
		return 5;
	else if (H263_VID_WH_E(width,height,640,480))
		return 0x4A;
	return 10;
}

HRESULT HDMediaSource::InitVideoHEVCMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,bool es)
{
	pMediaType->SetGUID(MF_MT_SUBTYPE,
		es ? MFVideoFormat_HEVC_ES:MFVideoFormat_HEVC);

	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,FALSE);
	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,1);
	pMediaType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES,FALSE);

	VideoBasicDescription desc = {};
	pDesc->GetVideoDescription(&desc);
	if (desc.aspect_ratio.num == 0 || desc.aspect_ratio.den == 0)
		MFSetAttributeRatio(pMediaType,MF_MT_PIXEL_ASPECT_RATIO,1,1);
	if (desc.frame_rate.num == 0 || desc.frame_rate.den == 0)
		pMediaType->DeleteItem(MF_MT_FRAME_RATE);

	H264_PROFILE_SPEC profile = {};
	pDesc->GetProfile(&profile);
	if (profile.profile >= 0)
	{
		if (profile.chroma_format > 1)
			return MF_E_INVALID_CODEC_MERIT; //4:2:0 chroma or monochrome only.

		pMediaType->SetUINT32(MF_MT_MPEG2_PROFILE,profile.profile);
		pMediaType->SetUINT32(MF_MT_MPEG2_LEVEL,profile.level);
		if (!es)
			pMediaType->SetUINT32(MF_MT_MPEG2_FLAGS,4);
	}

	if (pDesc->GetExtradataSize() > 0)
	{
		AutoComMem<unsigned char> pUserData(pDesc->GetExtradataSize() + 1);
		pDesc->GetExtradata(pUserData.Get());
		pMediaType->SetBlob(MF_MT_MPEG_SEQUENCE_HEADER,
		pUserData.Get(),pDesc->GetExtradataSize());
	}

	return S_OK;
}

HRESULT HDMediaSource::InitVideoH264MediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,bool es)
{
	pMediaType->SetGUID(MF_MT_SUBTYPE,
		es ? MFVideoFormat_H264_ES:MFVideoFormat_H264);

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	if (!IsWindows8())
	{
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_H264);
		_enableH264ES2H264 = es;
	}
#endif

	pMediaType->SetUINT32(MF_MT_DEFAULT_STRIDE,0);

	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,FALSE);
	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,1);
	pMediaType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES,FALSE);

	UINT32 scan_mode = MFVideoInterlace_Unknown;
	pMediaType->GetUINT32(MF_MT_INTERLACE_MODE,&scan_mode);
	if (scan_mode == MFVideoInterlace_Progressive)
		pMediaType->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_MixedInterlaceOrProgressive);

	VideoBasicDescription basicDesc = {};
	pDesc->GetVideoDescription(&basicDesc);
	if (basicDesc.width > 4096 ||
		basicDesc.height > 2304)
		return MF_E_INVALID_CODEC_MERIT;

	if (basicDesc.aspect_ratio.num == 0 || basicDesc.aspect_ratio.den == 0)
		pMediaType->DeleteItem(MF_MT_PIXEL_ASPECT_RATIO);
	if (basicDesc.frame_rate.num == 0 || basicDesc.frame_rate.den == 0)
		pMediaType->DeleteItem(MF_MT_FRAME_RATE);

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	if (!IsWindows8())
	{
		if (basicDesc.width > 1920 ||
			basicDesc.height > 1088)
			return MF_E_INVALID_CODEC_MERIT; //win7
	}
#endif

	H264_PROFILE_SPEC profile = {};
	pDesc->GetProfile(&profile);
	if (profile.profile == 0 && !es)
		return MF_E_INVALID_PROFILE;

	if (profile.chroma_format > 1 && !es)
		return MF_E_INVALID_CODEC_MERIT; //4:2:0 chroma or monochrome only.

	if (profile.profile >= H264_PROFILE_HIGH_10 && !es) {
		if (GlobalOptionGetBOOL(kCoreDisable10bitH264Video))
			return MF_E_INVALID_CODEC_MERIT; //Hi10P, Hi422P, Hi444P, Hi444PP is not support.
	}

	if (profile.chroma_bitdepth > 10 ||
		profile.luma_bitdepth > 10) {
		if (GlobalOptionGetBOOL(kCoreDisable10bitH264Video))
			return MF_E_INVALID_CODEC_MERIT; //10bit is not support.
	}

	if (profile.variable_framerate)
		if (basicDesc.frame_rate.num > 0 && basicDesc.frame_rate.den > 0)
			if (basicDesc.frame_rate.num / basicDesc.frame_rate.den > 512)
				MFSetAttributeRatio(pMediaType,MF_MT_FRAME_RATE,25,1);

	pMediaType->SetUINT32(MF_MT_MPEG2_PROFILE,profile.profile);
	pMediaType->SetUINT32(MF_MT_MPEG2_LEVEL,profile.level);
	if (!es)
		pMediaType->SetUINT32(MF_MT_MPEG2_FLAGS,4);

	if (profile.nalu_size == 1 || profile.nalu_size == 2 || profile.nalu_size == 4)
		pMediaType->SetUINT32(MF_MT_MPEG2_FLAGS,profile.nalu_size);

	if (pDesc->GetExtradataSize() > 0)
	{
		AutoComMem<unsigned char> pUserData(pDesc->GetExtradataSize() + 1);
		pDesc->GetExtradata(pUserData.Get());
		if (pDesc->GetExtradataSize() > 9 && profile.level == 52)
		{
			//fixed 5.2 -> 5.1
			if (pUserData.Get()[7] == 52)
				pUserData.Get()[7] = 51;
		}

		pMediaType->SetBlob(MF_MT_MPEG_SEQUENCE_HEADER,
		pUserData.Get(),pDesc->GetExtradataSize());
	}

	return S_OK;
}

HRESULT HDMediaSource::InitVideoMPEG2MediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType)
{
	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,1);
	pMediaType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES,FALSE);

	MPEG2_PROFILE_SPEC profile = {};
	if (!pDesc->GetProfile(&profile))
		return MF_E_INVALID_PROFILE;

	if (profile.exists && profile.colors != MPEG2C_420)
		return MF_E_INVALID_CODEC_MERIT;

	pMediaType->SetGUID(MF_MT_SUBTYPE,
		profile.exists ? MFVideoFormat_MPEG2:MFVideoFormat_MPG1);

	if (profile.exists)
	{
		pMediaType->SetUINT32(MF_MT_MPEG2_PROFILE,
			profile.profile);
		pMediaType->SetUINT32(MF_MT_MPEG2_LEVEL,
			profile.level);
	}else{
		pMediaType->SetUINT32(MF_MT_INTERLACE_MODE,
			MFVideoInterlace_Progressive);
	}

	if (pDesc->GetExtradataSize() == 0)
		return MF_E_INVALID_PROFILE;

	AutoComMem<unsigned char> pUserData(pDesc->GetExtradataSize() + 1);
	pDesc->GetExtradata(pUserData.Get());
	pMediaType->SetBlob(MF_MT_MPEG_SEQUENCE_HEADER,
		pUserData.Get(),pDesc->GetExtradataSize());

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	GUID subType = GUID_NULL;
	if (IsUseDShowFilter())
	{
		if (SUCCEEDED(CLSIDFromString(DS_MEDIASUBTYPE_MPEG1Packet,&subType)))
		{
			if (profile.exists)
				CLSIDFromString(DS_MEDIASUBTYPE_MPEG2_VIDEO,&subType);
			pMediaType->SetGUID(MF_MT_AM_FORMAT_TYPE,subType);
		}
	}
#endif

	return S_OK;
}

HRESULT HDMediaSource::InitVideoH263MediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType)
{
	VideoBasicDescription desc = {};
	pDesc->GetVideoDescription(&desc);

	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,1);
	pMediaType->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_MixedInterlaceOrProgressive);
	pMediaType->SetUINT32(MF_MT_MPEG2_LEVEL,GetH263ProfileLevelDefault(desc.width,desc.height));
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_H263);
#if WINAPI_PARTITION_PHONE_APP
	pMediaType->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive);
	pMediaType->SetUINT32(MF_MT_MPEG2_LEVEL,10);
#endif

	return S_OK;
}

HRESULT HDMediaSource::InitVideoMPEG4MediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType)
{
	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,1);
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_MP4V);

	MPEG4_PROFILE_SPEC profile = {};
	if (!pDesc->GetProfile(&profile))
		return MF_E_INVALID_PROFILE;

	if (profile.encoder != MPEG4V_UserData_Encoder::MP4V_Encoder_XVID)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_M4S2);
	if (profile.h263)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_H263);
	
	if (profile.bits != 8 ||
		!profile.chroma420)
		return MF_E_INVALID_PROFILE;

	if (pDesc->GetExtradataSize() == 0)
		return MF_E_INVALID_PROFILE;

	AutoComMem<unsigned char> pUserData(pDesc->GetExtradataSize() + 1);
	pDesc->GetExtradata(pUserData.Get());
	pMediaType->SetBlob(MF_MT_USER_DATA,pUserData.Get(),pDesc->GetExtradataSize());

	return S_OK;
}

HRESULT HDMediaSource::InitVideoMJPEGMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType)
{
	VideoBasicDescription desc = {};
	pDesc->GetVideoDescription(&desc);

	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);
	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,desc.width * desc.height * 3);

	return pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_MJPG);
}

HRESULT HDMediaSource::InitVideoMSMPEGMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType ct)
{
	static const GUID MF_MT_AVI_AVG_FPS = {0xc496f370,0x2f8b,0x4f51,{0xae,0x46,0x9c,0xfc,0x1b,0xc8,0x2a,0x47}};
	static const GUID MF_MT_AVI_FLAGS = {0x24974215,0x1b7b,0x41e4,{0x86,0x25,0xac,0x46,0x9f,0x2d,0xed,0xaa}};

	VideoBasicDescription desc = {};
	pDesc->GetVideoDescription(&desc);

	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,desc.width * desc.height * 3);
	
	pMediaType->SetUINT32(MF_MT_AVI_FLAGS,1);
	pMediaType->SetUINT32(MF_MT_AVI_AVG_FPS,
		(UINT32)((float)desc.frame_rate.num / (float)desc.frame_rate.den + 0.5f));

	pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_MP43);
	if (ct == MEDIA_CODEC_VIDEO_MS_MPEG4_V2)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_MP42);
	else if (ct == MEDIA_CODEC_VIDEO_MS_MPEG4_V1)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_MPG4);

	return S_OK;
}

HRESULT HDMediaSource::InitVideoWMVMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type)
{
	VideoBasicDescription desc = {};
	pDesc->GetVideoDescription(&desc);
	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,desc.width * desc.height * 3);

	switch (codec_type)
	{
	case MEDIA_CODEC_VIDEO_WMV7:
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_WMV1);
		break;
	case MEDIA_CODEC_VIDEO_WMV8:
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_WMV2);
		break;
	case MEDIA_CODEC_VIDEO_WMV9:
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_WMV3);
		break;
	}

	AutoComMem<unsigned char> pUserData(pDesc->GetExtradataSize() + 1);
	if (pDesc->GetExtradata(pUserData.Get()))
		pMediaType->SetBlob(MF_MT_USER_DATA,pUserData.Get(),pDesc->GetExtradataSize());

	return S_OK;
}

HRESULT HDMediaSource::InitVideoVC1MediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType)
{
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_WVC1);

	unsigned size = pDesc->GetExtradataSize();
	if (size > 0)
	{
		AutoComMem<unsigned char> pUserData(size + 2);
		pDesc->GetExtradata(pUserData.Get() + 1);
		if (*(pUserData.Get() + 1) == 0)
		{
			*pUserData.Get() = 0xFF;
			pMediaType->SetBlob(MF_MT_USER_DATA,pUserData.Get(),size + 1);
		}else{
			pMediaType->SetBlob(MF_MT_USER_DATA,pUserData.Get() + 1,size);
		}
	}

	return S_OK;
}

HRESULT HDMediaSource::InitVideoVPXMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type)
{
	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,FALSE);
	pMediaType->SetUINT32(MF_MT_AVG_BITRATE,0);
	pMediaType->SetUINT32(MF_MT_AVG_BIT_ERROR_RATE,0);
	pMediaType->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive);
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_VP8);
	if (codec_type == MediaCodecType::MEDIA_CODEC_VIDEO_VP9)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_VP9);
	else if (codec_type == MediaCodecType::MEDIA_CODEC_VIDEO_VP10)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_VP10);
	return S_OK;
}

HRESULT HDMediaSource::InitVideoVP6MediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type)
{
	InitVideoVPXMediaType(pDesc,pMediaType,codec_type);
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_VP6);
	if (codec_type == MediaCodecType::MEDIA_CODEC_VIDEO_VP6F)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_VP6F);
	else if (codec_type == MediaCodecType::MEDIA_CODEC_VIDEO_VP6A)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_VP6A);
	return S_OK;
}

HRESULT HDMediaSource::InitVideoQuickSyncMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType ct)
{
	if (ct != MEDIA_CODEC_VIDEO_MPEG2 &&
		ct != MEDIA_CODEC_VIDEO_H264 &&
		ct != MEDIA_CODEC_VIDEO_HEVC)
		return S_OK;
	if (ct == MEDIA_CODEC_VIDEO_MPEG2)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_Intel_QS_MPEG2);
	else if (ct == MEDIA_CODEC_VIDEO_H264)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_Intel_QS_H264);
	else if (ct == MEDIA_CODEC_VIDEO_HEVC)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_Intel_QS_HEVC);
	return S_OK;
}