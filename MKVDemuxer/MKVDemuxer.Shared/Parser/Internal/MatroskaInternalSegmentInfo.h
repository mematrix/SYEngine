#ifndef _MATROSKA_INTERNAL_SEGMENTINFO_H
#define _MATROSKA_INTERNAL_SEGMENTINFO_H

#include "MatroskaEbml.h"

namespace MKV {
namespace Internal {
namespace Object {

class SegmentInfo
{
public:
	explicit SegmentInfo(EBML::EbmlHead& head) throw() : _head(head)
	{
		memset(&_info,0,sizeof(_info));
	}

public:
	bool ParseSegmentInfo(long long size) throw();

public:
	const char* GetTitle() const throw() { return _info.Title; }
	const char* GetMuxingApp() const throw() { return _info.MuxingApp; }
	const char* GetWritingApp() const throw() { return _info.WritingApp; }

	long long GetTimecodeScale() const throw() { return _info.TimecodeScale; }
	double GetDuration() const throw() { return _info.Duration; }

	bool IsTimecodeMilliseconds() const throw() { return _info.TimecodeScale == 1000000; }

private:
	EBML::EbmlHead& _head;

	struct Info
	{
		char Title[1024];
		char MuxingApp[128];
		char WritingApp[256];
		long long TimecodeScale;
		double Duration;
		unsigned char Uuid[16];
	};
	Info _info;
};

}}} //MKV::Internal::Object.

#endif //_MATROSKA_INTERNAL_SEGMENTINFO_H