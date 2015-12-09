#ifndef _MATROSKA_INTERNAL_TRACK_H
#define _MATROSKA_INTERNAL_TRACK_H

#include "MatroskaList.h"
#include "MatroskaEbml.h"
#include "MatroskaInternalTrackContext.h"

#define __DEFAULT_MKV_TRACKS 2

namespace MKV {
namespace Internal {
namespace Object {

class Tracks
{
public:
	explicit Tracks(EBML::EbmlHead& head) throw() : _head(head) {}
	~Tracks()
	{
		for (unsigned i = 0;i < _list.GetCount();i++)
			_list.GetItem(i)->FreeResources();
	}

public:
	bool ParseTracks(long long size) throw();

	unsigned GetTrackCount() throw() { return _list.GetCount(); }
	Context::TrackEntry* GetTrackEntry(unsigned index) throw()
	{
		if (index >= _list.GetCount())
			return nullptr;

		return _list.GetItem(index);
	}

private:
	bool ParseTrackEntry(Context::TrackEntry& entry,long long size) throw();
	
	bool ParseAudioTrack(Context::AudioTrack& audio,long long size) throw();
	bool ParseVideoTrack(Context::VideoTrack& video,long long size) throw();

	bool ParseEncodings(Context::TrackEntry& entry,long long size) throw();
	bool ParseEncoding(Context::TrackEncoding& enc,long long size) throw();
	bool ParseCompression(Context::TrackCompression& comp,long long size) throw();
	bool ParseEncryption(Context::TrackEncryption& crypt,long long size) throw()
	{
		//NOT IMPL.
		_head.Skip();
		return true;
	}

private:
	EBML::EbmlHead& _head;
	MatroskaList<Context::TrackEntry,__DEFAULT_MKV_TRACKS> _list;
};

}}} //MKV::Internal::Object.

#endif //_MATROSKA_INTERNAL_TRACK_H