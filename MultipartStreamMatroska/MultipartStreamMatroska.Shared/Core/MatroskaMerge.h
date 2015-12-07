#ifndef __MATROSKA_MERGE_H
#define __MATROSKA_MERGE_H

#include "MergeManager.h"
#include "mkv/AkaMatroska.h"

class MatroskaMerge : public MergeManager, public AkaMatroska::Core::IOCallback
{
	AkaMatroska::Matroska* _muxer;
	bool _no_key_index;

	MemoryStream* _head_stream;
	MemoryBuffer _audio_extradata, _video_extradata;

public:
	MatroskaMerge(bool no_key_index = false) throw() : 
		_muxer(NULL), _no_key_index(no_key_index), _head_stream(NULL) {}
	virtual ~MatroskaMerge() throw() { if (_muxer) delete _muxer; }

private:
	bool CreateTracks();
	bool InitMatroskaTrack(DemuxProxy::Track* st, AkaMatroska::Objects::Track* dt);

protected:
	virtual bool OnNewInput(MergeManager::IOCallback* old_io, MergeManager::IOCallback* new_io);

	virtual bool DoProcessHeader(double total_duration);
	virtual bool DoProcessComplete(double total_duration);
	virtual bool DoProcessPacketOnce(Packet* pkt);

	unsigned OnWrite(const void* buf, unsigned size);
	bool OnSeek(int64_t offset);
	int64_t OnTell();

public:
	virtual unsigned Write(const void* buf, unsigned size)
	{ return OnWrite(buf, size); }
	virtual int64_t GetPosition()
	{ return OnTell(); }
	virtual int64_t SetPosition(int64_t offset)
	{ if (!OnSeek(offset)) return -1; return OnTell(); }

	virtual void* GetWorker() { return _muxer; }
};

#endif //__MATROSKA_MERGE_H