#include "stagefright/ABitReader.h"
#include "VC1HeadParser.h"

static const struct {int num,den;} kVC1AspectRatios[] = {
	{1,1},
	{12,11},
	{10,11},
	{16,11},
	{40,33},
	{24,11},
	{20,11},
	{32,11},
	{80,33},
	{18,11},
	{15,11},
	{64,33},
	{160,99},
	{0,0}
};

static const int kVC1FrameRateNr[] = {24,25,30,50,60,0};
static const int kVC1FrameRateDr[] = {1000,1001,0};

int FindVC1StartCodeOffset(unsigned char* pb,unsigned size,unsigned char find_type)
{
	//VC1 StartCode: 00 00 01 0X. (X = VC1_HEAD_CODE_***)
	for (unsigned i = 0;i < (size - SIZE_OF_VC1_HEAD_CODE);i++)
	{
		if (pb[i] != 0)
			continue;

		if (pb[i + 1] != 0 ||
			pb[i + 2] != 1)
			continue;

		auto type = pb[i + 3];
		if (type > VC1_HEAD_CODE_SEQ_HDR ||
			type < VC1_HEAD_CODE_END_OF_SEQ)
			continue;

		if (find_type == 0)
			return (int)i;
		else if (type == find_type)
			return (int)i;
	}

	return -1;
}

unsigned ParseVC1Header(unsigned char* pb,unsigned size,VC1_SEQUENCE_HEADER* head)
{
	if (pb == nullptr ||
		size < 8)
		return 0;

	int seq_head_offset = FindVC1StartCodeOffset(pb,size,VC1_HEAD_CODE_SEQ_HDR);
	if (seq_head_offset == -1)
		return 0;

	unsigned char* p = pb + seq_head_offset;
	if (size - seq_head_offset < 8)
		return 0;

	stagefright::ABitReader br(p + 4,size - seq_head_offset - 4);

	head->profile = br.getBits(2);
	if (head->profile != VC1P_Advanced)
		return 0;

	head->level = br.getBits(3);
	head->chroma_format = br.getBits(2);
	head->frame_rtq_postproc = br.getBits(3);
	head->bit_rtq_postproc = br.getBits(5);
	head->postproc_flag = br.getBits(1);
	head->max_coded_width = (br.getBits(12) + 1) * 2;
	head->max_coded_height = (br.getBits(12) + 1) * 2;
	head->pulldown_flag = br.getBits(1);
	head->interlace_flag = br.getBits(1);
	head->tf_counter_flag = br.getBits(1);
	head->f_inter_p_flag = br.getBits(1);
	br.skipBits(1);
	head->psf_mode_flag = br.getBits(1);

	head->display_info_flag = br.getBits(1);
	if (head->display_info_flag)
	{
		head->display_info.width = br.getBits(14) + 1;
		head->display_info.height = br.getBits(14) + 1;

		head->display_info.aspect_ratio_flag = br.getBits(1);
		if (head->display_info.aspect_ratio_flag)
		{
			int ar_index = br.getBits(4);
			if (ar_index > 0 && ar_index < 14)
			{
				head->display_info.aspect_ratio_width = kVC1AspectRatios[ar_index - 1].num;
				head->display_info.aspect_ratio_height = kVC1AspectRatios[ar_index - 1].den;
			}else if (ar_index == 15)
			{
				head->display_info.aspect_ratio_width = br.getBits(8);
				head->display_info.aspect_ratio_height = br.getBits(8);
			}else{
				head->display_info.aspect_ratio_flag = 0;
			}

			head->display_info.framerate_flag = br.getBits(1);
			if (head->display_info.framerate_flag)
			{
				if (br.getBits(1))
				{
					head->display_info.framerate_num = 32;
					head->display_info.framerate_den = br.getBits(16) + 1;
				}else{
					int nr = br.getBits(8);
					int dr = br.getBits(4);

					if (nr != 0 && nr < 7 &&
						dr != 0 && dr < 3) {
						head->display_info.framerate_num = kVC1FrameRateDr[dr - 1];
						head->display_info.framerate_den = kVC1FrameRateNr[nr - 1] * 1000;
					}else{
						head->display_info.framerate_flag = 0;
					}
				}
			}

			if (br.getBits(1))
			{
				head->display_info.color_prim = br.getBits(8);
				head->display_info.transfer_char = br.getBits(8);
				head->display_info.matrix_coef = br.getBits(8);
			}
		}
	}

	head->hrd_param_flag = br.getBits(1);
	if (head->hrd_param_flag)
	{
		head->hrd_params.hrd_num_leaky_buckets = br.getBits(5);
		head->hrd_params.bit_rate_exponent = br.getBits(4);
		head->hrd_params.buffer_size_exponent = br.getBits(4);
	}

	return br.numBitsLeft();
}