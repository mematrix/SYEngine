#include "FFmpegAudioDecoder.h"
#include "more_codec_uuid.h"

IMFMediaType* FFmpegAudioDecoder::Open(AVCodecID codecid, IMFMediaType* pMediaType)
{
	auto codec = avcodec_find_decoder(codecid);
	if (codec == NULL)
		return NULL;

	_context = avcodec_alloc_context3(codec);
	_frame = av_frame_alloc();
	if (_context == NULL || _frame == NULL)
		return NULL;

	_timestamp = 0;
	_saved_duration = 0;
	_prev_nb_samples = 0;

	_context->channels = (int)MFGetAttributeUINT32(pMediaType, MF_MT_AUDIO_NUM_CHANNELS, 0);
	_context->sample_rate = (int)MFGetAttributeUINT32(pMediaType, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
	_context->bits_per_coded_sample = (int)MFGetAttributeUINT32(pMediaType, MF_MT_AUDIO_BITS_PER_SAMPLE, 0);
	_context->block_align = (int)MFGetAttributeUINT32(pMediaType, MF_MT_AUDIO_BLOCK_ALIGNMENT, 0);
	_context->bit_rate = (int)MFGetAttributeUINT32(pMediaType, MF_MT_AVG_BITRATE, 0);
	_context->refcounted_frames = 1;
	_context->err_recognition = 0;
	_context->thread_count = 1;
	_context->thread_type = 0;

	AVRational mftb = {10000000, 1};
	av_codec_set_pkt_timebase(_context, mftb);

	bool error = false;
	PBYTE userdata = NULL;
	UINT32 userdataLen = 0;
	pMediaType->GetAllocatedBlob(MF_MT_USER_DATA, &userdata, &userdataLen);
	if (userdata) {
		_context->extradata = (unsigned char*)av_mallocz(userdataLen * 2 + FF_INPUT_BUFFER_PADDING_SIZE);
		if (_context->extradata) {
			_context->extradata_size = (int)userdataLen;
			if (codecid == AV_CODEC_ID_AAC) {
				if (userdataLen < 14) {
					_context->extradata_size = 0;
					av_freep(&_context->extradata);
				}else{
					_context->extradata_size = userdataLen - 12;
					memcpy(_context->extradata, userdata + 12, userdataLen - 12);
				}
			}else if (codecid == AV_CODEC_ID_ALAC) {
				static const unsigned char alac_head[12] = {0x00, 0x00, 0x00, 0x24, 0x61, 0x6C, 0x61, 0x63, 0x00, 0x00, 0x00, 0x00};
				memcpy(_context->extradata, alac_head, 12);
				memcpy(_context->extradata + 12, userdata, userdataLen);
				_context->extradata_size = (int)userdataLen + 12;
			}else{
				memcpy(_context->extradata, userdata, userdataLen);
			}
		}
		CoTaskMemFree(userdata);
	}
	if (error)
		return NULL;

	if (codecid == AV_CODEC_ID_FLAC)
		_context->request_sample_fmt = AV_SAMPLE_FMT_S16;

	int result = avcodec_open2(_context, codec, NULL);
	if (result < 0) {
		Close();
		return NULL;
	}
	if (_context->channels == 0 || _context->channels > 2 || _context->sample_rate < 8000) {
		Close();
		return NULL;
	}
	if (_context->sample_fmt == AVSampleFormat::AV_SAMPLE_FMT_NONE ||
		_context->sample_fmt == AVSampleFormat::AV_SAMPLE_FMT_DBL ||
		_context->sample_fmt == AVSampleFormat::AV_SAMPLE_FMT_DBLP) {
		Close();
		return NULL;
	}

	_is_planar = av_sample_fmt_is_planar(_context->sample_fmt) ? true:false;
	return CreateResultMediaType(_context);
}

void FFmpegAudioDecoder::Close()
{
	if (_context) {
		avcodec_close(_context);
		av_freep(&_context->extradata);
		av_freep(&_context);
	}
	av_frame_free(&_frame);
}

HRESULT FFmpegAudioDecoder::Decode(IMFSample* pSample, IMFSample** ppDecodedSample)
{
	if (pSample == NULL)
		return E_INVALIDARG;

	LONG64 pts = 0;

	ComPtr<IMFMediaBuffer> pBuffer;
	HRESULT hr = pSample->ConvertToContiguousBuffer(&pBuffer);
	if (FAILED(hr))
		return hr;

	PBYTE buf = NULL;
	DWORD size = 0;
	hr = pBuffer->Lock(&buf, NULL, &size);
	if (FAILED(hr))
		return hr;

	if (size == 0) {
		pBuffer->Unlock();
		return MF_E_TRANSFORM_NEED_MORE_INPUT;
	}else{
		hr = pSample->GetSampleTime(&pts);
		if (FAILED(hr) && _timestamp == -1) {
			pBuffer->Unlock();
			return MF_E_INVALID_TIMESTAMP;
		}
		if (_timestamp == -1)
			_timestamp = pts;

		bool discontinuity = MFGetAttributeUINT32(pSample, MFSampleExtension_Discontinuity, 0) ? true:false;
		hr = Process(buf, size, pts, discontinuity);

		pBuffer->Unlock();
	}
	if (FAILED(hr))
		return hr;

	LONG64 duration;
	if (_frame->nb_samples == _prev_nb_samples) {
		duration = _saved_duration;
	}else{
		unsigned len = _frame->nb_samples * _wfx.nBlockAlign;
		duration = _saved_duration = (LONG64)len * 10000000 / _wfx.nAvgBytesPerSec;
		_prev_nb_samples = _frame->nb_samples;
	}

	ComPtr<IMFSample> pDecodedSample;
	hr = CreateDecodedSample(_frame, &pDecodedSample);
	if (SUCCEEDED(hr)) {
		pDecodedSample->SetSampleTime(_timestamp);
		pDecodedSample->SetSampleDuration(duration);
		*ppDecodedSample = pDecodedSample.Detach();
	}

	av_frame_unref(_frame);
	_timestamp += duration;
	return hr;
}

void FFmpegAudioDecoder::Flush()
{
	_timestamp = -1;
	if (_context)
		avcodec_flush_buffers(_context);
}

HRESULT FFmpegAudioDecoder::Process(const BYTE* buf, unsigned size, LONG64 dts, bool discontinuity)
{
	if (discontinuity)
		avcodec_flush_buffers(_context);

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = (uint8_t*)buf;
	pkt.size = size;
	pkt.dts = dts > 0 ? dts : AV_NOPTS_VALUE;

	int got_picture = 0;
	int bytes = avcodec_decode_audio4(_context, _frame, &got_picture, &pkt);
	if (bytes < 0) {
		av_frame_unref(_frame);
		return E_FAIL;
	}
	if (got_picture == 0 || _frame->nb_samples == 0) {
		av_frame_unref(_frame);
		return MF_E_TRANSFORM_NEED_MORE_INPUT;
	}

	return S_OK;
}

HRESULT FFmpegAudioDecoder::CreateDecodedSample(AVFrame* frame, IMFSample** ppSample)
{
	if (frame->channels != (int)_wfx.nChannels ||
		frame->sample_rate != (int)_wfx.nSamplesPerSec)
		return MF_E_TRANSFORM_STREAM_CHANGE;

	unsigned len = frame->nb_samples * _wfx.nBlockAlign;
	ComPtr<IMFSample> pSample;
	ComPtr<IMFMediaBuffer> pBuffer;

	HRESULT hr = MFCreateSample(&pSample);
	if (FAILED(hr))
		return hr;
	hr = MFCreateMemoryBuffer(len << 1, &pBuffer);
	if (FAILED(hr))
		return hr;

	PBYTE buf = NULL;
	hr = pBuffer->Lock(&buf, NULL, NULL);
	if (FAILED(hr))
		return hr;

	PackCopyDecodedContent(buf, len, frame);

	pBuffer->Unlock();
	pBuffer->SetCurrentLength(len);

	pSample->AddBuffer(pBuffer.Get());
	*ppSample = pSample.Detach();
	return S_OK;
}

void FFmpegAudioDecoder::PackCopyDecodedContent(BYTE* copyTo, unsigned copySize, AVFrame* frame)
{
	if (!_is_planar) {
		memcpy(copyTo, frame->data[0], copySize);
	}else{
		switch (_context->sample_fmt)
		{
		case AV_SAMPLE_FMT_U8P: {
			auto offset = (uint8_t*)copyTo;
			for (int i = 0; i < frame->nb_samples; i++)
				for (int ch = 0; ch < frame->channels; ch++)
					*offset++ = ((uint8_t*)frame->extended_data[ch])[i];
			}
			break;
		case AV_SAMPLE_FMT_S16P: {
			auto offset = (int16_t*)copyTo;
			for (int i = 0; i < frame->nb_samples; i++)
				for (int ch = 0; ch < frame->channels; ch++)
					*offset++ = ((int16_t*)frame->extended_data[ch])[i];
			}
			break;
		case AV_SAMPLE_FMT_S32P: {
			auto offset = (int32_t*)copyTo;
			for (int i = 0; i < frame->nb_samples; i++)
				for (int ch = 0; ch < frame->channels; ch++)
					*offset++ = ((int32_t*)frame->extended_data[ch])[i];
			}
			break;
		case AV_SAMPLE_FMT_FLTP: {
			auto offset = (float*)copyTo;
			for (int i = 0; i < frame->nb_samples; i++)
				for (int ch = 0; ch < frame->channels; ch++)
					*offset++ = ((float*)frame->extended_data[ch])[i];
			}
			break;
		}
	}
}

IMFMediaType* FFmpegAudioDecoder::CreateResultMediaType(AVCodecContext* ctx)
{
	WAVEFORMATEX wfx = {};
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = ctx->channels;
	wfx.nSamplesPerSec = ctx->sample_rate;
	wfx.wBitsPerSample = av_get_bytes_per_sample(ctx->sample_fmt) * 8;
	wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

	if (ctx->sample_fmt == AVSampleFormat::AV_SAMPLE_FMT_FLT ||
		ctx->sample_fmt == AVSampleFormat::AV_SAMPLE_FMT_FLTP)
		wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;

	ComPtr<IMFMediaType> pMediaType;
	if (FAILED(MFCreateMediaType(&pMediaType)))
		return NULL;

	if (FAILED(MFInitMediaTypeFromWaveFormatEx(pMediaType.Get(), &wfx, sizeof(wfx))))
		return NULL;

	memcpy(&_wfx, &wfx, sizeof(_wfx));
	pMediaType->DeleteItem(MF_MT_AUDIO_PREFER_WAVEFORMATEX);
	return pMediaType.Detach();
}