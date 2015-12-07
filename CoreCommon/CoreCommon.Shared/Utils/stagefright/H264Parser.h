/*
 * H264 SPS Parser
 * Author: K.F Yang
*/

#ifndef H264PARSER_H_
#define H264PARSER_H_

#include "ABitReader.h"
#include "H264Golomb.h"

namespace stagefright {
namespace H264Parser {

struct AVCExtraInfo
{
	unsigned profile;
	unsigned profile_level;
	unsigned ref_frames;
	unsigned bit_depth; //+8
	unsigned width, height;
	unsigned ar_width, ar_height;
	unsigned fps_den, fps_num;
	enum ChromaSubsampling
	{
		ChromaFUnknown = 0,
		ChromaF420, //default
		ChromaF422,
		ChromaF444
	};
	ChromaSubsampling chroma_format;
	int progressive_flag;
	int fixed_fps_flag;
	int mbaff_flag; //if progressive_flag is no set.
	int full_range_flag;
	enum PictureCodingMode //from PPS.
	{
		CodingModeUnknown = 0,
		CodingModeCAVLC,
		CodingModeCABAC
	};
	PictureCodingMode coding_mode;
	unsigned slice_group_count;
};

void ParseH264SPS(unsigned char* pb,unsigned len,AVCExtraInfo* info);
void ParseH264PPS(unsigned char* pb,unsigned len,AVCExtraInfo* info);

}}

#endif