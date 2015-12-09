/*
 - FLV Stream Parser (Misc) -

 - Author: K.F Yang
 - Date: 2014-11-26
*/

#include "FLVParser.h"
#include <MPC/H264Nalu.h>
#include <MPC/GolombBuffer.h>
#include <stagefright/H264Parser.h>

using namespace FLVParser;

static void H264SPSGetWidthHeight(unsigned char* sps,unsigned size,unsigned* pw,unsigned* ph)
{
	unsigned width = 0,height = 0;

	CGolombBuffer gb(sps,size);
	int profile = (int)gb.BitRead(8);
	gb.BitRead(16); //profile_level
	gb.UExpGolombRead(); //seq_parameter_set_id
	unsigned chroma_format_idc = 0;
	if (profile == 100 || profile == 110 || profile == 122 || profile == 244 ||
		profile == 44 || profile == 83 || profile == 86) {
		chroma_format_idc = (unsigned)gb.UExpGolombRead();
		if (chroma_format_idc == 3) //chroma_format_idc
			gb.BitRead(1); //residual_colour_transform_flag
		gb.UExpGolombRead(); //bit_depth_luma_minus8
		gb.UExpGolombRead(); //bit_depth_chroma_minus8
		gb.BitRead(1); //qpprime_y_zero_transform_bypass_flag
		if (gb.BitRead(1)) //seq_scaling_matrix_present_flag
		{
			for (unsigned i = 0;i < 8;i++)
			{
				if (gb.BitRead(1)) //seq_scaling_list_present_flag
					return;
			}
		}
	}

	gb.UExpGolombRead(); //log2_max_frame_num_minus4
	unsigned pic_order_cnt_type = (unsigned)gb.UExpGolombRead();
	if (pic_order_cnt_type == 0) //pic_order_cnt_type
	{
		gb.UExpGolombRead(); //log2_max_pic_order_cnt_lsb_minus4
	}else if (pic_order_cnt_type == 1){
		gb.BitRead(1); //delta_pic_order_always_zero_flag
		gb.SExpGolombRead(); //offset_for_non_ref_pic
		gb.SExpGolombRead(); //offset_for_top_to_bottom_field
		unsigned num_ref_frames_in_pic_order_cnt_cycle = (unsigned)gb.UExpGolombRead();
		for (unsigned i = 0;i < num_ref_frames_in_pic_order_cnt_cycle;i++)
			gb.SExpGolombRead(); //offset_for_ref_frame
	}
	gb.UExpGolombRead(); //num_ref_frames
	gb.BitRead(1); //gaps_in_frame_num_value_allowed_flag

	unsigned pic_width_in_mbs_minus1 = (unsigned)gb.UExpGolombRead();
	unsigned pic_height_in_map_units_minus1 = (unsigned)gb.UExpGolombRead();
	unsigned frame_mbs_only_flag = (unsigned)gb.BitRead(1);
	if (frame_mbs_only_flag == 0)
		gb.BitRead(1); //mb_adaptive_frame_field_flag

	gb.BitRead(1); //direct_8x8_inference_flag
	unsigned frame_cropping_flag = (unsigned)gb.BitRead(1);
	unsigned frame_crop_left_offset = 0,frame_crop_right_offset = 0,frame_crop_top_offset = 0,frame_crop_bottom_offset = 0;
	if (frame_cropping_flag)
	{
		frame_crop_left_offset = (unsigned)gb.UExpGolombRead();
		frame_crop_right_offset = (unsigned)gb.UExpGolombRead();
		frame_crop_top_offset = (unsigned)gb.UExpGolombRead();
		frame_crop_bottom_offset = (unsigned)gb.UExpGolombRead();
	}

	//calc in pass1
	width = pic_width_in_mbs_minus1 * 16 + 16;
	height = (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 * 16 + 16);

	unsigned cropUnitX = 0,cropUnitY = 0;
	if (chroma_format_idc == 0)
	{
		cropUnitX = 1;
		cropUnitY = 2 - frame_mbs_only_flag;
	}else{
		unsigned subWC = (chroma_format_idc == 3) ? 1:2;
		unsigned subHC = (chroma_format_idc == 1) ? 2:1;
		cropUnitX = subWC;
		cropUnitY = subHC * (2 - frame_mbs_only_flag);
	}

	//calc in pass2
	width -= (frame_crop_left_offset + frame_crop_right_offset) * cropUnitX;
	height -= (frame_crop_top_offset + frame_crop_bottom_offset) * cropUnitY;
	*pw = width;
	*ph = height;
}

void FLVStreamParser::UpdateGlobalInfoH264()
{
	if (_global_info.delay_flush_spec_info.avc_spec_info == nullptr ||
		_global_info.delay_flush_spec_info.avc_info_size < 4)
		return;

	AutoBuffer sps(_global_info.delay_flush_spec_info.avc_info_size);
	memcpy(sps.Get(),_global_info.delay_flush_spec_info.avc_spec_info,sps.Size());

	CH264Nalu nalu;
	nalu.SetBuffer(sps.Get<unsigned char>(),sps.Size());

	stagefright::H264Parser::AVCExtraInfo avc = {};
	unsigned sps_width = 0,sps_height = 0;
	unsigned loop = 0;
	while (1)
	{
		if (loop > 14)
		{
			memset(&avc,0,sizeof(decltype(avc)));
			break;
		}

		if (!nalu.ReadNext())
			break;

		if (nalu.GetType() == NALU_TYPE::NALU_TYPE_SPS && nalu.GetDataLength() > 4)
		{
			H264SPSGetWidthHeight(nalu.GetDataBuffer() + 1,nalu.GetDataLength() - 1,
				&sps_width,&sps_height);
			if (sps_width != 0 && sps_height != 0)
			{
				avc.width = sps_width;
				avc.height = sps_height;
				avc.ref_frames = 1; //fake.
				break;
			}
		}

		if (nalu.GetType() == NALU_TYPE::NALU_TYPE_SPS)
			stagefright::H264Parser::ParseH264SPS(nalu.GetNALBuffer(),nalu.GetLength(),&avc);
		else if (nalu.GetType() == NALU_TYPE::NALU_TYPE_PPS)
			stagefright::H264Parser::ParseH264PPS(nalu.GetNALBuffer(),nalu.GetLength(),&avc);

		if (avc.chroma_format != stagefright::H264Parser::AVCExtraInfo::ChromaFUnknown &&
			avc.coding_mode != stagefright::H264Parser::AVCExtraInfo::CodingModeUnknown)
			break;

		loop++;
	}

	if (loop > 14)
		return;
	if (avc.ref_frames == 0)
		return;

	_global_info.video_info.width = avc.width;
	_global_info.video_info.height = avc.height;
}

bool FLVStreamParser::ProcessAVCDecoderConfigurationRecord(unsigned char* pb)
{
	if (_global_info.delay_flush_spec_info.avc_spec_info != nullptr)
		return true;

	if (pb[0] != 1 || pb[1] == 0) //configurationVersion
		return false;
	
	_global_info.delay_flush_spec_info.avc_profile = pb[1];
	_global_info.delay_flush_spec_info.avc_level = pb[3];

	_lengthSizeMinusOne = (pb[4] & 0x03) + 1;

	unsigned char* p = &pb[5];

	AutoBuffer sps_buf(1024);
	AutoBuffer pps_buf(1024);

	unsigned char* sps = sps_buf.Get<unsigned char>();
	unsigned char* pps = pps_buf.Get<unsigned char>();
	unsigned sps_size = 0,pps_size = 0;

	unsigned sps_count = *p & 0x1F;
	p++;
	if (sps_count == 0)
		return false;

	for (unsigned i = 0;i < sps_count;i++)
	{
		unsigned len = FLV_SWAP16(*(unsigned short*)p);
		p += 2;

		if (len == 0)
			continue;

		if ((len + 4) > (1024 - sps_size))
			break;

		sps[0] = 0;
		sps[1] = 0;
		sps[2] = 0;
		sps[3] = 1;

		memcpy(&sps[4],p,len);
		p += len;

		sps_size = sps_size + len + 4;
		sps = sps_buf.Get<unsigned char>() + sps_size;
	}

	unsigned pps_count = *p & 0x1F;
	p++;
	if (pps_count == 0)
		return false;

	for (unsigned i = 0;i < pps_count;i++)
	{
		unsigned len = FLV_SWAP16(*(unsigned short*)p);
		p += 2;

		if (len == 0)
			continue;

		if ((len + 4) > (1024 - (pps_size + sps_size)))
			break;

		pps[0] = 0;
		pps[1] = 0;
		pps[2] = 0;
		pps[3] = 1;

		memcpy(&pps[4],p,len);
		p += len;

		pps_size = pps_size + len + 4;
		pps = pps_buf.Get<unsigned char>() + pps_size;
	}

	if (sps_size > 0 && pps_size > 0)
	{
		_global_info.delay_flush_spec_info.avc_info_size = sps_size + pps_size;
		_global_info.delay_flush_spec_info.avc_spec_info = 
			(unsigned char*)malloc(FLV_ALLOC_ALIGNED(sps_size + pps_size));

		memcpy(_global_info.delay_flush_spec_info.avc_spec_info,
			sps_buf.Get<void>(),sps_size);
		memcpy(_global_info.delay_flush_spec_info.avc_spec_info + sps_size,
			pps_buf.Get<void>(),pps_size);
	}

	return (sps_size + pps_size) > 0;
}

bool FLVStreamParser::ProcessAACAudioSpecificConfig(unsigned char* pb)
{
	if (_global_info.delay_flush_spec_info.aac_spec_info == nullptr)
	{
		_global_info.delay_flush_spec_info.aac_spec_info = (unsigned char*)malloc(4);
		memcpy(_global_info.delay_flush_spec_info.aac_spec_info,pb,2);

		_global_info.delay_flush_spec_info.aac_profile = (pb[0] & 0xF8) >> 3;
		unsigned samplingFrequencyIndex = ((pb[0] & 0x07) << 1) | (pb[1] >> 7);
		unsigned channelConfiguration = (pb[1] >> 3) & 0x0F;

		static const int aac_srate_table[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000};
		if (samplingFrequencyIndex < 13)
			_global_info.audio_info.srate = aac_srate_table[samplingFrequencyIndex];
		_global_info.audio_info.nch = channelConfiguration;
	}

	return true;
}

double FLVStreamParser::SearchDurationFromLastTimestamp()
{
	bool ReadFlvStreamTag(FLVParser::IFLVParserIO* pIo,unsigned& tag_type,unsigned& data_size,unsigned& timestamp);

	auto size = _flv_io->GetSize();
	if (size < 8)
		return 0.0;

	size -= 4;
	if (!_flv_io->Seek(size))
		return 0.0;

	unsigned prevTagSize = 0;
	if (_flv_io->Read(&prevTagSize,4) != 4)
		return 0.0;

	prevTagSize = FLV_SWAP32(prevTagSize);
	if (prevTagSize == 0 || prevTagSize > size)
		return 0.0;

	size -= prevTagSize;
	if (!_flv_io->Seek(size - 4))
		return 0.0;

	unsigned tag_type = 0,tag_size = 0,tag_time = 0;
	if (!ReadFlvStreamTag(_flv_io,tag_type,tag_size,tag_time))
		return 0.0;
	if (tag_type != 8 && tag_type != 9)
		return 0.0;

	return (double)tag_time / 1000.0;
}