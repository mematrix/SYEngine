#pragma once

#include "FFmpegDecodeServices.h"

class FFmpegVideoDecoder
{
public:
	FFmpegVideoDecoder() { memset(&_decoder, 0, sizeof(_decoder)); }
	~FFmpegVideoDecoder() { Close(); }

public:
	IMFMediaType* Open(AVCodecID codecid, IMFMediaType* pMediaType);
	void Close();

	HRESULT Decode(IMFSample* pSample, IMFSample** ppDecodedSample);
	void Flush();

	inline void SetAllocator(ITransformAllocator* pAllocator) throw()
	{ _allocator = pAllocator; }

private:
	HRESULT Process(const BYTE* buf, unsigned size, LONG64 pts, LONG64 duration, bool keyframe, bool discontinuity);
	HRESULT CreateDecodedSample(AVFrame* frame, IMFSample** ppSample);
	
	bool OnceDecodeCallback();
	IMFMediaType* CreateResultMediaType(REFGUID outputFormat);

	struct Decoder
	{
		AVCodec* codec;
		AVCodecContext* context;
		AVFrame* frame;
		SwsContext* scaler;
		bool once_state;
	};
	Decoder _decoder;
	ComPtr<IMFMediaType> _rawMediaType;

	DWORD _image_size;
	LONG64 _default_duration;

	ComPtr<ITransformAllocator> _allocator;
};