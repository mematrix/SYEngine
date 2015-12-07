#include "stagefright\ABitReader.h"
#include "HVC1mkvToAnnexB.h"
#include <memory.h>

unsigned ParseHEVCDecoderConfigurationRecordHead(unsigned char* pb,unsigned len,HEVCDecoderConfigurationRecordHead* head)
{
	if (pb == nullptr || len < 28)
		return 0;
	if (head == nullptr)
		return 0;

	stagefright::ABitReader br(pb,len);
	head->configuration_version = br.getBits(8);
	head->general_profile_space = br.getBits(2);
	head->general_tier_flag = br.getBits(1);
	head->general_profile_idc = br.getBits(5);
	head->general_profile_compatibility_flag = br.getBits(32);
	head->general_progressive_source_flag = br.getBits(1);
	head->general_interlace_source_flag = br.getBits(1);
	head->general_non_packed_constraint_flag = br.getBits(1);
	head->general_frame_only_constraint_flag = br.getBits(1);
	br.skipBits(44);
	head->general_level_idc = br.getBits(8);
	br.skipBits(4);
	head->min_spatial_segmentation_idc = br.getBits(12);
	br.skipBits(6);
	head->Parallelism_type = br.getBits(2);
	br.skipBits(6);
	head->chroma_format_idc = br.getBits(2);
	br.skipBits(5);
	head->bit_depth_luma_minus8 = br.getBits(3);
	br.skipBits(5);
	head->bit_depth_chroma_minus8 = br.getBits(3);
	br.skipBits(18);
	head->max_sub_layers = br.getBits(3);
	head->temporal_id_nesting_flag = br.getBits(1);
	head->size_nalu_minus_one = br.getBits(2);

	return br.numBitsLeft();
}

unsigned StripHEVCNaluArrays(unsigned char* pb,unsigned char* nal_annexB,unsigned max_nal_size)
{
	if (*pb == 0)
		return 0;

	unsigned result = 0;
	unsigned char* pdst = nal_annexB;
	unsigned char* pdend = pdst + max_nal_size;

	unsigned count = *pb;
	unsigned char* p = pb + 1;
	for (unsigned i = 0;i < count;i++)
	{
		unsigned nal_unit_type = p[0] & 0x3F;
		if (p[1] != 0)
			return 0;
		unsigned nal_unit_count = p[2];
		p += 3;

		for (unsigned n = 0;n < nal_unit_count;n++)
		{
			unsigned short size = 0;
			unsigned char* ps = (unsigned char*)&size;
			ps[0] = p[1];
			ps[1] = p[0];
			p += 2;

			if ((pdst + 4 + size) > pdend)
				return 0xFFFFFFFF;

			pdst[0] = 0;
			pdst[1] = 0;
			pdst[2] = 0;
			pdst[3] = 1;
			pdst += 4;
			memcpy(pdst,p,size);
			pdst += size;

			p += size;
			result = result + 4 + size;
		}
	}

	return result;
}