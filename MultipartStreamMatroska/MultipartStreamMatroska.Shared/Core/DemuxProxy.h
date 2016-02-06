#ifndef __DEMUX_PROXY_H
#define __DEMUX_PROXY_H

#include <stdint.h>
#include <memory.h>

class DemuxProxy
{
protected:
	DemuxProxy() throw() { memset(&_core, 0, sizeof(_core)); }
	virtual ~DemuxProxy() throw()
	{ if (_core.FormatContext) CloseByteStream(); }

public:
	bool OpenByteStream(int io_buf_size = 0, bool non_find_stream_info = true);
	void CloseByteStream();

	const char* GetFormatName() throw() { return _format_name; }
	double GetStartTime();
	double GetDuration() const throw()
	{ return _core.TotalDuration; }

	struct Track
	{
		enum TrackType
		{
			Unknown,
			Audio,
			Video
		};
		TrackType Type;
		int Id;
		int Timescale;
		char CodecName[32];
		const unsigned char* CodecPrivate;
		unsigned CodecPrivateSize;
	};
	bool GetTrack(int index, Track* track) throw();
	int GetTrackCount() const throw()
	{ return _core.TotalTracks; }
	int GetDefaultTrackIndex(Track::TrackType type) throw();

	struct Packet
	{
		int Id;
		int64_t DTS, PTS;
		int64_t Duration;
		bool KeyFrame, Discontinuity;
		const unsigned char* Data;
		unsigned DataSize;
	};
	bool ReadPacket(Packet* pkt) throw();
	bool TimeSeek(double seconds) throw();

protected:
	virtual int ByteStreamRead(void* buf, int size) = 0;
	virtual bool ByteStreamSeek(int64_t offset, int whence) = 0;
	virtual int64_t ByteStreamTell() = 0;
	virtual int64_t ByteStreamSize() = 0;

private:
	static int avioRead(void* opaque, unsigned char* buf, int size)
	{ return ((DemuxProxy*)opaque)->ByteStreamRead(buf, size); }

	static int64_t avioSeek(void* opaque, int64_t offset, int whence)
	{
		if (whence == 0x10000)
			return ((DemuxProxy*)opaque)->ByteStreamSize();
		if (!((DemuxProxy*)opaque)->ByteStreamSeek(offset, whence))
			return -1;
		return ((DemuxProxy*)opaque)->ByteStreamTell();
	}

private:
	struct ProxyCore
	{
		void* IOContext;
		void* FormatContext;
		void* NowPacket;
		int TotalTracks;
		double TotalDuration;
	};
	ProxyCore _core;
	const char* _format_name;
};

#endif //__DEMUX_PROXY_H