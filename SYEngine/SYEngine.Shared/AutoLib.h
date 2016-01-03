#include <Windows.h>

class AutoLibrary
{
	HMODULE _mod;

public:
	explicit AutoLibrary(HMODULE hMod) throw() { _mod = hMod; }
	explicit AutoLibrary(const char* file_name) throw()
	{
		MEMORY_BASIC_INFORMATION mbi = {};
		VirtualQuery(&VirtualQuery, &mbi, sizeof(mbi));

		AutoLibrary kernel32((HMODULE)mbi.AllocationBase);
		_mod = kernel32.GetProcAddr<HMODULE (WINAPI*)(LPCSTR,HANDLE,DWORD)>
			("LoadLibraryExA")(file_name, NULL, 0);
	}
	~AutoLibrary() throw()
	{ if (_mod) FreeLibrary(_mod); }

	inline HMODULE GetModule() const throw()
	{ return _mod; }
	template<typename Return = FARPROC>
	inline Return GetProcAddr(const char* name) const throw()
	{ return (Return)GetProcAddress(_mod, name); }
};