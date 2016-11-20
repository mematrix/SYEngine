#include <stagefright/ABitReader.h>
#include "MPEG2HeadParser.h"

int FindMPEG2StartCodeOffset(unsigned char* pb,unsigned size,unsigned type)
{
	if (pb == nullptr ||
		size < SIZE_OF_MPEG2_START_CODE)
		return -1;

	decltype(pb) p = pb;
	while (1)
	{
		if (p >= (pb + size - SIZE_OF_MPEG2_START_CODE))
			break;

		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
		{
			auto code = p[3];
			if (code <= MPEG2_START_CODE_GOP)
			{
				if (type == 0xFF)
					return p - pb + SIZE_OF_MPEG2_START_CODE;

				if (code == type)
					return p - pb + SIZE_OF_MPEG2_START_CODE;
			}

			p += 3;
		}

		p++;
	}

	return -1;
}

void ParseMPEG2Headers(unsigned char* pb,unsigned size,MPEG2Headers* head)
{
	head->head.marker_bit = 0;
	head->ex.marker_bit = 0;

	int seq_head_offset = FindMPEG2StartCodeOffset(pb,size,MPEG2_START_CODE_SEQ_HEAD);
	if (seq_head_offset == -1)
		return;

	stagefright::ABitReader br(pb + seq_head_offset,size - seq_head_offset);
	MPEG2_SEQUENCE_HEADER& seq_head = head->head;

	seq_head.horizontal_size_value = br.getBits(12);
	seq_head.vertical_size_value = br.getBits(12);
	seq_head.aspect_ratio_infomation = br.getBits(4);
	seq_head.frame_rate_code = br.getBits(4);
	seq_head.bit_rate_value = br.getBits(18);
	seq_head.marker_bit = br.getBits(1);
	seq_head.vbv_buffer_size_value = br.getBits(10);
	seq_head.constrained_parameters_flag = br.getBits(1);

	int seq_head_ex_offset = -1;
	for (unsigned i = 0;i < 2;i++)
	{
		int offset = 0;
		int temp = FindMPEG2StartCodeOffset(pb + offset,size - offset,MPEG2_START_CODE_SEQ_HEAD_EX);
		if (temp == -1)
			break;

		offset += temp;

		//extension_start_code_identifier (0 = Sequence_Display_Extension, 1 = Sequence_Extension)
		if (((pb[offset] & 0xF0) >> 4) == 1)
		{
			seq_head_ex_offset = offset;
			break;
		}
	}

	if (seq_head_ex_offset != -1)
	{
		stagefright::ABitReader br(pb + seq_head_ex_offset,size - seq_head_ex_offset);
		MPEG2_SEQUENCE_HEADER_EX& seq_head_ex = head->ex;

		br.skipBits(4); //extension_start_code_identifier
		seq_head_ex.profile_and_level_indication = br.getBits(8);
		seq_head_ex.progressive_sequence = br.getBits(1);
		seq_head_ex.chroma_format = br.getBits(2);
		seq_head_ex.horizontal_size_extension = br.getBits(2);
		seq_head_ex.vertical_size_extension = br.getBits(2);
		seq_head_ex.bit_rate_extension = br.getBits(12);
		seq_head_ex.marker_bit = br.getBits(1);
		seq_head_ex.vbv_buffer_size_extension = br.getBits(8);
		seq_head_ex.low_delay = br.getBits(1);
		seq_head_ex.frame_rate_extension_n = br.getBits(2);
		seq_head_ex.frame_rate_extension_d = br.getBits(5);
	}
}

void InitMPEG2Headers(MPEG2Headers* head)
{
	if (head->head.marker_bit)
	{
		head->_bitrate = head->head.bit_rate_value * 400;

		switch (head->head.aspect_ratio_infomation)
		{
		case 1: //1:1
			head->_ar_den = head->_ar_num = 1;
			break;
		case 2: //4:3
			head->_ar_num = 4;
			head->_ar_den = 3;
			break;
		case 3: //16:9
			head->_ar_num = 16;
			head->_ar_den = 9;
			break;
		default:
			head->_ar_den = head->_ar_num = 1;
		}

		switch (head->head.frame_rate_code)
		{
		case 1: //23.976
			head->_fps_num = 10000000;
			head->_fps_den = 417071;
			break;
		case 2: //24.00
			head->_fps_num = 24000;
			head->_fps_den = 1000;
			break;
		case 3: //25.00
			head->_fps_num = 25000;
			head->_fps_den = 1000;
			break;
		case 4: //29.97
			head->_fps_num = 30000;
			head->_fps_den = 1001;
			break;
		case 5: //30.00
			head->_fps_num = 30000;
			head->_fps_den = 1000;
			break;
		case 6: //50.00
			head->_fps_num = 50000;
			head->_fps_den = 1000;
			break;
		case 7: //59.94
			head->_fps_num = 60000;
			head->_fps_den = 1001;
			break;
		case 8: //60.00
			head->_fps_num = 60000;
			head->_fps_den = 1000;
			break;
		default:
			head->_fps_den = head->_fps_num = 0;
		}
	}

	if (head->ex.marker_bit)
	{
		int c = head->ex.chroma_format;
		head->_colors = 
			c == 1 ? MPEG2C_420:
			c == 2 ? MPEG2C_422:
			c == 3 ? MPEG2C_444:
			MPEG2C_Error;

		int profile = (head->ex.profile_and_level_indication & 0xF0) >> 4;
		int level = head->ex.profile_and_level_indication & 0x0F;

		head->_profile = 
			profile == MPEG2_PROFILE_HIGH ? MPEG2P_High:
			profile == MPEG2_PROFILE_MAIN ? MPEG2P_Main:
			profile == MPEG2_PROFILE_SIMPLE ? MPEG2P_Simple:
			profile == MPEG2_PROFILE_422 ? MPEG2P_422:
			MPEG2P_Others;

		head->_level = 
			level == MPEG2_PROFILE_LEVEL_LOW ? MPEG2L_Low:
			level == MPEG2_PROFILE_LEVEL_MAIN ? MPEG2L_Main:
			level == MPEG2_PROFILE_LEVEL_HIGH ? MPEG2L_High:
			level == MPEG2_PROFILE_LEVEL_HIGH1440 ? MPEG2L_High1440:
			MPEG2L_Unknown;

		if (head->_fps_den == 0 ||
			head->_fps_num == 0) {
			head->_fps_num = head->ex.frame_rate_extension_n;
			head->_fps_den = head->ex.frame_rate_extension_d;
		}
	}
}