#ifndef __HVC1_MKV_TO_ANNEXB_H
#define __HVC1_MKV_TO_ANNEXB_H

#define HVC1_DCR_HEAD_LENGTH_BYTES 22

struct HEVCDecoderConfigurationRecordHead
{
	unsigned configuration_version; //8b
	unsigned general_profile_space; //2b
	unsigned general_tier_flag; //1b
	unsigned general_profile_idc; //5b
	unsigned general_profile_compatibility_flag; //32b
	unsigned general_progressive_source_flag; //1b
	unsigned general_interlace_source_flag; //1b
	unsigned general_non_packed_constraint_flag; //1b
	unsigned general_frame_only_constraint_flag; //1b
	//Reserved for 44b
	unsigned general_level_idc; //8b
	//Reserved for 4b
	unsigned min_spatial_segmentation_idc; //12b
	//Reserved for 6b
	unsigned Parallelism_type; //2b
	//Reserved for 6b
	unsigned chroma_format_idc; //2b
	//Reserved for 5b
	unsigned bit_depth_luma_minus8; //3b
	//Reserved for 5b
	unsigned bit_depth_chroma_minus8; //3b
	//Reserved for 18b
	unsigned max_sub_layers; //3b
	unsigned temporal_id_nesting_flag; //1b
	unsigned size_nalu_minus_one; //2b
};

unsigned ParseHEVCDecoderConfigurationRecordHead(unsigned char* pb,unsigned len,HEVCDecoderConfigurationRecordHead* head);
unsigned StripHEVCNaluArrays(unsigned char* pb,unsigned char* nal_annexB,unsigned max_nal_size = 2048);

#endif //__HVC1_MKV_TO_ANNEXB_H