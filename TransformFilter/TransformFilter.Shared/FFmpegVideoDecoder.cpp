#include "FFmpegVideoDecoder.h"
#include "more_codec_uuid.h"

static void UVCopyInterlace(unsigned char* data, const unsigned char* u, const unsigned char* v, unsigned count)
{
	unsigned n = (count + 7) >> 3;
	switch (count % 8)
	{
	case 0:
		do {
					*data++ = *u++;
			case 7: *data++ = *v++;
			case 6: *data++ = *u++;
			case 5: *data++ = *v++;
			case 4: *data++ = *u++;
			case 3: *data++ = *v++;
			case 2: *data++ = *u++;
			case 1: *data++ = *v++;
		} while (--n > 0);
	}
}

static void ConvertYUV420PToNV12Packed_NonLineSize(unsigned char* copyTo, unsigned char* y , unsigned char* u, unsigned char* v, unsigned width, unsigned height)
{
	unsigned ysize = width * height;
	MFCopyImage(copyTo, width, y, width, width, height); //memcpy(copyTo, y, ysize);
	UVCopyInterlace(copyTo + ysize, u, v, ysize >> 1);
}

static void ConvertYUV420PToNV12Packed_UseLineSize(unsigned char* copyTo, unsigned char* y , unsigned char* u, unsigned char* v, unsigned width, unsigned height, unsigned linesize_y, unsigned linesize_uv)
{
	unsigned ysize = width * height;
	unsigned char* copyToOffset = copyTo;
	//copy Y
	if (width == linesize_y) {
		MFCopyImage(copyTo, width, y, width, width, height);
		copyToOffset += ysize;
	}else{
		unsigned char* lumaYoffset = y;
		for (unsigned i = 0; i < height; i++, copyToOffset += width, lumaYoffset += linesize_y)
			memcpy(copyToOffset, lumaYoffset, width);
	}
	//copy UV
	unsigned uv_interlace_linesize = ysize / height;
	if ((uv_interlace_linesize >> 1) == linesize_uv) {
		UVCopyInterlace(copyToOffset, u, v, ysize >> 1);
	}else{
		unsigned char* lumaUoffset = u;
		unsigned char* lumaVoffset = v;
		unsigned count = height >> 1;
		for (unsigned i = 0; i < count; i++, copyToOffset += uv_interlace_linesize, lumaUoffset += linesize_uv, lumaVoffset += linesize_uv)
			UVCopyInterlace(copyToOffset, lumaUoffset, lumaVoffset, uv_interlace_linesize);
	}
}

static void ConvertYUV420PToYV12Packed_NonLineSize(unsigned char* copyTo, unsigned char* y , unsigned char* u, unsigned char* v, unsigned width, unsigned height)
{
	unsigned ysize = width * height;
	unsigned uvsize = ysize >> 2;
	MFCopyImage(copyTo, width, y, width, width, height); //memcpy(copyTo, y, ysize);
	memcpy(copyTo + ysize, v, uvsize);
	memcpy(copyTo + ysize + uvsize, u, uvsize);
}

static void ConvertYUV420PToYV12Packed_UseLineSize(unsigned char* copyTo, unsigned char* y , unsigned char* u, unsigned char* v, unsigned width, unsigned height, unsigned linesize_y, unsigned linesize_uv)
{
	unsigned ysize = width * height;
	unsigned uvsize = ysize >> 2;
	unsigned char* copyToOffset = copyTo;
	//copy Y
	if (width == linesize_y) {
		MFCopyImage(copyTo, width, y, width, width, height);
		copyToOffset += ysize;
	}else{
		unsigned char* lumaYoffset = y;
		for (unsigned i = 0; i < height; i++, copyToOffset += width, lumaYoffset += linesize_y)
			memcpy(copyToOffset, lumaYoffset, width);
	}
	//copy UV
	unsigned uv_interlace_linesize = ysize / height;
	unsigned uv_linesize = uv_interlace_linesize >> 1;
	if (uv_linesize == linesize_uv) {
		memcpy(copyToOffset, v, uvsize);
		memcpy(copyToOffset + uvsize, u, uvsize);
	}else{
		unsigned char* lumaUoffset = u;
		unsigned char* lumaVoffset = v;
		unsigned count = height >> 1;
		for (unsigned i = 0; i < count; i++, copyToOffset += uv_interlace_linesize, lumaUoffset += linesize_uv, lumaVoffset += linesize_uv) {
			memcpy(copyToOffset, v, uv_linesize);
			memcpy(copyToOffset + uv_linesize, u, uv_linesize);
		}
	}
}

static int QuerySystemCpuThreads()
{
	SYSTEM_INFO si = {};
	GetNativeSystemInfo(&si);
	return (int)si.dwNumberOfProcessors;
}

IMFMediaType* FFmpegVideoDecoder::Open(AVCodecID codecid, IMFMediaType* pMediaType)
{
	_decoder.codec = avcodec_find_decoder(codecid);
	if (_decoder.codec == NULL)
		return NULL;
	_decoder.context = avcodec_alloc_context3(_decoder.codec);
	if (_decoder.context == NULL)
		return NULL;
	_decoder.frame = av_frame_alloc();
	if (_decoder.frame == NULL)
		return NULL;

	//Get info from MediaFoundation type.
	UINT32 width = 0, height = 0;
	MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height);
	UINT32 num = 0, den = 0;
	MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, &num, &den);
	_default_duration = 0;
	if (num > 0 && den > 0) {
		double fps = (double)num / (double)den;
		_default_duration = (LONG64)(1.0 / fps * 10000000.0);
	}
	GUID subType = GUID_NULL;
	pMediaType->GetGUID(MF_MT_SUBTYPE, &subType);

	char codec_tag[8] = {0};
	*(unsigned*)&codec_tag = subType.Data1;
	strlwr(codec_tag);
	_decoder.context->codec_id = codecid;
	_decoder.context->codec_tag = *(unsigned*)&codec_tag;
	_decoder.context->width = _decoder.context->coded_width = (int)width;
	_decoder.context->height = _decoder.context->coded_height = (int)height;
	_decoder.context->err_recognition = 0;
	_decoder.context->workaround_bugs = FF_BUG_AUTODETECT;
	_decoder.context->refcounted_frames = 1;
	_decoder.context->pkt_timebase.num = 10000000;
	_decoder.context->pkt_timebase.den = 1;
	
	_decoder.context->thread_count = QuerySystemCpuThreads() + 1;
	if (_decoder.context->thread_count > 16)
		_decoder.context->thread_count = 16;

	_decoder.context->bit_rate = (int)MFGetAttributeUINT32(pMediaType, MF_MT_AVG_BITRATE, 0);
	if (MFGetAttributeUINT32(pMediaType, MF_MT_MPEG2_PROFILE, 0) > 0)
		_decoder.context->profile = (int)MFGetAttributeUINT32(pMediaType, MF_MT_MPEG2_PROFILE, 0);
	if (MFGetAttributeUINT32(pMediaType, MF_MT_MPEG2_LEVEL, 0) > 0)
		_decoder.context->level = (int)MFGetAttributeUINT32(pMediaType, MF_MT_MPEG2_LEVEL, 0);

	unsigned char* codecprivate = NULL;
	int codecprivate_len = 0;

	UINT32 extraLen = 0, userdataLen = 0;
	pMediaType->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, &extraLen);
	pMediaType->GetBlobSize(MF_MT_USER_DATA, &userdataLen);
	if ((extraLen > 0 || userdataLen > 0) &&
		(subType != MFVideoFormat_H264 &&
		subType != MFVideoFormat_H264_ES &&
		subType != MFVideoFormat_HEVC &&
		subType != MFVideoFormat_HEVC_ES)) {
		codecprivate_len = extraLen > 0 ? extraLen : userdataLen;
		codecprivate = (unsigned char*)av_mallocz(codecprivate_len + FF_INPUT_BUFFER_PADDING_SIZE);
		if (codecprivate) {
			if (extraLen > 0)
				pMediaType->GetBlob(MF_MT_MPEG_SEQUENCE_HEADER, codecprivate, codecprivate_len, &extraLen);
			else if (userdataLen > 0)
				pMediaType->GetBlob(MF_MT_USER_DATA, codecprivate, codecprivate_len, &userdataLen);
		}
	}else if (extraLen > 0 && codecprivate == NULL) {
		PBYTE buf = NULL;
		unsigned size = 0;
		pMediaType->GetAllocatedBlob(MF_MT_MPEG_SEQUENCE_HEADER, &buf, &size);
		if (buf && size > 4) {
			if (buf[0] + buf[1] == 0) {
				AVPacket pkt;
				av_init_packet(&pkt);
				pkt.data = buf;
				pkt.size = size;
				if (avcodec_open2(_decoder.context, _decoder.codec, NULL) >= 0)
					avcodec_decode_video2(_decoder.context, _decoder.frame, (int*)&size, &pkt);
			}
		}
		if (buf)
			CoTaskMemFree(buf);
	}

	if (codecprivate) {
		_decoder.context->extradata = codecprivate;
		_decoder.context->extradata_size = codecprivate_len;
	}

	if (avcodec_is_open(_decoder.context) == 0) {
		int result = avcodec_open2(_decoder.context, _decoder.codec, NULL);
		if (result < 0) {
			Close();
			return NULL;
		}
	}

	if (FAILED(MFCreateMediaType(_rawMediaType.ReleaseAndGetAddressOf()))) {
		Close();
		return NULL;
	}
	pMediaType->CopyAllItems(_rawMediaType.Get());

	_image_size = _decoder.context->width * _decoder.context->height * 3 / 2;
	return CreateResultMediaType(MFVideoFormat_NV12);
}

void FFmpegVideoDecoder::Close()
{
	if (_decoder.context) {
		avcodec_close(_decoder.context);
		av_freep(&_decoder.context->extradata);
		av_freep(&_decoder.context);
		sws_freeContext(_decoder.scaler);
	}
	av_frame_free(&_decoder.frame);
	memset(&_decoder, 0, sizeof(_decoder));
	_rawMediaType.Reset();
}

HRESULT FFmpegVideoDecoder::Decode(IMFSample* pSample, IMFSample** ppDecodedSample)
{
	HRESULT hr;
	LONG64 duration = 0;
	if (pSample == NULL) {
		hr = Process(NULL, 0, AV_NOPTS_VALUE, 0, false, false);
	}else{
		ComPtr<IMFMediaBuffer> pBuffer;
		hr = pSample->ConvertToContiguousBuffer(&pBuffer);
		if (SUCCEEDED(hr)) {
			PBYTE buf = NULL;
			DWORD size = 0;
			hr = pBuffer->Lock(&buf, NULL, &size);
			if (size == 0) {
				if (SUCCEEDED(hr))
					pBuffer->Unlock();
				hr = MF_E_TRANSFORM_NEED_MORE_INPUT;
			}
			if (SUCCEEDED(hr)) {
				LONG64 pts = 0;
				hr = pSample->GetSampleTime(&pts);
				if (FAILED(hr))
					hr = pSample->GetUINT64(MFSampleExtension_DecodeTimestamp, (PUINT64)&pts);
				pSample->GetSampleDuration(&duration);
				bool keyframe = MFGetAttributeUINT32(pSample, MFSampleExtension_CleanPoint, 0) ? true:false;
				bool discontinuity = MFGetAttributeUINT32(pSample, MFSampleExtension_Discontinuity, 0) ? true:false;

				hr = Process(buf, size,
					SUCCEEDED(hr) ? pts : AV_NOPTS_VALUE, duration > 0 ? duration : 0,
					keyframe, discontinuity);

				pBuffer->Unlock();
			}
		}
	}
	if (FAILED(hr))
		return hr;

	LONG64 pts;

	ComPtr<IMFSample> pDecodedSample;
	hr = CreateDecodedSample(_decoder.frame, &pDecodedSample);
	if (SUCCEEDED(hr))
		pts = av_frame_get_best_effort_timestamp(_decoder.frame);
	av_frame_unref(_decoder.frame);
	if (FAILED(hr))
		return hr;

	pDecodedSample->SetSampleTime(pts);
	pDecodedSample->SetSampleDuration(duration > 0 ? duration : _default_duration);

	*ppDecodedSample = pDecodedSample.Detach();
	return S_OK;
}

void FFmpegVideoDecoder::Flush()
{
	if (_decoder.context && avcodec_is_open(_decoder.context)) {
		avcodec_flush_buffers(_decoder.context);
	
		GUID subType = GUID_NULL;
		_rawMediaType->GetGUID(MF_MT_SUBTYPE, &subType);
		if (subType == MFVideoFormat_H264 ||
			subType == MFVideoFormat_AVC1 ||
			subType == MFVideoFormat_HEVC ||
			subType == MFVideoFormat_HVC1 ||
			subType == MFVideoFormat_MPEG2) {
			ComPtr<IMFMediaType> pMediaType;
			MFCreateMediaType(&pMediaType);
			_rawMediaType->CopyAllItems(pMediaType.Get());

			auto codecid = _decoder.context->codec_id;
			Close();
			ComPtr<IMFMediaType> result;
			result.Attach(Open(codecid, pMediaType.Get()));
		}
	}
}

HRESULT FFmpegVideoDecoder::Process(const BYTE* buf, unsigned size, LONG64 pts, LONG64 duration, bool keyframe, bool discontinuity)
{
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = (uint8_t*)buf;
	pkt.size = size;
	pkt.pts = pts;
	pkt.duration = (int)duration;
	if (keyframe)
		pkt.flags = AV_PKT_FLAG_KEY;

	int got_picture = 0;
	int bytes = avcodec_decode_video2(_decoder.context, _decoder.frame, &got_picture, &pkt);
	if (bytes < 0) {
		av_frame_unref(_decoder.frame);
		return E_FAIL;
	}
	if (got_picture == 0) {
		av_frame_unref(_decoder.frame);
		return MF_E_TRANSFORM_NEED_MORE_INPUT;
	}

	if (!_decoder.once_state) {
		_decoder.once_state = true;
		if (!OnceDecodeCallback()) //update codec info
			return E_ABORT;
	}
	return S_OK;
}

HRESULT FFmpegVideoDecoder::CreateDecodedSample(AVFrame* frame, IMFSample** ppSample)
{
	if (frame->width != _decoder.context->width ||
		frame->height != _decoder.context->height)
		return MF_E_TRANSFORM_STREAM_CHANGE;

	HRESULT hr = S_OK;
	ComPtr<IMFSample> pSample;
	ComPtr<IMFMediaBuffer> pBuffer;
	if (_allocator) {
		hr = _allocator->CreateSample(&pSample);
		if (SUCCEEDED(hr))
			hr = pSample->GetBufferByIndex(0, &pBuffer);
	}else{
		hr = MFCreateSample(&pSample);
		if (SUCCEEDED(hr)) {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
			hr = MFCreateMemoryBuffer(_image_size, &pBuffer);
#else
			hr = MFCreate2DMediaBuffer(frame->width, frame->height, FCC('NV12'), FALSE, &pBuffer);
#endif
			if (SUCCEEDED(hr))
				pSample->AddBuffer(pBuffer.Get());
		}
	}
	if (FAILED(hr))
		return hr;

	PBYTE buf = NULL;
	hr = pBuffer->Lock(&buf, NULL, NULL);
	if (FAILED(hr))
		return hr;

	if (_decoder.scaler == NULL) {
		if (frame->linesize[0] == frame->width)
			ConvertYUV420PToNV12Packed_NonLineSize(buf,
			frame->data[0], frame->data[1], frame->data[2],
			frame->width, frame->height);
		else
			ConvertYUV420PToNV12Packed_UseLineSize(buf,
			frame->data[0], frame->data[1], frame->data[2],
			frame->width, frame->height,
			frame->linesize[0], frame->linesize[1]);
	}else{
		int yoffset = frame->width * frame->height;
		unsigned char* image_buf[4] = {buf, buf + yoffset, NULL, NULL};
		int image_linesize[4] = {frame->width, frame->width, 0, 0};
		if (sws_scale(_decoder.scaler, frame->data, frame->linesize, 0, frame->height, image_buf, image_linesize) < 0)
			hr = MF_E_UNEXPECTED;
	}

	pBuffer->Unlock();
	pBuffer->SetCurrentLength(_image_size);

	*ppSample = pSample.Detach();
	return hr;
}

IMFMediaType* FFmpegVideoDecoder::CreateResultMediaType(REFGUID outputFormat)
{
	ComPtr<IMFMediaType> pMediaType;
	if (FAILED(MFCreateMediaType(&pMediaType)))
		return NULL;

	pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	pMediaType->SetGUID(MF_MT_SUBTYPE, outputFormat);

	pMediaType->SetUINT32(MF_MT_COMPRESSED, FALSE);
	pMediaType->SetUINT32(MF_MT_AVG_BIT_ERROR_RATE, 0);
	pMediaType->SetUINT32(MF_MT_DEFAULT_STRIDE, _decoder.context->width);
	pMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_MixedInterlaceOrProgressive);

	pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
	pMediaType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, TRUE);
	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE, _image_size);

	MFSetAttributeSize(pMediaType.Get(), MF_MT_FRAME_SIZE, _decoder.context->width, _decoder.context->height);
	UINT32 num = 0, den = 0;
	if (SUCCEEDED(MFGetAttributeRatio(_rawMediaType.Get(), MF_MT_FRAME_RATE, &num, &den)))
		MFSetAttributeRatio(pMediaType.Get(), MF_MT_FRAME_RATE, num, den);
	if (SUCCEEDED(MFGetAttributeRatio(_rawMediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, &num, &den)))
		MFSetAttributeRatio(pMediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, num, den);
	else
		MFSetAttributeRatio(pMediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

	return pMediaType.Detach();
}

bool FFmpegVideoDecoder::OnceDecodeCallback()
{
	auto ctx = _decoder.context;
	if (ctx->framerate.num > 0 && ctx->framerate.den > 0) {
		double fps = (double)ctx->framerate.num / (double)ctx->framerate.den;
		if (fps > 0 && fps < 240)
			_default_duration = (LONG64)(1.0 / fps * 10000000.0);
	}
	if (_default_duration == 0) {
		UINT32 num = 0, den = 0;
		MFGetAttributeRatio(_rawMediaType.Get(), MF_MT_CORE_DEMUX_FRAMERATE, &num, &den);
		if (num > 0 && den > 0) {
			double fps = (double)num / (double)den;
			_default_duration = (LONG64)(1.0 / fps * 10000000.0);
		}
	}
	
	//10bit,422 or 444.
	if (ctx->pix_fmt != AV_PIX_FMT_YUV420P) {
		_decoder.scaler = sws_getContext(ctx->width, ctx->height, ctx->pix_fmt,
			ctx->width, ctx->height, AV_PIX_FMT_NV12, SWS_BICUBIC, NULL, NULL, NULL);
		if (_decoder.scaler == NULL)
			return false;
	}
	return true;
}