#ifndef _MATROSKA_INTERNAL_SEEKHEAD_H
#define _MATROSKA_INTERNAL_SEEKHEAD_H

#include "MatroskaList.h"
#include "MatroskaEbml.h"
#include "MatroskaInternalSeekHeadContext.h"

#define __DEFAULT_MKV_SEEKHEADS 5

namespace MKV {
namespace Internal {
namespace Object {

class SeekHead
{
public:
	explicit SeekHead(EBML::EbmlHead& head) throw() : _head(head) {}

public:
	bool ParseSeekHead(long long size) throw();

	unsigned GetSeekHeadCount() throw() { return _list.GetCount(); }
	Context::HeadSeekIndex* GetSeekHead(unsigned index) throw()
	{
		if (index >= _list.GetCount())
			return nullptr;

		return _list.GetItem(index);
	}

private:
	bool ParseSeekEntry(unsigned* id,long long* pos) throw();

private:
	EBML::EbmlHead& _head;
	MatroskaList<Context::HeadSeekIndex,__DEFAULT_MKV_SEEKHEADS> _list;
};

}}} //MKV::Internal::Object.

#endif //_MATROSKA_INTERNAL_SEEKHEAD_H