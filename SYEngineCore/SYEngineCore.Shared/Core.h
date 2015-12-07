#pragma once

#include "pch.h"
#include <windows.media.h>

namespace SYEngineCore
{
	public ref class Core sealed
	{
	public:
		Core() {}

	public:
		void Instance();

	private:
		ComPtr<ABI::Windows::Media::IMediaExtensionManager> _pMediaExtensionManager;
	};
}