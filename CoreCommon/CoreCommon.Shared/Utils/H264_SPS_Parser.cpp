/*
 - H264 BitStream NALU SPS Parser -

 - Author: K.F Yang
 - Date: 2014-10-31
*/

#include "H264Spec.h"
#include "H264_SPS_Parser.h"
#include "HEVCAnnexBParser.h" //HEVC_EBSP2RBSP
#include <memory.h>
#include <malloc.h>

unsigned H264ParseSPS(unsigned char* ebsp,unsigned sps_len,H264_SPS* sps)
{
	auto pb = (unsigned char*)malloc(sps_len);
	memcpy(pb,ebsp,sps_len);

	HEVC_EBSP2RBSP(pb,sps_len); //skip 0x000003

	memset(sps,0,sizeof(sps));

	unsigned start_bit = 0;
	
	sps->profile_idc = h264_u(pb,8,start_bit);
	sps->reserved_8bits = h264_u(pb,8,start_bit);
	sps->level_idc = h264_u(pb,8,start_bit);

	sps->seq_parameter_set_id = h264_ue(pb,sps_len,start_bit);

	sps->chroma_format_idc = 1;
	if (sps->profile_idc == H264_PROFILE_HIGH || 
		sps->profile_idc == H264_PROFILE_HIGH_10 ||
		sps->profile_idc == H264_PROFILE_HIGH_422 ||
		sps->profile_idc == H264_PROFILE_HIGH_444 ||
		sps->profile_idc == H264_PROFILE_HIGH_444_PREDICTIVE ||
		sps->profile_idc == H264_PROFILE_CAVLC_444 ||
		sps->profile_idc == H264_PROFILE_HIGH_10_INTRA ||
		sps->profile_idc == H264_PROFILE_HIGH_422_INTRA ||
		sps->profile_idc == H264_PROFILE_HIGH_444_INTRA) {
		sps->chroma_format_idc = h264_ue(pb,sps_len,start_bit);
		if (sps->chroma_format_idc == 3)
			sps->residual_colour_transform_flag = h264_u(pb,1,start_bit);

		sps->bit_depth_luma_minus8 = h264_ue(pb,sps_len,start_bit);
		sps->bit_depth_chroma_minus8 = h264_ue(pb,sps_len,start_bit);

		sps->qpprime_y_zero_transform_bypass_flag = h264_u(pb,1,start_bit);
		sps->seq_scaling_matrix_present_flag = h264_u(pb,1,start_bit);
		if (sps->seq_scaling_matrix_present_flag)
		{
			for (int i = 0;i < 8;i++)
			{
				sps->seq_scaling_list_present_flag[i] = h264_u(pb,1,start_bit);
				if (sps->seq_scaling_list_present_flag[i])
					return 0; //地球不懂找不找到一个例子文件
			}
		}
	}

	if (sps->profile_idc != 0 && sps->level_idc != 0)
	{
		sps->log2_max_frame_num_minus4 = h264_ue(pb,sps_len,start_bit);
		sps->pic_order_cnt_type = h264_ue(pb,sps_len,start_bit);
		if (sps->pic_order_cnt_type == 0)
		{
			sps->log2_max_pic_order_cnt_lsb_minus4 = h264_ue(pb,sps_len,start_bit);
		}else if (sps->pic_order_cnt_type == 1)
		{
			sps->delta_pic_order_always_zero_flag = h264_u(pb,1,start_bit);
			sps->offset_for_non_ref_pic = h264_se(pb,sps_len,start_bit);
			sps->offset_for_top_to_bottom_field = h264_se(pb,sps_len,start_bit);
			sps->num_ref_frames_in_pic_order_cnt_cycle = h264_ue(pb,sps_len,start_bit);
			for (int i = 0;i < sps->num_ref_frames_in_pic_order_cnt_cycle;i++)
				sps->offset_for_ref_frame[i] = h264_se(pb,sps_len,start_bit);
		}

		sps->num_ref_frames = h264_ue(pb,sps_len,start_bit);
		sps->gaps_in_frame_num_value_allowed_flag = h264_u(pb,1,start_bit);

		sps->pic_width_in_mbs_minus1 = h264_ue(pb,sps_len,start_bit);
		sps->pic_height_in_map_units_minus1 = h264_ue(pb,sps_len,start_bit);

		sps->frame_mbs_only_flag = h264_u(pb,1,start_bit);
		if (!sps->frame_mbs_only_flag)
			sps->mb_adaptive_frame_field_flag = h264_u(pb,1,start_bit);

		sps->direct_8x8_inference_flag = h264_u(pb,1,start_bit);
		sps->frame_cropping_flag = h264_u(pb,1,start_bit);
		if (sps->frame_cropping_flag)
		{
			sps->frame_crop_left_offset = h264_ue(pb,sps_len,start_bit);
			sps->frame_crop_right_offset = h264_ue(pb,sps_len,start_bit);
			sps->frame_crop_top_offset = h264_ue(pb,sps_len,start_bit);
			sps->frame_crop_bottom_offset = h264_ue(pb,sps_len,start_bit);
		}

		sps->vui_parameters_present_flag = h264_u(pb,1,start_bit);
		if (sps->vui_parameters_present_flag)
		{
			sps->vui.aspect_ratio_info_present_flag = h264_u(pb,1,start_bit);
			if (sps->vui.aspect_ratio_info_present_flag)
			{
				sps->vui.aspect_ratio_idc = h264_u(pb,8,start_bit);
				if (sps->vui.aspect_ratio_idc == 255) //Extended_SAR
				{
					sps->vui.sar_width = h264_u(pb,16,start_bit);
					sps->vui.sar_height = h264_u(pb,16,start_bit);
				}
			}

			sps->vui.overscan_info_present_flag = h264_u(pb,1,start_bit);
			if (sps->vui.overscan_info_present_flag)
				sps->vui.overscan_appropriate_flag = h264_u(pb,1,start_bit);

			sps->vui.video_signal_type_present_flag = h264_u(pb,1,start_bit);
			if (sps->vui.video_signal_type_present_flag)
			{
				sps->vui.video_format = h264_u(pb,3,start_bit);
				sps->vui.video_full_range_flag = h264_u(pb,1,start_bit);
				sps->vui.colour_description_present_flag = h264_u(pb,1,start_bit);
				if (sps->vui.colour_description_present_flag)
				{
					sps->vui.colour_primaries = h264_u(pb,8,start_bit);
					sps->vui.transfer_characteristics = h264_u(pb,8,start_bit);
					sps->vui.matrix_coefficients = h264_u(pb,8,start_bit);
				}
			}

			sps->vui.chroma_loc_info_present_flag = h264_u(pb,1,start_bit);
			if (sps->vui.chroma_loc_info_present_flag)
			{
				sps->vui.chroma_sample_loc_type_top_field = h264_ue(pb,sps_len,start_bit);
				sps->vui.chroma_sample_loc_type_bottom_field = h264_ue(pb,sps_len,start_bit);
			}

			sps->vui.timing_info_present_flag = h264_u(pb,1,start_bit);
			if (sps->vui.timing_info_present_flag)
			{
				sps->vui.num_units_in_tick = h264_u(pb,32,start_bit);
				sps->vui.time_scale = h264_u(pb,32,start_bit);
				sps->vui.fixed_frame_rate_flag = h264_u(pb,1,start_bit);
				//fixed_frame_rate_flag == 0 时，上面的2个值可以忽略
			}

			h264_u(pb,1,start_bit);
		}
	}

	free(pb);

	return start_bit;
}