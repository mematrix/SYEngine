#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __WIN32_MACRO_TOOLS_H
#define __WIN32_MACRO_TOOLS_H

#ifndef _WIN32_MACRO_TOOLS
#define _WIN32_MACRO_TOOLS

#ifdef _MSC_VER
#define DLL_EXPORT __declspec(dllexport)
#define DLL_IMPORT __declspec(dllimport)

#define FORCEINLINE __forceinline
#define INLINE __inline
#endif

#define _HeapAlloc(dwFlags,dwBytes) HeapAlloc(GetProcessHeap(),(dwFlags),(dwBytes))
#define _HeapSize(lpMem) HeapSize(GetProcessHeap(),0,(lpMem))
#define _HeapFree(lpMem) HeapFree(GetProcessHeap(),0,(lpMem))

#endif //_WIN32_MACRO_TOOLS

#ifndef _HR_TOOLS
#define _HR_TOOLS

#define HR_FAILED_END(hr) if (FAILED(hr)) { return; }
#define HR_FAILED_RET(hr) if (FAILED(hr)) { return hr; }
#define HR_FAILED_BREAK(hr) if (FAILED(hr)) { break; }
#define HR_FAILED_CONTINUE(hr) if (FAILED(hr)) { continue; }
#define HR_FAILED_GOTO(hr,tag) if (FAILED(hr)) { goto tag; }

#endif //_HR_TOOLS

#ifndef _OTHER_TOOLS
#define _OTHER_TOOLS

#define OPEN_FLAG(f1,f2) (f1 | f2)
#define CLEAR_FLAG(f1,f2) (f1 ^ f2)
#define OPEN_FLAG_R(f1,f2) (f1 = (f1 | f2))
#define CLEAR_FLAG_R(f1,f2) (f1 = (f1 ^ f2))

#define _FOREVER_LOOP while(1)

#endif //_OTHER_TOOLS

#endif //__WIN32_MACRO_TOOLS_H