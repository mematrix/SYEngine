#pragma once

#include "pch.h"
#include "MediaExtensionActivate.h"

class MediaExtensionInstaller sealed
{
	ComPtr<IMFCollection> _objs;

public:
	MediaExtensionInstaller() { MFStartup(MF_VERSION); MFCreateCollection(&_objs); }
	~MediaExtensionInstaller() { _objs->RemoveAllElements(); MFShutdown(); }

public:
	bool InstallSchemeHandler(LPCWSTR clsid, LPCWSTR activatableClassId, LPCWSTR dllFile, LPCWSTR schemeType);
	bool InstallByteStreamHandler(LPCWSTR clsid, LPCWSTR activatableClassId, LPCWSTR dllFile, LPCWSTR fileExt, LPCWSTR mimeType);
};