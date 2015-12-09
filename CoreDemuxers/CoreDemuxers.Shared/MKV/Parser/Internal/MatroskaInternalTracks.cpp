#include "MatroskaInternalTracks.h"

using namespace MKV;
using namespace MKV::Internal::Object;
using namespace MKV::Internal::Object::Context;

bool Tracks::ParseTracks(long long size)
{
	if (_head.GetDataSource() == nullptr)
		return false;

	long long endPos = DataSource::SizeToAbsolutePosition(size,_head.GetDataSource());
	while (1)
	{
		if (DataSource::IsCurrentPosition(endPos,_head.GetDataSource()))
		{
			DataSource::ResetInSkipOffsetError(endPos,_head.GetDataSource());
			break;
		}

		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO(_head),true))
			return false;

		if (!_head.MatchId(MKV_ID_L2_TRACK_TRACKENTRY))
		{
			_head.Skip();
			continue;
		}

		TrackEntry entry;
		entry.ToDefault();
		if (!ParseTrackEntry(entry,_head.Size()))
			return false;

		_list.AddItem(&entry);
	}

	return true;
}

static const unsigned TrackEntryUIntIdListCount = 14;
static const unsigned TrackEntryUIntIdList[] = {MKV_ID_L3_TRACK_TRACKNUMBER,
	MKV_ID_L3_TRACK_TRACKUID,MKV_ID_L3_TRACK_TRACKTYPE,
	MKV_ID_L3_TRACK_TRACKFLAGENABLED,
	MKV_ID_L3_TRACK_TRACKFLAGDEFAULT,
	MKV_ID_L3_TRACK_TRACKFLAGFORCED,
	MKV_ID_L3_TRACK_TRACKFLAGLACING,
	MKV_ID_L3_TRACK_TRACKMINCACHE,
	MKV_ID_L3_TRACK_TRACKMAXCACHE,
	MKV_ID_L3_TRACK_TRACKDEFAULTDURATION,
	MKV_ID_L3_TRACK_TRACKMAXBLKADDID,
	MKV_ID_L3_TRACK_CODECDELAY,
	MKV_ID_L3_TRACK_SEEKPREROLL,
	MKV_ID_L3_TRACK_CODECDECODEALL};

static const unsigned TrackAudioUIntIdListCount = 2;
static const unsigned TrackAudioUIntIdList[] = {MKV_ID_L4_TRACK_AUDIO_BITDEPTH,
	MKV_ID_L4_TRACK_AUDIO_CHANNELS};

static const unsigned TrackVideoUIntIdListCount = 14;
static const unsigned TrackVideoUIntIdList[] = {MKV_ID_L4_TRACK_VIDEO_FRAMERATE,
	MKV_ID_L4_TRACK_VIDEO_DISPLAYWIDTH,
	MKV_ID_L4_TRACK_VIDEO_DISPLAYHEIGHT,
	MKV_ID_L4_TRACK_VIDEO_PIXELWIDTH,
	MKV_ID_L4_TRACK_VIDEO_PIXELHEIGHT,
	MKV_ID_L4_TRACK_VIDEO_PIXELCROPB,
	MKV_ID_L4_TRACK_VIDEO_PIXELCROPT,
	MKV_ID_L4_TRACK_VIDEO_PIXELCROPL,
	MKV_ID_L4_TRACK_VIDEO_PIXELCROPR,
	MKV_ID_L4_TRACK_VIDEO_DISPLAYUNIT,
	MKV_ID_L4_TRACK_VIDEO_FLAGINTERLACED,
	MKV_ID_L4_TRACK_VIDEO_STEREOMODE,
	MKV_ID_L4_TRACK_VIDEO_ALPHAMODE,
	MKV_ID_L4_TRACK_VIDEO_ASPECTRATIO};

bool Tracks::ParseTrackEntry(Context::TrackEntry& entry,long long size)
{
	long long endPos = DataSource::SizeToAbsolutePosition(size,_head.GetDataSource());
	while (1)
	{
		if (DataSource::IsCurrentPosition(endPos,_head.GetDataSource()))
		{
			DataSource::ResetInSkipOffsetError(endPos,_head.GetDataSource());
			break;
		}

		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO(_head),true))
			return false;

		EBML::EBML_VALUE_DATA value;
		if (_head.MatchIdList(TrackEntryUIntIdList,TrackEntryUIntIdListCount))
		{
			if (!EBML::EbmlReadValue(_head,value))
				return false;
		}

		switch (_head.Id())
		{
		case MKV_ID_L3_TRACK_TRACKNUMBER:
			entry.TrackNumber = value.Ui32;
			break;
		case MKV_ID_L3_TRACK_TRACKTYPE:
			entry.TrackType = value.Ui32;
			break;
		case MKV_ID_L3_TRACK_TRACKUID:
			entry.TrackUID = value.Ui64;
			break;

		case MKV_ID_L3_TRACK_TRACKTIMECODESCALE: //DEPRECATED.
			if (!EBML::EbmlReadValue(_head,value,true))
				return false;
			entry.TrackTimecodeScale = value.Float;
			break;

		case MKV_ID_L3_TRACK_TRACKFLAGENABLED:
			entry.fEnabled = value.UIntToBool();
			break;
		case MKV_ID_L3_TRACK_TRACKFLAGDEFAULT:
			entry.fDefault = value.UIntToBool();
			break;
		case MKV_ID_L3_TRACK_TRACKFLAGFORCED:
			entry.fForced = value.UIntToBool();
			break;
		case MKV_ID_L3_TRACK_TRACKFLAGLACING:
			entry.fLacing = value.UIntToBool();
			break;

		case MKV_ID_L3_TRACK_TRACKMINCACHE:
			entry.MinCache = value.Ui32;
			break;
		case MKV_ID_L3_TRACK_TRACKMAXCACHE:
			entry.MaxCache = value.Ui32;
			break;

		case MKV_ID_L3_TRACK_TRACKDEFAULTDURATION:
			entry.DefaultDuration = value.Ui64;
			break;
		case MKV_ID_L3_TRACK_TRACKMAXBLKADDID:
			entry.MaxBlockAdditionID = value.Ui64;
			break;
		case MKV_ID_L3_TRACK_CODECDECODEALL:
			entry.CodecDecodeAll = value.UIntToBool();
			break;

		case MKV_ID_L3_TRACK_TRACKNAME:
			if (!entry.Track.SetName(_head))
				return false;
			break;
		case MKV_ID_L3_TRACK_TRACKLANGUAGE:
			if (!EbmlReadStringSafe(_head,entry.Track.Language))
				return false;
			break;

		case MKV_ID_L3_TRACK_CODECID:
			if (!EbmlReadStringSafe(_head,entry.Track.CodecID))
				return false;
			break;
		case MKV_ID_L3_TRACK_CODECPRIVATE:
			if (!entry.Track.SetCodecPrivate(_head))
				return false;
			break;

		case MKV_ID_L3_TRACK_CODECDELAY: //v4
			entry.Track.CodecDelay = value.Ui64;
			break;
		case MKV_ID_L3_TRACK_SEEKPREROLL: //v4
			entry.Track.SeekPreroll = value.Ui64;
			break;

		default:
			if (_head.MatchId(MKV_ID_L3_TRACK_TRACKVIDEO))
			{
				if (!ParseVideoTrack(entry.Video,_head.Size()))
					return false;
			}else if (_head.MatchId(MKV_ID_L3_TRACK_TRACKAUDIO))
			{
				if (!ParseAudioTrack(entry.Audio,_head.Size()))
					return false;
			}else if (_head.MatchId(MKV_ID_L3_TRACK_TRACKCONTENTENCODINGS))
			{
				if (!ParseEncodings(entry,_head.Size()))
					return false;
			}else{
				_head.Skip();
			}
		}
	}

	return true;
}

bool Tracks::ParseAudioTrack(Context::AudioTrack& audio,long long size)
{
	long long endPos = DataSource::SizeToAbsolutePosition(size,_head.GetDataSource());
	while (1)
	{
		if (DataSource::IsCurrentPosition(endPos,_head.GetDataSource()))
		{
			DataSource::ResetInSkipOffsetError(endPos,_head.GetDataSource());
			break;
		}

		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO(_head),true))
			return false;

		EBML::EBML_VALUE_DATA value;
		if (_head.MatchIdList(TrackAudioUIntIdList,TrackAudioUIntIdListCount))
		{
			if (!EBML::EbmlReadValue(_head,value))
				return false;
		}

		switch (_head.Id())
		{
		case MKV_ID_L4_TRACK_AUDIO_SAMPLINGFREQ:
		case MKV_ID_L4_TRACK_AUDIO_OUTSAMPLINGFREQ:
			if (!EBML::EbmlReadValue(_head,value,true))
				return false;

			if (_head.Id() == MKV_ID_L4_TRACK_AUDIO_SAMPLINGFREQ)
				audio.SamplingFrequency = value.Float;
			else
				audio.OutputSamplingFrequency = value.Float;
			break;

		case MKV_ID_L4_TRACK_AUDIO_BITDEPTH:
			audio.BitDepth = value.Ui32;
			break;
		case MKV_ID_L4_TRACK_AUDIO_CHANNELS:
			audio.Channels = value.Ui32;
			break;

		default:
			_head.Skip();
		}
	}

	return true;
}

bool Tracks::ParseVideoTrack(Context::VideoTrack& video,long long size)
{
	long long endPos = DataSource::SizeToAbsolutePosition(size,_head.GetDataSource());
	while (1)
	{
		if (DataSource::IsCurrentPosition(endPos,_head.GetDataSource()))
		{
			DataSource::ResetInSkipOffsetError(endPos,_head.GetDataSource());
			break;
		}

		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO(_head),true))
			return false;

		EBML::EBML_VALUE_DATA value;
		if (_head.MatchIdList(TrackVideoUIntIdList,TrackVideoUIntIdListCount))
		{
			if (!EBML::EbmlReadValue(_head,value))
				return false;
		}

		switch (_head.Id())
		{
		case MKV_ID_L4_TRACK_VIDEO_COLORSPACE:
			if (_head.Size() > 4) {
				_head.Skip();
			}else{
				if (!EBML::EbmlReadBinary(_head,video.ColourSpace))
					return false;
			}
			break;

		case MKV_ID_L4_TRACK_VIDEO_FLAGINTERLACED:
			video.fInterlaced = value.UIntToBool();
			break;

		case MKV_ID_L4_TRACK_VIDEO_STEREOMODE:
			video.StereoMode = value.Ui32;
			break;
		case MKV_ID_L4_TRACK_VIDEO_ALPHAMODE:
			video.AlphaMode = value.Ui32;
			break;
		case MKV_ID_L4_TRACK_VIDEO_ASPECTRATIO:
			video.AspectRatioType = value.Ui32;
			break;

		case MKV_ID_L4_TRACK_VIDEO_PIXELWIDTH:
			video.PixelWidth= value.Ui32;
			break;
		case MKV_ID_L4_TRACK_VIDEO_PIXELHEIGHT:
			video.PixelHeight = value.Ui32;
			break;

		case MKV_ID_L4_TRACK_VIDEO_PIXELCROPB:
			video.PixelCropBottom = value.Ui32;
			break;
		case MKV_ID_L4_TRACK_VIDEO_PIXELCROPT:
			video.PixelCropTop = value.Ui32;
			break;
		case MKV_ID_L4_TRACK_VIDEO_PIXELCROPL:
			video.PixelCropLeft = value.Ui32;
			break;
		case MKV_ID_L4_TRACK_VIDEO_PIXELCROPR:
			video.PixelCropRight = value.Ui32;
			break;

		case MKV_ID_L4_TRACK_VIDEO_DISPLAYWIDTH:
			video.DisplayWidth  = value.Ui32;
			break;
		case MKV_ID_L4_TRACK_VIDEO_DISPLAYHEIGHT:
			video.DisplayHeight = value.Ui32;
			break;
		case MKV_ID_L4_TRACK_VIDEO_DISPLAYUNIT:
			video.DisplayUnit = value.Ui32;
			break;

		default:
			_head.Skip();
		}
	}

	return true;
}

bool Tracks::ParseEncodings(Context::TrackEntry& entry,long long size)
{
	int count_of_cncodings = 0;

	long long endPos = DataSource::SizeToAbsolutePosition(size,_head.GetDataSource());
	while (1)
	{
		if (count_of_cncodings > 1)
			return false; //不支持多个

		if (DataSource::IsCurrentPosition(endPos,_head.GetDataSource()))
		{
			DataSource::ResetInSkipOffsetError(endPos,_head.GetDataSource());
			break;
		}

		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO(_head),true))
			return false;

		if (!_head.MatchId(MKV_ID_L4_TRACK_TRACKCONTENTENCODING))
		{
			_head.Skip();
			continue;
		}

		if (!ParseEncoding(entry.Encoding,_head.Size()))
			return false;

		count_of_cncodings++;
	}

	return true;
}

bool Tracks::ParseEncoding(Context::TrackEncoding& enc,long long size)
{
	long long endPos = DataSource::SizeToAbsolutePosition(size,_head.GetDataSource());
	while (1)
	{
		if (DataSource::IsCurrentPosition(endPos,_head.GetDataSource()))
		{
			DataSource::ResetInSkipOffsetError(endPos,_head.GetDataSource());
			break;
		}

		if (!EBML::FastParseEbmlHeadIfSizeWithIfSpec(FAST_PARSE_WITH_IO(_head),true,MKV_ID_L5_TRACK_ENCODINGCOMPRESSION))
			return false;

		EBML::EBML_VALUE_DATA value;
		switch (_head.Id())
		{
		case MKV_ID_L5_TRACK_ENCODINGSCOPE:
		case MKV_ID_L5_TRACK_ENCODINGTYPE:
			if (!EBML::EbmlReadValue(_head,value,true))
				return false;

			if (_head.Id() == MKV_ID_L5_TRACK_ENCODINGSCOPE)
				enc.ContentEncodingScope = value.Ui32;
			else
				enc.ContentEncodingType = value.Ui32;
			break;

		case MKV_ID_L5_TRACK_ENCODINGCOMPRESSION:
			if (_head.Size() != 0)
				if (!ParseCompression(enc.Compression,_head.Size()))
					return false;
			enc.fCompressionExists = true;
			break;
		case MKV_ID_L5_TRACK_ENCODINGENCRYPTION:
			if (!ParseEncryption(enc.Encryption,_head.Size()))
				return false;
			enc.fEncryptionExists = true;
			break;

		default:
			_head.Skip();
		}
	}

	return true;
}

bool Tracks::ParseCompression(Context::TrackCompression& comp,long long size)
{
	long long endPos = DataSource::SizeToAbsolutePosition(size,_head.GetDataSource());
	while (1)
	{
		if (DataSource::IsCurrentPosition(endPos,_head.GetDataSource()))
		{
			DataSource::ResetInSkipOffsetError(endPos,_head.GetDataSource());
			break;
		}

		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO(_head),true))
			return false;

		EBML::EBML_VALUE_DATA value;
		switch (_head.Id())
		{
		case MKV_ID_L6_TRACK_ENCODINGCOMPALGO:
			if (!EBML::EbmlReadValue(_head,value,true))
				return false;
			comp.ContentCompAlgo = value.Ui32;
			break;
		case MKV_ID_L6_TRACK_ENCODINGCOMPSETTINGS:
			if (!comp.SetContentCompSettings(_head))
				return false;
			break;

		default:
			_head.Skip();
		}
	}

	return true;
}