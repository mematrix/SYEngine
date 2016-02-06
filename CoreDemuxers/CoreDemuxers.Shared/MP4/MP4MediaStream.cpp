#include "MP4MediaStream.h"

struct EsdsIdPair
{
	unsigned EsdsObjId;
	MediaCodecType NativeId;
};
static const EsdsIdPair kCodecMaps[] = {
	{Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_AAC_MPEG2_Main,MEDIA_CODEC_AUDIO_AAC},
	{Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_AAC_MPEG2_Low,MEDIA_CODEC_AUDIO_AAC},
	{Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_AAC_MPEG2_SSR,MEDIA_CODEC_AUDIO_AAC},
	{Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_MPEG2,MEDIA_CODEC_AUDIO_MP2},
	{Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_MPEG3,MEDIA_CODEC_AUDIO_MP3},
	{Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_AC3,MEDIA_CODEC_AUDIO_AC3},
	{Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_EAC3,MEDIA_CODEC_AUDIO_EAC3},
	{Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_DTS,MEDIA_CODEC_AUDIO_DTS},
	{Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_DTS_HD,MEDIA_CODEC_AUDIO_DTS},
	{Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_DTS_HD_Master,MEDIA_CODEC_AUDIO_DTS},
	{Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_DTS_Express,MEDIA_CODEC_AUDIO_DTS}
};

unsigned AVCDecoderConfigurationRecord2AnnexB(unsigned char* src,unsigned char** dst,unsigned* profile,unsigned* level,unsigned* nal_size,unsigned max_annexb_size);

bool MP4MediaStream::ProbeAudio(std::shared_ptr<Aka4Splitter>& demux)
{
	_main_type = MEDIA_MAIN_TYPE_AUDIO;
	if (_info->Codec.StoreType == Aka4Splitter::TrackInfo::CodecInfo::StoreType_SampleEntry)
	{
		if (_info->Codec.CodecId.CodecFcc == ISOM_FCC('NONE') ||
			_info->Codec.CodecId.CodecFcc == ISOM_FCC('raw '))
			_codec_type = MEDIA_CODEC_PCM_8BIT;
		else if (_info->Codec.CodecId.CodecFcc == ISOM_FCC('twos'))
			_codec_type = MEDIA_CODEC_PCM_16BIT_BE;
		else if (_info->Codec.CodecId.CodecFcc == ISOM_FCC('sowt'))
			_codec_type = MEDIA_CODEC_PCM_16BIT_LE;
		else
			return false;
		if (_info->Audio.BitDepth == 16)
			_codec_type = MEDIA_CODEC_PCM_16BIT_BE;
		else if (_info->Audio.BitDepth == 24)
			_codec_type = (_codec_type == MEDIA_CODEC_PCM_16BIT_BE ? 
				MEDIA_CODEC_PCM_24BIT_BE:MEDIA_CODEC_PCM_24BIT_LE);
		return InitAudioCommon();
	}

	if (_info->Codec.StoreType == Aka4Splitter::TrackInfo::CodecInfo::StoreType_Default)
	{
		//ALAC, AC3 etc.
		if (_info->Codec.CodecId.CodecFcc == ISOM_FCC('alac'))
			return InitAudioALAC(_info->Codec.Userdata,_info->Codec.UserdataSize);
		else if (_info->Codec.CodecId.CodecFcc == ISOM_FCC('ac-3') ||
			_info->Codec.CodecId.CodecFcc == ISOM_FCC('sac3') ||
			_info->Codec.CodecId.CodecFcc == ISOM_FCC('ec-3'))
			return InitAudioAC3(_info->Codec.CodecId.CodecFcc);
		else
			return false;
	}else if (_info->Codec.StoreType == Aka4Splitter::TrackInfo::CodecInfo::StoreType_Esds ||
		_info->Codec.StoreType == Aka4Splitter::TrackInfo::CodecInfo::StoreType_SampleEntryWithEsds) {
		//AAC, MP3 etc.
		switch (_info->Codec.CodecId.EsdsObjType)
		{
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_AAC:
			return InitAudioAAC(_info->Codec.Userdata,_info->Codec.UserdataSize);
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_AAC_MPEG2_Main:
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_AAC_MPEG2_Low:
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_AAC_MPEG2_SSR:
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_MPEG2:
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_MPEG3:
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_AC3:
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_EAC3:
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_DTS:
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_DTS_HD:
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_DTS_HD_Master:
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_DTS_Express:
			_codec_type = MEDIA_CODEC_UNKNOWN;
			for (auto id : kCodecMaps)
			{
				if (id.EsdsObjId == _info->Codec.CodecId.EsdsObjType)
				{
					_codec_type = id.NativeId;
					break;
				}
			}
			if (_codec_type == MEDIA_CODEC_UNKNOWN)
				return false;
			return InitAudioCommon();
		case Aka4Splitter::EsdsObjTypes::EsdsObjType_Audio_Vorbis:
			return InitAudioVorbis(_info->Codec.Userdata,_info->Codec.UserdataSize);
		default:
			return false;
		}
	}else{
		return false;
	}

	return true;
}

bool MP4MediaStream::ProbeVideo(std::shared_ptr<Aka4Splitter>& demux,bool avc1)
{
	_main_type = MEDIA_MAIN_TYPE_VIDEO;
	if (_info->Codec.StoreType == Aka4Splitter::TrackInfo::CodecInfo::StoreType_SampleEntry)
		return false;

	if (_info->Codec.StoreType == Aka4Splitter::TrackInfo::CodecInfo::StoreType_Default)
	{
		//H264, HEVC etc...
		if (_info->Codec.UserdataSize == 0)
			return false;
		if (_info->Codec.CodecId.CodecFcc == ISOM_FCC('avc1') ||
			_info->Codec.CodecId.CodecFcc == ISOM_FCC('AVC1'))
			return avc1 ? InitVideoAVC1():InitVideoH264(_info->Codec.Userdata,_info->Codec.UserdataSize);
		else if (_info->Codec.CodecId.CodecFcc == ISOM_FCC('hvc1') ||
			_info->Codec.CodecId.CodecFcc == ISOM_FCC('hev1'))
			return InitVideoHEVC(_info->Codec.Userdata,_info->Codec.UserdataSize);
	}else if (_info->Codec.StoreType == Aka4Splitter::TrackInfo::CodecInfo::StoreType_Esds)
	{
		//MPEG4, MPEG2, MPEG1 etc...
		if (_info->Codec.CodecId.EsdsObjType != Aka4Splitter::EsdsObjTypes::EsdsObjType_Video_MPEG4)
			return false;
		return InitVideoMPEG4(_info->Codec.Userdata,_info->Codec.UserdataSize);
	}else{
		return false;
	}

	return true;
}

bool MP4MediaStream::ProbeText(std::shared_ptr<Aka4Splitter>& demux,bool non_tx3g)
{
	if (!non_tx3g)
		_movsub2text = true;

	_main_type = MEDIA_MAIN_TYPE_SUBTITLE;
	_codec_type = MEDIA_CODEC_SUBTITLE_TEXT;

	memset(&_subtitle_info,0,sizeof(_subtitle_info));
	_subtitle_info.type = MediaSubtitleTextType::MEDIA_SUBTITLE_TEXT_UTF8;
	_subtitle_info.head_info_size = _info->Codec.UserdataSize;
	_subtitle_info.head_info = _info->Codec.Userdata;
	return true;
}

bool MP4MediaStream::InitAudioCommon()
{
	if (_info->Audio.Channels == 0 || _info->Audio.Channels > 8)
		return false;
	if (_info->Audio.SampleRate == 0)
		return false;

	CommonAudioCore common = {};
	common.type = _info->Codec.CodecId.CodecFcc;
	common.desc.nch = _info->Audio.Channels;
	common.desc.srate = _info->Audio.SampleRate;
	common.desc.bits = _info->Audio.BitDepth;

	common.extradata = _info->Codec.Userdata;
	common.extradata_size = _info->Codec.UserdataSize;

	_audio_desc = std::make_shared<CommonAudioDescription>(common);
	return true;
}

bool MP4MediaStream::InitAudioAAC(unsigned char* data,unsigned size)
{
	if (size < 2)
		return false;

	std::shared_ptr<IAudioDescription> aac = 
		std::make_shared<ADTSAudioDescription>((unsigned*)data,true,size);

	AudioBasicDescription basic = {};
	aac->GetAudioDescription(&basic);
	if (basic.nch == 0)
		return false;

	if (basic.srate == 0)
	{
		basic.srate = _info->Audio.SampleRate;
		aac->ExternalUpdateAudioDescription(&basic);
	}

	_audio_desc = aac;
	_codec_type = MEDIA_CODEC_AUDIO_AAC;
	return true;
}

bool MP4MediaStream::InitAudioALAC(unsigned char* data,unsigned size)
{
	if (size < 28)
		return false;

	std::shared_ptr<IAudioDescription> alac = 
		std::make_shared<ALACAudioDescription>(data + 4,size - 4);

	AudioBasicDescription basic = {};
	alac->GetAudioDescription(&basic);
	if (basic.nch == 0)
		return false;

	_audio_desc = alac;
	_codec_type = MEDIA_CODEC_AUDIO_ALAC;
	return true;
}

bool MP4MediaStream::InitAudioVorbis(unsigned char* data,unsigned size)
{
	if (size < 96)
		return false;

	auto p2 = (unsigned char*)malloc(size * 2);
	if (p2 == nullptr)
		return false;
	auto p = data;
	auto p3 = p2 + 3;
	memset(p2,0,size * 2);
	*p2 = 2;
	for (unsigned i = 0;i < 3;i++)
	{
		int16_t sz = *(int16_t*)p;
		sz = ISOM_SWAP16(sz);
		if ((unsigned)sz > (size - (p + 2 - data)))
			break;

		if (i != 2)
			if (sz > 255)
				break;

		memcpy(p3,p + 2,sz);
		p3 += sz;

		if (i != 2)
			p2[i + 1] = (int8_t)sz;

		p = p + 2 + sz;
		if (p >= (data + size))
			break;
	}
	if (p2[1] > 0 && p2[2] > 0)
	{
		std::shared_ptr<IAudioDescription> ogg = 
			std::make_shared<VorbisAudioDescription>(p2,p3 - p2);

		AudioBasicDescription basic = {};
		ogg->GetAudioDescription(&basic);
		if (basic.nch > 0)
		{
			_audio_desc = ogg;
			_codec_type = MEDIA_CODEC_AUDIO_VORBIS;
			return true;
		}
	}
	free(p2);
	return false;
}

bool MP4MediaStream::InitAudioAC3(unsigned fcc)
{
	if (fcc == ISOM_FCC('ac-3') || fcc == ISOM_FCC('sac3'))
		_codec_type = MEDIA_CODEC_AUDIO_AC3;
	else if (fcc == ISOM_FCC('ec-3'))
		_codec_type = MEDIA_CODEC_AUDIO_EAC3;
	else
		return false;
	return InitAudioCommon();
}

bool MP4MediaStream::InitVideoAVC1()
{
	if (_info->Codec.UserdataSize < 8)
		return false;
	std::shared_ptr<IVideoDescription> h264 = std::make_shared<AVC1VideoDescription>
		(_info->Codec.Userdata,_info->Codec.UserdataSize,_info->Video.Width,_info->Video.Height);

	H264_PROFILE_SPEC profile = {};
	h264->GetProfile(&profile);
	if (profile.profile == 0)
		return false;

	if (profile.variable_framerate && _info->Video.FrameRate > 0.1f)
	{
		VideoBasicDescription temp = {};
		h264->GetVideoDescription(&temp);
		temp.frame_rate.den = 10000000;
		temp.frame_rate.num = (int)(double(_info->Video.FrameRate) * 10000000.0);
		h264->ExternalUpdateVideoDescription(&temp);
	}

	_video_desc = h264;
	_codec_type = MEDIA_CODEC_VIDEO_H264;
	return true;
}

bool MP4MediaStream::InitVideoH264(unsigned char* avcc,unsigned size)
{
	if (size < 10)
		return false;

	unsigned char* pb = nullptr;
	unsigned sz = AVCDecoderConfigurationRecord2AnnexB(avcc,&pb,nullptr,nullptr,&_nal_size,2048);
	if (pb == nullptr)
		return false;

	std::shared_ptr<IVideoDescription> h264 = std::make_shared<X264VideoDescription>(pb,sz);
	free(pb);

	H264_PROFILE_SPEC profile = {};
	h264->GetProfile(&profile);
	if (profile.profile == 0)
		return false;

	if (profile.variable_framerate && _info->Video.FrameRate > 0.1f)
	{
		VideoBasicDescription temp = {};
		h264->GetVideoDescription(&temp);
		temp.frame_rate.den = 10000000;
		temp.frame_rate.num = (int)(double(_info->Video.FrameRate) * 10000000.0);
		h264->ExternalUpdateVideoDescription(&temp);
	}

	_video_desc = h264;
	_codec_type = MEDIA_CODEC_VIDEO_H264;
	return true;
}

bool MP4MediaStream::InitVideoMPEG4(unsigned char* data,unsigned size)
{
	if (data == nullptr || size < 8)
		return false;

	unsigned offset = FindNextMPEG4StartCode(data,size);
	if (offset < 3)
		return false;
	offset -= 3;

	std::shared_ptr<IVideoDescription> m4p2 = 
		std::make_shared<MPEG4VideoDescription>(data + offset,size - offset);

	if (m4p2->GetExtradataSize() == 0)
		return false;

	_video_desc = m4p2;
	_codec_type = MEDIA_CODEC_VIDEO_MPEG4;
	return true;
}