#include "HDMediaSource.h"
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#include <Shlwapi.h>
extern HMODULE khInstance;
#endif

#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)) && defined(_SYENGINE_DEMUX)
#define _SYENGINE_STATIC_DEMUX_MODULE
#endif

#ifdef _SYENGINE_STATIC_DEMUX_MODULE
IAVMediaFormat* CreateFLVMediaDemuxer();
std::shared_ptr<IAVMediaFormat> CreateFLVMediaDemuxerSP();
bool CheckFileStreamFLV(IAVMediaIO* pIo);

IAVMediaFormat* CreateMKVMediaDemuxer();
std::shared_ptr<IAVMediaFormat> CreateMKVMediaDemuxerSP();
bool CheckFileStreamMKV(IAVMediaIO* pIo);

IAVMediaFormat* CreateMP4MediaDemuxer();
std::shared_ptr<IAVMediaFormat> CreateMP4MediaDemuxerSP();
bool CheckFileStreamMP4(IAVMediaIO* pIo);
#endif

HRESULT GetExeModulePath(LPWSTR lpstrPath,HMODULE hModule);

static unsigned GetAllStreamsBitrate(IAVMediaFormat* parser,bool forceAllStream = true)
{
	//forceAllStream标识是否要求全部流都具有bitrate
	if (parser == nullptr)
		return 0;
	if (parser->GetStreamCount() <= 0)
		return 0;
	
	unsigned result = 0;
	for (int i = 0;i < parser->GetStreamCount();i++)
	{
		auto stream = parser->GetStream(i);
		if (stream == nullptr)
			continue;

		auto audio = stream->GetAudioInfo();
		auto video = stream->GetVideoInfo();
		int br = 0;
		if (audio == nullptr && video == nullptr)
			continue;

		if (audio != nullptr)
		{
			AudioBasicDescription desc = {};
			if (audio->GetAudioDescription(&desc))
				br = desc.bitrate;
			if (br == 0 && desc.wav_avg_bytes > 0)
				br = desc.wav_avg_bytes * 8;
		}else if (video != nullptr)
		{
			VideoBasicDescription desc = {};
			if (video->GetVideoDescription(&desc))
				br = desc.bitrate;
		}
		if (br == 0)
			br = stream->GetBitrate();

		if (forceAllStream && br == 0)
			return 0;
		result += br;
	}
	return result;
}

static bool IsByteStreamFromNetwork(IMFByteStream* pByteStream)
{
	/*
	DWORD dwStreamCaps = 0;
	pByteStream->GetCapabilities(&dwStreamCaps);
	if ((dwStreamCaps & MFBYTESTREAM_IS_REMOTE) == 0)
		return false;
	*/
	ComPtr<IMFAttributes> pAttrs;
	if (SUCCEEDED(pByteStream->QueryInterface(IID_PPV_ARGS(&pAttrs))))
		if (MFGetAttributeUINT32(pAttrs.Get(),MF_BYTESTREAM_TRANSCODED,0) == 1234)
			return true;

	LPWSTR mimeType = NULL;
	pAttrs->GetAllocatedString(MF_BYTESTREAM_CONTENT_TYPE,&mimeType,NULL);
	if (mimeType)
	{
		bool ret = (wcsicmp(mimeType,L"video/force-network-stream") == 0);
		CoTaskMemFree(mimeType);
		if (ret)
			return true;
	}

	ComPtr<IMFByteStreamBuffering> pBuffer;
	ComPtr<IMFMediaEventGenerator> pStreamEvent;

	HRESULT hr = pByteStream->QueryInterface(IID_PPV_ARGS(pBuffer.GetAddressOf()));
	if (FAILED(hr))
		return false;

	hr = pByteStream->QueryInterface(IID_PPV_ARGS(pStreamEvent.GetAddressOf()));
	if (FAILED(hr))
		return false;

	return true;
}

static HRESULT SetupNetworkStreamBufferTime(double bufTimeSec,double totalTimeSec,int bitratePerSec,QWORD totalFileSize,IMFByteStreamBuffering* pBuffer)
{
	if (bitratePerSec == 0 || bufTimeSec == 0.0)
		return E_FAIL;

	double durationFromBitrate = 0.0;
	if (totalFileSize > 0)
		durationFromBitrate = (double)totalFileSize / ((double)bitratePerSec / 8);

	QWORD fileSizeFromBitrate = 0;
	if (totalTimeSec > 0.0)
		fileSizeFromBitrate = (QWORD)(totalTimeSec * ((double)bitratePerSec / 8));

	double duration = totalTimeSec;
	if (duration > durationFromBitrate) //FLV后黑的情况
		duration = durationFromBitrate;
	QWORD fileSize = totalFileSize;
	if (fileSize == 0)
		fileSize = fileSizeFromBitrate;

	MFBYTESTREAM_BUFFERING_PARAMS params = {};
	params.cbTotalFileSize = params.cbPlayableDataSize = fileSize;
	params.qwNetBufferingTime = params.qwExtraBufferingTimeDuringSeek = 0;
	params.dRate = 1.0f;
	params.qwPlayDuration = (QWORD)(duration * 10000000.0);

	MF_LEAKY_BUCKET_PAIR bucket;
	bucket.dwBitrate = bitratePerSec;
	bucket.msBufferWindow = (DWORD)(bufTimeSec * 1000.0);

	params.cBuckets = 1;
	params.prgBuckets = &bucket;

	return pBuffer->SetBufferingParams(&params);
}

/*
static void SetupNetworkByteStreamWithBufTime(double sec,double duration,IMFByteStream* pByteStream)
{
	if (sec == 0 || duration == 0)
		return;

	ComPtr<IMFByteStreamBuffering> pBuffer;
	if (FAILED(pByteStream->QueryInterface(pBuffer.GetAddressOf())))
		return;

	MFBYTESTREAM_BUFFERING_PARAMS params = {};
	QWORD qwFileSize = pByteStream->GetLength(&qwFileSize);
	if (qwFileSize == 0)
		return;

	params.cbTotalFileSize = qwFileSize;
	params.cbPlayableDataSize = qwFileSize;
	params.dRate = 1.0f;
	params.qwExtraBufferingTimeDuringSeek = params.qwNetBufferingTime = 0;
	params.qwPlayDuration = (QWORD)(duration * 10000000);

	MF_LEAKY_BUCKET_PAIR bucket;
	bucket.msBufferWindow = (DWORD)(sec * 1000);
	bucket.dwBitrate = DWORD(double(qwFileSize * 8) / duration);
}
*/

HRESULT HDMediaSource::OpenAsync(IMFByteStream* pByteStream,IMFAsyncCallback* pCallback) throw()
{
	if (pByteStream == nullptr ||
		pCallback == nullptr)
		return E_INVALIDARG;

	WCHAR szFile[MAX_PATH] = {};
	ComPtr<IMFAttributes> pAttrs;
	if (SUCCEEDED(pByteStream->QueryInterface(IID_PPV_ARGS(pAttrs.GetAddressOf()))))
	{
		pAttrs->GetString(MF_BYTESTREAM_ORIGIN_NAME,szFile,ARRAYSIZE(szFile),nullptr);
		if (szFile[0] != 0)
			DbgLogPrintf(szFile);
	}

	_url_type = HandlerTypes::Default;
	//Check handler-types.
	if (pAttrs != nullptr)
	{
		WCHAR ctype[128] = {};
		pAttrs->GetString(MF_BYTESTREAM_CONTENT_TYPE,ctype,_countof(ctype),nullptr);
		if (wcsicmp(ctype,L"my-flv-rtmp") == 0)
			_url_type = HandlerTypes::RTMP;
		else if (wcsicmp(ctype,L"my-ts-hls") == 0)
			_url_type = HandlerTypes::HLS;
		else if (wcsicmp(ctype,L"my-mp4-dash") == 0)
			_url_type = HandlerTypes::DASH;
	}

	DbgLogPrintf(L"%s::OpenAsync...",L"HDMediaSource");

	if (IsByteStreamFromNetwork(pByteStream))
	{
		QWORD qwTotalDataSize = 0;
		pByteStream->GetLength(&qwTotalDataSize);

		DbgLogPrintf(L"%s::Open Network ByteStream... (%d)",L"HDMediaSource",(DWORD)qwTotalDataSize);

		ComPtr<IMFByteStreamBuffering> pBuffering;
		pByteStream->QueryInterface(IID_PPV_ARGS(pBuffering.GetAddressOf()));
		if (pBuffering != nullptr)
			pBuffering->EnableBuffering(TRUE);

		_network_mode = true;
		_network_delay = 1;
	}
	if (GlobalOptionGetBOOL(kCoreForceUseNetowrkMode))
		_network_mode = true;

	CritSec::AutoLock lock(_cs);

	if (_state != STATE_INVALID)
		return MF_E_INVALIDREQUEST;

	HRESULT hr = MFCreateAsyncResult(nullptr,pCallback,nullptr,_pOpenResult.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = MFCreateEventQueue(_pEventQueue.GetAddressOf());
	if (FAILED(hr))
		return hr;

	DWORD dwStreamCaps = 0;
	pByteStream->GetCapabilities(&dwStreamCaps);
	if ((dwStreamCaps & MFBYTESTREAM_IS_READABLE) == 0)
		return MF_E_UNSUPPORTED_BYTESTREAM_TYPE;
	if ((dwStreamCaps & MFBYTESTREAM_IS_SEEKABLE) == 0 &&
		_url_type == HandlerTypes::Default)
		return MF_E_UNSUPPORTED_BYTESTREAM_TYPE;

	pByteStream->AddRef();
	_pByteStream.Attach(pByteStream);
	_pByteStream.As(&_pByteStreamTimeSeek);

	_state = STATE_OPENING;

	DbgLogPrintf(L"%s::OpenAsync OK.",L"HDMediaSource");

	return QueueAsyncOperation(SourceOperation::OP_OPEN);
}

HRESULT HDMediaSource::DoOpen()
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	if (GetCurrentThreadId() != _dwWorkThreadId)
	{
		_dwWorkThreadId = GetCurrentThreadId();
		SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
	}
#endif

	DbgLogPrintf(L"%s::DoOpen...",L"HDMediaSource");

	_pChapters = nullptr;
	_pMetadata = nullptr;

	if (_state != STATE_OPENING)
		return MF_E_INVALID_STATE_TRANSITION;

	bool pAttrsValid = false;
	ComPtr<IMFAttributes> pAttrs;
	if (SUCCEEDED(_pByteStream.As(&pAttrs))) {
		pAttrsValid = true;
	}

#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
	GlobalSettings->SetUINT32(kNetworkForceUseSyncIO,TRUE);

	if (pAttrsValid && MFGetAttributeUINT32(pAttrs.Get(), MF_BYTESTREAM_TRANSCODED, 0) == 1234) //MultipartStreamMatroska
			GlobalSettings->DeleteItem(kNetworkForceUseSyncIO);

	QWORD temp;
	if (FAILED(_pByteStream->GetLength(&temp))) //live stream.
		GlobalSettings->DeleteItem(kNetworkForceUseSyncIO);
#endif

	IAVMediaIO* pMediaIO = nullptr;
	if (_network_mode && !GlobalOptionGetBOOL(kNetworkForceUseSyncIO)) {
		auto p = new MFMediaIOEx(_pByteStream.Get());
		_pMediaIOEx.Attach(p);
		pMediaIO = static_cast<IAVMediaIO*>(p);
	}else{
		_pMediaIO = std::make_shared<MFMediaIO>(_pByteStream.Get());
		pMediaIO = _pMediaIO.get();
	}

	if (pAttrsValid && MFGetAttributeUINT32(pAttrs.Get(), MF_BYTESTREAM_TRANSCODED, 0) == 1935) // RTMP
		pMediaIO->_IsLiveStream = true;
	
	if (pMediaIO->GetSize() == 0)
		return MF_E_INVALID_STREAM_DATA;

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	WCHAR szDllPath[MAX_PATH] = {};
	GetExeModulePath(szDllPath,khInstance);

	WCHAR szCurPath[MAX_PATH] = {};
	GetCurrentDirectoryW(MAX_PATH,szCurPath);
	SetCurrentDirectoryW(szDllPath);

	PathAppendW(szDllPath,INTEL_QUICKSYNC_DECODER_DLL_NAME);
	if (PathFileExistsW(szDllPath) && !PathIsDirectoryW(szDllPath))
	{
		if (GetModuleHandleW(INTEL_QUICKSYNC_DECODER_DLL_NAME) == nullptr)
			LoadLibraryW(szDllPath);
		if (GetModuleHandleW(INTEL_QUICKSYNC_DECODER_DLL_NAME) != nullptr)
		{
			PathRenameExtensionW(szDllPath,L".ok");
			_intelQSDecoder_found = PathFileExistsW(szDllPath) ? true:false;
#ifdef _DEBUG
			if (_intelQSDecoder_found)
			{
				GetModuleFileNameW(nullptr,szDllPath,ARRAYSIZE(szDllPath));
				PathStripPathW(szDllPath);
				if ((lstrcmpiW(szDllPath,L"HDCorePlayer.exe") != 0) &&
					(lstrcmpiW(szDllPath,L"topoEdit.exe") != 0))
					_intelQSDecoder_found = false;
			}
#endif
		}
	}
#endif

    std::shared_ptr<IAVMediaFormat> parser;
    _pMediaParser.reset();

#ifndef _SYENGINE_STATIC_DEMUX_MODULE
	if (_hCurrentDemuxMod)
		FreeLibrary(_hCurrentDemuxMod);
	_hCurrentDemuxMod = nullptr;

	auto factory = AVDemuxerFactory::CreateFactory();
#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
	if (!factory->NewComponent(L"AudioCoreSource",
		"CheckFileStream","CreateAudioCoreSource",true))
		return E_FAIL;
#endif

	if (!GlobalOptionGetBOOL(kCoreUseOnlyFFmpegDemuxer))
	{
#ifndef _SYENGINE_DEMUX
		if (!factory->NewComponent(L"FLVDemuxer",
			"CheckFileStreamFLV","CreateFLVMediaDemuxerSP",true))
			return E_FAIL;
		if (!factory->NewComponent(L"MKVDemuxer",
			"CheckFileStreamMKV","CreateMKVMediaDemuxerSP",true))
			return E_FAIL;
		if (!factory->NewComponent(L"MP4Demuxer",
			"CheckFileStreamMP4","CreateMP4MediaDemuxerSP",true))
			return E_FAIL;
		if (!factory->NewComponent(L"TSDemuxer",
			"CheckFileStreamTS","CreateTSMediaDemuxerSP",true))
			return E_FAIL;
		if (!factory->NewComponent(L"PSDemuxer",
			"CheckFileStreamPS","CreatePSMediaDemuxerSP",true))
			return E_FAIL;
		if (!factory->NewComponent(L"CoreDemuxer",
			"CheckFileStream","CreateMediaDemuxer",true))
			return E_FAIL;
#else
		if (!factory->NewComponent(L"CoreDemuxers","CheckFileStreamFLV","CreateFLVMediaDemuxerSP") ||
			!factory->NewComponent(L"CoreDemuxers","CheckFileStreamMKV","CreateMKVMediaDemuxerSP") ||
			!factory->NewComponent(L"CoreDemuxers","CheckFileStreamMP4","CreateMP4MediaDemuxerSP"))
			return E_FAIL;
#endif
	}

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	if (!factory->NewComponent(L"AudioCoreSource",
		"CheckFileStream","CreateAudioCoreSource",true))
		return E_FAIL;
#endif

	//ffmpeg demuxers
	if (!factory->NewComponent(L"FFDemuxer",
		"CheckFileStream","CreateFFMediaDemuxers",true))
		return E_FAIL;

	bool create_result = false;
	switch (_url_type)
	{
	case RTMP:
		create_result = factory->CreateFromSpecFile(L"FLVDemuxer",parser);
		break;
	case HLS:
		create_result = factory->CreateFromSpecFile(L"NULL",parser);
		break;
	case DASH:
		create_result = factory->CreateFromSpecFile(L"NULL",parser);
		break;
	default:
		create_result = factory->CreateFromIO(pMediaIO,parser);
	}
	if (!create_result)
		return MF_E_INVALID_FILE_FORMAT;
	if (factory->GetCurrent()->mod == nullptr)
		return MF_E_UNEXPECTED;

	_hCurrentDemuxMod = (decltype(_hCurrentDemuxMod))factory->GetCurrent()->mod;
	factory->GetCurrent()->mod = nullptr;
#else
    if (CheckFileStreamFLV(pMediaIO))
        parser = CreateFLVMediaDemuxerSP();
    if (parser == nullptr && pMediaIO->Seek(0,SEEK_SET) && CheckFileStreamMKV(pMediaIO))
        parser = CreateMKVMediaDemuxerSP();
    if (parser == nullptr && pMediaIO->Seek(0,SEEK_SET) && CheckFileStreamMP4(pMediaIO))
        parser = CreateMP4MediaDemuxerSP();

    pMediaIO->Seek(0,SEEK_SET);
    if (parser == nullptr)
        return MF_E_INVALID_FILE_FORMAT;
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	SetCurrentDirectoryW(szCurPath);
#endif

	if ((parser->GetFormatFlags() & MEDIA_FORMAT_CAN_SEEK_ALL) == 0)
		return MF_E_BYTESTREAM_NOT_SEEKABLE;

	unsigned readFlags = 0;
#ifndef _DEBUG
	//skip subtitle track.
	readFlags = MEDIA_FORMAT_READER_SKIP_AV_OUTSIDE;
#endif
	if (GlobalOptionGetBOOL(kNetworkMatroskaDNTParseSeekHead))
	{
		if (_network_mode)
			readFlags |= MEDIA_FORMAT_READER_MATROSKA_DNT_PARSE_SEEKHEAD;
	}
	if (GlobalOptionGetBOOL(kSubtitleMatroskaASS2SRT))
		readFlags |= MEDIA_FORMAT_READER_MATROSKA_ASS2SRT;

	if (GlobalOptionGetBOOL(kCoreForceSoftwareDecode)) //software decode submit raw sample data.
		readFlags |= MEDIA_FORMAT_READER_H264_FORCE_AVC1;

	if (readFlags != 0)
		parser->SetReadFlags(readFlags);
	if (_network_mode) {
		parser->SetIoCacheSize(64 * 1024); //network stream io buf size: 64K
		if (GlobalOptionGetUINT32(kNetworkPacketIOBufSize) > 1024)
			parser->SetIoCacheSize(GlobalOptionGetUINT32(kNetworkPacketIOBufSize));
	}

	auto err = parser->Open(pMediaIO);
	if (AVE_FAILED(err))
		return MF_E_INVALID_STREAM_DATA;

	if (parser->GetStreamCount() <= 0)
		return MF_E_INVALID_STREAM_STATE;

	_pChapters = 
		(decltype(_pChapters))parser->StaticCastToInterface(AV_MEDIA_INTERFACE_ID_CAST_CHAPTER);
	_pMetadata = 
		(decltype(_pMetadata))parser->StaticCastToInterface(AV_MEDIA_INTERFACE_ID_CAST_METADATA);

	_pMediaParser = parser;
	HRESULT hr = InitPresentationDescriptor();
	if (FAILED(hr))
		return hr;

	hr = CreateStreams();
	if (FAILED(hr))
		return hr;

#ifdef _SYENGINE_DEMUX
	hr = CheckDemuxAllow();
	if (FAILED(hr))
		return hr;
#endif

	if (_network_mode)
	{
		//网络模式，设置缓冲大小来自Bitrate。
		ComPtr<IMFByteStreamBuffering> pBuffer;
		if (SUCCEEDED(_pByteStream.As(&pBuffer)))
		{
			hr = pBuffer->EnableBuffering(FALSE);
			int br = GetAllStreamsBitrate(parser.get());
			if (br > 8)
			{
				double duration = parser->GetDuration();
				QWORD qwFileSize = 0;
				_pByteStream->GetLength(&qwFileSize);

				if (GlobalOptionGetDouble(kNetworkByteStreamBufferTime) > 0.1)
				{
					double forceBufTime = GlobalOptionGetDouble(kNetworkByteStreamBufferTime);
					hr = SetupNetworkStreamBufferTime(forceBufTime,duration,br,qwFileSize,pBuffer.Get());
					if (SUCCEEDED(hr))
						hr = pBuffer->EnableBuffering(TRUE);
				}
			}
		}

		if (pMediaIO->GetSize() == -1) {
			_network_live_stream = true;
			_pPresentationDescriptor->DeleteItem(MF_PD_DURATION);
		}
	}

	CompleteOpen(S_OK);
	_state = STATE_STOPPED;

	DbgLogPrintf(L"%s::DoOpen OK.",L"HDMediaSource");

	return S_OK;
}

HRESULT HDMediaSource::CompleteOpen(HRESULT hrStatus)
{
	decltype(_pOpenResult) openResult = _pOpenResult;

	if (FAILED(hrStatus))
		Shutdown();

	HRESULT hr = S_OK;

	if (_state == STATE_OPENING || _state == STATE_SHUTDOWN)
	{
		hr = openResult->SetStatus(hrStatus);
		if (SUCCEEDED(hr))
			hr = MFInvokeCallback(openResult.Get()); //IMFByteStreamHandler->Invoke.

		_pOpenResult.Reset(); //Free Resources.

		if (FAILED(hr))
			DbgLogPrintf(L"%s::DoOpen FAILED 0x%08X.",L"HDMediaSource",hrStatus);
	}

	return hr;
}

HRESULT HDMediaSource::InitMediaType(IAVMediaStream* pAVStream,IMFMediaType** ppMediaType)
{
	HRESULT hr = S_FALSE;
	if (pAVStream->GetMainType() == MediaMainType::MEDIA_MAIN_TYPE_AUDIO)
		hr = CreateAudioMediaType(pAVStream,ppMediaType);
	else if (pAVStream->GetMainType() == MediaMainType::MEDIA_MAIN_TYPE_VIDEO)
		hr = CreateVideoMediaType(pAVStream,ppMediaType);
	else
		hr = MF_E_INVALIDMEDIATYPE;

	if (SUCCEEDED(hr))
	{
		auto str = _pMediaParser->GetMimeType();
		if (strnicmp(str,"audio",5) == 0 ||
			strnicmp(str,"video",5) == 0) {
			str += 6;
			if (*str == 'x')
				str++;
			if (*str == '-')
				str++;

			AutoStrConv strConvert;
			(*ppMediaType)->SetString(MF_MT_CORE_DEMUX_MIMETYPE,strConvert(str));
		}
	}

	if (SUCCEEDED(hr))
	{
		if (pAVStream->GetStreamName())
		{
			AutoStrConv strConvert;
			(*ppMediaType)->SetString(MF_MY_STREAM_NAME,
				strConvert(pAVStream->GetStreamName()));
		}

		(*ppMediaType)->SetUINT32(MF_MY_STREAM_ID,pAVStream->GetStreamIndex());

		//update bitrate from file format record, such as MP4...
		if (MFGetAttributeUINT32(*ppMediaType,MF_MT_AVG_BITRATE,0) == 0)
		{
			if (pAVStream->GetBitrate() > 128)
			{
				(*ppMediaType)->SetUINT32(MF_MT_AVG_BITRATE,pAVStream->GetBitrate());
				if (pAVStream->GetMainType() == MEDIA_MAIN_TYPE_AUDIO)
				{
					if (MFGetAttributeUINT32(*ppMediaType,MF_MT_AUDIO_AVG_BYTES_PER_SECOND,0) == 0)
						(*ppMediaType)->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND,pAVStream->GetBitrate() / 1000 *  125);
				}
			}
		}
	}

	return hr;
}

HRESULT HDMediaSource::InitPresentationDescriptor()
{
	int count = _pMediaParser->GetStreamCount();

	auto ppsd = std::unique_ptr<IMFStreamDescriptor*>(
		new (std::nothrow)IMFStreamDescriptor*[count]);

	if (ppsd.get() == nullptr)
		return E_OUTOFMEMORY;

	HRESULT hr = S_OK;
	int stream_count = 0;

	for (int i = 0;i < count;i++)
	{
		auto pStream = _pMediaParser->GetStream(i);
		if (pStream->GetMainType() != MediaMainType::MEDIA_MAIN_TYPE_AUDIO &&
			pStream->GetMainType() != MediaMainType::MEDIA_MAIN_TYPE_VIDEO)
			continue;

		if (GlobalOptionGetBOOL(kCoreDisableAllVideos))
			if (pStream->GetMainType() != MediaMainType::MEDIA_MAIN_TYPE_AUDIO)
				continue;

		ComPtr<IMFMediaType> pMediaType;
		hr = InitMediaType(pStream,pMediaType.GetAddressOf());
		if (FAILED(hr))
			continue;

#ifdef _USE_DECODE_FILTER
		//初始化软解。。简化的like MFT模型。
		ComPtr<ITransformFilter> pDecodeFilter;
		bool bUseDecodeFilter = false;
		{
			ComPtr<IUnknown> pFilter;
			if ((GlobalOptionGetBOOL(kCoreForceSoftwareDecode) || _full_sw_decode) &&
				SUCCEEDED(CreateAVCodecTransformFilter(&pFilter))) {
				ComPtr<ITransformLoader> pLoader;
				((ITransformFilter*)pFilter.Get())->GetService(IID_PPV_ARGS(&pLoader));
				if (pLoader)
				{
					if (SUCCEEDED(pLoader->CheckMediaType(pMediaType.Get())) &&
						SUCCEEDED(pLoader->SetInputMediaType(pMediaType.Get()))) {
						ComPtr<IMFMediaType> pNewMediaType;
						if (SUCCEEDED(pLoader->GetOutputMediaType(&pNewMediaType)) && pNewMediaType) {
							GUID majorType = GUID_NULL, subType = GUID_NULL;
							pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
							pMediaType->GetGUID(MF_MT_SUBTYPE,&subType);
							pNewMediaType->SetGUID(MF_MT_MY_TRANSFORM_FILTER_RAWTYPE,subType);
							pNewMediaType->SetUINT32(MF_MY_STREAM_ID,pStream->GetStreamIndex());
							if (pStream->GetStreamName()) {
								AutoStrConv strConvert;
								pNewMediaType->SetString(MF_MY_STREAM_NAME,
									strConvert(pStream->GetStreamName()));
							}
							UINT32 temp = 0;
							temp = MFGetAttributeUINT32(pMediaType.Get(),MF_MT_AVG_BITRATE,0);
							if (temp > 0)
								pNewMediaType->SetUINT32(MF_MT_AVG_BITRATE,temp);
							temp = MFGetAttributeUINT32(pMediaType.Get(),MF_MT_VIDEO_ROTATION,0);
							if (temp > 0 && temp < 360)
								pNewMediaType->SetUINT32(MF_MT_AVG_BITRATE,temp);

							if (majorType == MFMediaType_Video &&
								subType != MFVideoFormat_AVC1 && subType != MFVideoFormat_HVC1) {
								PBYTE userdata = NULL;
								UINT32 udSize = 0;
								if (SUCCEEDED(pMediaType->GetAllocatedBlob(MF_MT_MPEG_SEQUENCE_HEADER,&userdata,&udSize)))
									pNewMediaType->SetBlob(MF_MT_MPEG_SEQUENCE_HEADER,userdata,udSize);
								else if (SUCCEEDED(pMediaType->GetAllocatedBlob(MF_MT_USER_DATA,&userdata,&udSize)))
									pNewMediaType->SetBlob(MF_MT_USER_DATA,userdata,udSize);
								if (userdata)
									CoTaskMemFree(userdata);
							}

							pMediaType = pNewMediaType;
							pFilter.As(&pDecodeFilter);
							bUseDecodeFilter = true;
						}
					}
				}
			}
		}
#endif

		ComPtr<IMFStreamDescriptor> psd;
		hr = MFCreateStreamDescriptor(pStream->GetStreamIndex(),
			1,pMediaType.GetAddressOf(),psd.GetAddressOf());
		if (FAILED(hr))
			break;
		
		ComPtr<IMFMediaTypeHandler> pHandler;
		hr = psd->GetMediaTypeHandler(pHandler.GetAddressOf());
		if (FAILED(hr))
			break;

		hr = pHandler->SetCurrentMediaType(pMediaType.Get());
		if (FAILED(hr))
			break;

#ifdef _USE_DECODE_FILTER
		if (bUseDecodeFilter) {
			psd->SetUINT32(MF_MT_MY_TRANSFORM_FILTER_ALLOW,TRUE);
			psd->SetUnknown(MF_MT_MY_TRANSFORM_FILTER_INTERFACE,pDecodeFilter.Get());
		}
#endif

		psd->SetString(MF_SD_LANGUAGE,L"und");

		if (pStream->GetStreamName())
			psd->SetString(MF_SD_STREAM_NAME,
			AutoStrConv(pStream->GetStreamName(),CP_UTF8).GetResultW());

		if (pStream->GetLanguageName())
			psd->SetString(MF_SD_LANGUAGE,
			AutoStrConv(pStream->GetLanguageName()).GetResultW());

		ppsd.get()[stream_count] = psd.Detach();
		stream_count++;
	}

	if (FAILED(hr))
	{
		for (int i = 0;i < stream_count;i++)
			ppsd.get()[i]->Release();

		return hr;
	}

	hr = MFCreatePresentationDescriptor(stream_count,
		ppsd.get(),_pPresentationDescriptor.GetAddressOf());

	for (int i = 0;i < stream_count;i++)
		ppsd.get()[i]->Release();

	if (FAILED(hr))
		return hr;

	if (_pMediaIO)
		hr = _pPresentationDescriptor->SetUINT64(MF_PD_TOTAL_FILE_SIZE,_pMediaIO->GetSize());

	if (_pMediaParser->GetDuration() > 0.0)
		hr = _pPresentationDescriptor->SetUINT64(MF_PD_DURATION,(UINT64)(_pMediaParser->GetDuration() * 10000000));

	if (_pMediaParser->GetDuration() == 0.0)
	{
		//如果没有时间信息，尝试从ByteStream拿。
		ComPtr<IMFAttributes> pAttrs;
		if (SUCCEEDED(_pByteStream.As(&pAttrs)))
		{
			UINT64 duration = 0;
			pAttrs->GetUINT64(MF_BYTESTREAM_DURATION,&duration);
			if (duration != 0)
				_pPresentationDescriptor->SetUINT64(MF_PD_DURATION,duration);
		}
	}

	if (_pMediaParser->GetBitRate() > 0)
		hr = _pPresentationDescriptor->SetUINT32(MF_PD_VIDEO_ENCODING_BITRATE,
		_pMediaParser->GetBitRate());

	if (_pMediaParser->GetMimeType())
		hr = _pPresentationDescriptor->SetString(MF_PD_MIME_TYPE,
			AutoStrConv(_pMediaParser->GetMimeType()).GetResultW());

	if (_pMediaParser->GetFormatName())
		hr = _pPresentationDescriptor->SetString(MF_MY_PD_MEDIA_DESC,
			AutoStrConv(_pMediaParser->GetFormatName(),CP_UTF8).GetResultW());

	_sampleStartTime = _pMediaParser->GetStartTime();

	//Windows Phone的bug，Source不能选择大于4个流，不然会报错
	hr = _pPresentationDescriptor->SelectStream(0);
	if (stream_count > 1)
	{
		hr = _pPresentationDescriptor->SelectStream(1);
		DWORD dwBestAudioStreamIndex = ObtainBestAudioStreamIndex(_pPresentationDescriptor.Get());
		if (dwBestAudioStreamIndex > 0 && dwBestAudioStreamIndex != 0xFF)
		{
			_pPresentationDescriptor->DeselectStream(1);
			_pPresentationDescriptor->SelectStream(dwBestAudioStreamIndex);
		}
	}

	if (GlobalOptionGetBOOL(kCoreUseFirstAudioStreamOnly))
	{
		bool selectOK = false;
		DWORD dwStreamCount = 0;
		_pPresentationDescriptor->GetStreamDescriptorCount(&dwStreamCount);
		for (unsigned i = 0;i < dwStreamCount;i++)
		{
			_pPresentationDescriptor->DeselectStream(i);
			ComPtr<IMFMediaType> pMediaType;
			if (SUCCEEDED(WMF::Misc::GetMediaTypeFromPD(_pPresentationDescriptor.Get(),i,&pMediaType)))
			{
				if (SUCCEEDED(WMF::Misc::IsAudioMediaType(pMediaType.Get())))
				{
					if (!selectOK)
					{
						selectOK = true;
						_pPresentationDescriptor->SelectStream(i);
					}
				}
			}
		}
	}

#if WINAPI_PARTITION_PHONE_APP
	//TODO...
#endif

	return hr;
}

HRESULT HDMediaSource::CheckDemuxAllow()
{
	//Load CoreDemuxers only.
	auto type = _pMediaParser->GetMimeType();
	if (type) {
		if (strstr(type, "mp4") == NULL &&
			strstr(type, "flv") == NULL &&
			strstr(type, "matroska") == NULL)
			return E_ABORT;
		if (_pMediaParser->StaticCastToInterface(AV_MEDIA_INTERFACE_ID_CASE_EX) == NULL)
			return E_ABORT;
	}
	return S_OK;
}