#include "HDMediaSource.h"

static bool FpsFloatToRatio(float fps,unsigned* num,unsigned* den)
{
	struct Ratio { int num, den; };
	if (fps > 120.0f && fps != 240.0f)
		return false;

	static const unsigned rates[] = {10,14,15,20,23,24,25,29,30,47,48,50,59,60,90,95,96,100,119,120,240};
	static const Ratio rationals[] = {
			{10,1}, //10.00
			{15000,1001}, //14.98
			{15,1}, //15.00
			{20,1}, //20.00
			{10000000,417083}, //23.976
			{24,1}, //24.00
			{25,1}, //25.00
			{30000,1001}, //29.97
			{30,1}, //30.00
			{20000000,417083}, //47.952
			{48,1}, //48.00
			{50,1}, //50.00
			{60000,1001}, //59.94
			{60,1}, //60.00
			{90,1}, //90.00
			{40000000,417083}, //95.904
			{96,1}, //96.00
			{100,1}, //100.00
			{120000,1001}, //119.88
			{120,1}, //120.00
			{240,1} //240.00
	};

	unsigned value = (unsigned)fps;
	for (int i = 0;i < _countof(rates);i++)
	{
		if (value == rates[i])
		{
			*num = (unsigned)rationals[i].num;
			*den = (unsigned)rationals[i].den;
			return true;
		}
	}

	return false;
}

static void AutoSetupFlacDecoderProfile()
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	bool sy = false,system = false,ffmpeg = false;
	sy = WMF::Misc::IsMFTExists(L"{5F87E609-FE55-4760-B107-D2E54B94A50E}");
	system = WMF::Misc::IsMFTExists(L"{6B0B3E6B-A2C5-4514-8055-AFE8A95242D9}");
	ffmpeg = WMF::Misc::IsMFTExists(L"{321DF3AE-3F60-48BC-90C8-25F76E028A4E}");
	if (ffmpeg)
		GlobalSettings->SetUINT32(kCoreUseDShowFLACDecoder,TRUE);
	if (system)
		GlobalSettings->SetUINT32(kCoreUseWin10FLACDecoder,TRUE);
#endif
}

HRESULT HDMediaSource::CreateAudioMediaType(IAVMediaStream* pAVStream,IMFMediaType** ppMediaType)
{
	ComPtr<IMFMediaType> pMediaType;
	HRESULT hr = WMF::Misc::CreateAudioMediaType(pMediaType.GetAddressOf());
	if (FAILED(hr))
		return hr;

	auto audioDesc = pAVStream->GetAudioInfo();
	AudioBasicDescription basicDesc = {};
	audioDesc->GetAudioDescription(&basicDesc);

	pMediaType->SetUINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX,TRUE);

	if (basicDesc.nch == 0 ||
		basicDesc.nch > 8)
		return MF_E_INVALIDMEDIATYPE;
	if (basicDesc.srate == 0)
		return MF_E_INVALIDMEDIATYPE;

	if (basicDesc.compressed)
		pMediaType->SetUINT32(MF_MT_COMPRESSED,TRUE);

	if (basicDesc.bits != 0)
		pMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,basicDesc.bits);
	if (basicDesc.nch != 0)
		pMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS,basicDesc.nch);
	if (basicDesc.srate != 0)
		pMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,basicDesc.srate),
		pMediaType->SetDouble(MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND,(double)basicDesc.srate);
	if (basicDesc.ch_layout != 0)
		pMediaType->SetUINT32(MF_MT_AUDIO_CHANNEL_MASK,basicDesc.ch_layout);
	if (basicDesc.bitrate > 8)
		pMediaType->SetUINT32(MF_MT_AVG_BITRATE,basicDesc.bitrate);

	switch (pAVStream->GetCodecType())
	{
	case MEDIA_CODEC_AUDIO_AAC:
	case MEDIA_CODEC_AUDIO_AAC_ADTS:
		hr = InitAudioAACMediaType(audioDesc,pMediaType.Get(),
			pAVStream->GetCodecType() == MEDIA_CODEC_AUDIO_AAC_ADTS);
		if (hr == MF_E_INVALID_PROFILE && 
			pAVStream->GetCodecType() != MEDIA_CODEC_AUDIO_AAC_ADTS)
			hr = InitAudioFAADMediaType(audioDesc,pMediaType.Get(),
				pAVStream->GetCodecType() == MEDIA_CODEC_AUDIO_AAC_ADTS);
		break;
	case MEDIA_CODEC_AUDIO_MP1:
	case MEDIA_CODEC_AUDIO_MP2:
	case MEDIA_CODEC_AUDIO_MP3:
		hr = InitAudioMP3MediaType(audioDesc,pMediaType.Get(),
			pAVStream->GetCodecType());
		break;
	case MEDIA_CODEC_AUDIO_AC3:
	case MEDIA_CODEC_AUDIO_EAC3:
		hr = InitAudioAC3MediaType(audioDesc,pMediaType.Get(),
			pAVStream->GetCodecType() == MEDIA_CODEC_AUDIO_EAC3);
		break;
	case MEDIA_CODEC_AUDIO_DTS:
		hr = InitAudioDTSMediaType(audioDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_AUDIO_TRUEHD:
		hr = InitAudioTrueHDMediaType(audioDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_AUDIO_ALAC:
		hr = InitAudioALACMediaType(audioDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_AUDIO_FLAC:
		AutoSetupFlacDecoderProfile();
		hr = InitAudioFLACMediaType(audioDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_AUDIO_VORBIS:
		hr = InitAudioOGGMediaType(audioDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_AUDIO_OPUS:
		hr = InitAudioOPUSMediaType(audioDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_AUDIO_TTA:
		hr = InitAudioTTAMediaType(audioDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_AUDIO_WAVPACK:
		hr = InitAudioWV4MediaType(audioDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_LPCM_BD:
		hr = InitAudioBDMediaType(audioDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_AUDIO_WMA7:
	case MEDIA_CODEC_AUDIO_WMA8:
	case MEDIA_CODEC_AUDIO_WMA9_PRO:
	case MEDIA_CODEC_AUDIO_WMA9_LOSSLESS:
		hr = InitAudioWMAMediaType(audioDesc,pMediaType.Get(),
			pAVStream->GetCodecType());
		break;
	case MEDIA_CODEC_AUDIO_AMR_NB:
	case MEDIA_CODEC_AUDIO_AMR_WB:
	case MEDIA_CODEC_AUDIO_AMR_WBP:
		hr = InitAudioAMRMediaType(audioDesc,pMediaType.Get(),
			pAVStream->GetCodecType());
		break;
	case MEDIA_CODEC_AUDIO_COOK:
		hr = InitAudioRealCookMediaType(audioDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_PCM_8BIT:
	case MEDIA_CODEC_PCM_16BIT_LE:
	case MEDIA_CODEC_PCM_16BIT_BE:
	case MEDIA_CODEC_PCM_24BIT_LE:
	case MEDIA_CODEC_PCM_24BIT_BE:
	case MEDIA_CODEC_PCM_32BIT_LE:
	case MEDIA_CODEC_PCM_32BIT_BE:
	case MEDIA_CODEC_PCM_SINT_LE:
	case MEDIA_CODEC_PCM_SINT_BE:
	case MEDIA_CODEC_PCM_FLOAT:
		hr = InitAudioPCMMediaType(audioDesc,pMediaType.Get(),pAVStream->GetCodecType());
		break;
	default:
		hr = MF_E_INVALID_CODEC_MERIT;
	}

	if (SUCCEEDED(hr))
	{
		AudioBasicDescription desc = {};
		audioDesc->GetAudioDescription(&desc);
		if (desc.wav_avg_bytes > 0) //such as OGG, AC3, DTS...
		{
			pMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,desc.wav_avg_bytes);
			pMediaType->SetUINT32(MF_MT_AVG_BITRATE,desc.wav_avg_bytes * 8);
		}
	}

	if (SUCCEEDED(hr))
		*ppMediaType = pMediaType.Detach();

	return hr;
}

HRESULT HDMediaSource::CreateVideoMediaType(IAVMediaStream* pAVStream,IMFMediaType** ppMediaType)
{
	ComPtr<IMFMediaType> pMediaType;
	HRESULT hr = WMF::Misc::CreateVideoMediaType(pMediaType.GetAddressOf());
	if (FAILED(hr))
		return hr;

	auto videoDesc = pAVStream->GetVideoInfo();
	VideoBasicDescription basicDesc = {};
	videoDesc->GetVideoDescription(&basicDesc);

	if (basicDesc.compressed)
		pMediaType->SetUINT32(MF_MT_COMPRESSED,TRUE);
	if (basicDesc.bitrate > 0)
		pMediaType->SetUINT32(MF_MT_AVG_BITRATE,basicDesc.bitrate);

	MFSetAttributeSize(pMediaType.Get(),MF_MT_FRAME_SIZE,
		basicDesc.width,basicDesc.height);

	MFSetAttributeRatio(pMediaType.Get(),MF_MT_FRAME_RATE,
		basicDesc.frame_rate.num,basicDesc.frame_rate.den);

	if (basicDesc.frame_rate.den != 0)
	{
		float fps = (float)basicDesc.frame_rate.num / (float)basicDesc.frame_rate.den;
		unsigned fn = 0,fd = 0;
		if (FpsFloatToRatio(fps,&fn,&fd))
			MFSetAttributeRatio(pMediaType.Get(),MF_MT_FRAME_RATE,fn,fd);
	}

	MFSetAttributeRatio(pMediaType.Get(),MF_MT_PIXEL_ASPECT_RATIO,
		basicDesc.aspect_ratio.num,basicDesc.aspect_ratio.den);

	/*
	if (basicDesc.frame_rate.den > 0)
	{
		if (basicDesc.frame_rate.num / basicDesc.frame_rate.den > 240) //bukexue.
			MFSetAttributeRatio(pMediaType.Get(),MF_MT_FRAME_RATE,
				10000000,417071); //23.976
	}
	*/

	if (basicDesc.scan_mode != VideoScanModeUnknown)
		pMediaType->SetUINT32(MF_MT_INTERLACE_MODE,((UINT32)basicDesc.scan_mode) + 1);
	else
		pMediaType->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_MixedInterlaceOrProgressive);

	if (basicDesc.bitrate > 8)
		pMediaType->SetUINT32(MF_MT_AVG_BITRATE,basicDesc.bitrate);

	if (pAVStream->GetRotation() > 0 &&
		pAVStream->GetRotation() < 360)
		pMediaType->SetUINT32(MF_MT_VIDEO_ROTATION,pAVStream->GetRotation());

	if (pAVStream->GetContainerFps() > 0.1f)
		pMediaType->SetDouble(MF_MT_CORE_DEMUX_FRAMERATE,pAVStream->GetContainerFps());

	switch (pAVStream->GetCodecType())
	{
	case MEDIA_CODEC_VIDEO_HEVC:
	case MEDIA_CODEC_VIDEO_HEVC_ES:
		hr = InitVideoHEVCMediaType(videoDesc,pMediaType.Get(),
			pAVStream->GetCodecType() == MEDIA_CODEC_VIDEO_HEVC_ES);
		break;
	case MEDIA_CODEC_VIDEO_H264:
	case MEDIA_CODEC_VIDEO_H264_ES:
		hr = InitVideoH264MediaType(videoDesc,pMediaType.Get(),
			pAVStream->GetCodecType() == MEDIA_CODEC_VIDEO_H264_ES);
		if (_intelQSDecoder_found && SUCCEEDED(hr))
			hr = InitVideoQuickSyncMediaType(videoDesc,pMediaType.Get(),pAVStream->GetCodecType());
		break;
	case MEDIA_CODEC_VIDEO_MPEG1:
	case MEDIA_CODEC_VIDEO_MPEG2:
		hr = InitVideoMPEG2MediaType(videoDesc,pMediaType.Get());
		if (_intelQSDecoder_found && SUCCEEDED(hr))
			hr = InitVideoQuickSyncMediaType(videoDesc,pMediaType.Get(),pAVStream->GetCodecType());
		break;
	case MEDIA_CODEC_VIDEO_MPEG4:
		hr = InitVideoMPEG4MediaType(videoDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_VIDEO_MJPEG:
		hr = InitVideoMJPEGMediaType(videoDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_VIDEO_MS_MPEG4_V1:
	case MEDIA_CODEC_VIDEO_MS_MPEG4_V2:
	case MEDIA_CODEC_VIDEO_MS_MPEG4_V3:
		hr = InitVideoMSMPEGMediaType(videoDesc,pMediaType.Get(),pAVStream->GetCodecType());
		break;
	case MEDIA_CODEC_VIDEO_H263:
		hr = InitVideoH263MediaType(videoDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_VIDEO_WMV7:
	case MEDIA_CODEC_VIDEO_WMV8:
	case MEDIA_CODEC_VIDEO_WMV9:
		hr = InitVideoWMVMediaType(videoDesc,pMediaType.Get(),
			pAVStream->GetCodecType());
		break;
	case MEDIA_CODEC_VIDEO_VC1:
		hr = InitVideoVC1MediaType(videoDesc,pMediaType.Get());
		break;
	case MEDIA_CODEC_VIDEO_VP8:
	case MEDIA_CODEC_VIDEO_VP9:
	case MEDIA_CODEC_VIDEO_VP10:
		hr = InitVideoVPXMediaType(videoDesc,pMediaType.Get(),pAVStream->GetCodecType());
		break;
	case MEDIA_CODEC_VIDEO_VP6:
	case MEDIA_CODEC_VIDEO_VP6F:
	case MEDIA_CODEC_VIDEO_VP6A:
		hr = InitVideoVP6MediaType(videoDesc,pMediaType.Get(),pAVStream->GetCodecType());
		break;
	case MEDIA_CODEC_VIDEO_RV30:
	case MEDIA_CODEC_VIDEO_RV40:
		hr = InitVideoRealMediaType(videoDesc,pMediaType.Get(),pAVStream->GetCodecType());
		break;
	default:
		hr = MF_E_INVALID_CODEC_MERIT;
	}

	if (SUCCEEDED(hr))
		*ppMediaType = pMediaType.Detach();

	return hr;
}