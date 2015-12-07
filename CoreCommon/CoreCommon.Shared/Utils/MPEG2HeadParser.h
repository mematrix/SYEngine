#ifndef __MPEG2_HEAD_PARSER_H
#define __MPEG2_HEAD_PARSER_H

//http://www.cnblogs.com/xkfz007/articles/2613654.html
//MPEG-PS Support: MPEG1\2 AV,AAC,MPEG4(XVID,DIVX),AC3 and DTS.

#define SIZE_OF_MPEG2_START_CODE 4

#define MPEG2_START_CODE_FRAME 0x00
#define MPEG2_START_CODE_FRAME_SLICE 0x01
#define MPEG2_START_CODE_FRAME_SLICE_END 0xAF

#define MPEG2_START_CODE_SKIP0 0xB0
#define MPEG2_START_CODE_SKIP1 0xB1
#define MPEG2_START_CODE_USER_DATA 0xB2
#define MPEG2_START_CODE_SEQ_HEAD 0xB3
#define MPEG2_START_CODE_SEQ_ERROR 0xB4
#define MPEG2_START_CODE_SEQ_HEAD_EX 0xB5
#define MPEG2_START_CODE_SKIP6 0xB6
#define MPEG2_START_CODE_SEQ_HEAD_END 0xB7
#define MPEG2_START_CODE_GOP 0xB8

#define MPEG2_PROFILE_422 0
#define MPEG2_PROFILE_HIGH 1
#define MPEG2_PROFILE_SS 2
#define MPEG2_PROFILE_SNR 3
#define MPEG2_PROFILE_MAIN 4
#define MPEG2_PROFILE_SIMPLE 5

#define MPEG2_PROFILE_LEVEL_LOW 10
#define MPEG2_PROFILE_LEVEL_MAIN 8
#define MPEG2_PROFILE_LEVEL_HIGH1440 6
#define MPEG2_PROFILE_LEVEL_HIGH 4

struct MPEG2_SEQUENCE_HEADER
{
	int horizontal_size_value;
	int vertical_size_value;
	int aspect_ratio_infomation;
	int frame_rate_code;
	int bit_rate_value;
	int marker_bit;
	int vbv_buffer_size_value;
	int constrained_parameters_flag;
	int load_intra_quantiser_matrix;
	int load_non_intra_quantiser_matrix;
};

struct MPEG2_SEQUENCE_HEADER_EX
{
	int profile_and_level_indication;
	int progressive_sequence;
	int chroma_format;
	int horizontal_size_extension;
	int vertical_size_extension;
	int bit_rate_extension;
	int marker_bit;
	int vbv_buffer_size_extension;
	int low_delay;
	int frame_rate_extension_n;
	int frame_rate_extension_d;
};

enum MPEG2Profile
{
	MPEG2P_Others = 0,
	MPEG2P_Simple,
	MPEG2P_Main,
	MPEG2P_High,
	MPEG2P_422
};

enum MPEG2Level
{
	MPEG2L_Unknown = 0,
	MPEG2L_Low,
	MPEG2L_Main,
	MPEG2L_High1440,
	MPEG2L_High
};

enum MPEG2Colors
{
	MPEG2C_Error,
	MPEG2C_420,
	MPEG2C_422,
	MPEG2C_444
};

struct MPEG2Headers
{
	MPEG2_SEQUENCE_HEADER head;
	MPEG2_SEQUENCE_HEADER_EX ex;
	int _fps_den, _fps_num;
	int _ar_den, _ar_num;
	int _bitrate;
	MPEG2Colors _colors;
	MPEG2Profile _profile;
	MPEG2Level _level;
};

int FindMPEG2StartCodeOffset(unsigned char* pb,unsigned size,unsigned type);

void ParseMPEG2Headers(unsigned char* pb,unsigned size,MPEG2Headers* head);
void InitMPEG2Headers(MPEG2Headers* head);

#endif //__MPEG2_HEAD_PARSER_H