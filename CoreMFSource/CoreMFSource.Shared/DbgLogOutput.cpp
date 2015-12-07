#include <stdio.h>
#include <Windows.h>

void DbgLogPrintf(const wchar_t* str,...)
{
#ifdef _DEBUG
	if (str == nullptr)
		return;

	va_list args;
	va_start(args,str);

	WCHAR sz[1024] = {};
	_vsnwprintf_s(sz,ARRAYSIZE(sz),str,args);
	wcscat(sz,L"\n");
	OutputDebugStringW(sz);

	va_end(args);
#endif
}