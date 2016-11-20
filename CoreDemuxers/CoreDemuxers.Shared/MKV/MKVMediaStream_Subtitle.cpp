#include "MKVMediaStream.h"

bool MKVMediaStream::ProbeSubtitle(std::shared_ptr<MKVParser::MKVFileParser>& parser) throw()
{
	if (_info.Codec.CodecID[0] != 'S')
		return false;
	auto ct = _info.Codec.CodecType;
	if (ct != MatroskaCodecIds::MKV_Subtitle_Text_UTF8 &&
		ct != MatroskaCodecIds::MKV_Subtitle_Text_ASS &&
		ct != MatroskaCodecIds::MKV_Subtitle_Text_SSA)
		return false;

	memset(&_subtitle_info,0,sizeof(_subtitle_info));
	_subtitle_info.same_matroska = true;
	_subtitle_info.type = MediaSubtitleTextType::MEDIA_SUBTITLE_TEXT_UTF8;
	if (ct != MatroskaCodecIds::MKV_Subtitle_Text_UTF8)
	{
		//ass ssa.
		if (_info.Codec.CodecPrivateSize != 0)
		{
			_subtitle_info.head_info_size = _info.Codec.CodecPrivateSize;
			if (!_subtitle_head.Alloc(_info.Codec.CodecPrivateSize))
				return false;
			_subtitle_info.head_info = _subtitle_head.Get<unsigned char>();
			memcpy(_subtitle_info.head_info,_info.Codec.CodecPrivate,_info.Codec.CodecPrivateSize);
		}
	}

	_main_type = MediaMainType::MEDIA_MAIN_TYPE_SUBTITLE;
	_ssa_ass_format = true;
	switch (ct)
	{
	case MKV_Subtitle_Text_UTF8:
		_codec_type = MediaCodecType::MEDIA_CODEC_SUBTITLE_TEXT;
		_ssa_ass_format = false;
		break;
	case MKV_Subtitle_Text_SSA:
		_codec_type = MediaCodecType::MEDIA_CODEC_SUBTITLE_SSA;
		break;
	case MKV_Subtitle_Text_ASS:
		_codec_type = MediaCodecType::MEDIA_CODEC_SUBTITLE_ASS;
		break;
	}

	_frame_duration = 0.0;
	_stream_index = _info.Number;
	return true;
}