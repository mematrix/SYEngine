#include "H264Parser.h"

using namespace stagefright::H264Golomb;

void stagefright::H264Parser::ParseH264SPS(unsigned char* pb,unsigned len,stagefright::H264Parser::AVCExtraInfo* info)
{
	if (pb == nullptr || len == 0)
		return;
	if (info == nullptr)
		return;

	if (pb[0] != 0 && pb[1] != 0)
		return;

	unsigned offset = (pb[2] == 1 ? 3:4);
	decltype(pb) p = pb + offset + 1;
	stagefright::ABitReader br(p,len - offset - 1);
	
	info->profile = H264U(br,8);
	br.skipBits(8);
	info->profile_level = H264U(br,8);
	H264UE(br); //seq_parameter_set_id

	info->chroma_format = stagefright::H264Parser::AVCExtraInfo::ChromaF420;
	static const stagefright::H264Parser::AVCExtraInfo::ChromaSubsampling cf[] = {
		stagefright::H264Parser::AVCExtraInfo::ChromaFUnknown,
		stagefright::H264Parser::AVCExtraInfo::ChromaF420,
		stagefright::H264Parser::AVCExtraInfo::ChromaF422,
		stagefright::H264Parser::AVCExtraInfo::ChromaF444
	};

	unsigned chroma_format_idc = 0;
	if (info->profile == 100 || info->profile == 110 ||
		info->profile == 122 || info->profile == 244 ||
		info->profile == 44 || info->profile == 83 || info->profile == 86) {
		chroma_format_idc = H264UE(br);
		if (chroma_format_idc == 3)
			br.skipBits(1); //residual_colour_transform_flag

		if (chroma_format_idc <= 3)
			info->chroma_format = cf[chroma_format_idc];

		unsigned bit_depth_luma_minus8 = H264UE(br);
		unsigned bit_depth_chroma_minus8 = H264UE(br);
		br.skipBits(1); //qpprime_y_zero_transform_bypass_flag

		info->bit_depth = bit_depth_luma_minus8 + 8;

		if (br.getBits(1)) //seq_scaling_matrix_present_flag
		{
			for (unsigned i = 0;i < 8;i++)
			{
				if (br.getBits(1)) //seq_scaling_list_present_flag
					return; //地球不懂找不找到一个例子文件
			}
		}
	}

	if (info->profile == 0 || info->profile_level == 0)
		return;

	H264UE(br); //log2_max_frame_num_minus4
	unsigned pic_order_cnt_type = H264UE(br);
	if (pic_order_cnt_type == 0) {
		H264UE(br); //log2_max_pic_order_cnt_lsb_minus4
	}else if (pic_order_cnt_type == 1) {
		br.skipBits(1); //delta_pic_order_always_zero_flag
		H264SE(br); //offset_for_non_ref_pic
		H264SE(br); //offset_for_top_to_bottom_field

		unsigned num_ref_frames_in_pic_order_cnt_cycle = H264UE(br);
		for (unsigned i = 0;i < num_ref_frames_in_pic_order_cnt_cycle;i++)
			H264SE(br); //offset_for_ref_frame
	}

	unsigned num_ref_frames = H264UE(br);
	br.skipBits(1); //gaps_in_frame_num_value_allowed_flag

	info->ref_frames = num_ref_frames;

	unsigned pic_width_in_mbs_minus1 = H264UE(br);
	unsigned pic_height_in_map_units_minus1 = H264UE(br);

	unsigned frame_mbs_only_flag = br.getBits(1);
	unsigned mb_adaptive_frame_field_flag = 0;
	if (!frame_mbs_only_flag)
		mb_adaptive_frame_field_flag = br.getBits(1);

	if (frame_mbs_only_flag)
		info->progressive_flag = 1;
	info->mbaff_flag = mb_adaptive_frame_field_flag;

	br.skipBits(1); //direct_8x8_inference_flag
	unsigned frame_cropping_flag = br.getBits(1);
	unsigned frame_crop_left_offset = 0,frame_crop_right_offset = 0,frame_crop_top_offset = 0,frame_crop_bottom_offset = 0;
	if (frame_cropping_flag)
	{
		frame_crop_left_offset = H264UE(br);
		frame_crop_right_offset = H264UE(br);
		frame_crop_top_offset = H264UE(br);
		frame_crop_bottom_offset = H264UE(br);
	}

	info->width = pic_width_in_mbs_minus1 * 16 + 16;
	info->height = (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 * 16 + 16);

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

	info->width -= (frame_crop_left_offset + frame_crop_right_offset) * cropUnitX;
	info->height -= (frame_crop_top_offset + frame_crop_bottom_offset) * cropUnitY;

	info->ar_width = info->ar_height = 1;
	if (br.getBits(1)) //vui_parameters_present_flag
	{
		if (br.getBits(1)) //aspect_ratio_info_present_flag
		{
			unsigned aspect_ratio_idc = H264U(br,8);
			unsigned sar_width = 1,sar_height = 1;
			if (aspect_ratio_idc == 255)
			{
				sar_width = H264U(br,16);
				sar_height = H264U(br,16);
			}else{
				static const unsigned sw[] = {1,12,10,16,40,24,20,32,80,18,15,64,160,4,3,2};
				static const unsigned sh[] = {1,11,11,11,33,11,11,11,33,11,11,33,99,3,2,1};
				if (aspect_ratio_idc > 0 && aspect_ratio_idc < 16)
				{
					sar_width = sw[aspect_ratio_idc - 1];
					sar_height = sh[aspect_ratio_idc - 1];
				}
			}

			info->ar_width = sar_width;
			info->ar_height = sar_height;
		}

		if (br.getBits(1)) //overscan_info_present_flag
			br.skipBits(1); //overscan_appropriate_flag

		if (br.getBits(1)) //video_signal_type_present_flag
		{
			br.skipBits(3); //video_format
			unsigned video_full_range_flag = info->full_range_flag = br.getBits(1);

			if (br.getBits(1)) //colour_description_present_flag
				br.skipBits(24); //colour_primaries & transfer_characteristics & matrix_coefficients
		}

		if (br.getBits(1)) //chroma_loc_info_present_flag
		{
			H264UE(br); //chroma_sample_loc_type_top_field
			H264UE(br); //chroma_sample_loc_type_bottom_field
		}

		if (br.getBits(1)) //timing_info_present_flag
		{
			unsigned num_units_in_tick = H264U(br,32);
			unsigned time_scale = H264U(br,32);
			unsigned fixed_frame_rate_flag = info->fixed_fps_flag = br.getBits(1);

			info->fps_den = num_units_in_tick * 2;
			info->fps_num = time_scale;
		}
	}
}

void stagefright::H264Parser::ParseH264PPS(unsigned char* pb,unsigned len,stagefright::H264Parser::AVCExtraInfo* info)
{
	if (pb == nullptr || len == 0)
		return;
	if (info == nullptr)
		return;

	if (pb[0] != 0 && pb[1] != 0)
		return;

	unsigned offset = (pb[2] == 1 ? 3:4);
	decltype(pb) p = pb + offset + 1;
	stagefright::ABitReader br(p,len - offset - 1);

	H264UE(br); //pic_parameter_set_id
	H264UE(br); //seq_parameter_set_id

	unsigned entropy_coding_mode_flag = br.getBits(1);
	br.skipBits(1); //pic_order_present_flag
	info->slice_group_count = H264UE(br) + 1; //num_slice_groups_minus1

	info->coding_mode = stagefright::H264Parser::AVCExtraInfo::CodingModeCAVLC;
	if (entropy_coding_mode_flag)
		info->coding_mode = stagefright::H264Parser::AVCExtraInfo::CodingModeCABAC;
}