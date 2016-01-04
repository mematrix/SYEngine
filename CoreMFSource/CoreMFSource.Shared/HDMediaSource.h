/*
 - AVMediaFormat Demuxer Media Foundation Wrapper -
   - HDCore Project. (WPlayer Kernel Component)
   - Author: ShanYe (K.F Yang)

   - Module: HDMediaSource
   - Description: Support AVMediaFormat in a MFSource.
*/

#pragma once

#include "pch.h"
#include "DbgLogOutput.h"
#include "GlobalSettings.h"

#include "LinkList.h"
#include "MFMediaIO.h"
#include "MFMediaIOEx.h"
#include "MyMediaTypeDef.h"
#include "Demuxer\DemuxerFactory.h"

#include <MediaAVError.h>
#include <MediaAVIO.h>
#include <MediaAVStream.h>
#include <MediaAVFormat.h>
#include <AutoMediaBuffer.h>
#include <AutoMediaPacket.h>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#include <CritSec.h>
#include <Win32MacroTools.h>

#include <AutoComMem.h>
#include <AutoPropVar.h>
#include <AutoStrConv.h>

#include <MFGenericList.h>
#include <MFGenericQueue.h>
#include <MFAutoBufLock.h>
#include <MFAsyncCalback.h>
#include <MFWorkQueue.h>
#include <MFWorkQueueOld.h>
#include <MFMiscHelp.h>
#else
#include "Common\CritSec.h"
#include "Common\Win32MacroTools.h"

#include "Common\AutoComMem.h"
#include "Common\AutoPropVar.h"
#include "Common\AutoStrConv.h"

#include "Common\MFGenericList.h"
#include "Common\MFGenericQueue.h"
#include "Common\MFAutoBufLock.h"
#include "Common\MFAsyncCalback.h"
#include "Common\MFWorkQueue.h"
#include "Common\MFWorkQueueOld.h"
#include "Common\MFMiscHelp.h"
#endif

#include "MFSeekInfo.hxx"

////////////// Sample Queue Size //////////////
#define STREAM_QUEUE_SIZE_DEFAULT 7

#define STREAM_QUEUE_SIZE_480P 15
#define STREAM_QUEUE_SIZE_720P 30
#define STREAM_QUEUE_SIZE_1080P 50
#define STREAM_QUEUE_SIZE_4K 100
////////////// Sample Queue Size //////////////

#define MF_SOURCE_STREAM_QUEUE_AUTO -1
#define MF_SOURCE_STREAM_QUEUE_FPS_AUTO 0

////////////// SourceOperation Helper Class //////////////
class SourceOperation : public IUnknown
{
	ULONG RefCount = 0;

public:
	enum Operation
	{
		OP_OPEN,
		OP_START,
		OP_PAUSE,
		OP_STOP,
		OP_SETRATE,
		OP_REQUEST_DATA,
		OP_END_OF_STREAM
	};

	static HRESULT CreateOperation(Operation op,SourceOperation** pp);
	static HRESULT CreateStartOperation(IMFPresentationDescriptor* ppd,double seek_time,SourceOperation** pp);
	static HRESULT CreateSetRateOperation(BOOL fThin,FLOAT flRate,SourceOperation** pp);

public: //IUnknown
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	STDMETHODIMP QueryInterface(REFIID iid,void** ppv);

public:
	SourceOperation(Operation op) throw();
	virtual ~SourceOperation() throw();

public:
	Operation GetOperation() const throw() { return _op; }

	int GetMagic() const throw() { return _magic; }
	void SetMagic(int magic) throw() { InterlockedExchange((unsigned*)&_magic,magic); } 

	const PROPVARIANT& GetData() const throw() { return _data; }
	HRESULT SetData(const PROPVARIANT& var) throw();

private:
	int _magic;
	Operation _op;
	PROPVARIANT _data;
};

class StartOperation WrlSealed : public SourceOperation
{
public:
	StartOperation(IMFPresentationDescriptor* ppd,double seek_time) throw();

public:
	HRESULT GetPresentationDescriptor(IMFPresentationDescriptor** pp) const throw();
	double GetSeekToTime() const throw() { return _seek_to_time; }

private:
	ComPtr<IMFPresentationDescriptor> _pd;
	double _seek_to_time; //DoStop in Network mode.
};

class SetRateOperation WrlSealed : public SourceOperation
{
public:
	SetRateOperation(BOOL fThin,FLOAT flRate) throw() : 
		SourceOperation(OP_SETRATE),
		_fThin(fThin), _flRate(flRate) {}

public:
	BOOL IsThin() const throw() { return _fThin; }
	FLOAT GetRate() const throw() { return _flRate; }

private:
	BOOL _fThin;
	FLOAT _flRate;
};
////////////// SourceOperation Helper Class //////////////


static const float MAX_MFSOURCE_RATE_SUPPORT = 1000000.0f;
static const float MIN_MFSOURCE_RATE_SUPPORT = 0.00f;
static const float PEAK_MFSOURCE_RATE_VALUE = 2.0f;

class HDMediaStream;

#define CallStreamMethod(stream,method) \
	if ((stream)->IsActive()) \
	(stream)->method

enum MediaSourceState
{
	STATE_INVALID,
	STATE_OPENING,
	STATE_BUFFERING,
	STATE_STOPPED,
	STATE_PAUSED,
	STATE_STARTED,
	STATE_SHUTDOWN
};

class HDMediaSource WrlSealed : 
	public IMFMediaSource,
	public IMFGetService,
	public IMFRateControl,
	public IMFRateSupport,
	public IMFMetadataProvider,
	public IMFMetadata,
	public IMFSeekInfo,
	public IPropertyStore {
	ULONG RefCount = 0;

public:
	static ComPtr<HDMediaSource> CreateMediaSource(
		int QueueSize = MF_SOURCE_STREAM_QUEUE_AUTO);

private:
	explicit HDMediaSource(int QueueSize);
#if _MSC_VER > 1800
public: //VS2015, Wrl ComPtr.
#endif
	virtual ~HDMediaSource();

public: //IUnknown
	STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&RefCount); }
	STDMETHODIMP_(ULONG) Release()
	{ ULONG rc = InterlockedDecrement(&RefCount); if (rc == 0) delete this; return rc; }
	STDMETHODIMP QueryInterface(REFIID iid,void** ppv);

public: //IMFMediaEventGenerator & IMFMediaSource
	STDMETHODIMP BeginGetEvent(IMFAsyncCallback *pCallback,IUnknown *punkState);
	STDMETHODIMP EndGetEvent(IMFAsyncResult *pResult,IMFMediaEvent **ppEvent);
	STDMETHODIMP GetEvent(DWORD dwFlags,IMFMediaEvent **ppEvent);
	STDMETHODIMP QueueEvent(MediaEventType met,REFGUID guidExtendedType,HRESULT hrStatus,const PROPVARIANT *pvValue);

	STDMETHODIMP CreatePresentationDescriptor(IMFPresentationDescriptor **ppPresentationDescriptor);
	STDMETHODIMP GetCharacteristics(DWORD *pdwCharacteristics);
	STDMETHODIMP Pause();
	STDMETHODIMP Shutdown();
	STDMETHODIMP Start(IMFPresentationDescriptor *pPresentationDescriptor,const GUID *pguidTimeFormat,const PROPVARIANT *pvarStartPosition);
	STDMETHODIMP Stop();

public: //IMFGetService
	STDMETHODIMP GetService(REFGUID guidService,REFIID riid,LPVOID *ppvObject);

public: //IMFSeekInfo
	STDMETHODIMP GetNearestKeyFrames(const GUID *pguidTimeFormat,const PROPVARIANT *pvarStartPosition,PROPVARIANT *pvarPreviousKeyFrame,PROPVARIANT *pvarNextKeyFrame);

public: //IMFRateControl
	STDMETHODIMP SetRate(BOOL fThin,float flRate);
	STDMETHODIMP GetRate(BOOL *pfThin,float *pflRate);
public: //IMFRateSupport
	STDMETHODIMP GetFastestRate(MFRATE_DIRECTION eDirection,BOOL fThin,float *pflRate);
	STDMETHODIMP GetSlowestRate(MFRATE_DIRECTION eDirection,BOOL fThin,float *pflRate);
	STDMETHODIMP IsRateSupported(BOOL fThin,float flRate,float *pflNearestSupportedRate);

public: //IMFMetadataProvider
	STDMETHODIMP GetMFMetadata(IMFPresentationDescriptor* pPresentationDescriptor,DWORD dwStreamIdentifier,DWORD dwFlags,IMFMetadata** ppMFMetadata);
public: //IMFMetadata
	STDMETHODIMP GetLanguage(LPWSTR* ppwszRFC1766) { return E_NOTIMPL; }
	STDMETHODIMP SetLanguage(LPCWSTR pwszRFC1766) { return E_NOTIMPL; }
	STDMETHODIMP GetAllLanguages(PROPVARIANT* ppvLanguages) { return S_OK; }
	STDMETHODIMP GetAllPropertyNames(PROPVARIANT* ppvNames);
	STDMETHODIMP DeleteProperty(LPCWSTR pwszName) { return S_OK; }
	STDMETHODIMP SetProperty(LPCWSTR pwszName,const PROPVARIANT* ppvValue) { return S_OK; }
	STDMETHODIMP GetProperty(LPCWSTR pwszName,PROPVARIANT* ppvValue);

public: //IPropertyStore
	STDMETHODIMP GetCount(DWORD* cProps);
	STDMETHODIMP GetAt(DWORD iProp,PROPERTYKEY *pkey);
	STDMETHODIMP GetValue(REFPROPERTYKEY key,PROPVARIANT *pv);
	STDMETHODIMP SetValue(REFPROPERTYKEY key,REFPROPVARIANT propvar)
	{ return E_NOTIMPL; }
	STDMETHODIMP Commit()
	{ return E_NOTIMPL; }

public:
	void Lock() throw() { _cs.Lock(); }
	void Unlock() throw() { _cs.Unlock(); }

	inline MediaSourceState GetCurrentState() const throw() { return _state; }

	inline bool IsNetworkMode() const throw() { return _network_mode; }
	inline unsigned GetNetworkDelay() const throw() { return _network_delay; }
	inline void SetNetworkPrerollTime(double seconds = 5.0) throw() { _network_preroll_time = seconds; }

	inline bool IsBuffering() const throw() { return _network_buffering; }
	void StartBuffering() { SendNetworkStartBuffering(); }
	void StopBuffering() { SendNetworkStopBuffering(); }

	HRESULT QueueAsyncOperation(SourceOperation::Operation opType) throw();
	HRESULT ProcessOperationError(HRESULT hrStatus) throw();

	HRESULT OpenAsync(IMFByteStream* pByteStream,IMFAsyncCallback* pCallback) throw();

private:
	HRESULT InitMediaType(IAVMediaStream* pAVStream,IMFMediaType** ppMediaType);
	HRESULT InitPresentationDescriptor();
	HRESULT CreateStreams();

	HRESULT CreateAudioMediaType(IAVMediaStream* pAVStream,IMFMediaType** ppMediaType);
	HRESULT CreateVideoMediaType(IAVMediaStream* pAVStream,IMFMediaType** ppMediaType);

	HRESULT InitAudioAC3MediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,bool ec3);
	HRESULT InitAudioDTSMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitAudioTrueHDMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitAudioAACMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,bool adts,bool skip_err = false);
	HRESULT InitAudioFAADMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,bool adts);
	HRESULT InitAudioMP3MediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type);
	HRESULT InitAudioWMAMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type);
	HRESULT InitAudioALACMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitAudioFLACMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitAudioOGGMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitAudioOPUSMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitAudioTTAMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitAudioWV4MediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitAudioBDMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitAudioPCMMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type);
	HRESULT InitAudioAMRMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type);
	HRESULT InitAudioRealCookMediaType(IAudioDescription* pDesc,IMFMediaType* pMediaType);

	HRESULT InitVideoHEVCMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,bool es);
	HRESULT InitVideoH264MediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,bool es);
	HRESULT InitVideoH263MediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitVideoVC1MediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitVideoWMVMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type);
	HRESULT InitVideoVPXMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType codec_type);
	HRESULT InitVideoMPEG2MediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType); //mpeg1 and 2
	HRESULT InitVideoMPEG4MediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitVideoMJPEGMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType);
	HRESULT InitVideoMSMPEGMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType ct);
	HRESULT InitVideoRealMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType ct);
	HRESULT InitVideoQuickSyncMediaType(IVideoDescription* pDesc,IMFMediaType* pMediaType,MediaCodecType ct);

	HRESULT ValidatePresentationDescriptor(IMFPresentationDescriptor* ppd);
	HRESULT SelectStreams(IMFPresentationDescriptor* ppd,const PROPVARIANT& varStartPosition,bool seek,bool sendstart = true);

	HRESULT SetMediaStreamPrivateDataFromBlob(HDMediaStream* pMediaStream,const GUID& key = MF_MT_MPEG_SEQUENCE_HEADER);

	HRESULT FindMediaStreamById(int id,HDMediaStream** ppStream);

	HRESULT FindBestVideoStreamIndex(IMFPresentationDescriptor* ppd,PDWORD pdwStreamId,UINT* width,UINT* height,float* fps);

	DWORD SetupVideoQueueSize(float fps,UINT width,UINT height);
	DWORD SetupAudioQueueSize(IMFMediaType* pAudioMediaType);

private:
	HRESULT PreloadStreamPacket();

	enum QueuePacketResult
	{
		QueuePacketOK,
		QueuePacketNotifyNetwork
	};
	QueuePacketResult QueueStreamPacket();
	bool StreamsNeedData();

	void NotifyParseEnded();

	HRESULT CreateMFSampleFromAVMediaBuffer(AVMediaBuffer* avBuffer,IMFSample** ppSample);
	HRESULT ProcessSampleMerge(unsigned char* pHead,unsigned len,IMFSample* pOldSample,IMFSample** ppNewSample);

	double GetAllStreamMaxQueueTime();
	bool IsNeedNetworkBuffering();
	unsigned QueryNetworkBufferProgressValue();
	bool UpdateNetworkDynamicPrerollTime(double now_pkt_time); //seek after

	HRESULT SendNetworkStartBuffering();
	HRESULT SendNetworkStopBuffering();

private:
	HRESULT CompleteOpen(HRESULT hrStatus);
	HRESULT SeekOpen(LONG64 seekTo);

	HRESULT DoOpen();
	HRESULT DoStart(SourceOperation* op);
	HRESULT DoPause(SourceOperation* op);
	HRESULT DoStop(SourceOperation* op);
	HRESULT DoSetRate(SourceOperation* op);
	HRESULT OnRequestSample(SourceOperation* op);
	HRESULT OnEndOfStream(SourceOperation* op);

private:
	HRESULT QueueOperation(SourceOperation* op);
	HRESULT ValidateOperation(SourceOperation* op);
	HRESULT DispatchOperation(SourceOperation* op);

	HRESULT OnInvoke(IMFAsyncResult* pAsyncResult);

private:
	HRESULT CheckShutdown() const throw()
	{
		return _state == STATE_SHUTDOWN ? MF_E_SHUTDOWN:S_OK;
	}

	HRESULT IsInitialized() const throw()
	{
		return (_state == STATE_OPENING || _state == STATE_INVALID) ? MF_E_NOT_INITIALIZED:S_OK;
	}

	HRESULT MakeKeyFramesIndex() throw();

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
private:
	static bool IsWindows8();
	HRESULT ProcessMPEG2TSToMP4Sample(IMFSample* pH264ES,IMFSample** ppH264);
#endif
	static DWORD ObtainBestAudioStreamIndex(IMFPresentationDescriptor* ppd);

private:
	CritSec _cs;
	MediaSourceState _state;

	int _taskMagicNumber;
	WMF::AsyncCallback<HDMediaSource> _taskInvokeCallback;
#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
	WMF::AutoWorkQueue _taskWorkQueue;
#else
	WMF::AutoWorkQueueOld<HDMediaSource> _taskWorkQueue;
	DWORD _dwWorkThreadId;
#endif
	int _forceQueueSize;

	WMF::ComPtrList<HDMediaStream,FALSE> _streamList;

	double _start_op_seek_time;
	double _sampleStartTime;

	float _currentRate;

	bool _network_mode;
	unsigned _network_delay;
	double _network_preroll_time;
	unsigned _network_buffer_progress;
	bool _network_buffering;
	bool _network_live_stream;

	int _nPendingEOS;
	ComPtr<SourceOperation> _pSampleRequest;

	ComPtr<IMFByteStreamTimeSeek> _pByteStreamTimeSeek;
	ComPtr<IMFByteStream> _pByteStream;
	ComPtr<IMFAsyncResult> _pOpenResult;

	ComPtr<IMFMediaEventQueue> _pEventQueue;
	ComPtr<IMFPresentationDescriptor> _pPresentationDescriptor;

	std::shared_ptr<MFMediaIO> _pMediaIO;
	std::shared_ptr<IAVMediaFormat> _pMediaParser;
	ComPtr<MFMediaIOEx> _pMediaIOEx;

	IAVMediaChapters* _pChapters;
	IAVMediaMetadata* _pMetadata;

	HMODULE _hCurrentDemuxMod;

	bool _seekAfterFlag;
	bool _notifyParserSeekAfterFlag;
	bool _enterReadPacketFlag;

	double _seekToTime;

	double* _key_frames;
	unsigned _key_frames_count;

	bool _enableH264ES2H264; //Windows 7 is not support H264_ES.
	bool _intelQSDecoder_found;

	enum HandlerTypes
	{
		Default,
		RTMP,
		HLS,
		DASH
	};
	HandlerTypes _url_type;
};