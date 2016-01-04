#include "pch.h"
#include "MediaExtensionInstaller.h"

#define _STM_HANDLER_CLSID L"{1A0DFC9E-009C-4266-ADFF-CA37D7F8E450}"
#define _URL_HANDLER_CLSID L"{2A0DFC9E-009C-4266-ADFF-CA37D7F8E450}"

#define _STM_HANDLER_FILE L"CoreMFSource.dll"
#define _URL_HANDLER_FILE L"MultipartStreamMatroska.dll"

void InitSystemUIStyle()
{
	InitCommonControls();

	ACTCTXW ctx = {};
	ctx.cbSize = sizeof(ctx);
	ctx.wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
	ctx.dwFlags = ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID|ACTCTX_FLAG_HMODULE_VALID|ACTCTX_FLAG_RESOURCE_NAME_VALID;
	ctx.hModule = LoadLibraryExA("notepad.exe", NULL, DONT_RESOLVE_DLL_REFERENCES);
	ctx.lpResourceName = MAKEINTRESOURCEW(1);
	ULONG_PTR ctxCookie = 0;
	HANDLE hCtx = CreateActCtxW(&ctx);
	ActivateActCtx(hCtx, &ctxCookie);
}

void InitProcessProfile()
{
	SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
	SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);
	SetCurrentProcessExplicitAppUserModelID(L"Microsoft.Windows.MediaPlayer32");
}

void StartWindowsMediaPlayer()
{
	typedef void (WINAPI* WinMain)(HMODULE,LPWSTR,int);
	auto mod = LoadLibraryA("wmp.dll");
	if (mod) {
		auto callback = (WinMain)GetProcAddress(mod, MAKEINTRESOURCEA(3000));
		if (callback)
			callback(GetModuleHandleA(NULL), GetCommandLineW(), SW_SHOWDEFAULT);
	}
}

void InstallMediaExtensions(MediaExtensionInstaller* Installer)
{
	struct ByteStreamHandlerPair
	{
		LPCWSTR FileExtension;
		LPCWSTR MimeType;
	};
	static ByteStreamHandlerPair Handlers[] = {
		{L".mp4", L"video/mp4"},
		{L".mkv", L"video/x-matroska"},
		{L".flv", L"video/x-flv"},
		{L".f4v", NULL}
	};
	for (auto h : Handlers)
		Installer->InstallByteStreamHandler(_STM_HANDLER_CLSID,
		NULL, _STM_HANDLER_FILE,
		h.FileExtension,
		h.MimeType);

	Installer->InstallSchemeHandler(_URL_HANDLER_CLSID,
		NULL, _URL_HANDLER_FILE,
		L"plist:");
}

int main()
{
	InitSystemUIStyle();
	InitProcessProfile();

	CoInitialize(NULL);
	CoInitializeSecurity(NULL, -1, NULL, NULL, 0, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);

	WSADATA wsa;
	WSAStartup(MAKEWORD(2,2), &wsa);

	auto Installer = new(std::nothrow) MediaExtensionInstaller();
	InstallMediaExtensions(Installer);
	StartWindowsMediaPlayer();
	delete Installer;

	WSACleanup();
	CoUninitialize();
	return 0;
}