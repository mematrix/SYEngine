#include "RtmpUrlHandler.h"
#include "RtmpStream.h"

#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#include <windows.storage.h>
#endif

#if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
ActivatableClass(RtmpUrlHandler);
#endif
CoCreatableClass(RtmpUrlHandler);


HRESULT RtmpUrlHandler::BeginCreateObject(
    LPCWSTR pwszURL,
    DWORD dwFlags, IPropertyStore *pProps,
    IUnknown **ppIUnknownCancelCookie,
    IMFAsyncCallback *pCallback, IUnknown *punkState) {

    if (pwszURL == NULL || pCallback == NULL)
        return E_INVALIDARG;
    if (ppIUnknownCancelCookie)
        *ppIUnknownCancelCookie = NULL;

    if (wcslen(pwszURL) < 10)
        return E_INVALIDARG;
    if (_wcsnicmp(pwszURL, L"rtmp://", 7) != 0)
        return E_INVALIDARG;

    if (pwszURL) {
        size_t len = wcslen(pwszURL);
        _rtmp_url.reserve(len + 1);
        wcscpy(&_rtmp_url[0], pwszURL);
    }

    std::lock_guard<decltype(_mutex)> lock(_mutex);
    if (_result != nullptr)
        return E_FAIL;

    auto hr = MFCreateAsyncResult(NULL, pCallback, punkState, &_result);
    if (FAILED(hr))
        return hr;

    return ThreadStart(nullptr) ? S_OK : E_UNEXPECTED;
}

HRESULT RtmpUrlHandler::EndCreateObject(
    IMFAsyncResult *pResult,
    MF_OBJECT_TYPE *pObjectType, IUnknown **ppObject) {

    if (pResult == NULL ||
        pObjectType == NULL ||
        ppObject == NULL)
        return E_INVALIDARG;

    std::lock_guard<decltype(_mutex)> lock(_mutex);
    if (_result == nullptr || _stream == nullptr)
        return E_FAIL;
    if (FAILED(_result->GetStatus()))
        return _result->GetStatus();

    *pObjectType = MF_OBJECT_BYTESTREAM;
    *ppObject = (IUnknown*)_stream.Get();
    (*ppObject)->AddRef();

    _stream.Reset();
    _result.Reset();
    return S_OK;
}

void RtmpUrlHandler::ThreadInvoke(void*)
{
    AddRef();
    _result->SetStatus(S_OK);
    {
        std::lock_guard<decltype(_mutex)> lock(_mutex);
        auto s = new(std::nothrow) RtmpStream(_rtmp_url);
        if (s == NULL) {
            _result->SetStatus(E_OUTOFMEMORY);
        } else {
            if (s->Open(this)) {
                _stream.Attach(static_cast<IMFByteStream*>(s));
            } else {
                _result->SetStatus(E_FAIL);
                s->Release();
            }
        }
    }
    MFInvokeCallback(_result.Get());
}