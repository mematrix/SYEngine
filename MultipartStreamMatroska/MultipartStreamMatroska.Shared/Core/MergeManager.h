#ifndef __MERGE_MANAGER_H
#define __MERGE_MANAGER_H

#include "DemuxProxy.h"
#include "MemoryStream.h"

class MergeManager
{
public:
	struct IOCallback
	{
		virtual int Read(void* buf, unsigned size) = 0;
		virtual int Write(const void* buf, unsigned size) = 0;
		virtual bool Seek(int64_t offset, int whence = SEEK_SET) = 0;
		virtual int64_t Tell() = 0;
		virtual int64_t Size() { return 0; }
		virtual void* GetPrivate() { return NULL; }
	protected:
		virtual ~IOCallback() {}
	};

	class DemuxCore : public DemuxProxy
	{
		IOCallback* Callback;
	public:
		explicit DemuxCore(IOCallback* cb) throw() : Callback(cb) {}
		virtual ~DemuxCore() throw() {}
	protected:
		virtual int ByteStreamRead(void* buf, int size)
		{ return Callback->Read(buf, size); }
		virtual bool ByteStreamSeek(int64_t offset, int whence)
		{ return Callback->Seek(offset, whence); }
		virtual int64_t ByteStreamTell()
		{ return Callback->Tell(); }
		virtual int64_t ByteStreamSize()
		{ return Callback->Size(); }
	};

public:
	MergeManager() throw() : _demux(NULL)
	{ _input = _output = NULL; }
	virtual ~MergeManager() throw()
	{ if (_demux) delete _demux; }

public:
	bool PutNewInput(IOCallback* cb);
	bool PutNewOutput(IOCallback* cb);

	void SetTimeOffset(double audio_offset, double video_offset)
	{ _audio_time_offset = audio_offset; _video_time_offset = video_offset; }

	bool ProcessHeader(double total_duration = 0.0, bool auto_duration = false);
	bool ProcessComplete(double total_duration = 0.0);

	void ProcessSkipPacket();
	int ProcessPacketOnce(double* cur_timestamp = NULL);

	inline MemoryStream* GetStreamHeader() { return &_header; };
	inline DemuxProxy* GetDemuxObject() { return static_cast<DemuxProxy*>(_demux); }

	virtual void* GetWorker() { return NULL; }
protected:
	struct Packet : public DemuxProxy::Packet
	{
		double ScaleDTS;
		double ScalePTS;
		double ScaleDuration;
	};
	static inline int64_t InvalidTimestamp()
	{ return ((int64_t)UINT64_C(0x8000000000000000)); }

	IOCallback* GetInputStream() { return _input; }
	IOCallback* GetOutputStream() { return _output; }

	virtual bool OnNewInput(IOCallback* old_io, IOCallback* new_io) { return true; }
	virtual bool OnNewOutput(IOCallback* old_io, IOCallback* new_io) { return true; }

	virtual bool DoProcessHeader(double total_duration) = 0;
	virtual bool DoProcessComplete(double total_duration) = 0;
	virtual bool DoProcessPacketOnce(Packet* pkt) = 0;

private:
	bool InternalNewDemux(double* duration = NULL);
	bool InternalInitDemux(void* demux);

	DemuxCore* _demux;
	MemoryStream _header;

	IOCallback* _input;
	IOCallback* _output;

	int _audio_ts, _video_ts; //视频和音频的timescale
	int _prev_audio_duration, _prev_video_duration; //预保存的av上一个帧的时长
	int64_t _prev_audio_time, _prev_video_time; //上一个帧的时间，dts优先pts
	double _audio_time_offset, _video_time_offset; //时间偏移
	double _start_time_offset; //such as IQIYI
};

#endif //__MERGE_MANAGER_H