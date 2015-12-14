#pragma once

#include "pch.h"
#include <windows.media.h>

namespace SYEngineCore
{
	static ComPtr<ABI::Windows::Media::IMediaExtensionManager> _pMediaExtensionManager;

	public ref class Core sealed
	{
	public:
		static void Initialize();
		
	private:
		Core() { }
	};
}