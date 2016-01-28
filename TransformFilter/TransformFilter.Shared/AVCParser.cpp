#include "BitReader.h"
#include "AVCParser.h"
#include <malloc.h>
#include <memory.h>

#ifdef _MSC_VER
#pragma warning(disable:4146)
#endif

#define _SWAP16(x) ((((x) & 0xFF00U) >> 8) | (((x) & 0x00FFU) << 8))

static int EBSP2RBSP(unsigned char* pb, unsigned len)
{
	unsigned aligned_size = len / 4 * 4 + 16;
	auto p = (unsigned char*)malloc(aligned_size * 2);
	auto buf = p + aligned_size;
	memcpy(p + 2, pb, len);
	*p = 0xFF;

	unsigned char* src = p + 2;
	unsigned char* dst = buf;
	unsigned skip_count = 0;
	for (unsigned i = 0; i < len; i++) {
		if (src[i] == 0x03 && src[i - 1] == 0 && src[i - 2] == 0) {
			skip_count++;
			continue;
		}
		*dst = src[i];
		dst++;
	}
	memcpy(pb, buf, len - skip_count);

	free(p);
	return (int)(len - skip_count);
}

static unsigned UE(android::ABitReader* br)
{
	unsigned numZeroes = 0;
	while (br->getBits(1) == 0)
		++numZeroes;
	unsigned x = br->getBits(numZeroes);
	return x + (1U << numZeroes) - 1;
}

static signed SE(android::ABitReader* br)
{
	unsigned codeNum = UE(br);
	return (codeNum & 1) ? (codeNum + 1) / 2 : -(codeNum / 2);
}

static void skipScalingList(android::ABitReader* br, unsigned sizeOfScalingList)
{
	size_t lastScale = 8;
	size_t nextScale = 8;
	for (size_t i = 0; i < sizeOfScalingList; ++i) {
		if (nextScale != 0) {
			signed delta_scale = SE(br);
			nextScale = (lastScale + delta_scale + 256) % 256;
		}
		lastScale = (nextScale == 0) ? lastScale : nextScale;
	}
}

void AVCParser::ParseSPS(const unsigned char* access_unit, unsigned size) throw()
{
	if (size < 4)
		return;
	if (*access_unit != 0x67)
		return;

	unsigned char* sps = (unsigned char*)malloc(size);
	memcpy(sps, access_unit + 1, size - 1);
	EBSP2RBSP(sps, size - 1);

	android::ABitReader br(sps, size - 1);
	profile = br.getBits(8);
	br.skipBits(8);
	profile_level = br.getBits(8);
	UE(&br); //seq_parameter_set_id

	chroma_format = ChromaTypes::ChromaFormat420; //default
	unsigned chroma_format_idc = 1;
	if (profile == 100 || profile == 110 ||
		profile == 122 || profile == 244 ||
		profile == 44 || profile == 83 || profile == 86) {
		chroma_format_idc = UE(&br);
		if (chroma_format_idc == 3)
			br.skipBits(1); //residual_colour_transform_flag
		if (chroma_format_idc <= 3)
			chroma_format = (ChromaTypes)chroma_format_idc;

		unsigned bit_depth_luma_minus8 = UE(&br);
		unsigned bit_depth_chroma_minus8 = UE(&br);
		bit_depth = bit_depth_luma_minus8 + 8;
		br.skipBits(1); //qpprime_y_zero_transform_bypass_flag
		if (br.getBits(1)) { //seq_scaling_matrix_present_flag
			for (unsigned i = 0; i < 8; i++) {
				if (br.getBits(1)) { //seq_scaling_list_present_flag[i]
					if (i < 6)
						skipScalingList(&br, 16);
					else
						skipScalingList(&br, 64);
				}
			}
		}
	}

	max_frame_num = UE(&br) + 4; //log2_max_frame_num_minus4
	unsigned pic_order_cnt_type = UE(&br);
	if (pic_order_cnt_type == 0) {
		max_pic_order_cnt = UE(&br) + 4; //log2_max_pic_order_cnt_lsb_minus4
	}else if (pic_order_cnt_type == 1) {
		br.skipBits(1); //delta_pic_order_always_zero_flag
		SE(&br); //offset_for_non_ref_pic
		SE(&br); //offset_for_top_to_bottom_field

		unsigned num_ref_frames_in_pic_order_cnt_cycle = UE(&br);
		for (unsigned i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; ++i)
			SE(&br); //offset_for_ref_frame
	}
	ref_frames = UE(&br);
	br.skipBits(1); //gaps_in_frame_num_value_allowed_flag

	unsigned pic_width_in_mbs_minus1 = UE(&br);
	unsigned pic_height_in_map_units_minus1 = UE(&br);

	unsigned frame_mbs_only_flag = br.getBits(1);
	unsigned mb_adaptive_frame_field_flag = 0;
	if (!frame_mbs_only_flag)
		mb_adaptive_frame_field_flag = br.getBits(1);
	else
		progressive_flag = 1;
	mbaff_flag = mb_adaptive_frame_field_flag;

	br.skipBits(1); //direct_8x8_inference_flag
	unsigned frame_cropping_flag = br.getBits(1);
	unsigned frame_crop_left_offset = 0, frame_crop_right_offset = 0, frame_crop_top_offset = 0, frame_crop_bottom_offset = 0;
	if (frame_cropping_flag) {
		frame_crop_left_offset = UE(&br);
		frame_crop_right_offset = UE(&br);
		frame_crop_top_offset = UE(&br);
		frame_crop_bottom_offset = UE(&br);
	}

	width = pic_width_in_mbs_minus1 * 16 + 16;
	height = (2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 * 16 + 16);
	unsigned cropUnitX = 0,cropUnitY = 0;
	if (chroma_format_idc == 0) {
		cropUnitX = 1;
		cropUnitY = 2 - frame_mbs_only_flag;
	}else{
		unsigned subWC = (chroma_format_idc == 3) ? 1 : 2;
		unsigned subHC = (chroma_format_idc == 1) ? 2 : 1;
		cropUnitX = subWC;
		cropUnitY = subHC * (2 - frame_mbs_only_flag);
	}
	width -= (frame_crop_left_offset + frame_crop_right_offset) * cropUnitX;
	height -= (frame_crop_top_offset + frame_crop_bottom_offset) * cropUnitY;

	sar_width = sar_height = 1;
	if (br.getBits(1)) { //vui_parameters_present_flag
		if (br.getBits(1)) { //aspect_ratio_info_present_flag
			unsigned aspect_ratio_idc = br.getBits(8);
			if (aspect_ratio_idc == 255) {
				sar_width = br.getBits(16);
				sar_height = br.getBits(16);
			}else{
				static const unsigned sw[] = {1,12,10,16,40,24,20,32,80,18,15,64,160,4,3,2};
				static const unsigned sh[] = {1,11,11,11,33,11,11,11,33,11,11,33,99,3,2,1};
				if (aspect_ratio_idc > 0 && aspect_ratio_idc < 16) {
					sar_width = sw[aspect_ratio_idc - 1];
					sar_height = sh[aspect_ratio_idc - 1];
				}
			}
		}

		if (br.getBits(1)) { //overscan_info_present_flag
			br.skipBits(1); //overscan_appropriate_flag
		}

		if (br.getBits(1)) { //video_signal_type_present_flag
			br.skipBits(3); //video_format
			br.skipBits(1); //video_full_range_flag
			if (br.getBits(1)) //colour_description_present_flag
				br.skipBits(24); //colour_primaries & transfer_characteristics & matrix_coefficients
		}

		if (br.getBits(1)) { //chroma_loc_info_present_flag
			UE(&br); //chroma_sample_loc_type_top_field
			UE(&br); //chroma_sample_loc_type_bottom_field
		}

		if (br.getBits(1)) { //timing_info_present_flag
			unsigned num_units_in_tick = br.getBits(32);
			unsigned time_scale = br.getBits(32);
			unsigned fixed_frame_rate_flag = fixed_fps_flag = br.getBits(1);
			fps_tick = num_units_in_tick * 2;
			fps_timescale = time_scale;
		}
	}
	free(sps);
}

void AVCParser::ParsePPS(const unsigned char* access_unit, unsigned size) throw()
{
	if (size < 2)
		return;
	if (*access_unit != 0x68)
		return;

	android::ABitReader br(access_unit + 1, size - 1);
	UE(&br); //pic_parameter_set_id
	UE(&br); //seq_parameter_set_id

	unsigned entropy_coding_mode_flag = br.getBits(1);
	br.skipBits(1); //pic_order_present_flag
	UE(&br); //num_slice_groups_minus1

	ec_type = EntropyCodingTypes::CodingModeCAVLC;
	if (entropy_coding_mode_flag)
		ec_type = EntropyCodingTypes::CodingModeCABAC;
}

void AVCParser::Parse(const unsigned char* buf, unsigned size) throw()
{
	if (*buf != 1)
		return;
	lengthSizeMinusOne = (buf[4] & 0x03) + 1;

	unsigned short sz = *(unsigned short*)(buf + 6);
	sz = _SWAP16(sz);
	ParseSPS(buf + 8, sz);

	const unsigned char* p = buf + 8 + sz;
	if (*p == 1)
	{
		p++;
		sz = *(unsigned short*)p;
		sz = _SWAP16(sz);
		ParsePPS(p + 2, sz);
	}
}

#ifdef _MSC_VER
#pragma warning(default:4146)
#endif