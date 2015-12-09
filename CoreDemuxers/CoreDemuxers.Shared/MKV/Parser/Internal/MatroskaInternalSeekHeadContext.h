#ifndef _MATROSKA_INTERNAL_SEEKHEAD_CONTEXT_H
#define _MATROSKA_INTERNAL_SEEKHEAD_CONTEXT_H

#include "MatroskaEbml.h"

namespace MKV {
namespace Internal {
namespace Object {
namespace Context {

struct HeadSeekIndex
{
	unsigned SeekID;
	long long SeekPosition;

	void ToDefault() throw()
	{
		SeekID = INVALID_EBML_ID;
		SeekPosition = 0;
	}

	void FreeResources() throw() {}
};

}}}} //MKV::Internal::Object::Context.

#endif //_MATROSKA_INTERNAL_SEEKHEAD_CONTEXT_H