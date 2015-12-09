#include "FLVMediaFormat.h"

bool FLVMediaFormat::FindAndMakeKeyFrames()
{
	if (_parser->Reset() != PARSER_FLV_OK)
		return false;

	shared_memory_ptr<FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX> 
		index(sizeof(FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX) * 1024);

	unsigned count = 0;
	unsigned size = 1024;
	while (1)
	{
		FLVParser::FLV_STREAM_PACKET packet = {};
		if (_parser->ReadNextPacketFast(&packet) != PARSER_FLV_OK)
			break;

		if (packet.type != FLVParser::PacketTypeVideo ||
			packet.key_frame == 0 ||
			packet.data_size == 0)
			continue;

		(index.ptr() + count)->time = (double)packet.timestamp / (double)1000.0;
		(index.ptr() + count)->pos = _av_io->Tell() - packet.data_size - sizeof(FLVParser::FLV_TAG);

		count++;

		if (count == size)
		{
			size *= 2;
			size *= sizeof(FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX);

			if (!index.realloc(size))
			{
				count = 0;
				break;
			}
		}
	}

	if (count > 0)
	{
		_keyframe_count = count;
		_keyframe_index = (FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX*)
			malloc((count + 1) * sizeof(FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX));

		memcpy(_keyframe_index,index.ptr(),count * sizeof(FLVParser::FLV_INTERNAL_KEY_FRAME_INDEX));
	}

	_parser->Reset();
	return true;
}