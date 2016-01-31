#pragma once

#include "FFmpegDecodeServices.h"

class FFmpegAudioDecoder
{
public:
	FFmpegAudioDecoder() {}
	~FFmpegAudioDecoder() { Close(); }

public:
	IMFMediaType* Open(AVCodecID codecid, IMFMediaType* pMediaType);
	void Close();

	HRESULT Decode(IMFSample* pSample, IMFSample** ppDecodedSample);
	void Flush();

private:
	HRESULT Process(const BYTE* buf, unsigned size, LONG64 dts, bool discontinuity);
	HRESULT CreateDecodedSample(AVFrame* frame, IMFSample** ppSample);
	void PackCopyDecodedContent(BYTE* copyTo, unsigned copySize, AVFrame* frame);

	IMFMediaType* CreateResultMediaType(AVCodecContext* ctx);

private:
	AVCodecContext* _context;
	AVFrame* _frame;

	WAVEFORMATEX _wfx;
	LONG64 _timestamp, _saved_duration;
	int _prev_nb_samples;
	bool _is_planar;
};