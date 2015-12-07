#ifndef _MATROSKA_INTERNAL_TRACK_CONTEXT_H
#define _MATROSKA_INTERNAL_TRACK_CONTEXT_H

#include "MatroskaEbml.h"

namespace MKV {
namespace Internal {
namespace Object {
namespace Context {

struct AVTrack
{
	char* Name; //UTF8
	char Language[32]; //und
	char CodecID[128];
	unsigned char* CodecPrivate;
	unsigned CodecPrivateSize;
	long long CodecDelay; //mkv V3
	long long SeekPreroll; //mkv V3

	void ToDefault() throw()
	{
		Name = nullptr;
		Language[0] = 0;
		CodecID[0] = 0;
		CodecPrivate = nullptr;
		CodecDelay = 0;
		SeekPreroll = 0;

		CodecPrivateSize = 0;
	}

	void FreeResources() throw()
	{
		if (Name != nullptr)
		{
			free(Name);
			Name = nullptr;
		}

		if (CodecPrivate != nullptr)
		{
			free(CodecPrivate);
			CodecPrivate = nullptr;

			CodecPrivateSize = 0;
		}
	}

	bool SetName(EBML::EbmlHead& head)
	{
		if (Name != nullptr)
			free(Name);

		Name = (char*)malloc(MKV_ALLOC_ALIGNED((unsigned)head.Size()));
		if (Name == nullptr)
			return false;
		if (!EBML::EbmlReadString(head,Name))
			return false;

		return true;
	}
	
	bool SetCodecPrivate(EBML::EbmlHead& head)
	{
		if (CodecPrivate != nullptr)
			free(CodecPrivate);

		CodecPrivate = (unsigned char*)malloc(MKV_ALLOC_ALIGNED((unsigned)head.Size()));
		if (CodecPrivate == nullptr)
			return false;
		if  (!head.Read(CodecPrivate))
			return false;

		CodecPrivateSize = (unsigned)head.Size();
		return true;
	}
};

struct AudioTrack
{
	float SamplingFrequency; //8000.0
	float OutputSamplingFrequency; //=SamplingFrequency
	unsigned Channels; //1
	unsigned BitDepth;

	void ToDefault() throw()
	{
		SamplingFrequency = OutputSamplingFrequency = 8000.0f;
		Channels = 1;
		BitDepth = 0;
	}
	void FreeResources() throw() {}
};
struct VideoTrack
{
	bool fInterlaced; //false
	unsigned StereoMode; //0
	unsigned AlphaMode; //0
	unsigned PixelWidth;
	unsigned PixelHeight;
	unsigned PixelCropBottom; //0
	unsigned PixelCropTop; //0
	unsigned PixelCropLeft; //0
	unsigned PixelCropRight; //0
	unsigned DisplayWidth; //=PixelWidth
	unsigned DisplayHeight; //=PixelHeight
	unsigned DisplayUnit; //0
	unsigned AspectRatioType; //0
	char ColourSpace[4]; //Same value as in AVI (32 bits).

	void ToDefault() throw()
	{
		fInterlaced = false;
		StereoMode = AlphaMode = AspectRatioType = 0;
		PixelWidth = PixelHeight = 
			PixelCropBottom = PixelCropTop = 
			PixelCropLeft = PixelCropRight = 0;
		DisplayWidth = DisplayHeight = DisplayUnit = 0;

		ColourSpace[0] = ColourSpace[1] = ColourSpace[2] = ColourSpace[3] = 0;
	}
	void FreeResources() throw() {}
};

struct TrackCompression
{
	int ContentCompAlgo; //0
	unsigned char* ContentCompSettings;

	unsigned ContentCompSettingsSize;

	void ToDefault() throw()
	{
		ContentCompAlgo = 0;
		ContentCompSettings = nullptr;

		ContentCompSettingsSize = 0;
	}

	void FreeResources() throw()
	{
		if (ContentCompSettings != nullptr)
		{
			free(ContentCompSettings);
			ContentCompSettings = nullptr;

			ContentCompSettingsSize = 0;
		}
	}

	bool SetContentCompSettings(EBML::EbmlHead& head)
	{
		if (ContentCompSettings != nullptr)
			free(ContentCompSettings);

		ContentCompSettings = (unsigned char*)malloc(MKV_ALLOC_ALIGNED((unsigned)head.Size()));
		if (ContentCompSettings == nullptr)
			return false;
		if  (!head.Read(ContentCompSettings))
			return false;

		ContentCompSettingsSize = (unsigned)head.Size();
		return true;
	}
};
struct TrackEncryption
{
	int ContentEncAlgo; //0
	unsigned char ContentEncKeyID[256];

	void ToDefault() throw()
	{
		ContentEncAlgo = 0;
		memset(ContentEncKeyID,0,256);
	}
	void FreeResources() throw() {}
};

struct TrackEncoding
{
	unsigned ContentEncodingScope; //1
	unsigned ContentEncodingType; //0
	bool fCompressionExists;
	bool fEncryptionExists;
	TrackCompression Compression;
	TrackEncryption Encryption;

	void ToDefault() throw()
	{
		Compression.ToDefault();
		Encryption.ToDefault();

		ContentEncodingScope = 1;
		ContentEncodingType = 0;

		fCompressionExists = fEncryptionExists = false;
	}

	void FreeResources() throw()
	{
		Compression.FreeResources();
		Encryption.FreeResources();
	}
};

struct TrackEntry
{
	unsigned TrackNumber;
	unsigned TrackType;
	long long TrackUID;
	float TrackTimecodeScale; //1.0 DEPRECATED.
	bool fEnabled; //true
	bool fDefault; //true
	bool fForced; //false
	bool fLacing; //true
	unsigned MinCache; //0
	unsigned MaxCache;
	long long DefaultDuration;
	long long MaxBlockAdditionID;
	bool CodecDecodeAll;
	AVTrack Track;
	AudioTrack Audio;
	VideoTrack Video;
	TrackEncoding Encoding;

	void ToDefault() throw()
	{
		Track.ToDefault();
		Audio.ToDefault();
		Video.ToDefault();
		Encoding.ToDefault();

		TrackNumber = TrackType = 0;
		TrackUID = 0;
		TrackTimecodeScale = 1.0f;
		fEnabled = fDefault = fLacing = true;
		fForced = false;
		MinCache = MaxCache = 0;
		DefaultDuration = MaxBlockAdditionID = 0;
		CodecDecodeAll = false;
	}

	void FreeResources() throw()
	{
		Track.FreeResources();
		Audio.FreeResources();
		Video.FreeResources();
		Encoding.FreeResources();
	}
};

}}}} //MKV::Internal::Object::Context.

#endif //_MATROSKA_INTERNAL_TRACK_CONTEXT_H