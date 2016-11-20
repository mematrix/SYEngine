#include "MKVParser.h"
#include "WV4Help.h"
#include <assert.h>

using namespace MKVParser;

bool MKVFileParser::ParseNext() throw()
{
	if (_frame.nextClusterSize == 0)
		return false;

	while (1)
	{
		//���Ϸ�����һ���أ�ֱ���ļ�������
		if (_mkv->NextCluster(_frame.nextClusterSize) != MKV_ERR_OK)
			return false;

		while (1)
		{
			//������һ�����ˣ��������е�һ���顣
			auto block = _mkv->GetCurrentCluster()->ParseNextBlock(&_frame.nextClusterSize);
			if (block != nullptr && block->GetFrameCount() == 0)
				return false;

			//����д���һ���飬�򷵻ء�
			if (block != nullptr && _frame.nextClusterSize == 0)
			{
				_frame.block = block;
				_frame.count = block->GetFrameCount() - 1;
				return true;
			}

			//����ֲ����ڿ飬�¸���Ҳ�����ڣ��򷵻ط���������
			if (block == nullptr && _frame.nextClusterSize == 0)
				return false;
			
			//�������������һ���أ������NextCluster...
			if (_frame.nextClusterSize != 0)
				break;
		}
	}
}

void MKVFileParser::BlockInfo::Next(std::shared_ptr<MKV::Internal::MatroskaSegment>& mkv) throw()
{
	if (IsFull()) //�ж��ǲ���Block�����Frame�Ѿ��������
	{
		//������ɷ�����һ��Block��
		block = mkv->GetCurrentCluster()->ParseNextBlock(&nextClusterSize);
		if (block)
		{
			if (block->GetFrameCount() == 0) //����ѧ�����ش���
			{
				nextClusterSize = 0;
				block = nullptr;
			}

			count = block->GetFrameCount() - 1;
		}

		now = 0;
		timestampLaced = 0.0;
	}else{
		now++; //��ȻmarkҪ�����¸�Frame��
	}
}

unsigned MKVFileParser::ReadPacket(MKVPacket* packet,bool fast) throw()
{
	if (packet == nullptr)
		return PARSER_MKV_ERR_MEMORY;

	if (_frame.IsEmpty()) //���û���κ�Block��
		if (!ParseNext()) //����һ���أ�ȡ��Block��
			return PARSER_MKV_ERR_READ_END_OF_PACKET;
	
	//�ӵ�ǰBlock��ȡ��Frame��Ϣ��
	MKV::Internal::Object::Cluster::Block::FrameInfo fi = {};
	if (!_frame.GetBlock()->GetFrameInfo(&fi,_frame.now))
		return PARSER_MKV_ERR_UNKNOWN;

	if (fi.TrackNumber == 0 && !_track_num_start_with_zero)
		return PARSER_MKV_ERR_READ_FRAME_INVALID;

	packet->Number = ConvertTrackNumber(fi.TrackNumber);
	packet->KeyFrame = fi.KeyFrame;

	auto info = FindTrackInfo(fi.TrackNumber);
	bool wavpackTrack = false;
	unsigned pktWriteOffset = 0;

    if (fi.Size == 0) {
        packet->SkipThis = true;
        goto done;
    }

	if (info)
	{
		//�����WV�����ݣ�Ҫ���⴦��
		wavpackTrack = info->Codec.CodecType == MatroskaCodecIds::MKV_Audio_WAVPACK4;

		if (info->InternalFrameDuration > 0.1)
			packet->Duration = info->InternalFrameDuration;
		else if (fi.Duration > 1)
			packet->Duration = CalcTimestamp(fi.Duration);
		if (info->Codec.CodecID[0] == 'A')
			packet->KeyFrame = true;

		//���û��KeyFrame����������ClusterPos���ڼ���
		if (_info.NonKeyFrameIndex)
			packet->ClusterPosition = _mkv->GetCurrentCluster()->GetClusterPosition();

		auto timecode = _mkv->GetCurrentCluster()->GetTimecode() + fi.Timecode;
		if (timecode <= 0)
			packet->Timestamp = 0.0;
		else
			//packet->Timestamp = (double)timecode * _info.TimecodeScale / 1000000000.0;
			packet->Timestamp = CalcTimestamp(timecode);

		double frameDuration = info->InternalFrameDuration;
		if (_frame.GetBlock()->GetFrameCount() > 1)
		{
			if (frameDuration == 0.0)
			{
				//����Ƿ�Ƭ��Frame������TrackEntry��û��DefaultDuration
				//�����ÿһ��Frame��Duration�����ǹ̶��ģ���Ҫ��̬����
				packet->NoTimestamp = true;
				//�������Frame��RAW���ݽ��м��㡣
			}else{
				//�����Frame�Ĺ̶�Duration��Ϣ����ֱ���ۼӼ���
				packet->Timestamp += frameDuration * (double)_frame.now;
			}
		}

		if (fast)
		{
			packet->PacketData = nullptr;
			packet->PacketSize = 0xFFFFFFFF;
			goto done;
		}

		unsigned csize = info->InternalEntry->Encoding.Compression.ContentCompSettingsSize;
		packet->PacketSize = fi.Size + csize;
		if (wavpackTrack && !info->InternalEntry->Encoding.fCompressionExists)
		{
			packet->PacketSize += WAVPACK4_BLOCK_FIXED_FRAME_SIZE;
			pktWriteOffset = WAVPACK4_BLOCK_FIXED_FRAME_SIZE;
		}

		if (!_frameBuffer.Alloc(packet->PacketSize,true))
			return PARSER_MKV_ERR_MEMORY;
		packet->PacketData = _frameBuffer.Get<unsigned char>();

		if (info->InternalEntry->Encoding.fCompressionExists)
		{
			if (!info->IsZLibCompressed())
			{
				if (info->InternalEntry->Encoding.Compression.ContentCompSettings == nullptr)
					return PARSER_MKV_ERR_UNEXPECTED;

				switch (csize)
				{
				case 1:
					*packet->PacketData = *info->InternalEntry->Encoding.Compression.ContentCompSettings;
					break;
				case 2:
					*(unsigned short*)packet->PacketData = 
						*(unsigned short*)info->InternalEntry->Encoding.Compression.ContentCompSettings;
					break;
				case 4:
					*(unsigned*)packet->PacketData = 
						*(unsigned*)info->InternalEntry->Encoding.Compression.ContentCompSettings;
					break;
				default:
					if (csize > 0)
						memcpy(packet->PacketData,info->InternalEntry->Encoding.Compression.ContentCompSettings,csize);
				}

				if (!_frame.GetBlock()->ReadFrame(packet->PacketData + csize,_frame.now))
					return PARSER_MKV_ERR_READ_DATA_INVALID;
			}else{
				//ZLIBѹ������ѹ��
				if (!_frame.GetBlock()->ReadFrame(packet->PacketData,_frame.now))
					return PARSER_MKV_ERR_READ_DATA_INVALID;

				unsigned char* pd = nullptr;
				unsigned dsize = 0;
				if (!zlibDecompress(packet->PacketData,packet->PacketSize,&pd,&dsize))
					return PARSER_MKV_ERR_READ_ZLIB_CLOSED;

				if (pd == nullptr ||
					dsize == 0)
					return PARSER_MKV_ERR_READ_DATA_INVALID;

				if (!_frameBuffer.Alloc(dsize,true)) {
					free(pd);
					return PARSER_MKV_ERR_MEMORY;
				}

				packet->PacketSize = dsize;
				packet->PacketData = _frameBuffer.Get<unsigned char>();
				memcpy(packet->PacketData,pd,dsize);
				free(pd);
			}
		}else{
			if (!_frame.GetBlock()->ReadFrame(packet->PacketData + pktWriteOffset,_frame.now))
				return PARSER_MKV_ERR_READ_DATA_INVALID;

			if (wavpackTrack)
			{
				//WAVPACK4�����죬���⴦��ϲ�WV4�İ�ͷ
				WAVPACK4_BLOCK_HEADER head;
				if (MKVBlockFrameToWV4BlockHead(packet->PacketData + pktWriteOffset,
					packet->PacketSize - pktWriteOffset,&head))
					memcpy(packet->PacketData,&head,sizeof(head));
			}
		}

		//�ж��Ƿ��־Ϊû��pts��������ܵĻ����ж�̬����
		if (packet->NoTimestamp)
		{
			if (info->InternalDurationCalc)
			{
				double duration = 0.0;
				if (info->InternalDurationCalc->
					Calc(packet->PacketData,packet->PacketSize,&duration) &&
					duration != 0.0) {
					packet->Duration = duration;
					packet->Timestamp += _frame.GetIncrementDuration();
					_frame.AddTimestampDuration(duration);
				}
			}
		}
	}else{
		packet->SkipThis = true; //����ǲ�֧�ֵ�Track�����ݣ�mark������
	}

done:
#ifdef _DEBUG
	if (_seek_after_flag)
	{
		_seek_after_flag = false;
		if (!packet->SkipThis)
			printf("MKVParser -> Seek After -> Frame(SID:%d) -> %.2f, Key:%s\n",
				packet->Number,float(packet->Timestamp),packet->KeyFrame ? "Yes":"No");
	}
#endif
	_frame.Next(_mkv); //������һ��Block����Frame��
	return PARSER_MKV_OK;
}

unsigned MKVFileParser::Seek(double seconds,bool fuzzy,unsigned base_on_track_number) throw()
{
	/*
	 * û��KeyFrame�����������
	 * 1�������MKA�ļ�������Block������ΪKeyFrame��������Ƶÿ��Frame����KeyFrame����
	 * 2���������Ƶ+��Ƶ�������ֻ�е����ΪKeyFrame����ƵFrame������Ϊ������
	*/

	if (_info.NonKeyFrameIndex)
	{
		MakeKeyFramesIndex(); //�����ؼ�֡����
		if (_info.NonKeyFrameIndex) //����ʧ��
			return Reset(); //ֱ��Reset��
	}

	double true_time = 0.0;
	bool found = false;
	unsigned num = base_on_track_number;
	if (num == 0xFFFFFFFF) //ʹ�õ�һ�������
		num = _default_seek_tnum;

	long long filepos = 0;
	unsigned block_num = 1;
	unsigned count = _keyframe_index.GetCount();
	for (unsigned i = 0;i < count;i++)
	{
		auto p = _keyframe_index.GetItem(i);
		if (p->TrackNumber != (num + 1) &&
			!p->SubtitleTrack)
			continue;

		if (p->Time >= seconds)
		{
			double time = p->Time;
			long long pos = p->ClusterPosition;
			if (i > 0)
			{
				auto p = _keyframe_index.GetItem(i - 1);
				time = p->Time;
				pos = filepos = p->ClusterPosition;
			}
			true_time = time;

			found = true;
			break;
		}
	}
	if (!found)
	{
		if ((count > 1) && (seconds > _keyframe_index.GetItem(count - 1)->Time))
			filepos = _keyframe_index.GetItem(count - 1)->ClusterPosition;
		else
			return Reset();
	}

	if (!_io->Seek(filepos))
		return PARSER_MKV_ERR_IO;

	MKV::EBML::EbmlHead head;
	if (!MKV::EBML::FastParseEbmlHeadIfSize(head,this))
		return PARSER_MKV_ERR_UNEXPECTED;

	_frame.Empty(head.Size());
	_frameBuffer.ResetSize();

#ifdef _DEBUG
	_seek_after_flag = true;
	printf("MKVParser -> Seek -> ClusterPosition(4GB Space) -> 0x%08X\n",(unsigned)filepos);
	printf("MKVParser -> Seek -> Time -> %.2f (User Seek: %.2f)\n",float(true_time),float(seconds));
#endif

	if (!fuzzy && block_num > 1)
	{
		//�������ģ�����ң��������Ҵ������Frame��
		//TODO...
		assert(block_num == 1);
	}

	return PARSER_MKV_OK;
}

unsigned MKVFileParser::Reset() throw()
{
	if (!_io->Seek(_resetPointer))
		return PARSER_MKV_ERR_IO;

	_frameBuffer.ResetSize();
	memset(&_frame,0,sizeof(_frame));

	_frame.Empty(_firstClusterSize);
	return PARSER_MKV_OK;
}

MKVTrackInfo* MKVFileParser::FindTrackInfo(unsigned TrackNumber) throw()
{
	unsigned count = _tracks.GetCount();
	for (unsigned i = 0;i < count;i++)
	{
		if (_tracks.GetItem(i)->InternalEntry->TrackNumber == TrackNumber)
			return _tracks.GetItem(i);
	}

	return nullptr;
}

unsigned MKVFileParser::OnExternalSeek() throw()
{
	MKV::EBML::EbmlHead head;
	if (!MKV::EBML::FastParseEbmlHeadIfSize(head,this))
		return PARSER_MKV_ERR_UNEXPECTED;

	_frame.Empty(head.Size());
	_frameBuffer.ResetSize();
	return MKV_ERR_OK;
}

unsigned MKVFileParser::ReadSinglePacket(MKVPacket* packet,unsigned number,bool key_frame,bool auto_reset) throw()
{
	auto oldPos = _io->Tell();
	if (oldPos == 0)
		return PARSER_MKV_ERR_UNEXPECTED;
	auto firstClusterSize = _firstClusterSize;
	auto blockInfo = _frame;

	memset(packet,0,sizeof(decltype(*packet)));

	int i;
	for (i = 0;i < 100;i++)
	{
		MKVPacket pkt = {};
		unsigned ret = ReadPacket(&pkt);
		if (ret != PARSER_MKV_OK)
			return ret;

		if (pkt.SkipThis)
			continue;

		if (pkt.Number == number)
		{
			if (!key_frame)
			{
				*packet = pkt;
				break;
			}else{
				if (pkt.KeyFrame)
				{
					*packet = pkt;
					break;
				}
			}
		}
	}

	if (auto_reset)
	{
		if (!_io->Seek(oldPos))
			return PARSER_MKV_ERR_IO;
		_firstClusterSize = firstClusterSize;
		_frame = blockInfo;
	}

	return i > 90 ? PARSER_MKV_ERR_READ_FRAME_INVALID:PARSER_MKV_OK;
}

void MKVFileParser::MakeKeyFramesIndex() throw()
{
	Reset();
	while (1)
	{
		MKVPacket pkt = {};
		if (ReadPacket(&pkt,true) != PARSER_MKV_OK)
			break;
		if (!pkt.KeyFrame ||
			pkt.SkipThis ||
			pkt.ClusterPosition < 1)
			continue;
		if (pkt.Timestamp <= 0.0)
			continue;

		if (!_is_mka_file)
		{
			auto info = FindTrackInfo(pkt.Number + 1);
			if (info == nullptr)
				continue;
			if (info->Codec.CodecID[0] != 'V')
				continue;
		}

		MKVKeyFrameIndex index = {};
		index.Time = pkt.Timestamp;
		index.TrackNumber = pkt.Number + 1;
		index.ClusterPosition = pkt.ClusterPosition;
		_keyframe_index.AddItem(&index);
	}

	if (!_is_mka_file)
	{
		_default_seek_tnum = ConvertTrackNumber(_keyframe_index.GetItem(0)->TrackNumber);
		for (unsigned i = 0;i < _tracks.GetCount();i++)
		{
			if (_tracks.GetItem(i)->InternalEntry->Track.CodecID[0] != 'V')
				continue;

			bool found = false;
			for (unsigned j = 0;j < _keyframe_index.GetCount();j++)
			{
				if (_keyframe_index.GetItem(j)->TrackNumber == _tracks.GetItem(i)->InternalEntry->TrackNumber) {
					_default_seek_tnum = ConvertTrackNumber(_keyframe_index.GetItem(j)->TrackNumber);
					found = true;
					break;
				}
			}

			if (found)
				break;
		}
	}

	_info.NonKeyFrameIndex = false;
}