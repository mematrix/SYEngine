#ifndef __AVC_PARSER_H
#define __AVC_PARSER_H

struct AVCParser
{
	unsigned profile;
	unsigned profile_level;
	unsigned ref_frames;
	unsigned bit_depth; //+8
	unsigned max_frame_num; //+4
	unsigned max_pic_order_cnt; //+4
	unsigned width, height;
	unsigned sar_width, sar_height;
	unsigned fps_tick, fps_timescale; //if fixed_fps_flag is set, tick * 2.
	enum ChromaTypes
	{
		ChromaFormatUnk = 0,
		ChromaFormat420 = 1, //default
		ChromaFormat422 = 2,
		ChromaFormat444 = 3
	};
	ChromaTypes chroma_format;
	int fixed_fps_flag;
	int progressive_flag;
	int mbaff_flag; //if progressive_flag is no set.
	enum EntropyCodingTypes //from PPS
	{
		CodingModeCAVLC,
		CodingModeCABAC
	};
	EntropyCodingTypes ec_type;
	unsigned lengthSizeMinusOne;

	void ParseSPS(unsigned char* access_unit, unsigned size) throw();
	void ParsePPS(unsigned char* access_unit, unsigned size) throw();

	void Parse(unsigned char* buf, unsigned size) throw();
};

#endif //__AVC_PARSER_H