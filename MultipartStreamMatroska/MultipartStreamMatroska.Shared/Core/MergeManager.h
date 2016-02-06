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
	MergeManager() throw() : _demux(NULL), _non_adjust_timestamp_offset(false)
	{ _input = _output = NULL; }
	virtual ~MergeManager() throw()
	{ if (_demux) delete _demux; }

public:
	bool PutNewInput(IOCallback* cb, bool no_check_demux_null = false);
	bool PutNewOutput(IOCallback* cb);

	void SetTimeOffset(double audio_offset, double video_offset)
	{ _audio_time_offset = audio_offset; _video_time_offset = video_offset; }
	void SetForceUseDuration()
	{ _non_adjust_timestamp_offset = true; }

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

	int GetAudioIndex() const throw() { return _audio_index; }
	int GetVideoIndex() const throw() { return _video_index; }

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

	int _audio_index, _video_index;
	int _audio_ts, _video_ts; //��Ƶ����Ƶ��timescale
	int _prev_audio_duration, _prev_video_duration; //Ԥ�����av��һ��֡��ʱ��
	int64_t _prev_audio_time, _prev_video_time; //��һ��֡��ʱ�䣬dts����pts
	double _audio_time_offset, _video_time_offset; //ʱ��ƫ��
	double _start_time_offset; //such as IQIYI
	int64_t _start_time_offset_audio_no_scale, _start_time_offset_video_no_scale;

	bool _non_adjust_timestamp_offset;
};

#endif //__MERGE_MANAGER_H