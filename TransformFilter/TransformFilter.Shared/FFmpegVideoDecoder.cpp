#include "FFmpegVideoDecoder.h"
#include "more_codec_uuid.h"
#include "AVCParser.h"
#ifndef _ARM_
#include "memcpy_sse.h"
#endif

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

static void ConvertYUV420PToNV12Packed_NonLineSize(unsigned char* copyTo, unsigned char* y , unsigned char* u, unsigned char* v, unsigned width, unsigned height, unsigned ysize = 0)
{
	unsigned ysz = ysize > 0 ? ysize : width * height;
#ifndef _ARM_
	memcpy_sse(copyTo, y, ysz);
#else
	MFCopyImage(copyTo, width, y, width, width, height); //memcpy(copyTo, y, ysz);
#endif
	UVCopyInterlace(copyTo + ysz, u, v, ysz >> 1);
}

static void ConvertYUV420PToNV12Packed_UseLineSize(unsigned char* copyTo, unsigned char* y , unsigned char* u, unsigned char* v, unsigned width, unsigned height, unsigned linesize_y, unsigned linesize_uv, unsigned ysize = 0)
{
	unsigned ysz = ysize > 0 ? ysize : width * height;
	unsigned char* copyToOffset = copyTo;
	//copy Y
	if (width == linesize_y) {
		MFCopyImage(copyTo, width, y, width, width, height);
		copyToOffset += ysz;
	}else{
		unsigned char* lumaYoffset = y;
		for (unsigned i = 0; i < height; i++, copyToOffset += width, lumaYoffset += linesize_y)
			memcpy(copyToOffset, lumaYoffset, width);
	}
	//copy UV
	unsigned uv_interlace_linesize = ysz / height;
	if ((uv_interlace_linesize >> 1) == linesize_uv) {
		UVCopyInterlace(copyToOffset, u, v, ysz >> 1);
	}else{
		unsigned char* chromaUoffset = u;
		unsigned char* chromaVoffset = v;
		unsigned count = height >> 1;
		for (unsigned i = 0; i < count; i++, copyToOffset += uv_interlace_linesize, chromaUoffset += linesize_uv, chromaVoffset += linesize_uv)
			UVCopyInterlace(copyToOffset, chromaUoffset, chromaVoffset, uv_interlace_linesize);
	}
}

static void ConvertYUV420PToYV12Packed_NonLineSize(unsigned char* copyTo, unsigned char* y , unsigned char* u, unsigned char* v, unsigned width, unsigned height, unsigned ysize = 0)
{
	unsigned ysz = ysize > 0 ? ysize : width * height;
	unsigned uvsize = ysz >> 2;
#ifndef _ARM_
	memcpy_sse(copyTo, y, ysz);
#else
	MFCopyImage(copyTo, width, y, width, width, height); //memcpy(copyTo, y, ysz);
#endif
	memcpy(copyTo + ysz, v, uvsize);
	memcpy(copyTo + ysz + uvsize, u, uvsize);
}

static void ConvertYUV420PToYV12Packed_UseLineSize(unsigned char* copyTo, unsigned char* y , unsigned char* u, unsigned char* v, unsigned width, unsigned height, unsigned linesize_y, unsigned linesize_uv, unsigned ysize = 0)
{
	unsigned ysz = ysize > 0 ? ysize : width * height;
	unsigned uvsize = ysz >> 2;
	unsigned char* copyToOffset = copyTo;
	//copy Y
	if (width == linesize_y) {
		MFCopyImage(copyTo, width, y, width, width, height);
		copyToOffset += ysz;
	}else{
		unsigned char* lumaYoffset = y;
		for (unsigned i = 0; i < height; i++, copyToOffset += width, lumaYoffset += linesize_y)
			memcpy(copyToOffset, lumaYoffset, width);
	}
	//copy UV
	unsigned uv_interlace_linesize = ysz / height;
	unsigned uv_linesize = uv_interlace_linesize >> 1;
	if (uv_linesize == linesize_uv) {
		memcpy(copyToOffset, v, uvsize);
		memcpy(copyToOffset + uvsize, u, uvsize);
	}else{
		unsigned count = height >> 1;
		for (unsigned i = 0; i < count; i++, copyToOffset += uv_interlace_linesize) {
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

class YUVCopyTask : public IUnknown
{
	ULONG _ref_count;
public:
	YUVCopyTask() : _ref_count(1) {}
	virtual ~YUVCopyTask() {}

	STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_ref_count); }
	STDMETHODIMP_(ULONG) Release()
	{ ULONG rc = InterlockedDecrement(&_ref_count); if (rc == 0) delete this; return rc; }
	STDMETHODIMP QueryInterface(REFIID iid,void** ppv)
	{ if (iid != IID_IUnknown) return E_NOINTERFACE; *ppv = this; AddRef(); return S_OK; }

public:
	struct TaskContext
	{
		HANDLE hevent;
		PBYTE copyTo;
		unsigned char *y, *u, *v;
		unsigned y_linesize, uv_linesize;
		unsigned yoffset, width, height;
	};
	enum TaskType
	{
		CopyY,
		CopyUV,
		CopyU,
		CopyV
	};

	inline void SetTaskContext(const TaskContext& ctx) { _ctx = ctx; }
	inline void SetTaskType(TaskType type) { _type = type; } 
	inline TaskContext* GetTaskContext() throw() { return &_ctx; }
	inline TaskType GetTaskType() const throw() { return _type; }

private:
	TaskContext _ctx;
	TaskType _type;
};

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

	_timestamp = _default_duration = 0;
	_fixed_framerate = false;
	_force_progressive = false;

	//Get info from MediaFoundation type.
	UINT32 width = 0, height = 0;
	MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height);
	UINT32 num = 0, den = 0;
	MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, &num, &den);
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

	AVRational mftb = {10000000, 1};
	av_codec_set_pkt_timebase(_decoder.context, mftb);
	
	_decoder.context->thread_count = QuerySystemCpuThreads() + 1;
	if (_decoder.context->thread_count > 16)
		_decoder.context->thread_count = 16;
	if (subType == MFVideoFormat_MPG1)
		_decoder.context->thread_count = 1;

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
	}

	if (codecprivate) {
		_decoder.context->extradata = codecprivate;
		_decoder.context->extradata_size = codecprivate_len;
		if (codecid == AV_CODEC_ID_H264 && subType == MFVideoFormat_AVC1)
			CheckOrUpdateFixedFrameRateAVC(codecprivate, codecprivate_len);
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

	if (FAILED(InitNV12MTCopy()))
		return NULL;

	if (subType == MFVideoFormat_MPG1 || subType == MFVideoFormat_H263 ||
		subType == MFVideoFormat_VP6 || subType == MFVideoFormat_VP6F ||
		subType == MFVideoFormat_VP8 || subType == MFVideoFormat_VP9)
		_force_progressive = true;
	if (subType == MFVideoFormat_MPG1 || subType == MFVideoFormat_MPG2)
		_fixed_framerate = true;

	_image_size = _decoder.context->width * _decoder.context->height * 3 / 2;
	return CreateResultMediaType(MFVideoFormat_NV12);
}

void FFmpegVideoDecoder::Close()
{
	CloseNV12MTCopy();

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
		if (_decoder.flush_after)
			return MF_E_UNEXPECTED;
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
				if (FAILED(hr) && _decoder.flush_after)
					return MF_E_INVALID_TIMESTAMP;
				pSample->GetSampleDuration(&duration);

				bool keyframe = MFGetAttributeUINT32(pSample, MFSampleExtension_CleanPoint, 0) ? true:false;
				bool discontinuity = MFGetAttributeUINT32(pSample, MFSampleExtension_Discontinuity, 0) ? true:false;

				if (_decoder.flush_after) {
					_decoder.flush_after = false;
					_timestamp = pts;
				}

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
	if (SUCCEEDED(hr)) {
		if (_fixed_framerate) {
			pts = _timestamp;
			_timestamp += _default_duration;
		}else{
			pts = av_frame_get_best_effort_timestamp(_decoder.frame);
		}
	}
	av_frame_unref(_decoder.frame);
	if (FAILED(hr))
		return hr;

	pDecodedSample->SetSampleTime(pts);
	if (_fixed_framerate)
		pDecodedSample->SetSampleDuration(_default_duration);
	else
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
			subType == MFVideoFormat_H264_ES ||
			subType == MFVideoFormat_HEVC_ES ||
			subType == MFVideoFormat_MPEG2) {
			ComPtr<IMFMediaType> pMediaType;
			MFCreateMediaType(&pMediaType);
			_rawMediaType->CopyAllItems(pMediaType.Get());

			auto codecid = _decoder.context->codec_id;
			Close();
			ComPtr<IMFMediaType> result;
			result.Attach(Open(codecid, pMediaType.Get()));
		}
		_decoder.flush_after = true;
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
		if (frame->linesize[0] == frame->width) {
			if (frame->width < 1280) {
				ConvertYUV420PToNV12Packed_NonLineSize(buf,
					frame->data[0], frame->data[1], frame->data[2],
					frame->width, frame->height, _image_luma_size);
			}else{
				NV12MTCopy(frame, buf); //multi-thread copy YUV.
			}
		}else{
			ConvertYUV420PToNV12Packed_UseLineSize(buf,
				frame->data[0], frame->data[1], frame->data[2],
				frame->width, frame->height,
				frame->linesize[0], frame->linesize[1], _image_luma_size);
		}
	}else{
		int yoffset = frame->width * frame->height;
		unsigned char* image_buf[4] = {buf, buf + yoffset, NULL, NULL};
		int image_linesize[4] = {frame->width, frame->width, 0, 0};
		if (sws_scale(_decoder.scaler, frame->data, frame->linesize, 0, frame->height, image_buf, image_linesize) < 0)
			hr = MF_E_UNEXPECTED;
	}

	pBuffer->Unlock();
	pBuffer->SetCurrentLength(_image_size);

	if (!_force_progressive) {
		if (frame->interlaced_frame) {
			pSample->SetUINT32(MFSampleExtension_Interlaced, TRUE);
			pSample->SetUINT32(MFSampleExtension_RepeatFirstField, frame->repeat_pict ? TRUE:FALSE);
			pSample->SetUINT32(MFSampleExtension_BottomFieldFirst, frame->top_field_first ? FALSE:TRUE);
		}else{
			pSample->SetUINT32(MFSampleExtension_Interlaced, FALSE);
		}
	}

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
	if (_force_progressive)
		pMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

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

void FFmpegVideoDecoder::CheckOrUpdateFixedFrameRateAVC(const unsigned char* avcc, unsigned avcc_size)
{
	AVCParser parser = {};
	parser.Parse(avcc, avcc_size);
	if (parser.fixed_fps_flag && parser.progressive_flag &&
		parser.fps_timescale > 0 && parser.fps_tick > 0) {
		double fps = (double)parser.fps_timescale / (double)parser.fps_tick;
		if (fps > 1.0 && fps < 240.0) {
			_default_duration = (LONG64)(1.0 / fps * 10000000.0);
			_fixed_framerate = true;
		}
	}
	if (parser.progressive_flag)
		_force_progressive = true;
}

bool FFmpegVideoDecoder::OnceDecodeCallback()
{
	auto ctx = _decoder.context;
	if (ctx->framerate.num > 0 && ctx->framerate.den > 0) {
		double fps = (double)ctx->framerate.num / (double)ctx->framerate.den;
		if (fps > 0 && fps < 240 && _default_duration == 0)
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
			ctx->width, ctx->height, AV_PIX_FMT_NV12, SWS_BILINEAR, NULL, NULL, NULL);
		if (_decoder.scaler == NULL)
			return false;
	}else{
		_image_luma_size = ctx->width * ctx->height; //ysize
	}

	if (ctx->field_order == AVFieldOrder::AV_FIELD_PROGRESSIVE)
		_force_progressive = true;
	return true;
}

HRESULT FFmpegVideoDecoder::InitNV12MTCopy()
{
	RtlZeroMemory(&_yuv420_mtcopy, sizeof(_yuv420_mtcopy));
	HRESULT hr = MFLockPlatform();
	if (FAILED(hr))
		return hr;

	for (int i = 0; i < _countof(_yuv420_mtcopy.Worker); i++) {
		hr = MFAllocateSerialWorkQueue(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, &_yuv420_mtcopy.Worker[i]);
		if (FAILED(hr))
			return hr;
	}
	for (int i = 0; i < _countof(_yuv420_mtcopy.Events); i++) {
		_yuv420_mtcopy.Events[i] = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
		if (_yuv420_mtcopy.Events[i] == NULL)
			return E_ABORT;
	}
	for (int i = 0; i < _countof(_yuv420_mtcopy.Tasks); i++) {
		_yuv420_mtcopy.Tasks[i] = new (std::nothrow) YUVCopyTask();
		if (_yuv420_mtcopy.Tasks[i] == NULL)
			return E_OUTOFMEMORY;
	}

	return S_OK;
}

void FFmpegVideoDecoder::CloseNV12MTCopy()
{
	for (int i = 0; i < _countof(_yuv420_mtcopy.Events); i++)
		if (_yuv420_mtcopy.Events[i])
			CloseHandle(_yuv420_mtcopy.Events[i]);
	for (int i = 0; i < _countof(_yuv420_mtcopy.Worker); i++)
		if (_yuv420_mtcopy.Worker[i] > 0)
			MFUnlockWorkQueue(_yuv420_mtcopy.Worker[i]);
	for (int i = 0; i < _countof(_yuv420_mtcopy.Tasks); i++)
		if (_yuv420_mtcopy.Tasks[i])
			_yuv420_mtcopy.Tasks[i]->Release();

	MFUnlockPlatform();
	RtlZeroMemory(&_yuv420_mtcopy, sizeof(_yuv420_mtcopy));
}

void FFmpegVideoDecoder::NV12MTCopy(AVFrame* frame, BYTE* copyTo)
{
	auto ytask = static_cast<YUVCopyTask*>(_yuv420_mtcopy.Tasks[0]);
	auto uvtask = static_cast<YUVCopyTask*>(_yuv420_mtcopy.Tasks[1]);

	YUVCopyTask::TaskContext ctx;
	ctx.copyTo = copyTo;
	ctx.width = frame->width;
	ctx.height = frame->height;
	ctx.y = frame->data[0];
	ctx.u = frame->data[1];
	ctx.v = frame->data[2];
	ctx.y_linesize = frame->linesize[0];
	ctx.uv_linesize = frame->linesize[1];
	ctx.yoffset = _image_luma_size;

	ctx.hevent = _yuv420_mtcopy.Events[0];
	ytask->SetTaskContext(ctx);
	ytask->SetTaskType(YUVCopyTask::TaskType::CopyY);
	MFPutWorkItem2(_yuv420_mtcopy.Worker[0], 0, this, ytask);
	ctx.hevent = _yuv420_mtcopy.Events[1];
	uvtask->SetTaskContext(ctx);
	uvtask->SetTaskType(YUVCopyTask::TaskType::CopyUV);
	MFPutWorkItem2(_yuv420_mtcopy.Worker[1], 0, this, uvtask);

	WaitForMultipleObjectsEx(2, _yuv420_mtcopy.Events, TRUE, INFINITE, FALSE);
}

HRESULT FFmpegVideoDecoder::Invoke(IMFAsyncResult *pAsyncResult)
{
	auto task = static_cast<YUVCopyTask*>(pAsyncResult->GetStateNoAddRef());
	auto ctx = task->GetTaskContext();
	switch (task->GetTaskType())
	{
	case YUVCopyTask::TaskType::CopyY:
#ifndef _ARM_
		memcpy_sse(ctx->copyTo, ctx->y, ctx->yoffset);
#else
		MFCopyImage(ctx->copyTo, ctx->width, ctx->y, ctx->width, ctx->width, ctx->height);
#endif
		break;
	case YUVCopyTask::TaskType::CopyUV:
		UVCopyInterlace(ctx->copyTo + ctx->yoffset, ctx->u, ctx->v, ctx->yoffset >> 1);
		break;
	}
	SetEvent(ctx->hevent);
	return S_OK;
}