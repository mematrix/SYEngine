#include "HDMediaStream.h"
#ifdef _USE_DECODE_FILTER
#include <d3d11.h>
#endif

ComPtr<HDMediaStream> HDMediaStream::CreateMediaStream(int index,HDMediaSource* pMediaSource,IMFStreamDescriptor* pStreamDesc)
{
	ComPtr<HDMediaStream> sp;
	
	auto p = new (std::nothrow)HDMediaStream(index,pMediaSource,pStreamDesc);
	if (p != nullptr)
	{
		p->AddRef();
		sp.Attach(p);
	}

	return sp;
}

HDMediaStream::HDMediaStream(int index,HDMediaSource* pMediaSource,IMFStreamDescriptor* pStreamDesc)
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
: _taskWorkQueue(this,&_taskInvokeCallback)
#endif
{
	pMediaSource->AddRef();
	pStreamDesc->AddRef();

	_pMediaSource.Attach(pMediaSource);
	_pStreamDesc.Attach(pStreamDesc);

	_dwQueueSize = 10;
	_preroll_time = _preroll_dynamic_time = 0;

	_index = index;

	_transform_filter = MFGetAttributeUINT32(pStreamDesc,MF_MT_MY_TRANSFORM_FILTER_ALLOW,0) ? true:false;
	_decode_processing = false;

	_state = STATE_STOPPED;

	_active = _eos = false;
	_discontinuity = FALSE;
	if (_transform_filter)
		_dec_eos = false;
	else
		_dec_eos = true;

	_private_size = 0;

	_taskInvokeCallback.SetCallback(this,&HDMediaStream::OnInvoke);

#ifdef _DEBUG
	_dbgRecordSampleTick = false;
#endif
}

HRESULT HDMediaStream::QueryInterface(REFIID iid,void** ppv)
{
	if (ppv == nullptr)
		return E_POINTER;

	*ppv = nullptr;
	if (iid == IID_IUnknown)
		*ppv = static_cast<IUnknown*>(this);
	else if (iid == IID_IMFMediaEventGenerator)
		*ppv = static_cast<IMFMediaEventGenerator*>(this);
	else if (iid == IID_IMFMediaStream)
		*ppv = static_cast<IMFMediaStream*>(this);
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}

HRESULT HDMediaStream::GetMediaSource(IMFMediaSource **ppMediaSource)
{
	if (ppMediaSource == nullptr)
		return E_POINTER;

	if (_pMediaSource == nullptr)
		return E_UNEXPECTED;

	SourceAutoLock lock(_pMediaSource.Get());

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	*ppMediaSource = _pMediaSource.Get();
	(*ppMediaSource)->AddRef();

	return S_OK;
}

HRESULT HDMediaStream::GetStreamDescriptor(IMFStreamDescriptor **ppStreamDescriptor)
{
	if (ppStreamDescriptor == nullptr)
		return E_POINTER;

	if (_pMediaSource == nullptr)
		return E_UNEXPECTED;

	SourceAutoLock lock(_pMediaSource.Get());

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	*ppStreamDescriptor = _pStreamDesc.Get();
	(*ppStreamDescriptor)->AddRef();

	return S_OK;
}

HRESULT HDMediaStream::RequestSample(IUnknown *pToken)
{
	SourceAutoLock lock(_pMediaSource.Get());

#ifdef _DEBUG
	if (_dbgRecordSampleTick)
	{
		auto prev_tick = _reqSampleTick;
		_reqSampleTick = GetTickCount64();
		int offset = (int)(_reqSampleTick - prev_tick);
		if (_reqSampleCount > 0)
			_reqSampleTickTotal += offset;
		_reqSampleCount++;
		DbgLogPrintf(L"Stream %d RequestSample Time GAP: %d ms, avg %d ms",
			_index,offset,_reqSampleTickTotal / _reqSampleCount);
	}
#endif

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		goto done;

	if (_state == STATE_STOPPED)
	{
		//hr = MF_E_INVALIDREQUEST;
		//goto done;
		//see http://msdn.microsoft.com/en-us/library/windows/desktop/ms696240%28v=vs.85%29.aspx.
		return MF_E_MEDIA_SOURCE_WRONGSTATE;
	}

	if (!_active)
	{
		hr = MF_E_INVALIDREQUEST;
		goto done;
	}

	if (_eos && _samples.IsEmpty() && _dec_eos)
	{
		hr = MF_E_END_OF_STREAM;
		goto done;
	}

#ifdef _USE_DECODE_FILTER
	//如果是要解码的情况下，直接转发调用
	if (_decoder) {
		hr = RequestSampleWithDecode(pToken);
		if (FAILED(hr))
			goto done;
		else
			return S_OK;
	}
#endif

	//如果MediaSource没有进入buffering模式。。。
	//只要有sample被缓存，有请求就提交
	if (!_pMediaSource->IsBuffering())
	{
		//如果缓冲池中有样本，直接提交
		if (!_samples.IsEmpty())
		{
			hr = SendSampleDirect(pToken);
			goto done;
		}

		//如果缓冲池中没有样本，保存本次请求，做分发处理（要求MediaSource做SubmitSample后才提交）
		hr = _requests.InsertBack(pToken);
		if (FAILED(hr))
			goto done;

		if (!_pMediaSource->IsNetworkMode())
			DispatchSamples(); //如果是本地文件，同步分发
		else
			hr = RequestSampleAsync(); //如果是网络流，异步处理
	}else{
		//如果在buffering，请求只会被插入到请求队列，不提交给pipeline
		hr = _requests.InsertBack(pToken);
		if (FAILED(hr))
			goto done;

		if (NeedsData())
			hr = _pMediaSource->QueueAsyncOperation(SourceOperation::OP_REQUEST_DATA);

		DbgLogPrintf(L"RequestSample skiped with Network Buffering...");
	}

done:
	if (FAILED(hr) && SUCCEEDED(CheckShutdown()))
		hr = _pMediaSource->ProcessOperationError(hr);

	return hr;
}

HRESULT HDMediaStream::Initialize()
{
	ComPtr<IMFMediaTypeHandler> pHandler;
	HRESULT hr = _pStreamDesc->GetMediaTypeHandler(pHandler.GetAddressOf());
	if (FAILED(hr))
		return hr;

	ComPtr<IMFMediaType> pMediaType;
	hr = pHandler->GetCurrentMediaType(pMediaType.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = MFCreateMediaType(_CopyMediaType.GetAddressOf());
	if (FAILED(hr))
		return hr;

	pMediaType->CopyAllItems(_CopyMediaType.Get());

	GUID type = GUID_NULL;
	hr = pMediaType->GetGUID(MF_MT_MAJOR_TYPE,&type);
	if (FAILED(hr))
		return hr;
	if (type == MFMediaType_Audio)
		_type = MediaStream_Audio;
	else if (type == MFMediaType_Video)
		_type = MediaStream_Video;
	else
		_type = MediaStream_Unknown;

	hr = pMediaType->GetGUID(MF_MT_SUBTYPE,&type);
	if (FAILED(hr))
		return hr;
	hr = pMediaType->GetGUID(MF_MT_MY_TRANSFORM_FILTER_RAWTYPE,&type); //fake subtype for decode

	if (type == MFVideoFormat_H264 || type == MFVideoFormat_AVC1 || type == MFVideoFormat_Intel_QS_H264)
		_h264 = true;
	else
		_h264 = false;
	if (type == MFVideoFormat_HEVC || type == MFVideoFormat_HVC1 ||type == MFVideoFormat_Intel_QS_HEVC)
		_hevc = true;
	else
		_hevc = false;

	_flac = false;
	if (type == MFAudioFormat_FLAC_FRAMED || type == MFAudioFormat_FLAC)
		_flac = true;

	_pcm = false;
	if (type == MFAudioFormat_PCM ||
		type == MFAudioFormat_Float) {
		_pcm = true;

		UINT32 nch = MFGetAttributeUINT32(pMediaType.Get(),MF_MT_AUDIO_NUM_CHANNELS,0);
		UINT32 bits = MFGetAttributeUINT32(pMediaType.Get(),MF_MT_AUDIO_BITS_PER_SAMPLE,0);
		UINT32 srate = MFGetAttributeUINT32(pMediaType.Get(),MF_MT_AUDIO_SAMPLES_PER_SECOND,0);
		UINT32 be_flag = MFGetAttributeUINT32(pMediaType.Get(),MF_MT_MY_PCM_BE_FLAG,0);

		_pcm_presec_bytes = _pcm_size = 0;
		if (bits > 0)
		{
			_pcm_presec_bytes = nch * (bits / 8) * srate;
			_pcm_size = bits;
		}
		if (type == MFAudioFormat_Float)
		{
			_pcm_format = PCM_FLOAT;
		}else{
			if (bits == 8)
				_pcm_format = PCM_8BIT;
			else if (be_flag != 0)
				_pcm_format = PCM_BE;
			else
				_pcm_format = PCM_LE;
		}
	}

	return MFCreateEventQueue(_pEventQueue.GetAddressOf());
}

void HDMediaStream::Activate(bool fActive)
{
	DbgLogPrintf(L"%s %d::Activate (%s).",L"HDMediaStream",_index,fActive ? L"Selected":L"Deselect");

	if (fActive == _active)
		return;

	_active = fActive;

	if (!fActive)
	{
		_samples.Clear();
		_requests.Clear();
	}

	_discontinuity = FALSE;
}

HRESULT HDMediaStream::Start(const PROPVARIANT& var,bool seek)
{
	DbgLogPrintf(L"%s %d::Start.",L"HDMediaStream",_index);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	hr = QueueEvent(seek ? MEStreamSeeked:MEStreamStarted,GUID_NULL,S_OK,&var);
	if (FAILED(hr))
		return hr;

	_state = STATE_STARTED;
	_eos = false;
	if (_transform_filter)
		_dec_eos = false;

	if (!_pMediaSource->IsNetworkMode())
#ifndef _USE_DECODE_FILTER
		DispatchSamples();
#else
		ProcessSampleRequest();
#endif

#ifdef _DEBUG
	_reqSampleTick = GetTickCount64();
	_reqSampleTickTotal = 0;
	_reqSampleTickAvg = 0;
	_reqSampleCount = 0;
#endif
	return S_OK;
}

HRESULT HDMediaStream::Pause()
{
	DbgLogPrintf(L"%s %d::Pause.",L"HDMediaStream",_index);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	hr = QueueEvent(MEStreamPaused,GUID_NULL,S_OK,nullptr);
	if (FAILED(hr))
		return hr;

	_discontinuity = FALSE;
	_state = STATE_PAUSED;

	return S_OK;
}

HRESULT HDMediaStream::Stop()
{
	DbgLogPrintf(L"%s %d::Stop.",L"HDMediaStream",_index);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	_samples.Clear();
	_requests.Clear();

	hr = QueueEvent(MEStreamStopped,GUID_NULL,S_OK,nullptr);
	if (FAILED(hr))
		return hr;

	_discontinuity = FALSE;
	_state = STATE_STOPPED;

	return S_OK;
}

HRESULT HDMediaStream::EndOfStream()
{
	DbgLogPrintf(L"%s %d::EndOfStream.",L"HDMediaStream",_index);

	_eos = true;
#ifndef _USE_DECODE_FILTER
	DispatchSamples();
#else
	ProcessSampleRequest();
#endif

	return S_OK;
}

HRESULT HDMediaStream::Shutdown()
{
	DbgLogPrintf(L"%s %d::Shutdown.",L"HDMediaStream",_index);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	_state = STATE_SHUTDOWN;

	if (_pEventQueue)
		hr = _pEventQueue->Shutdown();

	if (FAILED(hr))
		return hr;

	_samples.Clear();
	_requests.Clear();

	_pStreamDesc.Reset();
	_pEventQueue.Reset();

	return S_OK;
}

HRESULT HDMediaStream::SubmitSample(IMFSample* pSample)
{
	if (pSample == nullptr) {
#ifndef _USE_DECODE_FILTER
		DispatchSamples();
#else
		ProcessSampleRequest();
#endif
		return S_OK;
	}

	HRESULT hr = _samples.InsertBack(pSample);
	if (FAILED(hr))
		return hr;

	if (!_pMediaSource->IsBuffering())
#ifndef _USE_DECODE_FILTER
		DispatchSamples();
#else
		ProcessSampleRequest();
#endif
	return S_OK;
}

void HDMediaStream::ClearQueue()
{
	if (FAILED(CheckShutdown()))
		return;

	_samples.Clear();
	_requests.Clear();
}

bool HDMediaStream::IsQueueEmpty() const
{
	return _samples.IsEmpty();
}

bool HDMediaStream::NeedsData()
{
	if (!_active ||
		_eos)
		return false;

	if (_preroll_time == 0)
		return _samples.GetCount() < _dwQueueSize ? true:false;
	return NeedsDataUseNetworkTime();
}

bool HDMediaStream::NeedsDataUseNetworkTime()
{
	if (_samples.GetCount() <= 1)
		return true;

	ComPtr<IMFSample> pSampleStart,pSampleEnd;
	_samples.GetFront(&pSampleStart);
	_samples.GetBack(&pSampleEnd);

	LONG64 ptsStart = 0,ptsEnd = 0;
	pSampleStart->GetSampleTime(&ptsStart);
	pSampleEnd->GetSampleTime(&ptsEnd);
	if (ptsStart == 0 && ptsEnd == 0)
		return _samples.GetCount() < _dwQueueSize ? true:false;

	if ((ptsEnd - ptsStart) >= (_preroll_time + _preroll_dynamic_time))
		return false;
	return true;
}

unsigned HDMediaStream::QueryNetworkBufferProgressValue()
{
	if (_eos || !_active)
		return 100;
	if (_samples.GetCount() <= 1)
		return 0;

	ComPtr<IMFSample> pSampleStart,pSampleEnd;
	_samples.GetFront(&pSampleStart);
	_samples.GetBack(&pSampleEnd);

	LONG64 ptsStart = 0,ptsEnd = 0;
	pSampleStart->GetSampleTime(&ptsStart);
	pSampleEnd->GetSampleTime(&ptsEnd);
	if (ptsStart == 0 && ptsEnd == 0)
		return 0;

	LONG64 temp = ptsEnd - ptsStart;
	return (unsigned)((float)temp / float(_preroll_time + _preroll_dynamic_time) * 100.0f);
}

HRESULT HDMediaStream::SendSampleDirect(IUnknown* pToken)
{
	HRESULT hr = S_OK;
	ComPtr<IMFSample> pSample;
	while (1)
	{
		hr = _samples.RemoveFront(pSample.GetAddressOf());
		HR_FAILED_BREAK(hr);
		if (pToken != nullptr)
			pSample->SetUnknown(MFSampleExtension_Token,pToken);

		hr = _pEventQueue->QueueEventParamUnk(MEMediaSample,GUID_NULL,S_OK,pSample.Get());
		HR_FAILED_BREAK(hr);
		break;
	}
#ifdef _DEBUG
	if (_samples.IsEmpty())
		DbgLogPrintf(L"*** Stream %d Queue Pull Empty. (SendSampleDirect) ***",_index);
#endif

	if (SUCCEEDED(hr))
	{
		if (_samples.IsEmpty() && _eos)
		{
			DbgLogPrintf(L"%s::MEEndOfStream... (Pending Request:%d.)",L"HDMediaStream",_requests.GetCount());
			hr = _pEventQueue->QueueEventParamVar(MEEndOfStream,GUID_NULL,S_OK,nullptr);
			if (SUCCEEDED(hr))
				hr = _pMediaSource->QueueAsyncOperation(SourceOperation::OP_END_OF_STREAM);
		}
		else if (NeedsData()) {
			hr = _pMediaSource->QueueAsyncOperation(SourceOperation::OP_REQUEST_DATA);
		}
	}

	return hr;
}

void HDMediaStream::DispatchSamples()
{
	if (_state != STATE_STARTED)
		return;

	HRESULT hr = S_OK;

	while (!_samples.IsEmpty() && !_requests.IsEmpty())
	{
		ComPtr<IMFSample> pSample;
		ComPtr<IUnknown> pUnk;

		hr = _samples.RemoveFront(pSample.GetAddressOf());
		if (FAILED(hr))
			break;

		hr = _requests.RemoveFront(pUnk.GetAddressOf());
		if (FAILED(hr))
			break;

		if (pUnk != nullptr)
		{
			hr = pSample->SetUnknown(MFSampleExtension_Token,pUnk.Get());
			if (FAILED(hr))
				break;
		}

		//post new sample.
		hr = _pEventQueue->QueueEventParamUnk(MEMediaSample,
			GUID_NULL,S_OK,pSample.Get());
		
		if (FAILED(hr))
			break;

#ifdef _DEBUG
		if (_samples.IsEmpty())
			DbgLogPrintf(L"*** Stream %d Queue Pull Empty. (DispatchSamples) ***",_index);
		//else
			//DbgLogPrintf(L"Stream %d Current Queue Size:%d",_index,_samples.GetCount());
#endif
	}

	if (SUCCEEDED(hr))
	{
		if ((_samples.IsEmpty() && _eos) || (_state == STATE_SHUTDOWN))
		{
			DbgLogPrintf(L"%s::MEEndOfStream... (Pending Request:%d.)",L"HDMediaStream",_requests.GetCount());

			if (!_requests.IsEmpty())
				_requests.Clear();

			hr = _pEventQueue->QueueEventParamVar(MEEndOfStream,GUID_NULL,S_OK,nullptr);
			if (SUCCEEDED(hr))
				hr = _pMediaSource->QueueAsyncOperation(SourceOperation::OP_END_OF_STREAM);
		}
		else if (NeedsData())
		{
			hr = _pMediaSource->QueueAsyncOperation(SourceOperation::OP_REQUEST_DATA);
		}
	}

	if (FAILED(hr))
	{
		if (SUCCEEDED(CheckShutdown()))
			_pMediaSource->ProcessOperationError(hr);
	}
}

HRESULT HDMediaStream::DispatchSamplesAsync()
{
	SourceAutoLock lock(_pMediaSource.Get());
	DbgLogPrintf(L"Enter DispatchSamplesAsync...");
	DispatchSamples();
	MaybeSendNetworkBuffering();
	DbgLogPrintf(L"Leave DispatchSamplesAsync.");

	return S_OK;
}

double HDMediaStream::GetSampleQueueFirstTime()
{
	if (_samples.GetCount() == 0)
		return -1.0;
	
	ComPtr<IMFSample> pSample;
	_samples.GetFront(&pSample);
	return WMF::Misc::GetSecondsFromMFSample(pSample.Get());
}

double HDMediaStream::GetSampleQueueLastTime()
{
	if (_samples.GetCount() == 0)
		return -1.0;
	
	ComPtr<IMFSample> pSample;
	_samples.GetBack(&pSample);
	return WMF::Misc::GetSecondsFromMFSample(pSample.Get());
}

#ifdef _USE_DECODE_FILTER
bool HDMediaStream::GetTransformFilter(ITransformFilter** ppFilter)
{
	if (!_transform_filter)
		return false;
	return SUCCEEDED(_pStreamDesc->GetUnknown(MF_MT_MY_TRANSFORM_FILTER_INTERFACE,IID_PPV_ARGS(ppFilter)));
}

bool HDMediaStream::ProcessDirectXManager()
{
	DbgLogPrintf(L"%s %d::OnProcessDirectXManager...",L"HDMediaStream",_index);
	if (!_transform_filter)
		return false;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	auto dx = _pMediaSource->GetD3D9DeviceManager();
#else
	auto dx = _pMediaSource->GetDXGIDeviceManager();
#endif
	if (dx == NULL)
		return false;

	ComPtr<IMFAttributes> attrs;
	if (FAILED(MFCreateAttributes(&attrs,3)))
		return false;

	ComPtr<IMFVideoSampleAllocatorEx> alloctor;
	if (FAILED(MFCreateVideoSampleAllocatorEx(IID_PPV_ARGS(&alloctor))))
		return false;

	attrs->SetUINT32(MF_SA_D3D11_USAGE,D3D11_USAGE_DEFAULT);
	attrs->SetUINT32(MF_SA_D3D11_BINDFLAGS,D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE);
	attrs->SetUINT32(MF_SA_BUFFERS_PER_SAMPLE,1);
	
	bool result = false;
	if (SUCCEEDED(alloctor->SetDirectXManager(dx))) {
		GUID type = GUID_NULL;
		GetMediaType()->GetGUID(MF_MT_SUBTYPE,&type);
		if (type == MFVideoFormat_NV12 ||
			type == MFVideoFormat_YV12 ||
			type == MFVideoFormat_I420 ||
			type == MFVideoFormat_IYUV ||
			type == MFVideoFormat_UYVY ||
			type == MFVideoFormat_YUY2 ||
			type == MFVideoFormat_P010 ||
			type == MFVideoFormat_P016 ||
			type == MFVideoFormat_RGB24 ||
			type == MFVideoFormat_RGB32 ||
			type == MFVideoFormat_ARGB32) {
			if (SUCCEEDED(alloctor->InitializeSampleAllocatorEx(1,32,attrs.Get(),GetMediaType()))) {
				ComPtr<ITransformFilter> pFilter;
				if (GetTransformFilter(&pFilter)) {
					ComPtr<ITransformWorker> pWorker;
					if (SUCCEEDED(pFilter->GetService(IID_PPV_ARGS(&pWorker)))) {
						auto transform_allocator = Make<DXVATransformSampleAllocator>(alloctor.Get());
						pWorker->SetAllocator(transform_allocator.Get());
						_decoder = pWorker;
						result = true;
						DbgLogPrintf(L"Stream %d to use DXVA Allocator.",_index);
					}
				}
			}
		}
	}
	return result;
}

void HDMediaStream::OnProcessDirectXManager()
{
	if (!ProcessDirectXManager()) {
		ComPtr<ITransformFilter> pFilter;
		if (GetTransformFilter(&pFilter))
			pFilter->GetService(IID_PPV_ARGS(&_decoder));
	}
}
#endif

void HDMediaStream::SetPrivateData(unsigned char* pb,unsigned len)
{
	_private_state = true;
	_private_data.Free();

	_private_data(len);
	memcpy(_private_data.Get(),pb,len);

	_private_size = len;
}

void HDMediaStream::QueueTickEvent(LONG64 time)
{
	PROPVARIANT prop;
	AutoPropVar var(prop);
	var.SetInt64(time);
	_pEventQueue->QueueEventParamVar(MEStreamTick,GUID_NULL,S_OK,&prop);
}

/////////////////// Decode Processing ///////////////////

HRESULT HDMediaStream::RequestSampleWithDecode(IUnknown* pToken)
{
	HRESULT hr = _requests.InsertBack(pToken);
	HR_FAILED_RET(hr);

	if (_samples.IsEmpty() || _pMediaSource->IsBuffering())
		hr = _pMediaSource->QueueAsyncOperation(SourceOperation::OP_REQUEST_DATA);
	else
		hr = RequestSampleAsync();
	return hr;
}

HRESULT HDMediaStream::DoSampleDecodeRequests()
{
	HRESULT hr = S_OK;
#ifdef _USE_DECODE_FILTER
	while (1)
	{
		ComPtr<IMFSample> pSample;
		{
			SourceAutoLock lock(_pMediaSource.Get());
			if (FAILED(CheckShutdown()))
				break;

			_decode_processing = true;
			if (_requests.IsEmpty())
				break;
			if (_samples.IsEmpty() && !_eos)
				break;

			if (!_samples.IsEmpty()) {
				hr = _samples.RemoveFront(&pSample);
				HR_FAILED_BREAK(hr);
			}
		}

		ComPtr<IMFSample> pDecodedSample;
		hr = _decoder->ProcessSample(pSample.Get(),&pDecodedSample);
		if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
			continue;

		if (FAILED(hr) || pDecodedSample == nullptr) {
			SourceAutoLock lock(_pMediaSource.Get());
			if (_samples.IsEmpty() && _eos) {
				_dec_eos = true;
				_requests.Clear();
				_pEventQueue->QueueEventParamVar(MEEndOfStream,GUID_NULL,S_OK,nullptr);
				_pMediaSource->QueueAsyncOperation(SourceOperation::OP_END_OF_STREAM);
			}
			break;
		}

		ComPtr<IUnknown> pRequest;
		{
			SourceAutoLock lock(_pMediaSource.Get());
			_requests.RemoveFront(&pRequest);

			if (pDecodedSample && SUCCEEDED(CheckShutdown())) {
				if (pRequest)
					pDecodedSample->SetUnknown(MFSampleExtension_Token,pRequest.Get());
				hr = _pEventQueue->QueueEventParamUnk(MEMediaSample,GUID_NULL,S_OK,pDecodedSample.Get());
			}
		}
	}

	{
		SourceAutoLock lock(_pMediaSource.Get());
		_decode_processing = false;
		if (FAILED(CheckShutdown()))
			return hr;

		if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT && _samples.IsEmpty() && !_eos)
			_pMediaSource->QueueAsyncOperation(SourceOperation::OP_REQUEST_DATA);

		if (SUCCEEDED(hr)) {
			if (NeedsData()) {
				_pMediaSource->QueueAsyncOperation(SourceOperation::OP_REQUEST_DATA);
			}else if (_eos && _samples.IsEmpty() && _requests.IsEmpty()) {
				_dec_eos = true;
				_pEventQueue->QueueEventParamVar(MEEndOfStream,GUID_NULL,S_OK,nullptr);
				_pMediaSource->QueueAsyncOperation(SourceOperation::OP_END_OF_STREAM);
			}
		}else{
			if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT)
				_pMediaSource->ProcessOperationError(hr);
			else
				hr = S_OK;
		}
	}
#endif
	return hr;
}