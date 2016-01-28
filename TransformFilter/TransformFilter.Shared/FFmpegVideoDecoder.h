#pragma once

#include "FFmpegDecodeServices.h"

class FFmpegVideoDecoder : public IMFAsyncCallback
{
	ULONG _ref_count;
public:
	FFmpegVideoDecoder() : _ref_count(1) { memset(&_decoder, 0, sizeof(_decoder)); }
	virtual ~FFmpegVideoDecoder() { Close(); }

public:
	STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_ref_count); }
	STDMETHODIMP_(ULONG) Release()
	{ ULONG rc = InterlockedDecrement(&_ref_count); if (rc == 0) delete this; return rc; }
	STDMETHODIMP QueryInterface(REFIID iid,void** ppv)
	{ if (ppv == NULL) return E_POINTER;
	  if (iid != IID_IUnknown && iid != IID_IMFAsyncCallback) return E_NOINTERFACE;
	  *ppv = this; AddRef(); return S_OK; }

	STDMETHODIMP GetParameters(DWORD*, DWORD*) { return E_NOTIMPL; }
	STDMETHODIMP Invoke(IMFAsyncResult *pAsyncResult);

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

	void CheckOrUpdateFixedFrameRateAVC(const unsigned char* avcc, unsigned avcc_size);

	HRESULT InitNV12MTCopy();
	void CloseNV12MTCopy();
	void NV12MTCopy(AVFrame* frame, BYTE* copyTo);

	struct Decoder
	{
		AVCodec* codec;
		AVCodecContext* context;
		AVFrame* frame;
		SwsContext* scaler;
		bool once_state;
		bool flush_after;
	};
	Decoder _decoder;
	ComPtr<IMFMediaType> _rawMediaType;

	DWORD _image_size, _image_luma_size;
	LONG64 _timestamp;
	LONG64 _default_duration;
	bool _fixed_framerate;

	ComPtr<ITransformAllocator> _allocator;

	struct YUVCopyThreads
	{
		DWORD Worker[3];
		HANDLE Events[3];
		IUnknown* Tasks[3];
	};
	YUVCopyThreads _yuv420_mtcopy;
};