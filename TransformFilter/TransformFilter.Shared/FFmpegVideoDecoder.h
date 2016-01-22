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

private:
	IMFMediaType* CreateResultMediaType(REFGUID outputFormat);

	struct Decoder
	{
		AVCodec* codec;
		AVCodecContext* context;
		AVFrame* frame;
	};
	Decoder _decoder;
	ComPtr<IMFMediaType> _rawMediaType;
};