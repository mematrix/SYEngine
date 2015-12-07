#ifndef __VC1_HEAD_PARSER_H
#define __VC1_HEAD_PARSER_H

#define SIZE_OF_VC1_HEAD_CODE 4

#define VC1_HEAD_CODE_END_OF_SEQ 0x0A
#define VC1_HEAD_CODE_SLICE 0x0B
#define VC1_HEAD_CODE_FIELD 0x0C
#define VC1_HEAD_CODE_FRAME 0x0D
#define VC1_HEAD_CODE_EP 0x0E
#define VC1_HEAD_CODE_SEQ_HDR 0x0F

#define VC1_PICTURE_MODE_PROGRESSIVE 0
#define VC1_PICTURE_MODE_ILACE_FRAME 2
#define VC1_PICTURE_MODE_ILACE_FIELD 3

#define VC1_KEYFRAME_PIC_TYPE 6

enum VC1Profile
{
	VC1P_Sample = 0,
	VC1P_Main,
	VC1P_Complex,
	VC1P_Advanced
};

struct VC1_SEQ_HEAD_DISPLAY
{
	int width; //+1
	int height; //+1
	int aspect_ratio_flag;
	int aspect_ratio_width;
	int aspect_ratio_height;
	int framerate_flag;
	int framerate_num;
	int framerate_den;
	int color_prim;
	int transfer_char;
	int matrix_coef;
};

struct VC1_SRQ_HEAD_HRD_PARAMS
{
	int hrd_num_leaky_buckets;
	int bit_rate_exponent;
	int buffer_size_exponent;
};

struct VC1_SEQUENCE_HEADER
{
	int profile;
	int level;
	int chroma_format;
	int frame_rtq_postproc;
	int bit_rtq_postproc;
	int postproc_flag;
	int max_coded_width;
	int max_coded_height;
	int pulldown_flag;
	int interlace_flag;
	int tf_counter_flag;
	int f_inter_p_flag;
	//1bit is reserved.
	int psf_mode_flag;
	int display_info_flag;
	VC1_SEQ_HEAD_DISPLAY display_info;
	int hrd_param_flag;
	VC1_SRQ_HEAD_HRD_PARAMS hrd_params;
};

int FindVC1StartCodeOffset(unsigned char* pb,unsigned size,unsigned char find_type);
unsigned ParseVC1Header(unsigned char* pb,unsigned size,VC1_SEQUENCE_HEADER* head);

inline int GetVC1FramePType(unsigned char* pb)
{
	return (*pb & 0xE0) >> 5;
}

#endif //__VC1_HEAD_PARSER_H