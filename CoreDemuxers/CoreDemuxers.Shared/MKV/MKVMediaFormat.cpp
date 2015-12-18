#include "MKVMediaFormat.h"
#ifdef _MSC_VER
#include <Windows.h>
#endif

#define _IO_BUFFER_SIZE (131072 * 4) //512KB.

AV_MEDIA_ERR MKVMediaFormat::Open(IAVMediaIO* io)
{
	if (io == nullptr)
		return AV_COMMON_ERR_INVALIDARG;
	if (_av_io != nullptr)
		return AV_COMMON_ERR_CHANGED_STATE;

	if (!RESET_AV_MEDIA_IO(io))
		return AV_IO_ERR_SEEK_FAILED;

	memset(&_global_info,0,sizeof(_global_info));
	auto parser = std::make_shared<MKVParser::MKVFileParser>(this);

	unsigned io_buf_size = _IO_BUFFER_SIZE;
#ifdef _MSC_VER
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	//auto adjust io buffer size for windows.
	MEMORYSTATUS ms = {};
	GlobalMemoryStatus(&ms);
	auto memsize = ms.dwTotalPhys / 1024 / 1024;
	if (memsize > 1024)
		io_buf_size *= 2;
	if (memsize > 2048)
		io_buf_size *= 2;
#endif
#endif

	if (io->IsAliveStream())
		io_buf_size = 64 * 1024;
	auto io_pool = std::make_shared<IOPoolReader>(io,_force_io_pool_size == 0 ? io_buf_size:_force_io_pool_size);
	_av_io = io_pool.get();

	auto ret = parser->Open(true,
		(_read_flags & MEDIA_FORMAT_READER_MATROSKA_DNT_PARSE_SEEKHEAD) != 0);
	_av_io = nullptr;
	if (ret != PARSER_MKV_OK)
		return AV_OPEN_ERR_PROBE_FAILED;

	if (!parser->GetGlobalInfo(&_global_info) || parser->GetTrackCount() == 0)
	{
		parser->Close();
		return AV_OPEN_ERR_HEADER_INVALID;
	}

	_av_io = io_pool.get();
	if (!MakeAllStreams(parser) || _stream_count == 0)
	{
		parser->Close();
		_av_io = nullptr;
		return AV_OPEN_ERR_STREAM_INVALID;
	}

	unsigned bitrate = 0;
	if (_global_info.Duration > 0.1)
		bitrate = (unsigned)((double)_av_io->GetSize() / _global_info.Duration * 8.0 * 0.99);

	unsigned first_vindex = 0;
	unsigned abr = GetAllAudioStreamsBitrate(&first_vindex);
	if (abr != 0 && first_vindex < _stream_count)
	{
		auto p = _streams[first_vindex]->GetVideoInfo();
		if (p != nullptr && bitrate != 0)
		{
			VideoBasicDescription desc = {};
			p->GetVideoDescription(&desc);
			if (bitrate > abr)
				desc.bitrate = bitrate - abr;
			p->ExternalUpdateVideoDescription(&desc);
		}
	}

	_io_pool = io_pool;
	_parser = parser;

	LoadMatroskaChapters(parser);
	LoadMatroskaAttachFiles(parser);
	return AV_ERR_OK;
}

AV_MEDIA_ERR MKVMediaFormat::Close()
{
	if (_av_io == nullptr)
		AV_CLOSE_ERR_NOT_OPENED;

	if (_parser)
		_parser->Close();
	memset(&_global_info,0,sizeof(_global_info));

	_parser.reset();
	_io_pool.reset();

	for (unsigned i = 0;i < _stream_count;i++)
		_streams[i].reset();
	_stream_count = 0;

	_pktBuffer.Free();
	return AV_ERR_OK;
}

void MKVMediaFormat::SetReadFlags(unsigned flags)
{
	_read_flags = flags;
	_skip_av_others_streams = false;
	if ((flags & MEDIA_FORMAT_READER_SKIP_AV_OUTSIDE) ||
		(flags & MEDIA_FORMAT_READER_SKIP_ALL_SUBTITLE))
		_skip_av_others_streams = true;
	if ((flags & MEDIA_FORMAT_READER_H264_FORCE_AVC1))
		_force_avc1 = true;
}

void* MKVMediaFormat::StaticCastToInterface(unsigned id)
{
	if (id == AV_MEDIA_INTERFACE_ID_CAST_CHAPTER)
		return static_cast<IAVMediaChapters*>(this);
	else if (id == AV_MEDIA_INTERFACE_ID_CAST_METADATA)
		return static_cast<IAVMediaMetadata*>(this);
	else if (id == AV_MEDIA_INTERFACE_ID_CAST_RESOURCE)
		return static_cast<IAVMediaResources*>(this);
	else if (id == AV_MEDIA_INTERFACE_ID_CASE_EX)
		return static_cast<IAVMediaFormatEx*>(this);
	return nullptr;
}

int MKVMediaFormat::GetKeyFramesCount()
{
	if (_parser == nullptr)
		return -1;
	if (_parser->InternalGetKeyFrames() == nullptr)
		return -1;
	return _parser->InternalGetKeyFrames()->GetCount();
}

unsigned MKVMediaFormat::CopyKeyFrames(double* copyTo,int copyCount)
{
	if (GetKeyFramesCount() < 1)
		return 0;
	if (copyTo == nullptr)
		return 0;

	unsigned count = copyCount == -1 ? 
		_parser->InternalGetKeyFrames()->GetCount():(unsigned)copyCount;
	if (count > _parser->InternalGetKeyFrames()->GetCount() || count == 0)
		return 0;

	unsigned trackNum;
	auto list = _parser->InternalGetKeyFrames(&trackNum);

	unsigned result = 0;
	auto time = copyTo;
	for (unsigned i = 0;i < count;i++)
	{
		auto p = list->GetItem(i);
		if (p->TrackNumber != (trackNum + 1) && !p->SubtitleTrack)
			continue;
		if (p->Time == 0.0)
			continue;

		*time = p->Time;
		time++;
		result++;
	}
	return result;
}