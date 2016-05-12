#include "MatroskaInternal.h"

using namespace MKV;
using namespace MKV::Internal;
using namespace MKV::Internal::Object;

MatroskaSegment::MatroskaSegment(DataSource::IDataSource* dataSource) throw()
{
	_dataSource = dataSource;
	_core.Status = 0;

	_clusterHead.SetDataSource(dataSource);
	_user_req_calc_cluster_pos = false;
}

unsigned MatroskaSegment::Parse(long long segmentSize,long long* first_cluster_size) throw()
{
	_start_pos = (unsigned)_dataSource->Tell();

	if (_dataSource == nullptr)
		return MKV_ERR_FAIL;

	if (segmentSize < 64)
		return MKV_ERR_INVALID_SIZE;

	unsigned result = MKV_ERR_OK;
	while (1)
	{
		EBML::EbmlHead head;
		if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(head,_dataSource,true))
			BREAK_AND_SET_RESULT(result,MKV_ERR_INVALID_HEAD);

		bool clusterFound = false;
		switch (head.Id())
		{
		case MKV_ID_L1_SEEKHEAD:
			if (_core.SeekHead.get() == nullptr)
			{
				_core.SeekHead = 
					std::make_shared<Object::SeekHead>(head);
				if (!_core.SeekHead->ParseSeekHead(head.Size()))
					BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_SEEKHEAD);

				_core.Status |= MKV_SEGMENT_PARSE_SEEKHEAD;
			}else{
				head.Skip();
			}
			break;
		case MKV_ID_L1_INFO:
			if (_core.SegmentInfo.get() == nullptr)
			{
				_core.SegmentInfo = 
					std::make_shared<Object::SegmentInfo>(head);
				if (!_core.SegmentInfo->ParseSegmentInfo(head.Size()))
					BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_INFO);

				_core.Status |= MKV_SEGMENT_PARSE_INFO;
			}else{
				head.Skip();
			}
			break;
		case MKV_ID_L1_TRACKS:
			if (_core.Tracks.get() == nullptr)
			{
				_core.Tracks = 
					std::make_shared<Object::Tracks>(head);
				if (!_core.Tracks->ParseTracks(head.Size()))
					BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_TRACKS);

				_core.Status |= MKV_SEGMENT_PARSE_TRACKS;
			}else{
				head.Skip();
			}
			break;
		case MKV_ID_L1_CUES:
			if (_core.Cues.get() == nullptr)
			{
				_core.Cues = 
					std::make_shared<Object::Cues>(head);
				if (!_core.Cues->ParseCues(head.Size()))
					BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_CUES);

				_core.Status |= MKV_SEGMENT_PARSE_CUES;
			}else{
				head.Skip();
			}
			break;
		case MKV_ID_L1_CHAPTERS:
			if (_core.Chapters.get() == nullptr)
			{
				_core.Chapters = 
					std::make_shared<Object::Chapters>(head);
				if (!_core.Chapters->ParseChapters(head.Size()))
					BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_CHAPTERS);

				_core.Status |= MKV_SEGMENT_PARSE_CHAPTERS;
			}else{
				head.Skip();
			}
			break;
		case MKV_ID_L1_TAGS:
			if (_core.Tags.get() == nullptr)
			{
				_core.Tags = 
					std::make_shared<Object::Tags>(head);
				if (!_core.Tags->ParseTags(head.Size()))
					BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_TAGS);

				_core.Status |= MKV_SEGMENT_PARSE_TAGS;
			}else{
				head.Skip();
			}
			break;
		case MKV_ID_L1_ATTACHMENTS:
			if (_core.Attachments.get() == nullptr)
			{
				_core.Attachments = 
					std::make_shared<Object::Attachments>(head);
				if (!_core.Attachments->ParseAttachments(head.Size()))
					BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_FILES);

				_core.Status |= MKV_SEGMENT_PARSE_FILES;
			}else{
				head.Skip();
			}
			break;
		case MKV_ID_L1_CLUSTER:
			clusterFound = true;
			if (first_cluster_size != nullptr)
				*first_cluster_size = head.Size();
			if (!ProcessFirstClusterAndGetInfo())
				BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_CLUSTER);
			break;
		default:
			head.Skip();
		}

		if (clusterFound)
			break;
	}

	return result;
}

unsigned MatroskaSegment::ExecuteSeekHead() throw()
{
	if (_core.Status == 0)
		return MKV_ERR_FAIL;

	if (_core.SeekHead.get() == nullptr)
		return MKV_ERR_OK;
	if (_core.SeekHead->GetSeekHeadCount() == 0)
		return MKV_ERR_OK;

	//for restore.
	auto cur_pos = _dataSource->Tell();

	unsigned result = MKV_ERR_OK;
	for (unsigned i = 0;i < _core.SeekHead->GetSeekHeadCount();i++)
	{
		auto head = _core.SeekHead->GetSeekHead(i);
		if (head && !IsParsed(head->SeekID))
		{
			//¿Ó¡£¡£¡£
			if (_dataSource->Size() < (head->SeekPosition + _start_pos))
				continue;

			if (_dataSource->Seek(head->SeekPosition + _start_pos) == MKV_DATA_SOURCE_SEEK_ERROR)
				BREAK_AND_SET_RESULT(result,MKV_ERR_INVALID_IO);

			EBML::EbmlHead head;
			if (!EBML::FastParseEbmlHeadIfSizeWithNoErr(head,_dataSource,true))
				BREAK_AND_SET_RESULT(result,MKV_ERR_INVALID_HEAD);

			switch (head.Id())
			{
			case MKV_ID_L1_CUES:
				if (_core.Cues.get() == nullptr)
				{
					_core.Cues = 
						std::make_shared<Object::Cues>(head);
					if (!_core.Cues->ParseCues(head.Size()))
						BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_CUES);
					
					_core.Status |= MKV_SEGMENT_PARSE_CUES;
				}else{
					head.Skip();
				}
				break;
			case MKV_ID_L1_CHAPTERS:
				if (_core.Chapters.get() == nullptr)
				{
					_core.Chapters = 
						std::make_shared<Object::Chapters>(head);
					if (!_core.Chapters->ParseChapters(head.Size()))
						BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_CHAPTERS);
					
					_core.Status |= MKV_SEGMENT_PARSE_CHAPTERS;
				}else{
					head.Skip();
				}
				break;
			case MKV_ID_L1_TAGS:
				if (_core.Tags.get() == nullptr)
				{
					_core.Tags = 
						std::make_shared<Object::Tags>(head);
					if (!_core.Tags->ParseTags(head.Size()))
						BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_TAGS);
					
					_core.Status |= MKV_SEGMENT_PARSE_TAGS;
				}else{
					head.Skip();
				}
				break;
			case MKV_ID_L1_ATTACHMENTS:
				if (_core.Attachments.get() == nullptr)
				{
					_core.Attachments = 
						std::make_shared<Object::Attachments>(head);
					if (!_core.Attachments->ParseAttachments(head.Size()))
						BREAK_AND_SET_RESULT(result,MKV_ERR_PARSE_FILES);
					
					_core.Status |= MKV_SEGMENT_PARSE_FILES;
				}else{
					head.Skip();
				}
				break;
			}
		}
	}

	if (_dataSource->Tell() != cur_pos)
		_dataSource->Seek(cur_pos);

	return result;
}

bool MatroskaSegment::IsParsed(unsigned id) throw()
{
	switch (id)
	{
	case MKV_ID_L1_CUES:
		return (_core.Status & MKV_SEGMENT_PARSE_CUES) != 0;
	case MKV_ID_L1_CHAPTERS:
		return (_core.Status & MKV_SEGMENT_PARSE_CHAPTERS) != 0;
	case MKV_ID_L1_TAGS:
		return (_core.Status & MKV_SEGMENT_PARSE_TAGS) != 0;
	case MKV_ID_L1_ATTACHMENTS:
		return (_core.Status & MKV_SEGMENT_PARSE_FILES) != 0;
	default:
		return true;
	}

	return true;
}

bool MatroskaSegment::ProcessFirstClusterAndGetInfo() throw()
{
	_cluster_is_fixed_size = 0;

	if (_dataSource->Tell() < 64)
		return false;

	if (!_dataSource->Seek(_dataSource->Tell() - 32))
		return false;
	unsigned char buf[40] = {};
	if (_dataSource->Read(buf,32) != 32)
		return false;

	int cluster_offset = MKV::EBML::Core::SearchEbmlClusterStartCodeOffset(buf,32 + 4);
	if (cluster_offset > 16)
	{
		cluster_offset += 4;
		unsigned char* p = &buf[cluster_offset];

		unsigned ebml_size = MKV::EBML::Core::GetVIntLength(*p);
		long long result = MKV::EBML::Core::StripZeroBitInNum(*p,ebml_size - 1);
		if (ebml_size > 1)
		{
			for (unsigned i = 1;i < ebml_size;i++)
			{
				result <<= 8;
				result |= p[i];
			}
		}

		unsigned true_size = MKV::EBML::Core::GetEbmlBytes(result);
		if (true_size != ebml_size)
			_cluster_is_fixed_size = ebml_size;
	}

	return true;
}

unsigned MatroskaSegment::NextCluster(long long clusterSize) throw()
{
	if (clusterSize == 0)
		return MKV_ERR_INVALID_SIZE;

	_clusterHead.SetSimpleCore(MKV_ID_L1_CLUSTER,clusterSize);
	if (!_cluster.ParseCluster(_clusterHead,_user_req_calc_cluster_pos,_cluster_is_fixed_size))
		return MKV_ERR_INVALID_CLUSTER;

	return MKV_ERR_OK;
}

unsigned MatroskaSegment::NextClusterWithStart() throw()
{
	EBML::EbmlHead head;
	if (!EBML::FastParseEbmlHeadIfSize(head,_dataSource))
		return MKV_ERR_FAIL;

	return NextCluster(head.Size());
}