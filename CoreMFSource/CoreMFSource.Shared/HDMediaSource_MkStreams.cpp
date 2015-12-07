#include "HDMediaSource.h"
#include "HDMediaStream.h"

HRESULT HDMediaSource::SetMediaStreamPrivateDataFromBlob(HDMediaStream* pMediaStream,const GUID& key)
{
	ComPtr<IMFMediaType> pMediaType = pMediaStream->GetMediaType();

	UINT dwBlobSize = 0;
	pMediaType->GetBlobSize(key,&dwBlobSize);
	if (dwBlobSize == 0)
		return MF_E_UNEXPECTED;

	AutoComMem<unsigned char> buf(dwBlobSize);
	HRESULT hr = pMediaType->GetBlob(key,buf.Get(),dwBlobSize,&dwBlobSize);
	if (FAILED(hr))
		return hr;

	pMediaStream->SetPrivateData(buf.Get(),dwBlobSize);

	return S_OK;
}

HRESULT HDMediaSource::FindBestVideoStreamIndex(IMFPresentationDescriptor* ppd,PDWORD pdwStreamId,UINT* width,UINT* height,float* fps)
{
	if (ppd == nullptr)
		return E_INVALIDARG;

	DWORD dwCount = 0;
	HRESULT hr = ppd->GetStreamDescriptorCount(&dwCount);
	if (FAILED(hr))
		return hr;

	int vid_count = 0;

	auto pw = std::unique_ptr<unsigned[]>(new unsigned[dwCount]);
	auto ph = std::unique_ptr<unsigned[]>(new unsigned[dwCount]);
	auto psid = std::unique_ptr<DWORD[]>(new DWORD[dwCount]);

	for (unsigned i = 0;i < dwCount;i++)
	{
		BOOL fSelected;
		ComPtr<IMFStreamDescriptor> psd;

		hr = ppd->GetStreamDescriptorByIndex(i,&fSelected,psd.GetAddressOf());
		if (FAILED(hr))
			break;

		DWORD dwStreamId = 0;
		hr = psd->GetStreamIdentifier(&dwStreamId);
		if (FAILED(hr))
			break;

		ComPtr<IMFMediaTypeHandler> pHandler;
		hr = psd->GetMediaTypeHandler(pHandler.GetAddressOf());
		if (FAILED(hr))
			return hr;

		ComPtr<IMFMediaType> pMediaType;
		hr = pHandler->GetCurrentMediaType(pMediaType.GetAddressOf());
		if (FAILED(hr))
			break;

		if (FAILED(WMF::Misc::IsVideoMediaType(pMediaType.Get())))
			continue;
		
		UINT nWidth = 0,nHeight = 0;
		hr = MFGetAttributeSize(pMediaType.Get(),MF_MT_FRAME_SIZE,&nWidth,&nHeight);
		if (FAILED(hr))
			continue;

		MFRatio fps_ratio = {0,0};
		MFGetAttributeRatio(pMediaType.Get(),MF_MT_FRAME_RATE,
			(PUINT32)&fps_ratio.Numerator,(PUINT32)&fps_ratio.Denominator);

		if (fps && fps_ratio.Denominator != 0 && fps_ratio.Numerator != 0)
			*fps = (float)fps_ratio.Numerator / (float)fps_ratio.Denominator;

		pw[vid_count] = nWidth;
		ph[vid_count] = nHeight;
		psid[vid_count] = dwStreamId;

		vid_count++;
	}

	if (FAILED(hr))
		return hr;

	if (vid_count == 0)
		return MF_E_NOT_FOUND;

	unsigned cur_wh = pw[0] + ph[0];
	int max_index = 0;

	for (int i = 0;i < vid_count;i++)
	{
		if ((pw[i] + ph[i]) > cur_wh)
		{
			cur_wh = pw[i] + ph[i];
			max_index = i;
		}
	}

	if (pdwStreamId)
		*pdwStreamId = psid[max_index];

	if (width)
		*width = pw[max_index];
	if (height)
		*height = ph[max_index];

	return S_OK;
}

HRESULT HDMediaSource::CreateStreams()
{
	DWORD dwStreamCount = 0;
	_pPresentationDescriptor->GetStreamDescriptorCount(&dwStreamCount);
	if (dwStreamCount == 0)
		return MF_E_UNEXPECTED;

	for (unsigned i = 0;i < dwStreamCount;i++)
	{
		BOOL fSelected;
		ComPtr<IMFStreamDescriptor> psd;

		HRESULT hr = _pPresentationDescriptor->GetStreamDescriptorByIndex(i,
			&fSelected,psd.GetAddressOf());

		if (FAILED(hr))
			return hr;

		DWORD dwStreamId = 0;
		hr = psd->GetStreamIdentifier(&dwStreamId);
		if (FAILED(hr))
			return hr;

		auto stream = HDMediaStream::CreateMediaStream((int)dwStreamId,this,psd.Get());
		if (stream == nullptr)
			return E_OUTOFMEMORY;

		stream->SetQueueSize(STREAM_QUEUE_SIZE_DEFAULT); //default.

		hr = stream->Initialize();
		if (FAILED(hr))
			return hr;

		hr = _streamList.Add(stream.Get());
		if (FAILED(hr))
			return hr;

		if (stream->IsH264Stream() || stream->IsHEVCStream())
			SetMediaStreamPrivateDataFromBlob(stream.Get());
		if (stream->IsFLACStream())
			SetMediaStreamPrivateDataFromBlob(stream.Get(),MF_MT_USER_DATA);
		if (stream->IsFLACStream())
			stream->SetDiscontinuity();
	}

	DWORD dwQueueSize = 0;

	DWORD sid = 0;
	UINT width = 0,height = 0;
	float fps = 0.0f;
	if (SUCCEEDED(FindBestVideoStreamIndex(_pPresentationDescriptor.Get(),&sid,&width,&height,&fps)))
	{
		if (width <= 848) //480p
			dwQueueSize = STREAM_QUEUE_SIZE_480P;
		else if (width <= 1280) //720p
			dwQueueSize = STREAM_QUEUE_SIZE_720P;
		else if (width <= 1920) //1080p
			dwQueueSize = STREAM_QUEUE_SIZE_1080P;
		else if (width > 1920) //4K
			dwQueueSize = STREAM_QUEUE_SIZE_4K;

		if (_network_mode)
			dwQueueSize *= 2;

		if (fps > 120.0f)
			fps = 60.0f;

		if (_forceQueueSize == MF_SOURCE_STREAM_QUEUE_FPS_AUTO && fps > 3)
			dwQueueSize = SetupVideoQueueSize(fps,width,height);

		//如果外部强制设置大小
		if (GlobalOptionGetUINT32(kCoreForceBufferSamples) > 0)
			dwQueueSize = GlobalOptionGetUINT32(kCoreForceBufferSamples);

		for (unsigned i = 0;i < _streamList.Count();i++)
		{
			_streamList[i]->SetQueueSize(dwQueueSize);
			auto mediaType = _streamList[i]->GetMediaType();
			if (SUCCEEDED(WMF::Misc::IsAudioMediaType(mediaType)))
			{
				//WV和TTA的帧都很长，WV半秒，TTA一秒，队列放小小。
				GUID subType = GUID_NULL;
				mediaType->GetGUID(MF_MT_SUBTYPE,&subType);
				if (subType == MFAudioFormat_WV4)
					_streamList[i]->SetQueueSize(7);
				else if (subType == MFAudioFormat_TTA1)
					_streamList[i]->SetQueueSize(3);
				else if (subType == MFAudioFormat_FLAC_FRAMED && dwQueueSize > 4)
					_streamList[i]->SetQueueSize(dwQueueSize / 2);
			}
		}
	}else{
		//Audio Stream Only.
		DWORD dwStreamCount = 0;
		_pPresentationDescriptor->GetStreamDescriptorCount(&dwStreamCount);
		if (dwStreamCount == 1)
		{
			ComPtr<IMFMediaType> pMediaType;
			if (SUCCEEDED(WMF::Misc::GetMediaTypeFromPD(_pPresentationDescriptor.Get(),0,pMediaType.GetAddressOf())))
			{
				if (SUCCEEDED(WMF::Misc::IsAudioMediaType(pMediaType.Get())))
				{
					for (unsigned i = 0;i < _streamList.Count();i++)
						_streamList[i]->SetQueueSize(24);
				}

				if (SUCCEEDED(WMF::Misc::IsAudioPCMSubType(pMediaType.Get())))
				{
					dwQueueSize = SetupAudioQueueSize(pMediaType.Get());

					for (unsigned i = 0;i < _streamList.Count();i++)
						_streamList[i]->SetQueueSize(dwQueueSize);
				}
			}
		}
	}

	return S_OK;
}