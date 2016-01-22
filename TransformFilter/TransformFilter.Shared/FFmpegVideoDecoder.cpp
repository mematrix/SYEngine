#include "FFmpegVideoDecoder.h"
#include "more_codec_uuid.h"

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
	_decoder.context->thread_count = QuerySystemCpuThreads() * 3 / 2;
	if (_decoder.context->thread_count > 16)
		_decoder.context->thread_count = 16;

	if (MFGetAttributeUINT32(pMediaType, MF_MT_MPEG2_PROFILE, 0) > 0)
		_decoder.context->profile = (int)MFGetAttributeUINT32(pMediaType, MF_MT_MPEG2_PROFILE, 0);
	if (MFGetAttributeUINT32(pMediaType, MF_MT_MPEG2_LEVEL, 0) > 0)
		_decoder.context->level = (int)MFGetAttributeUINT32(pMediaType, MF_MT_MPEG2_LEVEL, 0);

	unsigned char* codecprivate = NULL;
	int codecprivate_len = 0;

	UINT32 extraLen = 0, userdataLen = 0;
	pMediaType->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, &extraLen);
	pMediaType->GetBlobSize(MF_MT_USER_DATA, &userdataLen);
	if (extraLen > 0 || userdataLen > 0) {
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
	}

	int result = avcodec_open2(_decoder.context, _decoder.codec, NULL);
	if (result < 0) {
		Close();
		return NULL;
	}

	if (FAILED(MFCreateMediaType(_rawMediaType.ReleaseAndGetAddressOf()))) {
		Close();
		return NULL;
	}
	pMediaType->CopyAllItems(_rawMediaType.Get());
	return CreateResultMediaType(MFVideoFormat_NV12);
}

void FFmpegVideoDecoder::Close()
{
	if (_decoder.context) {
		avcodec_close(_decoder.context);
		av_freep(&_decoder.context->extradata);
		av_freep(&_decoder.context);
	}
	av_frame_free(&_decoder.frame);
	memset(&_decoder, 0, sizeof(_decoder));
	_rawMediaType.Reset();
}

HRESULT FFmpegVideoDecoder::Decode(IMFSample* pSample, IMFSample** ppDecodedSample)
{
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
			auto codecid = _decoder.context->codec_id;
			Close();
			ComPtr<IMFMediaType> pMediaType;
			MFCreateMediaType(&pMediaType);
			_rawMediaType->CopyAllItems(pMediaType.Get());
			ComPtr<IMFMediaType> result;
			result.Attach(Open(codecid, pMediaType.Get()));
		}
	}
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
	pMediaType->SetUINT32(MF_MT_SAMPLE_SIZE, _decoder.context->width * _decoder.context->height * 3 / 2);

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