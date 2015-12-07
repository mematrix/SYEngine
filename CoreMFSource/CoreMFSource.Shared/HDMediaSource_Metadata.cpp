#include "HDMediaSource.h"
#include <shared_memory_ptr.h>

#define METADATA_VALUE_COUNT 10

struct MetadataValues
{
	enum VALUE_TYPE
	{
		ValueType_Unknown,
		ValueType_Copyright,
		ValueType_Comment,
		ValueType_Title,
		ValueType_Author,
		ValueType_Album,
		ValueType_Genre,
		ValueType_Year,
		ValueType_TrackNumber,
		ValueType_Composer,
		ValueType_Picture
	};
	VALUE_TYPE ValueType;
	LPCWSTR Name;
};

static const MetadataValues kMetadataValues[METADATA_VALUE_COUNT] = {
	{MetadataValues::ValueType_Copyright,L"Copyright"},
	{MetadataValues::ValueType_Comment,L"Description"},
	{MetadataValues::ValueType_Title,L"Title"},
	{MetadataValues::ValueType_Author,L"Author"},
	{MetadataValues::ValueType_Album,L"WM/AlbumTitle"},
	{MetadataValues::ValueType_Genre,L"WM/Genre"},
	{MetadataValues::ValueType_Year,L"WM/Year"},
	{MetadataValues::ValueType_TrackNumber,L"WM/TrackNumber"},
	{MetadataValues::ValueType_Composer,L"WM/Composer"},
	{MetadataValues::ValueType_Picture,L"WM/Picture"}};

static LPWSTR CoStrDup(LPCWSTR str)
{
	if (str == nullptr)
		return nullptr;
	auto p = (LPWSTR)CoTaskMemAlloc(wcslen(str) * 2 + 4);
	wcscpy(p,str);
	return p;
}

static HRESULT MyInitPropVariantFromString(PCWSTR psz,PROPVARIANT *ppropvar)
{
	if (psz == nullptr || ppropvar == nullptr)
		return E_INVALIDARG;

	PropVariantInit(ppropvar);
	ppropvar->vt = VT_LPWSTR;
	ppropvar->pwszVal = CoStrDup(psz);

	return S_OK;
}

HRESULT HDMediaSource::GetMFMetadata(IMFPresentationDescriptor* pPresentationDescriptor,DWORD dwStreamIdentifier,DWORD dwFlags,IMFMetadata** ppMFMetadata)
{
	if (pPresentationDescriptor == nullptr)
		return E_INVALIDARG;
	if (ppMFMetadata == nullptr)
		return E_POINTER;

	DWORD dwStreamCount0 = 0,dwStreamCount1 = 0;
	pPresentationDescriptor->GetStreamDescriptorCount(&dwStreamCount0);
	_pPresentationDescriptor->GetStreamDescriptorCount(&dwStreamCount1);
	if (dwStreamCount0 != dwStreamCount1)
		return E_INVALIDARG;

	if (dwStreamCount0 != 1)
		return MF_E_PROPERTY_NOT_FOUND;
	ComPtr<IMFMediaType> pMediaType;
	HRESULT hr = WMF::Misc::GetMediaTypeFromPD(pPresentationDescriptor,0,pMediaType.GetAddressOf());
	if (FAILED(hr))
		return hr;
	if (FAILED(WMF::Misc::IsAudioMediaType(pMediaType.Get())))
		return MF_E_PROPERTY_NOT_FOUND;

	if (_pMetadata == nullptr)
		return MF_E_PROPERTY_NOT_FOUND;

	return QueryInterface(IID_PPV_ARGS(ppMFMetadata));
}

HRESULT HDMediaSource::GetAllPropertyNames(PROPVARIANT* ppvNames)
{
	PropVariantInit(ppvNames);
	ppvNames->vt = VT_EMPTY;
	if (_pMetadata->GetValueCount() == 0)
		return S_OK;

	ppvNames->vt = VT_VECTOR|VT_LPWSTR;
	ppvNames->calpwstr.cElems = METADATA_VALUE_COUNT;
	ppvNames->calpwstr.pElems = (decltype(ppvNames->calpwstr.pElems))
		CoTaskMemAlloc(sizeof(LPWSTR) * (METADATA_VALUE_COUNT + 1));

	auto p = ppvNames->calpwstr.pElems;
	for (unsigned i = 0;i < _countof(kMetadataValues);i++)
		p[i] = CoStrDup(kMetadataValues[i].Name);

	return S_OK;
}

HRESULT HDMediaSource::GetProperty(LPCWSTR pwszName,PROPVARIANT* ppvValue)
{
	if (pwszName == nullptr || ppvValue == nullptr)
		return E_INVALIDARG;
	if (wcslen(pwszName) == 0)
		return E_INVALIDARG;

	MetadataValues::VALUE_TYPE type = MetadataValues::ValueType_Unknown;
	for (unsigned i = 0;i < _countof(kMetadataValues);i++)
	{
		if (_wcsicmp(pwszName,kMetadataValues[i].Name) == 0)
		{
			type = kMetadataValues[i].ValueType;
			break;
		}
	}
	if (type == MetadataValues::ValueType_Unknown)
		return MF_E_PROPERTY_NOT_FOUND;

	DWORD dwCodePage = CP_UTF8;
	if (_pMetadata->GetStrType() == AVMetadataStringType::MetadataStrType_ANSI)
		dwCodePage = CP_ACP;
	else if (_pMetadata->GetStrType() == AVMetadataStringType::MetadataStrType_Unicode)
		dwCodePage = 0xFF;
	
	const char* name = nullptr;
	unsigned value_size = 0;

	switch (type)
	{
	case MetadataValues::ValueType_Title:
		name = AV_METADATA_TYPE_TITLE;
		break;
	case MetadataValues::ValueType_Author:
		name = AV_METADATA_TYPE_ARTIST;
		break;
	case MetadataValues::ValueType_Album:
		name = AV_METADATA_TYPE_ALBUM;
		break;
	case MetadataValues::ValueType_Genre:
		name = AV_METADATA_TYPE_GENRE;
		break;
	case MetadataValues::ValueType_Year:
		name = AV_METADATA_TYPE_DATE;
		break;
	case MetadataValues::ValueType_TrackNumber:
		name = AV_METADATA_TYPE_NUMBER;
		break;
	case MetadataValues::ValueType_Composer:
		name = AV_METADATA_TYPE_COMPOSER;
		break;
	case MetadataValues::ValueType_Copyright:
	case MetadataValues::ValueType_Comment:
		name = AV_METADATA_TYPE_COMMENT;
		break;
	}

	if (name == nullptr && type != MetadataValues::ValueType_Picture)
		return MF_E_PROPERTY_NOT_FOUND;

	if (name != nullptr)
	{
		value_size = _pMetadata->GetValue(name,nullptr);
		if (value_size == 0)
			return MF_E_PROPERTY_NOT_FOUND;

		shared_memory_ptr<char> old(value_size + 10);
		RtlZeroMemory(old.ptr(),value_size + 10);
		_pMetadata->GetValue(name,old.ptr());

		if (dwCodePage != 0xFF)
		{
			auto count = MultiByteToWideChar(dwCodePage,0,old.ptr(),-1,nullptr,0);
			if (count == 0)
				return MF_E_PROPERTY_NOT_FOUND;

			shared_memory_ptr<WCHAR> str(count * 2 + 4);
			RtlZeroMemory(str.ptr(),count * 2 + 4);
			MultiByteToWideChar(dwCodePage,0,old.ptr(),-1,str.ptr(),count + 1);

			return MyInitPropVariantFromString(str.ptr(),ppvValue);
		}else{
			return MyInitPropVariantFromString((PCWSTR)old.ptr(),ppvValue);
		}
	}

	if (type == MetadataValues::ValueType_Picture)
	{
		if (_pMetadata->GetCoverImage(nullptr) == 0)
			return MF_E_PROPERTY_NOT_FOUND;

		const wchar_t* mimeType = L"image/bmp";
		switch (_pMetadata->GetCoverImageType())
		{
		case MetadataImageType_JPG:
			mimeType = L"image/jpeg";
			break;
		case MetadataImageType_PNG:
			mimeType = L"image/png";
			break;
		case MetadataImageType_GIF:
			mimeType = L"image/gif";
			break;
		case MetadataImageType_TIFF:
			mimeType = L"image/tiff";
			break;
		}

		ppvValue->vt = VT_BLOB;
		ppvValue->blob.cbSize = 5 + 4 +
			wcslen(mimeType) * 2 + 2 + _pMetadata->GetCoverImage(nullptr);

		auto p = ppvValue->blob.pBlobData = (PBYTE)CoTaskMemAlloc(ppvValue->blob.cbSize);
		if (p == nullptr)
			return E_OUTOFMEMORY;
		RtlZeroMemory(p,ppvValue->blob.cbSize);

		*p = 3;
		p += 1;
		*(PDWORD)p = _pMetadata->GetCoverImage(nullptr);
		p += 4;
		wcscpy((PWCHAR)p,mimeType);
		p += wcslen(mimeType) * 2 + 2;
		*(PDWORD)p = 0x65;
		p += 4;

		_pMetadata->GetCoverImage(p);
	}

	return S_OK;
}