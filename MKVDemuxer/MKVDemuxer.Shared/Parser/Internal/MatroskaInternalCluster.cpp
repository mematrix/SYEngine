#include "MatroskaInternalCluster.h"

using namespace MKV;
using namespace MKV::Internal::Object;

bool Cluster::ParseCluster(EBML::EbmlHead& head,bool calc_cluster_pos,unsigned calc_head_size_offset)
{
	_head = &head;
	_timeCode = -1;

	if (calc_cluster_pos)
	{
		_clusterOffsetInDataSource = head.GetDataSource()->Tell() - 4; //4=MKV_ID_L1_CLUSTER
		_clusterOffsetInDataSource -= 
			calc_head_size_offset == 0 ? MKV::EBML::Core::GetEbmlBytes(head.Size()):calc_head_size_offset;
	}else{
		_clusterOffsetInDataSource = 0;
	}

	for (unsigned i = 0;i < 50;i++)
	{
		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO(head)))
			return false;

		if (head.MatchId(MKV_ID_L2_CLUSTER_SIMPLEBLOCK) || 
			head.MatchId(MKV_ID_L2_CLUSTER_BLOCKGROUP)) {
			if (_timeCode == -1)
				return false;
		}

		if (!head.MatchId(MKV_ID_L2_CLUSTER_CLUSTERTIMECODE))
		{
			head.Skip();
			continue;
		}

		EBML::EBML_VALUE_DATA value;
		if (!EBML::EbmlReadValue(head,value))
			return false;

		_timeCode = value.Ui64;
		break;
	}

	return _timeCode != -1;
}

Cluster::Block* Cluster::ParseNextBlock(long long* next_cluster_size,bool no_more_data)
{
	*next_cluster_size = 0;
	for (unsigned i = 0;i < 10;i++)
	{
		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO_P(_head),true))
			return false;

		if (_head->IsCluster())
		{
			*next_cluster_size = _head->Size();
			return nullptr;
		}

		switch (_head->Id())
		{
		case MKV_ID_L2_CLUSTER_SIMPLEBLOCK:
			return ProcessSimpleBlock() ? &_block:nullptr;
		case MKV_ID_L2_CLUSTER_BLOCKGROUP:
			return ProcessBlockGroup(no_more_data) ? &_block:nullptr;
		default:
			if (_head->IsLevel1Elements())
				return nullptr;
			else
				_head->Skip();
		}
	}

	return nullptr;
}

bool Cluster::ProcessSimpleBlock()
{
	return _block.InitBlock(_head);
}

bool Cluster::ProcessBlockGroup(bool no_more_data)
{
	bool blockFound = false;
	bool refFound = false;

	long long endPos = DataSource::SizeToAbsolutePosition(_head->Size(),_head->GetDataSource());
	while (1)
	{
		if (DataSource::IsCurrentPosition(endPos,_head->GetDataSource()))
		{
			DataSource::ResetInSkipOffsetError(endPos,_head->GetDataSource());
			break;
		}

		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(FAST_PARSE_WITH_IO_P(_head)))
			return false;

		EBML::EBML_VALUE_DATA value;
		switch (_head->Id())
		{
		case MKV_ID_L3_CLUSTER_BLOCKGROUP_BLOCK:
			blockFound = _block.InitBlock(_head,false);
			break;
		case MKV_ID_L3_CLUSTER_BLOCKGROUP_BLOCKDURATION:
			if (!EBML::EbmlReadValue(*_head,value))
				return false;
			_block.SetBlockDuration(value.Ui64);
			break;
		case MKV_ID_L3_CLUSTER_BLOCKGROUP_BLOCKREFERENCE:
			_head->Skip();
			_block.SetKeyFrame(false);
			refFound = true;
			break;
		case MKV_ID_L3_CLUSTER_BLOCKGROUP_BLOCKADDITIONS:
			if (!no_more_data)
				_head->Skip();
			else
				if (!_block.ParseBlockAdditions(_head))
					return false;
			break;

		default:
			_head->Skip();
		}
	}

	if (!refFound)
		_block.SetKeyFrame(true);

	return blockFound;
}

bool Cluster::Block::InitBlock(EBML::EbmlHead* head,bool simpleBlock)
{
	if (!_ioBuffer.Alloc((unsigned)head->Size(),true))
		return false;
	if (!head->Read(_ioBuffer.Get()))
		return false;

	_packet_offset = 0;
	_listCount = 1;

	memset(&_core,0,sizeof(_core));
	if (simpleBlock)
	{
		if (!ParseSimpleBlock())
			return false;
	}else{
		if (!ParseBlock())
			return false;
	}

	return true;
}

bool Cluster::Block::GetFrameInfo(Cluster::Block::FrameInfo* info,unsigned index)
{
	if (index >= _listCount)
		return false;

	info->Duration = (unsigned)_core.Duration;
	info->KeyFrame = _core.Desc.fKeyFrame;
	info->Timecode = _core.Desc.Timecode;
	info->TrackNumber = _core.Desc.TrackNumber;

	if (_listCount == 1)
	{
		info->Size = _ioBuffer.Size() - _packet_offset;
		return true;
	}

	info->Size = *(_listBuffer.Get<unsigned>() + index);
	return true;
}

bool Cluster::Block::ReadFrame(void* pb,unsigned index)
{
	if (index >= _listCount)
		return false;

	auto p = _ioBuffer.Get<unsigned char>() + _packet_offset;
	if (_listCount == 1)
	{
		memcpy(pb,p,_ioBuffer.Size() - _packet_offset);
		return true;
	}

	unsigned size = *(_listBuffer.Get<unsigned>() + index);
	if (_core.Desc.LacingType == Cluster::Block::BlockDescription::BlockLacingType::FixedLacing)
		memcpy(pb,p + (size * index),size);
	else
		memcpy(pb,p + CalcListOffsetSize(index),size);

	return true;
}

bool Cluster::Block::ReadFrameDirectPointer(unsigned char** ppbuf,unsigned* ppsize,unsigned index)
{
	//直接返回IOBuffer的缓冲区指针。
	if (index >= _listCount)
		return false;

	auto p = _ioBuffer.Get<unsigned char>() + _packet_offset;
	if (_listCount == 1)
	{
		*ppbuf = p;
		if (ppsize)
			*ppsize = _ioBuffer.Size() - _packet_offset;
		return true;
	}

	unsigned size = *(_listBuffer.Get<unsigned>() + index);
	if (ppsize)
		*ppsize = size;
	if (_core.Desc.LacingType == Cluster::Block::BlockDescription::BlockLacingType::FixedLacing)
		*ppbuf = p + (size * index);
	else
		*ppbuf = p + CalcListOffsetSize(index);

	return true;
}

unsigned Cluster::Block::ParseBlockHeader()
{
	auto p = _ioBuffer.Get<unsigned char>();
	if (EBML::Core::GetVIntLength(*p) > 1)
		return 0; //WTF!

	_core.Desc.TrackNumber = (unsigned)EBML::Core::StripZeroBitInNum(*p,0);
	_core.Desc.Timecode = MKV_SWAP16((*(unsigned short*)(p + 1)));
	return 3;
}

unsigned Cluster::Block::ParseLacingType()
{
	auto p = _ioBuffer.Get<unsigned char>() + _packet_offset;

	unsigned flag = (*p & 0x06) >> 1;
	if (flag > 3)
		return 0; //WTF...

	_core.Desc.LacingType = (decltype(_core.Desc.LacingType))flag;
	return 1;
}

bool Cluster::Block::ParseSimpleBlock()
{
	_packet_offset += ParseBlockHeader();
	if (_packet_offset == 0)
		return false;

	if (ParseLacingType() != 1)
		return false;
	
	if ((*(_ioBuffer.Get<unsigned char>() + _packet_offset) & 0x80) != 0)
		_core.Desc.fKeyFrame = true;
	_packet_offset++;
	
	if (_core.Desc.LacingType == Cluster::Block::BlockDescription::NoLacing)
		return true;

	return InitFrameList();
}

bool Cluster::Block::ParseBlock()
{
	_packet_offset += ParseBlockHeader();
	if (_packet_offset == 0)
		return false;

	if (ParseLacingType() != 1)
		return false;
	_packet_offset++;

	if (_core.Desc.LacingType == Cluster::Block::BlockDescription::NoLacing)
		return true;

	return InitFrameList();
}

bool Cluster::Block::InitFrameList()
{
	_listCount = *(_ioBuffer.Get<unsigned char>() + _packet_offset);
	if (_listCount == 0)
		return false;
	_listCount++;

	if (!_listBuffer.Alloc(sizeof(unsigned) * _listCount))
		return false;

	_packet_offset++;
	if (_core.Desc.LacingType == Cluster::Block::BlockDescription::FixedLacing)
	{
		auto p = _listBuffer.Get<unsigned>();
		unsigned size = (_ioBuffer.Size() - _packet_offset) / _listCount;
		for (unsigned i = 0;i < _listCount;i++,p++)
			*p = size;

		return true;
	}

	if (_core.Desc.LacingType == Cluster::Block::BlockDescription::EbmlLacing)
		return InitEbmlFrameList();

	return InitXiphFrameList();
}

bool Cluster::Block::InitEbmlFrameList()
{
	auto list = _listBuffer.Get<unsigned>();
	auto p = _ioBuffer.Get<unsigned char>() + _packet_offset;

	unsigned long long first_num = 0;
	unsigned n = EBML::ReadUIntFromPointer(p,&first_num);
	if (n == 0)
		return false;
	if (first_num > INT_MAX)
		return false;

	p += n;
	list[0] = (unsigned)first_num;
	for (unsigned i = 1;i < _listCount - 1;i++)
	{
		long long snum;
		unsigned r = EBML::ReadSIntFromPointer(p,&snum);
		if (r == 0)
			return false;
		if (list[i - 1] + snum > INT_MAX)
			return false;

		n += r;
		p += r;
		list[i] = (unsigned)(list[i - 1] + snum);
	}
	_packet_offset += n;

	SetListLastNumber();
	return true;
}

bool Cluster::Block::InitXiphFrameList()
{
	auto list = _listBuffer.Get<unsigned>();
	auto p = _ioBuffer.Get<unsigned char>() + _packet_offset;

	unsigned n = 0;
	for (unsigned i = 0;i < _listCount - 1;i++)
	{
		for (unsigned j = 0;j < 8;j++)
		{
			auto temp = *p;
			list[i] += temp;

			p++;
			n++;

			if (temp != 0xFF)
				break;

			if (j > 4)
				return false;
		}
	}
	_packet_offset += n;

	SetListLastNumber();
	return true;
}

unsigned Cluster::Block::CalcListOffsetSize(unsigned index)
{
	unsigned total = 0;
	for (unsigned i = 0;i < index;i++)
		total = total + _listBuffer.Get<unsigned>()[i];

	return total;
}

void Cluster::Block::SetListLastNumber()
{
	unsigned total = CalcListOffsetSize(_listCount - 1);

	unsigned last_size = _ioBuffer.Size() - _packet_offset - total;
	_listBuffer.Get<unsigned>()[_listCount - 1] = last_size;
}