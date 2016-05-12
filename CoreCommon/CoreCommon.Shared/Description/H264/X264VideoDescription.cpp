#include "X264VideoDescription.h"

X264VideoDescription::X264VideoDescription(unsigned char* pNaluArray,unsigned len) throw() : _init_ok(false)
{
	_extradata_offset = _extradata_len = 0;
	_extradata_ok = false;

	//没有异常抛出，不要给空指针
	memset(&_profile,0,sizeof(_profile));
	memset(&_basic_desc,0,sizeof(_basic_desc));

	if (*(unsigned short*)pNaluArray == 0)
	{
		_parser = std::make_shared<H264AnnexBParser>();
		_init_ok = _parser->InitFromStartCode(pNaluArray,len);

		if (_init_ok)
			_init_ok = ParseSPS();

		if (_init_ok)
		{
			_extradata = std::unique_ptr<unsigned char>(new unsigned char[len / 4 * 4 + 8]);
			_extradata_offset = 0;
			_extradata_len = len;
			_extradata_ok = false;
		}
	}
}

int X264VideoDescription::GetType()
{
	return (int)X264_VIDEO_DESC_TYPE;
}

bool X264VideoDescription::GetProfile(void* profile)
{
	if (!_init_ok)
		return false;
	if (profile == nullptr)
		return false;

	memcpy(profile,&_profile,sizeof(_profile));
	return true;
}

bool X264VideoDescription::GetExtradata(void* p)
{
	if (p != nullptr && _init_ok)
	{
		InternalUpdateExtradata();
		memcpy(p,_extradata.get(),_extradata_offset);
		return true;
	}

	return false;
}

unsigned X264VideoDescription::GetExtradataSize()
{
	if (!_init_ok)
		return 0;

	InternalUpdateExtradata();
	return _extradata_offset;
}

bool X264VideoDescription::GetVideoDescription(VideoBasicDescription* desc)
{
	if (desc == nullptr)
		return false;
	if (!_init_ok)
		return false;

	*desc = _basic_desc;
	return true;
}

bool X264VideoDescription::ParseSPS()
{
	auto sps = (H264_SPS*)malloc(sizeof(H264_SPS));
	memset(sps,0,sizeof(H264_SPS));

	if (_parser->GetCurrentNaluType() == H264_NALU_TYPE_SLICE_SPS)
	{
		H264ParseSPS(_parser->GetCurrentDataPointer(),_parser->GetCurrentDataSize(),sps);
		InitFromSPS(sps);

		free(sps);
		return true;
	}

	while (1)
	{
		if (!_parser->ReadNext())
			break;

		if (_parser->GetCurrentNaluType() == H264_NALU_TYPE_SLICE_SPS)
		{
			memset(sps,0,sizeof(H264_SPS));
			H264ParseSPS(_parser->GetCurrentDataPointer(),_parser->GetCurrentDataSize(),sps);
			InitFromSPS(sps);

			free(sps);
			return true;
		}

		if (_parser->IsParseEOF())
			break;
	}

	free(sps);
	return false;
}

void X264VideoDescription::InitFromSPS(H264_SPS* sps)
{
	if (sps->profile_idc == 0)
	{
		_init_ok = false;
		return;
	}

	_profile.profile = sps->profile_idc;
	_profile.level = sps->level_idc;
	_profile.chroma_format = sps->chroma_format_idc;
	if (_profile.chroma_format <= 0)
		_profile.chroma_format = 1;

	_profile.ref_frames = sps->num_ref_frames;

	_profile.luma_bitdepth = sps->bit_depth_luma_minus8 + 8;
	_profile.chroma_bitdepth = sps->bit_depth_chroma_minus8 + 8;

	_profile.variable_framerate = sps->vui.fixed_frame_rate_flag == 0 ? 1:0;

	_basic_desc.width = (sps->pic_width_in_mbs_minus1 + 1) * 16;
	if (sps->frame_cropping_flag)
		_basic_desc.width -= sps->frame_crop_right_offset * 2;

	_basic_desc.height = (2 - sps->frame_mbs_only_flag) * (sps->pic_height_in_map_units_minus1 + 1) * 16;
	if (sps->frame_cropping_flag)
	{
		if (sps->frame_mbs_only_flag)
			_basic_desc.height -= sps->frame_crop_bottom_offset * 2;
		else
			_basic_desc.height -= sps->frame_crop_bottom_offset * 4;
	}

	if (sps->frame_mbs_only_flag)
	{
		_basic_desc.scan_mode = VideoScanModeProgressive;
		_basic_desc.interlaced_mode = VideoInterlacedScanModeUnknown;
	}else{
		_basic_desc.scan_mode = VideoScanModeInterleavedUpperFirst;
		_basic_desc.interlaced_mode = 
			sps->mb_adaptive_frame_field_flag == 1 ? VideoInterlacedScanModeMBAFF:
			VideoInterlacedScanModePAFF;
	}

	if (sps->vui_parameters_present_flag)
	{
		_basic_desc.frame_rate.num = sps->vui.time_scale;
		_basic_desc.frame_rate.den = sps->vui.num_units_in_tick * 2;
		if (sps->vui.fixed_frame_rate_flag) {
			if (_basic_desc.frame_rate.num > 0 && _basic_desc.frame_rate.den > 0)
				_profile.framerate = (double)_basic_desc.frame_rate.num / (double)_basic_desc.frame_rate.den;
		}

		_basic_desc.aspect_ratio.num = _basic_desc.aspect_ratio.den = 1;
		if (sps->vui.aspect_ratio_info_present_flag)
		{
			switch (sps->vui.aspect_ratio_idc)
			{
			case SAR_1_1:
				_basic_desc.aspect_ratio.num = _basic_desc.aspect_ratio.den = 1;
				break;
			case SAR_12_11:
				_basic_desc.aspect_ratio.num = 12;
				_basic_desc.aspect_ratio.den = 11;
				break;
			case SAR_10_11:
				_basic_desc.aspect_ratio.num = 10;
				_basic_desc.aspect_ratio.den = 11;
				break;
			case SAR_16_11:
				_basic_desc.aspect_ratio.num = 16;
				_basic_desc.aspect_ratio.den = 11;
				break;
			case SAR_40_33:
				_basic_desc.aspect_ratio.num = 40;
				_basic_desc.aspect_ratio.den = 33;
				break;
			case SAR_24_11:
				_basic_desc.aspect_ratio.num = 24;
				_basic_desc.aspect_ratio.den = 11;
				break;
			case SAR_20_11:
				_basic_desc.aspect_ratio.num = 20;
				_basic_desc.aspect_ratio.den = 11;
				break;
			case SAR_32_11:
				_basic_desc.aspect_ratio.num = 32;
				_basic_desc.aspect_ratio.den = 11;
				break;
			case SAR_80_33:
				_basic_desc.aspect_ratio.num = 80;
				_basic_desc.aspect_ratio.den = 33;
				break;
			case SAR_18_11:
				_basic_desc.aspect_ratio.num = 18;
				_basic_desc.aspect_ratio.den = 11;
				break;
			case SAR_15_11:
				_basic_desc.aspect_ratio.num = 15;
				_basic_desc.aspect_ratio.den = 11;
				break;
			case SAR_64_33:
				_basic_desc.aspect_ratio.num = 64;
				_basic_desc.aspect_ratio.den = 33;
				break;
			case SAR_160_99:
				_basic_desc.aspect_ratio.num = 160;
				_basic_desc.aspect_ratio.den = 99;
				break;
			case SAR_R_4_3:
				_basic_desc.aspect_ratio.num = 4;
				_basic_desc.aspect_ratio.den = 3;
				break;
			case SAR_R_3_2:
				_basic_desc.aspect_ratio.num = 3;
				_basic_desc.aspect_ratio.den = 2;
				break;
			case SAR_R_2_1:
				_basic_desc.aspect_ratio.num = 2;
				_basic_desc.aspect_ratio.den = 1;
				break;
			case SAR_Extended:
				_basic_desc.aspect_ratio.num = sps->vui.sar_width;
				_basic_desc.aspect_ratio.den = sps->vui.sar_height;
				break;
			default:
				_basic_desc.aspect_ratio.num = _basic_desc.aspect_ratio.den = 1;
			}
		}
	}

	_basic_desc.compressed = true;
	_basic_desc.bitrate = 0;
}

void X264VideoDescription::InternalUpdateExtradata()
{
	if (_extradata_ok)
		return;

	H264AnnexBParser parser(*_parser.get());
	if (parser.GetCurrentNaluType() == H264_NALU_TYPE_UNSPECIFIED)
		return;

	InternalCopyExtradata(parser);
	while (1)
	{
		if (!parser.ReadNext())
			break;

		InternalCopyExtradata(parser);

		if (parser.IsParseEOF())
			break;
	}

	_extradata_ok = true;
}

void X264VideoDescription::InternalCopyExtradata(H264AnnexBParser& parser)
{
	if (parser.GetCurrentNaluType() == H264_NALU_TYPE_SLICE_SPS ||
		parser.GetCurrentNaluType() == H264_NALU_TYPE_SLICE_PPS) {
		if (_extradata_offset < _extradata_len)
		{
			*(unsigned*)(_extradata.get() + _extradata_offset) = H264_ANNEXB_START_CODE_1;
			*(_extradata.get() + _extradata_offset + 4) = (unsigned char)parser.GetCurrentNalHead();
			_extradata_offset += 5;
			
			memcpy(_extradata.get() + _extradata_offset,parser.GetCurrentDataPointer(),parser.GetCurrentDataSize());
			_extradata_offset += parser.GetCurrentDataSize();
		}
	}
}