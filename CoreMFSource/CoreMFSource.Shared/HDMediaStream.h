/*
 - AVMediaFormat Demuxer Media Foundation Wrapper -
   - Author: ShanYe (K.F Yang)

   - Module: HDMediaStream
   - Description: Support AVMediaFormat in a MFSource.
*/

#pragma once

#include "pch.h"
#include "HDMediaSource.h"

enum PCMFormat
{
	PCM_8BIT,
	PCM_LE,
	PCM_BE,
	PCM_FLOAT
};

enum MediaStreamType
{
	MediaStream_Unknown,
	MediaStream_Audio,
	MediaStream_Video,
	MediaStream_Subtitle
};

class HDMediaStream WrlSealed : public IMFMediaStream
{
	ULONG RefCount = 0;

public:
	static ComPtr<HDMediaStream> CreateMediaStream(int index,HDMediaSource* pMediaSource,IMFStreamDescriptor* pStreamDesc);

private:
	HDMediaStream(int index,HDMediaSource* pMediaSource,IMFStreamDescriptor* pStreamDesc);
#if _MSC_VER > 1800
public: //VS2015, Wrl ComPtr.
#endif
	virtual ~HDMediaStream() { DbgLogPrintf(L"%s::Deleted.",L"HDMediaStream"); }

public: //IUnknown
	STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&RefCount); }
	STDMETHODIMP_(ULONG) Release()
	{ ULONG rc = InterlockedDecrement(&RefCount); if (rc == 0) delete this; return rc; }
	STDMETHODIMP QueryInterface(REFIID iid,void** ppv);

public: //IMFMediaEventGenerator & IMFMediaStream
	STDMETHODIMP BeginGetEvent(IMFAsyncCallback *pCallback,IUnknown *punkState);
	STDMETHODIMP EndGetEvent(IMFAsyncResult *pResult,IMFMediaEvent **ppEvent);
	STDMETHODIMP GetEvent(DWORD dwFlags,IMFMediaEvent **ppEvent);
	STDMETHODIMP QueueEvent(MediaEventType met,REFGUID guidExtendedType,HRESULT hrStatus,const PROPVARIANT *pvValue);

	STDMETHODIMP GetMediaSource(IMFMediaSource **ppMediaSource);
	STDMETHODIMP GetStreamDescriptor(IMFStreamDescriptor **ppStreamDescriptor);
	STDMETHODIMP RequestSample(IUnknown *pToken);

public:
	HRESULT Initialize();
	void Activate(bool fActive);

	HRESULT Start(const PROPVARIANT& var,bool seek);
	HRESULT Pause();
	HRESULT Stop();
	HRESULT EndOfStream();
	HRESULT Shutdown();

	void ClearQueue() throw();
	bool IsQueueEmpty() const throw();
	bool NeedsData() throw();
	HRESULT SubmitSample(IMFSample* pSample) throw();

	unsigned QueryNetworkBufferProgressValue() throw();

	unsigned GetSystemRequestCount() const throw() { return _requests.GetCount(); }
	unsigned GetSampleQueueCount() const throw() { return _samples.GetCount(); }
	double GetSampleQueueFirstTime() throw();
	double GetSampleQueueLastTime() throw();

	void QueueTickEvent(LONG64 time) throw();

	inline bool IsActive() const throw() { return _active; }
	inline bool IsEndOfStream() const throw() { return (_samples.IsEmpty() && _eos); }
	inline int GetStreamIndex() const throw() { return _index; }
	inline MediaStreamType GetStreamType() const throw() { return _type; }
	
	inline bool IsH264Stream()  const throw() { return _h264; }
	inline bool IsHEVCStream()  const throw() { return _hevc; }
	inline bool IsFLACStream()  const throw() { return _flac; }

	inline bool IsPCMStream() const throw() { return _pcm; }
	inline PCMFormat GetPCMFormat() const throw() { return _pcm_format; }
	inline unsigned GetPCMSize() const throw() { return _pcm_size; }
	inline unsigned GetPCMPerSecBS() const throw() { return _pcm_presec_bytes; }

	inline void SetQueueSize(DWORD dwQueueSize) throw() { _dwQueueSize = dwQueueSize; }
	inline DWORD GetQueueSize() const throw() { return _dwQueueSize; }

	inline void SetPrerollTime(double timeInSeconds = 0.0) throw()
	{ _preroll_time = (LONG64)(timeInSeconds * 10000000.0); }
	inline double GetPrerollTime() const throw()
	{ return (double)_preroll_time / 10000000.0; }
	inline void SetDynamicPrerollTime(double timeInSeconds = 0.0) throw()
	{ _preroll_dynamic_time = (LONG64)(timeInSeconds * 10000000.0); }
	inline void ClearDynamicPrerollTime() throw()
	{ _preroll_dynamic_time = 0; }

	inline ComPtrList<IMFSample>* GetSampleList() throw() { return &_samples; }

	inline BOOL SwitchDiscontinuity() throw()
	{
		auto old = _discontinuity;
		_discontinuity = FALSE;

		return old;
	}

	inline void SetDiscontinuity() throw()
	{
		_discontinuity = TRUE;
#ifdef _USE_DECODE_FILTER
		if (_decoder)
			_decoder->ProcessFlush();
#endif
	}

	inline BOOL SwitchOnceState() throw()
	{
		auto old = _once_flag;
		_once_flag = FALSE;
		return old;
	}
	inline void SetOnceState() throw()
	{ _once_flag = TRUE; }

	inline IMFMediaType* GetMediaType()
	{
		//non-AddRef.
		return _CopyMediaType.Get();
	}

#ifdef _USE_DECODE_FILTER
	bool GetTransformFilter(ITransformFilter** ppFilter);
	bool ProcessDirectXManager();
	void OnProcessDirectXManager();
#endif

#ifdef _DEBUG
	void DbgUseRequestSampleTickLog() throw() { _dbgRecordSampleTick = true; }
#endif

public:
	void SetPrivateData(unsigned char* pb,unsigned len);
	inline unsigned char* GetPrivateData() { return _private_data.Get(); }
	inline unsigned GetPrivateDataSize() { return _private_state ? _private_size:0; }
	inline unsigned GetPrivateDataSizeForce() { return _private_size; }

	inline void EnablePrivateData() { _private_state = true; }
	inline void DisablePrivateData() { _private_state = false; }

private:
	inline HRESULT CheckShutdown() const throw()
	{ return _state == STATE_SHUTDOWN ? MF_E_SHUTDOWN:S_OK; }

	inline void MaybeSendNetworkBuffering() throw()
	{ if (_samples.IsEmpty() && !_requests.IsEmpty() &&
		_pMediaSource->IsNetworkMode() &&
		_pMediaSource->IsReadPacketProcessing() &&
		!_pMediaSource->IsBuffering())
		_pMediaSource->StartBuffering(); }

	bool NeedsDataUseNetworkTime() throw();

	void DispatchSamples() throw();
	HRESULT RequestSampleAsync() throw()
	{ return _taskWorkQueue.PutWorkItem(&_taskInvokeCallback,nullptr); }
	HRESULT SendSampleDirect(IUnknown* pToken) throw();
	HRESULT DispatchSamplesAsync();

#ifdef _USE_DECODE_FILTER
	inline void ProcessSampleRequest()
	{ if (!_transform_filter) DispatchSamples();
		else if (!_decode_processing && !_requests.IsEmpty()) RequestSampleAsync(); }
#endif

	HRESULT RequestSampleWithDecode(IUnknown* pToken);
	HRESULT DoSampleDecodeRequests();

	HRESULT OnInvoke(IMFAsyncResult* pAsyncResult)
	{ return _transform_filter ? DoSampleDecodeRequests():DispatchSamplesAsync(); }

private:
	class SourceAutoLock
	{
		HDMediaSource* _source;

	public:
		SourceAutoLock(HDMediaSource* pSource) throw() : _source(pSource)
		{
			if (_source)
			{
				_source->AddRef();
				_source->Lock();
			}
		}

		~SourceAutoLock() throw()
		{
			if (_source)
			{
				_source->Unlock();
				_source->Release();
			}
		}
	};

private:
	MediaSourceState _state;
	MediaStreamType _type;
	BOOL _discontinuity;
	BOOL _once_flag;
	
	bool _h264;
	bool _hevc;
	bool _flac;
	bool _pcm;
	PCMFormat _pcm_format;
	unsigned _pcm_size, _pcm_presec_bytes;

	bool _active;
	bool _eos, _dec_eos;

	bool _transform_filter;
	bool _decode_processing;

	int _index;
	
	LONG64 _preroll_time; //���绺���ʱ��
	LONG64 _preroll_dynamic_time; //seek���ֻ����һ�ε����绺��ʱ��
	DWORD _dwQueueSize; //�����ļ��Ķ��д�С

	ComPtr<HDMediaSource> _pMediaSource;
	ComPtr<IMFStreamDescriptor> _pStreamDesc;
	ComPtr<IMFMediaEventQueue> _pEventQueue;

	ComPtr<IMFMediaType> _CopyMediaType;

	ComPtrList<IMFSample> _samples, _decoded_pending;
	ComPtrList<IUnknown,true> _requests;

	AutoComMem<unsigned char> _private_data;
	unsigned _private_size;
	bool _private_state;

	WMF::AsyncCallback<HDMediaStream> _taskInvokeCallback;
#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
	WMF::AutoWorkQueue _taskWorkQueue;
#else
	WMF::AutoWorkQueueOld<HDMediaStream> _taskWorkQueue;
#endif

#ifdef _DEBUG
	DWORD64 _reqSampleTick;
	int _reqSampleTickTotal;
	int _reqSampleTickAvg;
	int _reqSampleCount;
	bool _dbgRecordSampleTick;
#endif

#ifdef _USE_DECODE_FILTER
	class DXVATransformSampleAllocator :
		public RuntimeClass<RuntimeClassFlags<RuntimeClassType::ClassicCom>,ITransformAllocator> {
		ComPtr<IMFVideoSampleAllocator> _allocator;
	public:
		DXVATransformSampleAllocator(IMFVideoSampleAllocator* allocator)
		{ _allocator = allocator; }
		virtual ~DXVATransformSampleAllocator() {}

		STDMETHODIMP CreateSample(IMFSample** ppSample)
		{ return _allocator->AllocateSample(ppSample); }
		STDMETHODIMP IsUseDXVA(BOOL* bUseDXVA)
		{ if (bUseDXVA) *bUseDXVA = TRUE; return S_OK; }
	};

	ComPtr<ITransformWorker> _decoder;
#endif
};