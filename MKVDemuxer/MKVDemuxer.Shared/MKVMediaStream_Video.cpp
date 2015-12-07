#include "MKVMediaStream.h"
#include "VFWHelp.h"
#include <HVC1mkvToAnnexB.h>
#include <MiscUtils.h>

#define MS_BMP_HEADER_SIZE 40

#pragma pack(1)
struct ACM_BITMAPINFOHEADER
{
	unsigned biSize;
	int biWidth;
	int biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned biCompression;
	unsigned biSizeImage;
	int biXPelsPerMeter;
	int biYPelsPerMeter;
	unsigned biClrUsed;
	unsigned biClrImportant;
};
#pragma pack()

static bool InitHEVCTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IVideoDescription>& desc,unsigned* nal_size)
{
	if (info.Codec.CodecPrivateSize == 0 ||
		info.Codec.CodecPrivate == nullptr)
		return false;
	if (info.Video.Width == 0 ||
		info.Video.Height == 0)
		return false;

	HEVCDecoderConfigurationRecordHead head = {};
	if (ParseHEVCDecoderConfigurationRecordHead(info.Codec.CodecPrivate,info.Codec.CodecPrivateSize,&head) == 0)
		return false;

	H264_PROFILE_SPEC profile = {};
	CommonVideoCore core = {};
	core.profile = (decltype(core.profile))&profile;
	core.profile_size = sizeof(profile);
	core.type = 0xFFFFFFFF; //mkv flag.

	core.desc.width = info.Video.Width;
	core.desc.height = info.Video.Height;
	core.desc.frame_rate.den = 10000000;
	core.desc.frame_rate.num = (int)(info.Video.FrameRate * 10000000.0);
	core.desc.aspect_ratio.den = core.desc.aspect_ratio.num = 1;
	if (info.InternalEntry->Video.DisplayWidth != info.Video.Width &&
		info.InternalEntry->Video.DisplayHeight != info.Video.Height)
		core.desc.aspect_ratio.num = info.InternalEntry->Video.DisplayWidth,
		core.desc.aspect_ratio.den = info.InternalEntry->Video.DisplayHeight;

	if (head.general_progressive_source_flag ||
		head.general_interlace_source_flag)
		core.desc.scan_mode = VideoScanModeMixedInterlaceOrProgressive;
	if (head.general_progressive_source_flag && !head.general_interlace_source_flag)
		core.desc.scan_mode = VideoScanModeProgressive;

	core.desc.compressed = true;

	if (head.size_nalu_minus_one == 0)
		return false;

	profile.profile = head.general_profile_idc;
	profile.level = head.general_level_idc;
	profile.nalu_size = head.size_nalu_minus_one + 1;
	profile.chroma_format = head.chroma_format_idc;
	profile.luma_bitdepth = head.bit_depth_luma_minus8 + 8;
	profile.chroma_bitdepth = head.bit_depth_chroma_minus8 + 8;

	auto p = (unsigned char*)malloc(2048);
	if (!p)
		return false;

	unsigned size = StripHEVCNaluArrays(info.Codec.CodecPrivate + HVC1_DCR_HEAD_LENGTH_BYTES,p);
	if (size == 0)
	{
		free(p);
		return false;
	}

	core.extradata = p;
	core.extradata_size = size;
	std::shared_ptr<IVideoDescription> hevc = std::make_shared<CommonVideoDescription>(core);
	free(p);

	*nal_size = profile.nalu_size;
	desc = hevc;
	return true;
}

static bool InitAVC1Track(MKVParser::MKVTrackInfo& info,std::shared_ptr<IVideoDescription>& desc)
{
	if (info.Codec.CodecPrivateSize < 8 ||
		info.Codec.CodecPrivate == nullptr)
		return false;

	std::shared_ptr<IVideoDescription> avc1 = 
		std::make_shared<AVC1VideoDescription>(info.Codec.CodecPrivate,info.Codec.CodecPrivateSize,0,0);

	H264_PROFILE_SPEC profile = {};
	avc1->GetProfile(&profile);
	if (profile.profile == 0)
		return false;

	VideoBasicDescription temp = {};
	avc1->GetVideoDescription(&temp);
	temp.width = info.Video.Width;
	temp.height = info.Video.Height;
	temp.frame_rate.den = 10000000;
	temp.frame_rate.num = (int)(info.Video.FrameRate * 10000000.0);
	avc1->ExternalUpdateVideoDescription(&temp);

	desc = avc1;
	return true;
}

static bool InitH264Track(MKVParser::MKVTrackInfo& info,std::shared_ptr<IVideoDescription>& desc,unsigned* nal_size)
{
	unsigned AVCDecoderConfigurationRecord2AnnexB(unsigned char* src,unsigned char** dst,unsigned* profile,unsigned* level,unsigned* nal_size,unsigned max_annexb_size);

	if (info.Codec.CodecPrivateSize == 0 ||
		info.Codec.CodecPrivate == nullptr)
		return false;

	unsigned char* ab = nullptr;
	unsigned size = AVCDecoderConfigurationRecord2AnnexB(info.Codec.CodecPrivate,&ab,nullptr,nullptr,nal_size,2048);
	if (ab == nullptr)
		return false;

	std::shared_ptr<IVideoDescription> h264 = std::make_shared<X264VideoDescription>(ab,size);
	free(ab);

	H264_PROFILE_SPEC profile = {};
	h264->GetProfile(&profile);
	if (profile.profile == 0)
		return false;

	//处理可变帧率的H264 ES。
	if (profile.variable_framerate && info.Video.FrameRate != 0.0)
	{
		VideoBasicDescription temp = {};
		h264->GetVideoDescription(&temp);
		temp.frame_rate.den = 10000000;
		temp.frame_rate.num = (int)(info.Video.FrameRate * 10000000.0);
		h264->ExternalUpdateVideoDescription(&temp);
	}

	desc = h264;
	return true;
}

static bool InitH264NoCPTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IVideoDescription>& desc,std::shared_ptr<MKVParser::MKVFileParser>& parser,unsigned* nal_size)
{
	MKVParser::MKVPacket pkt = {};
	if (parser->ReadSinglePacket(&pkt,info.Number,false) != PARSER_MKV_OK)
		return false;
	if (pkt.PacketSize < 8 || pkt.PacketData == nullptr)
		return false;

	if (pkt.PacketData[0] != 0 && pkt.PacketData[1] != 0)
		return false;
	if (pkt.PacketData[2] != 1 && pkt.PacketData[3] != 1)
		return false;

	unsigned nsize = 3;
	if (pkt.PacketData[3] == 1)
		nsize = 4;
	*nal_size = nsize;

	std::shared_ptr<IVideoDescription> h264 = 
		std::make_shared<X264VideoDescription>(pkt.PacketData,pkt.PacketSize);

	H264_PROFILE_SPEC profile = {};
	h264->GetProfile(&profile);
	if (profile.profile == 0)
		return false;

	desc = h264;
	return true;
}

static bool InitMPEG2Track(MKVParser::MKVTrackInfo& info,std::shared_ptr<IVideoDescription>& desc)
{
	if (info.Codec.CodecPrivateSize == 0 ||
		info.Codec.CodecPrivate == nullptr)
		return false;

	if (FindMPEG2StartCodeOffset(info.Codec.CodecPrivate,info.Codec.CodecPrivateSize,0xFF) == -1)
		return false;

	std::shared_ptr<IVideoDescription> mpeg2 = std::make_shared<MPEG2VideoDescription>(
		info.Codec.CodecPrivate,info.Codec.CodecPrivateSize);

	if (mpeg2->GetExtradataSize() == 0)
		return false;

	desc = mpeg2;
	return true;
}

static bool InitMPEG2NoCPTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IVideoDescription>& desc,std::shared_ptr<MKVParser::MKVFileParser>& parser)
{
	MKVParser::MKVPacket pkt = {};
	if (parser->ReadSinglePacket(&pkt,info.Number,false) != PARSER_MKV_OK)
		return false;
	if (pkt.PacketSize < 8 || pkt.PacketData == nullptr)
		return false;

	if (pkt.PacketData[0] != 0 && pkt.PacketData[1] != 0 && pkt.PacketData[2] != 1)
		return false;

	std::shared_ptr<IVideoDescription> mpeg2 = 
		std::make_shared<MPEG2VideoDescription>(pkt.PacketData,pkt.PacketSize - 8);

	if (mpeg2->GetExtradataSize() == 0)
		return false;

	desc = mpeg2;
	return true;
}

static bool InitMPEG4Track(MKVParser::MKVTrackInfo& info,std::shared_ptr<IVideoDescription>& desc)
{
	if (info.Codec.CodecPrivateSize == 0 ||
		info.Codec.CodecPrivate == nullptr)
		return false;

	unsigned offset = FindNextMPEG4StartCode(info.Codec.CodecPrivate,info.Codec.CodecPrivateSize);
	if (offset < 3)
		return false;
	offset -= 3;

	std::shared_ptr<IVideoDescription> m4p2 = std::make_shared<MPEG4VideoDescription>(
		info.Codec.CodecPrivate + offset,info.Codec.CodecPrivateSize - offset);

	if (m4p2->GetExtradataSize() == 0)
		return false;

	desc = m4p2;
	return true;
}

static bool InitMPEG4NoCPTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IVideoDescription>& desc,std::shared_ptr<MKVParser::MKVFileParser>& parser)
{
	MKVParser::MKVPacket pkt = {};
	if (parser->ReadSinglePacket(&pkt,info.Number,false) != PARSER_MKV_OK)
		return false;
	if (pkt.PacketSize < 8 || pkt.PacketData == nullptr)
		return false;

	if (pkt.PacketData[0] != 0 && pkt.PacketData[1] != 0 && pkt.PacketData[2] != 1)
		return false;

	static const unsigned char frameEnd[] = {0,0,1,0xB6};
	int offset = MemorySearch(pkt.PacketData,pkt.PacketSize,frameEnd,4);
	if (offset < 4)
		return false;

	std::shared_ptr<IVideoDescription> m4p2 = 
		std::make_shared<MPEG4VideoDescription>(pkt.PacketData,offset);

	if (m4p2->GetExtradataSize() == 0)
		return false;

	desc = m4p2;
	return true;
}

static bool InitCommonTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IVideoDescription>& desc)
{
	if (info.Video.Width == 0 ||
		info.Video.Height == 0 ||
		info.Video.FrameRate < 1.0)
		return false;

	CommonVideoCore common = {};
	common.type = -1;
	common.desc.aspect_ratio.num = common.desc.aspect_ratio.den = 1;
	common.desc.scan_mode = VideoScanModeMixedInterlaceOrProgressive;
	common.desc.width = info.Video.Width;
	common.desc.height = info.Video.Height;
	common.desc.frame_rate.num = (int)(info.Video.FrameRate * 10000000.0);
	common.desc.frame_rate.den = 10000000;
	common.desc.compressed = true;

	if (common.desc.frame_rate.num < 0 && info.Codec.CodecType == MKV_Video_MJPEG)
	{
		common.desc.frame_rate.num = 10;
		common.desc.frame_rate.den = 1;
		common.desc.scan_mode = VideoScanModeProgressive;
	}

	std::shared_ptr<IVideoDescription> v =
		std::make_shared<CommonVideoDescription>(common);

	desc = v;
	return true;
}

static bool InitCommonTrackEx(MKVParser::MKVTrackInfo& info,std::shared_ptr<IVideoDescription>& desc,ACM_BITMAPINFOHEADER& head,bool interlaced = false)
{
	CommonVideoCore common = {};
	common.type = head.biCompression;
	common.desc.aspect_ratio.num = common.desc.aspect_ratio.den = 1;
	common.desc.scan_mode = 
		interlaced ? VideoScanModeMixedInterlaceOrProgressive:VideoScanModeProgressive;
	common.desc.width = head.biWidth;
	common.desc.height = head.biHeight;
	common.desc.frame_rate.num = (int)(info.Video.FrameRate * 10000000.0);
	common.desc.frame_rate.den = 10000000;
	if (common.desc.frame_rate.num == 0)
		common.desc.frame_rate.den = 0;

	if (info.Codec.CodecPrivateSize > sizeof(ACM_BITMAPINFOHEADER))
	{
		common.extradata = info.Codec.CodecPrivate + sizeof(ACM_BITMAPINFOHEADER);
		common.extradata_size = info.Codec.CodecPrivateSize - sizeof(ACM_BITMAPINFOHEADER);
	}

	std::shared_ptr<IVideoDescription> v =
		std::make_shared<CommonVideoDescription>(common);

	desc = v;
	return true;
}

static MediaCodecType InitMSVFWTrack(MKVParser::MKVTrackInfo& info,std::shared_ptr<IVideoDescription>& desc,std::shared_ptr<MKVParser::MKVFileParser>& parser)
{
	ACM_BITMAPINFOHEADER bih = {};
	memcpy(&bih,info.Codec.CodecPrivate,sizeof(bih));

	unsigned fourcc = *(unsigned*)(info.Codec.CodecPrivate + 16);
	AVI_VFW_RIFF_TYPES type = SearchVFWCodecIdList(fourcc);
	if (type == VFW_RIFF_Others)
		return MEDIA_CODEC_UNKNOWN;

	static const AVI_VFW_RIFF_TYPES vfwTypes[] = {VFW_RIFF_H264,VFW_RIFF_H263,VFW_RIFF_MPEG4,
		VFW_RIFF_MPEG4V3,VFW_RIFF_MPEG4V2,VFW_RIFF_MPEG4V1,
		VFW_RIFF_MPG1V,VFW_RIFF_MPG2V,VFW_RIFF_MJPEG,
		VFW_RIFF_WMV1,VFW_RIFF_WMV2,VFW_RIFF_WMV3,
		VFW_RIFF_VC1,VFW_RIFF_FFV1,VFW_RIFF_FLV1,
		VFW_RIFF_THEORA,VFW_RIFF_SNOW,VFW_RIFF_PRORES,
		VFW_RIFF_DIRAC,VFW_RIFF_CAVS,VFW_RIFF_AMV};
	static const MediaCodecType cTypes[] = {MEDIA_CODEC_VIDEO_H264,MEDIA_CODEC_VIDEO_H263,MEDIA_CODEC_VIDEO_MPEG4,
		MEDIA_CODEC_VIDEO_MS_MPEG4_V3,MEDIA_CODEC_VIDEO_MS_MPEG4_V2,MEDIA_CODEC_VIDEO_MS_MPEG4_V1,
		MEDIA_CODEC_VIDEO_MPEG1,MEDIA_CODEC_VIDEO_MPEG2,MEDIA_CODEC_VIDEO_MJPEG,
		MEDIA_CODEC_VIDEO_WMV7,MEDIA_CODEC_VIDEO_WMV8,MEDIA_CODEC_VIDEO_WMV9,
		MEDIA_CODEC_VIDEO_VC1,MEDIA_CODEC_VIDEO_FFV1,MEDIA_CODEC_VIDEO_FLV1,
		MEDIA_CODEC_VIDEO_THEORA,MEDIA_CODEC_VIDEO_SNOW,MEDIA_CODEC_VIDEO_PRORES,
		MEDIA_CODEC_VIDEO_DIRAC,MEDIA_CODEC_VIDEO_CAVS,MEDIA_CODEC_VIDEO_AMV};

	MediaCodecType codecType = MEDIA_CODEC_UNKNOWN;
	for (unsigned i = 0;i < MKV_ARRAY_COUNT(vfwTypes);i++)
	{
		if (type == vfwTypes[i]) {
			codecType = cTypes[i];
			break;
		}
	}

	MKVParser::MKVPacket pkt = {};
	if (parser->ReadSinglePacket(&pkt,info.Number,false) != PARSER_MKV_OK)
		return MEDIA_CODEC_UNKNOWN;
	if (pkt.PacketSize == 0 || pkt.PacketData == nullptr)
		return MEDIA_CODEC_UNKNOWN;

	MKVParser::MKVTrackInfo virtual_info = {};
	memcpy(&virtual_info,&info,sizeof(virtual_info));
	virtual_info.Codec.CodecPrivate = nullptr;
	virtual_info.Codec.CodecPrivateSize = 0;
	if (!virtual_info.Codec.InternalCopyCodecPrivate(pkt.PacketData,pkt.PacketSize))
		return MEDIA_CODEC_UNKNOWN;

	MediaCodecType result = MEDIA_CODEC_UNKNOWN;
	switch (type)
	{
	case VFW_RIFF_H264:
		if (fourcc != _MK_RIFF_FCC('avc1') && fourcc != _MK_RIFF_FCC('AVC1')
			&& pkt.PacketSize > 8 && pkt.PacketData[0] == 0 && pkt.PacketData[1] == 0 &&
			(pkt.PacketData[2] == 1 || pkt.PacketData[3] == 1)) {
			std::shared_ptr<IVideoDescription> h264 = 
				std::make_shared<X264VideoDescription>(pkt.PacketData,pkt.PacketSize);
			if (h264->GetExtradataSize() > 0)
			{
				result = MEDIA_CODEC_VIDEO_H264_ES;
				desc = h264;
			}
		}
		break;
	case VFW_RIFF_H263:
		if (InitCommonTrack(info,desc))
			result = codecType;
		break;
	case VFW_RIFF_MPEG4:
		if (pkt.PacketSize > 14 &&
			pkt.PacketData[0] == 0 && pkt.PacketData[1] == 0 && pkt.PacketData[2] == 1) {
			static const unsigned char frameEnd[] = {0,0,1,0xB6};
			int offset = MemorySearch(pkt.PacketData,pkt.PacketSize,frameEnd,4);
			if (offset > 4)
			{
				virtual_info.Codec.CodecPrivateSize = (unsigned)offset;
				if (InitMPEG4Track(virtual_info,desc))
					result = codecType;
			}
		}
		break;
	case VFW_RIFF_MJPEG:
	case VFW_RIFF_MPEG4V1:
	case VFW_RIFF_MPEG4V2:
	case VFW_RIFF_MPEG4V3:
		if (InitCommonTrack(info,desc))
			result = codecType;
		break;
	case VFW_RIFF_MPG1V:
	case VFW_RIFF_MPG2V:
		if (pkt.PacketSize > 10 &&
			pkt.PacketData[0] == 0 && pkt.PacketData[1] == 0 && pkt.PacketData[2] == 1) {
			virtual_info.Codec.CodecPrivateSize -= 8;
			if (InitMPEG2Track(virtual_info,desc))
				result = codecType;
		}
		break;
	case VFW_RIFF_WMV1:
	case VFW_RIFF_WMV2:
	case VFW_RIFF_WMV3:
		if (InitCommonTrackEx(info,desc,bih))
			result = codecType;
		break;
	case VFW_RIFF_VC1:
		if (pkt.PacketSize > 8 &&
			pkt.PacketData[0] == 0 && pkt.PacketData[1] == 0 &&
			pkt.PacketData[2] == 1 && pkt.PacketData[3] != 0) {
			if (FindVC1StartCodeOffset(pkt.PacketData,pkt.PacketSize - 4,0) != -1)
			{
				std::shared_ptr<IVideoDescription> vc1 = 
					std::make_shared<VC1VideoDescription>(pkt.PacketData,pkt.PacketSize);

				if (vc1->GetExtradataSize() > 0)
				{
					result = codecType;
					desc = vc1;
				}
			}
		}else{
			if (info.Codec.CodecPrivateSize > MS_BMP_HEADER_SIZE)
			{
				if (InitCommonTrackEx(info,desc,bih))
					result = codecType;
			}
		}
		break;
	}

	virtual_info.Codec.InternalFree();
	return result;
}

bool MKVMediaStream::ProbeVideo(std::shared_ptr<MKVParser::MKVFileParser>& parser,bool force_avc1)
{
	_main_type = MediaMainType::MEDIA_MAIN_TYPE_VIDEO;
	_fps = _info.Video.FrameRate;

	switch (_info.Codec.CodecType)
	{
	case MKV_Video_HEVC:
		_codec_type = MediaCodecType::MEDIA_CODEC_VIDEO_HEVC;
		if (!InitHEVCTrack(_info,_video_desc,&_264lengthSizeMinusOne))
			return false;
		break;
	case MKV_Video_H264:
		_codec_type = MediaCodecType::MEDIA_CODEC_VIDEO_H264;
		if (_info.Codec.CodecPrivateSize > 0)
		{
			if (force_avc1)
			{
				if (!InitAVC1Track(_info,_video_desc))
					return false;
			}else{
				if (!InitH264Track(_info,_video_desc,&_264lengthSizeMinusOne))
					return false;
			}
		}else{
			if (!InitH264NoCPTrack(_info,_video_desc,parser,&_264lengthSizeMinusOne))
				return false;
			_codec_type = MediaCodecType::MEDIA_CODEC_VIDEO_H264_ES;
		}
		break;
	case MKV_Video_MPEG1:
	case MKV_Video_MPEG2:
		_codec_type = MediaCodecType::MEDIA_CODEC_VIDEO_MPEG2;
		if (_info.Codec.CodecType == MKV_Video_MPEG1)
			_codec_type = MediaCodecType::MEDIA_CODEC_VIDEO_MPEG1;
		if (_info.Codec.CodecPrivateSize > 0)
		{
			if (!InitMPEG2Track(_info,_video_desc))
				return false;
		}else{
			if (!InitMPEG2NoCPTrack(_info,_video_desc,parser))
				return false;
		}
		break;
	case MKV_Video_MPEG4_SP:
	case MKV_Video_MPEG4_AP:
	case MKV_Video_MPEG4_ASP:
		_codec_type = MediaCodecType::MEDIA_CODEC_VIDEO_MPEG4;
		if (_info.Codec.CodecPrivateSize > 0)
		{
			if (!InitMPEG4Track(_info,_video_desc))
				return false;
		}else{
			if (!InitMPEG4NoCPTrack(_info,_video_desc,parser))
				return false;
		}
		break;
	case MKV_Video_MPEG4_MSV3:
		_codec_type = MediaCodecType::MEDIA_CODEC_VIDEO_MS_MPEG4_V3;
		if (!InitCommonTrack(_info,_video_desc))
			return false;
		break;
	case MKV_Video_MJPEG:
		_codec_type = MediaCodecType::MEDIA_CODEC_VIDEO_MJPEG;
		if (!InitCommonTrack(_info,_video_desc))
			return false;
		if ((int)_info.Video.FrameRate == 90000)
		{
			//FFMPEG会把MP3的COVER混流为MJPEG，忽略它。
			auto tag = parser->GetCore()->GetTags()->SearchTagByName("ENCODER");
			if (tag != nullptr)
			{
				if (strstr(tag->TagString,"lavf") != nullptr ||
					strstr(tag->TagString,"Lavf") != nullptr ||
					strstr(tag->TagString,"LAVF") != nullptr)
					return false;
			}
		}
		break;
	case MKV_Video_VP8:
	case MKV_Video_VP9:
	case MKV_Video_VP10:
		_codec_type = MediaCodecType::MEDIA_CODEC_VIDEO_VP8;
		if (_info.Codec.CodecType == MKV_Video_VP9)
			_codec_type = MediaCodecType::MEDIA_CODEC_VIDEO_VP9;
		else if (_info.Codec.CodecType == MKV_Video_VP10)
			_codec_type = MediaCodecType::MEDIA_CODEC_VIDEO_VP10;
		if (!InitCommonTrack(_info,_video_desc))
			return false;
		break;
	case MKV_Video_MS_VFW:
		//http://ffmpeg.org/doxygen/trunk/riff_8c_source.html (ff_codec_bmp_tags)
		//http://ffmpeg.org/doxygen/trunk/isom_8c_source.html (ff_codec_movvideo_tags)
		if (_info.Codec.CodecPrivateSize < MS_BMP_HEADER_SIZE || _info.Codec.CodecPrivate == nullptr)
			return false;
		_codec_type = InitMSVFWTrack(_info,_video_desc,parser);
		if (_codec_type == MEDIA_CODEC_UNKNOWN)
			return false;
		break;
	default:
		return false;
	}
	if (_frame_duration <= 0.0)
	{
		VideoBasicDescription desc = {};
		_video_desc->GetVideoDescription(&desc);
		_frame_duration = 1.0 / ((double)desc.frame_rate.num / (double)desc.frame_rate.den);
		if (_info.Codec.CodecType == MKV_Video_H264)
		{
			H264_PROFILE_SPEC profile = {};
			_video_desc->GetProfile(&profile);
			_frame_duration = 1.0 / profile.framerate;
		}
	}

	_stream_index = _info.Number;
	return true;
}