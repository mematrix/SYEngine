#include "MKVMediaFormat.h"
#ifdef _MSC_VER
#include <Windows.h>
#include <wchar.h>
#endif

#ifdef _MSC_VER
static void PrintUTF8(const unsigned char* utf8,unsigned size,double time,double duration,MediaCodecType ct)
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	if (utf8 == nullptr || GetConsoleWindow() == nullptr)
		return;
	int len = MultiByteToWideChar(CP_UTF8,0,(LPCCH)utf8,size,nullptr,0);
	if (len > 0)
	{
		auto p = (LPWSTR)GlobalAlloc(GPTR,len * 4);
		if (MultiByteToWideChar(CP_UTF8,0,(LPCCH)utf8,size,p,len * 4) > 0)
			wprintf(L"Text(%s): %s\nTime: %.3f,Duration: %.3f\n",
				ct == MEDIA_CODEC_SUBTITLE_TEXT ? L"SRT":L"ASS",
				p,(float)time,(float)duration);
		GlobalFree(p);
	}
#endif
}
#endif

static inline void SetSkipAVMediaPacket(AVMediaPacket* packet)
{
	memset(packet,0,sizeof(AVMediaPacket));
	packet->flag = MEDIA_PACKET_CAN_TO_SKIP_FLAG|MEDIA_PACKET_BUFFER_NONE_FLAG;
	packet->pts = PACKET_NO_PTS;
	packet->stream_index = -1;
}

static inline unsigned GetFrameNALUSize(unsigned char* p,unsigned lengthSizeMinusOne)
{
	if (lengthSizeMinusOne == 4)
		return MKV_SWAP32(*(unsigned*)p);
	else if (lengthSizeMinusOne == 3)
		return ((MKV_SWAP32(*(unsigned*)p)) >> 8);
	else if (lengthSizeMinusOne == 2)
		return MKV_SWAP16(*(unsigned short*)p);
	else if (lengthSizeMinusOne == 1)
		return *p;

	return 0xFFFFFFFF;
}

static unsigned ScanAllFrameNALUToAnnexB(unsigned char* source,unsigned len)
{
	decltype(source) src = source;
	unsigned count = 0,size = 0;
	while (1)
	{
		unsigned nal_size = *(unsigned*)src;
#ifndef _MSC_VER
		nal_size = MKV_SWAP32(nal_size);
#else
		nal_size = _byteswap_ulong(nal_size);
#endif

		if (nal_size == 0)
		{
			src += 4;
			continue;
		}

		if (nal_size > (len - size))
			break;

		*(unsigned*)&src[0] = 0x01000000;
		size = size + 4 + nal_size;
		src = source + size;

		if (size >= len)
			break;
	}
	return size;
}

unsigned ScanAllFrameNALU(unsigned char* psrc,unsigned char* pdst,unsigned len,unsigned lengthSizeMinusOne)
{
	if (len < lengthSizeMinusOne)
		return 0;

	decltype(psrc) ps = psrc;
	decltype(pdst) pd = pdst;

	unsigned size = 0,size2 = 0;
	unsigned count = 0;
	while (1)
	{
		unsigned nal_size = GetFrameNALUSize(ps,lengthSizeMinusOne);
		if (nal_size == 0)
		{
			ps += lengthSizeMinusOne;
			continue;
		}
		
		if (nal_size > len)
			return 0;

		/*
		pd[0] = 0;
		pd[1] = 0;
		pd[2] = 0;
		pd[3] = 1;
		*/
		*(unsigned*)&pd[0] = 0x01000000;

		memcpy(&pd[4],ps + lengthSizeMinusOne,nal_size);

		size = size + lengthSizeMinusOne + nal_size;
		if (lengthSizeMinusOne == 4)
			size2 = size;
		else
			size2 = size2 + 4 + nal_size;

		pd = pdst + size2;
		ps = psrc + size;

		if (size >= len)
			break;

#ifdef _DEBUG
        count++;
        if (count > 0)
            printf("AVC1 NALU:%d (%d Bytes), Total Size:%d, Offset:%d, Size:%d\n",
				count,lengthSizeMinusOne,len,ps - psrc,len - size);
#endif
	}

	return size2;
}

unsigned MKVMediaFormat::FindMediaStreamFromTrackNumber(unsigned number)
{
	for (unsigned i = 0;i < _stream_count;i++)
	{
		if (_streams[i]->GetStreamIndex() == number)
			return i;
	}
	return 0xFFFFFFFF;
}

AV_MEDIA_ERR MKVMediaFormat::Reset()
{
	if (_av_io == nullptr)
		return AV_READ_PACKET_ERR_NOT_OPENED;

	if (_parser->Reset() != PARSER_MKV_OK)
		return AV_SEEK_ERR_PARSER_FAILED;

	return AV_ERR_OK;
}

AV_MEDIA_ERR MKVMediaFormat::Seek(double seconds,bool key_frame,AVSeekDirection seek_direction)
{
	if (_av_io == nullptr)
		return AV_READ_PACKET_ERR_NOT_OPENED;
	
	if (seconds > _global_info.Duration)
		return Reset();
	if (seconds < 0.1)
		return Reset();

	auto ret = _parser->Seek(seconds);
	if (ret != PARSER_MKV_OK)
		return AV_SEEK_ERR_PARSER_FAILED;

	for (unsigned i = 0;i < _stream_count;i++)
		if (_streams[i]->HasBFrameExists())
			_streams[i]->SetDecodeTimestamp(PACKET_NO_PTS);

	return AV_ERR_OK;
}

AV_MEDIA_ERR MKVMediaFormat::OnNotifySeek()
{
	//外部Seek通知。。
	if (_av_io == nullptr)
		return AV_READ_PACKET_ERR_NOT_OPENED;

	_io_pool.get()->EmptyInternalBuffer(); //清空IO池缓冲区
	if (_parser->OnExternalSeek() != PARSER_MKV_OK)
		return AV_SEEK_ERR_PARSER_FAILED;
	return AV_ERR_OK;
}

AV_MEDIA_ERR MKVMediaFormat::ReadPacket(AVMediaPacket* packet)
{
	if (_av_io == nullptr)
		return AV_READ_PACKET_ERR_NOT_OPENED;
	if (packet == nullptr)
		return AV_COMMON_ERR_INVALIDARG;

	MKVParser::MKVPacket pkt = {};
	auto ret = _parser->ReadPacket(&pkt);
	if (ret != PARSER_MKV_OK)
		return AV_READ_PACKET_ERR_NON_MORE;

	unsigned n = FindMediaStreamFromTrackNumber(pkt.Number);
	if (pkt.SkipThis || pkt.PacketSize == 0 || n == 0xFFFFFFFF)
	{
		SetSkipAVMediaPacket(packet);
		return AV_ERR_OK;
	}

	if ((_read_flags & MEDIA_FORMAT_READER_MATROSKA_ASS2SRT) != 0 &&
		_streams[n]->IsSubtitleWithASS()) {
		//转换SSA&ASS字幕格式为SRT格式。
		if (!ProcessASS2SRT(&pkt))
		{
			//处理失败设置跳过这个包
			SetSkipAVMediaPacket(packet);
			return AV_ERR_OK;
		}
	}

	auto ct = _streams[n]->GetCodecType();
#if defined(_DEBUG) && defined(_MSC_VER) && defined(WIN32)
	if (ct == MediaCodecType::MEDIA_CODEC_SUBTITLE_TEXT ||
		ct == MediaCodecType::MEDIA_CODEC_SUBTITLE_SSA ||
		ct == MediaCodecType::MEDIA_CODEC_SUBTITLE_ASS)
		PrintUTF8(pkt.PacketData,pkt.PacketSize,pkt.Timestamp,pkt.Duration,ct);
#endif

	if ((ct == MediaCodecType::MEDIA_CODEC_VIDEO_H264 ||
		ct == MediaCodecType::MEDIA_CODEC_VIDEO_HEVC) && !_force_avc1) {
		unsigned nal_size = _streams[n]->GetH264HEVCNalSize();
		if (nal_size == 0)
			return AV_COMMON_ERR_UNEXPECTED;

		if (nal_size == 4)
		{
			//如果NAL头是4字节，不需要Copy，直接修改为AnnexB格式
			packet->data.buf = pkt.PacketData;
			packet->data.size = ScanAllFrameNALUToAnnexB(pkt.PacketData,pkt.PacketSize);
		}else{
			if (!_pktBuffer.Alloc(pkt.PacketSize << 1,true))
				return AV_COMMON_ERR_OUTOFMEMORY;

			packet->data.buf = _pktBuffer.Get<unsigned char>();
			packet->data.size = ScanAllFrameNALU(pkt.PacketData,packet->data.buf,pkt.PacketSize,nal_size);
		}
	}else{
		//if (AllocMediaPacketAndCopy(packet,pkt.PacketData,pkt.PacketSize) == 0)
			//return AV_READ_PACKET_ERR_ALLOC_PKT;
		packet->data.buf = pkt.PacketData;
		packet->data.size = pkt.PacketSize;
	}

	packet->flag = MEDIA_PACKET_FIXED_BUFFER_FLAG;
	packet->position = -1;
	packet->dts = PACKET_NO_PTS;

	packet->stream_index = pkt.Number;
	packet->pts = pkt.Timestamp;
	if (pkt.KeyFrame)
	{
		packet->flag |= MEDIA_PACKET_KEY_FRAME_FLAG;
		if (_streams[n]->GetDecodeTimestamp() == PACKET_NO_PTS)
			_streams[n]->SetDecodeTimestamp(pkt.Timestamp);
	}

	if (pkt.Duration > 0)
		packet->duration = pkt.Duration;
	else
		if (_streams[n]->GetFrameDuration() > 0)
			packet->duration = _streams[n]->GetFrameDuration();
		else
			packet->duration = PACKET_NO_PTS;


	if (_streams[n]->HasBFrameExists() &&
		_streams[n]->GetFrameDuration() > 0.0 &&
		_streams[n]->GetDecodeTimestamp() != PACKET_NO_PTS) {
	 	//H264 MP+ and HEVC...
		if (packet->dts == PACKET_NO_PTS)
			packet->dts = _streams[n]->GetDecodeTimestamp();
		_streams[n]->IncrementDTS();
	}

	if (packet->data.size == 0)
		packet->flag |= MEDIA_PACKET_BUFFER_NONE_FLAG;

	return AV_ERR_OK;
}

bool MKVMediaFormat::ProcessASS2SRT(MKVParser::MKVPacket* pkt)
{
	return true;
}