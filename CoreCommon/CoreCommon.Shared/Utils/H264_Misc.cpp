#include "H264AnnexBParser.h"

int H264_FindNALOffset(unsigned char* pb,unsigned len,H264_NALU_TYPE nal_type)
{
	H264AnnexBParser parser;
	if (!parser.InitFromStartCode(pb,len))
		return -1;

	if (parser.GetCurrentNaluType() == nal_type)
		return (int)parser.GetCurrentOffset();

	while (1)
	{
		if (!parser.ReadNext())
			return -1;

		if (parser.GetCurrentNaluType() == nal_type)
			return (int)parser.GetCurrentOffset();

		if (parser.IsParseEOF())
			break;
	}

	return -1;
}