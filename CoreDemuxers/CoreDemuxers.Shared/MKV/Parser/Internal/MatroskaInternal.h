#ifndef _MATROSKA_INTERNAL_H
#define _MATROSKA_INTERNAL_H

#include "MatroskaList.h"
#include "MatroskaEbml.h"
#include "MatroskaInternalError.h"
#include "MatroskaInternalSeekHead.h"
#include "MatroskaInternalSegmentInfo.h"
#include "MatroskaInternalTracks.h"
#include "MatroskaInternalCluster.h"
#include "MatroskaInternalCues.h"
#include "MatroskaInternalChapters.h"
#include "MatroskaInternalAttachments.h"
#include "MatroskaInternalTags.h"
#include <memory>

#define MKV_SEGMENT_PARSE_SEEKHEAD 1
#define MKV_SEGMENT_PARSE_INFO 2
#define MKV_SEGMENT_PARSE_TRACKS 4
#define MKV_SEGMENT_PARSE_CUES 8
#define MKV_SEGMENT_PARSE_CHAPTERS 16
#define MKV_SEGMENT_PARSE_TAGS 32
#define MKV_SEGMENT_PARSE_FILES 64

#define IMPL_GET_CORE_INFO(name,core) \
	Object::name* Get##name() const throw() { return core.name.get(); }

namespace MKV {
namespace Internal {

class MatroskaSegment
{
public:
	MatroskaSegment(DataSource::IDataSource* dataSource) throw();

public:
	//Ԥ����MKVͷ��Ϣ����������Ϣ�͹����Ϣ����
	unsigned Parse(long long segmentSize,long long* first_cluster_size) throw();
	//ȡ�÷����˶�����Ϣ��
	unsigned StatusOfParse() const throw() { return _core.Status; }

	IMPL_GET_CORE_INFO(SeekHead,_core);
	IMPL_GET_CORE_INFO(SegmentInfo,_core);
	IMPL_GET_CORE_INFO(Tracks,_core);
	IMPL_GET_CORE_INFO(Cues,_core); //SeekHead Include.
	IMPL_GET_CORE_INFO(Chapters,_core); //SeekHead Include.
	IMPL_GET_CORE_INFO(Tags,_core); //SeekHead Include.
	IMPL_GET_CORE_INFO(Attachments,_core); //SeekHead Include.

	//�����SeekHead��Parse��������͸���SeekHead����Seek������Щû������L1Ԫ�ء�
	unsigned ExecuteSeekHead() throw();

public: //Cluster
	inline Object::Cluster* GetCurrentCluster() throw() { return &_cluster; }
	unsigned NextCluster(long long clusterSize) throw();
	unsigned NextClusterWithStart() throw();

	inline void SetShouldCalcClusterPosition(bool state = true) throw() { _user_req_calc_cluster_pos = state; }

private:
	bool IsParsed(unsigned id) throw();
	bool ProcessFirstClusterAndGetInfo() throw();

private:
	DataSource::IDataSource* _dataSource;
	unsigned _start_pos;
	unsigned _cluster_is_fixed_size;
	bool _user_req_calc_cluster_pos;

	struct Core
	{
		unsigned Status;
		std::shared_ptr<Object::SeekHead> SeekHead;
		std::shared_ptr<Object::SegmentInfo> SegmentInfo;
		std::shared_ptr<Object::Tracks> Tracks;
		std::shared_ptr<Object::Cues> Cues;
		std::shared_ptr<Object::Chapters> Chapters;
		std::shared_ptr<Object::Tags> Tags;
		std::shared_ptr<Object::Attachments> Attachments;
	};
	Core _core;

	Object::Cluster _cluster; //current cluster.
	EBML::EbmlHead _clusterHead;
};

}} //MKV::Internal.

#endif //_MATROSKA_INTERNAL_H