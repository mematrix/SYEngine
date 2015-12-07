#include "HDMediaSource.h"
#include <AAC\AACAudioDescription.h>
#include <MP3\MPEGAudioDescription.h>
#include <OGG\VorbisAudioDescription.h>
#include <FLAC\FLACAudioDescription.h>
#include <LPCM\BDAudioDescription.h>

static PWAVEFORMATEX AllocAMRNBWaveFormatEx(IMFMediaType* pMediaType)
{
	PWAVEFORMATEX pwfx = (PWAVEFORMATEX)CoTaskMemAlloc(64);
	if (pwfx == nullptr)
		return nullptr;
	pwfx->wFormatTag = WAVE_FORMAT_AMR_NB;
	pwfx->nChannels = MFGetAttributeUINT32(pMediaType,MF_MT_AUDIO_NUM_CHANNELS,0);
	pwfx->nSamplesPerSec = MFGetAttributeUINT32(pMediaType,MF_MT_AUDIO_SAMPLES_PER_SECOND,0);
	pwfx->wBitsPerSample = MFGetAttributeUINT32(pMediaType,MF_MT_AUDIO_BITS_PER_SAMPLE,8);
	pwfx->nAvgBytesPerSec = MFGetAttributeUINT32(pMediaType,MF_MT_AUDIO_AVG_BYTES_PER_SECOND,0) * 2 * 10;
	pwfx->nBlockAlign = pwfx->nChannels * (pwfx->wBitsPerSample / 8);

	static const unsigned char temp[] = {0x50,0x4D,0x46,0x46,0x00,0xFF,0x81,0x00,0x01};
	memcpy((char*)pwfx + 18,temp,_countof(temp));
	pwfx->cbSize = _countof(temp);
	return pwfx;
}

HRESULT HDMediaSource::InitAudioAACMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,bool adts,bool skip_err)
{
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_AAC);
	pMediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT,1);
	pMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16);

	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);
	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,1);
	pMediaType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES,TRUE);

	//use default AAC profile.
	pMediaType->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION,0xFE);

	AutoComMem<unsigned char> pUserData(128);

	AAC_PROFILE_SPEC aac_profile = {};
	aac_profile.profile = -1;

	pDesc->GetProfile(&aac_profile);
	if (aac_profile.profile == -1 && !adts)
		return MF_E_INVALID_PROFILE;

	AudioBasicDescription basicDesc = {};
	pDesc->GetAudioDescription(&basicDesc);
#ifndef _DESKTOP_APP
	if (basicDesc.nch > 2 && (!skip_err))
		return MF_E_INVALID_PROFILE;
#else
	if (basicDesc.nch > 2 && !IsWindows8() && (!skip_err))
		return MF_E_INVALID_PROFILE;
#endif
	
	if (basicDesc.srate > 48000 && (!skip_err))
		return MF_E_INVALID_PROFILE;

	bool is_adts = pDesc->GetExtradataSize() == 0;
	if (!is_adts && !adts) //raw AAC
	{
		//AAC-LC only.
		if (aac_profile.profile != 1 && (!skip_err)) //0 = Main,1 = LC,2 = SRS
			return MF_E_INVALID_PROFILE;
		if (aac_profile.profile != 1 && skip_err)
			if (aac_profile.profile != 0 && aac_profile.profile != 3) //Only Main and LTP.
				return MF_E_INVALID_PROFILE;

		pMediaType->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE,0);
		if (pDesc->GetExtradataSize() > 100)
			return MF_E_UNEXPECTED;

		auto pAudioSpecificConfig = pUserData.Get();
		pAudioSpecificConfig[0] = 0x00;
		pAudioSpecificConfig[2] = 0xFE;

		pDesc->GetExtradata(&pAudioSpecificConfig[12]);

		//编码器是不认HE-AAC的，但是现在是HE-LC CORE模式
		//所以把audioObjectType改为2(AAC-LC)，让编码器认
		if (aac_profile.he_with_lc_core && !skip_err)
		{
			//audioObjectType 5bits (1 = Main,2 = LC,3 = SSR,4 = LTP,5 = SBR)
			//table: http://fossies.org/linux/MediaInfo_CLI/MediaInfoLib/Source/MediaInfo/Audio/File_Aac_Main.cpp
			unsigned char audioObjectType = pAudioSpecificConfig[12] & 0x07; //保留后3bit
			pAudioSpecificConfig[12] = audioObjectType | 0x10; //修改前5bits
		}
		
		pMediaType->SetBlob(MF_MT_USER_DATA,
			pUserData.Get(),14);
	}else{
		AudioBasicDescription basicDesc = {};
		pDesc->GetAudioDescription(&basicDesc);

		pMediaType->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE,1); //ADTS
		pMediaType->SetUINT32(MF_PD_AUDIO_ISVARIABLEBITRATE,TRUE);

		auto pAudioSpecificConfig = pUserData.Get();
		pAudioSpecificConfig[0] = 0x01;
		pAudioSpecificConfig[2] = 0xFE;

		if (aac_profile.profile != -1)
		{
			aac_profile.profile += 1;
			int adtsConfig0 = (aac_profile.profile << 3) | ((aac_profile.rate_index & 0x0E) >> 1);
			int adtsConfig1 = ((aac_profile.rate_index & 0x01) << 7) | (basicDesc.nch << 3);

			pAudioSpecificConfig[12] = (BYTE)adtsConfig0;
			pAudioSpecificConfig[13] = (BYTE)adtsConfig1;
		}

		if (pDesc->GetExtradataSize() == 2)
			pDesc->GetExtradata(&pAudioSpecificConfig[12]);

		if (pAudioSpecificConfig[12] == 0 && pAudioSpecificConfig[13] == 0)
		{
			pAudioSpecificConfig[12] = 0x11;
			pAudioSpecificConfig[13] = 0x90;
		}

		pMediaType->SetBlob(MF_MT_USER_DATA,
			pUserData.Get(),14);
	}

	return S_OK;
}

HRESULT HDMediaSource::InitAudioFAADMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,bool adts)
{
	HRESULT hr = InitAudioAACMediaType(pDesc,pMediaType,adts,true);
	if (FAILED(hr))
		return hr;

	BYTE buffer[128] = {};
	UINT32 temp = 0;
	if (SUCCEEDED(pMediaType->GetBlob(MF_MT_USER_DATA,buffer,ARRAYSIZE(buffer),&temp)))
	{
		if (temp >= 14)
		{
			temp -= 12;
			pMediaType->SetBlob(MF_MT_USER_DATA,&buffer[12],temp);
		}
	}

	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_FAAD);
	return S_OK;
}

HRESULT HDMediaSource::InitAudioMP3MediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type)
{
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_MP3);
	pMediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT,1);
	pMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16);

	if (codec_type == MEDIA_CODEC_AUDIO_MP3)
	{
		MPEG_AUDIO_PROFILE_SPEC profile = {};
		pDesc->GetProfile(&profile);

		if (profile.bitrate != 0)
			pMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,profile.bitrate * 125);
	}else{
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_MPEG);
		pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);
		pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,1);
		pMediaType->DeleteItem(MF_MT_AUDIO_BITS_PER_SAMPLE);
	}

	if (_pMediaParser->GetMimeType())
	{
		if (strstr(_pMediaParser->GetMimeType(),"matroska") != nullptr &&
			codec_type != MEDIA_CODEC_AUDIO_MP3)
			pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_MPA_FRAMED);
	}
	return S_OK;
}

HRESULT HDMediaSource::InitAudioAC3MediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,bool ec3)
{
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_Dolby_AC3);
	if (ec3)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_Dolby_DDPlus);

	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);
	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,1);
	pMediaType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES,TRUE);
	if (!ec3)
	{
		if (GlobalOptionGetBOOL(kCoreUseFloat2PcmAC3DTS))
		{
			pMediaType->SetUINT32(MF_MT_MY_FORCE_FLOAT32PCM16,TRUE);
			pMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16);
		}
	}

	return S_OK;
}

HRESULT HDMediaSource::InitAudioDTSMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType)
{
	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_DTS);
	if (GlobalOptionGetBOOL(kCoreUseFloat2PcmAC3DTS))
	{
		pMediaType->SetUINT32(MF_MT_MY_FORCE_FLOAT32PCM16,TRUE);
		pMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16);
	}
	return S_OK;
}

HRESULT HDMediaSource::InitAudioTrueHDMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType)
{
	InitAudioOPUSMediaType(pDesc,pMediaType);
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_Dolby_TrueHD);
	return S_OK;
}

HRESULT HDMediaSource::InitAudioWMAMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type)
{
	pMediaType->DeleteItem(MF_MT_AUDIO_AVG_BYTES_PER_SECOND);
	pMediaType->SetUINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX,TRUE);

	AudioBasicDescription desc = {};
	pDesc->GetAudioDescription(&desc);
	if (desc.wav_avg_bytes != 0)
		pMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,desc.wav_avg_bytes);
	if (desc.wav_block_align != 0)
		pMediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT,desc.wav_block_align);

	switch (codec_type)
	{
	case MEDIA_CODEC_AUDIO_WMA7:
		GUID subType;
		if (SUCCEEDED(CLSIDFromString(L"{00000160-0000-0010-8000-00AA00389B71}",&subType)))
			pMediaType->SetGUID(MF_MT_SUBTYPE,subType);
		break;
	case MEDIA_CODEC_AUDIO_WMA8:
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_WMAudioV8);
		break;
	case MEDIA_CODEC_AUDIO_WMA9_PRO:
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_WMAudioV9);
		break;
	case MEDIA_CODEC_AUDIO_WMA9_LOSSLESS:
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_WMAudio_Lossless);
		break;
	}

	AutoComMem<unsigned char> pUserData(pDesc->GetExtradataSize() + 1);
	if (!pDesc->GetExtradata(pUserData.Get()))
		return MF_E_INVALID_PROFILE;

	return pMediaType->SetBlob(MF_MT_USER_DATA,
		pUserData.Get(),pDesc->GetExtradataSize());
}

HRESULT HDMediaSource::InitAudioALACMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType)
{
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_ALAC);
	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);

	if (pDesc->GetExtradataSize() != 24)
		return MF_E_INVALID_PROFILE;

	AutoComMem<unsigned char> pUserData(24);
	if (!pDesc->GetExtradata(pUserData.Get()))
		return MF_E_INVALID_PROFILE;

	return pMediaType->SetBlob(MF_MT_USER_DATA,
		pUserData.Get(),pDesc->GetExtradataSize());
}

HRESULT HDMediaSource::InitAudioFLACMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType)
{
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_FLAC_FRAMED);
	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);
	if (GlobalOptionGetBOOL(kCoreUseWin10FLACDecoder) && pDesc->GetExtradataSize() > 0)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_FLAC);
	if (GlobalOptionGetBOOL(kCoreUseDShowFLACDecoder))
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_DS_FLAC_Framed);

	FLAC_PROFILE_SPEC profile = {};
	pDesc->GetProfile(&profile);
	if (profile.max_block_size > 0)
		pMediaType->SetUINT32(MF_MT_FLAC_MAX_BLOCK_SIZE,profile.max_block_size);

	AutoComMem<unsigned char> pUserData(128);
	if (pDesc->GetExtradata(pUserData.Get()))
		pMediaType->SetBlob(MF_MT_USER_DATA,pUserData.Get(),pDesc->GetExtradataSize());

	return pMediaType->SetUINT32(MF_MT_MY_DECODE_BUF_TIME_MS,500);
}

HRESULT HDMediaSource::InitAudioOGGMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType)
{
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_Vorbis_FRAMED);
	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);

	VORBIS_PROFILE_SPEC profile = {};
	if (!pDesc->GetProfile(&profile) || 
		pDesc->GetExtradataSize() == 0)
		return MF_E_INVALID_PROFILE;
	pMediaType->SetUINT32(MF_MT_VORBIS_MAX_BLOCK_SIZE,profile.block_size1);

	AutoComMem<unsigned char> pUserData(pDesc->GetExtradataSize() + 1);
	if (!pDesc->GetExtradata(pUserData.Get()))
		return MF_E_INVALID_PROFILE;

	pMediaType->SetUINT32(MF_MT_MY_FORCE_FLOAT32PCM16,TRUE);
	return pMediaType->SetBlob(MF_MT_USER_DATA,
		pUserData.Get(),pDesc->GetExtradataSize());
}

HRESULT HDMediaSource::InitAudioOPUSMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType)
{
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_OPUS_FRAMED);
	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);
	pMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16);
	if (MFGetAttributeUINT32(pMediaType,MF_MT_AUDIO_SAMPLES_PER_SECOND,0) == 44100)
		pMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,48000);

	AutoComMem<unsigned char> pUserData(pDesc->GetExtradataSize() + 1);
	if (pDesc->GetExtradata(pUserData.Get()))
		pMediaType->SetBlob(MF_MT_USER_DATA,pUserData.Get(),pDesc->GetExtradataSize());

	return pMediaType->SetUINT32(MF_MT_MY_FORCE_FLOAT32PCM16,TRUE);
}

HRESULT HDMediaSource::InitAudioTTAMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType)
{
	InitAudioOPUSMediaType(pDesc,pMediaType);
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_TTA1);
	return S_OK;
}

HRESULT HDMediaSource::InitAudioWV4MediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType)
{
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_WV4);
	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);
	return S_OK;
}

HRESULT HDMediaSource::InitAudioBDMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType)
{
	AudioBasicDescription basicDesc = {};
	pDesc->GetAudioDescription(&basicDesc);

	if (basicDesc.bits == 0)
		return MF_E_UNSUPPORTED_FORMAT;

	WAVEFORMATEX wfx = {};
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = basicDesc.nch;
	wfx.nSamplesPerSec = basicDesc.srate;
	wfx.wBitsPerSample = basicDesc.bits;
	wfx.nBlockAlign = basicDesc.nch * (basicDesc.bits / 8);
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

	HRESULT hr = MFInitMediaTypeFromWaveFormatEx(pMediaType,&wfx,sizeof WAVEFORMATEX);
	if (FAILED(hr))
		return hr;

	if (basicDesc.ch_layout != 0)
		pMediaType->SetUINT32(MF_MT_AUDIO_CHANNEL_MASK,basicDesc.ch_layout);

	return pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_LPCM_BD);
}

HRESULT HDMediaSource::InitAudioPCMMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type)
{
	HRESULT hr = InitAudioBDMediaType(pDesc,pMediaType);
	if (FAILED(hr))
		return hr;

	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_PCM);

	if (codec_type == MEDIA_CODEC_PCM_FLOAT)
		return pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_Float);

	if (codec_type == MEDIA_CODEC_PCM_16BIT_BE ||
		codec_type == MEDIA_CODEC_PCM_24BIT_BE ||
		codec_type == MEDIA_CODEC_PCM_32BIT_BE ||
		codec_type == MEDIA_CODEC_PCM_SINT_BE)
		pMediaType->SetUINT32(MF_MT_MY_PCM_BE_FLAG,TRUE);

#if defined(_DEBUG) && defined(_DESKTOP_APP)
	if (MFGetAttributeUINT32(pMediaType,MF_MT_MY_PCM_BE_FLAG,0) != 0)
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_PCM_BE);
#endif

	return S_OK;
}

HRESULT HDMediaSource::InitAudioAMRMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type)
{
	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT,TRUE);
	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE,1);
	pMediaType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES,TRUE);

	AudioBasicDescription desc = {};
	pDesc->GetAudioDescription(&desc);
	if (desc.wav_avg_bytes != 0)
	{
		pMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,desc.wav_avg_bytes);
		pMediaType->SetUINT32(MF_MT_AVG_BITRATE,desc.wav_avg_bytes * 8);
	}
	//pMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS,desc.nch + 1);

	auto pwfx = AllocAMRNBWaveFormatEx(pMediaType);
	if (pwfx)
	{
		pMediaType->SetBlob(MF_MT_USER_DATA,(UINT8*)pwfx,18 + pwfx->cbSize);
		CoTaskMemFree(pwfx);
	}

	switch (codec_type)
	{
	case MEDIA_CODEC_AUDIO_AMR_NB:
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_AMR_NB);
		break;
	case MEDIA_CODEC_AUDIO_AMR_WB:
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_AMR_WB);
		break;
	case MEDIA_CODEC_AUDIO_AMR_WBP:
		pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_AMR_WP);
		break;
	}

	AutoComMem<unsigned char> pUserData(pDesc->GetExtradataSize() + 1);
	if (pDesc->GetExtradata(pUserData.Get()))
		pMediaType->SetBlob(MF_MT_USER_DATA,pUserData.Get(),pDesc->GetExtradataSize());

	return S_OK;
}

HRESULT HDMediaSource::InitAudioRealCookMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType)
{
	InitAudioOPUSMediaType(pDesc,pMediaType);
	AudioBasicDescription desc = {};
	pDesc->GetAudioDescription(&desc);

	pMediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT,desc.wav_block_align);
	pMediaType->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_Real_Cook);
	return S_OK;
}