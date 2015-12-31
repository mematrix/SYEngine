#include "DemuxProxy.h"
#include "DemuxLavf.h"

bool DemuxProxy::OpenByteStream(int io_buf_size, bool non_find_stream_info)
{
	av_register_all();

	if (_core.IOContext != NULL ||
		_core.FormatContext != NULL)
		return false;
	
	if (_core.NowPacket == NULL)
		_core.NowPacket = malloc(sizeof AVPacket);
	if (_core.NowPacket == NULL)
		return false;

	av_init_packet((AVPacket*)_core.NowPacket);
	int avio_buffer_size = (io_buf_size == 0 ? 1024 * 64 : io_buf_size); //default is 64K
	unsigned char* avio_buf = (unsigned char*)av_malloc(avio_buffer_size + FF_INPUT_BUFFER_PADDING_SIZE);
	if (avio_buf == NULL)
		return false;

	_core.FormatContext = avformat_alloc_context();
	_core.IOContext = avio_alloc_context(avio_buf, avio_buffer_size, 0,
		this, &DemuxProxy::avioRead, NULL, &DemuxProxy::avioSeek);
	if (_core.FormatContext == NULL ||
		_core.IOContext == NULL)
		return false;

	AVFormatContext* format = (AVFormatContext*)_core.FormatContext;
	format->pb = (AVIOContext*)_core.IOContext;
	int result = avformat_open_input((AVFormatContext**)&_core.FormatContext, "", NULL, NULL);
	if (result < 0)
		return false;

	if (!non_find_stream_info) //mkv、mp4。。。不需要
		result = avformat_find_stream_info(format, NULL);
	if (non_find_stream_info) {
		if (strstr(format->iformat->name, "matroska") == NULL &&
			strstr(format->iformat->name, "mov") == NULL)
			result = avformat_find_stream_info(format, NULL);
	}
	if (result < 0)
		return false;

	_format_name = format->iformat->name;
	if (format->nb_streams == 0)
		return false;
	_core.TotalTracks = format->nb_streams;
	_core.TotalDuration = double(format->duration) / double(AV_TIME_BASE);
	return true;
}

void DemuxProxy::CloseByteStream()
{
	if (_core.NowPacket)
		av_free_packet((AVPacket*)_core.NowPacket);
	if (_core.IOContext)
		av_free(((AVIOContext*)_core.IOContext)->buffer),
		av_free(_core.IOContext);
	if (_core.FormatContext)
		avformat_close_input((AVFormatContext**)&_core.FormatContext);
	memset(&_core, 0, sizeof(_core));
}

double DemuxProxy::GetStartTime()
{
	if (((AVFormatContext*)_core.FormatContext)->start_time == 0 ||
		((AVFormatContext*)_core.FormatContext)->start_time == AV_NOPTS_VALUE)
		return 0.0;
	return ((double)((AVFormatContext*)_core.FormatContext)->start_time / double(AV_TIME_BASE));
}

bool DemuxProxy::GetTrack(int index, Track* track) throw()
{
	if (index >= _core.TotalTracks)
		return false;

	AVStream* stm = ((AVFormatContext*)_core.FormatContext)->streams[index];
	track->Id = stm->index;
	if (stm->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		track->Type = Track::TrackType::Audio;
	else if (stm->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		track->Type = Track::TrackType::Video;
	else
		track->Type = Track::TrackType::Unknown;

	strcpy(track->CodecName, avcodec_get_name(stm->codec->codec_id));
	track->CodecPrivate = stm->codec->extradata;
	track->CodecPrivateSize = stm->codec->extradata_size;

	//。。。只考虑flv、mp4这种容器的timescale
	int ts = 0;
	if (stm->time_base.den == stm->time_base.num)
		ts = stm->time_base.den;
	else if (stm->time_base.den > stm->time_base.num)
		ts = stm->time_base.den / stm->time_base.num;
	else
		ts = stm->time_base.num / stm->time_base.den;
	track->Timescale = ts;
	return true;
}

bool DemuxProxy::ReadPacket(Packet* pkt) throw()
{
	if (pkt == NULL)
		return false;
	memset(pkt, 0, sizeof(Packet));

	AVPacket* packet = (AVPacket*)_core.NowPacket;
	av_free_packet(packet);
	int result = av_read_frame((AVFormatContext*)_core.FormatContext, packet);
	if (result < 0)
		return false;

	pkt->Id = packet->stream_index;
	pkt->DTS = packet->dts;
	pkt->PTS = packet->pts;
	pkt->Duration = packet->duration;
	if ((packet->flags & AV_PKT_FLAG_KEY) != 0)
		pkt->KeyFrame = true;
	if ((packet->flags & AV_PKT_FLAG_CORRUPT) != 0)
		pkt->Discontinuity = true;

	pkt->Data = packet->data;
	pkt->DataSize = packet->size;
	return true;
}

bool DemuxProxy::TimeSeek(double seconds) throw()
{
	AVFormatContext* format = (AVFormatContext*)_core.FormatContext;
	int64_t time = (int64_t)(seconds * double(AV_TIME_BASE));
	if (format->start_time != AV_NOPTS_VALUE)
		time += format->start_time;

	//先尝试关键帧seek
	int flags = AVSEEK_FLAG_BACKWARD;
	if (av_seek_frame(format, -1, time, flags) >= 0)
		return true;

	//没关键帧就seek到任意帧
	flags |= AVSEEK_FLAG_ANY;
	if (av_seek_frame(format, -1, time, flags) >= 0)
		return true;

	//没救了
	flags = AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_BYTE;
	return av_seek_frame(format, -1, time, flags) >= 0;
}