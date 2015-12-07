#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __AUTO_PROP_VAR_H
#define __AUTO_PROP_VAR_H

#ifndef _WINDOWS_
#include <Windows.h>
#endif

class AutoPropVar final
{
	PROPVARIANT _propvar;
	BOOL _clear;

public:
	AutoPropVar() throw() : _clear(TRUE)
	{
		PropVariantInit(&_propvar);
	}

	explicit AutoPropVar(const PROPVARIANT& propvar) throw() : _clear(TRUE)
	{
		PropVariantInit(&_propvar);
		PropVariantCopy(&_propvar,&propvar);
	}

	explicit AutoPropVar(const PROPVARIANT* propvar) throw() : _clear(FALSE)
	{
		_propvar = *propvar;
	}

	explicit AutoPropVar(PROPVARIANT* propvar) throw() : _clear(FALSE)
	{
		_propvar = *propvar;
	}

	AutoPropVar(const AutoPropVar& r) throw()
	{
		_clear = TRUE;
		PropVariantCopy(&_propvar,&r._propvar);
	}

	AutoPropVar(AutoPropVar&& r) throw()
	{
		_clear = r._clear;
		_propvar = r._propvar;
	}

	~AutoPropVar()
	{
		if (_clear)
			PropVariantClear(&_propvar);
	}

public:
	VARTYPE GetType() const throw()
	{
		return _propvar.vt;
	}

public:
	CHAR GetChar() const throw()
	{
		return _propvar.cVal;
	}

	UCHAR GetUChar() const throw()
	{
		return _propvar.bVal;
	}

	SHORT GetShort() const throw()
	{
		return _propvar.iVal;
	}

	USHORT GetUShort() const throw()
	{
		return _propvar.uiVal;
	}

	LONG GetInt32() const throw()
	{
		return _propvar.lVal;
	}

	ULONG GetUInt32() const throw()
	{
		return _propvar.ulVal;
	}

	LONG64 GetInt64() const throw()
	{
		return _propvar.hVal.QuadPart;
	}

	ULONG64 GetUInt64() const throw()
	{
		return _propvar.uhVal.QuadPart;
	}

	FLOAT GetFloat() const throw()
	{
		return _propvar.fltVal;
	}

	DOUBLE GetDouble() const throw()
	{
		return _propvar.dblVal;
	}

	DWORD GetStringA(LPSTR psz,DWORD cchSize) const throw()
	{
		int len = strlen(_propvar.pszVal);
		if (psz == nullptr ||
			(int)cchSize < len)
			return len;

		strcpy(psz,_propvar.pszVal);
		return len;
	}

	DWORD GetStringW(LPWSTR psz,DWORD cchSize) const throw()
	{
		int len = wcslen(_propvar.pwszVal);
		if (psz == nullptr ||
			(int)cchSize < len)
			return len;

		wcscpy(psz,_propvar.pwszVal);
		return len;
	}

	IUnknown* GetUnknown() const throw()
	{
		return _propvar.punkVal;
	}

	IStream* GetStream() const throw()
	{
		return _propvar.pStream;
	}

	DWORD GetBlob(PVOID pb,DWORD dwBufSize) const throw()
	{
		if (pb == nullptr ||
			dwBufSize < _propvar.blob.cbSize)
			return _propvar.blob.cbSize;

		memcpy(pb,_propvar.blob.pBlobData,_propvar.blob.cbSize);
		return _propvar.blob.cbSize;
	}

public:
	void SetChar(CHAR value) throw()
	{
		_propvar.vt = VT_I1;
		_propvar.cVal = value;
	}

	void SetUChar(UCHAR value) throw()
	{
		_propvar.vt = VT_UI1;
		_propvar.bVal = value;
	}

	void SetShort(SHORT value) throw()
	{
		_propvar.vt = VT_I2;
		_propvar.iVal = value;
	}

	void SetUShort(USHORT value) throw()
	{
		_propvar.vt = VT_UI2;
		_propvar.uiVal = value;
	}

	void SetInt32(LONG value) throw()
	{
		_propvar.vt = VT_I4;
		_propvar.lVal = value;
	}

	void SetUInt32(ULONG value) throw()
	{
		_propvar.vt = VT_UI4;
		_propvar.ulVal = value;
	}

	void SetInt64(LONG64 value) throw()
	{
		_propvar.vt = VT_I8;
		_propvar.hVal.QuadPart = value;
	}

	void SetUInt64(ULONG64 value) throw()
	{
		_propvar.vt = VT_UI8;
		_propvar.uhVal.QuadPart = value;
	}

	void SetFloat(FLOAT value) throw()
	{
		_propvar.vt = VT_R4;
		_propvar.fltVal = value;
	}

	void SetDouble(DOUBLE value) throw()
	{
		_propvar.vt = VT_R8;
		_propvar.dblVal = value;
	}

	void SetStringA(LPCSTR psz) throw()
	{
		if (_propvar.pszVal && _propvar.vt == VT_LPSTR)
			CoTaskMemFree(_propvar.pszVal);

		_propvar.vt = VT_LPSTR;
		_propvar.pszVal = (LPSTR)CoTaskMemAlloc(strlen(psz) + 2);
		strcpy(_propvar.pszVal,psz);
	}

	void SetStringW(LPCWSTR psz) throw()
	{
		if (_propvar.pwszVal && _propvar.vt == VT_LPWSTR)
			CoTaskMemFree(_propvar.pwszVal);

		_propvar.vt = VT_LPWSTR;
		_propvar.pwszVal = (LPWSTR)CoTaskMemAlloc((wcslen(psz) + 2) * 2);
		wcscpy(_propvar.pwszVal,psz);
	}

	void SetUnknown(IUnknown* punk) throw()
	{
		if (_propvar.punkVal && _propvar.vt == VT_UNKNOWN)
			_propvar.punkVal->Release();

		_propvar.vt = VT_UNKNOWN;
		_propvar.punkVal = punk;
		punk->AddRef();
	}

	void SetStream(IStream* pstm) throw()
	{
		if (_propvar.pStream && _propvar.vt == VT_STREAM)
			_propvar.pStream->Release();

		_propvar.vt = VT_STREAM;
		_propvar.pStream = pstm;
		pstm->AddRef();
	}

	void SetBlob(PVOID pb,DWORD dwBufSize) throw()
	{
		if (_propvar.blob.pBlobData && _propvar.vt == VT_BLOB)
			CoTaskMemFree(_propvar.blob.pBlobData);

		_propvar.vt = VT_BLOB;
		_propvar.blob.pBlobData = (PBYTE)CoTaskMemAlloc(dwBufSize + 1);
		_propvar.blob.cbSize = dwBufSize;
		memcpy(_propvar.blob.pBlobData,pb,dwBufSize);
	}

public:
	AutoPropVar& operator=(const AutoPropVar&) = delete;
	AutoPropVar& operator=(AutoPropVar&&) = delete;
};

#endif //__AUTO_PROP_VAR_H