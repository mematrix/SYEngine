#include "Core.h"

#define _STM_HANDLER_NAME L"CoreMFSource.HDCoreByteStreamHandler"
#define _URL_HANDLER_NAME L"MultipartStreamMatroska.UrlHandler"

using namespace SYEngineCore;

void Core::Instance()
{
	if (_pMediaExtensionManager != nullptr)
		return;

	{
		auto p = ref new Windows::Media::MediaExtensionManager();
		_pMediaExtensionManager = reinterpret_cast<decltype(_pMediaExtensionManager.Get())>(p);
	}

	_pMediaExtensionManager->RegisterSchemeHandler(
		HStringReference(_URL_HANDLER_NAME).Get(),
		HStringReference(L"plist:").Get());

	_pMediaExtensionManager->RegisterByteStreamHandler(
		HStringReference(_STM_HANDLER_NAME).Get(),
		HStringReference(L".mka").Get(), HStringReference(L"video/force-network-stream").Get());

	_pMediaExtensionManager->RegisterByteStreamHandler(
		HStringReference(_STM_HANDLER_NAME).Get(),
		HStringReference(L".flv").Get(), HStringReference(L"video/x-flv").Get());
	_pMediaExtensionManager->RegisterByteStreamHandler(
		HStringReference(_STM_HANDLER_NAME).Get(),
		HStringReference(L".mkv").Get(), HStringReference(L"video/x-matroska").Get());
}