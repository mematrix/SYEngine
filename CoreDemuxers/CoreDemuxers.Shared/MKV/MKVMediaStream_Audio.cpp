#include "MKVMediaStream.h"
#include "VFWHelp.h"
#include <MiscUtils.h>
#include <Utils/stagefright/ABitReader.h>

#define AC3_SYNC_WORD_LE 0x770B
#define AC3_SYNC_WORD_BE 0x0B77

#define DTS_SYNC_LE_14BIT 0xE8001FFF
#define DTS_SYNC_BE_14BIT 0x00E8FF1F
#define DTS_SYNC_LE_RAW 0x80017FFE
#define DTS_SYNC_BE_RAW 0x0180FE7F

#ifndef _DONT_USE_LIBDCA
extern "C" {
void* dca_init(unsigned mm_accel);
int dca_syncinfo(void* state,unsigned char* buf,int* flags,int* srate,int* brate,int* frameLength);
void dca_free(void* state);
}
int GetDTSChannelsFromFlags(int flags);
#endif

#pragma pack(1)
struct OpusHead
{
	char flag_string[8]; //OpusHead
	unsigned char version; //default to 1
	unsigned char nch;
	unsigned short pre_skip; //default to 0
	unsigned int rate;
};
#pragma pack()

#pragma pack(1)
struct ACM_WAVEFORMATEX
{
	unsigned short wFormatTag;
	unsigned short nChannels;
	unsigned nSamplesPerSec;
	unsigned nAvgBytesPerSec;
	unsigned short nBlockAlign;
	unsigned short wBitsPerSample;
	unsigned short cbSize;
};
#pragma pack()

static int MKVAACGetAudioObjectType(const char* codecId)
{
	if (codecId == nullptr)
		return -1;
	if (strlen(codecId) == 0)
		return -1;
	if (*codecId != 'A')
		return -1;

	if (strstr(codecId,"AAC") == nullptr)
		return -1;
	if (strcmp(codecId,"A_AAC") == 0)
		return 2; //LC
	
	if (strstr(codecId,"MAIN") != nullptr)
		return 1;
	else if (strstr(codecId,"SBR") != nullptr && strstr(codecId,"LC") != nullptr)
		return 2; //HE AAC with LC-Core
	else if (strstr(codecId,"SBR") != nullptr)
		return 5; //SBR with LC-Core
	else if (strstr(codecId,"SSR") != nullptr)
		return 3;
	else if (strstr(codecId,"LTP") != nullptr)
		return 4;
	else if (strstr(codecId,"LC") != nullptr)
		return 2;

	return -1;
}

static bool ConvertMKVAudioInfoToCommonInfo(MKVParser::MKVTrackInfo::AudioInfo* audio,CommonAudioCore* core)
{
	if (audio->Channels == 0)
		return false;

	core->desc.nch = audio->Channels;
	core->desc.srate = audio->SampleRate;
	core->desc.bits = audio->BitDepth;
	return true;
}

static bool InitAACLCTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IAudioDescription>& desc)
{
	if (info.Codec.CodecPrivateSize < 2)
		return false;

	std::shared_ptr<IAudioDescription> aac = 
		std::make_shared<ADTSAudioDescription>((unsigned*)info.Codec.CodecPrivate,true,info.Codec.CodecPrivateSize);

	AudioBasicDescription basic = {};
	aac->GetAudioDescription(&basic);
	if (basic.nch == 0)
		return false;

	if (basic.srate == 0)
	{
		basic.srate = info.Audio.SampleRate;
		aac->ExternalUpdateAudioDescription(&basic);
	}

	desc = aac;
	return true;
}

static bool InitADTSTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IAudioDescription>& desc,unsigned char* adtsHeader)
{
	std::shared_ptr<IAudioDescription> aac = 
		std::make_shared<ADTSAudioDescription>((unsigned*)adtsHeader);

	AudioBasicDescription basic = {};
	aac->GetAudioDescription(&basic);
	if (basic.nch == 0)
		return false;

	if (basic.srate == 0)
	{
		basic.srate = info.Audio.SampleRate;
		aac->ExternalUpdateAudioDescription(&basic);
	}

	desc = aac;
	return true;
}

static bool InitAACTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IAudioDescription>& desc,unsigned audioObjectType)
{
	if (info.Audio.Channels == 0 ||
		info.Audio.Channels > 8)
		return false;
	if (info.Audio.SampleRate < 8000)
		return false;

	unsigned short config = MakeAACAudioSpecificConfig(audioObjectType,info.Audio.SampleRate,info.Audio.Channels);
	if (config == 0)
		return false;

	std::shared_ptr<IAudioDescription> aac = 
		std::make_shared<ADTSAudioDescription>((unsigned*)&config,true);

	AudioBasicDescription basic = {};
	aac->GetAudioDescription(&basic);
	if (basic.nch == 0)
		return false;

	desc = aac;
	return true;
}

static bool InitFLACTrackUseFrame(MKVParser::MKVTrackInfo& info,std::shared_ptr<IAudioDescription>& desc,std::shared_ptr<MKVParser::MKVFileParser>& parser)
{
	MKVParser::MKVPacket pkt = {};
	if (parser->ReadSinglePacket(&pkt,info.Number,false) != PARSER_MKV_OK)
		return false;
	if (pkt.PacketSize < 8 || pkt.PacketData == nullptr)
		return false;
	if (pkt.PacketData[0] != 0xFF)
		return false;

	stagefright::ABitReader br(pkt.PacketData,pkt.PacketSize);
	br.skipBits(16);
	unsigned tSamples = br.getBits(4);
	unsigned tSampleRate = br.getBits(4);
	unsigned tNch = br.getBits(4);
	unsigned tBits = br.getBits(3);

	if (tSampleRate == 0 || tBits == 0) //get from StreamInfo
		return false;

	if (tSampleRate > 11)
		return false; //sample_rate_hint, get from end.
	if (tBits == 3 || tBits == 7)
		return false;

	if (tNch == 0)
		return false;

	static const unsigned ts[] = {1,2,3,4,5,6,7,8,9,10,11};
	static const unsigned s[] = {88200,176400,192000,8000,16000,22050,24000,32000,44100,48000,96000};

	static const unsigned bs[] = {1,2,4,5,6};
	static const unsigned b[] = {8,12,16,20,24};

	for (unsigned i = 0;i < MKV_ARRAY_COUNT(ts);i++)
	{
		if (ts[i] == tSampleRate)
			tSampleRate = s[i];
	}
	for (unsigned i = 0;i < MKV_ARRAY_COUNT(bs);i++)
	{
		if (bs[i] == tBits)
			tBits = b[i];
	}

	if (tBits == 12 || tBits == 20)
		return false; //only 8,16,24.

	CommonAudioCore core = {};
	core.desc.compressed = true;
	core.desc.nch = tNch + 1;
	core.desc.srate = tSampleRate;
	core.desc.bits = tBits;
	desc = std::make_shared<CommonAudioDescription>(core);

	return true;
}

static bool InitFLACTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IAudioDescription>& desc,std::shared_ptr<MKVParser::MKVFileParser>& parser)
{
	if (info.Codec.CodecPrivateSize == 0 &&
		info.Audio.Channels == 0 || info.Audio.BitDepth < 8 || info.Audio.SampleRate < 8000) {
		return InitFLACTrackUseFrame(info,desc,parser);
	}

	std::shared_ptr<IAudioDescription> flac;

	if (info.Codec.CodecPrivateSize > 8) {
		flac = std::make_shared<FLACAudioDescription>(info.Codec.CodecPrivate,info.Codec.CodecPrivateSize);
	}else{
		unsigned char flacHead[42] = {};
		flacHead[0] = 0x66;
		flacHead[1] = 0x4C;
		flacHead[2] = 0x61;
		flacHead[3] = 0x43;
		flacHead[4] = 0x80;
		flacHead[7] = 0x22;
		flacHead[18] = ((unsigned char)(MKV_SWAP16(info.Audio.SampleRate)) >> 4);
		unsigned char temp0 = ((unsigned char)(MKV_SWAP16(info.Audio.SampleRate)) & 0x0F) << 4;
		unsigned char temp1 = (unsigned char)(MKV_SWAP16(info.Audio.SampleRate) >> 8) >> 4;
		flacHead[19] = temp0 | temp1;

		unsigned nch = info.Audio.Channels - 1;
		unsigned bits = info.Audio.BitDepth - 1;
		temp0 = (((unsigned char)nch) << 1) & 0x0E;
		temp1 = (((unsigned char)bits) << 3) >> 7;
		temp0 |= temp1;
		temp1 = (((unsigned char)bits) & 0x0F) << 4;

		flacHead[20] = temp0;
		flacHead[21] = temp1;
		flac = std::make_shared<FLACAudioDescription>(flacHead,42);
	}

	AudioBasicDescription basic = {};
	flac->GetAudioDescription(&basic);
	if (basic.nch == 0)
		return false;

	desc = flac;
	return true;
}

static bool InitVorbisTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IAudioDescription>& desc)
{
	if (info.Codec.CodecPrivateSize < 80)
		return false;

	unsigned h1size = 0,h2size = 0,h3size = 0;
	ParseMKVOggXiphHeader(info.Codec.CodecPrivate,info.Codec.CodecPrivateSize,&h1size,&h2size,&h3size);
	if (h1size == 0 || h2size == 0 || h3size == 0)
		return false;

	std::shared_ptr<IAudioDescription> ogg = 
		std::make_shared<VorbisAudioDescription>(info.Codec.CodecPrivate,info.Codec.CodecPrivateSize);

	AudioBasicDescription basic = {};
	ogg->GetAudioDescription(&basic);
	if (basic.nch == 0)
		return false;

	desc = ogg;
	return true;
}

static bool InitALACTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IAudioDescription>& desc,double* frameDuration)
{
	*frameDuration = 0.0;
	if (info.Codec.CodecPrivateSize < 24)
		return false;

	std::shared_ptr<IAudioDescription> alac = 
		std::make_shared<ALACAudioDescription>(info.Codec.CodecPrivate,info.Codec.CodecPrivateSize);

	AudioBasicDescription basic = {};
	alac->GetAudioDescription(&basic);
	if (basic.nch == 0)
		return false;

	unsigned frameLength = MKV_SWAP32(*(unsigned*)info.Codec.CodecPrivate);
	if (frameLength > 64 && basic.srate != 0)
		*frameDuration = (double)frameLength / (double)basic.srate;

	desc = alac;
	return true;
}

static bool InitFirstPacketTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IAudioDescription>& desc,std::shared_ptr<MKVParser::MKVFileParser>& parser,MatroskaCodecIds codecId)
{
	if (info.Codec.CodecPrivateSize > 0)
		return false;

	MKVParser::MKVPacket pkt = {};
	if (parser->ReadSinglePacket(&pkt,info.Number,false) != PARSER_MKV_OK)
		return false;
	if (pkt.PacketSize == 0 || pkt.PacketData == nullptr)
		return false;

	std::shared_ptr<IAudioDescription> audio;
	switch (codecId)
	{
	case MKV_Audio_MP2:
	case MKV_Audio_MP3:
		if (pkt.PacketSize < 5)
			return false;
		if (*pkt.PacketData != 0xFF)
			return false;
		audio = std::make_shared<MPEGAudioDescription>((unsigned*)pkt.PacketData);
		break;
	case MKV_Audio_AC3:
		if (pkt.PacketSize < 8)
			return false;
		unsigned short ac3Flag;
		ac3Flag = *(unsigned short*)pkt.PacketData;
		if (ac3Flag == AC3_SYNC_WORD_BE || ac3Flag == AC3_SYNC_WORD_LE)
			audio = std::make_shared<AC3AudioDescription>(pkt.PacketData);
		break;
	case MKV_Audio_DTS:
#ifndef _DONT_USE_LIBDCA
		if (pkt.PacketSize < 32)
			return false;
		unsigned dtsFlag;
		dtsFlag = *(unsigned*)pkt.PacketData;
		if (dtsFlag != DTS_SYNC_BE_RAW && dtsFlag != DTS_SYNC_LE_RAW &&
			dtsFlag != DTS_SYNC_BE_14BIT && dtsFlag != DTS_SYNC_LE_14BIT)
			return false;

		void* state;
		state = dca_init(0);
		if (state == nullptr)
			return false;
		int flags,sr,br,fl;
		int nch;
		nch = flags = br = sr = fl = 0;
		if (dca_syncinfo(state,pkt.PacketData,&flags,&sr,&br,&fl))
			nch = GetDTSChannelsFromFlags(flags);
		
		if (nch != 0)
		{
			CommonAudioCore common = {};
			common.type = -1;
			common.desc.compressed = true;
			common.desc.nch = nch;
			common.desc.srate = sr;
			common.desc.bits = info.Audio.BitDepth;
			common.desc.wav_avg_bytes = br / 8;
			common.desc.bitrate = br;
			audio = std::make_shared<CommonAudioDescription>(common);
		}

		dca_free(state);
#endif
		break;
	default:
		return false;
	}
	if (audio == nullptr)
		return false;

	AudioBasicDescription bDesc = {};
	audio->GetAudioDescription(&bDesc);
	if (bDesc.nch == 0 || bDesc.srate == 0)
		return false;
	
	desc = audio;
	return true;
}

static MediaCodecType InitMSVFWTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IAudioDescription>& desc,std::shared_ptr<MKVParser::MKVFileParser>& parser)
{
	ACM_WAVEFORMATEX wfx = {};
	memcpy(&wfx,info.Codec.CodecPrivate,sizeof(wfx));
	WAV_VFW_RIFF_TYPES type = SearchVFWCodecIdList2(wfx.wFormatTag);
	if (type == VFW_WAV_Others)
		return MEDIA_CODEC_UNKNOWN;

	static const WAV_VFW_RIFF_TYPES vfwTypes[] = {VFW_WAV_AMR_NB,VFW_WAV_AMR_WB,
		VFW_WAV_WMA1,VFW_WAV_WMA2,VFW_WAV_WMA3,VFW_WAV_WMA_LOSSLESS,
		VFW_WAV_AAC,VFW_WAV_AAC_ADTS,VFW_WAV_AAC_LATM};
	static const MediaCodecType cTypes[] = {MEDIA_CODEC_AUDIO_AMR_NB,MEDIA_CODEC_AUDIO_AMR_WB,
		MEDIA_CODEC_AUDIO_WMA7,MEDIA_CODEC_AUDIO_WMA8,MEDIA_CODEC_AUDIO_WMA9_PRO,MEDIA_CODEC_AUDIO_WMA9_LOSSLESS,
		MEDIA_CODEC_AUDIO_AAC,MEDIA_CODEC_AUDIO_AAC_ADTS,MEDIA_CODEC_AUDIO_AAC_LATM};

	MediaCodecType codecType = MEDIA_CODEC_UNKNOWN;
	for (unsigned i = 0;i < MKV_ARRAY_COUNT(vfwTypes);i++)
	{
		if (type == vfwTypes[i]) {
			codecType = cTypes[i];
			break;
		}
	}

	MKVParser::MKVPacket pkt = {};
	if (wfx.cbSize > 0)
	{
		if (info.Codec.CodecPrivateSize - sizeof(wfx) < wfx.cbSize)
			return MEDIA_CODEC_UNKNOWN;
	}else{
		if (parser->ReadSinglePacket(&pkt,info.Number,false) != PARSER_MKV_OK)
			return MEDIA_CODEC_UNKNOWN;
		if (pkt.PacketSize == 0 || pkt.PacketData == nullptr)
			return MEDIA_CODEC_UNKNOWN;
	}

	unsigned char* extradata = info.Codec.CodecPrivate + sizeof(wfx);
	CommonAudioCore core = {};
	core.type = wfx.wFormatTag;
	core.desc.bits = wfx.wBitsPerSample;
	core.desc.nch = wfx.nChannels;
	core.desc.srate = wfx.nSamplesPerSec;
	if (wfx.cbSize > 0)
	{
		core.extradata_size = wfx.cbSize;
		core.extradata = extradata;
	}

	MediaCodecType result = MEDIA_CODEC_UNKNOWN;
	switch (type)
	{
	case VFW_WAV_AMR_NB:
	case VFW_WAV_AMR_WB:
		core.desc.wav_avg_bytes = wfx.nAvgBytesPerSec;
		result = codecType;
		break;
	case VFW_WAV_WMA1:
	case VFW_WAV_WMA2:
	case VFW_WAV_WMA3:
	case VFW_WAV_WMA_LOSSLESS:
		if (wfx.nBlockAlign > 0)
		{
			result = codecType;
			core.desc.wav_block_align = wfx.nBlockAlign;
			core.desc.wav_avg_bytes = wfx.nAvgBytesPerSec;
		}
		break;
	case VFW_WAV_AAC:
	case VFW_WAV_AAC_ADTS:
		AAC_PROFILE_SPEC aac;
		memset(&aac,0,sizeof(aac));
		aac.profile = 1; //LC
		core.profile = (unsigned char*)&aac;
		core.profile_size = sizeof(aac);
		result = codecType;
		break;
	}

	if (result != MEDIA_CODEC_UNKNOWN)
	{
		std::shared_ptr<IAudioDescription> temp = 
			std::make_shared<CommonAudioDescription>(core);
		desc = temp;
	}
	return result;
}

static bool InitRealCookTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IAudioDescription>& desc,double* duration)
{
	if (info.Codec.CodecPrivateSize < sizeof(RealAudioPrivate))
		return false;
	if (info.Codec.CodecPrivateSize < (78 + 8))
		return false;

	auto cook = (RealAudioPrivate*)info.Codec.CodecPrivate;
	unsigned nch = 0,rate = 0,bits = 0;
	auto ver = MKV_SWAP16(cook->version);
	if (ver != 4 && ver != 5)
		return false;

	if (ver == 4)
	{
		if (info.Codec.CodecPrivateSize < sizeof(RealAudioPrivateV4))
			return false;
		auto cv4 = (RealAudioPrivateV4*)cook;
		nch = MKV_SWAP16(cv4->channels);
		rate = MKV_SWAP16(cv4->sample_rate);
		bits = MKV_SWAP16(cv4->sample_size);
	}else{
		if (info.Codec.CodecPrivateSize < sizeof(RealAudioPrivateV5))
			return false;
		auto cv5 = (RealAudioPrivateV5*)cook;
		nch = MKV_SWAP16(cv5->channels);
		rate = MKV_SWAP16(cv5->sample_rate);
		bits = MKV_SWAP16(cv5->sample_size);
	}

	if (info.Codec.CodecPrivateSize >= (78 + 8) && nch != 0 && rate != 0)
	{
		auto temp = *(unsigned short*)(info.Codec.CodecPrivate + 78 + 4);
		temp = MKV_SWAP16(temp) / nch;
		*duration = (double)temp / (double)rate;
	}

	CommonAudioCore core = {};
	core.type = -1;
	core.desc.compressed = true;
	core.desc.nch = nch;
	core.desc.srate = rate;
	core.desc.bits = bits;
	core.desc.wav_block_align = MKV_SWAP16(cook->sub_packet_size);
	core.extradata = info.Codec.CodecPrivate;
	core.extradata_size = info.Codec.CodecPrivateSize;

	std::shared_ptr<IAudioDescription> temp = 
		std::make_shared<CommonAudioDescription>(core);

	desc = temp;
	return true;
}

bool MKVMediaStream::ProbeAudio(std::shared_ptr<MKVParser::MKVFileParser>& parser) throw()
{
	_main_type = MediaMainType::MEDIA_MAIN_TYPE_AUDIO;

	CommonAudioCore common = {};
	common.type = (int)_info.Codec.CodecType;

	unsigned err_bits_fix = 0; //WV可能是错误的BITS，32->24。MP3必需是16。
	unsigned err_nch_fix = 0,err_rate_fix = 0; //opus的信息可以在CodecPrivate里面

	unsigned char* userdata = nullptr;
	unsigned userdataSize = 0;

	switch (_info.Codec.CodecType)
	{
	case MKV_Audio_AAC:
		common.type = 0;
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_AAC;
		if (_info.Codec.CodecPrivateSize > 0)
		{
			if (!InitAACLCTrack(_info,_audio_desc))
				return false;
		}else{
			//如果没有CP，则可能是MAIN\SSR等，并且Frame可能是ADTS。
			MKVParser::MKVPacket pkt = {};
			if (parser->ReadSinglePacket(&pkt,_info.Number,false) != PARSER_MKV_OK)
				return false;
			if (pkt.PacketSize < 4)
				return false;
			if (pkt.PacketData[0] == 0xFF)
			{
				_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_AAC_ADTS;
				if (!InitADTSTrack(_info,_audio_desc,pkt.PacketData))
					return false;
			}else{
				int audioObjectType = MKVAACGetAudioObjectType(_info.Codec.CodecID);
				if (audioObjectType == -1)
					return false;
				if (!InitAACTrack(_info,_audio_desc,(unsigned)audioObjectType))
					return false;
			}
		}
		break;
	case MKV_Audio_VORBIS:
		common.type = 0;
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_VORBIS;
		if (!InitVorbisTrack(_info,_audio_desc))
			return false;
		break;
	case MKV_Audio_FLAC:
		common.type = 0;
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_FLAC;
		if (!InitFLACTrack(_info,_audio_desc,parser))
			return false;
		break;
	case MKV_Audio_ALAC:
		common.type = 0;
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_ALAC;
		double alac_frame_duration;
		if (!InitALACTrack(_info,_audio_desc,&alac_frame_duration))
			return false;
		if (alac_frame_duration != 0.0)
			_frame_duration = alac_frame_duration;
		break;
	case MKV_Audio_MP1:
	case MKV_Audio_MP2:
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_MP2;
		if (_info.Codec.CodecType == MKV_Audio_MP1)
			_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_MP1;
		break;
	case MKV_Audio_MP3:
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_MP3;
		if (_info.Audio.Channels > 2)
			err_nch_fix = 2;
		err_bits_fix = 16;
		if (InitFirstPacketTrack(_info,_audio_desc,parser,_info.Codec.CodecType))
			common.type = 0;
		break;
	case MKV_Audio_AC3:
	case MKV_Audio_EAC3:
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_AC3;
		if (_info.Codec.CodecType == MKV_Audio_EAC3)
			_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_EAC3;
		if (_codec_type != MediaCodecType::MEDIA_CODEC_AUDIO_EAC3)
			if (InitFirstPacketTrack(_info,_audio_desc,parser,_info.Codec.CodecType))
				common.type = 0;
		break;
	case MKV_Audio_DTS:
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_DTS;
		if (InitFirstPacketTrack(_info,_audio_desc,parser,_info.Codec.CodecType))
			common.type = 0;
		break;
	case MKV_Audio_MLP:
	case MKV_Audio_TRUEHD:
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_TRUEHD;
		{
			//MLP\THD的音频，读第一个Packet用于给ffmpeg解码器预解码
			MKVParser::MKVPacket pkt = {};
			if (parser->ReadSinglePacket(&pkt,_info.Number,false) != PARSER_MKV_OK)
				return false;
			if (pkt.PacketSize < 4)
				return false;
			userdata = pkt.PacketData;
			userdataSize = pkt.PacketSize;
		}
		break;
	case MKV_Audio_TTA1:
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_TTA;
		if (_info.Audio.BitDepth > 24)
			return false; //ffmpeg bug.
		break;
	case MKV_Audio_WAVPACK4:
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_WAVPACK;
		{
			MKVParser::MKVPacket pkt = {};
			if (parser->ReadSinglePacket(&pkt,_info.Number,false) != PARSER_MKV_OK)
				return false;
			if (pkt.PacketSize < 40)
				return false;
			unsigned flag = *(unsigned*)(pkt.PacketData + 24);
			flag &= 0x3;
			flag++;
			err_bits_fix = flag * 8;
		}
		break;
	case MKV_Audio_OPUS:
	case MKV_Audio_OPUSE:
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_OPUS;
		if (_info.Codec.CodecPrivateSize >= sizeof(OpusHead))
		{
			OpusHead head = {};
			memcpy(&head,_info.Codec.CodecPrivate,sizeof(head));
			if (memcmp(head.flag_string,"OpusHead",8) == 0)
			{
				err_nch_fix = head.nch;
				err_rate_fix = head.rate;
			}
		}else{
			if (_info.Audio.Channels != 2 ||
				_info.Audio.SampleRate == 0 || _info.Audio.SampleRate > 48000)
				return false;
		}
		break;
	case MKV_Audio_PCM:
		_codec_type = MediaCodecType::MEDIA_CODEC_PCM_SINT_LE;
		break;
	case MKV_Audio_FLOAT:
		_codec_type = MediaCodecType::MEDIA_CODEC_PCM_FLOAT;
		break;
	case MKV_Audio_PCMBE:
		_codec_type = MediaCodecType::MEDIA_CODEC_PCM_SINT_BE;
		break;
	case MKV_Audio_MS_ACM:
		common.type = 0;
		if (_info.Codec.CodecPrivateSize < sizeof(ACM_WAVEFORMATEX) || _info.Codec.CodecPrivate == nullptr)
			return false;
		_codec_type = InitMSVFWTrack(_info,_audio_desc,parser);
		if (_codec_type == MEDIA_CODEC_UNKNOWN)
			return false;
		break;
	case MKV_Audio_Real_COOK:
		common.type = 0;
		_codec_type = MediaCodecType::MEDIA_CODEC_AUDIO_COOK;
		if (!InitRealCookTrack(_info,_audio_desc,&_frame_duration))
			return false;
		{
			auto cook = (RealAudioPrivate*)_info.Codec.CodecPrivate;
			_cook_info.CodedFrameSize = MKV_SWAP32(cook->coded_frame_size);
			_cook_info.FrameSize = MKV_SWAP16(cook->frame_size);
			_cook_info.SubPacketH = MKV_SWAP16(cook->sub_packet_h);
			_cook_info.SubPacketSize = MKV_SWAP16(cook->sub_packet_size);
			if (_cook_info.CodedFrameSize == 0 || _cook_info.FrameSize == 0 ||
				_cook_info.SubPacketH == 0 || _cook_info.SubPacketSize == 0)
				return false;
		}
		break;
	default:
		return false;
	}

	if (common.type != 0)
	{
		if (!ConvertMKVAudioInfoToCommonInfo(&_info.Audio,&common))
			return false;
		if (err_bits_fix > 0)
			common.desc.bits = err_bits_fix;
		if (err_nch_fix > 0)
			common.desc.nch = err_nch_fix;
		if (err_rate_fix > 0)
			common.desc.srate = err_rate_fix;

		common.extradata = _info.Codec.CodecPrivate;
		common.extradata_size = _info.Codec.CodecPrivateSize;
		if (common.extradata == nullptr)
		{
			common.extradata = userdata;
			common.extradata_size = userdataSize;
		}
		_audio_desc = std::make_shared<CommonAudioDescription>(common);
	}

	_stream_index = _info.Number;
	return true;
}