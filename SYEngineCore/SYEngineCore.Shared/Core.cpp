#include "Core.h"

#define _STM_HANDLER_CLSID L"{1A0DFC9E-009C-4266-ADFF-CA37D7F8E450}"
#define _URL_HANDLER_CLSID L"{2A0DFC9E-009C-4266-ADFF-CA37D7F8E450}"

#define _STM_HANDLER_NAME L"CoreMFSource.HDCoreByteStreamHandler"
#define _URL_HANDLER_NAME L"MultipartStreamMatroska.UrlHandler"

#define _STM_HANDLER_FILE L"CoreMFSource.dll"
#define _URL_HANDLER_FILE L"MultipartStreamMatroska.dll"

using namespace SYEngineCore;

bool Core::Initialize()
{
	if (Installer != nullptr)
		return false;

	Installer = new(std::nothrow) MediaExtensionInstaller();
	if (Installer == nullptr)
		return false;

	if (!Installer->InstallSchemeHandler(_URL_HANDLER_CLSID,
		_URL_HANDLER_NAME,
		_URL_HANDLER_FILE,
		L"plist:"))
		return false;

	struct ByteStreamHandlerPair
	{
		LPCWSTR FileExtension;
		LPCWSTR MimeType;
	};
	static ByteStreamHandlerPair Handlers[] = {
		{NULL, L"video/force-network-stream"},
		{L".mkv", L"video/x-matroska"},
		{L".flv", L"video/x-flv"},
		{L".f4v", NULL}
	};
	for (auto h : Handlers)
		Installer->InstallByteStreamHandler(_STM_HANDLER_CLSID,
		_STM_HANDLER_NAME,
		_STM_HANDLER_FILE,
		h.FileExtension,
		h.MimeType);

	return true;
}

void Core::Uninitialize()
{
	if (Installer)
		delete Installer;
	Installer = nullptr;
}