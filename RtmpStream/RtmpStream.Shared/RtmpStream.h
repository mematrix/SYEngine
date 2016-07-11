#pragma once

#include <cstdint>
#include <string>
#include <mutex>
#include <thread>
#include <mfapi.h>
#include <mfidl.h>
#include <wrl.h>

using namespace Microsoft::WRL;

struct RTMP;

class RtmpStream : public IMFByteStream, public IMFAttributes, public IMFByteStreamBuffering {
public:
    explicit RtmpStream(std::wstring& rtmp_url);
    virtual ~RtmpStream();
    bool Open(IMFAttributes* config = NULL);

public: // IUnknown
    STDMETHODIMP_(ULONG) AddRef() {
        return InterlockedIncrement(&_ref_count);
    }
    STDMETHODIMP_(ULONG) Release()
    {
        auto rc = InterlockedDecrement(&_ref_count);
        if (rc == 0) delete this;
        return rc;
    }
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);

public: // IMFByteStream
    STDMETHODIMP GetCapabilities(DWORD *pdwCapabilities);
    STDMETHODIMP GetLength(QWORD *pqwLength) { return E_NOTIMPL; }
    STDMETHODIMP SetLength(QWORD qwLength) { return E_NOTIMPL; }
    STDMETHODIMP GetCurrentPosition(QWORD *pqwPosition);
    STDMETHODIMP SetCurrentPosition(QWORD qwPosition) {
        return Seek(msoBegin, (LONG64)qwPosition, 0, NULL);
    }
    STDMETHODIMP IsEndOfStream(BOOL *pfEndOfStream);

    STDMETHODIMP Read(BYTE *pb, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP BeginRead(BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState);
    STDMETHODIMP EndRead(IMFAsyncResult *pResult, ULONG *pcbRead);

    STDMETHODIMP Write(const BYTE *pb, ULONG cb, ULONG *pcbWritten) {
        return E_NOTIMPL;
    }
    STDMETHODIMP BeginWrite(const BYTE *pb, ULONG cb, IMFAsyncCallback *pCallback, IUnknown *punkState) {
        return E_NOTIMPL;
    }
    STDMETHODIMP EndWrite(IMFAsyncResult *pResult, ULONG *pcbWritten) {
        return E_NOTIMPL;
    }

    STDMETHODIMP Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD *pqwCurrentPosition);
    STDMETHODIMP Flush() { return S_OK; }
    STDMETHODIMP Close();

public: // IMFByteStreamBuffering
    STDMETHODIMP SetBufferingParams(MFBYTESTREAM_BUFFERING_PARAMS *pParams) { return S_OK; }
    STDMETHODIMP EnableBuffering(BOOL fEnable);
    STDMETHODIMP StopBuffering();

public: // IMFAttributes
    STDMETHODIMP GetItem(REFGUID guidKey, PROPVARIANT *pValue) {
        return _attrs->GetItem(guidKey, pValue);
    }
    STDMETHODIMP GetItemType(REFGUID guidKey, MF_ATTRIBUTE_TYPE *pType) {
        return _attrs->GetItemType(guidKey, pType);
    }
    STDMETHODIMP CompareItem(REFGUID guidKey, REFPROPVARIANT Value, BOOL *pbResult) {
        return _attrs->CompareItem(guidKey, Value, pbResult);
    }
    STDMETHODIMP Compare(IMFAttributes *pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL *pbResult) {
        return _attrs->Compare(pTheirs, MatchType, pbResult);
    }
    STDMETHODIMP GetUINT32(REFGUID guidKey, UINT32 *punValue) {
        return _attrs->GetUINT32(guidKey, punValue);
    }
    STDMETHODIMP GetUINT64(REFGUID guidKey, UINT64 *punValue) {
        return _attrs->GetUINT64(guidKey, punValue);
    }
    STDMETHODIMP GetDouble(REFGUID guidKey, double *pfValue) {
        return _attrs->GetDouble(guidKey, pfValue);
    }
    STDMETHODIMP GetGUID(REFGUID guidKey, GUID *pguidValue) {
        return _attrs->GetGUID(guidKey, pguidValue);
    }
    STDMETHODIMP GetStringLength(REFGUID guidKey, UINT32 *pcchLength) {
        return _attrs->GetStringLength(guidKey, pcchLength);
    }
    STDMETHODIMP GetString(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32 *pcchLength) {
        return _attrs->GetString(guidKey, pwszValue, cchBufSize, pcchLength);
    }
    STDMETHODIMP GetAllocatedString(REFGUID guidKey, LPWSTR *ppwszValue, UINT32 *pcchLength) {
        return _attrs->GetAllocatedString(guidKey, ppwszValue, pcchLength);
    }
    STDMETHODIMP GetBlobSize(REFGUID guidKey, UINT32 *pcbBlobSize) {
        return _attrs->GetBlobSize(guidKey, pcbBlobSize);
    }
    STDMETHODIMP GetBlob(REFGUID guidKey, UINT8 *pBuf, UINT32 cbBufSize, UINT32 *pcbBlobSize) {
        return _attrs->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize);
    }
    STDMETHODIMP GetAllocatedBlob(REFGUID guidKey, UINT8 **ppBuf, UINT32 *pcbSize) {
        return _attrs->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
    }
    STDMETHODIMP GetUnknown(REFGUID guidKey, REFIID riid, LPVOID *ppv) {
        return _attrs->GetUnknown(guidKey, riid, ppv);
    }
    STDMETHODIMP SetItem(REFGUID guidKey, REFPROPVARIANT Value) {
        return _attrs->SetItem(guidKey, Value);
    }
    STDMETHODIMP DeleteItem(REFGUID guidKey) {
        return _attrs->DeleteItem(guidKey);
    }
    STDMETHODIMP DeleteAllItems() {
        return _attrs->DeleteAllItems();
    }
    STDMETHODIMP SetUINT32(REFGUID guidKey, UINT32 unValue) {
        return _attrs->SetUINT32(guidKey, unValue);
    }
    STDMETHODIMP SetUINT64(REFGUID guidKey, UINT64 unValue) {
        return _attrs->SetUINT64(guidKey, unValue);
    }
    STDMETHODIMP SetDouble(REFGUID guidKey, double fValue) {
        return _attrs->SetDouble(guidKey, fValue);
    }
    STDMETHODIMP SetGUID(REFGUID guidKey, REFGUID guidValue) {
        return _attrs->SetGUID(guidKey, guidValue);
    }
    STDMETHODIMP SetString(REFGUID guidKey, LPCWSTR wszValue) {
        return _attrs->SetString(guidKey, wszValue);
    }
    STDMETHODIMP SetBlob(REFGUID guidKey, const UINT8 *pBuf, UINT32 cbBufSize) {
        return _attrs->SetBlob(guidKey, pBuf, cbBufSize);
    }
    STDMETHODIMP SetUnknown(REFGUID guidKey, IUnknown *pUnknown) {
        return _attrs->SetUnknown(guidKey, pUnknown);
    }
    STDMETHODIMP LockStore() {
        return _attrs->LockStore();
    }
    STDMETHODIMP UnlockStore() {
        return _attrs->UnlockStore();
    }
    STDMETHODIMP GetCount(UINT32 *pcItems) {
        return _attrs->GetCount(pcItems);
    }
    STDMETHODIMP GetItemByIndex(UINT32 unIndex, GUID *pguidKey, PROPVARIANT *pValue) {
        return _attrs->GetItemByIndex(unIndex, pguidKey, pValue);
    }
    STDMETHODIMP CopyAllItems(IMFAttributes *pDest) {
        return _attrs->CopyAllItems(pDest);
    }

private:
    void StopBackgroundRtmpThread();
    int ReadRtmp(uint8_t* buffer, size_t size, bool* is_header);
    void RtmpLoop(void* userData);
    void SetThreadName(DWORD thread_id, const char* thread_name);
private:
    uint32_t _ref_count;
    std::mutex _mutex;
    ComPtr<IMFAttributes> _attrs;

    std::string _url;
    RTMP* _rtmp;
    std::thread _thread;
    bool _rtmp_connected;

    HANDLE _event_async_read;
    HANDLE _event_async_exit;

    uint8_t _flv_header[9];
    uint64_t _received_length;
    bool _first_read;
    bool _request_seek;
    bool _is_eof;

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
    AsyncReadParameters _async_read_ps;
};