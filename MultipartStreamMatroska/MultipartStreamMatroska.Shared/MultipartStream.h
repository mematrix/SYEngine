/* ---------------------------------------------------------------
 -
 - ʵ�ֻ�����IMFByteStream�������mkv�ļ�live����
 - �����ڣ�2015-09-21
 - ���ߣ�ShanYe
 --------------------------------------------------------------- */

#pragma once

#include "pch.h"
#include "ThreadImpl.h"
#include "Core\MatroskaJoinStream.h"
#include <thread>

/* Todo List...
 IMFByteStream -> �ṩ��������
 IMFByteStreamTimeSeek -> �ӹ�Seek������
 IMFByteStreamBuffering -> �ṩ������ƣ�������ʵ��IMFMediaEventGenerator��
 IMFByteStreamCacheControl -> �ṩֹͣ����Ĳ�����
 IMFAttributes -> �ṩ���Լ��ϡ�
 IMFGetService -> �ṩ��ѯȡ��IPropertyStore�ķ�ʽ��
 IPropertyStore -> �ṩ���ؽ��ȡ�
*/

class MultipartStream : 
	public IMFByteStream,
	public IMFByteStreamTimeSeek,
	public IMFByteStreamBuffering,
	public IMFByteStreamCacheControl,
	public IMFAttributes,
	public IMFGetService,
	public IPropertyStore,
	protected ThreadImpl,
	protected MatroskaJoinStream {

public:
	explicit MultipartStream(const wchar_t* list) :
		_ref_count(1), _state(Invalid), _flag_eof(false), _flag_user_stop(false),
		_stm_length(0), _stm_cur_pos(0), _header_prue_size(0), _download_progress(100), _closed(true) {
		_update_url_callback = _update_detail_callback = NULL;
		_event_async_read = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
		_event_exit_thr = CreateEventExW(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
		_async_time_seek.eventStarted = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
		_async_time_seek.seekThread = NULL;
		_hDebugFile = INVALID_HANDLE_VALUE;
		wcscpy_s(_list_file, list);
		MFCreateAttributes(&_attrs, 0);
		memset(&_async_read_ps, 0, sizeof(_async_read_ps));
		Module<InProc>::GetModule().IncrementObjectCount();
	}
	virtual ~MultipartStream()
	{
		Close();
		CloseHandle(_event_async_read);
		CloseHandle(_event_exit_thr);
		CloseHandle(_async_time_seek.eventStarted);
		Module<InProc>::GetModule().DecrementObjectCount();
	}

	bool Open(IMFAttributes* config = NULL, bool delete_list_file = false);

public: //IUnknown
	STDMETHODIMP_(ULONG) AddRef()
	{ return InterlockedIncrement(&_ref_count); }
	STDMETHODIMP_(ULONG) Release()
	{ auto rc = InterlockedDecrement(&_ref_count); if (rc == 0) delete this; return rc; }
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);

public: //IMFByteStream
	STDMETHODIMP GetCapabilities(DWORD *pdwCapabilities);
	STDMETHODIMP GetLength(QWORD *pqwLength);
	STDMETHODIMP SetLength(QWORD qwLength) { return E_NOTIMPL; }
	STDMETHODIMP GetCurrentPosition(QWORD *pqwPosition);
	STDMETHODIMP SetCurrentPosition(QWORD qwPosition)
	{ return Seek(msoBegin, (LONG64)qwPosition, 0, NULL); }
	STDMETHODIMP IsEndOfStream(BOOL *pfEndOfStream);

	STDMETHODIMP Read(BYTE *pb, ULONG cb, ULONG *pcbRead);
	STDMETHODIMP BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState);
	STDMETHODIMP EndRead(IMFAsyncResult *pResult, ULONG *pcbRead);

	STDMETHODIMP Write(const BYTE *pb, ULONG cb, ULONG *pcbWritten)
	{ return E_NOTIMPL; }
	STDMETHODIMP BeginWrite(const BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState)
	{ return E_NOTIMPL; }
	STDMETHODIMP EndWrite(IMFAsyncResult *pResult, ULONG *pcbWritten)
	{ return E_NOTIMPL; }

	STDMETHODIMP Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition);
	STDMETHODIMP Flush() { return S_OK; }
	STDMETHODIMP Close();

public: //IMFByteStreamTimeSeek
	STDMETHODIMP IsTimeSeekSupported(BOOL *pfTimeSeekIsSupported);
	STDMETHODIMP TimeSeek(QWORD qwTimePosition);
	STDMETHODIMP GetTimeSeekResult(QWORD *pqwStartTime, QWORD *pqwStopTime, QWORD *pqwDuration)
	{ return E_NOTIMPL; }

public: //IMFByteStreamBuffering
	STDMETHODIMP SetBufferingParams(MFBYTESTREAM_BUFFERING_PARAMS *pParams) { return S_OK; }
	STDMETHODIMP EnableBuffering(BOOL fEnable);
	STDMETHODIMP StopBuffering() { return StopBackgroundTransfer(); }

public: //IMFByteStreamCacheControl
	STDMETHODIMP StopBackgroundTransfer();

public: //IMFAttributes
	STDMETHODIMP GetItem(REFGUID guidKey, PROPVARIANT *pValue)
	{ return _attrs->GetItem(guidKey, pValue); }
	STDMETHODIMP GetItemType(REFGUID guidKey, MF_ATTRIBUTE_TYPE *pType)
	{ return _attrs->GetItemType(guidKey, pType); }
	STDMETHODIMP CompareItem(REFGUID guidKey, REFPROPVARIANT Value, BOOL *pbResult)
	{ return _attrs->CompareItem(guidKey, Value, pbResult); }
	STDMETHODIMP Compare(IMFAttributes *pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL *pbResult)
	{ return _attrs->Compare(pTheirs, MatchType, pbResult); }
	STDMETHODIMP GetUINT32(REFGUID guidKey, UINT32 *punValue)
	{ return _attrs->GetUINT32(guidKey, punValue); }
	STDMETHODIMP GetUINT64(REFGUID guidKey, UINT64 *punValue)
	{ return _attrs->GetUINT64(guidKey, punValue); }
	STDMETHODIMP GetDouble(REFGUID guidKey, double *pfValue)
	{ return _attrs->GetDouble(guidKey, pfValue); }
	STDMETHODIMP GetGUID(REFGUID guidKey, GUID *pguidValue)
	{ return _attrs->GetGUID(guidKey, pguidValue); }
	STDMETHODIMP GetStringLength(REFGUID guidKey, UINT32 *pcchLength)
	{ return _attrs->GetStringLength(guidKey, pcchLength); }
	STDMETHODIMP GetString(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32 *pcchLength)
	{ return _attrs->GetString(guidKey, pwszValue, cchBufSize, pcchLength); }
	STDMETHODIMP GetAllocatedString(REFGUID guidKey, LPWSTR *ppwszValue, UINT32 *pcchLength)
	{ return _attrs->GetAllocatedString(guidKey, ppwszValue, pcchLength); }
	STDMETHODIMP GetBlobSize(REFGUID guidKey, UINT32 *pcbBlobSize)
	{ return _attrs->GetBlobSize(guidKey, pcbBlobSize); }
	STDMETHODIMP GetBlob(REFGUID guidKey, UINT8 *pBuf, UINT32 cbBufSize, UINT32 *pcbBlobSize)
	{ return _attrs->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize); }
	STDMETHODIMP GetAllocatedBlob(REFGUID guidKey, UINT8 **ppBuf, UINT32 *pcbSize)
	{ return _attrs->GetAllocatedBlob(guidKey, ppBuf, pcbSize); }
	STDMETHODIMP GetUnknown(REFGUID guidKey, REFIID riid, LPVOID *ppv)
	{ return _attrs->GetUnknown(guidKey, riid, ppv); }
	STDMETHODIMP SetItem(REFGUID guidKey, REFPROPVARIANT Value)
	{ return _attrs->SetItem(guidKey, Value); }
	STDMETHODIMP DeleteItem(REFGUID guidKey)
	{ return _attrs->DeleteItem(guidKey); }
	STDMETHODIMP DeleteAllItems()
	{ return _attrs->DeleteAllItems(); }
	STDMETHODIMP SetUINT32(REFGUID guidKey, UINT32 unValue)
	{ return _attrs->SetUINT32(guidKey, unValue); }
	STDMETHODIMP SetUINT64(REFGUID guidKey, UINT64 unValue)
	{ return _attrs->SetUINT64(guidKey, unValue); }
	STDMETHODIMP SetDouble(REFGUID guidKey, double fValue)
	{ return _attrs->SetDouble(guidKey, fValue); }
	STDMETHODIMP SetGUID(REFGUID guidKey, REFGUID guidValue)
	{ return _attrs->SetGUID(guidKey, guidValue); }
	STDMETHODIMP SetString(REFGUID guidKey, LPCWSTR wszValue)
	{ return _attrs->SetString(guidKey, wszValue); }
	STDMETHODIMP SetBlob(REFGUID guidKey, const UINT8 *pBuf, UINT32 cbBufSize)
	{ return _attrs->SetBlob(guidKey, pBuf, cbBufSize); }
	STDMETHODIMP SetUnknown(REFGUID guidKey, IUnknown *pUnknown)
	{ return _attrs->SetUnknown(guidKey, pUnknown); }
	STDMETHODIMP LockStore()
	{ return _attrs->LockStore(); }
	STDMETHODIMP UnlockStore()
	{ return _attrs->UnlockStore(); }
	STDMETHODIMP GetCount(UINT32 *pcItems)
	{ return _attrs->GetCount(pcItems); }
	STDMETHODIMP GetItemByIndex(UINT32 unIndex, GUID *pguidKey, PROPVARIANT *pValue)
	{ return _attrs->GetItemByIndex(unIndex, pguidKey, pValue); }
	STDMETHODIMP CopyAllItems(IMFAttributes *pDest)
	{ return _attrs->CopyAllItems(pDest); }

public: //IMFGetService
	STDMETHODIMP GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

public: //IPropertyStore
	STDMETHODIMP GetCount(DWORD* cProps);
	STDMETHODIMP GetAt(DWORD iProp, PROPERTYKEY *pkey);
	STDMETHODIMP GetValue(REFPROPERTYKEY key, PROPVARIANT *pv);
	STDMETHODIMP SetValue(REFPROPERTYKEY key, REFPROPVARIANT propvar)
	{ return E_NOTIMPL; }
	STDMETHODIMP Commit()
	{ return E_NOTIMPL; }

protected:
	virtual void ThreadInvoke(void*);
	virtual void ThreadEnded() { Release(); }
	enum CurrentState
	{
		Invalid,
		AfterOpen,
		AfterSeek
	};
	CurrentState _state;

	virtual bool OnPartInit() { TryUpdateItemUrl(0, "OPEN"); return true; }
	virtual bool OnPartError(unsigned index, bool reconnect)
	{ return TryUpdateItemUrl(index, reconnect ? "RECONNECT" : "ERROR"); }

private:
	unsigned InternalGetStreamSize();
	double InternalGetStreamDuration();

	unsigned ReadFile(PBYTE pb, unsigned size);
	bool SeekTo(double time);
	bool Reset(bool timeseek);

	bool WaitTimeSeekResult();
	void DoTimeSeekAsync(double time);

	void InitUpdateUrlCb(IMFAttributes* cfg);
	bool TryUpdateItemUrl(int index, const char* type);

private:
	ULONG _ref_count; //COM���ü���
	std::recursive_mutex _mutex; //��ȫͬ��lock
	ComPtr<IMFAttributes> _attrs; //���Ե�thunk

	wchar_t _list_file[MAX_PATH]; //��p���ļ��б������Ǳ��ػ������磩
	QWORD _stm_length, _stm_cur_pos; //��׼��fake���������Ⱥ�curpos
	bool _flag_eof, _flag_user_stop;

	MemoryStream _header; //�洢MKVͷ+��һ��Cluster
	unsigned _header_prue_size; //��MKVͷ�Ĵ�С

	void *_update_url_callback, *_update_detail_callback;
	ULONG64 _prev_readfile_tick;

	struct AsyncReadParameters
	{
		PBYTE pb;
		unsigned size;
		IMFAsyncCallback* callback;
		IUnknown* punkState;
		unsigned cbRead;

		void AddRef()
		{
			callback->AddRef();
			if (punkState)
				punkState->AddRef();
		}
		void Release()
		{
			callback->Release();
			if (punkState)
				punkState->Release();
		}
	};
	AsyncReadParameters _async_read_ps; //�첽������
	HANDLE _event_async_read, _event_exit_thr; //�����첽���������˳��¼�

	struct AsyncTimeSeekStatus
	{
		std::thread* seekThread;
		HANDLE eventStarted;
		bool result;
	};
	AsyncTimeSeekStatus _async_time_seek;

	int _download_progress;
	bool _closed;

	HANDLE _hDebugFile;
};