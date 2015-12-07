#ifndef _MATROSKA_INTERNAL_CUES_H
#define _MATROSKA_INTERNAL_CUES_H

#include "MatroskaList.h"
#include "MatroskaEbml.h"
#include "MatroskaInternalCueContext.h"

#define __DEFAULT_MKV_CUES 20

namespace MKV {
namespace Internal {
namespace Object {

class Cues
{
public:
	explicit Cues(EBML::EbmlHead& head) throw() : _head(head) {}

public:
	bool ParseCues(long long size) throw();

	unsigned GetCuePointCount() throw() { return _list.GetCount(); }
	Context::CuePoint* GetCuePoint(unsigned index) throw()
	{
		if (index >= _list.GetCount())
			return nullptr;

		return _list.GetItem(index);
	}

private:
	bool ParseCuePoint(Context::CuePoint& point,long long size) throw();
	bool ParseCueTrackPositions(Context::CueTrackPositions& pos,long long size) throw();

private:
	EBML::EbmlHead& _head;
	MatroskaList<Context::CuePoint,__DEFAULT_MKV_CUES> _list;
};

}}} //MKV::Internal::Object.

#endif //_MATROSKA_INTERNAL_CUES_H