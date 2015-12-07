#ifndef _MATROSKA_INTERNAL_CUE_CONTEXT_H
#define _MATROSKA_INTERNAL_CUE_CONTEXT_H

#include "MatroskaEbml.h"

namespace MKV {
namespace Internal {
namespace Object {
namespace Context {

struct CueTrackPositions
{
	unsigned CueTrack;
	long long CueClusterPosition;
	long long CueRelativePosition;
	long long CueDuration;
	long long CueBlockNumber;

	void ToDefault() throw()
	{
		CueTrack = 0;

		CueClusterPosition = CueRelativePosition = -1;
		CueDuration = 0;
		CueBlockNumber = 1;
	}

	void FreeResources() throw() {}
};

struct CuePoint
{
	long long CueTime;
	CueTrackPositions Positions;

	void ToDefault() throw()
	{
		Positions.ToDefault();
		CueTime = -1;
	}

	void FreeResources() throw() {}
};

}}}} //MKV::Internal::Object::Context.

#endif //_MATROSKA_INTERNAL_CUE_CONTEXT_H