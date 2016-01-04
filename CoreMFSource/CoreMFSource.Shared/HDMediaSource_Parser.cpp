#include "HDMediaSource.h"
#include "HDMediaStream.h"

static inline bool Is264NaluWithPPS(unsigned char* pb)
{
	return (pb[4] & 0x1F) == 8;
}

static inline bool Is265NaluWithPPS(unsigned char* pb)
{
	return pb[4] == 0x44;
}

static int FindNextNaluOffset(unsigned char* pb,unsigned size)
{
	if (pb == nullptr || size < 7)
		return -1;

	unsigned char* p = pb;
	for (unsigned i = 4;i < (size - 4);i++)
	{
		if (p[i] == 0 &&
			p[i + 1] == 0 &&
			p[i + 2] == 0 &&
			p[i + 3] == 1)
			return (int)i;
	}
	return -1;
}

static HRESULT SwapPCMBE2LE(unsigned bits,IMFSample* pSample,IMFSample** ppNewSample)
{
	if (bits != 16 &&
		bits != 24 &&
		bits != 32)
		return E_ABORT;

	ComPtr<IMFSample> pNewSample;
	HRESULT hr = MFCreateSample(pNewSample.GetAddressOf());
	HR_FAILED_RET(hr);

	LONG64 pts = 0,duration = 0;
	if (SUCCEEDED(pSample->GetSampleTime(&pts)))
		pNewSample->SetSampleTime(pts);
	if (SUCCEEDED(pSample->GetSampleDuration(&duration)))
		pNewSample->SetSampleDuration(duration);

	ComPtr<IMFMediaBuffer> pOldBuffer;
	hr = pSample->ConvertToContiguousBuffer(pOldBuffer.GetAddressOf());
	HR_FAILED_RET(hr);

	DWORD dwBufLength = 0;
	pOldBuffer->GetCurrentLength(&dwBufLength);
	if (dwBufLength < 2)
		return E_FAIL;

	ComPtr<IMFMediaBuffer> pNewBuffer;
	hr = MFCreateMemoryBuffer(dwBufLength,pNewBuffer.GetAddressOf());
	HR_FAILED_RET(hr);

	pNewBuffer->SetCurrentLength(dwBufLength);

	WMF::AutoBufLock lockOld(pOldBuffer.Get());
	WMF::AutoBufLock lockNew(pNewBuffer.Get());

	if (lockNew.GetPtr() == nullptr ||
		lockOld.GetPtr() == nullptr)
		return E_UNEXPECTED;

	unsigned step = 2;
	if (bits == 24)
		step = 3;
	else if (bits == 32)
		step = 4;

	DWORD length = dwBufLength / step;
	PBYTE pn = lockNew.GetPtr();
	PBYTE po = lockOld.GetPtr();
	for (unsigned i = 0;i < length;i++)
	{
		for (unsigned j = 0;j < step;j++)
			pn[j] = po[(step - j) - 1];

		pn += step;
		po += step;
	}

	hr = pNewSample->AddBuffer(pNewBuffer.Get());
	HR_FAILED_RET(hr);

	*ppNewSample = pNewSample.Detach();
	return S_OK;
}

HRESULT HDMediaSource::CreateMFSampleFromAVMediaBuffer(AVMediaBuffer* avBuffer,IMFSample** ppSample)
{
	if (avBuffer->buf == nullptr ||
		avBuffer->size == 0)
		return MF_E_BUFFERTOOSMALL;

	ComPtr<IMFSample> pSample;
	ComPtr<IMFMediaBuffer> pMediaBuffer;

	HRESULT hr = WMF::Misc::CreateSampleWithBuffer(pSample.GetAddressOf(),
		pMediaBuffer.GetAddressOf(),avBuffer->size,4); //4 bytes aligned.

	if (FAILED(hr))
		return hr;

	PBYTE pData = nullptr;
	hr = pMediaBuffer->Lock(&pData,nullptr,nullptr);
	if (FAILED(hr))
		return hr;

	memcpy(pData,avBuffer->buf,avBuffer->size);

	hr = pMediaBuffer->Unlock();
	if (FAILED(hr))
		return hr;

	hr = pMediaBuffer->SetCurrentLength(avBuffer->size);
	if (FAILED(hr))
		return hr;

	*ppSample = pSample.Detach();
	return S_OK;
}

HRESULT HDMediaSource::PreloadStreamPacket()
{
	if (QueueStreamPacket() == QueuePacketNotifyNetwork || _network_buffering) {
		CritSec::AutoLock lock(_cs);
		SendNetworkStopBuffering();

		unsigned count = _streamList.Count();
		for (unsigned i = 0;i < count;i++)
		{
			ComPtr<HDMediaStream> pStream;
			_streamList.GetAt(i,pStream.GetAddressOf());

			if (pStream->IsActive())
				pStream->SubmitSample(nullptr);
		}
	}
	return S_OK;
}

bool HDMediaSource::StreamsNeedData()
{
	CritSec::AutoLock lock(_cs);

	if (_state == STATE_SHUTDOWN ||
		_state == STATE_STOPPED)
		return false;

	bool result = false;
	unsigned count = _streamList.Count();

	for (unsigned i = 0;i < count;i++)
	{
		ComPtr<HDMediaStream> pStream;
		HRESULT hr = _streamList.GetAt(i,pStream.GetAddressOf());
		HR_FAILED_BREAK(hr);

		if (pStream->NeedsData())
		{
			result = true;
			break;
		}
	}

	return result;
}

HDMediaSource::QueuePacketResult HDMediaSource::QueueStreamPacket()
{
	_enterReadPacketFlag = true;
	QueuePacketResult result = QueuePacketOK;
	bool eos = false;
	while (StreamsNeedData())
	{
		if (_network_mode)
		{
			CritSec::AutoLock lock(_cs);
			if (!_network_buffering && IsNeedNetworkBuffering())
			{
				result = QueuePacketNotifyNetwork;
				SendNetworkStartBuffering();
			}else if (_network_buffering) {
				result = QueuePacketNotifyNetwork;
			}
		}

		_bReadPacketFlag = true;
		AVMediaPacket packet = {};
		auto ave = _pMediaParser->ReadPacket(&packet);
		_bReadPacketFlag = false;
		if (AVE_FAILED(ave))
		{
			eos = true;
			break;
		}
#ifdef _DEBUG
		if (packet.pts != PACKET_NO_PTS)
			DbgLogPrintf(L"PARSER->ReadPacket %.2f (KF:%s, Index:%d, Size:%d)",
			float(packet.pts),
			(packet.flag & MEDIA_PACKET_KEY_FRAME_FLAG) ? L"True":L"False",
			packet.stream_index, packet.data.size);
#endif

		if ((packet.flag & MEDIA_PACKET_BUFFER_NONE_FLAG) == 0)
		{
			CritSec::AutoLock lock(_cs);
			if (_seekAfterFlag)
			{
				_seekAfterFlag = false;
				if (_network_buffering)
					UpdateNetworkDynamicPrerollTime(packet.pts); //保证缓存的packet数据量达到关键帧范围
			}

			ComPtr<HDMediaStream> pStream;
			HRESULT hr = FindMediaStreamById(packet.stream_index,
				pStream.GetAddressOf());

			if (SUCCEEDED(hr))
			{
				ComPtr<IMFSample> pSample;
				hr = CreateMFSampleFromAVMediaBuffer(&packet.data,pSample.GetAddressOf());
				if (SUCCEEDED(hr))
				{
					//if ((packet.flag & MEDIA_PACKET_BUFFER_NONE_FLAG) != 0)
						//continue;
					if ((packet.flag & MEDIA_PACKET_CAN_TO_SKIP_FLAG) != 0)
					{
						FreeMediaPacket(&packet);
						continue;
					}

					if (!pStream->IsActive())
					{
						//此包所属的流不被激活，忽略
						FreeMediaPacket(&packet);
						continue;
					}

					//if is H264 stream, in first frame, to submit SPS\PPS
					if (pStream->IsH264Stream() || pStream->IsHEVCStream() || pStream->IsFLACStream())
					{
						if (pStream->IsQueueEmpty() && pStream->GetPrivateDataSize() > 0)
						{
							ComPtr<IMFSample> pNewSample;
							hr = ProcessSampleMerge(pStream->GetPrivateData(),
								pStream->GetPrivateDataSize(),pSample.Get(),pNewSample.GetAddressOf());

							if (SUCCEEDED(hr))
							{
								pSample.Reset();
								pSample = pNewSample;
							}

							pStream->DisablePrivateData();
						}

						if ((pStream->IsH264Stream() || pStream->IsHEVCStream()) && (packet.data.size > 4))
						{
							bool toMerge = false;
							int size;
							if (pStream->IsH264Stream())
							{
								if (Is264NaluWithPPS(packet.data.buf))
								{
									size = FindNextNaluOffset(pStream->GetPrivateData(),pStream->GetPrivateDataSizeForce());
									if (size != -1)
										toMerge = true;
								}
							}else if (pStream->IsHEVCStream())
							{
								if (Is265NaluWithPPS(packet.data.buf))
								{
									size = (int)pStream->GetPrivateDataSizeForce();
									toMerge = true;
								}
							}

							if (toMerge)
							{
								ComPtr<IMFSample> pNewSample;
								hr = ProcessSampleMerge(pStream->GetPrivateData(),
									size,pSample.Get(),pNewSample.GetAddressOf());

								if (SUCCEEDED(hr))
								{
									pSample.Reset();
									pSample = pNewSample;
								}
							}
						}
					}

					//do PCM-BE to PCM-LE.
					if (pStream->IsPCMStream() && (pStream->GetPCMFormat() == PCM_BE))
					{
						ComPtr<IMFSample> pNewSample;
						if (SUCCEEDED(SwapPCMBE2LE(pStream->GetPCMSize(),
							pSample.Get(),pNewSample.GetAddressOf()))) {
							pSample.Reset();
							pSample = pNewSample;
						}
					}

					if (FAILED(hr))
					{
						FreeMediaPacket(&packet);
						continue;
					}

					double pts = packet.pts;
					if (_sampleStartTime != 0.0 && pts != PACKET_NO_PTS)
					{
						if (_sampleStartTime > pts)
						{
							_sampleStartTime = _pMediaParser->GetStartTime();

							if (_sampleStartTime > pts)
								_sampleStartTime = 0.0;
						}

						pts -= _sampleStartTime;
					}

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
					if (pStream->IsH264Stream() && _enableH264ES2H264)
					{
						ComPtr<IMFSample> pNewSample;
						if (SUCCEEDED(ProcessMPEG2TSToMP4Sample(pSample.Get(),pNewSample.GetAddressOf())))
						{
							pSample.Reset();
							pSample = pNewSample;
						}
					}
#endif

					if (pts != PACKET_NO_PTS)
					{
						LONG64 pts100ns = (LONG64)(pts * 10000000.0);
						pSample->SetSampleTime(pts100ns);
					}

					if (packet.duration != 0.0 && packet.duration != PACKET_NO_PTS)
					{
						pSample->SetSampleDuration((LONG64)(packet.duration * 10000000.0));
					}else{
						if (pStream->IsPCMStream() && pStream->GetPCMPerSecBS() > 0)
						{
							LONG64 duration = 
								packet.data.size * 10000000 /
								pStream->GetPCMPerSecBS();
							pSample->SetSampleDuration(duration);
						}
					}

					if (pStream->GetStreamType() == MediaStream_Video &&
						packet.dts != PACKET_NO_PTS && packet.dts <= packet.pts)
						pSample->SetUINT64(MFSampleExtension_DecodeTimestamp,
							(LONG64)(packet.dts * 10000000.0));

					if (pStream->SwitchDiscontinuity())
						pSample->SetUINT32(MFSampleExtension_Discontinuity,TRUE);

					if ((packet.flag & MEDIA_PACKET_KEY_FRAME_FLAG) != 0)
						pSample->SetUINT32(MFSampleExtension_CleanPoint,TRUE);

					if (_network_mode)
					{
#ifdef _DEBUG
						if (pStream->GetSampleQueueCount() == 0)
							DbgLogPrintf(L"Stream %d Queue Zero!",pStream->GetStreamIndex());
#endif
						if (pStream->GetSampleQueueCount() == 0 && !_network_buffering)
						{
							result = QueuePacketNotifyNetwork;
							SendNetworkStartBuffering();
						}

						if (_network_buffering)
						{
							unsigned progress_value = QueryNetworkBufferProgressValue();
							if (result == QueuePacketNotifyNetwork && progress_value > _network_buffer_progress)
								InterlockedExchange(&_network_buffer_progress,progress_value);
						}
					}

					hr = pStream->SubmitSample(pSample.Get());
					if (FAILED(hr))
					{
						FreeMediaPacket(&packet);

						eos = true;
						break;
					}
				}
			}
		}

		FreeMediaPacket(&packet);
	}

	if (eos) //EndOfStream.
		NotifyParseEnded();
	_enterReadPacketFlag = false;
	return result;
}

void HDMediaSource::NotifyParseEnded()
{
	DbgLogPrintf(L"%s::NotifyParseEnded.",L"HDMediaSource");

	CritSec::AutoLock lock(_cs);

	unsigned count = _streamList.Count();

	for (unsigned i = 0;i < count;i++)
	{
		ComPtr<HDMediaStream> pStream;
		HRESULT hr = _streamList.GetAt(i,pStream.GetAddressOf());
		HR_FAILED_CONTINUE(hr);

		CallStreamMethod(pStream,EndOfStream)();
	}
}

bool HDMediaSource::UpdateNetworkDynamicPrerollTime(double now_pkt_time)
{
	if (now_pkt_time == PACKET_NO_PTS)
		return false;
	if (now_pkt_time >= _seekToTime)
		return false;
	if (_seekToTime + _network_preroll_time >= _pMediaParser->GetDuration())
		return false;

	double prev_keyframe_time = now_pkt_time;
	PROPVARIANT var = {};
	var.vt = VT_I8;
	var.hVal.QuadPart = WMF::Misc::SecondsToMFTime(_seekToTime);
	GUID timeFormat = GUID_NULL;
	PROPVARIANT prev = {}, next = {};
	if (FAILED(MakeKeyFramesIndex()) ||
		FAILED(GetNearestKeyFrames(&timeFormat,&var,&prev,&next)))
		goto UpdatePrerollTime;

	if (prev.vt != next.vt)
		return false;
	if (prev.hVal.QuadPart == next.hVal.QuadPart && now_pkt_time > 15.0)
		return false;

	prev_keyframe_time = 
		(prev.hVal.QuadPart == next.hVal.QuadPart ? now_pkt_time : (double)prev.hVal.QuadPart / 10000000.0);
UpdatePrerollTime:
	double dyn_offset = 
		(_seekToTime + _network_preroll_time) - (prev_keyframe_time + _network_preroll_time);
	if (dyn_offset > _network_preroll_time) {
		unsigned count = _streamList.Count();
		for (unsigned i = 0;i < count;i++)
		{
			ComPtr<HDMediaStream> pStream;
			HRESULT hr = _streamList.GetAt(i,pStream.GetAddressOf());
			if (pStream->IsActive())
				pStream->SetDynamicPrerollTime(dyn_offset);
		}
	}
	return true;
}