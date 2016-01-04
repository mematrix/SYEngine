#include "HDMediaSource.h"
#include "HDMediaStream.h"

ComPtr<HDMediaSource> HDMediaSource::CreateMediaSource(int QueueSize)
{
	ComPtr<HDMediaSource> sp;
	
	auto p = new (std::nothrow)HDMediaSource(QueueSize);
	if (p != nullptr)
	{
		p->AddRef();
		sp.Attach(p);
	}

	return sp;
}

HDMediaSource::HDMediaSource(int QueueSize) : 
_state(STATE_INVALID), _taskMagicNumber(0), _nPendingEOS(0), 
_start_op_seek_time(PACKET_NO_PTS), _sampleStartTime(0.0), _currentRate(1.0f),
_network_mode(false), _network_delay(0), _network_buffering(false),
_network_preroll_time(0.0), _network_buffer_progress(0), _network_live_stream(false),
_enableH264ES2H264(false), _intelQSDecoder_found(false)
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
,_taskWorkQueue(this,&_taskInvokeCallback)
#endif
{
	Module<InProc>::GetModule().IncrementObjectCount();

	_forceQueueSize = QueueSize;
	_taskInvokeCallback.SetCallback(this,&HDMediaSource::OnInvoke);

	_pChapters = nullptr;
	_pMetadata = nullptr;

	_key_frames = nullptr;
	_key_frames_count = 0;

	_hCurrentDemuxMod = nullptr;

	_seekAfterFlag = _notifyParserSeekAfterFlag = false;
}

HDMediaSource::~HDMediaSource()
{
	_pMediaParser.reset();
	if (_hCurrentDemuxMod)
		FreeLibrary(_hCurrentDemuxMod);
	if (_key_frames)
		free(_key_frames);

	DbgLogPrintf(L"%s::Deleted.",L"HDMediaSource");
	Module<InProc>::GetModule().DecrementObjectCount();
}

HRESULT HDMediaSource::CreatePresentationDescriptor(IMFPresentationDescriptor **ppPresentationDescriptor)
{
	if (ppPresentationDescriptor == nullptr)
		return E_POINTER;

	CritSec::AutoLock lock(_cs);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	hr = IsInitialized();
	if (FAILED(hr))
		return hr;

	if (_pPresentationDescriptor == nullptr)
		return MF_E_NOT_INITIALIZED;

	//created in OpenAsync.
	return _pPresentationDescriptor->Clone(ppPresentationDescriptor);
}

HRESULT HDMediaSource::GetCharacteristics(DWORD *pdwCharacteristics)
{
	if (pdwCharacteristics == nullptr)
		return E_POINTER;

	CritSec::AutoLock lock(_cs);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	*pdwCharacteristics = 
		MFMEDIASOURCE_CAN_PAUSE|MFMEDIASOURCE_CAN_SEEK;
	if (_network_live_stream)
		*pdwCharacteristics = MFMEDIASOURCE_CAN_PAUSE|MFMEDIASOURCE_IS_LIVE;

	return S_OK;
}

HRESULT HDMediaSource::Start(IMFPresentationDescriptor *pPresentationDescriptor,const GUID *pguidTimeFormat,const PROPVARIANT *pvarStartPosition)
{
	DbgLogPrintf(L"%s::Start...",L"HDMediaSource");

	if (pPresentationDescriptor == nullptr ||
		pvarStartPosition == nullptr)
		return E_INVALIDARG;

	if ((pguidTimeFormat != nullptr) && (*pguidTimeFormat != GUID_NULL))
		return MF_E_UNSUPPORTED_TIME_FORMAT; //只支持100ns

	if (pvarStartPosition->vt != VT_I8 && pvarStartPosition->vt != VT_EMPTY)
		return MF_E_UNSUPPORTED_TIME_FORMAT;

	CritSec::AutoLock lock(_cs);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	hr = IsInitialized();
	if (FAILED(hr))
		return hr;

	hr = ValidatePresentationDescriptor(pPresentationDescriptor);
	if (FAILED(hr))
		return hr;

	if (_state == STATE_STARTED)
		DbgLogPrintf(L"%s::was already started.",L"HDMediaSource");

	ComPtr<SourceOperation> op;
	hr = SourceOperation::CreateStartOperation(
		pPresentationDescriptor,
		_start_op_seek_time,
		op.GetAddressOf());
	if (FAILED(hr))
		return hr;
	_start_op_seek_time = PACKET_NO_PTS;

	hr = op->SetData(*pvarStartPosition);
	if (FAILED(hr))
		return hr;

	DbgLogPrintf(L"%s::Start OK.",L"HDMediaSource");

	_enterReadPacketFlag = false;
	_network_buffering = false;
	return QueueOperation(op.Get());
}

HRESULT HDMediaSource::Pause()
{
	DbgLogPrintf(L"%s::Pause...",L"HDMediaSource");

	CritSec::AutoLock lock(_cs);

	if (_state == STATE_PAUSED)
		return MF_E_INVALID_STATE_TRANSITION;

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	hr = IsInitialized();
	if (FAILED(hr))
		return hr;

	DbgLogPrintf(L"%s::Pause OK.",L"HDMediaSource");

	return QueueAsyncOperation(SourceOperation::OP_PAUSE);
}

HRESULT HDMediaSource::Stop()
{
	DbgLogPrintf(L"%s::Stop...",L"HDMediaSource");

	CritSec::AutoLock lock(_cs);

	if (_state == STATE_STOPPED)
		return MF_E_INVALID_STATE_TRANSITION;

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	hr = IsInitialized();
	if (FAILED(hr))
		return hr;

	DbgLogPrintf(L"%s::Stop OK.",L"HDMediaSource");

	return QueueAsyncOperation(SourceOperation::OP_STOP);
}

HRESULT HDMediaSource::Shutdown()
{
	DbgLogPrintf(L"%s::Shutdown...",L"HDMediaSource");

	if (_key_frames)
		free(_key_frames);
	_key_frames = nullptr;
	_key_frames_count = 0;

	if (_pMediaIOEx && _network_mode)
		_pMediaIOEx->CancelReading();

	if (_network_mode)
	{
		IMFByteStream* stream = nullptr;
		if (_pMediaIO)
			stream = _pMediaIO->GetMFByteStream();
		else if (_pMediaIOEx)
			stream = _pMediaIOEx->GetMFByteStream();
		if (stream)
			stream->Close();
	}

	if (_enterReadPacketFlag) {
		while (1)
		{
			if (!StreamsNeedData())
				break;
		}
	}

	CritSec::AutoLock lock(_cs);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr))
		return hr;

	unsigned count = _streamList.Count();
	for (unsigned i = 0;i < count;i++)
	{
		ComPtr<HDMediaStream> pStream;
		HRESULT hr = _streamList.GetAt(i,pStream.GetAddressOf());
		HR_FAILED_CONTINUE(hr);

		hr = pStream->Shutdown();
		HR_FAILED_RET(hr);
	}

	_pChapters = nullptr;
	_pMetadata = nullptr;
	if (_pMediaParser != nullptr)
	{
		auto ave = _pMediaParser->Close();
		if (AVE_FAILED(ave))
		{
			if (IsDebuggerPresent())
				OutputDebugStringW(L"Close Parser FAILED!\n");
		}
	}

	_streamList.Clear();
	_pEventQueue->Shutdown();

	_pByteStreamTimeSeek.Reset();
	_pByteStream.Reset();
	_pOpenResult.Reset();

	_pEventQueue.Reset();
	_pPresentationDescriptor.Reset();

	_pMediaIO.reset();
	_pMediaIOEx.Reset();

	_taskMagicNumber = -1;
	_state = STATE_SHUTDOWN;

	DbgLogPrintf(L"%s::Shutdown OK.",L"HDMediaSource");

	return S_OK;
}

HRESULT HDMediaSource::SelectStreams(IMFPresentationDescriptor* ppd,const PROPVARIANT& varStartPosition,bool seek,bool sendstart)
{
	_nPendingEOS = 0;

	unsigned count = _streamList.Count();
	for (unsigned i = 0;i < count;i++)
	{
		BOOL fSelected = FALSE;
		ComPtr<IMFStreamDescriptor> psd;

		HRESULT hr = ppd->GetStreamDescriptorByIndex(i,&fSelected,psd.GetAddressOf());
		if (FAILED(hr))
			return hr;

		DWORD dwStreamId = 0xFFFFFFFF;
		hr = psd->GetStreamIdentifier(&dwStreamId);
		if (FAILED(hr))
			return hr;

		if (dwStreamId == 0xFFFFFFFF)
			continue;

		ComPtr<HDMediaStream> pStream;
		hr = FindMediaStreamById((int)dwStreamId,pStream.GetAddressOf());
		if (FAILED(hr))
			return hr;

		if (seek)
			pStream->DisablePrivateData();
		else
			pStream->EnablePrivateData();

		BOOL fWasSelected = pStream->IsActive();

		pStream->Activate(!!fSelected);

		if (fSelected)
		{
			_nPendingEOS++;

			hr = _pEventQueue->QueueEventParamUnk(fWasSelected ? MEUpdatedStream:MENewStream,
				GUID_NULL,S_OK,pStream.Get());

			if (FAILED(hr))
				return hr;

			if (sendstart) {
				hr = pStream->Start(varStartPosition,seek);
				if (FAILED(hr))
					return hr;
			}

			if (_network_mode)
				pStream->SetPrerollTime(_network_preroll_time);
		}
	}

	DbgLogPrintf(L"%s::DoStart->SelectStreams %d.",L"HDMediaSource",_nPendingEOS);

	return S_OK;
}

HRESULT HDMediaSource::SeekOpen(LONG64 seekTo)
{
	DbgLogPrintf(L"%s::DoStart->SeekOpen %lld",L"HDMediaSource",seekTo);

	unsigned count = _streamList.Count();
	if (count == 0)
		return E_FAIL;

	for (unsigned i = 0;i < count;i++)
	{
		ComPtr<HDMediaStream> pStream;
		HRESULT hr = _streamList.GetAt(i,pStream.GetAddressOf());
		HR_FAILED_CONTINUE(hr);

		CallStreamMethod(pStream,ClearQueue)();

		pStream->SetDiscontinuity();
	}

	if (_pSampleRequest)
		_pSampleRequest.Reset();

	bool seek_byte_stm = false;
	if (_pByteStreamTimeSeek != nullptr)
	{
		BOOL ok = FALSE;
		_pByteStreamTimeSeek->IsTimeSeekSupported(&ok);
		if (ok)
			seek_byte_stm = true;
	}

	if (!seek_byte_stm)
	{
		double seconds = (double)seekTo / 10000000.0;
		auto ave = _pMediaParser->Seek(seconds,true,AVSeekDirection::SeekDirection_Back);
		//使用SeekDirection_Back，保证Parser的Seek一直是前于关键帧的。
		//MF的Render会负责丢弃额外的帧。

		if (AVE_FAILED(ave))
			return MF_E_INVALID_POSITION;
	}else{
		auto hr = _pByteStreamTimeSeek->TimeSeek(seekTo);
		if (FAILED(hr))
			return hr;

		if (!_network_mode)
			_pMediaParser->OnNotifySeek();
		else
			_notifyParserSeekAfterFlag = true;
	}

	_seekToTime = (double)seekTo / 10000000.0;
	_seekAfterFlag = true; //Seek成功后，标识下一次ReadPacket是Seek后的
	DbgLogPrintf(L"%s::DoStart->SeekOpen OK.",L"HDMediaSource");

	return S_OK;
}

HRESULT HDMediaSource::DoStart(SourceOperation* op)
{
	DbgLogPrintf(L"%s::DoStart...",L"HDMediaSource");

	if (op->GetOperation() != SourceOperation::OP_START)
		return E_UNEXPECTED;

	ComPtr<IMFPresentationDescriptor> ppd;
	auto start_op = static_cast<StartOperation*>(op);
	HRESULT hr = start_op->GetPresentationDescriptor(ppd.GetAddressOf());
	if (FAILED(hr))
		return hr;

	bool bSeek = false,bResume = false;
	bool forceSeek = false;
	AutoPropVar propvar(&op->GetData());
	if (propvar.GetType() == VT_I8)
	{
		if ((_state != STATE_STOPPED) && (propvar.GetInt64() != 0))
		{
			bSeek = true;
		}else if (propvar.GetInt64() <= 0)
		{
			if (_state == STATE_STARTED || _state == STATE_PAUSED)
				bSeek = forceSeek = true;
		}
	}else if (propvar.GetType() == VT_EMPTY)
	{
		if (_state == STATE_PAUSED)
			bResume = true;
	}

	if (bResume)
		DbgLogPrintf(L"%s::DoStart->Resume Play...",L"HDMediaSource");

	hr = SelectStreams(ppd.Get(),op->GetData(),bSeek,false);

	_seekAfterFlag = false; //指示Seek后的第一个Packet
	if ((SUCCEEDED(hr) && (propvar.GetInt64() != 0)) || forceSeek) {
		hr = SeekOpen(propvar.GetInt64()); //to Seek...
	}else{
		if (start_op->GetSeekToTime() != PACKET_NO_PTS)
			_pMediaParser->Seek(start_op->GetSeekToTime(), true, AVSeekDirection::SeekDirection_Auto);
	}

	if (SUCCEEDED(hr))
	{
		DbgLogPrintf(L"%s::DoStart->Event %s",L"HDMediaSource",
			bSeek ? L"MESourceSeeked":L"MESourceStarted");

		ComPtr<IMFMediaEvent> pNewEvent;
		hr = MFCreateMediaEvent(bSeek ? MESourceSeeked:MESourceStarted,GUID_NULL,S_OK,
			&op->GetData(),pNewEvent.GetAddressOf());

		if (SUCCEEDED(hr)) //[optional] MF_EVENT_SOURCE_ACTUAL_START...
			hr = _pEventQueue->QueueEvent(pNewEvent.Get());

		if (SUCCEEDED(hr) && !bResume)
		{
			_state = STATE_BUFFERING;
			if (_network_mode)
				SendNetworkStartBuffering();
			else
				PreloadStreamPacket(); //如果不是打开网络流，DoOpen就执行LoadSample队列

#ifdef _DEBUG
			double queueMaxTime = GetAllStreamMaxQueueTime();
			DbgLogPrintf(L"%s::DoStart->Queue Max Time %0.3f",L"HDMediaSource",(float)queueMaxTime);
#endif
		}

		if (SUCCEEDED(hr))
		{
			for (unsigned i = 0;i < _streamList.Count();i++)
			{
				ComPtr<HDMediaStream> pStream;
				_streamList.GetAt(i,pStream.GetAddressOf());

				if (pStream->IsActive()) {
					pStream->SetDiscontinuity();
					pStream->Start(op->GetData(),bSeek);
				}
			}
		}
	}

	_state = STATE_STARTED;

	if (FAILED(hr))
		_pEventQueue->QueueEventParamVar(MESourceStarted,GUID_NULL,hr,nullptr);

	DbgLogPrintf(L"%s::DoStart Result:0x%08X",L"HDMediaSource",hr);
	return hr;
}

HRESULT HDMediaSource::DoPause(SourceOperation* op)
{
	DbgLogPrintf(L"%s::DoPause...",L"HDMediaSource");

	if (op->GetOperation() != SourceOperation::OP_PAUSE)
		return E_UNEXPECTED;

	unsigned count = _streamList.Count();
	if (count == 0)
		return E_FAIL;

	for (unsigned i = 0;i < count;i++)
	{
		ComPtr<HDMediaStream> pStream;
		HRESULT hr = _streamList.GetAt(i,pStream.GetAddressOf());
		HR_FAILED_CONTINUE(hr);

		CallStreamMethod(pStream,Pause)();
	}

	_state = STATE_PAUSED;

	DbgLogPrintf(L"%s::DoPause OK.",L"HDMediaSource");

	return _pEventQueue->QueueEventParamVar(MESourcePaused,GUID_NULL,S_OK,nullptr);
}

HRESULT HDMediaSource::DoStop(SourceOperation* op)
{
	DbgLogPrintf(L"%s::DoStop...",L"HDMediaSource");

	if (op->GetOperation() != SourceOperation::OP_STOP)
		return E_UNEXPECTED;

	unsigned count = _streamList.Count();
	if (count == 0)
		return E_FAIL;

	for (unsigned i = 0;i < count;i++)
	{
		ComPtr<HDMediaStream> pStream;
		HRESULT hr = _streamList.GetAt(i,pStream.GetAddressOf());
		HR_FAILED_CONTINUE(hr);

		CallStreamMethod(pStream,Stop)();

		pStream->SetDiscontinuity();
	}

	//忽略那些Stop前还在队列的请求。
	++_taskMagicNumber;

	if (!_network_mode) {
		auto ave = _pMediaParser->Seek(0.0,true,AVSeekDirection::SeekDirection_Auto);
		if (AVE_FAILED(ave))
			return MF_E_INVALID_POSITION;
	}else{
		_start_op_seek_time = 0.0;
		//网络流的情况下，为了减少seek，这里不处理。
	}

	if (_pSampleRequest)
		_pSampleRequest.Reset();

	_notifyParserSeekAfterFlag = false;
	_state = STATE_STOPPED;

	DbgLogPrintf(L"%s::DoStop OK (Task Magic:%d).",L"HDMediaSource",_taskMagicNumber);
	return _pEventQueue->QueueEventParamVar(MESourceStopped,GUID_NULL,S_OK,nullptr);
}

HRESULT HDMediaSource::DoSetRate(SourceOperation* op)
{
	DbgLogPrintf(L"%s::DoSetRate...",L"HDMediaSource");

	if (op->GetOperation() != SourceOperation::OP_SETRATE)
		return E_UNEXPECTED;

	float flRate = static_cast<SetRateOperation*>(op)->GetRate();

	if (flRate >= PEAK_MFSOURCE_RATE_VALUE)
	{
		unsigned peak = (unsigned)(flRate / PEAK_MFSOURCE_RATE_VALUE) + 1;

		unsigned count = _streamList.Count();
		for (unsigned i = 0;i < count;i++)
		{
			ComPtr<HDMediaStream> pStream;
			HRESULT hr = _streamList.GetAt(i,pStream.GetAddressOf());
			HR_FAILED_CONTINUE(hr);

			CallStreamMethod(pStream,SetQueueSize)(pStream->GetQueueSize() * peak);
		}
	}

	_currentRate = flRate;

	DbgLogPrintf(L"%s::DoSetRate OK (%0.2f).",L"HDMediaSource",flRate);

	return _pEventQueue->QueueEventParamVar(MESourceRateChanged,GUID_NULL,S_OK,nullptr);
}

HRESULT HDMediaSource::OnRequestSample(SourceOperation* op)
{
	{
		CritSec::AutoLock lock(_cs);

		if (_pSampleRequest != nullptr)
			return S_OK;

		_pSampleRequest = op;
	}

	bool notifySeek = false;
	if (_network_mode)
	{
		CritSec::AutoLock lock(_cs);
		notifySeek = _notifyParserSeekAfterFlag;
		_notifyParserSeekAfterFlag = false;
	}

	bool seek_error = false;
	if (notifySeek)
	{
		DbgLogPrintf(L"OnNotifySeek...");
		if (_pMediaParser->OnNotifySeek() != AV_ERR_OK) {
			CritSec::AutoLock lock(_cs);
			seek_error = true;
			if (_network_buffering)
				SendNetworkStopBuffering();
			NotifyParseEnded();
		}
		DbgLogPrintf(L"OnNotifySeek OK.");
	}

	if (!seek_error)
	{
		DbgLogPrintf(L"OnRequestSample...");
		PreloadStreamPacket();
		DbgLogPrintf(L"OnRequestSample OK.");
	}

	{
		CritSec::AutoLock lock(_cs);

		if (_pSampleRequest)
			_pSampleRequest.Reset();
	}

	return S_OK;
}

HRESULT HDMediaSource::OnEndOfStream(SourceOperation* op)
{
	if (op->GetOperation() != SourceOperation::OP_END_OF_STREAM)
		return E_UNEXPECTED;

	DbgLogPrintf(L"%s::OnEndOfStream (Current:%d).",L"HDMediaSource",_nPendingEOS);

	--_nPendingEOS;
	if (_nPendingEOS <= 0)
		return _pEventQueue->QueueEventParamVar(MEEndOfPresentation,GUID_NULL,S_OK,nullptr);

	return S_OK;
}