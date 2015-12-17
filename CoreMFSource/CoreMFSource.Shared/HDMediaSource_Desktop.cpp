#include "HDMediaSource.h"

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#define WIN8_CHECK_COND "GetPackageId"
bool HDMediaSource::IsWindows8()
{
	return GetProcAddress(
		GetModuleHandleA("KernelBase.dll"),WIN8_CHECK_COND) != nullptr;
}

static unsigned SkipH264AudOffset(unsigned char* pb,unsigned len)
{
	if (pb[0] != 0 ||
		pb[1] != 0 ||
		pb[2] != 0 ||
		pb[3] != 0 ||
		pb[4] != 0x09)
		return 0;

	return 6;
}

HRESULT HDMediaSource::ProcessMPEG2TSToMP4Sample(IMFSample* pH264ES,IMFSample** ppH264)
{
	ComPtr<IMFMediaBuffer> pOldBuffer;
	HRESULT hr = pH264ES->GetBufferByIndex(0,pOldBuffer.GetAddressOf());
	HR_FAILED_RET(hr);

	WMF::AutoBufLock lock_old(pOldBuffer.Get());
	if (lock_old.GetPtr() == nullptr)
		return E_UNEXPECTED;

	DWORD dwOldBufSize = 0;
	pOldBuffer->GetCurrentLength(&dwOldBufSize);
	if (dwOldBufSize == 0)
		return E_UNEXPECTED;

	unsigned offset = SkipH264AudOffset(lock_old.GetPtr(),dwOldBufSize);
	if (offset == 0 ||
		offset >= dwOldBufSize)
		return E_NOT_SET;

	ComPtr<IMFSample> pNewSample;
	hr = WMF::Misc::CreateSampleAndCopyData(pNewSample.GetAddressOf(),
		lock_old.GetPtr() + offset,dwOldBufSize - offset,-1,0);

	if (SUCCEEDED(hr))
		*ppH264 = pNewSample.Detach();

	return hr;
}

#endif