/*
 - H264 BitStream NALU SPS Parser -

 - Author: K.F Yang
 - Date: 2014-10-31
*/

#ifndef __H264_SPS_PARSER_H
#define __H264_SPS_PARSER_H

#include <math.h>

struct H264_SPS
{
	int profile_idc;
	int reserved_8bits;
	int level_idc;
	int seq_parameter_set_id;
	int chroma_format_idc;
	int residual_colour_transform_flag;
	int bit_depth_luma_minus8;
	int bit_depth_chroma_minus8;
	int qpprime_y_zero_transform_bypass_flag;
	int seq_scaling_matrix_present_flag;
		int seq_scaling_list_present_flag[8];
	int log2_max_frame_num_minus4;
	int pic_order_cnt_type;
		int log2_max_pic_order_cnt_lsb_minus4;
		int delta_pic_order_always_zero_flag;
		int offset_for_non_ref_pic;
		int offset_for_top_to_bottom_field;
		int num_ref_frames_in_pic_order_cnt_cycle;
		int offset_for_ref_frame[256];
	int num_ref_frames;
	int gaps_in_frame_num_value_allowed_flag;
	int pic_width_in_mbs_minus1;
	int pic_height_in_map_units_minus1;
	int frame_mbs_only_flag;
	int mb_adaptive_frame_field_flag;
	int direct_8x8_inference_flag;
	int frame_cropping_flag;
		int frame_crop_left_offset;
		int frame_crop_right_offset;
		int frame_crop_top_offset;
		int frame_crop_bottom_offset;
	int vui_parameters_present_flag;

	struct
	{
		int aspect_ratio_info_present_flag;
			int aspect_ratio_idc;
				int sar_width;
				int sar_height;
		int overscan_info_present_flag;
			int overscan_appropriate_flag;
		int video_signal_type_present_flag;
			int video_format;
			int video_full_range_flag;
			int colour_description_present_flag;
				int colour_primaries;
				int transfer_characteristics;
				int matrix_coefficients;
		int chroma_loc_info_present_flag;
			int chroma_sample_loc_type_top_field;
			int chroma_sample_loc_type_bottom_field;
		int timing_info_present_flag;
			int num_units_in_tick;
			int time_scale;
			int fixed_frame_rate_flag;
	} vui;

	/*
		int nal_hrd_parameters_present_flag;
		int vcl_hrd_parameters_present_flag;
			int low_delay_hrd_flag;
		int pic_struct_present_flag;
		int bitstream_restriction_flag;
			int motion_vectors_over_pic_boundaries_flag;
			int max_bytes_per_pic_denom;
			int max_bits_per_mb_denom;
			int log2_max_mv_length_horizontal;
			int log2_max_mv_length_vertical;
			int num_reorder_frames;
			int max_dec_frame_buffering;
	} vui;

	struct
	{
		int cpb_cnt_minus1;
		int bit_rate_scale;
		int cpb_size_scale;
			int bit_rate_value_minus1[32]; // up to cpb_cnt_minus1, which is <= 31
			int cpb_size_value_minus1[32];
			int cbr_flag[32];
		int initial_cpb_removal_delay_length_minus1;
		int cpb_removal_delay_length_minus1;
		int dpb_output_delay_length_minus1;
		int time_offset_length;
	} hrd;
	*/
};


static unsigned h264_u(unsigned char* pb,unsigned bit_count,unsigned& start_bit)
{
	unsigned result = 0;
	for (unsigned i = 0;i < bit_count;i++)
	{
		result <<= 1;
		if (pb[start_bit >> 3] & (0x80 >> (start_bit % 8)))
			result += 1;

		start_bit++;
	}

	return result;
}

static unsigned h264_ue(unsigned char* pb,unsigned len,unsigned& start_bit)
{
	unsigned zero_num = 0;
	while (start_bit < len * 8)
	{
		if (pb[start_bit >> 3] & (0x80 >> (start_bit % 8)))
			break;

		zero_num++;
		start_bit++;
	}

	start_bit++;

	unsigned result = 0;
	for (unsigned i = 0;i < zero_num;i++)
	{
		result <<= 1;
		if (pb[start_bit >> 3] & (0x80 >> (start_bit % 8)))
			result += 1;

		start_bit++;
	}

	return (1 << zero_num) - 1 + result; 
}

static int h264_se(unsigned char* pb,unsigned len,unsigned& start_bit)
{
	int ue_val = h264_ue(pb,len,start_bit);
	int val = (int)ceil(((double)ue_val / 2));

	if (ue_val % 2 == 0)
		val = -val;

	return val;
}

unsigned H264ParseSPS(unsigned char* ebsp,unsigned sps_len,H264_SPS* sps);

#endif //__H264_SPS_PARSER_H