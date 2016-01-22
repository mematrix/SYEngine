#pragma once

#include "TransformServices.h"
#include <mutex>
#include <memory>
#include <wrl.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/opt.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
}

using namespace Microsoft::WRL;

class FFmpegAudioDecoder;
class FFmpegVideoDecoder;

class FFmpegDecodeServices : public ITransformLoader, public ITransformWorker
{
public:
	FFmpegDecodeServices() : _ref_count(1), _audio_decoder(NULL), _video_decoder(NULL) {}
	virtual ~FFmpegDecodeServices() { DestroyDecoders(); }

public:
	STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_ref_count); }
	STDMETHODIMP_(ULONG) Release()
	{ ULONG rc = InterlockedDecrement(&_ref_count); if (rc == 0) delete this; return rc; }
	STDMETHODIMP QueryInterface(REFIID iid,void** ppv);

	STDMETHODIMP CheckMediaType(IMFMediaType* pMediaType);
	STDMETHODIMP SetInputMediaType(IMFMediaType* pMediaType);
	STDMETHODIMP GetOutputMediaType(IMFMediaType** ppMediaType);
	STDMETHODIMP GetCurrentInputMediaType(IMFMediaType** ppMediaType);
	STDMETHODIMP GetCurrentOutputMediaType(IMFMediaType** ppMediaType);

	STDMETHODIMP SetAllocator(ITransformAllocator* pAllocator);
	STDMETHODIMP ProcessSample(IMFSample* pSample, IMFSample** ppNewSample);
	STDMETHODIMP ProcessFlush();

private:
	AVCodecID ConvertGuidToCodecId(IMFMediaType* pMediaType);
	bool VerifyMediaType(IMFMediaType* pMediaType);
	bool VerifyAudioMediaType(IMFMediaType* pMediaType);
	bool VerifyVideoMediaType(IMFMediaType* pMediaType);

	HRESULT InitAudioDecoder(IMFMediaType* pMediaType) { return E_NOTIMPL; }  //not impl.
	HRESULT InitVideoDecoder(IMFMediaType* pMediaType);

	void DestroyDecoders();

private:
	ULONG _ref_count;

	ComPtr<IMFMediaType> _inputMediaType, _outputMediaType;
	ComPtr<ITransformAllocator> _sampleAllocator;

	FFmpegAudioDecoder* _audio_decoder;
	FFmpegVideoDecoder* _video_decoder;

	std::recursive_mutex _mutex;
};