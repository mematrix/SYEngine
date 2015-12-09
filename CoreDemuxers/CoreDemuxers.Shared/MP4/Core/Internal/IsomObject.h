#ifndef __ISOM_OBJECT_H_
#define __ISOM_OBJECT_H_

#include "IsomGlobal.h"

namespace Isom {
namespace Object {

struct FileHeader
{
	unsigned MajorBand;
	unsigned MinorVersion;
	char CompBands[512];
};

struct MovieHeader
{
	int64_t CreationTime;
	int64_t ModificationTime;
	int32_t TimeScale;
	int64_t Duration;
};

struct TrackHeader
{
	enum TrackFlags
	{
		None = 0,
		Enabled = 1,
		InMovie = 2,
		InPreview = 4,
		InPoster = 8
	};
	unsigned Flags;
	int64_t CreationTime;
	int64_t ModificationTime;
	int32_t Id;
	int64_t Duration;
	int32_t Width, Height;
};

struct MediaHeader
{
	int64_t CreationTime;
	int64_t ModificationTime;
	int32_t TimeScale;
	int64_t Duration;
	int32_t LangId;
};

namespace Internal {

	struct BoxObject
	{
		unsigned type;
		int64_t size;
	};

	/*
	  在此处加入pts_offset，如果是音频轨道或者视频没有ctts box，会无故浪费很多内存。
	  正确的做法应该是保留ctts box的指针list，在运行时动态的调整，。。等等，运行时。。。。
	  本库并不提供运行时ReadSample功能，所以Parse()后就会将所有的Sample表导出到Track的Samples指针。
	  在此情况下，不得已，才浪费这样的内存，后续可以优化（比如使用指针判NULL代替等）。
	*/
	struct IndexObject
	{
		int32_t size;
		int64_t pos;
		double timestamp; //dts
		float pts_offset; //pts = dts + pts_offset;
		bool keyframe : 1;
		unsigned stsd_index : 3;

		inline bool IsInvalidTimestamp() const throw() { return timestamp == -1.0; }
		inline double GetDTS() const throw() { return timestamp; }
		inline double GetPTS() const throw() { return timestamp + double(pts_offset); }
	};

	struct sttsObject
	{
		int32_t count;
		int32_t duration;
	};

	struct stscObject
	{
		int32_t first;
		int32_t count;
		int32_t id;
	};

	struct elstObject
	{
		int64_t duration;
		int64_t time;
		float rate;
	};
}

}}

#endif //__ISOM_OBJECT_H_