#include <Windows.h>

class AutoLibrary
{
	HMODULE _mod;

public:
	AutoLibrary() noexcept // -> KernelBase.dll
	{
		MEMORY_BASIC_INFORMATION mbi = {};
		VirtualQuery(&VirtualQuery, &mbi, sizeof(mbi));
		_mod = (HMODULE)mbi.AllocationBase;
	}
	explicit AutoLibrary(HMODULE hMod) noexcept { _mod = hMod; }
	explicit AutoLibrary(const char* file_name) noexcept
    {
		MEMORY_BASIC_INFORMATION mbi = {};
		VirtualQuery(&VirtualQuery, &mbi, sizeof(mbi));

		AutoLibrary kernel32((HMODULE)mbi.AllocationBase);
		_mod = kernel32.GetProcAddr<HMODULE (WINAPI*)(LPCSTR,HANDLE,DWORD)>
			("LoadLibraryExA")(file_name, NULL, 0);
	}
	~AutoLibrary() noexcept
    { if (_mod) FreeLibrary(_mod); }

	inline HMODULE GetModule() const noexcept
    { return _mod; }
	template<typename Return = FARPROC>
	inline Return GetProcAddr(const char* name) const noexcept
    { return (Return)GetProcAddress(_mod, name); }
};