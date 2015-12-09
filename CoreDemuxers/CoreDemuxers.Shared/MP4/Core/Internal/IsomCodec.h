#ifndef __ISOM_CODEC_H_
#define __ISOM_CODEC_H_

#include "IsomGlobal.h"
#include "StructTable.h"

namespace Isom {

class CodecSampleEntryParser
{
public:
	struct CodecInfo
	{
		unsigned MainFcc;

		struct VideoInfo
		{
			uint16_t Width;
			uint16_t Height;
			unsigned HorizResolution;
			unsigned VertResolution;
			uint16_t FrameCount;
			uint16_t Depth;
			unsigned ParNum, ParDen; //pasp
		};
		VideoInfo Video;

		struct AudioInfo
		{
			uint16_t ChannelCount;
			uint16_t SampleSize;
			unsigned SampleRate;
			unsigned QT_Version;
			unsigned QT_SamplesPerFrame;
			unsigned QT_BytesPerFrame;
		};
		AudioInfo Audio;

		unsigned CodecId;
		unsigned char* CodecPrivate;
		unsigned CodecPrivateSize;
		struct Esds
		{
			unsigned ObjectTypeIndication; // == 0 is invalid.
			unsigned StreamType;
			unsigned MaxBitrate, AvgBitrate;
			unsigned DecoderConfigSize;
			unsigned char* DecoderConfig;
		};
		Esds esds;
	};

public:
	CodecSampleEntryParser() throw()
	{ memset(&_info, 0, sizeof(_info)); }
	~CodecSampleEntryParser() throw()
	{ if (_info.CodecPrivate) free(_info.CodecPrivate);
		if (_info.esds.DecoderConfig) free(_info.esds.DecoderConfig); }

	bool ParseSampleEntry(unsigned type,const unsigned char* data, unsigned size) throw();
	CodecInfo* GetCodecInfo() throw() { return &_info; }

private:
	bool IsAudioEntry(unsigned type) throw();
	bool IsVideoEntry(unsigned type) throw();

	unsigned InitAudioSampleEntry(unsigned type, const unsigned char* data, unsigned size);
	unsigned InitVideoSampleEntry(unsigned type, const unsigned char* data, unsigned size);

	void ScanChildBoxToList(const unsigned char* data, unsigned size);
	void ProcessChildBoxs();
	void ProcessVideoPasp(unsigned index);

	void ParseEsdsCodecPrivate();
	void ProcessEsdsBoxs(const unsigned char* esds, unsigned size);
	unsigned GetEsdsDescLen(const unsigned char* data, unsigned size, unsigned* len); //return offset.

	unsigned OnEsdsESDescriptor(const unsigned char* data, unsigned size);
	unsigned OnEsdsDecoderConfigDescriptor(const unsigned char* data, unsigned size);
	unsigned OnEsdsDecoderSpecificInfo(const unsigned char* data, unsigned size);

private:
	CodecInfo _info;
	struct SubBox
	{
		unsigned DataType;
		unsigned char* DataPointer;
		unsigned DataSize;
	};
	StructTable<SubBox> _boxs;
};

}

#endif //__ISOM_CODEC_H_